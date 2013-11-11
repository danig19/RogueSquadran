/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_FEED_H__
#define __VIDEO_FEED_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <png.h>
//#include <video_image.h>

#include "snowmix.h"
#include "controller.h"
#include "video_mixer.h"

#define MAX_VIDEO_FEEDS 12

// Struct to hold feed parameters
struct feed_type {
	struct feed_type	*prev;
	struct feed_type	*next;

	enum feed_state_enum	state;			/* State of the feed. */
	enum feed_state_enum	previous_state;		/* Previous state of the feed. */
	int			id;			/* ID number used for identifying
							   the feed to a controller. */
	struct timeval		start_time;		// Time feed changed to RUNNING
	char			*feed_name;		/* Descriptive name of the feed. */
	int			oneshot;		/* If true the feed is deleted on EOF. */
	int			is_live;		/* True if the feed is a live feed
							   (no pushback on source). */

	unsigned int		width;			/* Width of this feed. */
	unsigned int		height;			/* Height of this feed. */
	unsigned int		cut_start_column;	/* Cutout region specifiers
							   (for PIP or Mosaic display). */
	unsigned int		cut_start_row;
	int			cut_columns;
	int			cut_rows;
	int			par_w;			/* Pixel Aspect Ratio Width */
	int			par_h;			/* Pixel Aspect Ratio Height */
	int			scale_1;		/* Scale image to scale_1/scale_2 */
	int			scale_2;		/* Scale image to scale_1/scale_2 */

	unsigned int		shift_columns;		/* Paint offset for PIP or Mosaic
							   display. */
	unsigned int		shift_rows;

	char			*dead_img_name;		/* Pointer to the name of the dead
							   feed image. */
	u_int8_t		*dead_img_buffer;	/* Pointer to the dead image. */
	int			dead_img_timeout;	/* Limit to when to start displaying
							   the dead image. */
	int			good_frames;		/* Number of received good frames. */
	int			missed_frames;		/* Number of missed frames. */
	int			dropped_frames;		/* Number of fifo overruns. */

	char			*control_socket_name;	/* Name of the unix domain control socket. */
	int			control_socket;		/* Socket for talking to the sender shmsink. */
	int			connect_retries;	/* Number of retries on connecting to the socket. */
	struct area_list_type	*area_list;		/* List of areas mapped by this feed. */

	int			max_fifo;		/* Max allowed number of fifo frames. */
	int			fifo_depth;		/* Number of elements in the fifo. */
	struct frame_fifo_type	frame_fifo [MAX_FIFO_DEPTH];	/* Pointer to the handle holding the last frames. */
	int			dequeued;		/* Flag to show that a frame have been dequeued from this feed. */
};

class CVideoFeed {
  public:
	CVideoFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds = MAX_VIDEO_FEEDS);
	~CVideoFeed();
	u_int32_t		MaxFeeds() { return m_max_feeds; }

	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	//int			UpdateMove(u_int32_t id);
	int 			output_select_feed (struct controller_type *ctr, int no);
	feed_state_enum*	PreviousFeedState(int id);
	feed_state_enum*	FeedState(int id);
	struct feed_type* 	FindFeed (int id);
	struct feed_type*	GetFeedList() { return m_pFeed_list; }
	void			SetFeedList(struct feed_type* pFeed) { m_pFeed_list = pFeed; }

/*
	//virtual_feed_enum_t GetFeedSourceType(u_int32_t id);
	//int		GetFeedSourceId(u_int32_t id);
	char*		GetFeedName(u_int32_t id);
	int		GetFeedGeometry(u_int32_t id, u_int32_t* w, u_int32_t* h);
	int		GetFeedCoordinates(u_int32_t id, int32_t* x, int32_t* y);
	int		GetFeedClip(u_int32_t id, u_int32_t* clip_x, u_int32_t* clip_y,
				u_int32_t* clip_w, u_int32_t* clip_h);
	//int		GetFeedScale(u_int32_t id, double* scale_x, double* scale_y);
	//int		GetFeedRotation(u_int32_t id, double* rotation);
	//int		GetFeedAlpha(u_int32_t id, double* rotation);
*/

private:
/*
	int		CreateFeed(u_int32_t id);
	int		DeleteFeed(u_int32_t id);

	int		SetFeedName(u_int32_t id, const char* feed_name);

	int		SetFeedSource(u_int32_t id, feed_type* pFeed);
	int		SetFeedSource(u_int32_t id, image_item_t* pImage);

	//int		SetFeedGeometry(u_int32_t id, u_int32_t width, u_int32_t height);

	int		SetFeedScale(u_int32_t id, double scale_x, double scale_y);
	int		SetFeedScaleDelta(u_int32_t id, double delta_scale_x,
				double delta_scale_y, u_int32_t delta_scale_x_steps,
				u_int32_t delta_scale_y_steps);

	int		SetFeedRotation(u_int32_t id, double rotation);
	int		SetFeedRotationDelta(u_int32_t id, double delta_rotation,
				u_int32_t delta_rotation_steps);

	int		SetFeedAlpha(u_int32_t id, double rotation);
	int		SetFeedAlphaDelta(u_int32_t id, double delta_alpha,
				u_int32_t delta_alpha_steps);

	int		PlaceFeed(u_int32_t id, int32_t* x, int32_t* y,
				u_int32_t* clip_x = NULL, u_int32_t* clip_y = NULL,
				u_int32_t* clip_w = NULL, u_int32_t* clip_h = NULL,
				double* rotation = NULL, double* scale_x = NULL,
				double* scale_y = NULL, double* alpha = NULL);

	int		SetFeedCoordinates(u_int32_t id, int32_t x, int32_t y);
	int		SetFeedCoordinatesDelta(u_int32_t id, int32_t delta_x, int32_t delta_y,
				int32_t steps_x, int32_t steps_y);

	int		SetFeedClip(u_int32_t id, u_int32_t clip_x, u_int32_t clip_y,
				u_int32_t clip_w, u_int32_t clip_h);
	int		SetFeedClipDelta(u_int32_t id,
				int32_t delta_clip_x, int32_t delta_clip_y,
				int32_t delta_clip_w, int32_t delta_clip_h,
				u_int32_t delta_clip_x_steps, u_int32_t delta_clip_y_steps,
				u_int32_t delta_clip_w_steps, u_int32_t delta_clip_h_steps);

	//virtual_feed_t*	FindFeedByNumber(u_int32_t id);
	int		set_virtual_feed_add(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_move_clip(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_move_alpha(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_move_rotation(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_move_scale(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_move_coor(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_place_rect(struct controller_type* ctr, const char* str);
	int		set_virtual_feed_source(struct controller_type* ctr, const char* str);
*/


private:
	int 		list_help(struct controller_type* ctr, const char* str);
	int 		feed_info(struct controller_type* ctr, const char* str);
	int 		set_feed_add(struct controller_type* ctr, const char* ci);
	int 		set_feed_name(struct controller_type* ctr, const char* ci);
	int 		set_feed_par(struct controller_type* ctr, const char* ci);
	int 		set_feed_scale(struct controller_type* ctr, const char* ci);
	int 		set_feed_socket(struct controller_type* ctr, const char* ci);
	int 		set_feed_geometry(struct controller_type* ctr, const char* ci);
	int 		set_feed_idle(struct controller_type* ctr, const char* ci);
	int 		set_feed_cutout(struct controller_type* ctr, const char* ci);
	int 		set_feed_shift(struct controller_type* ctr, const char* ci);
	int 		set_feed_live(struct controller_type* ctr, const char* ci);
	int 		set_feed_recorded(struct controller_type* ctr, const char* ci);
	int 		feed_add_new (struct controller_type *ctr, int id, char *name);
	int 		feed_list_scales (struct controller_type *ctr, int verbose);
	int 		feed_list_par (struct controller_type *ctr, int verbose);
	int 		feed_list_feeds (struct controller_type *ctr, int verbose);
	int 		feed_set_name (struct controller_type *ctr, int id, char *name);
	int 		feed_set_socket (struct controller_type *ctr, int id, char *name);
	int 		feed_set_idle (struct controller_type *ctr, int id, int timeout, char *name);
	int 		feed_set_geometry (struct controller_type *ctr, int id, unsigned int width, unsigned int height);
	int 		feed_set_scale (struct controller_type *ctr, int id, int scale_1, int scale_2);
	int 		feed_set_par (struct controller_type *ctr, int id, int par_w, int par_h);
	int 		feed_set_cutout (struct controller_type *ctr, int id, unsigned int cut_start_column, unsigned int cut_start_row, unsigned int cut_columns, unsigned int cut_rows);
	int 		feed_set_shift (struct controller_type *ctr, int id, unsigned int shift_columns, unsigned int shift_rows);
	int 		feed_set_live (struct controller_type *ctr, int id);
	int 		feed_set_recorded (struct controller_type *ctr, int id);
	int 		feed_dump_buffers (struct controller_type *ctr);

	u_int32_t		m_max_feeds;
	//virtual_feed_t**	m_feeds;
	struct feed_type*	m_pFeed_list;
	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_feed_count;
};
	
#endif	// VIDEO_FEED_H
