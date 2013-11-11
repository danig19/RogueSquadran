/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __TCL_INTERFACE_H__
#define __TCL_INTERFACE_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <png.h>
//#include <video_image.h>

#include "video_mixer.h"

#ifdef HAVE_TCL
#include <tcl8.5/tcl.h>
#endif


class CTclInterface {
  public:
	CTclInterface(CVideoMixer* pVideoMixer);
	~CTclInterface();

	int	ParseCommand(struct controller_type* ctr, const char* str);

 private:
#ifdef HAVE_TCL
	int		set_tcl_eval(struct controller_type* ctr, const char* str);
	int		set_tcl_exec(struct controller_type* ctr, const char* str);

	Tcl_Interp*	 m_pInterp;
#endif

	CVideoMixer*		m_pVideoMixer;
};
	
#endif	// TCL_INTERFACE_H
