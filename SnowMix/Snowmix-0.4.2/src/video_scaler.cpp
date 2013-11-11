/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowcup@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
#include <stdio.h>
//#include <limits.h>
#include <string.h>
#include <malloc.h>
//#include <sys/types.h>

#include "video_scaler.h"
#include "add_by.h"

CVideoSource::CVideoSource(u_int32_t width, u_int32_t height, VideoType video_type) {
	m_video_type = video_type;
	m_width = width;
	m_height = height;
	m_col = 0; m_row = 0;
	m_start_col = 0; m_start_row = 0;
	m_clip_width = width;
	m_clip_height = height;
	m_scale = 1;
}
CVideoSource::~CVideoSource() {
}
void CVideoSource::Place(u_int32_t col, u_int32_t row,
                u_int32_t start_col, u_int32_t start_row,
                u_int32_t clip_width, u_int32_t clip_height, u_int32_t scale) {
	m_col = col;
	m_row = row;
	m_start_col = start_col;
	m_start_row = start_row;
	m_clip_width = clip_width;
	m_clip_height = clip_height;
	if (scale == 1 || scale == 2) m_scale = scale;
	else m_scale = 1;
fprintf(stderr, "Scale = %u m_scale %u \n", scale, m_scale);

};
void CVideoSource::Mix(CVideoScaler* scaler, u_int8_t* scaler_frame, u_int8_t* source_frame) {
	if (scaler && scaler_frame && source_frame) {
		if (m_scale == 1) {
			scaler->Overlay(scaler_frame, source_frame,
				m_col, m_row,
				m_width, m_height,
				m_start_col, m_start_row,
				m_clip_width, m_clip_height);
		} else if (m_scale == 2) {
			scaler->Overlay2(scaler_frame, source_frame,
				m_col, m_row,
				m_width, m_height,
				m_start_col, m_start_row,
				m_clip_width, m_clip_height);
		}
	}
}

	

// ////////////////////////////////////////////////////////////////////////
//
// ~CVideoScaler()
//
CVideoScaler::CVideoScaler(u_int32_t width, u_int32_t height,
	VideoType video_type) {
	m_width = m_height = m_Ysize = m_Usize = 0;

	// Check video geometry
	if (width < 2 || height < 2) {
		fprintf(stderr, "Illegal video geometry %ux%u for VideoScaler\n",
			 width, height);
		m_video_type = VIDEO_NONE;
		return;
	}

	m_video_type = video_type;		// Save video type
	m_Ysize = width*height;
	m_width = width;
	m_height = height;
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {
		m_Usize = 0;			// RGB has no U_size. Framesize is 4 x m_Ysize;
	} else if (m_video_type == VIDEO_YUY2) {
		m_Usize = m_Ysize >> 1;		// Vsize == Usize
	} else if (m_video_type == VIDEO_I420) {
		m_Usize = m_Ysize >> 2;		// Vsize == Usize
	} else {
		fprintf(stderr, "Unknown video type for scaler\n");
		m_video_type = VIDEO_NONE;
		m_width = m_height = m_Ysize = m_Usize = 0;
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// ~CVideoScaler()
//
CVideoScaler::~CVideoScaler() {
}


// ////////////////////////////////////////////////////////////////////////
//
// NewFrame()
//
// Returns a fresh allocated memory area for a complete frame
// By default the frame is blackpadded (zero'ed)
// Returns NULL if no memory is available
//
u_int8_t* CVideoScaler::NewFrame(bool blackpad) {
	u_int8_t* pY = (u_int8_t*) malloc(m_Ysize + (m_Usize<<1));
	// fprintf(stderr, "Newframe Ysize %u + 2 * Usize %u = %u\n",
	// m_Ysize, m_Usize, m_Ysize+(m_Usize<<1));
	if (blackpad && pY) {
		if (m_video_type == VIDEO_YUY2) {
			for (u_int32_t i=0; i < m_Ysize; i++) {
				*pY++ = 127;
				*pY++ = 127;
			}
			pY -= (m_Ysize<<1);
		} else if (m_video_type == VIDEO_I420) {
			memset(pY, 0, m_Ysize);
			pY += m_Usize;
			memset(pY, 127, (m_Usize<<1));
		} else {
			fprintf(stderr,
				"Unknown video type for blackpad scaler\n");
		}
	}
	return pY;
}

// ////////////////////////////////////////////////////////////////////////
//
// PAR()
//
// Returns a fresh allocated memory area for a complete frame with the
// pointed to by pY_src is scaled/copied according to w_par and h_par
// Returns NULL if no memory is available
u_int8_t* CVideoScaler::PAR(u_int8_t* pY_src,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par) {
	u_int8_t *pY = NULL;
	u_int8_t *pY_tmp = NULL;
	if (!pY_src || !w_src || !h_src || !w_par || !h_par || m_video_type == VIDEO_NONE) return NULL;
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {
		u_int32_t width = w_src*w_par/h_par;
		if (!(pY = (u_int8_t*) malloc(width*h_src*4))) return NULL;
		pY_tmp = pY;
		u_int16_t divisor = h_par;
		for (u_int32_t n=0; n < h_src; n++) {
		u_int16_t x = w_par;
		u_int16_t y = 0;
		u_int8_t* pY_src_tmp = pY_src + (w_src<<2);
		for (u_int32_t i=0; i<width; i++) {
//fprintf(stderr, "PAR %u ",i);
			if (h_par <= x) {
//fprintf(stderr, "%u of %u\n",h_par, x);
				*pY++ = *(pY_src+0);
				*pY++ = *(pY_src+1);
				*pY++ = *(pY_src+2);
				*pY++ = *(pY_src+3);
				x -= h_par;
				if (x <= 0) {
					pY_src +=4;
					x = w_par;
				}
			} else {
				y = h_par-x;
//fprintf(stderr, "%u of %u + %u of %u div %u\n", x, x, y, w_par, x+y);
				*pY++ = Add2by1mul(pY_src,  0,4,  x,y, divisor);
				*pY++ = Add2by1mul(pY_src,  1,5,  x,y, divisor);
				*pY++ = Add2by1mul(pY_src,  2,6,  x,y, divisor);
				*pY++ = Add2by1mul(pY_src,  3,7,  x,y, divisor);
				//*pY++ = (((u_int16_t)(*(pY_src+0)))*x+((u_int16_t)(*(pY_src+4)))*y)/(divisor);
				//*pY++ = (((u_int16_t)(*(pY_src+1)))*x+((u_int16_t)(*(pY_src+5)))*y)/(divisor);
				//*pY++ = (((u_int16_t)(*(pY_src+2)))*x+((u_int16_t)(*(pY_src+6)))*y)/(divisor);
				//*pY++ = (((u_int16_t)(*(pY_src+3)))*x+((u_int16_t)(*(pY_src+7)))*y)/(divisor);
				pY_src += 4;
				x = w_par - y;
			}
		}
		pY_src = pY_src_tmp;
		}
	} else {
			fprintf(stderr,
				"Unsupported video type for PAR scaler\n");
	}
	return pY_tmp;
}

// ////////////////////////////////////////////////////////////////////////
//
// PAR_5_4()
//
// Returns a fresh allocated memory area for a complete frame with the
// pointed to by pY_src is scaled/copied according to w_par and h_par
// Returns NULL if no memory is available
u_int8_t* CVideoScaler::PAR_5_4(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par) {
	u_int8_t *pY_tmp = NULL;
	if (!pY_src || !w_src || !h_src || !w_par || !h_par || m_video_type == VIDEO_NONE) return NULL;
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {
		u_int32_t width = w_src*w_par/h_par;
		if (!pY) {
			if (!(pY = (u_int8_t*) malloc(width*h_src*4))) return NULL;
		}
		pY_tmp = pY;
		for (u_int32_t n=0; n < h_src; n++) {
			u_int8_t* pY_src_tmp = pY_src + (w_src<<2);
			for (u_int32_t i=0; i<width; i += 5) {

				// First
				*pY++ = *(pY_src+0);
				*pY++ = *(pY_src+1);
				*pY++ = *(pY_src+2);
				*pY++ = *(pY_src+3);
				// 2nd
				*pY++ = Add2by1mul(pY_src,  0,4,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  1,5,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  2,6,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  3,7,  1,10, 11);
				// 3rd
				*pY++ = Add2by1mul(pY_src,  4,8,  2,9,  11);
				*pY++ = Add2by1mul(pY_src,  5,9,  2,9,  11);
				*pY++ = Add2by1mul(pY_src,  6,10, 2,9,  11);
				*pY++ = Add2by1mul(pY_src,  7,11, 2,9,  11);
				// 4th
				*pY++ = Add2by1mul(pY_src,  8,12, 3,8,  11);
				*pY++ = Add2by1mul(pY_src,  9,13, 3,8,  11);
				*pY++ = Add2by1mul(pY_src, 10,14, 3,8,  11);
				*pY++ = Add2by1mul(pY_src, 11,15, 3,8,  11);
				// Last
				*pY++ = *(pY_src+12);
				*pY++ = *(pY_src+13);
				*pY++ = *(pY_src+14);
				*pY++ = *(pY_src+15);
				pY_src += 16;
			}
			pY_src = pY_src_tmp;
		}
	} else {
			fprintf(stderr,
				"Unsupported video type for PAR scaler\n");
	}
	return pY_tmp;
}

// ////////////////////////////////////////////////////////////////////////
//
// PAR_12_11()
//
// Returns a fresh allocated memory area for a complete frame with the
// pointed to by pY_src is scaled/copied according to w_par and h_par
// Returns NULL if no memory is available
u_int8_t* CVideoScaler::PAR_12_11(u_int8_t* pY, u_int8_t* pY_src,
		u_int32_t w_src, u_int32_t h_src,
		u_int32_t w_par, u_int32_t h_par) {
	u_int8_t *pY_tmp = NULL;
	if (!pY_src || !w_src || !h_src || !w_par || !h_par || m_video_type == VIDEO_NONE) return NULL;
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {
		u_int32_t width = w_src*w_par/h_par;
		if (!pY) {
			if (!(pY = (u_int8_t*) malloc(width*h_src*4))) return NULL;
		}
		pY_tmp = pY;
		for (u_int32_t n=0; n < h_src; n++) {
			u_int8_t* pY_src_tmp = pY_src + (w_src<<2);
			for (u_int32_t i=0; i<width; i += 12) {

				// First
				*pY++ = *(pY_src+0);
				*pY++ = *(pY_src+1);
				*pY++ = *(pY_src+2);
				*pY++ = *(pY_src+3);
				// 2nd
				*pY++ = Add2by1mul(pY_src,  0,4,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  1,5,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  2,6,  1,10, 11);
				*pY++ = Add2by1mul(pY_src,  3,7,  1,10, 11);
				// 3rd
				*pY++ = Add2by1mul(pY_src,  4,8,  2,9,  11);
				*pY++ = Add2by1mul(pY_src,  5,9,  2,9,  11);
				*pY++ = Add2by1mul(pY_src,  6,10, 2,9,  11);
				*pY++ = Add2by1mul(pY_src,  7,11, 2,9,  11);
				// 4th
				*pY++ = Add2by1mul(pY_src,  8,12, 3,8,  11);
				*pY++ = Add2by1mul(pY_src,  9,13, 3,8,  11);
				*pY++ = Add2by1mul(pY_src, 10,14, 3,8,  11);
				*pY++ = Add2by1mul(pY_src, 11,15, 3,8,  11);
				// 5th
				*pY++ = Add2by1mul(pY_src, 12,16, 4,7,  11);
				*pY++ = Add2by1mul(pY_src, 13,17, 4,7,  11);
				*pY++ = Add2by1mul(pY_src, 14,18, 4,7,  11);
				*pY++ = Add2by1mul(pY_src, 15,19, 4,7,  11);
				// 6th
				*pY++ = Add2by1mul(pY_src, 16,20, 5,6,  11);
				*pY++ = Add2by1mul(pY_src, 17,21, 5,6,  11);
				*pY++ = Add2by1mul(pY_src, 18,22, 5,6,  11);
				*pY++ = Add2by1mul(pY_src, 19,23, 5,6,  11);
				// 7th
				*pY++ = Add2by1mul(pY_src, 20,24, 6,5,  11);
				*pY++ = Add2by1mul(pY_src, 21,25, 6,5,  11);
				*pY++ = Add2by1mul(pY_src, 22,26, 6,5,  11);
				*pY++ = Add2by1mul(pY_src, 23,27, 6,5,  11);
				// 8th
				*pY++ = Add2by1mul(pY_src, 24,28, 7,4,  11);
				*pY++ = Add2by1mul(pY_src, 25,29, 7,4,  11);
				*pY++ = Add2by1mul(pY_src, 26,30, 7,4,  11);
				*pY++ = Add2by1mul(pY_src, 27,31, 7,4,  11);
				// 9th
				*pY++ = Add2by1mul(pY_src, 28,32, 8,3,  11);
				*pY++ = Add2by1mul(pY_src, 29,33, 8,3,  11);
				*pY++ = Add2by1mul(pY_src, 30,34, 8,3,  11);
				*pY++ = Add2by1mul(pY_src, 31,35, 8,3,  11);
				// 10th
				*pY++ = Add2by1mul(pY_src, 32,36, 9,2,  11);
				*pY++ = Add2by1mul(pY_src, 33,37, 9,2,  11);
				*pY++ = Add2by1mul(pY_src, 34,38, 9,2,  11);
				*pY++ = Add2by1mul(pY_src, 35,39, 9,2,  11);
				// 11th
				*pY++ = Add2by1mul(pY_src, 36,40, 10,1,  11);
				*pY++ = Add2by1mul(pY_src, 37,41, 10,1,  11);
				*pY++ = Add2by1mul(pY_src, 38,42, 10,1,  11);
				*pY++ = Add2by1mul(pY_src, 39,43, 10,1,  11);
				// Last
				*pY++ = *(pY_src+40);
				*pY++ = *(pY_src+41);
				*pY++ = *(pY_src+42);
				*pY++ = *(pY_src+43);
				pY_src += 44;
			}
			pY_src = pY_src_tmp;
		}
	} else {
			fprintf(stderr,
				"Unsupported video type for PAR scaler\n");
	}
	return pY_tmp;
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay()
//
// Copy the frame pointed by pY_src having the width w_src and height h_src
// to the frame pointed by pY. Position the copied framein pY at column col
// and row row. Clip the copied source from column c_src and row r_src and
// clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for rows, columns, widths and heights
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay\n");
		return;
	}
	if ((m_video_type == VIDEO_ARGB) || (m_video_type == VIDEO_BGRA)) {
		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		// Copy each clipped line from pY_src to pY
		// and advance Y pointer 2*width of frame
		for (u_int32_t i=0; i < clip_r_src; i++) {
			memcpy(pY, pY_src, clip_c_src<<2);
			pY += (m_width << 2);
			pY_src += (w_src << 2);
		}
	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Copy each clipped line from pY_src to pY
		// and advance Y pointer 2*width of frame
		for (u_int32_t i=0; i < clip_r_src; i++) {
			// copy a clipped line from src to pY
			memcpy(pY, pY_src, clip_c_src<<1);
			pY += (m_width << 1);
			pY_src += (w_src << 1);
		}
	} else if (m_video_type == VIDEO_I420) {
		// Advance pY, pU and pV to relevant start pixel
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		u_int8_t* pV = pU + m_Usize;
		pY += (row*m_width + col);

		// Advance pY_src, pU_src and pV_src to relevant start pixel
		u_int8_t* pU_src = pY_src + w_src*h_src +
			((r_src>>1) * (w_src>>1) + (c_src>>1));
		u_int8_t* pV_src = pY_src + ((w_src*h_src)>>1);
		pY_src += (r_src*w_src + c_src);

		// Copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {
			// pY_src copy a clipped line
			memcpy(pY, pY_src, clip_c_src);
			pY += m_width;
			pY_src += w_src;

			// pY_src copy a clipped line
			memcpy(pY, pY_src, clip_c_src);
			pY += m_width;
			pY_src += w_src;

			// pU_src copy a clipped line
			memcpy(pU, pU_src, (clip_c_src>>1));
			// pV_src copy a clipped line
			memcpy(pV, pV_src, (clip_c_src>>1));
			pU += (m_width>>1);
			pV += (m_width>>1);
			pU_src += (w_src >>1);
			pV_src += (w_src >>1);
		}
	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay2()
//
// Copy the frame scaled by 1/2 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for rows, columns, widths and heights.
//	     Clip width should be divisable by 4.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay2(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay2\n");
		return;
	}
	if ((m_video_type == VIDEO_ARGB) || (m_video_type == VIDEO_BGRA)) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<3);
			for (u_int32_t n=0; n < (clip_c_src>>1); n++) {
				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				*pY++ = Add2by2(pY_src, Y_off1, 0,4, 0,0,0,0, 4);
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 0,0,0,0, 4);
				*pY++ = Add2by2(pY_src, Y_off1, 2,6, 0,0,0,0, 4);
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 0,0,0,0, 4);
				pY_src += 8;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
		
	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Loop for half the lines to copy as we scale and copy two
		// lines into one.
		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<1);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<2);

			// Scale and copy 4 pixels into one, two pixels on
			// two lines
			for (u_int32_t n=0; n < (clip_c_src>>2); n++) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);

				// Y0 = YS0+YS1+YS8+YS9 (width = 8 pixels)
				*pY++ = Add2by2(pY_src, Y_off1, 0,2, 0,0,0,0, 4);

				// U0 = US0+US1+US4+US5 (width = 8 pixels)
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 0,0,0,0, 4);

				// Y1 = YS2+YS3+YS10+YS12 (width = 8 pixels)
				*pY++ = Add2by2(pY_src, Y_off1, 4,6, 0,0,0,0, 4);

				// V0 = VS0+VS1+VS4+VS5 (width = 8 pixels)
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 0,0,0,0, 4);

				pY_src += 8;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
	} else if (m_video_type == VIDEO_I420) {
		// Advance pY, pU and pV to relevant start pixel
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		u_int8_t* pV = pU + m_Usize;
		pY += (row*m_width + col);

		// Advance pY_src, pU_src and pV_src to relevant start pixel
		u_int8_t* pU_src = pY_src + w_src*h_src +
			((r_src>>1) * (w_src>>1) + (c_src>>1));
		u_int8_t* pV_src = pY_src + ((w_src*h_src)>>1);
		pY_src += (r_src*w_src + c_src);

		// For pY_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + m_width;
			u_int8_t* pY_src_tmp = pY_src + w_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < (clip_c_src>>1); n++) {
				// Y0 = YS0+YS1+YS8+YS9 (width = 8 pixels)
				*pY++ = ((u_int16_t)(*pY_src) +
					(u_int16_t)(*(pY_src+1)) +
					(u_int16_t)(*(pY_src+w_src)) +
					(u_int16_t)(*(pY_src+1+w_src))) >> 2;
				pY_src++; pY_src++;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
		// For pU_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>2); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pU_tmp = pU + (m_width>>1);
			u_int8_t* pV_tmp = pV + (m_width>>1);
			u_int8_t* pU_src_tmp = pU_src + w_src;
			u_int8_t* pV_src_tmp = pV_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < (clip_c_src>>2); n++) {
				// U0 = US0+US1+US4+US5 (width = 8 pixels)
				*pU++ = ((u_int16_t)(*pU_src) +
				  (u_int16_t)(*(pU_src+1)) +
				  (u_int16_t)(*(pU_src+(w_src>>1))) +
				  (u_int16_t)(*(pU_src+1+(w_src>>1)))) >> 2;
				pU_src++; pU_src++;
				// V0 = VS0+VS1+VS4+VS5 (width = 8 pixels)
				*pV++ = ((u_int16_t)(*pV_src) +
				  (u_int16_t)(*(pV_src+1)) +
				  (u_int16_t)(*(pV_src+(w_src>>1))) +
				  (u_int16_t)(*(pV_src+1+(w_src>>1)))) >> 2;
				pV_src++; pV_src++;
			}
			// Set pU and pV to next new line and column on the
			// overlay
			pU = pU_tmp;
			pU_src = pU_src_tmp;
			pV = pV_tmp;
			pV_src = pV_src_tmp;
		}
	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay3()
//
// Copy the frame scaled by 1/3 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for rows, columns, widths and heights.
//	     Clip width should be divisable by 3.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay3(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay3\n");
		return;
	}
	if ((m_video_type == VIDEO_ARGB) || (m_video_type == VIDEO_BGRA)) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		// Loop for 1/3 of the rows as we add 3 rows at a time
		for (u_int32_t i=0; i < clip_r_src; i += 3) {
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + ((3*w_src)<<2);

			// Loop for 1/3 of the cols as we add 3 cols at a time
			for (u_int32_t n=0; n < clip_c_src; n += 3) {
				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<2);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,4,8, 0,0,0,0,0,0,0,0,0, 9);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9, 0,0,0,0,0,0,0,0,0, 9);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						2,6,10, 0,0,0,0,0,0,0,0,0, 9);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 0,0,0,0,0,0,0,0,0, 9);
				pY_src += 12;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}

	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Loop for 1/3 the lines to copy as we scale and copy 3
		// lines into one.
		for (u_int32_t i=0; i < clip_r_src; i += 3) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<1);
			u_int8_t* pY_src_tmp = pY_src + 6*w_src;

			// Scale and copy 9 pixels into one, 3 pixels on
			// 3 lines
			for (u_int32_t n=0; n < clip_c_src; n += 6) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<1);

				// Y0 = YS0+YS1+Y2+YS6+YS7+YS8+YS12+YS13+YS14
				// (width = 9 pixels)
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,2,4, 0,0,0,0,0,0,0,0,0, 9);

				// U0 = US0+US1+US2+US3+US4+US5+US6+US7+US8
				// (width = 9 pixels)
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9, 0,0,0,0,0,0,0,0,0, 9);

				// Y0 = YS3+YS4+Y5+YS9+YS10+YS11+YS15+YS16+YS17
				// (width = 9 pixels)
				// Y1 = YS2+YS3+YS10+YS12 (width = 8 pixels)
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						6,8,10, 0,0,0,0,0,0,0,0,0, 9);

				// V0 = VS0+VS1+VS2+VS3+VS4+VS5+VS6+VS7+VS8
				// (width = 9 pixels)
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 0,0,0,0,0,0,0,0,0, 9);

				pY_src += 12;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
	} else if (m_video_type == VIDEO_I420) {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay3\n");
		return;

		// The code below is from Overlay2. Needs rewriting.
		// Advance pY, pU and pV to relevant start pixel
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		u_int8_t* pV = pU + m_Usize;
		pY += (row*m_width + col);

		// Advance pY_src, pU_src and pV_src to relevant start pixel
		u_int8_t* pU_src = pY_src + w_src*h_src +
			((r_src>>1) * (w_src>>1) + (c_src>>1));
		u_int8_t* pV_src = pY_src + ((w_src*h_src)>>1);
		pY_src += (r_src*w_src + c_src);

		// For pY_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + m_width;
			u_int8_t* pY_src_tmp = pY_src + w_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < (clip_c_src>>1); n++) {
				// Y0 = YS0+YS1+YS8+YS9 (width = 8 pixels)
				*pY++ = ((u_int16_t)(*pY_src) +
					(u_int16_t)(*(pY_src+1)) +
					(u_int16_t)(*(pY_src+w_src)) +
					(u_int16_t)(*(pY_src+1+w_src))) >> 2;
				pY_src++; pY_src++;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
		// For pU_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>2); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pU_tmp = pU + (m_width>>1);
			u_int8_t* pV_tmp = pV + (m_width>>1);
			u_int8_t* pU_src_tmp = pU_src + w_src;
			u_int8_t* pV_src_tmp = pV_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < (clip_c_src>>2); n++) {
				// U0 = US0+US1+US4+US5 (width = 8 pixels)
				*pU++ = ((u_int16_t)(*pU_src) +
				  (u_int16_t)(*(pU_src+1)) +
				  (u_int16_t)(*(pU_src+(w_src>>1))) +
				  (u_int16_t)(*(pU_src+1+(w_src>>1)))) >> 2;
				pU_src++; pU_src++;
				// V0 = VS0+VS1+VS4+VS5 (width = 8 pixels)
				*pV++ = ((u_int16_t)(*pV_src) +
				  (u_int16_t)(*(pV_src+1)) +
				  (u_int16_t)(*(pV_src+(w_src>>1))) +
				  (u_int16_t)(*(pV_src+1+(w_src>>1)))) >> 2;
				pV_src++; pV_src++;
			}
			// Set pU and pV to next new line and column on the
			// overlay
			pU = pU_tmp;
			pU_src = pU_src_tmp;
			pV = pV_tmp;
			pV_src = pV_src_tmp;
		}
	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay3\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay4()
//
// Copy the frame scaled by 1/4 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for rows, columns, widths and heights.
//	     Clip width should be divisable by 4.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay4(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay4\n");
		return;
	}
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		for (u_int32_t i=0; i < (clip_r_src>>2); i++) {
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<4);
			for (u_int32_t n=0; n < (clip_c_src>>2); n++) {
				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<2);
				u_int8_t* Y_off3 = Y_off2 + (w_src<<2);
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3,
						0,4,8,12, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3,
						1,5,9,13, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3,
						2,6,10,14, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3,
						3,7,11,15, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);
				pY_src += 16;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}

	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Loop for half the lines to copy as we scale and copy two
		// lines into one.
		for (u_int32_t i=0; i < (clip_r_src>>2); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<1);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<3);

			// Scale and copy 2x16 pixels into one, four pixels on
			// four lines
			for (u_int32_t n=0; n < (clip_c_src>>3); n++) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<1);
				u_int8_t* Y_off3 = Y_off2 + (w_src<<1);

				// Y0 = YS0+YS1+YS2+YS3+YS8+YS9+YS10+YS11+
				//      YS16+YS17+YS18+YS19+YS24+YS25+YS26+YS27
				//      / 16;
				//(width = 8 pixels)
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3, 0,2,4,6,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);

				// U0 = US0+US1+US2+US3 + ..
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3, 1,5,9,13,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);

				// Y1 = YS4+YS5+YS6+YS7+YS12+YS13 ..
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3, 8,10,12,14,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);

				// V0 = VS0+VS1+VS2+VS3 + ..
				*pY++ = Add4by4(pY_src, Y_off1, Y_off2, Y_off3, 3,7,11,15,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 16);

				pY_src += 16;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
	} else if (m_video_type == VIDEO_I420) {
		// Advance pY, pU and pV to relevant start pixel
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		u_int8_t* pV = pU + m_Usize;
		pY += (row*m_width + col);

		// Advance pY_src, pU_src and pV_src to relevant start pixel
		u_int8_t* pU_src = pY_src + w_src*h_src +
			((r_src>>1) * (w_src>>1) + (c_src>>1));
		u_int8_t* pV_src = pY_src + ((w_src*h_src)>>1);
		pY_src += (r_src*w_src + c_src);

		// For pY_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < (clip_r_src>>1); i++) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + m_width;
			u_int8_t* pY_src_tmp = pY_src + w_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < (clip_c_src>>1); n++) {
				// Y0 = YS0+YS1+YS8+YS9 (width = 8 pixels)
				*pY++ = ((u_int16_t)(*pY_src) +
					(u_int16_t)(*(pY_src+1)) +
					(u_int16_t)(*(pY_src+w_src)) +
					(u_int16_t)(*(pY_src+1+w_src))) >> 2;
				pY_src++; pY_src++;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
		// For pU_src copy 2 clipped line in each iteration
		for (u_int32_t i=0; i < clip_r_src; i +=4) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pU_tmp = pU + (m_width>>1);
			u_int8_t* pV_tmp = pV + (m_width>>1);
			u_int8_t* pU_src_tmp = pU_src + w_src;
			u_int8_t* pV_src_tmp = pV_src + w_src;

			// Scale 4 pixels (2 pixels in 2 rows) into one
			// pixel
			for (u_int32_t n=0; n < clip_c_src; n += 4) {
				// U0 = US0+US1+US4+US5 (width = 8 pixels)
				*pU++ = ((u_int16_t)(*pU_src) +
				  (u_int16_t)(*(pU_src+1)) +
				  (u_int16_t)(*(pU_src+(w_src>>1))) +
				  (u_int16_t)(*(pU_src+1+(w_src>>1)))) >> 2;
				pU_src++; pU_src++;
				// V0 = VS0+VS1+VS4+VS5 (width = 8 pixels)
				*pV++ = ((u_int16_t)(*pV_src) +
				  (u_int16_t)(*(pV_src+1)) +
				  (u_int16_t)(*(pV_src+(w_src>>1))) +
				  (u_int16_t)(*(pV_src+1+(w_src>>1)))) >> 2;
				pV_src++; pV_src++;
			}
			// Set pU and pV to next new line and column on the
			// overlay
			pU = pU_tmp;
			pU_src = pU_src_tmp;
			pV = pV_tmp;
			pV_src = pV_src_tmp;
		}
	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay4\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay5()
//
// Copy the frame scaled by 1/5 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for rows, columns, widths and heights.
//	     Clip width should be divisable by 4.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay5(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay4\n");
		return;
	}
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		for (u_int32_t i=0; i < clip_r_src; i += 5) {
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<4);
			for (u_int32_t n=0; n < clip_c_src; n += 5) {
				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<2);
				u_int8_t* Y_off3 = Y_off2 + (w_src<<2);
				u_int8_t* Y_off4 = Y_off3 + (w_src<<2);
				*pY++ = Add5by5(pY_src, Y_off1, Y_off2, Y_off3, Y_off4,
						0,4,8,12,16, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 25);
				*pY++ = Add5by5(pY_src, Y_off1, Y_off2, Y_off3, Y_off4,
						1,5,9,13,17, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 25);
				*pY++ = Add5by5(pY_src, Y_off1, Y_off2, Y_off3, Y_off4,
						2,6,10,14,18, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 25);
				*pY++ = Add5by5(pY_src, Y_off1, Y_off2, Y_off3, Y_off4,
						3,7,11,15,19, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 25);
				pY_src += 20;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}

	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		fprintf(stderr, "Unsupported video type VideoScaler::Overlay5\n");

	} else if (m_video_type == VIDEO_I420) {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay5\n");
/*
		// Advance pY, pU and pV to relevant start pixel
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		u_int8_t* pV = pU + m_Usize;
		pY += (row*m_width + col);

		// Advance pY_src, pU_src and pV_src to relevant start pixel
		u_int8_t* pU_src = pY_src + w_src*h_src +
			((r_src>>1) * (w_src>>1) + (c_src>>1));
		u_int8_t* pV_src = pY_src + ((w_src*h_src)>>1);
		pY_src += (r_src*w_src + c_src);
*/


	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay5\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay2_3()
//
// Copy the frame scaled by 2/3 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for columns, widths and heights.
//	     Clip width should be divisable by 6.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay2_3(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay2_3\n");
		return;
	}
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		// Loop for 1/3 the lines to copy as we scale and copy 3
		// lines into 2.
		for (u_int32_t i=0; i < clip_r_src; i+=3) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<2);

			// First we generate a line from 1+1/2 line 3 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=3) {

				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				*pY++ = Add2by2(pY_src, Y_off1, 0,4, 2,1,1,0, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 2,1,1,0, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 2,6, 2,1,1,0, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 2,1,1,0, 9);
				pY_src += 4;
				*pY++ = Add2by2(pY_src, Y_off1, 0,4, 1,2,0,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 1,2,0,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 2,6, 1,2,0,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 1,2,0,1, 9);
				pY_src += 8;
			}
			pY = pY_tmp;			// pY set to point at next line
			pY_tmp = pY + (m_width<<2);	// tmp set to point at the next line there after
			pY_src = pY_src_tmp;		// pY_src to point at the next line
			pY_src_tmp = pY_src + (w_src<<3); // and tmp to point two lines further ahead
	
			// The we generate a line from 1/2+1 line 3 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=3) {

				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				*pY++ = Add2by2(pY_src, Y_off1, 0,4, 1,0,2,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 1,0,2,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 2,6, 1,0,2,1, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 1,0,2,1, 9);
				pY_src += 4;
				*pY++ = Add2by2(pY_src, Y_off1, 0,4, 0,1,1,2, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 0,1,1,2, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 2,6, 0,1,1,2, 9);
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 0,1,1,2, 9);
				pY_src += 8;
			}
			pY = pY_tmp;			// pY set to point at next line
			pY_src = pY_src_tmp;		// pY_src to point at the next line two lines ahead
		}
	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Loop for 1/3 the lines to copy as we scale and copy 3
		// lines into 2.
		for (u_int32_t i=0; i < clip_r_src; i+=3) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<1);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<1);

			// Scale and copy 18 pixels on 3 lines to 8 pixels
			// on 2 lines
			// First we generate a line from 1+1/2 line 6 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=6) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);

	
				// Y0 = (4YS0+2YS1+2YS12+YS13)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 0,2, 2,1,1,0, 9);

				// U0 = (4US0+2US1+2US6+US7)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 2,1,1,0, 9);

				// Y1 = (2YS1+4YS2+YS13+2YS14)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 2,4, 1,2,0,1, 9);

				// V0 = (4VS0+2VS1+2VS6+VS7)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 2,1,1,0, 9);

				// Y2 = (4YS3+2YS4+2YS15+YS16)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 6,8, 2,1,1,0, 9);

				// U1 = (2US1+4US2+US7+2US8)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 5,9, 1,2,0,1, 9);

				// Y3 = (2YS4+4YS5+2YS16+YS17)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 8,10, 1,2,1,0, 9);

				// V1 = (2VS1+4VS2+VS7+2VS8)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 7,11, 1,2,0,1, 9);

				pY_src += 12;
			}

			// Jump to next destination line
			pY = pY_tmp;
			pY_tmp = pY + (m_width<<1);

			// Set pY_src to offset in second line
			pY_src = pY_src_tmp;

			// and save the position two lines further ahead
			pY_src_tmp = pY_src + (w_src<<2);

			// Then we generate teh second line from 1/2+1 line 6 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=6) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);

				// Y0 = (2YS0+YS1+4YS12+2YS13)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 0,2, 1,0,2,1, 9);

				// U0 = (2US0+US1+4US6+2US7)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 1,5, 1,0,2,1, 9);

				// Y1 = (YS1+2YS2+2YS13+4YS14)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 2,4, 0,1,1,2, 9);

				// V0 = (2VS0+VS1+4VS6+2VS7)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 3,7, 1,0,2,1, 9);

				// Y2 = (2YS3+YS4+4YS15+2YS16)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 6,8, 1,0,2,1, 9);

				// U1 = (US1+2US2+2US7+4US8)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 5,9, 0,1,1,2, 9);

				// Y3 = (YS4+2YS5+4YS16+2YS17)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 8,10, 0,1,2,1, 9);

				// V1 = (VS1+2VS2+2VS7+4VS8)/9
				// width : 12 pixels-> 8 pixels
				*pY++ = Add2by2(pY_src, Y_off1, 7,11, 0,1,1,2, 9);

				pY_src += 12;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
	} else if (m_video_type == VIDEO_I420) {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay\n");
		return;
	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay2_3\n");
	}
}

// ////////////////////////////////////////////////////////////////////////
//
// Overlay2_5()
//
// Copy the frame scaled by 2/5 pointed by pY_src having the width w_src and
// height h_src to the frame pointed by pY. Position the copied framein pY at
// column col and row row. Clip the copied source from column c_src and row
// r_src and clip for clip_c_src columns and clip_r_src rows
//
// Warning : Use even numbers for columns, widths and heights.
//	     Clip width should be divisable by 6.
//           Boundaries beyond both scalerframe and clip frame are not checked
//
void CVideoScaler::Overlay2_5(u_int8_t* pY, u_int8_t* pY_src,
	u_int32_t col, u_int32_t row,		// Destination col, row
	u_int32_t w_src, u_int32_t h_src,	// Source width, height
	u_int32_t c_src, u_int32_t r_src,	// Source start col, row
	u_int32_t clip_c_src,			// Clip columns
	u_int32_t clip_r_src) {			// Clip rows
	if (!pY || !pY_src) {
		fprintf(stderr, "Frame was NULL for VideoScaler::Overlay2_5\n");
		return;
	}
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {

		// Advance pY and pY_src to relevant start pixels
		// We use pY though we actually have RGB
		pY += ((row*m_width + col)<<2);
		pY_src += ((r_src*w_src + c_src)<<2);

		// Loop for 1/5 the lines to copy as we scale and copy two
		// lines into 2.
		for (u_int32_t i=0; i < clip_r_src; i+=5) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<2);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<3);

			// First we merge 2.5 lines into one
			for (u_int32_t n=0; n < (clip_c_src); n+=5) {

				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<2);

				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,4,8,  2,2,1,2,2,1,1,1,0, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9,  2,2,1,2,2,1,1,1,0, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						2,6,10, 2,2,1,2,2,1,1,1,0, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 2,2,1,2,2,1,1,1,0, 25);

				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						8,12,16,  1,2,2,1,2,2,0,1,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						9,13,17,  1,2,2,1,2,2,0,1,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						10,14,18, 1,2,2,1,2,2,0,1,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						11,15,19, 1,2,2,1,2,2,0,1,1, 25);
				pY_src += 20;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
			pY_tmp = pY + (m_width<<2);
			pY_src_tmp = pY_src + (w_src<<3) + (w_src<<2);

			// then we merge the next 2.5 lines into one
			for (u_int32_t n=0; n < (clip_c_src); n+=5) {

				u_int8_t* Y_off1 = pY_src + (w_src<<2);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<2);

				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,4,8,  1,1,0,2,2,1,2,2,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9,  1,1,0,2,2,1,2,2,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						2,6,10, 1,1,0,2,2,1,2,2,1, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 1,1,0,2,2,1,2,2,1, 25);

				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						8,12,16,  0,1,1,1,2,2,1,2,2, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						9,13,17,  0,1,1,1,2,2,1,2,2, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						10,14,18, 0,1,1,1,2,2,1,2,2, 25);
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						11,15,19, 0,1,1,1,2,2,1,2,2, 25);
				pY_src += 20;
			}
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}


	} else if (m_video_type == VIDEO_YUY2) {

		// Advance pY and pY_src to relevant start pixel
		pY += 2*(row*m_width + col);
		pY_src += 2*(r_src*w_src + c_src);

		// Loop for 1/5 the lines to copy as we scale and copy two
		// lines into 2.
		for (u_int32_t i=0; i < clip_r_src; i+=5) {

			// Save the next starting position on the overlay
			// and source
			u_int8_t* pY_tmp = pY + (m_width<<1);
			u_int8_t* pY_src_tmp = pY_src + (w_src<<2);

			// Scale and copy 50 pixels on 5 lines to 8 pixels
			// on 2 lines
			// First we generate a line from 1+1/2 line 6 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=10) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<1);



				// Y0 = (4YS0+4YS1+2YS2+4YS10+4YS11+2YS12+
				//       2YS20+2YS21+YS22)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,2,4, 2,2,1,2,2,1,1,1,0, 25);

				// U0 = (4US0+4US1+2US2+4US5+4US6+2US7+2US10+2US11+US12)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9, 2,2,1,2,2,1,1,1,0, 25);

				// Y1 = (2YS2+4YS3+4YS4+2YS12+4YS13+4YS14+
				//       YS22+2YS23+2YS24)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						4,6,8, 1,2,2,1,2,2,0,1,1, 25);

				// V0 = (4VS0+4VS1+2VS2+4VS5+4VS6+2VS7+2VS10+2VS11+VS12)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 2,2,1,2,2,1,1,1,0, 25);

				// Y2 = (4YS5+4YS6+2YS7+4YS15+4YS16+2YS17+
				//       2YS25+2YS26+YS27)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						10,12,14, 2,2,1,2,2,1,1,1,0, 25);

				// U1 = (2US2+4US3+4US4+2US7+4US8+4US9+US12+2US13+2US14)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						9,13,17, 1,2,2,1,2,2,0,1,1, 25);

				// Y3 = (2YS7+4YS8+4YS9+2YS17+4YS18+4YS19+
				//       YS27+2YS28+2YS29)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						14,16,18, 1,2,2,1,2,2,0,1,1, 25);

				// V1 = (2VS2+4VS3+4VS4+2VS7+4VS8+4VS9+VS12+2VS13+2VS14)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						11,15,19, 1,2,2,1,2,2,0,1,1, 25);

				pY_src += 20;
			}

			// Jump to next destination line
			pY = pY_tmp;
			pY_tmp = pY + (m_width<<1);

			// Set pY_src to offset in third line
			pY_src = pY_src_tmp;

			// and save the position 3 lines further ahead
			pY_src_tmp = pY_src + (w_src<<2) + (w_src<<1);

			// Then we generate teh second line from 1/2+1 line 6 pixels
			// per line at a time
			for (u_int32_t n=0; n < (clip_c_src); n+=10) {

				u_int8_t* Y_off1 = pY_src + (w_src<<1);
				u_int8_t* Y_off2 = Y_off1 + (w_src<<1);

				// Y0 = (2YS0+2YS1+YS2+4YS10+4YS11+2YS12+
				//       4YS20+4YS21+2YS22)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						0,2,4, 1,1,0,2,2,1,2,2,1, 25);

				// U0 = (2US0+2US1+US2+4US5+4US6+2US7+4US10+4US11+2US12)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						1,5,9, 1,1,0,2,2,1,2,2,1, 25);

				// Y1 = (YS2+2YS3+2YS4+2YS12+4YS13+4YS14+
				//       2YS22+4YS23+4YS24)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						4,6,8, 0,1,1,1,2,2,1,2,2, 25);

				// V0 = (2VS0+2VS1+VS2+4VS5+4VS6+2VS7+4VS10+4VS11+2VS12)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						3,7,11, 1,1,0,2,2,1,2,2,1, 25);

				// Y2 = (2YS5+2YS6+YS7+4YS15+4YS16+2YS17+
				//       4YS25+4YS26+2YS27)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						10,12,14, 1,1,0,2,2,1,2,2,1, 25);

				// U1 = (US2+2US3+2US4+2US7+4US8+4US9+2US12+4US13+4US14)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						9,13,17, 0,1,1,1,2,2,1,2,2, 25);

				// Y3 = (YS7+2YS8+2YS9+2YS17+4YS18+4YS19+
				//       2YS27+4YS28+4YS29)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						14,16,18, 0,1,1,1,2,2,1,2,2, 25);

				// V1 = (VS2+2VS3+2VS4+2VS7+4VS8+4VS9+2VS12+4VS13+4VS14)/25
				// width : 10 pixels-> 4 pixels
				*pY++ = Add3by3(pY_src, Y_off1, Y_off2,
						11, 15, 19, 0,1,1,1,2,2,1,2,2, 25);

				pY_src += 20;
			}
			// Set pY to next new line and column on the overlay
			pY = pY_tmp;
			pY_src = pY_src_tmp;
		}
	} else if (m_video_type == VIDEO_I420) {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay\n");
		return;

	} else {
		fprintf(stderr, "Unsupported video type VideoScaler::Overlay2_5\n");
	}
}

// BlackPad()
// Blackpad (fill with zero) a block with the width x height
// starting at col,row.
// col, row, width and height must be even and positive.
// No upper check of col, row, width and height
void CVideoScaler::BlackPad(u_int32_t col, u_int32_t row,
              u_int32_t width, u_int32_t height,
              u_int8_t* pY) {
	if (col < 0 || row < 0 || width < 0 || height < 0 ||
		col+width > m_width || row+height > m_height ||
		col & 0x01 || row & 0x01 || width & 0x01 || height & 0x01) {
		fprintf(stderr, "Illegal geometry for VideoScaler::Black "
			"%u, %u, %u, %u\n", col, row, width, height);
		return;
	}
	if (!pY) {
		fprintf(stderr, "Pointer to scaler surface is NULL for "
			"VideoScaler::Black\n");
		return;
	}
	// Make sure we have an even number
	width &= 0xfffe; height &= 0xfffe;
	col &= 0xfffe; row &= 0xfffe;

	if (m_video_type == VIDEO_YUY2) {
		// This only work with col, row, width, height being even
		// First we find starting point in frame
		pY += ((row*m_width + col)<<1);
		for (u_int32_t i=0; i < height; i++) {
			
			u_int8_t* pY_tmp = pY + (m_width<<1);
			for (u_int32_t n=0; n < width; n++) {
				//*pY++ = 0;
				//*pY++ = 127;
				*pY++ = 0;
				*pY++ = 127;
			}
			pY = pY_tmp; // Advance to same col next row
		}
	} 
	else if (m_video_type == VIDEO_I420) {
		u_int32_t half_width = width >> 1;
		u_int8_t* pU = pY + m_Ysize + (row>>1)*(m_width>>1) + (col>>1);
		pY += (row*m_width + col);
		for (u_int32_t i=0; i < (height>>1); i++) {
			memset(pY, 0, width);
			pY += m_width;		// Advance to same col next row
			memset(pY, 0, width);
			pY += m_width;		// Advance to same col next row
			memset(pU, 127, half_width);
			memset(pU+m_Usize, 127, half_width);
			pU += (m_width>>1);	// Advance to same col next row
		}
	}
	return;
}

