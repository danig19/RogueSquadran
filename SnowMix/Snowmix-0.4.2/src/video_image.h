/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_IMAGE_H__
#define __VIDEO_IMAGE_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
#include <png.h>


#define	IMAGE_ALIGN_LEFT   1
#define	IMAGE_ALIGN_CENTER 2
#define	IMAGE_ALIGN_RIGHT  4
#define	IMAGE_ALIGN_TOP    8
#define	IMAGE_ALIGN_MIDDLE 16
#define	IMAGE_ALIGN_BOTTOM 32

struct image_item_t {
	char*		file_name;
	u_int32_t	id;
	u_int32_t	width;
	u_int32_t	height;
	png_byte	bit_depth;
	png_byte	color_type;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep*	row_pointers;

	// Pointer to RGBA data - 32 bit 
	u_int8_t*	pImageData;
};

struct image_place_t {
	u_int32_t	image_id;
	int32_t		x;
	int32_t		y;
	int32_t		anchor_x;
	int32_t		anchor_y;
	int32_t		offset_x;
	int32_t		offset_y;
	u_int32_t	align;
	bool		display;

	double		scale_x;
	double		scale_y;
	double		rotation;
	double		alpha;
	struct transform_matrix_t* pMatrix;

	// move parameters
	int32_t		delta_x;
	u_int32_t	delta_x_steps;
	int32_t		delta_y;
	u_int32_t	delta_y_steps;
	double		delta_scale_x;
	u_int32_t	delta_scale_x_steps;
	double		delta_scale_y;
	u_int32_t	delta_scale_y_steps;
	double		delta_rotation;
	u_int32_t	delta_rotation_steps;
	double		delta_alpha;
	u_int32_t	delta_alpha_steps;
	//double		delta_alpha_bg;
	//u_int32_t	delta_alpha_bg_steps;
        double                  delta_clip_left;
        u_int32_t               delta_clip_left_steps;
        double                  delta_clip_right;
        u_int32_t               delta_clip_right_steps;
        double                  delta_clip_top;
        u_int32_t               delta_clip_top_steps;
        double                  delta_clip_bottom;
        u_int32_t               delta_clip_bottom_steps;


        // Clip parameters
        double                  clip_left;
        double                  clip_right;
        double                  clip_top;
        double                  clip_bottom;

};
	

// Max images to be hold and max images to be placed on video surface.
// Can be set arbitrarily.
#define	MAX_IMAGES		32
#define	MAX_IMAGE_PLACES	2*MAX_IMAGES

#include "controller.h"
#include "cairo_graphic.h"

class CVideoImage {
  public:
				CVideoImage(CVideoMixer* pVideoMixer,
					u_int32_t max_images = MAX_IMAGES,
					u_int32_t max_images_placed = MAX_IMAGE_PLACES);
				~CVideoImage();

	int			ParseCommand(CController* pController,
					struct controller_type* ctr, const char* str);

	unsigned int		MaxImages();
	struct image_item_t*	GetImageItem(unsigned int i);

	int			RemoveImagePlaced(u_int32_t image_id);
	unsigned int		MaxImagesPlace();
	struct image_place_t*	GetImagePlaced(unsigned int i);
	void			SetImageAlign(u_int32_t place_id, int align);

	void			UpdateMove(u_int32_t place_id);

	int			OverlayImage(const u_int32_t place_id, u_int8_t* pY,
					const u_int32_t width, const u_int32_t height);
	
  private:
	int 			set_image_help(struct controller_type* ctr,
					CController* pController, const char* str);

	int			set_image_load (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_write (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_align(struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_alpha (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_rotation (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_scale (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_image (struct controller_type *ctr, CController* pController, const char* str);

	int 			set_image_place_move_scale (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_coor (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_alpha (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_rotation (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_move_clip (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_clip (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_offset (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_matrix (struct controller_type *ctr,
					CController* pController, const char* ci);


	void 			SetImageMoveAlpha(u_int32_t place_id, double delta_alpha,
					u_int32_t delta_alpha_steps);
	void 			SetImageMoveRotation(u_int32_t place_id, double delta_rotation,
					u_int32_t delta_rotation_steps);
	void 			SetImageMoveScale(u_int32_t place_id, double delta_scale_x,
					double delta_scale_y, u_int32_t delta_scale_x_steps,
					u_int32_t delta_scale_y_steps);
	void 			SetImageMoveCoor(u_int32_t place_id, int32_t delta_x,
					int32_t delta_y, u_int32_t delta_x_steps,
					u_int32_t delta_y_steps);

	int			WriteImage(u_int32_t image_id, const char* file_name);
	int			LoadImage(u_int32_t image_id, const char* file_name);

	void			SetImageAlpha(u_int32_t place_id, double alpha);

	void			SetImageOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y);

	void			SetImageRotation(u_int32_t place_id, double rotation);

	void			SetImageScale(u_int32_t place_id, double scale_x, double scale_y);

	int			SetImagePlace(u_int32_t place_id, u_int32_t image_id, int32_t x,
					int32_t y, bool display, bool coor, const char* anchor = "nw");

	int			SetImagePlaceImage(u_int32_t place_id, u_int32_t image_id);

	void			SetImageClip(u_int32_t place_id, double clip_left, double clip_right,
					double clip_top, double clip_bottom);

	void			SetImageMoveClip(u_int32_t place_id, double delta_clip_left,
					double delta_clip_right, double delta_clip_top,
					double delta_clip_bottom, u_int32_t step_left,
					u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom);

	void			SetAnchor(u_int32_t place_id, const char* anchor);

	int			SetImageMatrix(u_int32_t id, double xx, double xy, double yx, double yy,
					double x0, double y0);

	CVideoMixer*		m_pVideoMixer;
	image_place_t**		m_image_places;		// Array to hold pointer to placed images
	u_int32_t		m_max_image_places;	// Max number in array
	u_int32_t		m_max_images;
	struct image_item_t**	m_image_items;
	bool			m_verbose;

	//u_int32_t		m_max_images_placed;
	//struct image_place_t**	m_image_placed;
	u_int32_t		m_width;
	u_int32_t		m_height;
};
	
#endif	// VIDEO_IMAGE
