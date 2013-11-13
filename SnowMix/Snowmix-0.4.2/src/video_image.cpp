/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
//#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <png.h>
#include <math.h>
//#include <string.h>


#include "video_image.h"

CVideoImage::CVideoImage(CVideoMixer* pVideoMixer, u_int32_t max_images, u_int32_t max_images_places)
{
	m_image_items = NULL;
	m_image_places = NULL;
	m_verbose = false;
	m_image_items = (struct image_item_t**) calloc(max_images, sizeof(struct image_item_t*));
	m_image_places = (struct image_place_t**) calloc(max_images_places, sizeof(struct image_place_t*));
	m_max_images = max_images;
	m_max_image_places = max_images_places;
	m_pVideoMixer = pVideoMixer;
	m_width = m_pVideoMixer ? m_pVideoMixer->m_geometry_width : 0;
	m_height = m_pVideoMixer ? m_pVideoMixer->m_geometry_height : 0;
	if (!m_pVideoMixer) fprintf(stderr, "Class CVideoImage created with pVideoMixer NULL. Will fail\n");
}

CVideoImage::~CVideoImage()
{
	if (m_image_items) {
		for (unsigned int id=0; id < m_max_images; id++) {
			if (m_image_items[id])
				free(m_image_items[id]->pImageData);
			free(m_image_items[id]);
			m_image_items[id] = NULL;
		}
		free(m_image_items);
		m_image_items = NULL;
	}
	if (m_image_places) {
		for (unsigned int id=0; id < m_max_image_places; id++) {
			if (m_image_places[id]->pMatrix)
				free(m_image_places[id]->pMatrix);
			free(m_image_places[id]);
			m_image_places[id] = NULL;
		}
		free(m_image_places);
		m_image_places = NULL;
	}
}


int CVideoImage::ParseCommand(CController* pController, struct controller_type* ctr,
	const char* str)
{
	if (!strncasecmp (str, "overlay ", 8)) {
		if (m_pVideoMixer) return m_pVideoMixer->OverlayImage(str + 8);
		else return 1;
	}
	// image load [<id no> [<file name>]]  // empty string deletes entry
	if (!strncasecmp (str, "load ", 5)) {
		return set_image_load (ctr, pController, str+5);
	}
	// image place align [<id no> (left | center | right) (top | middle | bottom)]
	else if (!strncasecmp (str, "place align ", 12)) {
		return set_image_place_align(ctr, pController, str+12);
	}
	// image place alpha [<id no> <alpha>]
	else if (!strncasecmp (str, "place alpha ", 12)) {
		return set_image_place_alpha(ctr, pController, str+12);
	}
	// image place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
	else if (!strncasecmp (str, "place clip ", strlen ("place clip "))) {
		 return set_image_place_clip (ctr, pController, str+11);
	}
	// image place move alpha [<place id> <delta_alpha> <step_alpha>]
	else if (!strncasecmp (str, "place move alpha ", 17)) {
		return set_image_place_move_alpha (ctr, pController, str+17);
	}
	// image place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
        //                <left steps> <right steps> <top steps> <bottom steps>]
	else if (!strncasecmp (str, "place move clip ", strlen ("place move clip "))) {
		return set_image_place_move_clip (ctr, pController, str+16);
	}
	// image place move scale [<place id> <delta_scale_x> <delta_scale_y> <step_x> <step_y>]
	else if (!strncasecmp (str, "place move scale ", 17)) {
		return set_image_place_move_scale (ctr, pController, str+17);
	}
	// image place move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]
	else if (!strncasecmp (str, "place move coor ", 16)) {
		return set_image_place_move_coor (ctr, pController, str+16);
	}
	// image place offset [<place id> <offset x> <offset y>]
	else if (!strncasecmp (str, "place offset ", 13)) {
		return set_image_place_offset (ctr, pController, str+13);
	}
	// image place move rotation [<place id> <delta_rotation> <step_rotation>]
	else if (!strncasecmp (str, "place move rotation ", 20)) {
		return set_image_place_move_rotation (ctr, pController, str+20);
	}
	// image place image [<place id> <image id>]
	else if (!strncasecmp (str, "place image ", 12)) {
		return set_image_place_image(ctr, pController, str+12);
	}
	// image place coor [<id no> <>]
	else if (!strncasecmp (str, "place coor ", 11)) {
		return set_image_place(ctr, pController, str+6);
	}
	// image place rotation [<id no> <rotation>]
	else if (!strncasecmp (str, "place rotation ", 15)) {
		return set_image_place_rotation(ctr, pController, str+15);
	}
	// image place scale [<id no> <scale_x> <scale_y>]
	else if (!strncasecmp (str, "place scale ", 12)) {
		return set_image_place_scale(ctr, pController, str+12);
	}
	// image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]
	else if (!strncasecmp (str, "place matrix ", 13)) {
		return set_image_place_matrix(ctr, pController, str+13);
	}
	// image place [<id no> [<image id> <x> <y> [(n | s | e | w | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
	else if (!strncasecmp (str, "place ", 6)) {
		return set_image_place (ctr, pController, str+6);
	}
	// image write <id no> <file name>
	if (!strncasecmp (str, "write ", 6)) {
		return set_image_write (ctr, pController, str+6);
	}
	// image help
	else if (!strncasecmp (str, "help ", 5)) {
		return set_image_help (ctr, pController, str+5);
	}
	// image verbose
	else if (!strncasecmp (str, "verbose ", 8)) {
		m_verbose = !m_verbose; return 0;
		if (m_verbose) fprintf(stderr, "image verbose is set to true\n");
	} else return 1;
	return 0;
}



int CVideoImage::set_image_help(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	pController->controller_write_msg (ctr, "MSG: Commands:\n"
			"MSG:  image help	// this list\n"
			"MSG:  image load [<id no> [<file name>]]  // empty string "
				"deletes entry\n"
			"MSG:  image place [<id no> [<image id> <x> <y> [(left | center | right) "
				"(top | middle | bottom)] [nw | ne | se | sw | n | s | e | w ]]] "
				"// id only deletes entry\n"
			"MSG:  image load [<id no> [<file name>]]  // empty string deletes entry\n"
			"MSG:  image place align [<id no> (left | center | right) (top | middle | bottom)]\n"
			"MSG:  image place alpha [<id no> <alpha>]\n"
			"MSG:  image place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]\n"
			"MSG:  image place move alpha [<place id> <delta_alpha> <step_alpha>]\n"
			"MSG:  image place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom> "
				"<left steps> <right steps> <top steps> <bottom steps>]\n"
			"MSG:  image place move scale [<place id> <delta_scale_x> <delta_scale_y> <step_x> <step_y>]\n"
			"MSG:  image place move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]\n"
			"MSG:  image place offset [<place id> <offset x> <offset y>]\n"
			"MSG:  image place move rotation [<place id> <delta_rotation> <step_rotation>]\n"
			"MSG:  image place image [<place id> <image id>]\n"
			"MSG:  image place coor [<id no> <coor x> <coor y>]\n"
			"MSG:  image place rotation [<id no> <rotation>]\n"
			"MSG:  image place scale [<id no> <scale_x> <scale_y>]\n"
			"MSG:  image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]\n"
			"MSG:  image place [<id no> [<image id> <x> <y> [(n | s | e | w | ne | nw | se | sw)] "
				"[(left | center | right) (top | middle | bottom)]\n"
			"MSG:  image write <id no> <file name>\n"
			"MSG:\n"
			);
	return 0;
}

// image load [<id no> [<file name>]]  // empty string deletes entry
int CVideoImage::set_image_load (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (m_verbose) fprintf(stderr, "image load <%s>\n", ci ? ci : "nil");
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		int max_id = MaxImages();
		for (int i=0; i < max_id; i++) {
			struct image_item_t* pImageItem = GetImageItem(i);
			//controller_write_msg (ctr, "%d %s\n", i, pImageItem ? "ptr" : "NULL");
			if (pImageItem)
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image load %d <%s> %ux%u "
					"bit depth %d type %s\n", i, pImageItem->file_name,
					pImageItem->width, pImageItem->height,
					(int)(pImageItem->bit_depth),
					pImageItem->color_type == PNG_COLOR_TYPE_RGB ? "RGB" :
					  (pImageItem->color_type == PNG_COLOR_TYPE_RGBA ? "RGBA" : "Unsupported"));
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
		if (m_verbose) fprintf(stderr, " - image load <%s>\n", ci ? ci : "nil");
		char* str = (char*) calloc(1,strlen(ci)-1);
		if (!str) {
			if (pController) pController->controller_write_msg (ctr,
				"MSG: Failed to allocate memory for file name %s\n", ci);
			return 0;
		}
		*str = '\0';
		int id;
		int n = sscanf(ci, "%u %[^\n]", &id, str);
		if (n != 1 && n != 2) { free(str); return 1; }
		trim_string(str);
		if (n == 1) n = LoadImage(id, NULL);	// Deletes the entry
		else if (n == 2) n = LoadImage(id, str);	// Adds the entry
		free(str);
		if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image %s.\nMSG:\n", n ? "failed to load" : "loaded");
		return n;
	}
	return 0;
}

// image write <id no> <file name>
int CVideoImage::set_image_write (struct controller_type *ctr, CController* pController, const char* ci)
{
        if (!ci) return -1;
	if (m_verbose) fprintf(stderr, "image write <%s>\n", ci ? ci : "nil");
	while (isspace(*ci)) ci++;
	if (!(*ci)) return -1;
	char* str = (char*) calloc(1,strlen(ci)-1);
	if (!str) {
		if (pController) pController->controller_write_msg (ctr,
			"MSG: Failed to allocate memory for file name %s\n", ci);
		return 1;
	}
	*str = '\0';
	int id;
	int n = sscanf(ci, "%u %[^\n]", &id, str);
	if (n != 2) { free(str); return 1; }
	trim_string(str);
	n = WriteImage(id, str);
	free(str);
	if (m_verbose && pController) pController->controller_write_msg (ctr,
		"MSG: Image %s.\nMSG:\n", n ? "failed to write" : "written");
	return n;
}

// image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]
int CVideoImage::set_image_place_matrix (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: place scale matrix "
					"%2d (%.4lf,%.4lf, %.4lf,%.4lf, %.4lf,%.4lf)\n", id,
					pImage->pMatrix ? pImage->pMatrix->matrix_xx : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_yx : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_xy : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_yy : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_x0 : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_y0 : 0.0);
			}
		}
	} else {
		u_int32_t place_id;
		double xx, xy, yx, yy, x0, y0;
		int n = sscanf(ci, "%u %lf %lf %lf %lf %lf %lf", &place_id, &xx, &xy, &yx, &yy, &x0, &y0);
		if (n == 7)
			return SetImageMatrix(place_id, xx, xy, yx, yy, x0, y0);
		else return 1;		// parameter error
	}

	return 0;
}

// image place scale [<id no> <scale_x> <scale_y>]
int CVideoImage::set_image_place_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pImage->image_id]) {
					w =  m_image_items[pImage->image_id]->width;
					h =  m_image_items[pImage->image_id]->height;
				}
				pController->controller_write_msg (ctr, "MSG: place scale "
					"%2d %.4lf %.4lf placed at %d,%d wxh (%4ux%-3u)\n", id,
					pImage->scale_x, pImage->scale_y,
					pImage->x, pImage->y, w, h);
			}
		}
	} else {
		u_int32_t place_id;
		double scale_x, scale_y;
		int n = sscanf(ci, "%u %lf %lf", &place_id, &scale_x, &scale_y);
		if (n == 3) SetImageScale(place_id, scale_x, scale_y);
		else return 1;		// parameter error
	}

	return 0;
}

// image place alpha [<id no> <alpha>]
int CVideoImage::set_image_place_alpha (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: place alpha "
					"%2d %.5lf\n", id, pImage->alpha);
			}
		}
	} else {
		u_int32_t place_id;
		double alpha;
		int n = sscanf(ci, "%u %lf", &place_id, &alpha);
		if (n == 2) SetImageAlpha(place_id, alpha);
		else return 1;		// parameter error
	}

	return 0;
}

// image place rotation [<id no> <rotation>]
int CVideoImage::set_image_place_rotation (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pImage->image_id]) {
					w =  m_image_items[pImage->image_id]->width;
					h =  m_image_items[pImage->image_id]->height;
				}
				pController->controller_write_msg (ctr, "MSG: place rotation "
					"%2d %.5lf placed at %5d,%-5d wxh %4ux%-3u\n", id,
					pImage->rotation,
					pImage->x, pImage->y, w, h);
			}
		}
	} else {
		u_int32_t place_id;
		double rotation;
		int n = sscanf(ci, "%u %lf", &place_id, &rotation);
		if (n == 2) SetImageRotation(place_id, rotation);
		else return 1;		// parameter error
	}

	return 0;
}

//image place [<id no> (left | center | right) (top | middle | bottom)] "
int CVideoImage::set_image_place_align (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str)) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pPlacedImage->image_id]) {
					w =  m_image_items[pPlacedImage->image_id]->width;
					h =  m_image_items[pPlacedImage->image_id]->height;
				}
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d x,y %5d,%-5d "
					"wxh %4dx%-4d rot %.4lf %s %s\n",
					id, pPlacedImage->x, pPlacedImage->y,
					w, h, pPlacedImage->rotation,
					pPlacedImage->align & IMAGE_ALIGN_CENTER ? "center" :
					  (pPlacedImage->align & IMAGE_ALIGN_RIGHT ? "right" : "left"),
					pPlacedImage->align & IMAGE_ALIGN_MIDDLE ? "middle" :
					  (pPlacedImage->align & IMAGE_ALIGN_BOTTOM ? "bottom" : "top"));
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
fprintf(stderr, "image align <%s>\n", str);
		u_int32_t place_id;
		//u_int32_t align = IMAGE_ALIGN_LEFT | IMAGE_ALIGN_TOP;
		int n = sscanf(str, "%u", &place_id);
		if (n == 1) {
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			while (*str) {
				int align = 0;
				if (strncasecmp(str, "left", 4) == 0) {
					align |= IMAGE_ALIGN_LEFT; str += 4;
				} else if (strncasecmp(str, "center", 6) == 0) {
					align |= IMAGE_ALIGN_CENTER; str += 6;
				} else if (strncasecmp(str, "right", 5) == 0) {
					align |= IMAGE_ALIGN_RIGHT; str += 5;
				} else if (strncasecmp(str, "top", 3) == 0) {
					align |= IMAGE_ALIGN_TOP; str += 3;
				} else if (strncasecmp(str, "middle", 6) == 0) {
					align |= IMAGE_ALIGN_MIDDLE; str += 6;
				} else if (strncasecmp(str, "bottom", 6) == 0) {
					align |= IMAGE_ALIGN_BOTTOM; str += 6;
				} else {
					return 1;		// parameter error
				}
	 			SetImageAlign(place_id, align);
				while (isspace(*str)) str++;
			}
			if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image Align Set.\nMSG:\n");
		} else return 1;
	}
	return 0;
}

// image place coor [<place id no> [<x> <y> [(n | s | e | w | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
// image place [<place id no> [<image id> <x> <y> [(n | s | e | w | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
int CVideoImage::set_image_place (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str) || strcasecmp(str, "coor ") == 0) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pPlacedImage->image_id]) {
					w =  m_image_items[pPlacedImage->image_id]->width;
					h =  m_image_items[pPlacedImage->image_id]->height;
				}
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d image_id %2d x,y %5d,%-5d "
					"wxh %4dx%-4d rot %.4lf alpha %.3lf anchor %u,%u %s %s %s\n",
					id, pPlacedImage->image_id, pPlacedImage->x, pPlacedImage->y,
					w, h, pPlacedImage->rotation, pPlacedImage->alpha,
					pPlacedImage->anchor_x, pPlacedImage->anchor_y,
					pPlacedImage->align & IMAGE_ALIGN_CENTER ? "center" :
					  (pPlacedImage->align & IMAGE_ALIGN_RIGHT ? "right" : "left"),
					pPlacedImage->align & IMAGE_ALIGN_MIDDLE ? "middle" :
					  (pPlacedImage->align & IMAGE_ALIGN_BOTTOM ? "bottom" : "top"),
					  
					pPlacedImage->display ? "on" : "off");
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
		u_int32_t place_id, image_id;
		int32_t x, y;
		char anchor[3];
		bool coor = false;
		//u_int32_t align = IMAGE_ALIGN_LEFT | IMAGE_ALIGN_TOP;
		anchor[0] = '\0';
		if (!strncasecmp(str, "coor ", 5)) {
			coor = true;
			str += 5;
			while (isspace(*str)) str++;
		}
		int n = 0;
		if (coor) n = sscanf(str, "%u %d %d %2[nsew]", &place_id, &x, &y, &anchor[0]);
		else n = sscanf(str, "%u %u %d %d %2[nsew]", &place_id, &image_id, &x, &y, &anchor[0]);

		// Should we delete the entry or place it
		if (n == 1) {
			RemoveImagePlaced(place_id);
			if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image removed.\nMSG:\n");
		}
		else if (n == 4 || (coor && n == 3) || (!coor && n == 5)) {
			int i = SetImagePlace(place_id, image_id, x, y, true, coor, anchor);
			if (i) return i;
			for (i=0; i < (coor ? 3 : 4) ; i++) {
				while (isspace(*str)) str++;
				if (*str == '-' || *str == '+') str++;
				while (isdigit(*str)) str++;
			}
			if ((coor && n == 4) || (!coor && n ==5)) {
				while (isspace(*str)) str++;
				while (*str == 'n' || *str == 's' || *str == 'e' || *str == 'w') str++;
			}
			while (isspace(*str)) str++;
			while (*str) {
				int align = 0;
				if (strncasecmp(str, "left", 4) == 0) {
					align |= IMAGE_ALIGN_LEFT; str += 4;
				} else if (strncasecmp(str, "center", 6) == 0) {
					align |= IMAGE_ALIGN_CENTER; str += 6;
				} else if (strncasecmp(str, "right", 5) == 0) {
					align |= IMAGE_ALIGN_RIGHT; str += 5;
				} else if (strncasecmp(str, "top", 3) == 0) {
					align |= IMAGE_ALIGN_TOP; str += 3;
				} else if (strncasecmp(str, "middle", 6) == 0) {
					align |= IMAGE_ALIGN_MIDDLE; str += 6;
				} else if (strncasecmp(str, "bottom", 6) == 0) {
					align |= IMAGE_ALIGN_BOTTOM; str += 6;
				} else {
					return 1;		// parameter error
				}
	 			SetImageAlign(place_id, align);
				while (isspace(*str)) str++;
			}
			if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image placed.\nMSG:\n");
		} else return 1;
	}
	return 0;
}
// image place image [<place id no> <image id>]
int CVideoImage::set_image_place_image (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str) || strcasecmp(str, "coor ") == 0) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				char* file_name = NULL;
				if (m_image_items && m_image_items[pPlacedImage->image_id])
					file_name = m_image_items[pPlacedImage->image_id]->file_name;
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d image_id %2d %s\n",
					id, pPlacedImage->image_id, file_name ? file_name : "null");
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
		u_int32_t place_id, image_id;
		int n = sscanf(str, "%u %u", &place_id, &image_id);
		if (n == 2) SetImagePlaceImage(place_id, image_id);
		else return -1;
	}
	return 0;
}


// image place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
//                <left steps> <right steps> <top steps> <bottom steps>]
int CVideoImage::set_image_place_move_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"clip %2d %.3lf %.3lf %.3lf %.3lf %u %u %u %u\n", id,
					pImage->delta_clip_left, pImage->delta_clip_right,
					pImage->delta_clip_top, pImage->delta_clip_bottom,
					pImage->delta_clip_left_steps, pImage->delta_clip_right_steps,
					pImage->delta_clip_top_steps, pImage->delta_clip_bottom_steps
					);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, step_left, step_right, step_top, step_bottom;
	double delta_clip_left, delta_clip_right, delta_clip_top, delta_clip_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf %u %u %u %u",
		&place_id, &delta_clip_left, &delta_clip_right, &delta_clip_top, &delta_clip_bottom,
		&step_left, &step_right, &step_top, &step_bottom);
	if (n == 9) SetImageMoveClip(place_id, delta_clip_left, delta_clip_right, delta_clip_top,
		delta_clip_bottom, step_left, step_right, step_top, step_bottom);
	else return 1;		// parameter error

	return 0;
}

// image place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
int CVideoImage::set_image_place_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place "
					"clip %2d %.3lf %.3lf %.3lf %.3lf\n", id,
					pImage->clip_left, pImage->clip_right,
					pImage->clip_top, pImage->clip_bottom);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double clip_left, clip_right, clip_top, clip_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf",
		&place_id, &clip_left, &clip_right, &clip_top, &clip_bottom);
	if (n == 5) SetImageClip(place_id, clip_left, clip_right, clip_top, clip_bottom);
	else return 1;		// parameter error

	return 0;
}
// image place move scale [<id no> [<delta_scale_x> <delta_scale_y> <steps_x> <steps_y>]]
int CVideoImage::set_image_place_move_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"scale %2d %.3lf %.3lf %u %u\n", id,
					pImagePlaced->delta_scale_x, pImagePlaced->delta_scale_y,
					pImagePlaced->delta_scale_x_steps,
					pImagePlaced->delta_scale_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	double delta_scale_x, delta_scale_y;
	int n = sscanf(ci, "%u %lf %lf %u %u", &place_id, &delta_scale_x, &delta_scale_y, &dxs, &dys);
	if (n == 5) SetImageMoveScale(place_id, delta_scale_x, delta_scale_y, dxs, dys);
	else return 1;		// parameter error

	return 0;
}

// image place offset [<id no> <offset x> <offset y>]
int CVideoImage::set_image_place_offset (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place offset"
					" %2d at %d,%d offset %d,%d\n", id,
					pImagePlaced->x, pImagePlaced->y,
					pImagePlaced->offset_x, pImagePlaced->offset_y);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	int32_t offset_x, offset_y;
	int n = sscanf(ci, "%u %d %d", &place_id, &offset_x, &offset_y);
	if (n == 3) SetImageOffset(place_id, offset_x, offset_y);
	else return 1;		// parameter error

	return 0;
}

// image place move coor [<id no> [<delta_x> <delta_y> <steps_x> <steps_y>]]
int CVideoImage::set_image_place_move_coor (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move coor"
					" %2d %d %d %u %u\n", id,
					pImagePlaced->delta_x, pImagePlaced->delta_y,
					pImagePlaced->delta_x_steps, pImagePlaced->delta_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	int32_t dx, dy;
	int n = sscanf(ci, "%u %d %d %u %u", &place_id, &dx, &dy, &dxs, &dys);
	if (n == 5) SetImageMoveCoor(place_id, dx, dy, dxs, dys);
	else return 1;		// parameter error

	return 0;
}

// image place move alpha [<place id> <delta_alpha> <step_alpha>]
int CVideoImage::set_image_place_move_alpha (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"alpha %2d %.6lf %u\n", id,
					pImagePlaced->delta_alpha, 
					pImagePlaced->delta_alpha_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_alpha_steps;
	double delta_alpha;
	int n = sscanf(ci, "%u %lf %u", &place_id, &delta_alpha, &delta_alpha_steps);
	if (n == 3) SetImageMoveAlpha(place_id, delta_alpha, delta_alpha_steps);
	else return 1;		// parameter error

	return 0;
}

// image place move rotation [<place id> <delta_rotation> <step_rotation>]
int CVideoImage::set_image_place_move_rotation (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"rotation %2d %.6lf %u\n", id,
					pImagePlaced->delta_rotation, 
					pImagePlaced->delta_rotation_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_rotation_steps;
	double delta_rotation;
	int n = sscanf(ci, "%u %lf %u", &place_id, &delta_rotation, &delta_rotation_steps);
	if (n == 3) SetImageMoveRotation(place_id, delta_rotation, delta_rotation_steps);
	else return 1;		// parameter error

	return 0;
}

int CVideoImage::SetImageMatrix(u_int32_t place_id, double xx, double xy, double yx, double yy, double x0, double y0)
{
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		if (!pImage->pMatrix)
			pImage->pMatrix = (transform_matrix_t*)
				calloc(sizeof(transform_matrix_t), 1);
		if (!pImage->pMatrix) return 1;
		pImage->pMatrix->matrix_xx = xx;
		pImage->pMatrix->matrix_xy = xy;
		pImage->pMatrix->matrix_yx = yx;
		pImage->pMatrix->matrix_yy = yy;
		pImage->pMatrix->matrix_x0 = x0;
		pImage->pMatrix->matrix_y0 = y0;
	} else return -1;
	return 0;
}

void CVideoImage::SetImageClip(u_int32_t place_id, double clip_left, double clip_right,
	double clip_top, double clip_bottom) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];

		if (clip_left < 0.0) clip_left = 0.0;
		else if (clip_left > 1.0) clip_left = 1.0;
		if (clip_right < 0.0) clip_right = 0.0;
		else if (clip_right > 1.0) clip_right = 1.0;
		if (clip_top < 0.0) clip_top = 0.0;
		else if (clip_top > 1.0) clip_top = 1.0;
		if (clip_bottom < 0.0) clip_bottom = 0.0;
		else if (clip_bottom > 1.0) clip_bottom = 1.0;
		pImage->clip_left = clip_left;
		pImage->clip_right = clip_right;
		pImage->clip_top = clip_top;
		pImage->clip_bottom = clip_bottom;
	}
}

void CVideoImage::SetImageMoveClip(u_int32_t place_id, double delta_clip_left,
	double delta_clip_right, double delta_clip_top, double delta_clip_bottom,
	u_int32_t step_left, u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];

		if (delta_clip_left < -1.0) delta_clip_left = -1.0;
		else if (delta_clip_left > 1.0) delta_clip_left = 1.0;
		if (delta_clip_right < -1.0) delta_clip_right = -1.0;
		else if (delta_clip_right > 1.0) delta_clip_right = 1.0;
		if (delta_clip_top < -1.0) delta_clip_top = -1.0;
		else if (delta_clip_top > 1.0) delta_clip_top = 1.0;
		if (delta_clip_bottom < -1.0) delta_clip_bottom = -1.0;
		else if (delta_clip_bottom > 1.0) delta_clip_bottom = 1.0;

		pImage->delta_clip_left = delta_clip_left;
		pImage->delta_clip_right = delta_clip_right;
		pImage->delta_clip_top = delta_clip_top;
		pImage->delta_clip_bottom = delta_clip_bottom;
		pImage->delta_clip_left_steps = step_left;
		pImage->delta_clip_right_steps = step_right;
		pImage->delta_clip_top_steps = step_top;
		pImage->delta_clip_bottom_steps = step_bottom;
	}
}
void CVideoImage::SetImageMoveAlpha(u_int32_t place_id, double delta_alpha,
	u_int32_t delta_alpha_steps) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		pImage->delta_alpha = delta_alpha;
		pImage->delta_alpha_steps = delta_alpha_steps;

	}
}
void CVideoImage::SetImageMoveRotation(u_int32_t place_id, double delta_rotation,
	u_int32_t delta_rotation_steps) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		pImage->delta_rotation = delta_rotation;
		pImage->delta_rotation_steps = delta_rotation_steps;

	}
}
void CVideoImage::SetImageMoveScale(u_int32_t place_id, double delta_scale_x,
	double delta_scale_y, u_int32_t delta_scale_x_steps, u_int32_t delta_scale_y_steps) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		pImage->delta_scale_x = delta_scale_x;
		pImage->delta_scale_y = delta_scale_y;
		pImage->delta_scale_x_steps = delta_scale_x_steps;
		pImage->delta_scale_y_steps = delta_scale_y_steps;

	}
}
void CVideoImage::SetImageOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y)
{
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		pImage->offset_x = offset_x;
		pImage->offset_y = offset_y;
	}
}
void CVideoImage::SetImageMoveCoor(u_int32_t place_id, int32_t delta_x, int32_t delta_y,
	u_int32_t delta_x_steps, u_int32_t delta_y_steps) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		pImage->delta_x = delta_x;
		pImage->delta_y = delta_y;
		pImage->delta_x_steps = delta_x_steps;
		pImage->delta_y_steps = delta_y_steps;

	}
}
void CVideoImage::SetImageScale(u_int32_t place_id, double scale_x, double scale_y) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		m_image_places[place_id]->scale_x = scale_x;
		m_image_places[place_id]->scale_y = scale_y;
	}
}

void CVideoImage::SetImageAlpha(u_int32_t place_id, double alpha) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		m_image_places[place_id]->alpha = alpha;
	}
}

void CVideoImage::SetImageRotation(u_int32_t place_id, double rotation) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		m_image_places[place_id]->rotation = rotation;
	}
}

// Max places image possible
unsigned int CVideoImage::MaxImagesPlace()
{
	return m_max_image_places;
}

//
void CVideoImage::SetImageAlign(u_int32_t place_id, int align) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		if (align & (IMAGE_ALIGN_LEFT | IMAGE_ALIGN_CENTER | IMAGE_ALIGN_RIGHT)) {
			m_image_places[place_id]->align = (m_image_places[place_id]->align &
				(~(IMAGE_ALIGN_LEFT | IMAGE_ALIGN_CENTER | IMAGE_ALIGN_RIGHT))) | align;
		} else if  (align & (IMAGE_ALIGN_TOP | IMAGE_ALIGN_MIDDLE | IMAGE_ALIGN_BOTTOM)) {
			m_image_places[place_id]->align = (m_image_places[place_id]->align &
				(~(IMAGE_ALIGN_TOP  | IMAGE_ALIGN_MIDDLE | IMAGE_ALIGN_BOTTOM))) | align;
		}
	}
}

// Change the image of a placed image
int CVideoImage::SetImagePlaceImage(u_int32_t place_id, u_int32_t image_id)
{
	if (place_id >= m_max_image_places || !m_image_places || !m_image_places[place_id])
		return 1;
	m_image_places[place_id]->image_id = image_id;
	return 0;
}

// Create a image placed and add to image place list
int CVideoImage::SetImagePlace(u_int32_t place_id, u_int32_t image_id, int32_t x, int32_t y, bool display, bool coor, const char* anchor)
{
	if (place_id >= m_max_image_places || !m_image_places ||
		(coor && !m_image_places[place_id]))
		return 1;
	struct image_place_t* pImagePlaced = NULL;
	if (!m_image_places[place_id]) {
		if (!(m_image_places[place_id] =
			(struct image_place_t*) calloc(sizeof(struct image_place_t), 1)))
			return 1;
		if ((pImagePlaced =m_image_places[place_id])) {
			pImagePlaced->align = IMAGE_ALIGN_TOP | IMAGE_ALIGN_LEFT;
			pImagePlaced->anchor_x = 0;
			pImagePlaced->anchor_y = 0;
			pImagePlaced->offset_x = 0;
			pImagePlaced->offset_y = 0;
			pImagePlaced->scale_x = 1.0;
			pImagePlaced->scale_y = 1.0;
			pImagePlaced->display = true;
			pImagePlaced->alpha = 1.0;
			pImagePlaced->rotation = 0.0;
			pImagePlaced->pMatrix = NULL;
			pImagePlaced->delta_x		= 0;
			pImagePlaced->delta_x_steps	= 0;
			pImagePlaced->delta_y		= 0;
			pImagePlaced->delta_y_steps	= 0;
			pImagePlaced->delta_scale_x	= 0.0;
			pImagePlaced->delta_scale_x_steps  = 0;
			pImagePlaced->delta_scale_y	= 0.0;
			pImagePlaced->delta_scale_y_steps  = 0;
			pImagePlaced->delta_rotation	= 0.0;
			pImagePlaced->delta_rotation_steps = 0;
			pImagePlaced->delta_alpha	= 0.0;
			pImagePlaced->delta_alpha_steps	= 0;
			pImagePlaced->clip_left		= 0.0;
			pImagePlaced->clip_right	= 1.0;
			pImagePlaced->clip_top		= 0.0;
			pImagePlaced->clip_bottom	= 1.0;
		}
	} 
	pImagePlaced = m_image_places[place_id];
	if (!coor) pImagePlaced->image_id = image_id;
	pImagePlaced->x = x;
	pImagePlaced->y = y;

	// default anchor nw
	if (anchor && *anchor) SetAnchor(place_id, anchor);

	return 0;
}


void CVideoImage::SetAnchor(u_int32_t place_id, const char* anchor)
{
	if (!anchor || place_id >= m_max_image_places || !m_image_places[place_id]) return;
	image_place_t* pImage = m_image_places[place_id];
	if (strcasecmp("ne", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = 0;
	} else if (strcasecmp("se", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("sw", anchor) == 0) {
		pImage->anchor_x = 0;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("n", anchor) == 0) {
		pImage->anchor_x = m_width>>1;
		pImage->anchor_y = 0;
	} else if (strcasecmp("w", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height>>1;
	} else if (strcasecmp("s", anchor) == 0) {
		pImage->anchor_x = m_width>>1;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("e", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height>>1;
	} else {
		pImage->anchor_x = 0;
		pImage->anchor_y = 0;
	}
}

// Remove a placed image
int CVideoImage::RemoveImagePlaced(u_int32_t image_id)
{
	if (image_id >= m_max_image_places || !m_image_places)
		return 1;
	if (m_image_places[image_id]) free(m_image_places[image_id]);
	m_image_places[image_id] = NULL;
	return 0;
}

// Get a placed image
struct image_place_t* CVideoImage::GetImagePlaced(unsigned int id)
{
	if (id >= m_max_image_places || !m_image_places) return NULL;
	return m_image_places[id];
}

// Max images to load from files
unsigned int CVideoImage::MaxImages()
{
	return m_max_images;
}

// Image item loaded from file
struct image_item_t* CVideoImage::GetImageItem(unsigned int i)
{
	return (i >= m_max_images ? NULL : m_image_items[i]);
}


void CVideoImage::UpdateMove(u_int32_t place_id)
{
	image_place_t* p = GetImagePlaced(place_id);
	if (!p) return;

	// check for 'image place move coor'
	if (p->delta_x_steps > 0) {
		p->delta_x_steps--;
		p->x += p->delta_x;
	}
	if (p->delta_y_steps > 0) {
		p->delta_y_steps--;
		p->y += p->delta_y;
	}

	// check for 'image place move scale'
	if (p->delta_scale_x_steps > 0) {
		p->delta_scale_x_steps--;
		p->scale_x += p->delta_scale_x;
		if (p->scale_x > 10000.0) p->scale_x = 10000.0;
		else if (p->scale_x < 0.0) p->scale_x = 0.0;
	}
	// check for 'image place move scale'
	if (p->delta_scale_y_steps > 0) {
		p->delta_scale_y_steps--;
		p->scale_y += p->delta_scale_y;
		if (p->scale_y > 10000.0) p->scale_y = 10000.0;
		else if (p->scale_y < 0.0) p->scale_y = 0.0;
	}

	// check for 'image place move alpha'
	if (p->delta_alpha_steps > 0) {
		p->delta_alpha_steps--;
		p->alpha += p->delta_alpha;
		if (p->alpha > 1.0) p->alpha = 1.0;
		else if (p->alpha < 0.0) p->alpha = 0.0;
	}

	// check for 'image place move rotation'
	if (p->delta_rotation_steps > 0) {
		p->delta_rotation_steps--;
		p->rotation += p->delta_rotation;
//fprintf(stderr, "Rotation %.3lf\n", p->rotation);
		if (p->rotation > 2*M_PI)
			p->rotation -= (2*M_PI);
		else if (p->rotation < -2*M_PI)
			p->rotation += (2*M_PI);
	}
	// check for 'image place move clip'
	if (p->delta_clip_left_steps > 0) {
		p->delta_clip_left_steps--;
		p->clip_left += p->delta_clip_left;
		if (p->clip_left > 1.0) p->clip_left = 1.0;
		else if (p->clip_left < 0.0) p->clip_left = 0.0;
	}
	// check for 'image place move clip'
	if (p->delta_clip_right_steps > 0) {
		p->delta_clip_right_steps--;
		p->clip_right += p->delta_clip_right;
		if (p->clip_right > 1.0) p->clip_right = 1.0;
		else if (p->clip_right < 0.0) p->clip_right = 0.0;
	}
	// check for 'image place move clip'
	if (p->delta_clip_top_steps > 0) {
		p->delta_clip_top_steps--;
		p->clip_top += p->delta_clip_top;
		if (p->clip_top > 1.0) p->clip_top = 1.0;
		else if (p->clip_top < 0.0) p->clip_top = 0.0;
	}
	// check for 'image place move clip'
	if (p->delta_clip_bottom_steps > 0) {
		p->delta_clip_bottom_steps--;
		p->clip_bottom += p->delta_clip_bottom;
		if (p->clip_bottom > 1.0) p->clip_bottom = 1.0;
		else if (p->clip_bottom < 0.0) p->clip_bottom = 0.0;
	}
}

int CVideoImage::WriteImage(u_int32_t image_id, const char* file_name)
{
fprintf(stderr, "IMAGE WRITE\n");
	if (image_id >= m_max_images) {
		fprintf(stderr, "Image id %u larger than configured max %u\n", image_id, m_max_images-1);
		return -1;
	}
	if (!m_image_items[image_id] || !m_image_items[image_id]->pImageData) {
		fprintf(stderr, "Image id %u does not exist or has no image data\n", image_id);
		return -1;
	}
	if (!file_name || !(*file_name)) {
		fprintf(stderr, "File name error\n");
		return -1;
	}
	struct image_item_t* pImage = m_image_items[image_id];
	int fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
	if (fd < 0) {
		perror("Opening file error");
		fprintf(stderr, "File opening error for file %s\n", file_name);
		return -1;
	}
	int written = 0;
	int to_write = 4 * pImage->width * pImage->height;
	while (written < to_write) {
		int n = write(fd, pImage->pImageData, to_write - written);
		if (n < 1) {
			fprintf(stderr, "File write error for file %s\n", file_name);
			close(fd);
			return -1;
		}
		written += n;
	}
	close(fd);
	return 0;
}

// load image into image item list
int CVideoImage::LoadImage(u_int32_t image_id, const char* file_name)
{
	png_byte header[8];    // 8 is the maximum size that can be checked
	struct image_item_t* pImage = NULL;
	FILE* fp = NULL;
	int number_of_passes = 0;
	int n = 0;
	if (m_verbose)
		fprintf(stderr, "Load Image %u <%s>\n", image_id, file_name ?
			file_name : "no file name given");

	if (image_id >= m_max_images) {
		fprintf(stderr, "Image id %u larger than configured max %u\n", image_id, m_max_images-1);
		return -1;
	}

	if (!file_name) {
		if (m_image_items[image_id]) {
			free(m_image_items[image_id]);
			m_image_items[image_id] = NULL;
		}
		return 0;
	}

	if (!(pImage = (struct image_item_t*) calloc(sizeof(struct image_item_t)+strlen(file_name)+1,1))) {
		fprintf(stderr, "failed to allocate memory for iamge for png.\n");
		goto error_free_and_return;
	}
	pImage->id = image_id;
	pImage->file_name = ((char*) pImage) + sizeof(struct image_item_t);
	strcpy(pImage->file_name, file_name);

	/* open file and test for it being a png */
	fp = fopen(file_name, "rb");
	if (!fp) {
		fprintf(stderr, "File %s could not be opened for reading for png\n", file_name);
		goto error_free_and_return;
	}
	n = fread(header, 1, 8, fp);
	if (n != 8 || png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "File %s is not recognized as a PNG file\n", file_name);
		goto error_free_and_return;
	}


	/* initialize stuff */
	if (!(pImage->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		fprintf(stderr, "Failed to create read structure for png file %s\n", file_name);
		goto error_free_and_return;
	}

	if (!(pImage->info_ptr = png_create_info_struct(pImage->png_ptr))) {
		fprintf(stderr, "Failed to create info structure for png file %s\n", file_name);
		goto error_free_and_return;
	}

	if (setjmp(png_jmpbuf(pImage->png_ptr))) {
		fprintf(stderr, "Error while init for png file %s\n", file_name);
		goto error_free_and_return;
	}

	png_init_io(pImage->png_ptr, fp);
	png_set_sig_bytes(pImage->png_ptr, 8);

	png_read_info(pImage->png_ptr, pImage->info_ptr);

	pImage->width = png_get_image_width(pImage->png_ptr, pImage->info_ptr);
	pImage->height = png_get_image_height(pImage->png_ptr, pImage->info_ptr);
	pImage->color_type = png_get_color_type(pImage->png_ptr, pImage->info_ptr);
	pImage->bit_depth = png_get_bit_depth(pImage->png_ptr, pImage->info_ptr);

	number_of_passes = png_set_interlace_handling(pImage->png_ptr);
	png_read_update_info(pImage->png_ptr, pImage->info_ptr);


	/* read file */
	if (setjmp(png_jmpbuf(pImage->png_ptr))) {
		fprintf(stderr, "Error during read_image for png file %s\n", file_name);
		goto error_free_and_return;
	}

	pImage->row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * pImage->height);
	for (unsigned int y=0; y<pImage->height; y++)
		pImage->row_pointers[y] = (png_byte*)
			malloc(png_get_rowbytes(pImage->png_ptr,pImage->info_ptr));

	png_read_image(pImage->png_ptr, pImage->row_pointers);

	pImage->pImageData =
			(u_int8_t*) malloc(pImage->width*pImage->height*4);
	if (pImage->pImageData) {
		u_int8_t* pTmp = pImage->pImageData;
		for (unsigned int iy=0; iy < pImage->height; iy++) {
			png_byte* row = pImage->row_pointers[iy];
			for (unsigned int ix=0; ix < pImage->width; ix++) {
				png_byte* ptr = &(row[ix*4]);
				*pImage->pImageData++ = ptr[2];
				*pImage->pImageData++ = ptr[1];
				*pImage->pImageData++ = ptr[0];
				*pImage->pImageData++ = ptr[3];
			}
			//free(pImage->row_pointers[iy]);
		}
		pImage->pImageData = pTmp;
	}

	fclose(fp);
	if (pImage->row_pointers) {
		//free(pImage->row_pointers);
		//pImage->row_pointers = NULL;
	}
	if (m_verbose) fprintf(stderr, "Image %u created\n", image_id);

	m_image_items[image_id] = pImage;
	return 0;

error_free_and_return:
	if (pImage) free(pImage);
	if (fp) fclose(fp);
	return -1;
}

int CVideoImage::OverlayImage(const u_int32_t place_id, u_int8_t* pY, const u_int32_t width,
	const u_int32_t height)
{
	struct image_place_t* pImagePlaced = GetImagePlaced(place_id);

	// look up image placed and check we have an image item
	if (!pImagePlaced || pImagePlaced->image_id >= m_max_images ||
		!m_image_items[pImagePlaced->image_id] || !pImagePlaced->display) return 0;

	int x = pImagePlaced->x;
	int y = pImagePlaced->y;
	u_int32_t image_id = pImagePlaced->image_id;	// Should be the same id

	// See if align is different from align left and top
	if (pImagePlaced->align &(~(IMAGE_ALIGN_LEFT | IMAGE_ALIGN_TOP))) {
		if (pImagePlaced->align & IMAGE_ALIGN_CENTER)
			x -= m_image_items[image_id]->width/2;
		else if (pImagePlaced->align & IMAGE_ALIGN_RIGHT)
			x -= m_image_items[image_id]->width;
		if (pImagePlaced->align & IMAGE_ALIGN_MIDDLE)
			y -= m_image_items[image_id]->height/2;
		else if (pImagePlaced->align & IMAGE_ALIGN_BOTTOM)
			y -= m_image_items[image_id]->height;
	}
fprintf(stderr, "Place image at %d,%d -> %d,%d\n", pImagePlaced->x, pImagePlaced->y, x,y);

	pY = pY + 4*(y*width+x);
	if (m_image_items[image_id]->pImageData) {
		u_int8_t* ptr = m_image_items[image_id]->pImageData;
		for (unsigned int iy=0; iy<m_image_items[image_id]->height; iy++) {
			u_int8_t* pY_tmp = pY + 4*width;
			for (unsigned int ix=0; ix<m_image_items[image_id]->width; ix++) {
				if (ptr[3]) {
					if (ptr[3] >= 255) {
						*pY++ = ptr[0];
						*pY++ = ptr[1];
						*pY++ = ptr[2];
						pY++;
					} else {
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[0]))*ptr[3]))/255;
						pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[1]))*ptr[3]))/255;
						pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[2]))*ptr[3]))/255;
						pY++;
						pY++;
					}
				} else pY += 4;
				ptr += 4;
			}
			pY = pY_tmp;
		}
	} else {
		fprintf(stderr, "Image without image data ???\n");
/*
		struct image_item_t* pImage = m_image_items[image_id];
		u_int8_t* p = pImage->pImageData =
			(u_int8_t*) malloc(pImage->width*pImage->height*4);
		for (unsigned int iy=0; iy < pImage->height; iy++) {
			//png_byte* row = m_image_items[image_id]->row_pointers[iy];
			png_byte* row = pImage->row_pointers[iy];
			u_int8_t* pY_tmp = pY + 4*width;
			for (unsigned int ix=0; ix < pImage->width; ix++) {
				png_byte* ptr = &(row[ix*4]);
				//printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
				// x, y, ptr[0], ptr[1], ptr[2], ptr[3]);

				*p++ = ptr[2];
				*p++ = ptr[1];
				*p++ = ptr[0];
				*p++ = ptr[3];
				if (ptr[3]) {
					if (ptr[3] >= 255) {
						*pY++ = ptr[2];
						*pY++ = ptr[1];
						*pY++ = ptr[0];
						pY++;
					} else {
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[2]))*ptr[3]))/255; pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[1]))*ptr[3]))/255; pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[0]))*ptr[3]))/255; pY++;
						pY++;
					}
				} else pY += 4;
			}
			pY = pY_tmp;
		}
*/
	}
	return 0;
}
