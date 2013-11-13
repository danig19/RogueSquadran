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
#include <math.h>
#include <string.h>
#include "cairo/cairo.h"

#include "video_mixer.h"
#include "video_text.h"
#include "video_image.h"
//#include "video_scaler.h"
//#include "add_by.h"
#include "cairo_graphic.h"
#include "video_shape.h"

CCairoGraphic::CCairoGraphic(u_int32_t width, u_int32_t height, u_int8_t* data) {
	m_width = width;
	m_height = height;
	m_pLayout = NULL;
	m_pDesc = NULL;
	//m_pTextItems = NULL;
	if (data) {
		m_pSurface = cairo_image_surface_create_for_data(data,
			CAIRO_FORMAT_ARGB32, m_width, m_height, m_width*4);
		m_pCr = cairo_create(m_pSurface);
	} else {
		m_pSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			m_width, m_height);
		m_pCr = cairo_create(m_pSurface);
		cairo_set_source_rgb(m_pCr, 1.0, 1.0, 1.0);
		cairo_paint(m_pCr);
	}
}

CCairoGraphic::~CCairoGraphic() {
	if (m_pDesc) pango_font_description_free(m_pDesc);
	if (m_pLayout) g_object_unref(m_pLayout);
	if (m_pCr) cairo_destroy(m_pCr);
	if (m_pSurface) cairo_surface_destroy(m_pSurface);
}

void CCairoGraphic::SetFont(char* font_str) {
	if (m_pDesc) pango_font_description_free(m_pDesc);
	m_pDesc = pango_font_description_from_string(font_str ? font_str : "Sans Bold 12");
	if (m_pDesc && m_pLayout) pango_layout_set_font_description(m_pLayout, m_pDesc);
	if (m_pDesc) pango_font_description_free(m_pDesc);
	m_pDesc = NULL;
}

void CCairoGraphic::SetTextExtent(u_int32_t x1, u_int32_t y1, char* str, char* font_str, double red, double green, double blue) {

	/* Variable declarations */
	//cairo_surface_t *surface = m_pSurface;
	cairo_t *cr = m_pCr;
	double x, y, px, ux=1, uy=1, dashlength;
	char* text = str;
	cairo_font_extents_t fe;
	cairo_text_extents_t te;

	/* Prepare drawing area */
	//surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 240, 240);
	//cr = cairo_create (surface);
	/* Example is in 26.0 x 1.0 coordinate space */
	cairo_scale (cr, 1, 1);
	cairo_set_font_size (cr, 14);

	/* Drawing code goes here */
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_select_font_face (cr, "Georgia",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_font_extents (cr, &fe);

	cairo_device_to_user_distance (cr, &ux, &uy);
	if (ux > uy)
		px = ux;
	else
		px = uy;
	cairo_font_extents (cr, &fe);
	cairo_text_extents (cr, text, &te);
	x = x1 + 0.5 - te.x_bearing - te.width / 2;
	y = y1 + 0.5 - fe.descent + fe.height / 2;

	/* baseline, descent, ascent, height */
	cairo_set_line_width (cr, 4*px);
	dashlength = 9*px;
	cairo_set_dash (cr, &dashlength, 1, 0);
	cairo_set_source_rgba (cr, 0, 0.6, 0, 0.5);
	cairo_move_to (cr, x + te.x_bearing, y);
	cairo_rel_line_to (cr, te.width, 0);
	cairo_move_to (cr, x + te.x_bearing, y + fe.descent);
	cairo_rel_line_to (cr, te.width, 0);
	cairo_move_to (cr, x + te.x_bearing, y - fe.ascent);
	cairo_rel_line_to (cr, te.width, 0);
	cairo_move_to (cr, x + te.x_bearing, y - fe.height);
	cairo_rel_line_to (cr, te.width, 0);
	cairo_stroke (cr);

	/* extents: width & height */
	cairo_set_source_rgba (cr, 0, 0, 0.75, 0.5);
	cairo_set_line_width (cr, px);
	dashlength = 3*px;
	cairo_set_dash (cr, &dashlength, 1, 0);
	cairo_rectangle (cr, x + te.x_bearing, y + te.y_bearing, te.width, te.height);
	cairo_stroke (cr);

	/* text */
	cairo_move_to (cr, x, y);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_show_text (cr, text);

	/* bearing */
	cairo_set_dash (cr, NULL, 0, 0);
	cairo_set_line_width (cr, 2 * px);
	cairo_set_source_rgba (cr, 0, 0, 0.75, 0.5);
	cairo_move_to (cr, x, y);
	cairo_rel_line_to (cr, te.x_bearing, te.y_bearing);
	cairo_stroke (cr);

	/* text's advance */
	cairo_set_source_rgba (cr, 0, 0, 0.75, 0.5);
	cairo_arc (cr, x + te.x_advance, y + te.y_advance, 5 * px, 0, 2 * M_PI);
	cairo_fill (cr);

	/* reference point */
	cairo_arc (cr, x, y, 5 * px, 0, 2 * M_PI);
	cairo_set_source_rgba (cr, 0.75, 0, 0, 0.5);
	cairo_fill (cr);
}

void CCairoGraphic::OverlayFrame(u_int8_t* src_buf,	// Pointer to src buffer (image to overlay)
	u_int32_t src_width, u_int32_t src_height,	// width and height of src
	int32_t x, int32_t y,				// x,y of clip
	u_int32_t src_w, u_int32_t src_h,		// width and height of clip
	u_int32_t src_x, u_int32_t src_y,		// start pixel of src to be shown
	double scale_x, double scale_y,			// scale of overlay image
	double rotate,					// Rotation
	double alpha,					// Blend factor
	transform_matrix_t* pMatrix)
{
	if (!src_buf || !src_width || !src_height || !src_w || !src_h || scale_x == 0.0 ||
		scale_y == 0.0 || alpha == 0.0) return;
	//u_int8_t* p = NULL;

	// Check to see if we should set alpha. If we want to do that, we need to allocate
	// a new frame and copy to it, as the shared mem is read only for input

	/*if (alpha < 1.0) {
		p = (u_int8_t*) malloc(src_width*src_height*4);
		u_int8_t* p2 = p + 3;
		u_int8_t alpha_int = (255*alpha + 0.5);
		memcpy(p, src_buf, src_width*src_height*4);
		for (unsigned int i=0 ; i < src_height; i++) {
			for (unsigned int n=0 ; n < src_width; n++) {
				*p2 = alpha_int;
				p2 += 4;
			}
		}
	} // else we assume alpha is set to 1.0 (255) in the image
*/
	// Create a surface from image data
	//cairo_surface_t* pSurface = cairo_image_surface_create_for_data(p ? p : src_buf,
	cairo_surface_t* pSurface = cairo_image_surface_create_for_data(src_buf,
		CAIRO_FORMAT_ARGB32, src_width, src_height, src_width*4);
	if (!pSurface) {
		// We failed to get a surface. We will return silently
		//if (p) free(p);
		return;
	}
	// Create the context
	cairo_t* pCr = cairo_create(pSurface);
	if (pCr) {
		cairo_save (m_pCr);
		if (pMatrix) {
			cairo_matrix_t matrix;
			cairo_matrix_init(&matrix,
				pMatrix->matrix_xx, pMatrix->matrix_xy,
				pMatrix->matrix_yx, pMatrix->matrix_yy,
				pMatrix->matrix_x0, pMatrix->matrix_y0);
			cairo_transform (m_pCr, &matrix);
		}

		// src_width, src_height,		// width and height of src
		// x, y,				// x,y of clip
		// src_w, src_h,			// width and height of clip
		// src_x, src_y,			// start pixel of src to be shown

		int clip_x = x; int clip_y = y;
		int clip_w = src_x + src_w/scale_x > src_width ? src_width - src_x : src_w;
		int clip_h = src_y + src_h/scale_y > src_height ? src_height - src_y : src_h;
		int ms_x = src_x-x; int ms_y = src_y-y;

		// Move coordinate system to x/y
		int trans_x = x ; int trans_y = y;
		trans_x = x + (clip_w>>1); trans_y = y + (clip_h>>1);

		cairo_translate(m_pCr, trans_x, trans_y);
		  if (rotate != 0.0) cairo_rotate(m_pCr,rotate);
		    if (scale_x != 1.0 || scale_y != 1.0) cairo_scale(m_pCr, scale_x, scale_y);

		      cairo_rectangle(m_pCr, clip_x/scale_x - trans_x/scale_x,
						clip_y/scale_y - trans_y/scale_y,
						clip_w/scale_x, clip_h/scale_y);
		//cairo_arc (m_pCr, -100.0, 0.0, 75, 0.125*M_PI, (2-0.125)*M_PI);
		//cairo_arc (m_pCr, 100.0, 0.0, 75, 1.125*M_PI, (2+0.875)*M_PI);
		//cairo_line_to (m_pCr, -100.0, 0.0);
		      cairo_clip (m_pCr);
		      cairo_new_path (m_pCr); /* path not consumed by clip()*/


		      //cairo_set_operator(pCr, CAIRO_OPERATOR_ADD);
			// This command place the clipped surface at x, y. It does not move the clip
		      cairo_set_source_surface(m_pCr, pSurface, -ms_x/scale_x -trans_x/scale_x, -ms_y/scale_y -trans_y/scale_y);
  		      if (alpha < 1.0) cairo_paint_with_alpha(m_pCr, alpha); else cairo_paint(m_pCr);
		    if (scale_x != 1.0 || scale_y != 1.0) cairo_scale(m_pCr, 1/scale_x, 1/scale_y);
		  if (rotate != 0.0) cairo_rotate(m_pCr,-rotate);
		cairo_translate(m_pCr, -trans_x, -trans_y);

		//cairo_surface_flush(m_pSurface);
		cairo_reset_clip (m_pCr);
		cairo_restore (m_pCr);
	}  // else we failed, but will return silently
	//if (p) free(p);
	if (pCr) cairo_destroy(pCr);
	if (pSurface) cairo_surface_destroy(pSurface);
}

void CCairoGraphic::OverlayText(CVideoMixer* pVideoMixer, u_int32_t place_id)
{

	// First we need to do a lot of checking
	if (!pVideoMixer || !pVideoMixer->m_pTextItems) return;
	text_item_t* pT = pVideoMixer->m_pTextItems->GetTextPlace(place_id);
	if (!pVideoMixer || !pT) return;

	char* newstr = NULL;
	char* s = NULL;
	char* str = pVideoMixer->m_pTextItems->GetString(pT->text_id);
	char* font = pVideoMixer->m_pTextItems->GetFont(pT->font_id);
	if (!str || !font) return;

	// Special case for handling [#DATETIME]
	if ((s=strstr(str,"[#DATETIME]"))) {
		char date[26];
		newstr = (char*) malloc(strlen(str)+26-11);
		if (!newstr) return;
		*newstr = '\0';
		int n = (int)(s - str);
		strncpy(newstr,str,n);
		
		struct timeval now;
		gettimeofday(&now,NULL);

		sprintf(newstr+n, "%s", ctime_r(&(now.tv_sec),&date[0]));
		char* s2 = newstr+n;
		while (*s2 != '\n') s2++;
		strcpy(s2, s+11);
//fprintf(stderr, "Time %d %d\n", now.tv_sec, m_start.tv_sec);
		str = newstr;
	} 
	// Special case for handling [#RUNTIME] and [#RUNTIME,n]
	else if ((s=strstr(str,"[#RUNTIME"))) {
		struct timeval*	base_time = NULL;
		struct timeval	now;
		u_int32_t	feedno = 0;
		int		i = 0;

		newstr = (char*) malloc(strlen(str)+4);
		if (!newstr) return;
		*newstr = '\0';

		// Copy anything before [#RUNTIME to newstr
		int n = (int)(s - str);
		strncpy(newstr,str,n);

		// Now advance to first position after [#RUNTIME
		s += 9;

		// We use system time as base, if we encountered [#RUNTIME]
		if (*s == ']') base_time = &(pVideoMixer->m_start);
		else if ((i = sscanf(s, ",%u]", &feedno)) == 1) {

			// We found [#RUNTIME,n] where n is feed no.
			if (!pVideoMixer->m_pVideoFeed) {
				fprintf(stderr, "pVideoFeed was NULL in OverlayText\n");
				return;
			}
			feed_type* pFeed = pVideoMixer->m_pVideoFeed->FindFeed(feedno);
			if (pFeed) base_time = &(pFeed->start_time);
		}

		// Advance to after '['
		while (*s && *s != '[') s++;
		if (base_time) {

			// hhhh:mm:ss.fff
			gettimeofday(&now,NULL);
			int secs = now.tv_sec - base_time->tv_sec;
			int hours = secs/3600; secs -= 3600*hours;
			int minutes = secs/60; secs -= 60*minutes;
			int millisecs = (now.tv_usec - base_time->tv_usec)/1000;
			if (millisecs < 0) {
				millisecs += 1000;
				secs -= 1;
				if (secs < 0) {
					secs += 60;
					minutes -= 1;
					if (minutes < 0) {
						minutes += 60;
						hours -=1;
					}
				}
			}
			if (base_time->tv_sec == 0 && base_time->tv_usec == 0) {
				hours = minutes = secs = millisecs = 0;
			}
			sprintf(newstr+n, "%d:%02d:%02d.%03d", hours, minutes, secs, millisecs);
			char* s2 = newstr+n;
			while (*s2) s2++;
			strcpy(s2, s);
			str = newstr;
		}
	}
	if ((s=strstr(str,"[#FEEDSTATE,"))) {	// Looking for "[#FEEDSTATE,2,DISCONNECTED]"
		s+=12;
		int n, feedno;
		n=sscanf(s,"%u",&feedno);
		if (n==1) {
			while (*s && isdigit(*s)) s++;
			if (*s == ',') {
				s++;
#define	check_state(text, statetype)	if ((strncasecmp(s,text, strlen(text))) == 0) { \
					s += strlen(text) +1; \
					feed_type* pFeed = pVideoMixer->m_pVideoFeed->FindFeed(feedno); \
					if (!pFeed || pFeed->state != statetype) return; \
					str = s; \
				}
				check_state("STALLED", STALLED)
				else check_state("DISCONNECTED", DISCONNECTED)
				else check_state("PENDING", PENDING)
				else check_state("RUNNING", RUNNING)
				else check_state("SETUP", SETUP);
			}
		}
	}

	const char* growtext_original = NULL;
	if (pT->text_grow > -1) {
		char* fromtext = newstr ? newstr : str;
		int len = strlen(fromtext);
		if (pT->text_grow_bg_fixed) growtext_original = fromtext;
		int delta = pT->text_grow_delta ? pT->text_grow_delta :
			(len >>4) | 0x01;
		if (pT->text_grow < len) {

			char* totext = (char*) calloc(1,pT->text_grow + 1);
			if (totext) {
				strncpy(totext, fromtext, pT->text_grow);
				pT->text_grow += delta;
				if (newstr) free(newstr);
				str = newstr = totext;
			
			}
		} else pT->text_grow = -1;
	}


	// We now want ot define x,y as the position where the text
	// is intended to be placed.
	// repeat_move_x and y is the position the text is currently
	// placed at while pT->x and y is the position the text
	// started and and will return to. Int should be ok, but
	// for now we use double to test if text scale animation can improve
	double x = pT->repeat_move_x + pT->anchor_x;
	double y = pT->repeat_move_y + pT->anchor_y;
	//u_int32_t x = pT->repeat_move_x + pT->anchor_x;
	//u_int32_t y = pT->repeat_move_y + pT->anchor_y;


	// Without a cairo context we can't do much
	if (m_pCr) {

		// Save the Cairo context so we can restore it when finished with the text item
		cairo_save (m_pCr);
		// Check to see if we should clip absolute
		if (pT->clip_abs_x != 0 || pT->clip_abs_y != 0 ||
			pT->clip_abs_width != 0 || pT->clip_abs_height != 0) {
			cairo_rectangle(m_pCr, pT->clip_abs_x, pT->clip_abs_y,
				pT->clip_abs_width, pT->clip_abs_height);
			cairo_clip(m_pCr);
			cairo_new_path(m_pCr);
		}
		// Scale if we have to
		if (pT->scale_x != 1.0 || pT->scale_y != 1.0)
			cairo_scale(m_pCr, pT->scale_x, pT->scale_y);

		// We want to create a PangoLayout to hold the text string
		PangoLayout* pLayout = pango_cairo_create_layout(m_pCr);
		if (pLayout) {

			// Then we need to set the font
			PangoFontDescription* pDesc = pango_font_description_from_string(font ?
				font : "Sans Bold 12");

			// We need the font description to ba able to do anything
			// Without it we can do much
			if (pDesc) {

				// Tell Pango to use the font description
				pango_layout_set_font_description(pLayout, pDesc);

				// We can now dispose the font description
				pango_font_description_free(pDesc);

				// We now want to estimate the extent of the text
				// One could argue whether to use double instead of int.
				int w, h;
				pango_layout_set_text(pLayout, growtext_original ? growtext_original : str, -1);
				pango_layout_get_size(pLayout, &w, &h);
				// The wxh is returned in PANGO_SCALE units 
				w = w / PANGO_SCALE;
				h = h / PANGO_SCALE;

				// Now we have the w and h of the text if placed.
				// We save this info - maybe for later query - but
				// we have to do this at every frame as the text or font
				// can change between frames.
				pT->w = w;
				pT->h = h;


				// delta_x and y is the offset we need to make when placing the
				// text in case we have an alignment different from left,top
				int32_t delta_x = 0;
				int32_t delta_y = 0;

				// See if align is different from align left and top
				if (pT->align &(~(TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP))) {
					if (pT->align & TEXT_ALIGN_CENTER)
						delta_x =-w/2;
					else if (pT->align & TEXT_ALIGN_RIGHT)
						delta_x =-w;
					if (pT->align & TEXT_ALIGN_MIDDLE)
						delta_y =-h/2;
					else if (pT->align & TEXT_ALIGN_BOTTOM)
						delta_y =-h;
				}

#define x_0 0
#define y_0 0
				// We now translate 0,0 to x,y so we can place text and rectangles
				// 0,0 plus delta
				cairo_translate(m_pCr,
					x/pT->scale_x, y/pT->scale_y);

				// Then we need to check for rotation
				if (pT->rotate != 0.0) {
					//cairo_move_to(m_pCr, 0, 0);
					cairo_rotate(m_pCr, pT->rotate);
				}

				cairo_pattern_t* linpat_h = NULL;
				cairo_pattern_t* linpat_v = NULL;
				// Check for alpha of background. If it is non zero,
				// we can set a rect
				if (pT->alpha_bg != 0.0 || pT->linpat_h
					|| pT->linpat_v) {

					linpat_t* p = NULL;
					if ((p = pT->linpat_h)) {
						linpat_h = cairo_pattern_create_linear(
							delta_x+x_0-pT->pad_left,
							delta_y + y_0,
							//delta_y + y_0 + (h/2),
							delta_x + x_0 + w + pT->pad_right,
							delta_y + y_0 + (h/1));
							//delta_y + y_0 + (h/2));
						while (p) {
							double alpha = pT->alpha_bg*
								p->alpha;
							if (alpha < 0.0) alpha = 0.0;
							else if (alpha > 1.0) alpha = 1.0;
							cairo_pattern_add_color_stop_rgba (
								linpat_h, p->fraction, p->red,
								p->green, p->blue, alpha);
							p = p->next;
						}
					}
					if ((p = pT->linpat_v)) {
						linpat_v = cairo_pattern_create_linear(
							delta_x + x_0 + (w/2),
							delta_y + y_0 - pT->pad_top,
							delta_x + x_0 + (w/2),
							delta_y + y_0 + pT->pad_bottom + h);
						while (p) {
							double alpha = pT->alpha_bg*
								p->alpha;
							if (alpha < 0.0) alpha = 0.0;
							else if (alpha > 1.0) alpha = 1.0;
							cairo_pattern_add_color_stop_rgba (
								linpat_v, p->fraction, p->red,
								p->green, p->blue, alpha);
							p = p->next;
						}
					}
					if (pT->clip_bg_left > 0.0 || pT->clip_bg_right < 1.0 ||
						pT->clip_bg_top > 0.0 || pT->clip_bg_bottom < 1.0) {
						int bg_w = pT->pad_left + pT->pad_right + w;
						int bg_h = pT->pad_top + pT->pad_bottom + h;
						int clip_x = x_0+delta_x - pT->pad_left +
							bg_w*pT->clip_bg_left;
						int clip_y = y_0+delta_y - pT->pad_top +
							bg_h*pT->clip_bg_top;
						int clip_w  = bg_w*(pT->clip_bg_right -
							pT->clip_bg_left);
						int clip_h  = bg_h*(pT->clip_bg_bottom -
							pT->clip_bg_top);
						cairo_rectangle(m_pCr, clip_x, clip_y,
							clip_w, clip_h);
						cairo_clip(m_pCr);
						cairo_new_path (m_pCr);
					}
					if (1 || pT->round_left_top || pT->round_right_top ||
					    pT->round_left_bottom || pT->round_right_bottom) {
						double x = x_0+delta_x-pT->pad_left;
						double y = y_0+delta_y-pT->pad_top;
						double width = pT->pad_left+pT->pad_right+w;
						double height = pT->pad_top+pT->pad_bottom+h;
						cairo_new_path (m_pCr);
						cairo_move_to(m_pCr, x+pT->round_left_top, y+pT->round_left_top);
						if (pT->round_left_top) {
							cairo_arc(m_pCr, x + pT->round_left_top, y + pT->round_left_top,
								(double)pT->round_left_top, -M_PI, -M_PI/2);
						}
						cairo_line_to(m_pCr, x + width - pT->round_right_top, y);
						if (pT->round_right_top) {
							cairo_arc(m_pCr, x + width-pT->round_right_top,
								y + pT->round_right_top,
								(double)pT->round_right_top, -M_PI/2, 0.0);
						}
						cairo_line_to(m_pCr, x + width, y + height - pT->round_right_bottom);
						if (pT->round_right_bottom) {
							cairo_arc(m_pCr,x+width-pT->round_right_bottom,
								y + height - pT->round_right_bottom,
								(double)pT->round_right_bottom, 0.0, M_PI/2);
						}
						cairo_line_to(m_pCr, x + pT->round_left_bottom, y + height);
						if (pT->round_left_bottom) {
							cairo_arc(m_pCr, x + pT->round_left_bottom, y + height - pT->round_left_bottom,
								(double)pT->round_left_bottom, M_PI/2, M_PI);
						}
						cairo_line_to(m_pCr, x, y + pT->round_left_top);
					} else {
						cairo_rectangle(m_pCr,
							x_0+delta_x-pT->pad_left,
							y_0+delta_y-pT->pad_top,
							pT->pad_left+pT->pad_right+w,
							pT->pad_top+pT->pad_bottom+h);
					}

					if (pT->linpat_h || pT->linpat_v ) {
						if (pT->linpat_h)
							cairo_set_source(m_pCr, linpat_h);
						if (pT->linpat_v)
							cairo_set_source(m_pCr, linpat_v);
					} else cairo_set_source_rgba(m_pCr, pT->red_bg,
						pT->green_bg, pT->blue_bg,
						pT->alpha_bg);
					cairo_fill (m_pCr);
				}

				if (pT->clip_left > 0.0 || pT->clip_right < 1.0 ||
					pT->clip_top > 0.0 || pT->clip_bottom < 1.0) {
					int clip_x = x_0+delta_x + w*pT->clip_left;
					int clip_y = y_0+delta_y + h*pT->clip_top;
					int clip_w  = w*(pT->clip_right - pT->clip_left);
					int clip_h  = h*(pT->clip_bottom - pT->clip_top);
					cairo_rectangle(m_pCr, clip_x, clip_y, clip_w, clip_h);
					cairo_clip(m_pCr);
					cairo_new_path (m_pCr);
				}
				// Check if we need to paint with alpha
				if (pT->alpha >= 1.0) cairo_set_source_rgb(m_pCr,
					pT->red, pT->green, pT->blue);
				else cairo_set_source_rgba(m_pCr, pT->red,
					pT->green, pT->blue, pT->alpha);

				//if (pT->scale_x != 1.0 || pT->scale_y != 1.0)
				//	cairo_move_to(m_pCr, (x_0+delta_x)/pT->scale_x,
				//		(y_0 + delta_y)/pT->scale_y);
				cairo_move_to(m_pCr, x_0+delta_x, y_0 + delta_y);

				pango_layout_set_text(pLayout, str, -1);
				pango_cairo_update_layout(m_pCr, pLayout);
				pango_cairo_show_layout(m_pCr, pLayout);

				if (linpat_h) cairo_pattern_destroy(linpat_h);
				if (linpat_v) cairo_pattern_destroy(linpat_v);

				// Check to see if we need to rotate back
				if (pT->rotate != 0.0)
					cairo_rotate(m_pCr,-pT->rotate);

				// and we move offset from x,y back to 0,0
				//cairo_translate(m_pCr, -x, -y);
			}
			g_object_unref(pLayout);
		}
// Scale back if we have to
if (pT->scale_x != 1.0 && pT->scale_y != 1.0)
	cairo_scale(m_pCr, 1.0/pT->scale_x, 1.0/pT->scale_y);

		if (pT->update) pVideoMixer->m_pTextItems->UpdateMove(place_id);

cairo_restore (m_pCr);
	}

	if (newstr) free(newstr);
}

void CCairoGraphic::OverlayShape(CVideoMixer* pVideoMixer, u_int32_t shape_id, double alpha, double scale_x, double scale_y, double* rotation, u_int32_t* save, u_int32_t level)
{
	//u_int32_t width, height;
	image_item_t* pImage;
	cairo_surface_t* pSurface	= NULL;
	pattern_t *pattern		= NULL;
	cairo_pattern_t *cairo_pattern	= NULL;
	struct feed_type* pFeed		= NULL;
	u_int8_t* src_buf		= NULL;

	if (!pVideoMixer || !pVideoMixer->m_pVideoShape ||
		!level) return;
	shape_t* pShape = pVideoMixer->m_pVideoShape->GetShape(shape_id);
	if (!pShape || !pShape->pDraw) return;

	double scale = scale_x > scale_y ? scale_x : scale_y;
	draw_operation_t* pDraw = pShape->pDraw;
	while (pDraw) {
		switch (pDraw->type) {
			case SHAPE:
				if (level) OverlayShape(pVideoMixer, pDraw->parameter,
					alpha, scale_x, scale_y, rotation,
					save, level-1);
				break;
			case CLIP:
				cairo_clip(m_pCr);
				break;
			case NEWPATH:
				cairo_new_path(m_pCr);
				break;
			case MOVETO:
				cairo_move_to(m_pCr, pDraw->x, pDraw->y);
				break;
			case MOVEREL:
				cairo_rel_move_to(m_pCr, pDraw->x, pDraw->y);
				break;
			case LINETO:
				cairo_line_to(m_pCr, pDraw->x, pDraw->y);
				break;
			case LINEREL:
				cairo_rel_line_to(m_pCr, pDraw->x, pDraw->y);
				break;
			case SCALE:
				if (pDraw->x == 0 || pDraw->y == 0) break;
				if (pDraw->x > 0 && pDraw->y > 0) {
					cairo_scale(m_pCr, pDraw->x, pDraw->y);
					scale_x *= pDraw->x;
					scale_y *= pDraw->y;
				}
				else if (pDraw->x < 0 && pDraw->y < 0) {
					cairo_scale(m_pCr, -pDraw->x/scale_x,
						-pDraw->y/scale_y);
					scale_x = -pDraw->x;
					scale_y = -pDraw->y;
				}
				scale = scale_x > scale_y ? scale_x : scale_y;
				break;
			case TRANSLATE:
				cairo_translate(m_pCr, pDraw->x, pDraw->y);
				break;
			case CURVETO:
				cairo_curve_to(m_pCr, pDraw->x, pDraw->y,
					pDraw->radius, pDraw->width,		// x2,y2
					pDraw->angle_from, pDraw->angle_to);	// x3,y3
				break;
			case CURVEREL:
				cairo_rel_curve_to(m_pCr, pDraw->x, pDraw->y,
					pDraw->radius, pDraw->width,		// x2,y2
					pDraw->angle_from, pDraw->angle_to);	// x3,y3
				break;
			case RECTANGLE:
				// angle_from and angle_to are width and height of rect
				cairo_rectangle(m_pCr, pDraw->x, pDraw->y,
					pDraw->angle_from, pDraw->angle_to);
				break;
			case ARC_CW:
				cairo_arc(m_pCr, pDraw->x, pDraw->y, pDraw->radius,
					pDraw->angle_from, pDraw->angle_to);
				break;
			case ARC_CCW:
				cairo_arc_negative(m_pCr, pDraw->x, pDraw->y, pDraw->radius,
					pDraw->angle_from, pDraw->angle_to);
				break;
			case ARCREL_CW:
			case ARCREL_CCW: {
					double x, y;
					cairo_get_current_point(m_pCr, &x, &y);
					x += pDraw->x;
					y += pDraw->y;
					if (pDraw->type == ARCREL_CW)
						cairo_arc(m_pCr, x, y, pDraw->radius,
							pDraw->angle_from, pDraw->angle_to);
					else cairo_arc_negative(m_pCr, x, y, pDraw->radius,
							pDraw->angle_from, pDraw->angle_to);
				}
				break;
			case CLOSEPATH:
				cairo_close_path(m_pCr);
				break;
			case MASK_PATTERN:
			case SOURCE_PATTERN:
				// If alpha is less than 1.0, then we need to copy the pattern
				// and recalculate all alpha values for rgba stops. 
				if (alpha < 1.0) {
					cairo_pattern = NULL;
					// First we get the pattern_t to get coordinates
					if (!(pattern = pVideoMixer->m_pVideoShape->GetPattern(
						pDraw->parameter))) break;

					// Then we get the actual cairo_pattern_t
					if (!pVideoMixer || !pVideoMixer->m_pVideoShape) break;
					cairo_pattern_t* cairo_pattern_source =
						pVideoMixer->m_pVideoShape->GetCairoPattern(
						pDraw->parameter);
					if (!cairo_pattern_source) break;
					int stops = 0;

					// Then we get the number of rgba stops
					if (cairo_pattern_get_color_stop_count(
						cairo_pattern_source, &stops) !=
						CAIRO_STATUS_SUCCESS || !stops) break;

					// Then we deal with the RADIAL_PATTERN type
					if (pattern->type == RADIAL_PATTERN) {
	
						cairo_pattern = cairo_pattern_create_radial(
							pattern->x1,
							pattern->y1,
							pattern->radius0,
							pattern->x2,
							pattern->y2,
							pattern->radius1);
					} else if (pattern->type == LINEAR_PATTERN) {
						cairo_pattern = cairo_pattern_create_linear(
							pattern->x1,
							pattern->y1,
							pattern->x2,
							pattern->y2);
					}
					if (!cairo_pattern) break;

					// Now fill the new copied pattern with rgba stops
					double offset, red, green, blue, alpha_stop;
					while (stops--) {
						if (cairo_pattern_get_color_stop_rgba(
							cairo_pattern_source, stops, &offset,
							&red, &green, &blue, &alpha_stop) !=
							CAIRO_STATUS_SUCCESS) {
								break;
fprintf(stderr, "Failed to get color stop for pattern for mask - %d\n", stops);
							}
						cairo_pattern_add_color_stop_rgba(cairo_pattern,
							offset, red, green, blue, alpha_stop*alpha);
					}
					if (pDraw->type == MASK_PATTERN)
						cairo_mask(m_pCr, cairo_pattern);
					else if (pDraw->type == SOURCE_PATTERN)
						cairo_set_source(m_pCr, cairo_pattern);
					cairo_pattern_destroy(cairo_pattern);
					cairo_pattern = NULL;
				} else if (alpha > 0.0) {
					cairo_pattern = pVideoMixer->m_pVideoShape->GetCairoPattern(
						pDraw->parameter);
					if (cairo_pattern) {
						if (pDraw->type == MASK_PATTERN)
							cairo_mask(m_pCr, cairo_pattern);
						else if (pDraw->type == SOURCE_PATTERN)
							cairo_set_source(m_pCr, cairo_pattern);
					}
				}
				break;
/*
			case SOURCE_PATTERN:
				cairo_pattern = pVideoMixer->m_pVideoShape->GetCairoPattern(pDraw->parameter);
				if (cairo_pattern) cairo_set_source(m_pCr, cairo_pattern);
				break;
*/
			case ROTATION:
				// Check to see if we should rotate absolute
				if (pDraw->parameter) {
					cairo_rotate(m_pCr, pDraw->angle_from +
						pDraw->angle_to - (*rotation));
					*rotation = pDraw->angle_from + pDraw->angle_to;
				} else {
					if (pDraw->angle_from) {
						cairo_rotate(m_pCr, pDraw->angle_from);
						*rotation += pDraw->angle_from;
					}
				}
				break;
			case SOURCE_RGB:
				if (alpha < 0.0) cairo_set_source_rgb(m_pCr,
					pDraw->red, pDraw->green, pDraw->blue);
				else cairo_set_source_rgba(m_pCr, pDraw->red,
					pDraw->green, pDraw->blue, alpha);
				break;
			case ALPHAADD:
			case ALPHAMUL:
				if (pDraw->type == ALPHAADD) alpha += pDraw->alpha;
				else alpha *= pDraw->alpha;
				if (alpha < 0.0) alpha = 0.0;
				else if (alpha > 1.0) alpha = 1.0;
				break;
			case SOURCE_RGBA:
				if (alpha < 0.0) cairo_set_source_rgba(m_pCr, pDraw->red,
					pDraw->green, pDraw->blue, pDraw->alpha);
				else cairo_set_source_rgba(m_pCr, pDraw->red,
					pDraw->green, pDraw->blue, alpha*pDraw->alpha);
				break;
			case LINEWIDTH:
				if (pDraw->width < 0.0)
					cairo_set_line_width(m_pCr, -pDraw->width/scale);
				else
					cairo_set_line_width(m_pCr, pDraw->width);
				break;
			case LINEJOIN:
				cairo_set_line_join(m_pCr,
					pDraw->x < 0.0 ? CAIRO_LINE_JOIN_MITER :
					  pDraw->x == 0.0 ? CAIRO_LINE_JOIN_ROUND :
					    CAIRO_LINE_JOIN_BEVEL);
				if (pDraw->x < 0.0) cairo_set_miter_limit(m_pCr, pDraw->y);
				break;
			case LINECAP:
				cairo_set_line_cap(m_pCr,
					pDraw->width < 0.0 ? CAIRO_LINE_CAP_BUTT :
					  pDraw->width == 0.0 ? CAIRO_LINE_CAP_ROUND :
					    CAIRO_LINE_CAP_SQUARE);
			case STROKE:
				cairo_stroke(m_pCr);
				break;
			case STROKEPRESERVE:
				cairo_stroke_preserve(m_pCr);
				break;
			case FILL:
				cairo_fill(m_pCr);
				break;
			case FILLPRESERVE:
				cairo_fill_preserve(m_pCr);
				break;
			case PAINT:
				if (alpha < 0.0 || pDraw->alpha < 0.0)
					cairo_paint(m_pCr);
				else cairo_paint_with_alpha(m_pCr, alpha*pDraw->alpha);
				break;
			case OPERATOR:
				cairo_set_operator(m_pCr, (cairo_operator_t) pDraw->parameter);
				break;
			case TRANSFORM:
				cairo_transform(m_pCr, (cairo_matrix_t*)(&pDraw->x));
				break;
			case SAVE:
				cairo_save(m_pCr);
				(*save)++;
				break;
			case RESTORE:
				if (*save) {
					cairo_matrix_t matrix;
					cairo_restore(m_pCr);
					cairo_get_matrix(m_pCr,&matrix);
					scale_x = matrix.xx;
					scale_y = matrix.yy;
					(*save)--;
				}
				break;
			case SCALETELL: {
					cairo_matrix_t matrix;
					cairo_get_matrix(m_pCr,&matrix);
					fprintf(stderr, "SCALETELL %.5lf %.5lf\n",matrix.xx, matrix.yy);
				}
				break;
			case RECURSION:
				level = pDraw->parameter;
				break;
			case IMAGE:
				// scale_x < 0.0 : relative scale - scale from whatever shape is scaled to.
				// scale_x = 0.0 : no scale - follow whatever the placed shape is scale to
				// scale_x > 0.0 : absolute scale - scale absolute taking into account scale of shape
				if (!pVideoMixer->m_pVideoImage) break;
				pImage = pVideoMixer->m_pVideoImage->GetImageItem(pDraw->parameter);
				if (!pImage) break;
				// Use width and radius for scaling.
				if (pDraw->width < 0.0 && pDraw->radius < 0.0)
					cairo_scale(m_pCr, -pDraw->width, -pDraw->radius);
				else if (pDraw->width > 0.0 && pDraw->radius > 0.0)
					cairo_scale(m_pCr, pDraw->width/scale_x, pDraw->radius/scale_y);
				//else fprintf(stderr, "Image using scale %.4lf,%.4lf\n", scale_x, scale_y);
				//fprintf(stderr, "Image scale %.4lf,%.4lf\n", pDraw->width, pDraw->radius);
				// Create a surface from image data
				if (pSurface) cairo_surface_destroy(pSurface);
				pSurface = cairo_image_surface_create_for_data(
					pImage->pImageData, CAIRO_FORMAT_ARGB32,
					pImage->width, pImage->height, pImage->width*4);
				if (!pSurface) break;
				cairo_set_source_surface(m_pCr, pSurface, pDraw->x, pDraw->y);
				//if (alpha < 0.0) cairo_paint(m_pCr);
				//else cairo_paint_with_alpha(m_pCr, alpha*pDraw->alpha);

				// Set scaling back to what it was.
				if (pDraw->width < 0.0 && pDraw->radius < 0.0)
					cairo_scale(m_pCr, -1.0/pDraw->width, -1.0/pDraw->radius);
				else if (pDraw->width > 0.0 && pDraw->radius > 0.0)
					cairo_scale(m_pCr, scale_x/pDraw->width, scale_y/pDraw->radius);

				break;
			case FEED:
				// scale_x < 0.0 : relative scale - scale from whatever shape is scaled to.
				// scale_x = 0.0 : no scale - follow whatever the placed shape is scale to
				// scale_x > 0.0 : absolute scale - scale absolute taking into account scale of shape
				if (!pVideoMixer->m_pVideoFeed) break;
				if (!(pFeed = pVideoMixer->m_pVideoFeed->FindFeed(pDraw->parameter))) break;

				// Use width and radius for scaling.
				if (pDraw->width < 0.0 && pDraw->radius < 0.0)
					cairo_scale(m_pCr, -pDraw->width, -pDraw->radius);
				else if (pDraw->width > 0.0 && pDraw->radius > 0.0)
					cairo_scale(m_pCr, pDraw->width/scale_x, pDraw->radius/scale_y);
//fprintf(stderr, " - shape Feed scale %.4lf,%.4lf\n", pDraw->width/scale_x, pDraw->radius/scale_y);

				// Check that we have an image in the feed buffer
				if (pFeed->fifo_depth <= 0) {
					/* Nothing in the fifo. Show the idle image. */
					src_buf = pFeed->dead_img_buffer;
				} else {
					/* Pick the first one from the fifo. */
					src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
						pFeed->frame_fifo [0].buffer->offset;
				}

				// Create a surface from image data
				if (pSurface) cairo_surface_destroy(pSurface);
				pSurface = cairo_image_surface_create_for_data(src_buf,
					CAIRO_FORMAT_ARGB32, pFeed->width, pFeed->height,
					pFeed->width*4);
				if (!pSurface) break;
				cairo_set_source_surface(m_pCr, pSurface, pDraw->x, pDraw->y);
				//if (alpha < 0.0) cairo_paint(m_pCr);
				//else cairo_paint_with_alpha(m_pCr, alpha*pDraw->alpha);

				// Set scaling back to what it was.
				if (pDraw->width < 0.0 && pDraw->radius < 0.0)
					cairo_scale(m_pCr, -1.0/pDraw->width, -1.0/pDraw->radius);
				else if (pDraw->width > 0.0 && pDraw->radius > 0.0)
					cairo_scale(m_pCr, scale_x/pDraw->width, scale_y/pDraw->radius);

				break;
			default:
fprintf(stderr, "Unknown command %d\n", (int)pDraw->type);
				break;

		}
		pDraw = pDraw->next;
	}
	if (pSurface) cairo_surface_destroy(pSurface);
}

void CCairoGraphic::OverlayPlacedShape(CVideoMixer* pVideoMixer, u_int32_t place_id)
{
	if (!pVideoMixer || !pVideoMixer->m_pVideoShape) return;
	placed_shape_t* pPlacedShape = pVideoMixer->m_pVideoShape->GetPlacedShape(place_id);
	if (!pPlacedShape || pPlacedShape->alpha == 0.0) return;

	u_int32_t save = 0;		// count context savings for shape
	double rotation = pPlacedShape->rotation;

	// Save the context for later restore.
	cairo_save (m_pCr);

		//cairo_new_path(m_pCr);
		// Set source colour and alpha
		cairo_set_source_rgba(m_pCr, pPlacedShape->red, pPlacedShape->green,
			pPlacedShape->blue, pPlacedShape->alpha >= 1.0 ? 1.0 :
			  pPlacedShape->alpha);

		// move to x,y
		cairo_translate(m_pCr, pPlacedShape->x + pPlacedShape->anchor_x,
			pPlacedShape->y + pPlacedShape->anchor_y);
//fprintf(stderr, "Place shape %.1lf,%.1lf\n",pPlacedShape->x, pPlacedShape->y);

		// Rotate if we have to
		if (pPlacedShape->rotation != 0.0)
			cairo_rotate(m_pCr, pPlacedShape->rotation);

		cairo_translate(m_pCr, pPlacedShape->offset_x, pPlacedShape->offset_y);
	
		// Scale if we have to
		if (pPlacedShape->scale_x != 1.0 || pPlacedShape->scale_y != 1.0)
			cairo_scale(m_pCr, pPlacedShape->scale_x, pPlacedShape->scale_y);

		// Set a new path
		cairo_new_path(m_pCr);

		OverlayShape(pVideoMixer, pPlacedShape->shape_id, pPlacedShape->alpha,
			pPlacedShape->scale_x, pPlacedShape->scale_y, &rotation, &save,
			OVERLAY_SHAPE_LEVELS);
		while (save) {
			cairo_restore (m_pCr);
			save--;
		}
			

	//	if (pPlacedShape->alpha < 0.0) cairo_paint(m_pCr);
	//	else cairo_paint_with_alpha(m_pCr, pPlacedShape->alpha);
		

	// Restore the context again
	cairo_restore (m_pCr);
}

void CCairoGraphic::OverlayImage(CVideoMixer* pVideoMixer, u_int32_t place_id)
{
	// First we need to check we can reach CVideoImage via CVideoMixer
	if (!pVideoMixer || !pVideoMixer->m_pVideoImage) return;

	// The we get the placed Image
	struct image_place_t* pImagePlaced = pVideoMixer->m_pVideoImage->GetImagePlaced(place_id);
	if (!pImagePlaced || pImagePlaced->alpha == 0.0) {
		pVideoMixer->m_pVideoImage->UpdateMove(place_id);
		return;
	}

	// and then we get the associated Image itself
	image_item_t* pImageItem = pVideoMixer->m_pVideoImage->GetImageItem(pImagePlaced->image_id);
	if (!pImageItem || !pImageItem->pImageData) return;

	u_int32_t width = pImageItem->width;
	u_int32_t height = pImageItem->height;
	int32_t delta_x = 0;
	int32_t delta_y = 0;

        // See if align is different from align left and top
        if (pImagePlaced->align &(~(IMAGE_ALIGN_LEFT | IMAGE_ALIGN_TOP))) {
                if (pImagePlaced->align & IMAGE_ALIGN_CENTER)
                        delta_x = -((signed)width/2);
                else if (pImagePlaced->align & IMAGE_ALIGN_RIGHT)
                        delta_x = -((signed)width);
                if (pImagePlaced->align & IMAGE_ALIGN_MIDDLE)
                        delta_y = -((signed)height/2);
                else if (pImagePlaced->align & IMAGE_ALIGN_BOTTOM)
                        delta_y = -((signed)height);
        }

	// Create a surface from image data
	cairo_surface_t* pSurface = cairo_image_surface_create_for_data(
		pImageItem->pImageData,
		CAIRO_FORMAT_ARGB32, width, height, width*4);
	if (!pSurface) {
		// We failed to get a surface. We will return silently
		return;
	}
	// Save the context for later restore.
	cairo_save (m_pCr);

	// Check if we need to do a matrix transformation
	if (pImagePlaced->pMatrix) {
		cairo_matrix_t matrix;
		cairo_matrix_init(&matrix,
			pImagePlaced->pMatrix->matrix_xx, pImagePlaced->pMatrix->matrix_xy,
			pImagePlaced->pMatrix->matrix_yx, pImagePlaced->pMatrix->matrix_yy,
			pImagePlaced->pMatrix->matrix_x0, pImagePlaced->pMatrix->matrix_y0);
		cairo_transform (m_pCr, &matrix);
	}

	// move to x,y
	cairo_translate(m_pCr, pImagePlaced->x + pImagePlaced->anchor_x, pImagePlaced->y + pImagePlaced->anchor_y);

	// Rotate if we have to
	if (pImagePlaced->rotation != 0.0)
		cairo_rotate(m_pCr, pImagePlaced->rotation);

	// Scale if we have to
	if (pImagePlaced->scale_x != 1.0 || pImagePlaced->scale_y != 1.0) {
		cairo_scale(m_pCr, pImagePlaced->scale_x, pImagePlaced->scale_y);
		//delta_x *= pImagePlaced->scale_x;
		//delta_y *= pImagePlaced->scale_y;
	}

/*
	// Create the context
	cairo_t* pCr = cairo_create(pSurface);
	if (!pCr) {
		// We failed and must clean up
		if (pSurface) cairo_surface_destroy(pSurface);
		return;
	}
*/
	if (pImagePlaced->clip_left > 0.0 || pImagePlaced->clip_right < 1.0 ||
		pImagePlaced->clip_top > 0.0 || pImagePlaced->clip_bottom < 1.0) {
		int clip_x = pImagePlaced->offset_x + delta_x + pImageItem->width * pImagePlaced->clip_left;
		int clip_y = pImagePlaced->offset_y + delta_y + pImageItem->height * pImagePlaced->clip_top;
		int clip_w  = pImageItem->width*(pImagePlaced->clip_right - pImagePlaced->clip_left);
		int clip_h  = pImageItem->height*(pImagePlaced->clip_bottom - pImagePlaced->clip_top);
		cairo_rectangle(m_pCr, clip_x, clip_y, clip_w, clip_h);
		cairo_clip(m_pCr);
		cairo_new_path (m_pCr);
	}
	cairo_set_source_surface(m_pCr, pSurface, delta_x + pImagePlaced->offset_x, delta_y + pImagePlaced->offset_y);
	if (pImagePlaced->alpha < 1.0)
		cairo_paint_with_alpha(m_pCr, pImagePlaced->alpha);
	else cairo_paint(m_pCr);

	// Restore the context again
	cairo_restore (m_pCr);

	// Now destroy the created and used context
	//if (pCr) cairo_destroy(pCr);
	// And destroy the surface
	if (pSurface) cairo_surface_destroy(pSurface);
	pVideoMixer->m_pVideoImage->UpdateMove(place_id);
	return;
}

static int write_png_file(void* data)
{
	void** p = (void**) data;
	cairo_surface_t* surface = (cairo_surface_t*) p[0];

	cairo_surface_write_to_png(surface, (char*) (&p[1]));
	free(p);
	return 0;
}

cairo_status_t CCairoGraphic::WriteFilePNG(cairo_surface_t* surface, char* filename) {

/*	// Fixme. This wont work. We need to copy the surface before creating the thread
	void** p = (void**) malloc(sizeof(void*)*2+strlen(filename)+1);
	p[0] = (void*) surface;
	strcpy((char*)&p[1], filename);
	SDL_CreateThread(write_png_file,p);
	return (cairo_status_t) 0;
*/
	return cairo_surface_write_to_png(surface, filename);
}
