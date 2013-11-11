/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
//#include <string.h>
#include <malloc.h>
//#include <stdlib.h>
//#include <png.h>
#include <sys/types.h>
#include <math.h>
//#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>


//#include "cairo/cairo.h"
#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "video_feed.h"
#include "controller.h"

CVideoFeed::CVideoFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds)
{
	m_max_feeds = max_feeds;
	m_pFeed_list = NULL;
	m_feed_count = 0;
	m_pVideoMixer = pVideoMixer;
}

CVideoFeed::~CVideoFeed()
{
	while (m_pFeed_list) {
		struct feed_type* p = m_pFeed_list->next;
		free(m_pFeed_list);
		m_pFeed_list = p;
	}
	m_max_feeds = 0;
}

int CVideoFeed::ParseCommand(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer) return 0;

	// Commands are checked in the order of runtime importance
	// Commands usually only executed at startup or seldom used
	// are placed last
	// feed info
	if (!strncasecmp (ci, "info ", 5)) {
		return feed_info (ctr, ci+5);
	}
	// feed cutout
	else if (!strncasecmp (ci, "cutout ", 7)) {
		return set_feed_cutout(ctr, ci+7);
	}
	// feed shift
	else if (!strncasecmp (ci, "shift ", 6)) {
		return set_feed_shift(ctr, ci+6);
	}
	// feed list verbose
	else if (!strncasecmp (ci, "list verbose ", 13)) {
		return feed_list_feeds (ctr, 1);
	}
	// feed list
	else if (!strncasecmp (ci, "list ", 5)) {
		return feed_list_feeds (ctr, 0);
	}
	// feed scale
	else if (!strncasecmp (ci, "scale ", 6)) {
		return set_feed_scale(ctr, ci+6);
	}
	// feed buffers
	else if (!strncasecmp (ci, "buffers ", 8)) {
		return feed_dump_buffers (ctr);
	}
	// feed name
	else if (!strncasecmp (ci, "name ", 5)) {
		return set_feed_name(ctr, ci+5);
	}
	// feed live
	else if (!strncasecmp (ci, "live ", 5)) {
		return set_feed_live(ctr, ci+5);
	}
	// feed recorded
	else if (!strncasecmp (ci, "recorded ", 9)) {
		return set_feed_recorded(ctr, ci+9);
	}
	// feed help
	else if (!strncasecmp (ci, "help ", 5)) {
		return list_help (ctr, ci+5);
	}
	// feed add
	else if (!strncasecmp (ci, "add ", 4)) {
		return set_feed_add(ctr, ci+4);
	}
	// feed par
	else if (!strncasecmp (ci, "par ", 4)) {
		return set_feed_par(ctr, ci+4);
	}
	// feed socket
	else if (!strncasecmp (ci, "socket ", 7)) {
		return set_feed_socket(ctr, ci+7);
	}
	// feed geometry
	else if (!strncasecmp (ci, "geometry ", 9)) {
		return set_feed_geometry(ctr, ci+9);
	}
	// feed idle
	else if (!strncasecmp (ci, "idle ", 5)) {
		return set_feed_idle(ctr, ci+5);
	} else return 1;

	return 0;
}

// Create a virtual feed
int CVideoFeed::list_help(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
		MESSAGE"feed add [<feed no> <feed name>]\n"
		MESSAGE"feed buffers\n"
		MESSAGE"feed cutout <feed no> <start col> <start row> <columns> <rows>\n"
		MESSAGE"feed geometry <feed no> <width> <height>\n"
		MESSAGE"feed help // this list\n"
		MESSAGE"feed idle <feed no> <timeout in frames> <idle image file>\n"
		MESSAGE"feed info\n"
		MESSAGE"feed list [verbose]\n"
		MESSAGE"feed live <feed no>\n"
		MESSAGE"feed name <feed no> <feed name>\n"
		MESSAGE"feed par [<feed no> <scale_1> <scale_2>]\n"
		MESSAGE"feed recorded <feed no>\n"
		MESSAGE"feed scale [<feed no> <scale_1> <scale_2>]\n"
		MESSAGE"feed shift <feed no> <column> <row>\n"
		MESSAGE"feed socket <feed no> <socket name>\n"
		MESSAGE"\n");
	return 0;
}

int CVideoFeed::feed_info(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;

	// Count feed
	struct feed_type	*feed;
	timeval timenow;
	gettimeofday(&timenow, NULL);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"Feed info:\n"
		STATUS"Feed count : %u of %d\n"
		STATUS"time       : %u.%u\n"
		STATUS"feed id : state islive oneshot geometry cutstart cutsize offset fifo good missed dropped <name>\n",
		m_feed_count, (int)MAX_VIDEO_FEEDS, timenow.tv_sec, (u_int32_t)(timenow.tv_usec/1000)
		);
	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"feed %d : %s %s %s %ux%u %u,%u %dx%d %u,%u %d:%d %u %u %u <%s>\n",
			feed->id, feed_state_string(feed->state),
			feed->is_live ? "live" : "recorded", feed->oneshot ? "oneshot" : "continuously",
			feed->width, feed->height, feed->cut_start_column, feed->cut_start_row,
			feed->cut_columns, feed->cut_rows,
			feed->shift_columns, feed->shift_rows,
			feed->fifo_depth, feed->max_fifo,
			feed->good_frames, feed->missed_frames, feed->dropped_frames,
			feed->feed_name ? feed->feed_name : ""
			);
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}
/*
struct feed_type {
	struct feed_type	*prev;
	struct feed_type	*next;

	enum feed_state_enum	state;			// State of the feed. 
	enum feed_state_enum	previous_state;		// Previous state of the feed. 
	int			id;			// ID number used for identifying
							//   the feed to a controller. 
	struct timeval		start_time;		// Time feed changed to RUNNING
	char			*feed_name;		// Descriptive name of the feed. 
	int			oneshot;		// If true the feed is deleted on EOF. 
	int			is_live;		// True if the feed is a live feed
							//   (no pushback on source). 

	unsigned int		width;			// Width of this feed. 
	unsigned int		height;			// Height of this feed. 
	unsigned int		cut_start_column;	// Cutout region specifiers
							//   (for PIP or Mosaic display). 
	unsigned int		cut_start_row;
	int			cut_columns;
	int			cut_rows;
	int			par_w;			// Pixel Aspect Ratio Width 
	int			par_h;			// Pixel Aspect Ratio Height 
	int			scale_1;		// Scale image to scale_1/scale_2 
	int			scale_2;		// Scale image to scale_1/scale_2 

	unsigned int		shift_columns;		// Paint offset for PIP or Mosaic
							//   display. 
	unsigned int		shift_rows;

	char			*dead_img_name;		// Pointer to the name of the dead
							//   feed image. 
	u_int8_t		*dead_img_buffer;	// Pointer to the dead image. 
	int			dead_img_timeout;	// Limit to when to start displaying
							//   the dead image. 
	int			good_frames;		// Number of received good frames. 
	int			missed_frames;		// Number of missed frames. 
	int			dropped_frames;		// Number of fifo overruns. 

	char			*control_socket_name;	// Name of the unix domain control socket. 
	int			control_socket;		// Socket for talking to the sender shmsink. 
	int			connect_retries;	// Number of retries on connecting to the socket. 
	struct area_list_type	*area_list;		// List of areas mapped by this feed. 

	int			max_fifo;		// Max allowed number of fifo frames. 
	int			fifo_depth;		// Number of elements in the fifo. 
	struct frame_fifo_type	frame_fifo [MAX_FIFO_DEPTH];	// Pointer to the handle holding the last frames. 
	int			dequeued;		// Flag to show that a frame have been dequeued from this feed. 
*/

// feed add
int CVideoFeed::set_feed_add(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !ci) return 1;
	if (!m_pVideoMixer->m_geometry_width ||
		!m_pVideoMixer->m_frame_rate) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Setup geometry and frame rate first\n");
		return 1;
	}
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		struct feed_type	*feed;
		for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"feed %d <%s>\n", feed->id,
				feed->feed_name ? feed->feed_name : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;
	}

	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %99c", &no, &name [0]);
	if (x != 2) return -1;
	return feed_add_new (ctr, no, name);
}

// feed name
int CVideoFeed::set_feed_name(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	if (!m_pVideoMixer || ! m_pVideoMixer->m_pController) return 1;
	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %99c", &no, &name [0]);
	if (x != 2) return -1;
	return feed_set_name (ctr, no, name);
}

// feed par
int CVideoFeed::set_feed_par(struct controller_type* ctr, const char* ci)
{
	if (!(*ci)) return feed_list_par (ctr, 0);
	else {
		int no;
		int par_w, par_h;
		int x = sscanf(ci, "%d %d %d",
			&no, &par_w, &par_h);
		if (x != 3) return -1;
		return feed_set_par(ctr, no, par_w, par_h);
	}
}

// feed scale
int CVideoFeed::set_feed_scale(struct controller_type* ctr, const char* ci)
{
	if (!(*ci)) return feed_list_scales (ctr, 0);
	else {
		int no;
		int scale_1, scale_2;
		int x = sscanf(ci, "%d %d %d",
			&no, &scale_1, &scale_2);
		if (x != 3) return -1;
		return feed_set_scale(ctr, no, scale_1, scale_2);
	}
}

// feed socket
int CVideoFeed::set_feed_socket(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %s", &no, &name [0]);
	if (x != 2) return -1;
	return feed_set_socket (ctr, no, name);
}

// feed geometry
int CVideoFeed::set_feed_geometry(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	width;
	unsigned int	height;

	int x = sscanf (ci, "%d %u %u",
		&no, &width, &height);
	if (x != 3) return -1;
	return feed_set_geometry (ctr, no, width, height);
}

// feed idle
int CVideoFeed::set_feed_idle(struct controller_type* ctr, const char* ci)
{
	int	no;
	int	timeout;
	char	name [256];

	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %d %s",
		&no, &timeout, &name[0]);
	if (x != 3 && x != 2) return -1;
	return feed_set_idle (ctr, no, timeout, name);
}

// feed cutout
int CVideoFeed::set_feed_cutout(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	cut_start_column, cut_start_row, cut_columns, cut_rows;

	int x = sscanf (ci, "%d %u %u %u %u", &no,
		&cut_start_column, &cut_start_row, &cut_columns, &cut_rows);
	if (x != 5) return -1;
	return feed_set_cutout (ctr, no, cut_start_column, cut_start_row,
		cut_columns, cut_rows);
}

// feed shift
int CVideoFeed::set_feed_shift(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	shift_columns, shift_rows;

	int x = sscanf (ci, "%d %u %u", &no,
		&shift_columns, &shift_rows);
	if (x != 3) return -1;
	return feed_set_shift (ctr, no, shift_columns, shift_rows);
}

// feed live
int CVideoFeed::set_feed_live(struct controller_type* ctr, const char* ci)
{
	int	no;

	int x = sscanf (ci, "%d", &no);
	if (x != 1) return -1;
	return feed_set_live (ctr, no);
}

// feed recorded
int CVideoFeed::set_feed_recorded(struct controller_type* ctr, const char* ci)
{
	int	no;

	int x = sscanf (ci, "%d", &no);
	if (x != 1) return -1;
	return feed_set_recorded (ctr, no);
}

// Return current feed state
feed_state_enum* CVideoFeed::PreviousFeedState(int id)
{
        struct feed_type        *feed = NULL;
        for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
                if (feed->id == id) {
                        break;
                }
        }
	if (feed) return &(feed->previous_state);
	else return NULL;
}

// Return current feed state
feed_state_enum* CVideoFeed::FeedState(int id)
{
        struct feed_type        *feed = NULL;
        for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
                if (feed->id == id) {
                        break;
                }
        }
	if (feed) return &(feed->state);
	else return NULL;
}

/* Find a feed by its id number. */
struct feed_type* CVideoFeed::FindFeed (int id)
{
        struct feed_type        *feed;
	//if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return NULL;

        for (feed = m_pFeed_list; feed != NULL;
		feed = feed->next) {
                if (feed->id == id) {
                        return feed;
                }
        }
        return NULL;
}

/* Add a new feed to the system. */
int CVideoFeed::feed_add_new (struct controller_type *ctr, int id, char *name)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	/* First check if the controller no is unique. */
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (feed->id == id) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Feed ID %d already used\n", id);
			return -1;
		}
	}

	/* Setup a new feed structure. */
	trim_string (name);
	if (!(feed = (feed_type*) calloc (1, sizeof (*feed)))) {
		fprintf(stderr, "Failed to allocate memory for feed %d named %s\n", id, name);
		return -1;
	}
	feed->state          = SETUP;
	feed->previous_state = SETUP;
	feed->id             = id;
	feed->control_socket = -1;
	feed->max_fifo       = MAX_FIFO_DEPTH;	/* Default value. */
	feed->feed_name      = strdup (name);

	list_add_tail (m_pFeed_list, feed);
	m_feed_count++;

	return 0;
}

/* List scale of all feeds in the system*/
int CVideoFeed::feed_list_scales (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Feed ID %d  Scale: %2d:%-2d\n",
			feed->id, feed->scale_1, feed->scale_2);
	}
	return 0;
}

/* List par of all feeds in the system*/
int CVideoFeed::feed_list_par (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Feed ID %d  Pixel Aspect Ratio: %2d:%-2d\n",
			feed->id, feed->par_w, feed->par_h);
	}
	return 0;
}

/* List all feeds in the system. */
int CVideoFeed::feed_list_feeds (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;
	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_list_feeds\n");
		return -1;
	}

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (verbose == 0) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"Feed ID %d  Name: %s\n", feed->id,
				feed->feed_name ? feed->feed_name : "");
		} else {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"Feed ID %d\n",       feed->id);
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"  Name:       %s\n", feed->feed_name ?
					feed->feed_name : "");
			switch (feed->state) {
				case SETUP:        m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      SETUP\n"); break;
				case PENDING:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      PENDING\n"); break;
				case DISCONNECTED: m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      DISCONNECTED\n"); break;
				case RUNNING:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      RUNNING\n"); break;
				case STALLED:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      STALLED\n"); break;
				case READY:        m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      READY\n"); break;
				case UNDEFINED:    m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      UNDEFINED\n"); break;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Socket:     %s\n", feed->control_socket_name ? feed->control_socket_name : "N/A");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Fullscreen: %s\n", (unsigned) feed->width == m_pVideoMixer->m_geometry_width && (unsigned) feed->height == m_pVideoMixer->m_geometry_height ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Geometry:   %d %d\n", feed->width, feed->height);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Cutout:     %d %d %d %d\n", feed->cut_start_column, feed->cut_start_row,
										       feed->cut_columns, feed->cut_rows);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Shift:      %d %d\n", feed->shift_columns, feed->shift_rows);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Open:       %s\n", feed->control_socket >= 0 ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Is Live:    %s\n", feed->is_live ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Oneshot:    %s\n", feed->oneshot ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Idle Time:  %d\n", feed->dead_img_timeout);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Image file: %s\n", feed->dead_img_name ? feed->dead_img_name : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Frames:     %d\n", feed->good_frames);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Dropped:    %d\n", feed->dropped_frames);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Missed:     %d\n", feed->missed_frames);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");

	return 0;
}

/* Set the name of a feed. */
int CVideoFeed::feed_set_name (struct controller_type *ctr, int id, char *name)
{
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_name\n", id);
		return -1;
	}

	if (feed->feed_name) {
		free (feed->feed_name);
	}
	trim_string (name);
	feed->feed_name = strdup (name);
	
	return 0;
}


/* Set the input socket for a feed. */
int CVideoFeed::feed_set_socket (struct controller_type *ctr, int id, char *name)
{
	struct feed_type	*feed = FindFeed (id);

	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_set_socket\n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_socket\n", id);
		return -1;
	}

	if (feed->control_socket_name) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Socket already set.\n");
		return -1;
	}

	trim_string (name);
	feed->control_socket_name = strdup (name);

	/* This locks the geometry. */
	if (feed->width == 0 && feed->height == 0) {
		feed->width            = m_pVideoMixer->m_geometry_width;
		feed->height           = m_pVideoMixer->m_geometry_height;
		feed->cut_start_column = 0;
		feed->cut_start_row    = 0;
		feed->cut_columns      = m_pVideoMixer->m_geometry_width;
		feed->cut_rows         = m_pVideoMixer->m_geometry_height;
		feed->shift_columns    = 0;
		feed->shift_rows       = 0;
		feed->par_w            = 1;
		feed->par_h            = 1;
		feed->scale_1          = 1;
		feed->scale_2          = 1;
	}
	feed->previous_state = feed->state;
	feed->state = PENDING;
	feed->start_time.tv_sec = 0;
	feed->start_time.tv_usec = 0;

	return 0;
}


/* Set the idle timeout value and the timeout image. */
int CVideoFeed::feed_set_idle (struct controller_type *ctr, int id, int timeout, char *name)
{
	struct feed_type	*feed = FindFeed (id);
	uint8_t			*buffer;
	int			fd;

	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_idle\n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid feed no: %d in feed_set_idle\n", id);
		return -1;
	}
	if (timeout < 1) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid timeout value: %d\n", timeout);
		return -1;
	}

	/* This locks the geometry. */
	if (feed->width == 0 && feed->height == 0) {
		feed->width            = m_pVideoMixer->m_geometry_width;
		feed->height           = m_pVideoMixer->m_geometry_height;
		feed->cut_start_column = 0;
		feed->cut_start_row    = 0;
		feed->cut_columns      = m_pVideoMixer->m_geometry_width;
		feed->cut_rows         = m_pVideoMixer->m_geometry_height;
		feed->shift_columns    = 0;
		feed->shift_rows       = 0;
		feed->par_w            = 1;
		feed->par_h            = 1;
		feed->scale_1          = 1;
		feed->scale_2          = 1;
	}

	/* Try to open the new file. */
	trim_string (name);
	if (*name) {
		fd = open (name, O_RDONLY);
		if (fd < 0) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: Unable to open dead image file: \"%s\"\n", name);
			return -1;
		}

		/* Load the file into a empty buffer. */
		buffer = (u_int8_t*) calloc (1, m_pVideoMixer->m_pixel_bytes * feed->width *
			feed->height);
		memset(buffer, 1, m_pVideoMixer->m_pixel_bytes * feed->width * feed->height);
		if (name) {
			if (read (fd, buffer, m_pVideoMixer->m_block_size) < (signed)
				(m_pVideoMixer->m_pixel_bytes * feed->width * feed->height)) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: Unable to read file.\"%s\"\n", name);
				free (buffer);
				return -1;
			}
		}
		close(fd);

		/* Store the new name. */
		if (feed->dead_img_name) {
			free (feed->dead_img_name);
		}
		feed->dead_img_name = strdup (name ? name : "");
	
		/* Update the buffer pointer. */
		if (feed->dead_img_buffer) {
			free (feed->dead_img_buffer);
		}
		feed->dead_img_buffer = buffer;
	}
	feed->dead_img_timeout = timeout;

	return 0;
}


/* Set the geometry of the feed. */
int CVideoFeed::feed_set_geometry (struct controller_type *ctr, int id, unsigned int width, unsigned int height)
{
	struct feed_type	*feed = FindFeed (id);
	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_set_geometry<n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_geometry\n", id);
		return -1;
	}
	if (width < 1 || width > m_pVideoMixer->m_geometry_width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid width value: %d\n", width);
		return -1;
	}
	if (height < 1 || height > m_pVideoMixer->m_geometry_height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid height value: %d\n", height);
		return -1;
	}
	if (feed->width > 0 || feed->height > 0) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Can't change geometry. It's already set.\n");
		return -1;
	}

	// fprintf(stderr, "VideoScaler (%dx%d) for feed %d named %s\n",
	//	width, height, feed->id, feed->feed_name);

	feed->width            = width;
	feed->height           = height;
	feed->cut_start_column = 0;
	feed->cut_start_row    = 0;
	feed->cut_columns      = width;
	feed->cut_rows         = height;
	feed->shift_columns    = 0;
	feed->shift_rows       = 0;
	feed->par_w            = 1;
	feed->par_h            = 1;
	feed->scale_1          = 1;
	feed->scale_2          = 1;

	return 0;
}

/* Set the feed scale */
int CVideoFeed::feed_set_scale (struct controller_type *ctr, int id, int scale_1, int scale_2)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid feed no: %d in feed_set_scale\n", id);
		return -1;
	}
	if (scale_1 == scale_2) scale_1 = scale_2 = 1;
	do {
		if ((scale_1 & 1) || (scale_2 & 1)) break;
		scale_1 = scale_1 >> 1;
		scale_2 = scale_2 >> 1;
	} while (1);
	if (m_pVideoMixer->m_pController->m_verbose)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"Feed %d scaled %d:%d\n",
			feed->id, scale_1, scale_2);
	if (scale_1 < 1 || scale_2 < 1 ||
	    (scale_1 == 1 && scale_2 > 5)) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid scale value %d %d. Valid are 1 1, 1 2, 1 3, "
			"1 4, 1 5, 2 3\n", scale_1, scale_2);
		return -1;
	}
	feed->scale_1 = scale_1;
	feed->scale_2 = scale_2;
	return 0;
}

/* Set the feed par */
int CVideoFeed::feed_set_par (struct controller_type *ctr, int id, int par_w, int par_h)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_par\n", id);
		return -1;
	}
	if (par_w == par_h || (par_w < 1) || (par_h < 1)) par_w = par_h = 1;
	do {
		if ((par_w & 1) || (par_h & 1)) break;
		par_w = par_w >> 1;
		par_h = par_h >> 1;
	} while (1);
	fprintf(stderr, "Feed %d par %d:%d\n", feed->id, par_w, par_h);
	feed->par_w = par_w;
	feed->par_h = par_h;
	return 0;
}


/* Set the cutout area of the feed. */
int CVideoFeed::feed_set_cutout (struct controller_type *ctr, int id, unsigned int cut_start_column, unsigned int cut_start_row, unsigned int cut_columns, unsigned int cut_rows)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_cutout\n", id);
		return -1;
	}
	if (cut_start_column < 0 || cut_start_column >= feed->width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid start column value: 0 < %d < %d\n",
					    cut_start_column, feed->width);
		return -1;
	}
	if (cut_start_row < 0 || cut_start_row >= feed->height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid start row value: 0 < %d < %d\n",
					   cut_start_row, feed->height);
		return -1;
	}
	if (cut_columns < 0 || cut_columns > feed->width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid cut width: 0 < %d < %d\n",
					   cut_columns, feed->width);
		return -1;
	}
	if (cut_rows < 0 || cut_rows > feed->height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid cut height: 0 < %d < %d\n",
					   cut_rows, feed->height);
		return -1;
	}

	feed->cut_start_column = cut_start_column;
	feed->cut_start_row    = cut_start_row;
	feed->cut_columns      = cut_columns;
	feed->cut_rows         = cut_rows;

	return 0;
}


/* Set the shifted position of the feed. */
int CVideoFeed::feed_set_shift (struct controller_type *ctr, int id, unsigned int shift_columns, unsigned int shift_rows)
{
	struct feed_type	*feed = FindFeed (id);

	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_shift\n");
		return 1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_shift\n", id);
		return -1;
	}
	if (shift_columns < 0 || shift_columns >= m_pVideoMixer->m_geometry_width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid shift column value: 0 < %d < %d\n",
					   shift_columns, m_pVideoMixer->m_geometry_width);
		return -1;
	}
	if (shift_rows < 0 || shift_rows >= m_pVideoMixer->m_geometry_height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid shift row value: 0 < %d < %d\n",
					   shift_rows, m_pVideoMixer->m_geometry_height);
		return -1;
	}

	feed->shift_columns = shift_columns;
	feed->shift_rows    = shift_rows;

	return 0;
}


/* Set the feed to be a live feed. */
int CVideoFeed::feed_set_live (struct controller_type *ctr, int id)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_live\n", id);
		return -1;
	}

	feed->is_live = 1;

	return 0;
}


/* Set the feed to be a recorded feed. */
int CVideoFeed::feed_set_recorded (struct controller_type *ctr, int id)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_recorded\n", id);
		return -1;
	}

	feed->is_live = 0;

	return 0;
}


/* Dump buffer info for all the feeds. */
int CVideoFeed::feed_dump_buffers (struct controller_type *ctr)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		struct area_list_type	*area;

		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"DBG: Feed %d %s\n", feed->id, feed->feed_name);
		for (area = feed->area_list; area != NULL; area = area->next) {
			struct buffer_type	*buffer;

			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"DBG:   Area ID: %d  Blocks: ", area->area_id);
			for (buffer = area->buffers; buffer != NULL; buffer = buffer->next) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%lu ", buffer->offset / buffer->bsize);
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n");
		}
	}
	return 0;
}

/* Select the currently streamed feed. */
int CVideoFeed::output_select_feed (struct controller_type *ctr, int no)
{
	struct feed_type		*feed;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) {
		fprintf(stderr, "pVideoMixer was NULL in output_select_feed\n");
		return 1;
	}

	feed = FindFeed (no);
	if (feed == NULL) {
		/* Unknown feed. */
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Unknown feed: %d\n", no);
		return -1;
	}
	if (feed->width  != m_pVideoMixer->m_geometry_width ||
	    feed->height != m_pVideoMixer->m_geometry_height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Not a full-screen feed\n");
		return -1;
	}
	m_pVideoMixer->m_output_stack [0] = no;
	m_pVideoMixer->m_output_stack [1] = -1;

	/* Let everyone know that a new feed have been selected. */
	m_pVideoMixer->m_pController->controller_write_msg (NULL, "Selected %d %s\n", no, feed->feed_name);

	return 0;
}
