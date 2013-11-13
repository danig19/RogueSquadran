/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
#include <stdio.h>
//#include <limits.h>
#include <string.h>
#include <malloc.h>
//#include <sys/types.h>

#include <sys/socket.h>

#include "snowmix.h"
#include "video_mixer.h"
#include "video_text.h"
#include "video_scaler.h"
#include "video_image.h"
//#include "add_by.h"
#include "command.h"

CVideoMixer::CVideoMixer() {
	m_geometry_width = 0;
	m_geometry_height = 0;
	memset(m_pixel_format, 0, 5);
	m_video_type = VIDEO_NONE;
	m_pixel_bytes = 0;
	m_frame_rate = 0;
	m_block_size = 0;
	//m_system_monitor = false;
	m_pMonitor = NULL;
	m_pVideoScaler = NULL;
	m_pCairoGraphic = NULL;
	m_pTextItems = NULL;
	m_output_stack[0] = 0;
	for (int i=1 ; i<MAX_STACK; i++ ) m_output_stack[i] = -1;
	m_pOutput_master_name = NULL;
	m_output_master_socket = -1;
	m_output_client_socket = -1;
	m_output_memory_handle_name[0] = '\0';
	m_output_memory_handle = -1;
	m_output_memory_size = 0;
	m_pOutput_memory_base = NULL;
	m_pOutput_memory_list = NULL;
	m_pController = NULL;
	gettimeofday(&m_start,NULL);
	m_start_master.tv_sec = m_start_master.tv_usec = 0;
	m_time_last.tv_sec = m_time_last.tv_usec = 0;
	m_mixer_duration.tv_sec = m_mixer_duration.tv_usec = 0;
	m_output_called = 0;
	m_output_missed = 0;
	m_pVideoImage	= NULL;
	m_pVideoFeed	= NULL;
	m_pVideoShape	= NULL;
	m_pAudioFeed	= NULL;
	m_pAudioMixer	= NULL;
	m_pAudioSink	= NULL;
	m_pVirtualFeed	= NULL;
	m_overlay	= NULL;
	m_command_pre	= NULL;
	m_command_finish = NULL;
	m_write_png_file = NULL;
	//m_rotate = 0.0;
	m_pTclInterface = NULL;
/*
	if (m_pVideoImage->LoadImage(0, "cs-logo.png")) {
		fprintf(stderr, "Failed to load png file\n");
	} else {
		fprintf(stderr, "png file loaded\n");
		if (png_get_color_type(m_pVideoImage->m_image_items[0]->png_ptr, m_pVideoImage->m_image_items[0]->info_ptr) == PNG_COLOR_TYPE_RGB)
			fprintf(stderr, "png file was RGB\n");
			else fprintf(stderr, "png file was NOT RGB\n");
		if (png_get_color_type(m_pVideoImage->m_image_items[0]->png_ptr, m_pVideoImage->m_image_items[0]->info_ptr) != PNG_COLOR_TYPE_RGBA) fprintf(stderr, "png file was NOT RGBA\n");
			else fprintf(stderr, "png file was RGBA\n");
	}
*/
}

CVideoMixer::~CVideoMixer() {

	if (m_pOutput_master_name) free(m_pOutput_master_name); m_pOutput_master_name = NULL;
	if (m_pMonitor) delete m_pMonitor; m_pMonitor = NULL;
	if (m_pVideoScaler) delete m_pVideoScaler; m_pVideoScaler = NULL;
	if (m_pCairoGraphic) delete m_pCairoGraphic; m_pCairoGraphic = NULL;
	if (m_pTextItems) delete m_pTextItems; m_pTextItems = NULL;
	if (m_pAudioFeed) delete m_pAudioFeed; m_pAudioFeed = NULL;
	if (m_pVideoFeed) delete m_pVideoFeed; m_pVideoFeed = NULL;
	if (m_pVideoImage) delete m_pVideoImage; m_pVideoImage = NULL;
	if (m_pVirtualFeed) delete m_pVirtualFeed; m_pVirtualFeed = NULL;
	if (m_pController) delete m_pController; m_pController = NULL;
}

// SetGeometry()
// Sets m_video_type, m_pixel_bytes, m_pixel_format, m_block_size
// WARNING. pixelformat must be nul terminated.
int CVideoMixer::SetGeometry(const u_int32_t width, const u_int32_t height, const char* pixelformat) {
	u_int32_t w_mod = 0;
	u_int32_t h_mod = 0;

	if (!width || !height || !pixelformat) return -1;
	m_pixel_bytes = 0;		// For safe keeping

	//pixelformat[4]= '\0'; Wee need to ensure this, as we print it if we have errors.
	if (pixelformat[4]) {
		fprintf(stderr, "pixelformat was not nulterminated on 5th position\n");
		return -1;
	}

	// Cycle through allowable formats
	if (strcmp (pixelformat, "ARGB") == 0) {
		m_pixel_bytes = 4;
		m_video_type = VIDEO_ARGB;
		w_mod = 1;
		h_mod = 1;
	} else if (strcmp (pixelformat, "BGRA") == 0) {
		m_pixel_bytes = 4;
		m_video_type = VIDEO_ARGB;
		w_mod = 1;
		h_mod = 1;
	} else if (strcmp (pixelformat, "YUY2") == 0) {
		m_pixel_bytes = 2;
		m_video_type = VIDEO_ARGB;
		w_mod = 2;
		h_mod = 1;
	} else if (strcmp (pixelformat, "I420") == 0) {
		fprintf(stderr, "Unsupported pixelformat <%s> as we can not "
			"yet handle 1.5 bytes per pixel\n", pixelformat);
		return -1;
	} else {
		fprintf(stderr, "Unsupported pixelformat <%s>\n", pixelformat);
		return -1;
	}

	// Check for valid widths and heights
	if (width < 0 || (width % w_mod) != 0) {
		fprintf(stderr, "Video width must be divisable by %u.\n", w_mod);
		m_pixel_bytes = 0;
		m_video_type = VIDEO_NONE;
		return -1;
	}
	if (height < 0 || (height % h_mod) != 0) {
		fprintf(stderr, "Video height must be divisable by %u.\n", w_mod);
		m_pixel_bytes = 0;
		m_video_type = VIDEO_NONE;
		return -1;
	}

	if (m_pVideoScaler) delete m_pVideoScaler;
	m_pVideoScaler = new CVideoScaler(width, height, m_video_type);
	if (!m_pVideoScaler) {
		fprintf(stderr, "Faild to get video scaler for video mixer.\n");
		m_pixel_bytes = 0;
		m_video_type = VIDEO_NONE;
		return -1;
	}
	strncpy (m_pixel_format, pixelformat, sizeof (m_pixel_format) - 1);
	m_block_size = m_pixel_bytes * width * height;
	m_geometry_width = width;
	m_geometry_height = height;
	return 0;
}

#define time2double(a) ((double) a.tv_sec + ((double) a.tv_usec) / 1000000.0)
/* Produce output data. */
/* This function is invoked at the frame rate. */
int CVideoMixer::output_producer (struct feed_type* system_feed)
{
	struct feed_type		*feeds [MAX_STACK];
	int				feed_no;
	struct output_memory_list_type	*buf;
	struct shm_commandbuf_type	cb;
	int				x;

	m_output_called++;
	struct timeval timenow, time;
	gettimeofday(&timenow, NULL);
        if (m_output_called == 1) {
		m_start_master = timenow;
		m_time_last = m_start_master;
		m_output_missed = 0;
	} else {
		timersub(&timenow,&m_start_master,&time);
		double since_start = time2double(time);
		timersub(&timenow,&m_time_last,&time);
		double since_last = time2double(time);
		double frame_period = 1.0 / m_frame_rate;
		double mixer_duration = time2double(m_mixer_duration);
		if (since_last/frame_period >= 2.0) {
			fprintf(stderr, "FRAME %u:%u Period Delays %.1lf frame(s)\n",
				m_pController->m_frame_seq_no,
				m_output_called,
				since_last/frame_period);
		}
/*
		if (since_last < frame_period) {
			fprintf(stderr, "CHECK %4u:%-4u %.3lf %7.3lf <<<\n" , m_output_called, m_pController->m_frame_seq_no, mixer_duration, since_last);
		} else if (since_last > 2*frame_period) {
			fprintf(stderr, "CHECK %4u:%-4u %.3lf %7.3lf >>>+++\n" , m_output_called, m_pController->m_frame_seq_no, mixer_duration, since_last);
		} else if (since_last > 1.5*frame_period) {
			fprintf(stderr, "CHECK %4u:%-4u %.3lf %7.3lf >>>\n" , m_output_called, m_pController->m_frame_seq_no, mixer_duration, since_last);
		} else if (since_last > 1.2*frame_period) {
			fprintf(stderr, "CHECK %4u:%-4u %.3lf %7.3lf >\n" , m_output_called, m_pController->m_frame_seq_no, mixer_duration, since_last);
		} else  fprintf(stderr, "CHECK %4u:%-4u %.3lf %7.3lf\n" , m_output_called, m_pController->m_frame_seq_no, mixer_duration, since_last);
*/

	}
	m_time_last.tv_sec = timenow.tv_sec;
	m_time_last.tv_usec = timenow.tv_usec;


	if (m_pController && m_command_pre)
		m_pController->ParseCommandByName(this, NULL, m_command_pre);
	if (m_pAudioFeed) m_pAudioFeed->Update();
	if (m_pAudioMixer) m_pAudioMixer->Update();
	if (m_pAudioSink) m_pAudioSink->Update();

	// Run timed commands, if any
	if (m_pController && m_pController->m_pCommand) {
		m_pController->m_pCommand->RunTimedCommands();
	}

	if (m_output_client_socket < 0) {
		return 0;		/* No receiver yet. Do nothing. */
	}

	/* Check if any members of the output stack have disappeared. */
	feed_no = 0;
	for (x = 0; x < MAX_STACK; x++) {
		if (m_output_stack [x] < 0) break;
		if (!m_pVideoFeed) {
			fprintf(stderr, "pVideoFeed for output producer was NULL\n");
			return -1;
		}
		feeds [feed_no] = m_pVideoFeed->FindFeed (m_output_stack [x]);
		if (feeds [feed_no] != NULL) {
			feed_no++;
		}
	}
	if (feed_no > 0 && (feeds [0]->width != m_geometry_width || feeds [0]->height != m_geometry_height)) {
		/* The first feed is NOT a full-screen feed. Just display the system feed. */
		feed_no = 0;
		feeds [feed_no++] = system_feed;
	}
	if (feed_no == 0) {
		/* All selected feeds have disappeared. */
		feeds [feed_no++] = system_feed;
	}
	if (feeds [0] == NULL) {
		/* No feed available. The system is not yet initialized. Nothing to deliver. */
		return -1;
	}

	/* Not able to do it without copying the frame. */
	/* This is due to a bug in gst-plugins/bad/sys/shm/shmpipe.c line 931 where self->shm_area->id */
	/* should have been shm_area->id. In other words it will always acknowledge buffers using the */
	/* newest area_id recorded. */
	buf = m_pController->output_get_free_block (this);
	if (buf == NULL) {
		/* Out of memory. Not able to deliver a frame. Skip the timeslot. */
		return -1;
	}

	// Set Overlay (active video frame)
	m_overlay = m_pOutput_memory_base + buf->offset;

	/* Deliver a frame from the selected feed(s). */
	for (x = 0; x < feed_no; x++) {
		struct feed_type	*feed = feeds [x];
		uint8_t	*src_buf = NULL;

		if (feed->fifo_depth <= 0) {
			/* Nothing in the fifo. Show the idle image. */
			src_buf = feed->dead_img_buffer;
		} else {
			/* Pick the first one from the fifo. */
			src_buf = feed->frame_fifo [0].buffer->area_list->buffer_base +
				feed->frame_fifo [0].buffer->offset;
		}
		feed->dequeued = 1;	/* Set the dequeued flag on the feed. */

		if (x == 0) {
			/* The bottom layer have to be fullscreen. Just copy/zero the whole image in one operation. */
			if (src_buf != NULL) {
				memcpy (m_pOutput_memory_base + buf->offset, src_buf, m_block_size);
			} else {
				memset (m_pOutput_memory_base + buf->offset, 0x00, m_block_size);
			}
		} else if (src_buf != NULL) {
			/* Not the bottom layer => Add it on top of the bottom layer. */
			/* Setup pointers for helping with the copy. */
			int	out_row_size   = m_geometry_width * m_pixel_bytes;
			uint8_t	*out_row_start = m_pOutput_memory_base + buf->offset +
						 (feed->shift_columns * m_pixel_bytes) +
						 (feed->shift_rows * out_row_size);

			int	in_row_size    = feed->width * m_pixel_bytes;
			uint8_t	*in_row_start  = src_buf +
						 (feed->cut_start_column * m_pixel_bytes) +
						 (feed->cut_start_row * in_row_size);

			int	copy_size;
			unsigned int	row;
			unsigned int	maxrow;

			if (feed->cut_start_column + feed->cut_columns >= m_geometry_width) {
				/* Don't exceed the size of a image row. */
				copy_size = (m_geometry_width - feed->cut_start_column) * m_pixel_bytes;
			} else {
				/* It is inside the limit. */
				copy_size = feed->cut_columns * m_pixel_bytes;
			}

			maxrow = feed->cut_start_row + feed->cut_rows;
			if (maxrow > m_geometry_height) {
				maxrow = m_geometry_height;	/* Stay inside the frame. */
			}

			// PMM Code Start
			// Check to see if we need scaling
			u_int8_t* pNew = NULL;
			int32_t width_tmp = feed->width;
			if (feed->par_w != feed->par_h) {
				if (feed->par_w == 4 && feed->par_h == 5) {
					pNew = m_pVideoScaler->PAR_5_4(
						NULL,
						src_buf,			// Feed Frame start
						feed->width,			// Feed width
						feed->height,			// Feed height
						5,
						4);
					feed->width = feed->width*5/4;
				} else if (feed->par_w == 11 && feed->par_h ==12) {
					pNew = m_pVideoScaler->PAR_12_11(
						NULL,
						src_buf,			// Feed Frame start
						feed->width,			// Feed width
						feed->height,			// Feed height
						12,
						11);
					feed->width = feed->width*12/11;
				} else {
					pNew = m_pVideoScaler->PAR(
						src_buf,			// Feed Frame start
						feed->width,			// Feed width
						feed->height,			// Feed height
						feed->par_w,
						feed->par_h);
					feed->width = feed->width*feed->par_w/feed->par_h;
				}
			}

				
			if (feed->scale_1 > 1 || feed->scale_2 > 1) {
				u_int32_t cut_rows = 0;
				u_int32_t cut_columns = 0;

				if (!(m_pVideoScaler)) {
					fprintf(stderr, "Feed %d has no VideoScaler. Skipping feed\n", feed->id);
					continue;
				}
				if (feed->scale_1 == 1 || feed->scale_1 == 2) {
					// We know we can multiply the 
					cut_columns =
				  		(feed->shift_columns + feed->cut_columns >= m_geometry_width ?
				   		 m_geometry_width - feed->shift_columns :
				   		 feed->cut_columns)*feed->scale_2/feed->scale_1;
					if (cut_columns > feed->width) cut_columns = feed->width;

					// maxrow - feed->cut_start_row is the number of rows to clip
					// We scale the number of clip rows because we can - until height
					// Now we also know the rows are divisable by 3
					cut_rows = (maxrow - feed->cut_start_row)*feed->scale_2/feed->scale_1;
					if (cut_rows > feed->height) cut_rows = feed->height;
				}
				// We want cut_rows to be divisable by scale_2
				cut_rows /= feed->scale_2;
				cut_rows *= feed->scale_2;

				// We will center the cut
				u_int32_t cut_start_column = (feed->width - cut_columns)>>1;
				u_int32_t cut_start_row = (feed->height - cut_rows)>>1;

				// Truncate to even start pixel
				cut_start_column &= 0xfffe;

				if (feed->scale_1 == 1 && feed->scale_2 == 2) {
					m_pVideoScaler->Overlay2(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else if (feed->scale_1 == 1 && feed->scale_2 == 3) {
					m_pVideoScaler->Overlay3(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else if (feed->scale_1 == 1 && feed->scale_2 == 4) {
					m_pVideoScaler->Overlay4(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else if (feed->scale_1 == 1 && feed->scale_2 == 5) {
					m_pVideoScaler->Overlay5(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else if (feed->scale_1 == 2 && feed->scale_2 == 3) {
					m_pVideoScaler->Overlay2_3(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else if (feed->scale_1 == 2 && feed->scale_2 == 5) {
//fprintf(stderr, "Feed scale 2/3 %d\n", feed->id);
					m_pVideoScaler->Overlay2_5(
						m_overlay,// Overlay Frame start
						pNew ? pNew : src_buf,			// Feed Frame start
						feed->shift_columns,		// Overlay Start Column
						feed->shift_rows,		// Overlay Start row
						feed->width,			// Feed width
						feed->height,			// Feed height
						cut_start_column,		// Feed cut start col
						cut_start_row,			// Feed cut start row
						cut_columns,			// Feed cut columns
						cut_rows);			// Feed cut rows
				} else {
fprintf(stderr, "Scale not supported %d:%d\n", feed->scale_1, feed->scale_2);
				}
//		if (feed->id == 4 && m_pController->m_monitor_status && src_buf) {
//			fprintf(stderr, "Show buf %u\n", (u_int32_t) src_buf);
//			//memset (src_buf, 0, 50);
//			if (system_monitor) system_monitor->ShowDisplay(src_buf, 0);
//		}
				feed->width = width_tmp;
				if (pNew) free(pNew); pNew = NULL;
			} else {
			// PMM Code End

				// PMM Here we copy line by line
				/* Copy it row by row. */
				for (row = feed->cut_start_row; row < maxrow; row++) {
					memcpy (out_row_start, in_row_start, copy_size);
					out_row_start += out_row_size;
					in_row_start  += in_row_size;
				}
			}
		} else {
			/* Don't PIP a unset black image. Just make it transparent
				(i.e. ignore it). */
		}
	}

	if (m_pController) m_pController->m_frame_seq_no++;
/*
	if (m_pController->m_frame_seq_no > 0 && m_pController->m_frame_seq_no % 6 == 0) {
		struct timeval timenow, time;
		gettimeofday(&timenow, NULL);
		timersub(&timenow, &m_start_master, &time);
		int32_t frames = time.tv_sec*m_frame_rate + time.tv_usec*m_frame_rate/1000000 + 1;
		fprintf(stderr, "Frames at %5.2f : %s %d (calc %d produced %u\n",
			time.tv_sec + time.tv_usec/1000000.0,
			frames != m_pController->m_frame_seq_no ? "missing" : "ok",
			frames - m_pController->m_frame_seq_no, frames, m_pController->m_frame_seq_no);
		
	}
*/
	if (m_pTextItems || m_pVideoImage || m_pVideoShape) {

		// Create the Cairo video surface using the current system frame
		m_pCairoGraphic = new CCairoGraphic(m_geometry_width, m_geometry_height,
			m_overlay);
		if (m_pCairoGraphic) {
			if (m_pController && m_command_finish)
				m_pController->ParseCommandByName(this, NULL, m_command_finish);

		}

		if (m_pVideoShape) m_pVideoShape->Update();
		if (m_write_png_file) {
			m_pCairoGraphic->WriteFilePNG(m_pCairoGraphic->m_pSurface,
				m_write_png_file);
			free(m_write_png_file);
			m_write_png_file = NULL;
		}
		delete m_pCairoGraphic;
		m_pCairoGraphic = NULL;
	}
	if (m_pController->m_monitor_status) {
		m_pMonitor->ShowDisplay(m_overlay, 0);
	}

	// Set pointer to active video to NULL to prevent other part from accessing it
	m_overlay = NULL;


	/* Transmit the buffer reference to the output socket. */
	memset (&cb, 0, sizeof (cb));
	cb.type = 3;
	cb.area_id = 1;
	cb.payload.buffer.offset = buf->offset;
	cb.payload.buffer.bsize  = m_block_size;

	if (send (m_output_client_socket, &cb, sizeof (cb), MSG_NOSIGNAL) > 0) {
		if (0) {
			dump_cb (m_block_size, "Mixer", "Client", &cb, sizeof (cb));
		}
	} else {
		/* Client disappeared. */
		fprintf (stderr, "Reset ved buffer TX\n");
		m_pController->output_reset (this);
	}
	gettimeofday(&timenow, NULL);
	timersub(&timenow,&m_time_last,&m_mixer_duration);

	return 0;
}

// overlay text (<id> | <id>..<id>) [(<id> | <id>..<id>)] [(<id> | <id>..<id>)] ....
int CVideoMixer::OverlayImage(const char *str)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	int from, to;
	from = to = 0;
	while (*str) {
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pVideoImage->MaxImages();
			while (*str) str++;
		} else if (!strncmp(str,"end",3)) {
			to = m_pVideoImage->MaxImages();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) to = from;
			else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			// PMM
			if (m_pVideoImage && m_pVideoImage->GetImagePlaced(from)) {
				if (m_pCairoGraphic) {
					m_pCairoGraphic->OverlayImage(this, from);
				}
				//m_pVideoImage->OverlayImage(from, m_overlay,
					//m_geometry_width, m_geometry_height);
			}
			from++;
		}
	}
	return 0;
}

// overlay text (<id> | <id>..<id>) [(<id> | <id>..<id>)] [(<id> | <id>..<id>)] ....
int CVideoMixer::OverlayText(const char *str, CCairoGraphic* pCairoGraphic)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	while (*str) {
		int from, to;
		from=0;
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pTextItems->MaxPlaces();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) {
				if (!strncmp(str,"..end",5)) {
					to = m_pTextItems->MaxPlaces();
					while (*str) str++;
				} else to = from;
			} else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			pCairoGraphic->OverlayText(this, from);
			//text_item_t* pListItem = m_pTextItems->GetTextPlace(from);
			//	if (pListItem) pCairoGraphic->OverlayText(this, pListItem);
			from++;
		}
	}
	return 0;
}

// overlay virtual feed feed_no
// overlay virtual feed (<id> | <id>..<id>) [(<id> | <id>..<id>)] [(<id> | <id>..<id>)] ....
int CVideoMixer::OverlayVirtualFeed(CCairoGraphic* pCairoGraphic, char* str)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	int from, to;
	from = to = 0;
	while (*str) {
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pVirtualFeed->MaxFeeds();
			while (*str) str++;
		} else if (!strncmp(str,"end",3)) {
			to = m_pVirtualFeed->MaxFeeds();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) to = from;
			else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			int32_t x, y;
			u_int32_t vir_feed_no, w, h, src_x, src_y, width, height;
			double scale_x, scale_y, rotation, alpha;
			vir_feed_no = from;
			int i = m_pVirtualFeed->GetFeedCoordinates(vir_feed_no, &x, &y);
			int j = m_pVirtualFeed->GetFeedClip(vir_feed_no, &src_x, &src_y, &w, &h);
			int k = m_pVirtualFeed->GetFeedScale(vir_feed_no, &scale_x, &scale_y);
			int l = m_pVirtualFeed->GetFeedRotation(vir_feed_no, &rotation);
			int m = m_pVirtualFeed->GetFeedAlpha(vir_feed_no, &alpha);
			int n = m_pVirtualFeed->GetFeedGeometry(vir_feed_no, &width, &height);
			transform_matrix_t* pMatrix = m_pVirtualFeed->GetFeedMatrix(vir_feed_no);

			if (i || j || k || l || m || n ) return 1;

			int real_feed_no = m_pVirtualFeed->GetFeedSourceId(vir_feed_no);
			if (!m_pVideoFeed) {
				fprintf(stderr, "pVideoFeed was NULL for OverlayVirtualFeed\n");
				return 1;
			}
			struct feed_type* pFeed = m_pVideoFeed->FindFeed (real_feed_no);
			u_int8_t* src_buf = NULL;
			if (pFeed->fifo_depth <= 0) {
		       		/* Nothing in the fifo. Show the idle image. */
		       		src_buf = pFeed->dead_img_buffer;
			} else {
		       		/* Pick the first one from the fifo. */
		       		src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
			       		pFeed->frame_fifo [0].buffer->offset;
			}
			if (pCairoGraphic) {
		 		pCairoGraphic->OverlayFrame(src_buf, width, height,
		       			x, y, w, h, src_x, src_y, scale_x , scale_y, rotation, alpha, pMatrix);
		 		m_pVirtualFeed->UpdateMove(vir_feed_no);
			}
			from++;
		}
	}
	return 0;
}


// cairooverlay feed feed_no x y w h src_x src_y scale_x scale_y rotate alpha
int CVideoMixer::CairoOverlayFeed(CCairoGraphic* pCairoGraphic, char* str)
{
	if (!str || !pCairoGraphic) return 1;
	while (isspace(*str)) str++;
	u_int32_t feed_no, x, y, w, h, src_x, src_y;
	double scale_x, scale_y, rotate, alpha;
	int n = sscanf(str, "%u %u %u %u %u %u %u %lf %lf %lf %lf", &feed_no, &x, &y, &w, &h,
		&src_x, &src_y, &scale_x, &scale_y, &rotate, &alpha);
	if (!m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL for CairoOverlayFeed\n");
		return 1;
	}
	struct feed_type* pFeed = m_pVideoFeed->FindFeed (feed_no);
	if (pFeed) {
		u_int8_t* src_buf = NULL;
		if (pFeed->fifo_depth <= 0) {
			/* Nothing in the fifo. Show the idle image. */
			src_buf = pFeed->dead_img_buffer;
		} else {
			/* Pick the first one from the fifo. */
			src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
				pFeed->frame_fifo [0].buffer->offset;
		}
		
		if (pCairoGraphic) {

			if (n == 11) pCairoGraphic->OverlayFrame(src_buf, pFeed->width, pFeed->height,
				x, y, w, h, src_x, src_y, scale_x , scale_y, rotate, alpha);
			else if (n == 9) pCairoGraphic->OverlayFrame(src_buf, pFeed->width, pFeed->height,
				x, y, w, h, src_x, src_y, scale_x , scale_y);
			else if (n == 7) pCairoGraphic->OverlayFrame(src_buf, pFeed->width, pFeed->height,
				x, y, w, h, src_x, src_y);
			else return 1;

		} else return 1;
	} else return 1;
	return 0;
}

// overlay feed <feed id> <col> <row> <feed col> <feed row> <cut cols> <cut rows> <scale 1> <scale 2> <place>
int CVideoMixer::OverlayFeed(char *str)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	u_int32_t feed_no, col, row, feed_col, feed_row, cut_cols, cut_rows, scale_1, scale_2;
	char c = 'c';
	int n = sscanf(str, "%u %u %u %u %u %u %u %u %u %c", &feed_no,
		&col, &row, &feed_row, &feed_col, &cut_cols, &cut_rows, &scale_1, &scale_2, &c);
	if (n == 7) {
		scale_1 = 1;
		scale_2 = 1;
	} else if ((n != 9 && n != 10) || scale_1 == 0 || scale_2 == 0) return 1;
	if (!m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL for OverlayFeed\n");
		return 1;
	}
	struct feed_type* pFeed = m_pVideoFeed->FindFeed (feed_no);
	if (!pFeed) return 1;
	if (scale_1 == scale_2) scale_1 = scale_2 = 1;
	while ((!(scale_1 & 1)) && (!(scale_2 & 1))) { scale_1 >>= 1; scale_2 >>= 1; }
	if (((scale_2/scale_1)*scale_1) == scale_2) { scale_2 /= scale_1 ; scale_1 = 1; }
	if (scale_1 > 1 || scale_2 > 1) {

		if (!(m_pVideoScaler)) {
			fprintf(stderr, "Overlay feed %d has no VideoScaler. Skipping overlay\n",
				feed_no);
			return 1;
		}
		// Check that we max cut width and height of feed.
		if (cut_cols > pFeed->width) { cut_cols = pFeed->width; feed_col = 0; }
		if (cut_rows > pFeed->height) { cut_rows = pFeed->height; feed_row = 0; }

		// Now get avilable cols/rows on base given col/row placement
		u_int32_t available_cols = m_geometry_width - col;
		u_int32_t available_rows = m_geometry_height - row;

		// Now check that we are allowed to produce the cut
		if (cut_cols*scale_1/scale_2 > available_cols) cut_cols = available_cols*scale_2/scale_1;
		if (cut_rows*scale_1/scale_2 > available_rows) cut_rows = available_rows*scale_2/scale_1;

		// Now check if we want to center
		if (c == 'c' || c == 'C') {
			feed_col = (pFeed->width - cut_cols)>>1;
			feed_row = (pFeed->height - cut_rows)>>1;
		}

		// We want cut_rows to be divisable by scale_2
//		cut_rows /= feed->scale_2;
//		cut_rows *= feed->scale_2;

		// Truncate to even start pixel
//		cut_start_column &= 0xfffe;
		u_int8_t* src_buf = NULL;
		if (pFeed->fifo_depth <= 0) {
			/* Nothing in the fifo. Show the idle image. */
			src_buf = pFeed->dead_img_buffer;
		} else {
			/* Pick the first one from the fifo. */
			src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
				pFeed->frame_fifo [0].buffer->offset;
		}
#define VideoScale(scaler) m_pVideoScaler->scaler(	\
						m_overlay, 	/* Overlay Frame start */	\
						src_buf, 	/* Feed Frame start */		\
						col,	 	/* Overlay Start Column */	\
						row,	 	/* Overlay Start row */		\
						pFeed->width, 	/* Feed width */		\
						pFeed->height, 	/* Feed height */		\
						feed_col, 	/* Feed cut start col */	\
						feed_row, 	/* Feed cut start row */	\
						cut_cols, 	/* Feed cut columns */		\
						cut_rows)	/* Feed cut rows  */
		if (scale_1 == 1 && scale_2 == 1) VideoScale(Overlay);
		else if (scale_1 == 1 && scale_2 == 2) VideoScale(Overlay2);
		else if (scale_1 == 1 && scale_2 == 3) VideoScale(Overlay3);
		else if (scale_1 == 1 && scale_2 == 4) VideoScale(Overlay4);
		else if (scale_1 == 1 && scale_2 == 4) VideoScale(Overlay5);
		else if (scale_1 == 2 && scale_2 == 3) VideoScale(Overlay2_3);
		else if (scale_1 == 2 && scale_2 == 5) VideoScale(Overlay2_5);

		// else we cant scale;
	}

	return 0;
}
