/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_SCALER_H__
#define __VIDEO_SCALER_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include <pango/pangocairo.h>

// Video types. Layout shown below
// YUY2   : Y0U0Y1V0 Y2U1Y3V1 Y4U2Y5V2 Y6U3Y7V3
// YUV420 : Y0Y1Y2Y3Y4Y5Y6Y7 U0U1 V0V1
typedef enum VideoType {
	VIDEO_ARGB,
	VIDEO_BGRA,
	VIDEO_YUY2,
	VIDEO_I420,
	VIDEO_NONE
} VideoType;


// // Get a pointer to a videomixer with a width x height of 1024x576
// CVideoScaler* pVideoScaler = new CVideoScaler(1024,576,VIDEO_YUY2);
//
// // Get a video mixer overlay and blackpad it.
// u_int8_t* pOverlay = pVideoScaler->NewFrame(true);
//
// // Assume pSrc0 points to a frame of 1024x576
// // Assume pSrc1 points to a frame of 704x576
// // Assume pSrc2 points to a frame of 704x576
// // Assume pSrc3 points to a frame of 704x576
//
// // Overlay with Src0 completely
// pVideoScaler->Overlay(pOverlay,pSrc0,		// Destination and source
//			0,0,			// Destination start col and row
//			1024,576,		// Source width and height
//			0,0,			// Source start col and row
//			1024,576);		// Source clip cols and rows
//
// // Overlay with Src0 clipped width to 704 and left justified
// pVideoScaler->Overlay(pOverlay,pSrc0,		// Destination and source
//			0,0,			// Destination start col and row
//			1024,576,		// Source width and height
//			160,0,			// Source start col and row
//			704,576);		// Source clip cols and rows
//
// // Overlay with Src1 scaled by 1/2 and clipped centered to width 320 and
// // placed in the rightmost top corner. Scaled height is 288
// pVideoScaler->Overlay2(pOverlay,pSrc1,	// Destination and source
//			704,0,			// Destination start col and row
//			704,576,		// Source width and height
//			32,0,			// Source start col and row
//			640,576);		// Source clip cols and rows
// // Overlay with Src2 scaled by 1/2 and clipped centered to width 320 and
// // placed in the rightmost bottom corner. Scaled height is 288
// pVideoScaler->Overlay2(pOverlay,pSrc1,	// Destination and source
//			704,288,		// Destination start col and row
//			704,576,		// Source width and height
//			32,0,			// Source start col and row
//			640,576);		// Source clip cols and rows
//
// // Overlay with Src0, Src1 and Src2 all scaled by 1/2 and clipped to 320x192
// // (640x384 before scaling) and placed rightmost beneath each other
// // as well as overlaying with src3 left justified
// pVideoScaler->Overlay(pOverlay,pSrc3,0,0,704,576,0,0,704,576);
// pVideoScaler->Overlay2(pOverlay,pSrc0,704,0,1024,576,192,96,640,384);
// pVideoScaler->Overlay2(pOverlay,pSrc1,704,192,704,576,32,96,640,384);
// pVideoScaler->Overlay2(pOverlay,pSrc2,704,384,704,576,32,96,640,384);
// 
// // Pad with 160 x 576 pixels of black on each side
// pVideoScaler->BlackPad(0, 0, 160, 576, pOverlay);
// pVideoScaler->BlackPad(704+160, 0, 160, 576, pOverlay);

// Class CVideomixer
// Initialized with width and height as well as video (YUV) type
// NewFrame() : Returns a memory area large enough to hold a single YUV frame
//	for the width and height and video type.
// Overlay() : Copy (and possibly clipping) a video source frame into a mixer
//	frame. Recommended to use even numbers for positions and clipping
// Overlay2() : As for Overlay(), but the source is scaled by 1/2 before
//	copying and possibly clipping. Clip width MUST be divisable by 4.

	
class CVideoScaler {
  public:
	CVideoScaler(u_int32_t width, u_int32_t height, VideoType video_type);
	~CVideoScaler();
	u_int8_t* NewFrame(bool blackpad=true);
	u_int8_t* PAR(u_int8_t* pY,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par);
	u_int8_t* PAR_5_4(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par);
	u_int8_t* PAR_12_11(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par);
	void Overlay(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay2(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay3(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay4(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay5(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay2_3(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void Overlay2_5(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t col, u_int32_t row,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t c_src, u_int32_t r_src,
		u_int32_t clip_c_src,
		u_int32_t clip_r_src);
	void BlackPad(u_int32_t col, u_int32_t row,
	      u_int32_t width, u_int32_t height,
	      u_int8_t* pY);
  private:
	u_int32_t	m_width;	// Overlay width
	u_int32_t	m_height;	// Overlay height
	u_int32_t	m_Ysize;	// Overlay Y size
	u_int32_t	m_Usize;	// Overlay U size
	//u_int8_t*	m_Yp		// Pointer to overlay Y
	//u_int8_t*	m_Up		// Pointer to overlay U
	//u_int8_t*	m_Vp		// Pointer to overlay V
	VideoType	m_video_type;	// Defines YUV layout
};

class CVideoSource {
  public:
	CVideoSource(u_int32_t width, u_int32_t height, VideoType video_type);
	~CVideoSource();
	void Place(u_int32_t col, u_int32_t row,
		u_int32_t start_col, u_int32_t start_row,
		u_int32_t clip_width, u_int32_t clip_height,
		u_int32_t scale);
	void Mix(CVideoScaler* mixer, u_int8_t* mixer_frame, u_int8_t* source_frame);
  private:
	
	VideoType	m_video_type;	// Defines YUV layout
	u_int32_t	m_width;	// Source width
	u_int32_t	m_height;	// Source height
	u_int32_t	m_col;		// Source col placement
	u_int32_t	m_row;		// Source row placement
	u_int32_t	m_start_col;	// Source start col
	u_int32_t	m_start_row;	// Source start row
	u_int32_t	m_clip_width;	// Source clip width
	u_int32_t	m_clip_height;	// Source clip width
	u_int32_t	m_scale;	// Clip scale
};

#endif	// VIDEO_SCALER
