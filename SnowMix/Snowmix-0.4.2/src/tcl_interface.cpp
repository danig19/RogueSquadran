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
//#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
//#include <string.h>
//#include <malloc.h>
//#include <stdlib.h>
//#include <png.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

#include "snowmix.h"
#include "tcl_interface.h"
#include "command.h"

//#include "cairo/cairo.h"
//#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

//#include "virtual_feed.h"

class CVideoMixer;

int SnowmixGet(ClientData clientData, Tcl_Interp
	*interp, int argc, const char **argv) {
//	interp->result = "text place rgb 1 1.0 0.0 0.0";
	return TCL_OK;
}

void SnowmixGetDelete(ClientData clientData) {
}

CTclInterface::CTclInterface(CVideoMixer* pVideoMixer)
{
	m_pVideoMixer = pVideoMixer;
	int res = 0;
#ifdef HAVE_TCL
	//Tcl_FindExecutable(argv0);
	m_pInterp = Tcl_CreateInterp();
	if ((res = Tcl_Init(m_pInterp) != TCL_OK)) {
		Tcl_DeleteInterp(m_pInterp);
		m_pInterp = NULL;
		fprintf(stderr, "TCL INIT Failed\n");
	} else fprintf(stderr, "TCL INIT OK\n");
	if (m_pInterp)
		Tcl_CreateCommand(m_pInterp, "snowmix_get", SnowmixGet,
			(ClientData) NULL, SnowmixGetDelete);
#endif
	res = TCL_OK;
}

CTclInterface::~CTclInterface()
{
#ifdef HAVE_TCL
	if (m_pInterp) {
		Tcl_Finalize();
		Tcl_DeleteInterp(m_pInterp);
	}
	m_pInterp = NULL;
#endif
}


/*
int EvalFile (char *fileName)
{
    return Tcl_EvalFile(interp, fileName);
}
*/


int CTclInterface::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer) return 0;
#ifdef HAVE_TCL
	if (!m_pInterp) return 0;
//fprintf(stderr, "Tcl Parse command <%s>\n", str);

	if (!strncasecmp (str, "eval ", 5)) {
		if (set_tcl_eval(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "exec ", 5)) {
		if (set_tcl_exec(ctr, str+5)) return -1;
	} else return 1;
#endif
	return 0;
}

// Evaluate expression
int CTclInterface::set_tcl_eval(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str == '\0') return -1;
	int res = 0;
#ifdef HAVE_TCL
#ifdef DEBUG
	fprintf(stderr, "TCL eval <%s>\n", str);
#endif
        res = Tcl_Eval(m_pInterp, str);
        const char* str_res = Tcl_GetStringResult(m_pInterp);
	char* p = strdup(str_res);
	char* p_to_free = p;
	Tcl_ResetResult(m_pInterp);
	Tcl_FreeResult(m_pInterp);
        if (res == TCL_OK) {
#ifdef DEBUG
		fprintf(stderr, "TCL got <%s>\n", p);
#endif
		while (*p && *p != '\n') p++;
		if (*p == '\n') {
			p++;
			while (*p) {
				char* p2 = p;
				while (*p && *p != '\n') p++;
				if (*p == '\n') *p++ = '\0';
#ifdef DEBUG
fprintf(stderr, "PARSING <%s> ctr is %s\n", p2, ctr ? "ptr" : "null");
#endif
				m_pVideoMixer->m_pController->controller_parse_command(
					m_pVideoMixer, ctr, p2);
			}
		} // else fprintf(stderr, "TCL, no command to parse\n");
	} else {
#ifdef DEBUG
		fprintf(stderr, "TCL error. No result to parse. Script was <%s>\n<%s>\n", str, p_to_free ? p_to_free : "null");
#endif
	}
	if (p_to_free) free(p_to_free);
#endif
	return res;
}

// Evaluate command
int CTclInterface::set_tcl_exec(struct controller_type* ctr, const char* str)
{
	//char* line = NULL;

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str == '\0') return -1;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController ||
		!m_pVideoMixer->m_pController->m_pCommand) return -1;
	CCommand* pCommand = m_pVideoMixer->m_pController->m_pCommand;
	char* script_name = strdup(str);
	if (!script_name) return -1;
	char* p = script_name;
	while (*p) p++;
	p--;
	while (isspace(*p)) { *p = '\0'; p--; }
	char* script = pCommand->GetWholeCommand(script_name);
	if (set_tcl_eval(ctr, script)) return -1;
	return 0;
}
