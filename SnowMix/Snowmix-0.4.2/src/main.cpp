
/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Scott            pscott@mail.dk
 *		 Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "snowmix.h"
#include "video_mixer.h"
#include "video_scaler.h"
#include "video_text.h"
#include "monitor.h"
#include "video_image.h"

#include <glib.h>
#include <gtk/gtk.h>

/* System wide globals. */
int		exit_program		= 0;

bool set_fd_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ? true : false);
}


/********************************************************************************
*										*
*	Stuff for handling gstreamer shmsink/shmsrc data.			*
*										*
********************************************************************************/

//struct area_list_type;

/* Find a area_id element for a feed. */
static struct area_list_type *find_area (struct feed_type *feed, int area_id)
{
	struct area_list_type	*area;
       
	for (area = feed->area_list; area != NULL; area = area->next) {
		if (area->area_id != area_id) continue;

		return area;	/* Found it. */
	}
	return NULL;		/* Not found. */
}


/* Locate a buffer control struct. */
static struct buffer_type *find_buffer_by_area (struct area_list_type *area, unsigned long offset)
{
	struct buffer_type 	*buffer;

	for (buffer = area->buffers; buffer != NULL; buffer = buffer->next) {
		if (buffer->offset != offset) continue;

		return buffer;	/* Found it. */
	}
	return NULL;		/* Not found. */
}



// static void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len)
void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len)
{
	printf ("%s => %s %d bytes: ", src, dst, (int)len);
	switch (cb->type) {
		case 1:	{	/* COMMAND_NEW_SHM_AREA. */
			unsigned int	x;
			char	*c = ((char *)cb) + sizeof (*cb);

			printf ("New SHM Area. ID: %d size: %d  name_len: %d  name: ",
				cb->area_id,
				(int)cb->payload.new_shm_area.size,
				cb->payload.new_shm_area.path_size);
			for (x = 0; x < cb->payload.new_shm_area.path_size; x++, c++) {
				printf ("%c", *c);
			}
			printf ("\n");
			break;
		}

		case 2:		/* COMMAND_CLOSE_SHM_AREA. */
			printf ("Close SHM Area. ID: %d\n", cb->area_id);
			break;

		case 3:		/* COMMAND_NEW_BUFFER. */
			printf ("New Buffer. ID: %d  block: %4lu  offset: %9lu  size: %9lu\n",
				cb->area_id,
				cb->payload.buffer.offset/block_size,
				cb->payload.buffer.offset,
				cb->payload.buffer.bsize);
			break;

		case 4:		/* COMMAND_ACK_BUFFER. */
			printf ("ACK Buffer. ID: %d  block: %4lu  offset: %9lu\n",
				cb->area_id,
				cb->payload.ack_buffer.offset/block_size,
				cb->payload.ack_buffer.offset);
			break;

		default:
			printf ("Unknown packet. Code: %d\n", cb->type);
			break;
	}
}

/** Transmit an ACK message to a feed. */
static void transmit_ack (u_int32_t block_size, struct feed_type *feed, int area_id, unsigned long offset)
{
	struct shm_commandbuf_type	cb;

	memset (&cb, 0, sizeof (cb));
	cb.type = 4;
	cb.area_id = area_id;
	cb.payload.ack_buffer.offset = offset;

	if (send (feed->control_socket, &cb, sizeof (cb), MSG_NOSIGNAL) > 0) {
		if (feed->id == 9) {
		dump_cb (block_size, "Proxy", feed->control_socket_name, &cb, sizeof (cb));
		}
	} else {
		/* Feed disappeared. */
		perror ("send to feed");
		//FIXME: Cleanup.
	}
}



/* Bump up the use count on this buffer reference. */
static struct buffer_type *feed_use_buffer (struct feed_type *feed, int area_id, unsigned long offset, unsigned long bsize)
{
	struct area_list_type	*area = find_area (feed, area_id);
	struct buffer_type	*buffer;

	if (area == NULL) {
		fprintf (stderr, "Area %d Unknown\n", area_id);
		return NULL;	/* Onknown area. */
	}
       
	buffer = find_buffer_by_area (area, offset);
	if (buffer == NULL) {
		/* Not seen before. Create a new descriptor entry. */
		buffer = (buffer_type*) calloc (1, sizeof (*buffer));
		buffer->area_list = area;
		buffer->offset    = offset;
		buffer->bsize     = bsize;
		buffer->use       = 1;

		list_add_head (area->buffers, buffer);
	} else {
		/* Already pending. Just bump the use counter. */
		buffer->use++;
	}

	return buffer;
}


/* Increment the use count on this buffer pointer. */
static void feed_use_buffer_ptr (struct buffer_type *buffer)
{
	if (buffer == NULL) return;	/* Nothing to be done. */

	buffer->use++;
}


/* Decrement the use count on this buffer pointer. */
static void feed_ack_buffer_ptr (u_int32_t block_size, struct buffer_type *buffer)
{
	if (buffer == NULL) return;	/* Nothing to be done. */

	buffer->use--;
	if (buffer->use == 0) {
		/* No longer in use. Free it back on the master. */
		struct area_list_type	*area = buffer->area_list;
		struct feed_type	*feed = area->feed;

		transmit_ack (block_size, feed, area->area_id, buffer->offset);

		/* Remove from memory list. */
		list_del_element (area->buffers, buffer);
		free (buffer);
	}
}


/* Release all buffers assigned to a feed. */
static void feed_release_buffers (struct feed_type *feed)
{
	struct area_list_type	*area, *next_area;

	for (area = feed->area_list, next_area = area ? area->next : NULL;
	     area != NULL;
	     area = next_area, next_area = area ? area->next : NULL) {
		struct buffer_type	*buffer, *next_buffer;

		/* Delete all buffers associated with this area. */
		for (buffer = area->buffers, next_buffer = buffer ? buffer->next : NULL;
		     buffer != NULL;
		     buffer = next_buffer, next_buffer = buffer ? buffer->next : NULL) {
			list_del_element (area->buffers, buffer);
			free (buffer);
		}

		list_del_element (feed->area_list, area);
		if (area->buffer_base != NULL) {
			/* Unmap the memory segment. */
//fprintf(stderr, "Feed Releas Buffer unmap %s PMM\n", area->area_handle_name);
			munmap (area->buffer_base, area->area_size);
if (shm_unlink(area->area_handle_name)) {
  perror("shm_unlink error : ");
}
			close (area->area_handle);
		}
		free (area->area_handle_name);
		free (area);
	}

	/* Clear the fifo structs. */
	memset (&feed->frame_fifo, 0, sizeof (feed->frame_fifo));
	feed->fifo_depth = 0;
}



/* Populate rfds and wrfd for the feed subsystem. */
static int feed_set_fds (CVideoMixer* pVideoMixer, int maxfd, fd_set *rfds, fd_set *wfds)
{
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_fds\n");
		return -1;
	}
	if (!pVideoMixer->m_pController) {
		fprintf(stderr, "pController was NULL in feed_set_fds\n");
		return -1;
	}
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL in feed_set_fds\n");
		return -1;
	}
	struct feed_type* feed = pVideoMixer->m_pVideoFeed->GetFeedList();
	CController* pController = pVideoMixer->m_pController;


	

	for ( ; feed != NULL; feed = feed->next) {
		if (feed->control_socket < 0 && feed->control_socket_name != NULL) {
			/* No socket yet. Try to create it. */
			feed->control_socket = socket (AF_UNIX, SOCK_STREAM, 0);
			if (feed->control_socket >= 0) {
				/* Bind it to the socket. */
				struct sockaddr_un	addr;

				memset (&addr, 0, sizeof (addr));
				addr.sun_family = AF_UNIX;
				strncpy (addr.sun_path, feed->control_socket_name, sizeof (addr.sun_path) - 1);
				if (connect (feed->control_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
					if (feed->connect_retries == 0) {
						pController->controller_write_msg (NULL, "Feed %d \"%s\" unable to connect: %s\n",
								      feed->id, feed->feed_name ? feed->feed_name : "",
								      strerror (errno));
					}
					close (feed->control_socket);
					feed->control_socket = -1;
					feed->connect_retries++;
				} else {
					pController->controller_write_msg (NULL, "Feed %d \"%s\" connected\n",
							      feed->id, feed->feed_name ? feed->feed_name : "");
					feed->connect_retries = 0;
				}
			}
		}

		if (feed->control_socket >= 0 && (feed->is_live || feed->fifo_depth < feed->max_fifo)) {
			/* Add its descriptor to the bitmap. */
			FD_SET (feed->control_socket, rfds);
			if (feed->control_socket > maxfd) {
				maxfd = feed->control_socket;
			}
		}
	}

	return maxfd;
}



/* Check for incomming data on a feed. */
static void feed_check_read (CVideoMixer* pVideoMixer, fd_set *rfds, u_int32_t block_size)
{
	struct feed_type		*feed, *next_feed;
	int				x;
	struct shm_commandbuf_type	cb;

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_fds\n");
		return;
	}
	if (!pVideoMixer->m_pController) {
		fprintf(stderr, "pController was NULL in feed_set_fds\n");
		return;
	}
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL in feed_check_read\n");
		return;
	}
	feed = pVideoMixer->m_pVideoFeed->GetFeedList();
	CController* pController = pVideoMixer->m_pController;

	for (next_feed = feed ? feed->next : NULL; feed != NULL;
	     feed = next_feed, next_feed = feed ? feed->next : NULL) {
		if (feed->control_socket < 0) continue;			/* Not open. */
		if (!FD_ISSET (feed->control_socket, rfds)) continue;	/* Not this one. */

		/* This one have pending data. */
		x = read (feed->control_socket, &cb, sizeof (cb));
		if (x <= 0) {
			pController->controller_write_msg (NULL, "Feed %d \"%s\" disconnected\n",
					      feed->id, feed->feed_name ? feed->feed_name : "");
			close (feed->control_socket);
			feed->control_socket = -1;
			feed->previous_state = feed->state;
			feed->state = DISCONNECTED;
//fprintf(stderr, "Feed %d set to DISCONNECTED\n", feed->id);
			feed_release_buffers (feed);

			if (feed->oneshot) {
				/* Delete this feed. */
				//FIXME:
			}
		} else {
			/* Got something of the socket. Mark the feed as running. */
			if (feed->state != RUNNING) {
	
				// We changed to RUNNING and will restart the clock
				gettimeofday(&(feed->start_time), NULL);
				feed->previous_state = feed->state;
				feed->state = RUNNING;
fprintf(stderr, "Feed %d set to RUNNING\n", feed->id);
			}

			/* Interpret the received message. */
			switch (cb.type) {
				case 1:	{	/* COMMAND_NEW_SHM_AREA. */
					/* Create a new area for this feed. */
					struct area_list_type	*area = (area_list_type*)
						calloc (1, sizeof (*area));

					area->area_id = cb.area_id;
					area->area_size = cb.payload.new_shm_area.size;
					area->buffers = NULL;		/* No buffers used yet. */
					area->feed = feed;
					area->area_handle_name = (char*)
						calloc (cb.payload.new_shm_area.path_size + 1, 1);
					ssize_t n = read (feed->control_socket, area->area_handle_name,
						cb.payload.new_shm_area.path_size);
					if (n < 1) {
						if (n < 0) {
							perror("Read error from control socket.\n");
							fprintf(stderr, "Socket handle name %s\n",
								area->area_handle_name);
						} else fprintf(stderr, "Read zero bytes from socket %s\n",
								area->area_handle_name);
					}

					/* Map the area into memory. */
					area->area_handle = shm_open (area->area_handle_name,
						O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
					if (area->area_handle < 0) {
						fprintf (stderr, "Unable to open shared memory buffer "
							"handle: \"%s\"\n", area->area_handle_name);
						dump_cb (block_size, "Her", "Der", &cb, sizeof (cb));
						exit (1);
					}

					area->buffer_base = (uint8_t*) mmap (NULL, area->area_size, PROT_READ, MAP_SHARED, area->area_handle, 0);
					if (area->buffer_base == MAP_FAILED) {
						perror ("Unable to map area into memory.");
						exit (1);
					}

					fprintf (stderr, "New SHM Area. Feed: %s  ID: %d  size: %lu  pathname: %s\n",
						 feed->feed_name, area->area_id, area->area_size, area->area_handle_name);

					list_add_head (feed->area_list, area);
					break;
				}

				case 2:	{	/* COMMAND_CLOSE_SHM_AREA. */
					struct area_list_type	*area = find_area (feed, cb.area_id);
					if (area != NULL) {
						if (area->buffers != NULL) {
							/* This area still ahve outstanding buffers. */
							fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - Still active buffers\n",
								 feed->feed_name, cb.area_id);
						} else {
							/* Release the area. */
							fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - OK\n",
							 	 feed->feed_name, cb.area_id);

							list_del_element (feed->area_list, area);
							if (area->buffer_base != NULL) {
								/* Unmap the memory segment. */
								munmap (area->buffer_base, area->area_size);
								close (area->area_handle);
							}
							free (area->area_handle_name);
							free (area);
						}
					} else {
						fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - Unknown area\n",
							 feed->feed_name, cb.area_id);
					}
					break;
				}
		
				case 3: {	/* COMMAND_NEW_BUFFER. */
					struct buffer_type	*buffer;

					/* Add the buffer to the input fifo - or loose if. */
					if (feed->id == 9) {
					fprintf (stderr, "New Buffer. Feed: %s  ID: %d  block: %4lu  offset: %9lu  size: %9lu\n",
						 feed->feed_name,
						 cb.area_id,
						 cb.payload.buffer.offset/cb.payload.buffer.bsize,
						 cb.payload.buffer.offset,
						 cb.payload.buffer.bsize);
					}

					/* Get a buffer handle for it. */
					buffer = feed_use_buffer (feed, cb.area_id, cb.payload.buffer.offset, cb.payload.buffer.bsize);

					/* Figure out how to add it to the fifo. */
					if (feed->fifo_depth >= feed->max_fifo) {
						/* The fifo is full. Discard the head buffer (head drop). */
						feed_ack_buffer_ptr (block_size, feed->frame_fifo [0].buffer);

						memmove (&feed->frame_fifo [0].buffer,
							 &feed->frame_fifo [1].buffer,
							 sizeof (feed->frame_fifo [0]) * (MAX_FIFO_DEPTH - 1));
						feed->frame_fifo [MAX_FIFO_DEPTH - 1].buffer = NULL;

						feed->frame_fifo [feed->max_fifo - 1].buffer = buffer;
						feed->dropped_frames++;
					} else if (feed->fifo_depth == 0) {
						/* The fifo is empty. Duplicate this frame to half the slots. */
						int	x;

						feed->frame_fifo [0].buffer = buffer;
						feed->fifo_depth++;
						for (x = 1; x < feed->max_fifo/2; x++) {
							feed_use_buffer_ptr (buffer);
							feed->frame_fifo [x].buffer = buffer;
							feed->fifo_depth++;
						}
						feed->good_frames++;
					} else {
						/* Just add it to the end of the fifo. */
						feed->frame_fifo [feed->fifo_depth++].buffer = buffer;
						feed->good_frames++;
					}

					break;
				}

				case 4:	{	/* COMMAND_ACK_BUFFER. */
					/* Not expected from a feed socket. */
					fprintf (stderr, "ACK Buffer. Feed: %s  ID: %d  block: %4lu  offset: %9lu - DUH\n",
						 feed->feed_name,
						 cb.area_id,
						 cb.payload.ack_buffer.offset/block_size,
						 cb.payload.ack_buffer.offset);
					break;
				}

				default:
					fprintf (stderr, "Unknown packet. Feed: %s  Code: %d\n",
						 feed->feed_name, cb.type);
					break;
			}
		}
	}
}


/* Update feeds based on the output frame rate. */
static void feed_timertick (u_int32_t block_size, struct feed_type* feed_list)
{
	struct feed_type	*feed;

	for (feed = feed_list; feed != NULL; feed = feed->next) {
		if (feed->is_live == 0 && feed->dequeued == 0) {
			/* This feed is not being dequeued and it is not live. Don't advance the fifo. */
			continue;
		}
		feed->dequeued = 0;	/* Clear the dequeue flag again. */

		/* Shift the contents of the fifo buffers to advance time. */
		if (feed->fifo_depth > 1) {
			/* The fifo contains multiple frames. Discard the head buffer. */
			feed_ack_buffer_ptr (block_size, feed->frame_fifo [0].buffer);

			memmove (&feed->frame_fifo [0].buffer,
				 &feed->frame_fifo [1].buffer,
				 sizeof (feed->frame_fifo [0]) * (MAX_FIFO_DEPTH - 1));
			feed->frame_fifo [MAX_FIFO_DEPTH - 1].buffer = NULL;
			feed->fifo_depth--;
			feed->missed_frames = 0;
		} else if (feed->fifo_depth == 1) {
			/* The fifo only holds a single frame. Keep it there and do idle timer math. */
			feed->missed_frames++;
			if (feed->missed_frames > feed->dead_img_timeout && feed->dead_img_timeout > 0) {
				/* The feed is dead. Flush the last image. */
				feed_ack_buffer_ptr (block_size, feed->frame_fifo [0].buffer);
				feed->frame_fifo [0].buffer = NULL;
				feed->previous_state = feed->state;
				feed->state = STALLED;
				feed->fifo_depth = 0;
			}
		} else {
			/* The feed is dead. */
			feed->missed_frames++;
		}
	}
}


/************************************************************************
*									*
*	Output handling functions.					*
*									*
************************************************************************/

/* Populate rfds and wrfd for the output subsystem. */
static int output_set_fds (CVideoMixer* pVideoMixer, int maxfd, fd_set *rfds, fd_set *wfds)
{
	if (pVideoMixer->m_output_client_socket >= 0) {
		/* Got a client connection. Only care about that. */
		FD_SET (pVideoMixer->m_output_client_socket, rfds);
		if (pVideoMixer->m_output_client_socket > maxfd) {
			maxfd = pVideoMixer->m_output_client_socket;
		}
	} else {
		if (pVideoMixer->m_output_master_socket >= 0) {
			/* Wait for new connections. */
			FD_SET (pVideoMixer->m_output_master_socket, rfds);
			if (pVideoMixer->m_output_master_socket > maxfd) {
				maxfd = pVideoMixer->m_output_master_socket;
			}
		}
	}

	return maxfd;
}

// Run main poller until exit. Returns 1 upon error, otherwise 0.
static int main_poller (CVideoMixer* pVideoMixer)
{
	struct timeval	next_frame = {0, 0};
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for main_poller\n");
		return 1;
	}
	if (!pVideoMixer->m_pController) {
		fprintf(stderr, "pVideoMixer->m_pController was NULL for main_poller\n");
		return 1;
	}

	/* Start doing some work. */
	while (!exit_program) {
		struct timeval	timeout;
		fd_set		read_fds;
		fd_set		write_fds;
		int		x;
		int		maxfd = 0;

		/* Setup for select. */
		FD_ZERO (&read_fds);
		FD_ZERO (&write_fds);

		maxfd = feed_set_fds       (pVideoMixer, maxfd, &read_fds, &write_fds);
		maxfd = output_set_fds     (pVideoMixer, maxfd, &read_fds, &write_fds);
		maxfd = pVideoMixer->m_pController->controller_set_fds (maxfd, &read_fds, &write_fds);

		/* Calculate the timeout value. */
		{
			struct timeval now;

			gettimeofday (&now, NULL);
			timersub (&next_frame, &now, &timeout);

			if (timeout.tv_sec < 0 || timeout.tv_usec < 0) {
				timeout.tv_sec  = 0;	/* No negative values here. */
				timeout.tv_usec = pVideoMixer->m_frame_rate > 0.0 ?
					1.0/pVideoMixer->m_frame_rate : 50000;
			}
		}

		x = select (maxfd + 1, &read_fds, &write_fds, NULL, &timeout);
		if (x < 0) {
			if (errno == EINTR) {
				continue;	/* Just try again. */
			}
			perror ("Select error");
			return 1;
		}

		if (x > 0) {
			/* Invoke the service functions. */
			feed_check_read(pVideoMixer, &read_fds, pVideoMixer->m_block_size);
			pVideoMixer->m_pController->output_check_read(pVideoMixer, &read_fds);
			pVideoMixer->m_pController->controller_check_read (pVideoMixer, &read_fds);
		}

		if (pVideoMixer->m_frame_rate > 0) {
			struct timeval now;
			gettimeofday (&now, NULL);
			if (timercmp (&now, &next_frame, >)) {
				/* Time to produce some more output. */
				struct timeval	delta, new_time;

				pVideoMixer->output_producer (pVideoMixer->m_pController->m_pSystem_feed);

				/* Check if the frame time is too much behind schedule. */
				delta.tv_sec  = 0;
				delta.tv_usec = 500000;		// Half a second
				new_time = next_frame;

				timeradd (&next_frame, &delta, &new_time);
				if (timercmp (&now, &new_time, >)) {
					struct timeval delta_time;
					timersub(&now, &new_time, &delta_time);
					/* Ups Lagging - Reset the next_frame time to now. */
					next_frame = now;

					if (0 && pVideoMixer->m_pController->m_frame_seq_no) {
						fprintf(stderr, "Frame lagging frame %u : %ld ms\n",
							pVideoMixer->m_pController->m_frame_seq_no,
							(long)(1000*delta_time.tv_sec+delta_time.tv_usec/1000));
						pVideoMixer->m_pController->m_lagged_frames++;
					}
				}

				/* Calculate the next frame time. */
				delta.tv_sec  = 0;
				delta.tv_usec = 1000000 / pVideoMixer->m_frame_rate;
				timeradd (&next_frame, &delta, &next_frame);

				/* Maintain the feeds in sync with the output. */
				feed_timertick (pVideoMixer->m_block_size, pVideoMixer->m_pVideoFeed->GetFeedList());
			}
		}
	}
	if (pVideoMixer->m_output_memory_handle > -1) {
		if (shm_unlink(pVideoMixer->m_output_memory_handle_name)) {
  			perror("shm_unlink error for output handle: ");
		} else fprintf(stderr,"Closed output handle\n");
	}

	return 0;
}

// PMM. To be developed
// Must block for signals and clean up after it and exit nicely unlinking shared memory
int exit_function() {
	fprintf(stderr,"ON Exit nicely\n");
	return 0;
}

#ifdef WITH_GUI
static GtkWidget* CreateMainWindow()
{
	GtkWidget*	MainWindow = NULL;
	GtkWidget*	vbox1;
	GtkWidget*	menubar1;
	GtkWidget*	menuitem1;
	GtkWidget*	menuitem1_menu;
	GtkWidget*	new1;
	GtkWidget*	save1;
	GtkWidget*	open1;
	GtkWidget*	separatormenuitem1;
	GtkWidget*	quit1;
	GtkWidget*	menuitem2;
	GtkWidget*	menuitem2_menu;

	GtkAccelGroup*	accel_group;

	//tooltips = gtk_tooltips_new();

	accel_group = gtk_accel_group_new();

	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(MainWindow), vbox1);

	menubar1 = gtk_menu_bar_new();
	gtk_widget_show(menubar1);
	gtk_box_pack_start(GTK_BOX(vbox1), menubar1, FALSE, FALSE, 0);

	menuitem1 = gtk_menu_item_new_with_mnemonic(("_File"));
	gtk_widget_show(menuitem1);
	gtk_container_add(GTK_CONTAINER(menubar1), menuitem1);

	menuitem1_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem1), menuitem1_menu);

	new1 = gtk_image_menu_item_new_from_stock("gtk-new", accel_group);
	gtk_widget_show(new1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), new1);

	open1 = gtk_image_menu_item_new_from_stock("gtk-open", accel_group);
	gtk_widget_show(open1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), open1);

	save1 = gtk_image_menu_item_new_from_stock("gtk-save", accel_group);
	gtk_widget_show(save1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), save1);

	separatormenuitem1 = gtk_menu_item_new();
	gtk_widget_show(separatormenuitem1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), separatormenuitem1);
	gtk_widget_set_sensitive(separatormenuitem1, FALSE);

	quit1 = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);
	gtk_widget_show(quit1);
	gtk_container_add(GTK_CONTAINER(menuitem1_menu), quit1);

	menuitem2 = gtk_menu_item_new_with_mnemonic(("_Edit"));
	gtk_widget_show(menuitem2);
	gtk_container_add(GTK_CONTAINER(menubar1), menuitem2);

	menuitem2_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem2), menuitem2_menu);

	return MainWindow;
}
static int LauchGtkMain(void* p)
{
	gtk_main();
	return 0;
}
#endif // WITH_GUI

int main(int argc, char *argv[])
{
#ifdef WITH_GUI
	GtkWidget *MainWindow = NULL;
	SDL_Thread* GtkThread = NULL;
#endif // WITH_GUI
	CVideoMixer* pVideoMixer = new CVideoMixer();
	if (!pVideoMixer) {
		fprintf(stderr, "Failed to allocate VideoMixer\n");
		return 1;
	}
	pVideoMixer->m_pVideoFeed =
                                new CVideoFeed(pVideoMixer, MAX_VIDEO_FEEDS);
	pVideoMixer->m_pController = 
		((argc > 1) ? new CController(argv [1]) :	/* Use specified setup file. */
			new CController((char*) "videomixer.ini")); /* Use the default file. */
	if (!pVideoMixer->m_pController) {
		fprintf(stderr, "Failed to create controller for video mixer.\n");
		return 1;
	}

#ifdef WITH_GUI
	gtk_init(&argc, &argv);
	MainWindow = CreateMainWindow();
	gtk_window_set_title(GTK_WINDOW(MainWindow), "Snowmix");
	gtk_widget_show(MainWindow);
	GtkThread = SDL_CreateThread(LauchGtkMain, NULL);
#endif // WITH_GUI

	int ret = main_poller (pVideoMixer);
#ifdef WITH_GUI
	SDL_KillThread(GtkThread);
#endif // WITH_GUI
	return ret;
}

