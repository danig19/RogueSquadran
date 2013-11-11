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
#include <ctype.h>
//#include <limits.h>
//#include <string.h>
#include <malloc.h>
//#include <stdlib.h>
//#include <png.h>
//#include <sys/types.h>
#include <math.h>
//#include <string.h>

//#include "cairo/cairo.h"
#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "virtual_feed.h"

CVirtualFeed::CVirtualFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds)
{
	m_max_feeds = 0;
	m_feeds = (virtual_feed_t**) calloc(
		sizeof(virtual_feed_t*), max_feeds);
	//for (unsigned int i=0; i< m_max_feeds; i++) m_feeds[i] = NULL;
	if (m_feeds) {
		m_max_feeds = max_feeds;
	}
	m_pVideoMixer = pVideoMixer;
}

CVirtualFeed::~CVirtualFeed()
{
	if (m_feeds) {
		for (unsigned int i=0; i<m_max_feeds; i++) {
			if (m_feeds[i]) {
				if (m_feeds[i]->name)
					free(m_feeds[i]->name);
				free(m_feeds[i]);
			}
		}
		free(m_feeds);
		m_feeds = NULL;
	}
	m_max_feeds = 0;
}

int CVirtualFeed::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer) return 0;

	if (!strncasecmp (str, "add ", strlen ("add "))) {
		if (set_virtual_feed_add(ctr, str+4)) return -1;
	} else if (!strncasecmp (str, "source ", 7)) {
		if (set_virtual_feed_source(ctr, str+7)) return -1;
	// virtual feed place rect [<vir id> [<x> <y> <width> <height> <src_x> <src_y>
	//	[<rotation> <scale_x> <scale_y> <alpha>]]]
	} else if (!strncasecmp (str, "place rect ", 11)) {
		if (set_virtual_feed_place_rect(ctr, str+11)) return -1;
	} else if (!strncasecmp (str, "move coor ", 10)) {
		if (set_virtual_feed_move_coor(ctr, str+10)) return -1;
	} else if (!strncasecmp (str, "move clip ", 10)) {
		if (set_virtual_feed_move_clip(ctr, str+10)) return -1;
	} else if (!strncasecmp (str, "move scale ", 11)) {
		if (set_virtual_feed_move_scale(ctr, str+11)) return -1;
	} else if (!strncasecmp (str, "move rotation ", 14)) {
		if (set_virtual_feed_move_rotation(ctr, str+14)) return -1;
	} else if (!strncasecmp (str, "move alpha ", 11)) {
		if (set_virtual_feed_move_alpha(ctr, str+11)) return -1;
	} else if (!strncasecmp (str, "place matrix ", 13)) {
		if (set_virtual_feed_matrix(ctr, str+13)) return -1;
	} else if (!strncasecmp (str, "help ", 5)) {
		if (list_help(ctr, str+5)) return -1;
	} else return 1;
	return 0;
}

// Create a virtual feed
int CVirtualFeed::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Commands:\n"
		"virtual feed add [<vir id> [<feed name>]]  // empty <feed name> deletes entry\n"
		"virtual feed source [(feed | image) <vir id> (<feed id> | <image id>)]\n"
		"virtual feed geometry <vir id> <width> <height>\n"
		"virtual feed place rect [<vir id> [<x> <y> <width> <height> <src_x> <src_y> "
			"[<rotation> <scale_x> <scale_y> <alpha>]]]\n"
//		"virtual feed place circle <vir id> <x> <y> <radius> <src_x> <src_y> "
//			"<scale_x> <scale_y> <alpha>\n"
		"virtual feed move coor <vir id> <delta x> <delta y> <step x> <step y>\n"
		"virtual feed move clip <vir id> <delta clip x> <delta clip y> <delta clip w> "
			" <delta clip h> <step x> <step y> <step w> <step y>\n"
		"virtual feed move scale <vir id> <delta sx> <delta sy> <step sx> <step sy>\n"
		"virtual feed move alpha <vir id> <delta a> <step a>\n"
		);
	return 0;
}

// Create a virtual feed
int CVirtualFeed::set_virtual_feed_add(struct controller_type* ctr, const char* str)
{
	int id, n;

	if (!m_feeds || !str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;

	while (isspace(*str)) str++;
	if (*str == '\0') {
		
		for (id = 0; (unsigned) id < m_max_feeds ; id++) if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"virtual feed %2u : <%s>\n",
				id, m_feeds[id]->name ? m_feeds[id]->name : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}

	// Get the feed id.
	n = sscanf(str, "%u", &id);
	if (n != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;

	// Now if eos, only id was given and we delete the feed
	if (!(*str)) return (DeleteFeed(id));

	// Now we know there is more after the id and we can create the feed
	if (CreateFeed(id)) return -1;
	return SetFeedName(id, str);
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_clip(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// virtual feed move clip <vir id> <delta_clip_x> <delta_clip_y> <delta_clip_w>
	//	<delta_clip_h> <steps_x> <steps_y> <steps_w> <steps_h>
	int32_t delta_clip_x, delta_clip_y, delta_clip_w, delta_clip_h;
	u_int32_t id, step_x, step_y, step_w, step_h;
	int n = sscanf(str, "%u %d %d %d %d %u %u %u %u", &id, &delta_clip_x, &delta_clip_y, &delta_clip_w, &delta_clip_h, &step_x, &step_y, &step_w, &step_h);
	if (n == 9)
		return SetFeedClipDelta(id, delta_clip_x, delta_clip_y, delta_clip_w, delta_clip_h,
			step_x, step_y, step_w, step_h);
	return -1;
}

// Set virtual feed matrix params
int CVirtualFeed::set_virtual_feed_matrix(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// virtual feed matrix <vir id> <xx> <xy> <yx> <yy> <x0> <y0>
	double xx, xy, yx, yy, x0, y0;
	u_int32_t id;
	int n = sscanf(str, "%u %lf %lf %lf %lf %lf %lf", &id, &xx, &xy, &yx, &yy, &x0, &y0);
	if (n == 7)
		return SetFeedMatrix(id, xx, xy, yx, yy, x0, y0);
	return -1;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_alpha(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// place rect <vir id> <delta_alpha> <step_alpha>
	double delta_alpha;
	u_int32_t id, step_alpha;
	int n = sscanf(str, "%u %lf %u", &id, &delta_alpha, &step_alpha);
	if (n == 3) return SetFeedAlphaDelta(id, delta_alpha, step_alpha);
	return -1;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_rotation(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// place rect <vir id> <delta_rotation> <step_rotation>
	double delta_rotation;
	u_int32_t id, step_rotation;
	int n = sscanf(str, "%u %lf %u", &id, &delta_rotation, &step_rotation);
	if (n == 3) return SetFeedRotationDelta(id, delta_rotation, step_rotation);
	return -1;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_scale(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// place rect <vir id> <scale_x> <scale_y> <step_x> <step_y>
	double delta_scale_x, delta_scale_y;
	u_int32_t id, step_x, step_y;
	int n = sscanf(str, "%u %lf %lf %u %u", &id, &delta_scale_x, &delta_scale_y, &step_x, &step_y);
	if (n == 5) return SetFeedScaleDelta(id, delta_scale_x, delta_scale_y, step_x, step_y);
	return -1;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_coor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// place rect <vir id> <delta_x> <delta_y> <step_x> <step_y>
	int32_t delta_x, delta_y;
	u_int32_t id, step_x, step_y;
	int n = sscanf(str, "%u %d %d %u %u", &id, &delta_x, &delta_y, &step_x, &step_y);
	if (n == 5) return SetFeedCoordinatesDelta(id, delta_x, delta_y, step_x, step_y);
	return -1;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_place_rect(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// place rect <vir id> <x> <y> <width> <height> <src_x> <src_y> <rotation> <scale_x> <scale_y> <alpha>
	int32_t x, y;
	u_int32_t id, w, h, sx, sy;
	double rotation, scx, scy, alpha;
	int n = sscanf(str, "%u %d %d %u %u %u %u %lf %lf %lf %lf",
		&id, &x, &y, &sx, &sy, &w, &h, &rotation, &scx, &scy, &alpha);
	if (n == 1) return DeleteFeed(id);
	if (n == 7) return PlaceFeed(id, &x, &y, &w, &h, &sx, &sy);
	else if (n == 11) return PlaceFeed(id, &x, &y, &w, &h, &sx, &sy, &rotation, &scx, &scy, &alpha);
	return -1;
}

// Set virtual feed source
int CVirtualFeed::set_virtual_feed_source(struct controller_type* ctr, const char* str)
{
	if (!str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		for (u_int32_t id=0; id < m_max_feeds; id++) if (m_feeds[id]) {
			feed_type* feed = NULL;
			if (m_feeds[id]->pFeed)
				feed = m_pVideoMixer->m_pVideoFeed->FindFeed(m_feeds[id]->source_id);
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"virtual feed %u : source %s %u %ux%u %s\n",
				id, m_feeds[id]->pFeed ? "feed" :
				  m_feeds[id]->pImage ? "image" : "unknown",
				m_feeds[id]->source_id,
				m_feeds[id]->width,
				m_feeds[id]->height,
				feed ? feed_state_string(feed->state) : "STILL");

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"\n");
		return 0;
	}
	int vir_id, source_id;
	if (sscanf(str, "feed %u %u", &vir_id, &source_id) == 2) {
		if (!m_pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "pVideoFeed is NULL for set_virtual_feed_source\n");
			return -1;
		}
		feed_type* feed = m_pVideoMixer->m_pVideoFeed->FindFeed(source_id);
		if (!feed) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: Failed to find feed %d for virtual feed\n", source_id);
			return -1;
		}
		return SetFeedSource(vir_id, feed);
	} else if (sscanf(str, "image %u %u", &vir_id, &source_id) == 2) {
		image_item_t* pImage = NULL;
		if (!m_pVideoMixer->m_pVideoImage ||
			!(pImage = m_pVideoMixer->m_pVideoImage->GetImageItem(source_id))) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: Failed to find image %d for virtual feed.\n");
			return -1;
		}
		return SetFeedSource(vir_id, pImage);
	}
	return -1;
}

int CVirtualFeed::CreateFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds) return -1;
	virtual_feed_t* p = m_feeds[id];
	if (!p) p = (virtual_feed_t*) calloc(sizeof(virtual_feed_t),1);
	if (!p) return -1;
	m_feeds[id]		= p;
	p->name			= NULL;
	p->id			= 0;
	p->source_id		= 0;
	p->pFeed		= NULL;
	p->pImage		= NULL;
	p->width		= 0;
	p->height		= 0;
	p->scale_x		= 1.0;
	p->scale_y		= 1.0;
	p->x			= 0;
	p->y			= 0;
	p->clip_x		= 0;
	p->clip_y		= 0;
	p->clip_w		= 0;
	p->clip_h		= 0;
	p->rotation		= 0.0;
	p->alpha		= 1.0;

	p->delta_x		= 0;
	p->delta_y		= 0;
	p->delta_clip_x		= 0;
	p->delta_clip_y		= 0;
	p->delta_clip_w		= 0;
	p->delta_clip_h		= 0;
	p->delta_scale_x	= 0.0;
	p->delta_scale_x	= 0.0;
	p->delta_rotation	= 0.0;
	p->delta_alpha		= 0.0;

	p->delta_x_steps	= 0;
	p->delta_y_steps	= 0;
	p->delta_clip_x_steps	= 0;
	p->delta_clip_y_steps	= 0;
	p->delta_clip_w_steps	= 0;
	p->delta_clip_h_steps	= 0;
	p->delta_scale_x_steps	= 0;
	p->delta_scale_x_steps	= 0;
	p->delta_rotation_steps	= 0;
	p->delta_alpha_steps	= 0;

	p->pMatrix	= NULL;

/*
	p->matrix_xx	= 1.0;
	p->matrix_yx	= 0.0;
	p->matrix_xy	= 0.0;
	p->matrix_yy	= 1.0;
	p->matrix_x0	= 0.0;
	p->matrix_y0	= 0.0;
*/

	return 0;
}
int CVirtualFeed::DeleteFeed(u_int32_t id)
{
fprintf(stderr, "Delete feed %d\n",id);
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
fprintf(stderr, " Deleting feed %d\n",id);
	if (m_feeds[id]->name) free(m_feeds[id]->name);
	if (m_feeds[id]) free(m_feeds[id]);
	m_feeds[id] = NULL;
fprintf(stderr, " Deleted feed %d\n",id);
	return 0;
}

int CVirtualFeed::SetFeedName(u_int32_t id, const char* feed_name)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !feed_name)
		return -1;
	m_feeds[id]->name = strdup(feed_name);
	return m_feeds[id]->name ? 0 : -1;
}
	
char* CVirtualFeed::GetFeedName(u_int32_t id)
{
	return (id >= m_max_feeds || !m_feeds || !m_feeds[id]) ?
		NULL : m_feeds[id]->name;
}

int CVirtualFeed::SetFeedSource(u_int32_t id, feed_type* pFeed)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !pFeed)
		return -1;
	virtual_feed_t* p = m_feeds[id];
	p->pImage = NULL;
	p->source_id = pFeed->id;
	p->width = pFeed->width;
	p->height = pFeed->height;
	p->pFeed = pFeed;
	return 0;
}
int CVirtualFeed::SetFeedSource(u_int32_t id, image_item_t* pImage)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !pImage)
		return -1;
	virtual_feed_t* p = m_feeds[id];
	p->pFeed = NULL;
	p->source_id = pImage->id;
	p->width = pImage->width;
	p->height = pImage->height;
	p->pImage = pImage;
	return 0;
}

int CVirtualFeed::GetFeedSourceId(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return -1;
	return m_feeds[id]->source_id;
}
virtual_feed_enum_t CVirtualFeed::GetFeedSourceType(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return VIRTUAL_FEED_NONE;
	return m_feeds[id]->pFeed ? VIRTUAL_FEED_FEED :
		 (m_feeds[id]->pImage ?  VIRTUAL_FEED_IMAGE :
			VIRTUAL_FEED_NONE);
}
int CVirtualFeed::SetFeedScale(u_int32_t id, double scale_x, double scale_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		scale_x < 0.0 || scale_y < 0.0) return -1;
	m_feeds[id]->scale_x = scale_x;
	m_feeds[id]->scale_y = scale_y;
	return 0;
}
int CVirtualFeed::GetFeedScale(u_int32_t id, double* scale_x, double* scale_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!scale_x || !scale_y) return -1;
	*scale_x = m_feeds[id]->scale_x;
	*scale_y = m_feeds[id]->scale_y;
	return 0;
}
int CVirtualFeed::SetFeedScaleDelta(u_int32_t id, double delta_scale_x, double delta_scale_y, u_int32_t delta_scale_x_steps, u_int32_t delta_scale_y_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_scale_x = delta_scale_x;
	m_feeds[id]->delta_scale_y = delta_scale_y;
	m_feeds[id]->delta_scale_x_steps = delta_scale_x_steps;
	m_feeds[id]->delta_scale_y_steps = delta_scale_y_steps;
	return 0;
}

int CVirtualFeed::SetFeedRotation(u_int32_t id, double rotation)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->rotation = rotation;
	return 0;
}
int CVirtualFeed::GetFeedRotation(u_int32_t id, double* rotation)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !rotation) return -1;
	*rotation = m_feeds[id]->rotation;
	return 0;
}
int CVirtualFeed::SetFeedRotationDelta(u_int32_t id, double delta_rotation, u_int32_t delta_rotation_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_rotation = delta_rotation;
	m_feeds[id]->delta_rotation_steps = delta_rotation_steps;
	return 0;
}
int CVirtualFeed::SetFeedAlpha(u_int32_t id, double alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->alpha = alpha;
	return 0;
}
int CVirtualFeed::GetFeedAlpha(u_int32_t id, double* alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !alpha) return -1;
	*alpha = m_feeds[id]->alpha;
	return 0;
}
transform_matrix_t* CVirtualFeed::GetFeedMatrix(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ) return NULL;
	return m_feeds[id]->pMatrix;
}
int CVirtualFeed::SetFeedAlphaDelta(u_int32_t id, double delta_alpha, u_int32_t delta_alpha_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_alpha = delta_alpha;
	m_feeds[id]->delta_alpha_steps = delta_alpha_steps;
	return 0;
}
int CVirtualFeed::PlaceFeed(u_int32_t id, int32_t* x, int32_t* y,
	u_int32_t* clip_x, u_int32_t* clip_y,
	u_int32_t* clip_w, u_int32_t* clip_h,
	double* rotation, double* scale_x, double* scale_y, double* alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	virtual_feed_t* p = m_feeds[id];
	if (x) p->x = *x;
	if (y) p->y = *y;
	if (clip_x) p->clip_x = *clip_x;
	if (clip_y) p->clip_y = *clip_y;
	if (clip_w) p->clip_w = *clip_w;
	if (clip_h) p->clip_h = *clip_h;
	if (rotation) p->rotation = *rotation;
	if (scale_x) p->scale_x = *scale_x;
	if (scale_y) p->scale_y = *scale_y;
	if (alpha) p->alpha = *alpha;
	return 0;
}
int CVirtualFeed::GetFeedGeometry(u_int32_t id, u_int32_t* w, u_int32_t* h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (w) *w = m_feeds[id]->width;
	if (h) *h = m_feeds[id]->height;
	return 0;
}
int CVirtualFeed::SetFeedCoordinates(u_int32_t id, int32_t x, int32_t y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->x = x;
	m_feeds[id]->y = y;
	return 0;
}
int CVirtualFeed::GetFeedCoordinates(u_int32_t id, int32_t* x, int32_t* y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (x) *x = m_feeds[id]->x;
	if (y) *y = m_feeds[id]->y;
	return 0;
}
int CVirtualFeed::SetFeedCoordinatesDelta(u_int32_t id, int32_t delta_x, int32_t delta_y,
	int32_t steps_x, int32_t steps_y )
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_x = delta_x;
	m_feeds[id]->delta_y = delta_y;
	m_feeds[id]->delta_x_steps = steps_x;
	m_feeds[id]->delta_y_steps = steps_y;
	return 0;
}

int CVirtualFeed::SetFeedClip(u_int32_t id, u_int32_t clip_x, u_int32_t clip_y,
	u_int32_t clip_w, u_int32_t clip_h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!clip_w || !clip_h) return -1;
	m_feeds[id]->clip_x = clip_x;
	m_feeds[id]->clip_y = clip_y;
	m_feeds[id]->clip_w = clip_w;
	m_feeds[id]->clip_h = clip_h;
	return 0;
}
int CVirtualFeed::GetFeedClip(u_int32_t id,
	u_int32_t* clip_x, u_int32_t* clip_y,
	u_int32_t* clip_w, u_int32_t* clip_h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (clip_x) *clip_x = m_feeds[id]->clip_x;
	if (clip_y) *clip_y = m_feeds[id]->clip_y;
	if (clip_w) *clip_w = m_feeds[id]->clip_w;
	if (clip_h) *clip_h = m_feeds[id]->clip_h;
	return 0;
}
int CVirtualFeed::SetFeedMatrix(u_int32_t id, double xx, double xy, double yx, double yy, double x0, double y0)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (!m_feeds[id]->pMatrix)
		m_feeds[id]->pMatrix = (transform_matrix_t*)
			calloc(sizeof(transform_matrix_t), 1);
	if (!m_feeds[id]->pMatrix) return 1;
	m_feeds[id]->pMatrix->matrix_xx = xx;
	m_feeds[id]->pMatrix->matrix_xy = xy;
	m_feeds[id]->pMatrix->matrix_yx = yx;
	m_feeds[id]->pMatrix->matrix_yy = yy;
	m_feeds[id]->pMatrix->matrix_x0 = x0;
	m_feeds[id]->pMatrix->matrix_y0 = y0;
	return 0;
}
int CVirtualFeed::SetFeedClipDelta(u_int32_t id,
	int32_t delta_clip_x, int32_t delta_clip_y,
	int32_t delta_clip_w, int32_t delta_clip_h,
	u_int32_t delta_clip_x_steps, u_int32_t delta_clip_y_steps,
	u_int32_t delta_clip_w_steps, u_int32_t delta_clip_h_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_clip_x = delta_clip_x;
	m_feeds[id]->delta_clip_y = delta_clip_y;
	m_feeds[id]->delta_clip_w = delta_clip_w;
	m_feeds[id]->delta_clip_h = delta_clip_h;
	m_feeds[id]->delta_clip_x_steps = delta_clip_x_steps;
	m_feeds[id]->delta_clip_y_steps = delta_clip_y_steps;
	m_feeds[id]->delta_clip_w_steps = delta_clip_w_steps;
	m_feeds[id]->delta_clip_h_steps = delta_clip_h_steps;
	return 0;
}

int CVirtualFeed::UpdateMove(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (m_feeds[id]->delta_x_steps) {
		m_feeds[id]->delta_x_steps--;
		m_feeds[id]->x += m_feeds[id]->delta_x;
	}
	if (m_feeds[id]->delta_y_steps) {
		m_feeds[id]->delta_y_steps--;
		m_feeds[id]->y += m_feeds[id]->delta_y;
	}
	if (m_feeds[id]->delta_clip_x_steps) {
		m_feeds[id]->delta_clip_x_steps--;
		int32_t x = m_feeds[id]->clip_x + m_feeds[id]->delta_clip_x;
		x >= 0 ? m_feeds[id]->clip_x = x : m_feeds[id]->clip_x = 0;
	}
	if (m_feeds[id]->delta_clip_y_steps) {
		m_feeds[id]->delta_clip_y_steps--;
		int32_t y = m_feeds[id]->clip_y + m_feeds[id]->delta_clip_y;
		y >= 0 ? m_feeds[id]->clip_y = y : m_feeds[id]->clip_y = 0;
	}
	if (m_feeds[id]->delta_clip_w_steps) {
		m_feeds[id]->delta_clip_w_steps--;
		int32_t w = m_feeds[id]->clip_w + m_feeds[id]->delta_clip_w;
		w >= 0 ? m_feeds[id]->clip_w = w : m_feeds[id]->clip_w = 0;
	}
	if (m_feeds[id]->delta_clip_h_steps) {
		m_feeds[id]->delta_clip_h_steps--;
		int32_t h = m_feeds[id]->clip_h + m_feeds[id]->delta_clip_h;
		h >= 0 ? m_feeds[id]->clip_h = h : m_feeds[id]->clip_h = 0;
	}
	if (m_feeds[id]->delta_scale_x_steps) {
		m_feeds[id]->delta_scale_x_steps--;
		double scale = m_feeds[id]->scale_x + m_feeds[id]->delta_scale_x;
		if (scale > 0.0) m_feeds[id]->scale_x = scale;
	}
	if (m_feeds[id]->delta_scale_y_steps) {
		m_feeds[id]->delta_scale_y_steps--;
		double scale = m_feeds[id]->scale_y + m_feeds[id]->delta_scale_y;
		if (scale > 0.0) m_feeds[id]->scale_y = scale;
	}
	if (m_feeds[id]->delta_rotation_steps) {
		m_feeds[id]->delta_rotation_steps--;
		m_feeds[id]->rotation += m_feeds[id]->delta_rotation;
		if (m_feeds[id]->rotation > 2*M_PI) m_feeds[id]->rotation -= 2*M_PI;
		if (m_feeds[id]->rotation < -2*M_PI) m_feeds[id]->rotation += 2*M_PI;
	}
	if (m_feeds[id]->delta_alpha_steps) {
		m_feeds[id]->delta_alpha_steps--;
		double alpha = m_feeds[id]->alpha + m_feeds[id]->delta_alpha;
		if (alpha < 0.0) m_feeds[id]->alpha = 0.0;
		else if (alpha > 1.0) m_feeds[id]->alpha = 1.0;
		else m_feeds[id]->alpha = alpha;
	}
    return 0;
}
