/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_MIXER_H__
#define __VIDEO_MIXER_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
#include <sys/time.h>

#include "snowmix.h"
#include "monitor.h"
#include "audio_feed.h"
#include "audio_mixer.h"
#include "audio_sink.h"
#include "video_feed.h"
#include "video_text.h"
#include "video_scaler.h"
#include "controller.h"
#include "video_image.h"
#include "video_shape.h"
#include "virtual_feed.h"
#include "tcl_interface.h"
#include "cairo_graphic.h"

class CTclInterface;
class CCairoGraphic;
class CVideoImage;
class CVideoFeed;
class CVideoText;
class CVirtualFeed;
class CAudioFeed;
class CAudioMixer;
class CAudioSink;
class CVideoShape;

class CVideoMixer {
  public:
	CVideoMixer();
	~CVideoMixer();
	int		SetGeometry(const u_int32_t width,const u_int32_t height, const char* pixelformat);
	int 		output_producer (struct feed_type* system_feed);
	int		OverlayFeed(char *str);
	int		OverlayImage(const char *str);
	int		OverlayText(const char *str, CCairoGraphic* pCairoGraphic);
	int		CairoOverlayFeed(CCairoGraphic* pCairoGraphic, char* str);
	int		OverlayVirtualFeed(CCairoGraphic* pCairoGraphic, char* str);

	u_int32_t	m_geometry_width;
	u_int32_t	m_geometry_height;
	char		m_pixel_format[5];
	VideoType	m_video_type;
	u_int32_t	m_pixel_bytes;
	double		m_frame_rate;
	u_int32_t	m_block_size;
	//int		m_exit_program;
	//bool		m_system_monitor;
	CMonitor_SDL*	m_pMonitor;
	CVideoScaler*	m_pVideoScaler;
	CCairoGraphic*	m_pCairoGraphic;
	CTextItems*	m_pTextItems;
	struct timeval	m_start;
	struct timeval	m_start_master;
	struct timeval	m_time_last;
	struct timeval	m_mixer_duration;
	u_int32_t	m_output_called;
        u_int32_t	m_output_missed;
	CVideoImage*	m_pVideoImage;
	CAudioFeed*	m_pAudioFeed;
	CAudioMixer*	m_pAudioMixer;
	CAudioSink*	m_pAudioSink;
	CVideoFeed*	m_pVideoFeed;
	CVideoShape*	m_pVideoShape;
	CVirtualFeed*	m_pVirtualFeed;
	CTclInterface*	m_pTclInterface;

	// Overall control variables.
	char*		m_pOutput_master_name;
	int		m_output_stack[MAX_STACK];
	int		m_output_master_socket;
	int		m_output_client_socket;

	/* Shared memory for delivering output frames. */
	char    m_output_memory_handle_name [64];         /* Name of the shared memory handle. */
	int     m_output_memory_handle;              /* File handle for the shared memory block. */
	int     m_output_memory_size;                 /* Size of the shared memory block. */
	uint8_t*	m_pOutput_memory_base;             /* Pointer to the shared mamory block as mapped in RAM. */
	struct output_memory_list_type*  m_pOutput_memory_list;     /* List of individual buffer blocks. */

	CController*	m_pController;			// Active controller for this mixer

	u_int8_t*	m_overlay;			// Active overlay
	char*		m_command_pre;			// Name of command to run always and before overlay
	char*		m_command_finish;		// Name of command to run before output
	char*		m_write_png_file;		// Write a PNG file of output once if not NULL

  private:
};

#endif	// VIDEO_MIXER
