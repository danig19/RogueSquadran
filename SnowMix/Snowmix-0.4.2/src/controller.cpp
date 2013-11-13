/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Scott		pscott@mail.dk
 *		 Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

/************************************************************************
*									*
*	Command interface.						*
*									*
************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <arpa/inet.h>


#include <netinet/in.h>
//#include <stdbool.h>

#include "snowmix.h"
#include "controller.h"
#include "video_mixer.h"
#include "video_text.h"
#include "audio_feed.h"
#include "audio_sink.h"
#include "video_shape.h"
#include "video_mixer.h"
#include "video_feed.h"
#include "virtual_feed.h"
#include "tcl_interface.h"
#include "command.h"


/* Function to delete trailing white space chars on a string. */
void trim_string (char *str)
{
	char	*cp;

	if (strlen (str) <= 0) return;

	cp = str + strlen (str) - 1;
	while (cp >= str && *cp <= ' ') {
		*cp = 0;			/* Move the terminator back. */
		cp--;
	}
}

/* Initialize the controller subsystem. */
CController::CController(char *init_file)
{
	struct controller_type	*ctr;
	m_pController_list = NULL;
	m_controller_listener = -1;
	m_host_allow = NULL;

	//m_pFeed_list = NULL;
        m_pSystem_feed = NULL;
	m_monitor_status = false;
	//m_pMonitor = NULL;

	/* Create the default controler on stdin/stdout. */
	ctr = (controller_type*) calloc (1, sizeof (*ctr));
	ctr->read_fd = 0;
	ctr->write_fd = 1;
	list_add_head (m_pController_list, ctr);

	/* Say hellow to the console controller. */
	controller_write_msg (ctr, "%s", BANNER);

	if (init_file != NULL) {
		/* Setup a controller entity to read the init file. */
		ctr = (controller_type*) calloc (1, sizeof (*ctr));
		ctr->read_fd = open (init_file, O_RDONLY);
		if (ctr->read_fd < 0) {
			fprintf (stderr, "Unable to open init file: \"%s\": %s\n",
				init_file, strerror (errno));
			fprintf(stderr, "Snowmix require an ini file to start.\n"
				"Read more about Snowmix on http://sourceforge.net/projects/snowmix/ "
				"and http://sourceforge.net/p/snowmix/wiki/Snowmix%%20Configuration/\n");
			free (ctr);
			exit(0);
		}
		ctr->write_fd = -1;
		list_add_head (m_pController_list, ctr);
	}
	m_command_name				= NULL;
	m_pCommand				= NULL;
	m_verbose				= false;
	m_maxplaces_text			= MAX_STRINGS;
        m_maxplaces_font			= MAX_FONTS;
        m_maxplaces_images			= MAX_IMAGES;
        m_maxplaces_text_places 		= MAX_TEXT_PLACES;
        m_maxplaces_image_places 		= MAX_IMAGE_PLACES;
        m_maxplaces_shapes 			= MAX_SHAPES;
        m_maxplaces_shape_places 		= MAX_SHAPE_PLACES;
        m_maxplaces_patterns	 		= MAX_PATTERNS;
        m_maxplaces_audio_feeds			= MAX_AUDIO_FEEDS;
        m_maxplaces_audio_mixers		= MAX_AUDIO_MIXERS;
        m_maxplaces_audio_sinks			= MAX_AUDIO_SINKS;
        m_maxplaces_video_feeds			= MAX_VIDEO_FEEDS;
        m_maxplaces_virtual_feeds		= MAX_VIRTUAL_FEEDS;
	m_frame_seq_no				= 0;
	m_lagged_frames				= 0;
	m_broadcast				= false;
	m_control_port				= -1;
	m_control_connections			= 2;		// The ini file and stdin/stdout/stderr.
								// Closing the inifile will decrement the count
}

CController::~CController() {
	while (m_host_allow) {
		host_allow_t* p = m_host_allow->next;
		free(m_host_allow);
		m_host_allow = p;
	}
}

/* Write a message to a controller (or all controllers). */
int CController::controller_write_msg (struct controller_type *ctr, const char *format, ...)
{
	va_list	ap;
	char	buffer [2000];
	int	x;

	va_start (ap, format);
	x = vsnprintf (buffer, sizeof (buffer), format, ap);
	va_end (ap);

	if (ctr) {
		/* Write to this controller. */
		if (ctr->write_fd < 0) {
			x = write (2, buffer, x);			/* No output. Write to stderr. */
		} else {
			x = write (ctr->write_fd, buffer, x);
		}
	} else {
		/* Write to all controllers. */
		if (m_broadcast) for (ctr = m_pController_list; ctr != NULL; ctr = ctr->next) {
			if (!ctr->is_audio) x = write (ctr->write_fd, buffer, x);
		}
	}

	return x;
}

char* CController::SetCommandName(const char* name)
{
	if (m_command_name) free(m_command_name);
	if (name) {
		while (isspace(*name)) name++;
		m_command_name = strdup(name);
		if (m_command_name) trim_string(m_command_name);
		return m_command_name;
	}
	return NULL;
}


/* Parse received commands. */
void CController::controller_parse_command (CVideoMixer* pVideoMixer, struct controller_type *ctr, char *line)
{
	char	*co = line;
	char	*ci = line;
	int	space_last;
	int	x;
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for controller_parse_command.\n");
		return;
	}

	/* Loose any initial whitespace and prune up the command. */
	while (*ci != 0 && *ci <= ' ') 	ci++;
	space_last = 0;
	while (*ci != 0) {
		if (*ci <= ' ' && space_last) {
			/* Skip this extra space. */
			ci++;
		} else if (*ci <= ' ') {
			/* Add a space to the output. */
			*co = ' ';
			co++;
			ci++;
			space_last = 1;
		} else {
			/* Add the char. */
			*co = *ci;
			co++;
			ci++;
			space_last = 0;
		}
	}
	*co = 0;	/* Terminate the string properly. */
	ci = line;

	/* Terminate the line on a '#' char - that is for comments. */
	if (*ci == '#') {
		*ci = 0;
	}

	//co = index (ci, '#');
	//if (co != NULL) {
	//	*co = 0;
	//}

	// ctr is NULL if we are executing a command
	if (ctr && m_command_name) {
		if (!strncasecmp (ci, "command end ",
			strlen ("command end "))) {
			if (m_verbose) controller_write_msg(ctr, "MSG: command"
				" end <%s>\n", m_command_name);
                        bool not_tcl = true;
                        int len = strlen(m_command_name);
                        if (len > 3) {
				not_tcl = strncasecmp(m_command_name+len-4,".tcl",4);
                        }
			if (not_tcl && m_pCommand && m_command_name)
				m_pCommand->CommandCheck(m_command_name);
			if (m_command_name) free(m_command_name);
			m_command_name = NULL;
		} else {
			//fprintf(stderr, "command add <%s>\n", ci);
			if (!m_pCommand) m_pCommand = new CCommand(pVideoMixer);
			if (m_pCommand) m_pCommand->AddCommandByName(
				m_command_name, ci);
		}
		return;
	}
	// command .....
	else if (!strncasecmp (ci, "command ", strlen("command "))) {
		if (!m_pCommand) m_pCommand = new CCommand(pVideoMixer);
		if (!m_pCommand) {
			controller_write_msg (ctr, "MSG: Failed to "
				"initialize CCommand\n");
			return;
		}
		int n = m_pCommand->ParseCommand(this, ctr, ci+8);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto invalid;
	}
	// tcl ....
	else if (!strncasecmp (ci, "tcl ", strlen ("tcl "))) {
		if (!pVideoMixer->m_pTclInterface)
			pVideoMixer->m_pTclInterface =
			new CTclInterface(pVideoMixer);
		if (!pVideoMixer->m_pTclInterface) {
			controller_write_msg (ctr, "MSG: Failed to "
				"initialize CTclInterface\n");
			return;
		}
		int n = pVideoMixer->m_pTclInterface->ParseCommand(ctr, ci+4);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto wrong_param_no;	// Fixme. Actual something else failed
	}
	// virtual feed ....
	else if (!strncasecmp (ci, "virtual feed ", strlen ("virtual feed "))) {
		if (!pVideoMixer->m_pVirtualFeed) pVideoMixer->m_pVirtualFeed =
			 new CVirtualFeed(pVideoMixer, m_maxplaces_virtual_feeds);
		if (!pVideoMixer->m_pVirtualFeed) {
			controller_write_msg (ctr,
				"MSG: Failed to initialize CVirtualFeed\n");
			return;
		}
		int n = pVideoMixer->m_pVirtualFeed->ParseCommand(ctr, ci+13);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto wrong_param_no;	// Fixme. Actual something else failed

	}
	else if (!strncasecmp (ci, "system ", strlen ("system "))) {
		ci += 7;

		// Check for system info
		if (!strncasecmp (ci, "info ", 5)) {
			int n = list_system_info(pVideoMixer, ctr, ci+5);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		// Check for system geometry
		} else if (!strncasecmp (ci, "geometry ", strlen ("geometry "))) {
			int	w, h;
			char	format [100];
			// We need to initalize CVideoFeed first to set geometry for feed 0
			if (!pVideoMixer->m_pVideoFeed)
				pVideoMixer->m_pVideoFeed =
					new CVideoFeed(pVideoMixer, m_maxplaces_video_feeds);
			if (!pVideoMixer->m_pVideoFeed) {
				fprintf(stderr, "pVideoFeed failed to initialize before "
					"setting geometry.\n");
				goto invalid;
			}
			x = sscanf (ci + strlen ("geometry "), "%d %d %s", &w, &h, &format [0]);
			if (x != 3) goto wrong_param_no;
			x = feed_init_basic (pVideoMixer, w, h, format);
			if (x < 0) goto invalid;
		}
		else if (!strncasecmp (ci, "frame rate ", strlen ("frame rate "))) {
			double r;
			x = sscanf (ci + strlen ("frame rate "), "%lf", &r);
			if (x != 1) goto wrong_param_no;
			if (r < MIN_SYSTEM_FRAME_RATE ||
				r > MAX_SYSTEM_FRAME_RATE) goto invalid;
			pVideoMixer->m_frame_rate = r;
		}
		else if (!strncasecmp (ci, "socket ", strlen ("socket "))) {
			if (!pVideoMixer->m_geometry_width ||
				!pVideoMixer->m_frame_rate) goto setup_geo_frame_first;
			x = output_init_runtime (pVideoMixer, ctr, ci + strlen ("socket "));
			if (!x) return;
			if (x<1) goto wrong_param_no;
			else goto invalid;
		}
		else if (!strncasecmp (ci, "control port ", strlen ("control port "))) {
			x = set_system_controller_port(ctr, ci+13);
			if (!x) return;
			else if (x<0) goto wrong_param_no;
			else goto invalid;
		}
		else if (!strncasecmp (ci, "host allow ", 11)) {
			int x = host_allow(ctr, ci+11);
			if (x < 0) goto wrong_param_no;
			else if (x > 0) goto invalid;
		}
		else goto invalid;
	}
	// audio ...
	else if (!strncasecmp (ci, "audio ", strlen ("audio "))) {
		char* str = ci + 6;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "feed ", 5)) {
			if (!pVideoMixer->m_pAudioFeed) pVideoMixer->m_pAudioFeed =
			 	new CAudioFeed(pVideoMixer, m_maxplaces_audio_feeds);
			if (!pVideoMixer->m_pAudioFeed) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioFeed\n");
				return;
			}
			int n = pVideoMixer->m_pAudioFeed->ParseCommand(ctr, str+5);
			if (n == 0) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		}
		else if (!strncasecmp (str, "sink ", 5)) {
			if (!pVideoMixer->m_pAudioSink) pVideoMixer->m_pAudioSink =
			 	new CAudioSink(pVideoMixer, m_maxplaces_audio_sinks);
			if (!pVideoMixer->m_pAudioSink) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioSink\n");
				return;
			}
			int n = pVideoMixer->m_pAudioSink->ParseCommand(ctr, str+5);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		}
		else if (!strncasecmp (str, "mixer ", 6)) {
			if (!pVideoMixer->m_pAudioMixer) pVideoMixer->m_pAudioMixer =
			 	new CAudioMixer(pVideoMixer, m_maxplaces_audio_mixers);
			if (!pVideoMixer->m_pAudioMixer) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioMixer\n");
				return;
			}
			int n = pVideoMixer->m_pAudioMixer->ParseCommand(ctr, str+6);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		} else goto invalid;

	}
	// feed
	else if (!strncasecmp (ci, "feed ", 5)) {
		if (!pVideoMixer->m_pVideoFeed) pVideoMixer->m_pVideoFeed =
			 new CVideoFeed(pVideoMixer, m_maxplaces_video_feeds);
		if (!pVideoMixer->m_pVideoFeed) {
			controller_write_msg (ctr,
				"MSG: Failed to initialize CVideoFeed\n");
			return;
		}
		int n = pVideoMixer->m_pVideoFeed->ParseCommand(ctr, ci+5);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto invalid;

	}
	// text ...
	else if (!strncasecmp (ci, "text ", 5)) {
		if (!pVideoMixer->m_pTextItems)
			pVideoMixer->m_pTextItems =
				new CTextItems(pVideoMixer,
					m_maxplaces_text, m_maxplaces_font,
					m_maxplaces_text_places);
		if (pVideoMixer->m_pTextItems) {
			int n = pVideoMixer->m_pTextItems->ParseCommand(this, ctr, ci+5);
			if (!n) return;
			else if (n < 0) goto wrong_param_no;
			goto invalid;
		} else controller_write_msg (ctr,
			"MSG : Failed to create class TextItem.\n");
		return;
	}
	// image ...
	else if (!strncasecmp (ci, "image ", strlen ("image "))) {
		if (!pVideoMixer->m_pVideoImage) {
			pVideoMixer->m_pVideoImage = new CVideoImage( pVideoMixer,
				m_maxplaces_images, m_maxplaces_image_places);
			if (!pVideoMixer->m_pVideoImage) {
				controller_write_msg (ctr,
					"MSG : Unabale to create class CVideoImage.\n");
				goto invalid;
			}
		}
		int n = pVideoMixer->m_pVideoImage->ParseCommand(this, ctr, ci+6);
		if (!n) return;
		else if (n < 0) goto wrong_param_no;
		goto invalid;
	}
	else if (!strncasecmp (ci, "shape ", 6)) {
		if (!pVideoMixer->m_pVideoShape) {
			pVideoMixer->m_pVideoShape = new CVideoShape( pVideoMixer,
				m_maxplaces_shapes, m_maxplaces_shape_places);
			if (!pVideoMixer->m_pVideoShape) {
				controller_write_msg (ctr,
					"MSG : Unabale to create class CVideoShape.\n");
				goto invalid;
			}
		}
		int n = pVideoMixer->m_pVideoShape->ParseCommand(ctr, ci+6);
		if (!n) return;
		else if (n < 0) goto wrong_param_no;
		goto invalid;
	}
	else if (!strncasecmp (ci, "overlay pre ", 12)) {
		if (pVideoMixer->m_command_pre) free(pVideoMixer->m_command_pre);
		pVideoMixer->m_command_pre = NULL;
		char* str = ci + 12;
		while (isspace(*str)) str++;
		trim_string(str);
		if (*str) pVideoMixer->m_command_pre = strdup(str);
		else goto wrong_param_no;
	}
	else if (!strncasecmp (ci, "overlay finish ", strlen ("overlay finish "))) {
		if (pVideoMixer->m_command_finish) free(pVideoMixer->m_command_finish);
		pVideoMixer->m_command_finish = NULL;
		char* str = ci + strlen ("overlay finish ");
		while (isspace(*str)) str++;
		trim_string(str);
		if (*str) pVideoMixer->m_command_finish = strdup(str);
		else goto wrong_param_no;
	}
	else if (!strncasecmp (ci, "png write ", strlen ("png write "))) {
		if (pVideoMixer->m_write_png_file) free(pVideoMixer->m_write_png_file);
		pVideoMixer->m_write_png_file = NULL;
		char* str = ci + strlen ("png write ");
		while (isspace(*str)) str++;
		trim_string(str);
		if (*str) pVideoMixer->m_write_png_file = strdup(str);
		else goto wrong_param_no;
	}
	else if (!strncasecmp (ci, "monitor ", strlen ("monitor "))) {
		if (strlen(ci) == strlen("monitor ")) list_monitor (ctr);
		else if (!strncasecmp (ci, "monitor on ", strlen ("monitor on ")))
			set_monitor_on (pVideoMixer, ctr);
		else if (!strncasecmp (ci, "monitor off ", strlen ("monitor off ")))
			set_monitor_off (pVideoMixer, ctr);
		else  goto invalid;
	}

	else if (!strncasecmp (ci, "select ", strlen ("select "))) {
		int	no;

		x = sscanf (ci + strlen ("select "), "%d", &no);
		if (x != 1) goto wrong_param_no;
		if (!pVideoMixer || !pVideoMixer->m_pVideoFeed) {
			controller_write_msg (ctr, "MSG: pVideoMixer and pVideoFeed must "
				"be set before 'select ' can be used.\n");
			goto invalid;
		}
		pVideoMixer->m_pVideoFeed->output_select_feed (ctr, no);
	}
	else if (!strncasecmp (ci, "stack ", strlen ("stack "))) {
		output_set_stack (pVideoMixer, ctr, ci + strlen ("stack "));
	}

	else if (!strncasecmp (ci, "stat ", strlen ("stat "))) {
		/* Dump a feed/output stack status. */
		if (!pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "Can not stat feeds as pVideoFeed is NULL\n");
			goto invalid;
		}
		struct feed_type *feed = pVideoMixer->m_pVideoFeed->GetFeedList();

		controller_write_msg (ctr, "STAT: ID  Name                  State               Mode     Cutout       Size     Offset  Stack Layer\n");
		for ( ; feed != NULL; feed = feed->next) {
			char	*state;
			char	cut [20];
			char	layer [20];

			switch (feed->state) {
				case SETUP:        state = (char*) "Setup";        break;
				case PENDING:      state = (char*) "Pending";      break;
				case DISCONNECTED: state = (char*) "Disconnected"; break;
				case RUNNING:      state = (char*) "Running";      break;
				case STALLED:      state = (char*) "Stalled";      break;
				default:      state = NULL;      break;
			}
			if (feed->width == pVideoMixer->m_geometry_width &&
				feed->height == pVideoMixer->m_geometry_height) {
				strcpy (cut, "Fullscreen");
			} else {
				strcpy (cut, "Cutout");
			}

			strcpy (layer, "--");
			for (x = 0; x < MAX_STACK; x++) {
				if (pVideoMixer->m_output_stack [x] == feed->id) {
					sprintf (layer, "%d", x);
					break;
				}
			}

			controller_write_msg (ctr, "STAT: %2d  <%-20s>  %-12s  "
				"%10s  %4dx%-4d  %4dx%-4d  %4dx%-4d  %2d:%-2d %s\n",
						   feed->id, feed->feed_name ? feed->feed_name : "",
						   state, cut,
						   feed->cut_start_column, feed->cut_start_row,
						   feed->cut_columns, feed->cut_rows,
						   feed->shift_columns, feed->shift_rows,
						   feed->scale_1, feed->scale_2,
						   layer);
		}
		controller_write_msg (ctr, "STAT: --\n");	/* End of table marker. */
	}
	else if (!strncasecmp (ci, "quit ", strlen ("quit "))) {
		exit_program = 1;
	}

	// Set or list maxplaces
	else if (!strncasecmp (ci, "maxplaces ", strlen ("maxplaces "))) {
		int n = set_maxplaces (ctr, ci );
		if (n < 0) goto wrong_param_no;
		else if (n > 0) goto invalid;
		else return;
		if (n != 1) goto wrong_param_no;
		 else goto invalid;
		return;


	// write out message
	} else if (!strncasecmp (ci, "message ", 8)) {
		//fprintf(stderr, MESSAGE"<%u><%s>\n", m_frame_seq_no, ci + strlen ("message "));
		controller_write_msg(ctr, "MSG: %s\n", ci+8);

	// write out message with frame number
	} else if (!strncasecmp (ci, "messagef ", 9)) {
		controller_write_msg(ctr, "MSG: <%u> %s\n", m_frame_seq_no, ci+9);

	// Toggle verbosity
	} else if (!strncasecmp (ci, "verbose ", strlen ("verbose "))) {
		m_verbose = !m_verbose;
		if (m_verbose) controller_write_msg(ctr, "MSG: verbose is on\n");

	// include <file name>
	} else if (!strncasecmp (ci, "include ", strlen ("include "))) {
		trim_string(ci+8);
		int read_fd = open (ci+8, O_RDONLY);
		if (read_fd < 0) {
			fprintf (stderr, "Unable to open include file: \"%s\": %s\n",
				ci+8, strerror (errno));
			goto invalid;
		}
		off_t file_size = lseek(read_fd, 0, SEEK_END);
		if (file_size < 0 || lseek(read_fd, 0, SEEK_SET) < 0) {
			fprintf (stderr, "Unable to seek in include file: \"%s\": %s\n",
				ci+8, strerror (errno));
			close (read_fd);
			goto invalid;
		}
		char* buffer = (char*) malloc((size_t) file_size + 2);
		char* buffer_to_free = buffer;
		if (!buffer) {
			fprintf (stderr, "Unable to allocate memory for reading include file: \"%s\"\n",ci+8);
			close(read_fd);
			goto invalid;
		}
		int bytes_read = 0;
		while (bytes_read < file_size) {
			int n = read(read_fd, buffer+bytes_read, file_size - bytes_read);
			if (n<0) break;
			bytes_read += n;
		}
		close(read_fd);
		if (bytes_read != file_size) {
			fprintf(stderr, "Failed to read all the bytes of the include file: \"%s\"\n",ci+8);
			free(buffer_to_free);
			goto invalid;
		}
		*(buffer+bytes_read) = '\0';
		char* ci = buffer;
		char* p = NULL;
		while ((p = index(buffer, '\n')) != NULL) {
			if (*p) {
				*p = ' ';
				p++;
				char tmp = *p;
				*p = '\0';
				controller_parse_command (pVideoMixer, ctr, buffer);
				*p = tmp;
			} else {
				fprintf(stderr, "Unexpected nul while reading include file %s\n", ci+8);
				break;
			}
			buffer = p;
		}
		free (buffer_to_free);
	
	// require version
	} else if (!strncasecmp (ci, "require version ",
		strlen ("require version "))) {
		int n = require_version(ctr, ci);
		if (n > 0) exit(1);

	// List Help
//fprintf(stderr, " - Parsing <%s> near help\n", ci);
	} else if (!strncasecmp (ci, "h ", strlen ("h ")) ||
	    !strncasecmp (ci, "help ", strlen ("help "))) {
		if (list_help(ctr, ci)) goto invalid;

	// Command not found. Search in the command list
	} else if (m_pCommand) {
		trim_string(ci);
		if (m_pCommand->CommandExists(ci)) {
			if (ParseCommandByName (pVideoMixer, ctr, ci)) goto invalid;
			return;
		} else {
			/* Only complain if it contains anything but space chars. */
			for (co = ci; *co != 0; co++) {
				if (*co > ' ') {
					if (ctr) controller_write_msg (ctr,
						"MSG: Unknown command: \"%s\"\n", ci);
					break;
				}
			}
		}
	} else {

		/* Only conplain if it contains anything but space chars. */
		for (co = ci; *co != 0; co++) {
			if (*co > ' ') {
				if (ctr) controller_write_msg (ctr,
					"MSG: Unknown command: \"%s\"\n", ci);
				break;
			}
		}
	}
	return;


setup_geo_frame_first:
	controller_write_msg (ctr, "MSG: Setup geometry and frame rate first\n");
	return;

invalid:
	controller_write_msg (ctr, "MSG: Invalid parameters: \"%s\"\n", ci);
	return;

wrong_param_no:
	controller_write_msg (ctr, "MSG: Invalid number of parameters: \"%s\"\n", ci);
	return;
}

int CController::list_system_info(CVideoMixer* pVideoMixer, struct controller_type* ctr, const char* str)
{
	if (!ctr || !pVideoMixer) return 1;
	struct output_memory_list_type		*buf;
	u_int32_t outbuf_count = 0;
	u_int32_t inuse = 0;
	for (buf = pVideoMixer->m_pOutput_memory_list; buf != NULL;
		buf = buf->next) {
		outbuf_count++;
		if (buf->use) inuse++;
	}
	
	controller_write_msg (ctr, STATUS" System info\n"
		STATUS" Snowmix version     : "SNOWMIX_VERSION"\n"
		STATUS" System geometry     : %ux%u pixels\n"
		STATUS" Pixel format        : %s\n"
		STATUS" Bytes per pixel     : %u\n"
		STATUS" Frame rate          : %.3lf\n"
		STATUS" Block size mmap     : %u\n"
		STATUS" Output frames inuse : %u of %u\n"
		STATUS" Frame sequence no.  : %u\n"
		STATUS" Lagged frames       : %u\n"
		STATUS" Open Control conns. : %d\n"
		STATUS" Control port number : %d\n"
		STATUS" Host allow          :%s",
		pVideoMixer->m_geometry_width,
		pVideoMixer->m_geometry_height,
		pVideoMixer->m_pixel_format,
		pVideoMixer->m_pixel_bytes,
		pVideoMixer->m_frame_rate,
		pVideoMixer->m_block_size,
		inuse,
		outbuf_count,
		m_frame_seq_no,
		m_lagged_frames,
		m_control_connections,
		m_control_port,
		m_host_allow ? "" : " undefined - only 127.0.0.1");
	host_allow_t* ph = m_host_allow;
	while (ph) {
		u_int32_t mask = ph->ip_to - ph->ip_from;
		u_int32_t count = 32;
		while (mask) {
			mask = mask >> 1;
			count--;
		}
		controller_write_msg (ctr, " %u.%u.%u.%u/%u",
			(ph->ip_from >> 24),
			(0xff0000 & ph->ip_from) >> 16,
			(0xff00 & ph->ip_from) >> 8,
			ph->ip_from & 0xff, count);
		ph = ph->next;
	}

	controller_write_msg (ctr, "\n"
		STATUS" Ctr verbose         : %s\n"
		STATUS" Ctr broadcast       : %s\n"
		STATUS" Output ctr socket   : %s\n"
		STATUS" Output ctr fd       : %d\n"
		STATUS" Video feeds         : %s\n"
		STATUS" Virtual video feeds : %s\n"
		STATUS" Video text          : %s\n"
		STATUS" Video image         : %s\n"
		STATUS" Video shapes        : %s\n"
		STATUS" Command interface   : %s\n"
		STATUS" Audio feeds         : %s\n"
		STATUS" Audio mixers        : %s\n"
		STATUS" Audio sinks         : %s\n"
		STATUS" TCL interpreter     : %s\n"
		STATUS" Stack               :",
		m_verbose ? "yes" : "no",
		m_broadcast ? "yes" : "no",
		pVideoMixer->m_pOutput_master_name ? pVideoMixer->m_pOutput_master_name : "",
		pVideoMixer->m_output_master_socket,
		pVideoMixer->m_pVideoFeed ? "loaded" : "no",
		pVideoMixer->m_pVirtualFeed ? "loaded" : "no",
		pVideoMixer->m_pTextItems ? "loaded" : "no",
		pVideoMixer->m_pVideoImage ? "loaded" : "no",
		pVideoMixer->m_pVideoShape ? "loaded" : "no",
		m_pCommand ? "loaded" : "no",
		pVideoMixer->m_pAudioFeed ? "loaded" : "no",
		pVideoMixer->m_pAudioMixer ? "loaded" : "no",
		pVideoMixer->m_pAudioSink ? "loaded" : "no",
		pVideoMixer->m_pTclInterface ? "loaded" : "no"

	);
	for (int x = 0; x < MAX_STACK; x++) {
		if (pVideoMixer->m_output_stack[x] > -1)
			controller_write_msg (ctr, " %d", pVideoMixer->m_output_stack[x]);
	}
	controller_write_msg (ctr, "\n"STATUS"\n");
	return 0;
}

int CController::ParseCommandByName (CVideoMixer* pVideoMixer, controller_type* ctr, char* commandname)
{
	if (m_pCommand && m_pCommand->CommandExists(commandname)) {
		char* str = NULL;
		while ((str = m_pCommand->GetNextCommandLine(commandname))) {
			while (isspace(*str)) str++;
			if (!strncasecmp(str, "loop ", 5)) {
				//m_pCommand->RestartCommandPointer(commandname);
				break;
			}
			if (!strncasecmp(str, "next ", 5) || !strncasecmp(str, "nextframe ", 10)) break;
			if (pVideoMixer) {
				char* command = strdup(str);
				if (!strncasecmp(str, "overlay feed ", 13)) {
					if (pVideoMixer)
						pVideoMixer->OverlayFeed(str + 13);
				} else if (!strncasecmp(str, "overlay text ", 13)) {
					if (pVideoMixer)
						pVideoMixer->OverlayText(str + 13,
							pVideoMixer->m_pCairoGraphic);
				} else if (!strncasecmp(str, "overlay image ", 14)) {
					if (pVideoMixer)
						pVideoMixer->OverlayImage(str + 14);
				} else if (!strncasecmp(str, "cairooverlay feed ", 18)) {
					if (pVideoMixer)
						pVideoMixer->CairoOverlayFeed(
							pVideoMixer->m_pCairoGraphic,
								str + 18);
				} else if (!strncasecmp(str, "overlay virtual feed ",
					21)) {
					if (pVideoMixer)
						pVideoMixer->OverlayVirtualFeed(
							pVideoMixer->m_pCairoGraphic,
								str + 21);
				} else {
					int n = strlen(commandname);
					if (strncmp(command, commandname, n) == 0 &&
						*(command + n) == ' ') {
						fprintf(stderr, "Recursive call In ParseCommandByName is forbidden. <%s> calling <%s>\n", commandname, command);
						free (command);
						break;
					}
					controller_parse_command (pVideoMixer,
						ctr, command);		// Can we safely parse ctr being not NULL ??????
						//NULL, command);	// We used as a test earlier to prevent loop
									// but the need for that appear to no longer
									// exist
				}
				free (command);
			}
		}
	} else return -1;
	return 0;
}
/* Populate rfds and wrfd for the controller subsystem. */
int CController::controller_set_fds (int maxfd, fd_set *rfds, fd_set *wfds)
{
	struct controller_type	*ctr;

	for (ctr = m_pController_list; ctr != NULL; ctr = ctr->next) {
		if (ctr->read_fd < 0) continue;			/* Not reading. */

		/* Add its descriptor to the bitmap. */
		FD_SET (ctr->read_fd, rfds);
		if (ctr->read_fd > maxfd) {
			maxfd = ctr->read_fd;
		}
	}

	/* Add the master socket to the list. */
	if (m_controller_listener >= 0) {
		FD_SET (m_controller_listener, rfds);
		if (m_controller_listener > maxfd) {
			maxfd = m_controller_listener;
		}
	}

	return maxfd;
}



/* Check if there is anything to do on a controller socket. */
void CController::controller_check_read (CVideoMixer* pVideoMixer, fd_set *rfds)
{
	struct controller_type	*ctr, *next_ctr;

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer for controller_check_read was NULL.\n");
		return;
	}

	// Check the list of known controllers.
	for (ctr = m_pController_list, next_ctr = ctr ? ctr->next : NULL;
	     ctr != NULL;
	     ctr = next_ctr, next_ctr = ctr ? ctr->next : NULL) {
		int	x;
		char	*ci, *co;

		if (ctr->read_fd < 0) continue;			// Not able to read.
		if (!FD_ISSET (ctr->read_fd, rfds)) continue;	// No data for this one.

		// Check if the ctr is used for streaming audio
		if (ctr->is_audio) {
			// Get state
			// Check that we have a pointer to the audio feed class
			if (pVideoMixer->m_pAudioFeed) {

				feed_state_enum state = pVideoMixer->m_pAudioFeed->GetState(ctr->feed_id);
				if (state == RUNNING || state == PENDING || state == STALLED) {
					u_int32_t size;
					audio_buffer_t* buf = pVideoMixer->m_pAudioFeed->
						GetEmptyBuffer(ctr->feed_id, &size);
					if (buf) {
						buf->len = x = read(ctr->read_fd, buf->data,
							size);
						if (x > 0) pVideoMixer->m_pAudioFeed->
							AddAudioToFeed(ctr->feed_id, buf);
						else free(buf);
					// else we need to close the connection
					} else x = -1;
				} else {
					fprintf(stderr, "Error. Controller is set to audio feed, "
						"but the feed is in state %s. Closing controller for fd %d,%d\n",
						feed_state_string(state), ctr->read_fd, ctr->write_fd);
					x = -1;
				}

			} else {
				fprintf(stderr, "Error. Controllor set to audio feed, "
					"but we have no CAudioFeed.\n");
				// We set x = -1. This should close the connection
				x = -1;
			}
		// Otherwise this is a normal control channel
		} else {

			/* Read the new data from the socket. */
			x = read (ctr->read_fd, &ctr->linebuf [ctr->got_bytes],
				sizeof (ctr->linebuf) - ctr->got_bytes - 1);
			//fprintf(stderr, "Got %d bytes - ctr\n", x);

		}
		if (x <= 0) {
			/* EOF. Close this controller. */
			list_del_element (m_pController_list, ctr);
			close (ctr->read_fd);
if (!ctr->feed_id && ! ctr->sink_id) fprintf(stderr, "Closing ctr\n");
			if (ctr->write_fd >= 0) {
				close (ctr->write_fd);
			}
			if (ctr->feed_id && pVideoMixer->m_pAudioFeed) {
				fprintf(stderr, "Closing ctr feed %d\n",ctr->feed_id);
				pVideoMixer->m_pAudioFeed->StopFeed(ctr->feed_id);
			}
			if (ctr->sink_id && pVideoMixer->m_pAudioSink) {
				fprintf(stderr, "Calling stop sink %u\n", ctr->sink_id);
				pVideoMixer->m_pAudioSink->StopSink(ctr->sink_id);
			}
			free (ctr);
			m_control_connections--;
			continue;
		}
		if (ctr->is_audio) continue;

		ctr->got_bytes += x;
		ctr->linebuf [ctr->got_bytes] = 0;	/* Make sure it is properly terminated. */

		/* Check if the buffer contains a full line. */
		while ((ci = index (ctr->linebuf, '\n')) != NULL) {
			char	line [1024];

			/* Yes. Copy the line to a new buffer. */
			ci = ctr->linebuf;
			co = line;
			while (*ci != 0) {
				*co = *ci;
				co++;
				if (*ci == '\n') break;
				ci++;
			}
			*co = 0;
			controller_parse_command (pVideoMixer, ctr, line);
			if (ctr->close_controller) break;

			/* Shift the rest of the buffer down to the beginning and try again. */
			ci++;
			co = ctr->linebuf;
			while (*ci != 0) {
				*(co++) = *(ci++);
			}
			*co = 0;	/* Terminate the result. */
		}
		if (ctr->close_controller) {
fprintf(stderr, "Closing Controller\n");
			list_del_element (m_pController_list, ctr);
			close (ctr->read_fd);
			if (ctr->write_fd >= 0) {
				close (ctr->write_fd);
			}
			free (ctr);
fprintf(stderr, "Controller Closed\n");
			continue;
		}

		/* Prepare for the next read. */
		ctr->got_bytes = strlen (ctr->linebuf);
	}

	if (m_controller_listener >= 0 && FD_ISSET (m_controller_listener, rfds)) {
		/* New connection. Create a new controller element. */
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		ctr = (controller_type*) calloc (1, sizeof (*ctr));
		ctr->read_fd = accept (m_controller_listener, (struct sockaddr*) &addr, &len);

		// Now check the connecting address. Assume IPV4
		// FIXME : Rewrite to include IPv6
		if (addr.sin_family != AF_INET) {
			fprintf(stderr, "Only IPv4 supported yet\n");
			close(ctr->read_fd);
			ctr->read_fd = -1;
		} else {
			u_int32_t ip = ntohl(*((u_int32_t*)&addr.sin_addr));
			if (!m_host_allow) {
				// Only ip = 127.0.0.1 is allowed then
				if (ip != 0x7f000001) {
					close(ctr->read_fd);
					ctr->read_fd = -1;
				}
				if (m_verbose) fprintf(stderr, "No host allow. "
					"Host %u %x allowed ? %s\n", ip, ip,
					ctr->read_fd > -1 ? "yes" : "no");
			} else {
				if (host_not_allow(ip)) {
					close(ctr->read_fd);
					ctr->read_fd = -1;
				}
				// FIXME. Use inet_ntop to print address.
				// See http://www.beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
				if (m_verbose) fprintf(stderr, "Host allow. Host allowed ? %s\n",
					ctr->read_fd > -1 ? "yes" : "no");
			}
		}
		if (ctr->read_fd >= 0) {
			char	msg [100];

			ctr->write_fd = ctr->read_fd;
			ctr->got_bytes = 0;
			ctr->is_audio = false;
			ctr->feed_id = 0;
			ctr->sink_id = 0;
			ctr->close_controller = false;

			/* Mark the writer socket as nonblocking. */
			list_add_head (m_pController_list, ctr);

			/* Say hellow to the new controller. */
			sprintf (msg, "%s", BANNER);
			ssize_t l = strlen(msg);
			do {
				ssize_t n = write (ctr->write_fd, msg, l);
				if (n < 0) {
					perror("Failed to write to controllor\n");
				}
				l -= n;
			} while (l > 0);
			m_control_connections++;
		} else {
			/* Failed on accept. */
			free (ctr);
		}
	}
}

void CController::ControllerListDeleteElement (struct controller_type* ctr)
{
fprintf(stderr, "Deleting Controller\n");
	list_del_element (m_pController_list, ctr);
	if (ctr->read_fd > -1) close (ctr->read_fd);
	if (ctr->write_fd >= 0) close (ctr->write_fd);
	free (ctr);
}

/* Find a feed by its id number. */
/*
struct feed_type* CController::find_feed (int id)
{
        struct feed_type        *feed;

        for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
                if (feed->id == id) {
                        return feed;
                }
        }
        return NULL;
}
*/

// require version <version>
int CController::require_version (struct controller_type *ctr, char* ci )
{
	if (!ci || !(*ci)) return 1;
	u_int32_t snow_major1, snow_major2, snow_minor;
	u_int32_t req_major1, req_major2, req_minor;
	req_minor = 0;
	int n = sscanf(SNOWMIX_VERSION,"%u.%u.%u", &snow_major1, &snow_major2,
		&snow_minor);
	if (n != 3) return 1;
	int j = sscanf(ci, "require version %u.%u.%u", &req_major1, &req_major2,
		&req_minor);
	if (j < 2) return -1;
	if (req_major1 == snow_major1 && req_major2 == snow_major2 &&
		req_minor <= snow_minor) {
		if (m_verbose) controller_write_msg (ctr, "Snowmix version %s "
			"satisfy requirement for version %u.%u.%u\n",
			SNOWMIX_VERSION, req_major1, req_major2, req_minor);
		return 0;
	}
	controller_write_msg (ctr, "This configuration require Snowmix version "
		"%u.%u.n where n must be at least %u.", req_major1, req_major2,
		req_minor);
	controller_write_msg (ctr, "Found Snowmix version %s\n", SNOWMIX_VERSION);
	return 1;
}


// Set system control port
int CController::set_system_controller_port (struct controller_type *ctr, char* ci )
{
	int	port;
	if (!ci) return 1;
	while (isspace(*ci)) ci++;
	int x = sscanf (ci, "%d", &port);
	if (x != 1 || port < 0 || port >= 65536) return -1;
	if (m_controller_listener >= 0) {
		controller_write_msg (ctr, "MSG: Listener have already been setup\n");
	} else {
		/* Setup the controller listener socket. */
		int	yes = 1;

		m_controller_listener = socket (AF_INET, SOCK_STREAM, 0);
		if (m_controller_listener < 0) {
			perror ("Unable to create controller listener");
			exit (1);
		}

		if (setsockopt (m_controller_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
			perror ("setsockopt on m_controller_listener");
			exit (1);
		}

		{
			struct sockaddr_in addr;

			memset (&addr, 0, sizeof (addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
				addr.sin_port = htons (port);
			if (bind (m_controller_listener, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
				perror ("bind on m_controller_listener");
				exit (1);
			}
		}

		if (listen (m_controller_listener, MAX_LISTEN_BACKLOG) < 0) {
			perror ("listen on m_controller_listener");
			exit (1);
		}
		m_control_port = port;
	}
	return 0;
}

// List or set maxplaces
int CController::set_maxplaces (struct controller_type *ctr, char* ci )
{
	if (strlen(ci) == strlen("maxplaces ")) {
		controller_write_msg (ctr,
			"MSG: Max text strings     : %u\n"
			"MSG: Max font definitions : %u\n"
			"MSG: Max text places      : %u\n"
			"MSG: Max loaded images    : %u\n"
			"MSG: Max placed images    : %u\n"
			"MSG: Max shapes           : %u\n"
			"MSG: Max placed shapes    : %u\n"
			"MSG: Max shape patterns   : %u\n"
			"MSG: Max audio feeds      : %u\n"
			"MSG: Max audio mixers     : %u\n"
			"MSG: Max audio sinks      : %u\n"
			"MSG: Max video feeds      : %u\n"
			"MSG: Max virtual feeds    : %u\n"
			"MSG:\n",
			m_maxplaces_text, m_maxplaces_font, m_maxplaces_text_places,
			m_maxplaces_images, m_maxplaces_image_places,
			m_maxplaces_shapes, m_maxplaces_shape_places,
			m_maxplaces_patterns,
			m_maxplaces_audio_feeds,
			m_maxplaces_audio_mixers,
			m_maxplaces_audio_sinks,
			m_maxplaces_video_feeds,
			m_maxplaces_virtual_feeds);
		return 0;
	}
	char* s = ci + strlen("maxplaces ");
	if (!strncasecmp (s, "strings ", strlen ("strings "))) {
		int n = sscanf(s, "strings %u", &m_maxplaces_text);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "fonts ", strlen ("fonts "))) {
		int n = sscanf(s, "fonts %u", &m_maxplaces_font);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "texts ", strlen ("texts "))) {
		int n = sscanf(s, "texts %u", &m_maxplaces_text_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "images ", strlen ("images "))) {
		int n = sscanf(s, "images %u", &m_maxplaces_images);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "imageplaces ", strlen ("imageplaces "))) {
		int n = sscanf(s, "imageplaces %u", &m_maxplaces_image_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shapes ", strlen ("shapes "))) {
		int n = sscanf(s, "shapes %u", &m_maxplaces_shapes);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shapeplaces ", strlen ("shapeplaces "))) {
		int n = sscanf(s, "shapeplaces %u", &m_maxplaces_shape_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shape patterns ", strlen ("shape patterns "))) {
		int n = sscanf(s, "shape patterns %u", &m_maxplaces_patterns);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio feeds ", strlen ("audio feeds "))) {
		int n = sscanf(s, "audio feeds %u", &m_maxplaces_audio_feeds);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio mixers ", strlen ("audio mixers "))) {
		int n = sscanf(s, "audio mixers %u", &m_maxplaces_audio_mixers);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio sinks ", strlen ("audio sinks "))) {
		int n = sscanf(s, "audio sinks %u", &m_maxplaces_audio_sinks);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "video feeds ", strlen ("video feeds "))) {
		int n = sscanf(s, "video feeds %u", &m_maxplaces_video_feeds);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "virtual feeds ", strlen ("virtual feeds "))) {
		int n = sscanf(s, "virtual feeds %u", &m_maxplaces_virtual_feeds);
		if (n != 1) return -1;
	} else return 1;
	return 0;
}


bool CController::host_not_allow (u_int32_t ip)
{
	if (!m_host_allow) return true;
	host_allow_t* p = m_host_allow;

	// m_host_allow is a sorted list with lowest range first
	while (p) {
		if (p->ip_from > ip) break;
		if (p->ip_to >= ip) return false; // it is allowed
		p = p->next;
	}
	return true; // it is not allowed
}

int CController::host_allow (struct controller_type *ctr, const char* ci)
{
	u_int32_t ip_from, ip_to, a, b, c, d, e;
	if (!ci) return -1;
	e = 33;
	ip_to = ip_from = 0;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		host_allow_t* ph = m_host_allow;
		while (ph) {
			u_int32_t mask = ph->ip_to - ph->ip_from;
			u_int32_t count = 32;
			while (mask) {
				mask = mask >> 1;
				count--;
			}
			controller_write_msg (ctr,
				MESSAGE" host allow %u.%u.%u.%u/%u\n",
				(ph->ip_from >> 24),
				(0xff0000 & ph->ip_from) >> 16,
				(0xff00 & ph->ip_from) >> 8,
				ph->ip_from & 0xff, count);
			ph = ph->next;
		}
		return 0;
	}
	if (sscanf(ci, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &e) == 5) {
		if (a > 255 || b > 255 || c > 255 || d > 255 || e > 32)
			return -1;
		ip_from = (a << 24) | (b << 16) | (c << 8) | d;
		ip_to = ip_from | (0xffffffff >> e);
	}
	else if (sscanf(ci, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
		if (a > 255 || b > 255 || c > 255 || d > 255)
			return -1;
		ip_to = ip_from = (a << 24) | (b << 16) | (c << 8) | d;
	} else return -1;
	while (isdigit(*ci)) ci++; ci++; // a
	while (isdigit(*ci)) ci++; ci++; // b
	while (isdigit(*ci)) ci++; ci++; // c
	while (isdigit(*ci)) ci++;       // d
	if (e < 33) {
		ci++;
		while (isdigit(*ci)) ci++;       // e
	}
		
	host_allow_t* p = (host_allow_t*) calloc(sizeof(host_allow_t),1);
	if (!p) return 1;
	p->ip_from = ip_from;
	p->ip_to = ip_to;
	p->next = NULL;

	// Do we have any on the list at all
	if (!m_host_allow) m_host_allow = p;

	// yes we have
	else {
		// should we insert it as the first
		if (ip_from < m_host_allow->ip_from) {
			p->next = m_host_allow;
			m_host_allow = p;
		} else {
			// we have to go through the list
			host_allow_t* ph = m_host_allow;
			while (ph) {
				// have we reach the end of the list next
				if (!ph->next) {
					ph->next = p;
					break;
				} else {
					if (ip_from < ph->next->ip_from) {
						p->next = ph->next;
						ph->next = p;
						break;
					}
				}
				ph = ph->next;
			}
		}
	}
	while (isspace(*ci)) ci++;
	return (*ci ? host_allow(ctr, ci) : 0);
}


int CController::list_help (struct controller_type *ctr, char* ci)
{
	if (!ci) return 1;
	//if (!ctr || !ci) return 1;
	if (!strcasecmp(ci, "help ")) {
		controller_write_msg (ctr,
			"MSG: Commands:\n"
			"MSG:  maxplaces [(strings | fonts | texts | images | "
				"imageplaces | shapes | shapeplaces | "
				"video feeds | virtual feeds | audio feeds | "
				"audio mixers | audio sinks) <number>\n"
			"MSG:  monitor [on | off]\n"
			"MSG:  overlay finish <command name>\n"
			"MSG:  overlay pre <command name>\n"
			"MSG:  png write <file name>\n"
			"MSG:  select <feed no>\n"
			"MSG:  stack <feed no> ....\n"
			"MSG:  stat\n"
		 	"MSG:  system geometry <width> <height> <fourcc>\n"
			"MSG:  system frame rate <frames/s>\n"
			"MSG:  system info\n"
			"MSG:  system socket <output socket name>\n"
			"MSG:  system host allow [(<network> | <host>)[(<network> | <host>)[ ...]]]\n"
			"MSG:  verbose\n"
			"MSG:  help  // prints this list\n"
			"MSG:  audio feed help  // to see additional help on audio feed\n"
			"MSG:  audio mixer help  // to see additional help on audio feed\n"
			"MSG:  audio sink help  // to see additional help on audio feed\n"
			"MSG:  command help  // to see additional help on command)\n"
			"MSG:  feed help  // to see additional help on feed\n"
			"MSG:  image help  // to see additional help on image\n"
			"MSG:  text help  // (to see additional help on text)\n"
			"MSG:  virtual feed help   // to see additional help on virtual feed\n"
			"MSG:  quit   // Quit snowmix. This clean up shm alloacted in /run/shm/\n"
			"MSG:  \n"
			);
	} else return -1;
	return 0;
}

/* list monitor status */
int CController::list_monitor (struct controller_type *ctr)
{
	if (m_monitor_status) controller_write_msg (ctr, "Monitor is on.\n");
	else controller_write_msg (ctr, "Monitor is off.\n");
	return 0;
}

/* set monitor on */
int CController::set_monitor_on (CVideoMixer* pVideoMixer, struct controller_type *ctr)
{
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for set_monitor_on\n");
		return -1;
	}
	if (!m_monitor_status) {
		if (pVideoMixer->m_video_type == VIDEO_NONE) {
			controller_write_msg (ctr, "Monitor can not be turned on as video type not defined.\n");
			return -1;
		}
		if (pVideoMixer->m_pMonitor) delete pVideoMixer->m_pMonitor;
		pVideoMixer->m_pMonitor = new CMonitor_SDL(pVideoMixer->m_geometry_width, pVideoMixer->m_geometry_height, pVideoMixer->m_video_type);
		if (!pVideoMixer->m_pMonitor) {
			controller_write_msg (ctr, "Failed to get monitor class. Monitor set to off\n");
			return -1;
		} else {
			m_monitor_status = true;
			controller_write_msg (ctr, "Monitor status set to on.\n");
		}
	} else {
		controller_write_msg (ctr, "Monitor status was already on. Turn it off to change.\n");
	}
	return 0;
}

/* set monitor off */
int CController::set_monitor_off (CVideoMixer* pVideoMixer, struct controller_type *ctr)
{
	if (m_monitor_status) {
		m_monitor_status = false;
		if (pVideoMixer->m_pMonitor) delete pVideoMixer->m_pMonitor;
		pVideoMixer->m_pMonitor = NULL;
		controller_write_msg (ctr, "Monitor status set to off.\n");
	} else {
		controller_write_msg (ctr, "Monitor was off. Turn it on to change.\n");
	}
	return 0;
}


/*
// image load [<id no> [<file name>]]  // empty string deletes entry
int CController::set_image_load (struct controller_type *ctr, CVideoMixer* pVideoMixer, char* ci)
{
	CVideoImage* pVideoImage = pVideoMixer->m_pVideoImage;
	if (strlen(ci) == strlen("image load ")) {
		if (pVideoImage) {
			int max_id = pVideoImage->MaxImages();
			for (int i=0; i < max_id; i++) {
				struct image_item_t* pImageItem = pVideoImage->GetImageItem(i);
				//controller_write_msg (ctr, "%d %s\n", i, pImageItem ? "ptr" : "NULL");
				if (pImageItem)
					controller_write_msg (ctr, "MSG: image load %d <%s> %ux%u "
						"bit depth %d type %s\n", i, pImageItem->file_name,
						pImageItem->width, pImageItem->height,
						(int)(pImageItem->bit_depth),
						pImageItem->color_type == PNG_COLOR_TYPE_RGB ? "RGB" :
						  (pImageItem->color_type == PNG_COLOR_TYPE_RGBA ? "RGBA" : "Unsupported"));
			}
		} else controller_write_msg (ctr, "MSG: No images loaded\n");
		//return 0;
	} else {
		char* str = (char*) calloc(1,strlen(ci)-10);
		if (!str) {
			controller_write_msg (ctr, "MSG: Failed to allocate memory for file name %s\n", str);
			return 0;
		}
		*str = '\0';
		int id;
		int x = sscanf(ci + strlen ("image load "), "%u %[^\n]", &id, str);
		if (x != 1 && x != 2) { free(str); return 1; }
		if (!pVideoMixer->m_pVideoImage) {
			pVideoMixer->m_pVideoImage = new CVideoImage( pVideoMixer,
				m_maxplaces_images, m_maxplaces_image_places);
			if (!pVideoMixer->m_pVideoImage) {
				controller_write_msg (ctr, "MSG: Failed to create class CVideoImages\n");
				free(str);
				return 0;
			}
		}
		trim_string(str);
		if (x == 1) pVideoMixer->m_pVideoImage->LoadImage(id, NULL);	// Deletes the entry
		else if (x == 2) pVideoMixer->m_pVideoImage->LoadImage(id, str);	// Adds the entry
		free(str);
	}
	return 0;
}

//image place [<id no> [<x> <y> [(left | center | right) (top | middle | bottom)] "
// image place [<id no> | <text_id> <font_id> <x> <y> [<red> <green> <blue>]]]
int CController::set_image_place (struct controller_type *ctr, CVideoMixer* pVideoMixer, char* ci)
{
	if (strlen(ci) == strlen("image place ")) {
		if (pVideoMixer->m_pVideoImage) {
			struct image_place_t* pPlacedImage = NULL;
			for (unsigned int id = 0; id < pVideoMixer->m_pVideoImage->MaxImagesPlace(); id++) {
				if ((pPlacedImage = pVideoMixer->m_pVideoImage->GetImagePlaced(id))) {
					controller_write_msg (ctr, "MSG: image place id %2d image_id %2d x,y %5d,%-5d %s\n",
					id, pPlacedImage->image_id, pPlacedImage->x, pPlacedImage->y,
					pPlacedImage->display ? "on" : "off");
				}
			}
		} else {
			controller_write_msg (ctr, "MSG: No image placed yet.\n");
		}
	} else {
		if (!pVideoMixer->m_pVideoImage) {
			controller_write_msg (ctr, "MSG: No image loaded yet hence no image to place\n");
			return 0;
		}
		u_int32_t place_id, image_id;
		int32_t x;
		int32_t y;
		u_int32_t align = IMAGE_ALIGN_LEFT | IMAGE_ALIGN_TOP;
		int n = sscanf(ci + strlen ("image place "), "%u %u %d %d", &place_id, &image_id, &x, &y);

		// Should we delete the entry or place it
		if (n == 1) pVideoMixer->m_pVideoImage->RemoveImagePlaced(place_id);
		else if (n == 4) {
			pVideoMixer->m_pVideoImage->SetImagePlace(place_id, image_id, x, y, align, true);

			char* str = ci + strlen("image place ");
			for (int i=0; i < 4; i++) {
				while (*str == ' ' || *str == '\t') str++;
				while (isdigit(*str)) str++;
			}
			while (*str == ' ' || *str == '\t') str++;
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
	 			pVideoMixer->m_pVideoImage->SetImageAlign(place_id, align);
				while (*str == ' ' || *str == '\t') str++;
			}
		} else return 1;
	}
	return 0;
}
*/

/* Initialize the output system. */
int CController::output_init_runtime (CVideoMixer* pVideoMixer, struct controller_type *ctr, char *path)
{
	int	no = 0;

	/* Create a shared memory region for talking to the output receivers. */
	pVideoMixer->m_output_memory_size = 50 * pVideoMixer->m_block_size;	/* Space for 50 buffers (should be more than enough). */

	/* Create the shared memory handle. */
	do {
		pid_t	pid = getpid ();
#ifdef HAVE_MACOSX
		sprintf (pVideoMixer->m_output_memory_handle_name, "/shmpipe.%d.%d", pid, no);
		pVideoMixer->m_output_memory_handle =
			shm_open (pVideoMixer->m_output_memory_handle_name,
				O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR | S_IRGRP);
#else
		sprintf (pVideoMixer->m_output_memory_handle_name, "/shmpipe.%5d.%5d", pid, no);
		pVideoMixer->m_output_memory_handle =
			shm_open (pVideoMixer->m_output_memory_handle_name,
				O_RDWR | O_CREAT | O_TRUNC | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP);
#endif
	} while (pVideoMixer->m_output_memory_handle < 0 && errno == EEXIST);
	if (pVideoMixer->m_output_memory_handle < 0) {
		fprintf (stderr, "Unable to open shared memory handle: name %s. Error : %s\n",
			pVideoMixer->m_output_memory_handle_name, strerror (errno));
		exit (1);
	}
	if (ftruncate (pVideoMixer->m_output_memory_handle,
		pVideoMixer->m_output_memory_size) < 0) {
		perror ("Unable to resize shm area.");
		exit (1);
	}

	pVideoMixer->m_pOutput_memory_base = (uint8_t*) mmap (NULL,
		pVideoMixer->m_output_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		pVideoMixer->m_output_memory_handle, 0);
	if (pVideoMixer->m_pOutput_memory_base == MAP_FAILED) {
		perror ("Unable to map area into memory.");
		exit (1);
	}

	/* Clear the area. */
	memset (pVideoMixer->m_pOutput_memory_base, 0x00, pVideoMixer->m_output_memory_size);

//fprintf(stderr, "Shm memset\n");
	/* Assign individual buffer pointers to the area. */
	{
		struct output_memory_list_type	*buf;
		int				offset;

		for (offset = 0; offset < pVideoMixer->m_output_memory_size; offset += pVideoMixer->m_block_size) {
			buf = (output_memory_list_type*) calloc (1, sizeof (*buf));
			buf->offset    = offset;
			buf->use       = 0;

			list_add_tail (pVideoMixer->m_pOutput_memory_list, buf);
		}
	}
//fprintf(stderr, "Assigned buffers\n");

	/* Create the output socket. */
	trim_string (path);
	pVideoMixer->m_pOutput_master_name = strdup (path);

	unlink (pVideoMixer->m_pOutput_master_name);
	pVideoMixer->m_output_master_socket = socket (AF_UNIX, SOCK_STREAM, 0);
	if (pVideoMixer->m_output_master_socket < 0) {
		controller_write_msg (ctr, "MSG: Output master socket create error: %s", strerror (errno));
		return -1;
	}

	/* Bind it. */
	{
		struct sockaddr_un addr;

		memset (&addr, 0, sizeof (addr));
		addr.sun_family = AF_UNIX;
		strncpy (addr.sun_path, pVideoMixer->m_pOutput_master_name, sizeof (addr.sun_path) - 1);
		if (bind (pVideoMixer->m_output_master_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
			controller_write_msg (ctr, "MSG: Output master socket bind error: %s", strerror (errno));
			return -1;
		}
	}

	/* Prepare to receive new connections. */
	if (listen (pVideoMixer->m_output_master_socket, 1) < 0) {
		controller_write_msg (ctr, "MSG: Output master socket listen error: %s", strerror (errno));
		return -1;
	}

	return 0;
}

/* Check if there is anything to do on a output socket. */
void CController::output_check_read (CVideoMixer* pVideoMixer, fd_set *rfds)
{
	if (pVideoMixer->m_output_client_socket >= 0 && FD_ISSET (pVideoMixer->m_output_client_socket, rfds)) {
		/* Got data. */
		struct shm_commandbuf_type	cb;
		int				x;

		x = read (pVideoMixer->m_output_client_socket, &cb, sizeof (cb));
		if (x < 0) {
			/* EOF or socket disconnect. */
			close (pVideoMixer->m_output_client_socket);
			pVideoMixer->m_output_client_socket = -1;
			output_reset (pVideoMixer);
		} else if (x > 0) {
			switch (cb.type) {
				case 4:	{	/* COMMAND_ACK_BUFFER. */
					if (0)
					fprintf (stderr, "ACK Buffer. Client  ID: %d  block: %4lu  offset: %9lu\n",
						 cb.area_id,
						 cb.payload.ack_buffer.offset/pVideoMixer->m_block_size,
						 cb.payload.ack_buffer.offset);

					/* Release the buffer. */
					output_ack_buffer (pVideoMixer, cb.area_id, cb.payload.ack_buffer.offset);

					break;
				}

				case 1:		/* COMMAND_NEW_SHM_AREA. */
				case 2:		/* COMMAND_CLOSE_SHM_AREA. */
				case 3: 	/* COMMAND_NEW_BUFFER. */
				default:
					fprintf (stderr, "Unknown/unexpected packet. Client  Code: %d\n",
						 cb.type);
					break;
			}
		}
	}

	if (pVideoMixer->m_output_master_socket >= 0 && FD_ISSET (pVideoMixer->m_output_master_socket, rfds)) {
		/* Got a new connection. */
		pVideoMixer->m_output_client_socket = accept (pVideoMixer->m_output_master_socket, NULL, NULL);
		if (pVideoMixer->m_output_client_socket >= 0) {

			gettimeofday(&pVideoMixer->m_start_master, NULL);
			/* Initialize it. */
			uint8_t				buffer [200];
			struct shm_commandbuf_type	*cb = (shm_commandbuf_type*)&buffer;
			int				x;

			/* Inform the client about which shared memory area to map. */
			memset (&buffer, 0, sizeof (buffer));
			cb->type = 1;
			cb->area_id = 1;
			cb->payload.new_shm_area.size = pVideoMixer->m_output_memory_size;
			cb->payload.new_shm_area.path_size = strlen (pVideoMixer->m_output_memory_handle_name) + 1;
			strcpy ((char *)&buffer [sizeof (*cb)], pVideoMixer->m_output_memory_handle_name);

// FIXME - MacOSX doesn't seem to have MSG_NOSIGNAL
// SG_NOSIGNAL (since Linux 2.2)
// Requests  not  to  send SIGPIPE on errors on stream oriented sockets when the other end breaks the
// connection.  The EPIPE error is still returned.
            
			x = send (pVideoMixer->m_output_client_socket, &buffer,
				  sizeof (*cb) + strlen (pVideoMixer->m_output_memory_handle_name) + 1, MSG_NOSIGNAL);
			if (x > 0) {
				//dump_cb (pVideoMixer->m_block_size, "Mixer", "Client", cb, sizeof (*cb) + strlen (pVideoMixer->m_output_memory_handle_name) + 1);
			} else {
				/* Output disappeared. */
				output_reset (pVideoMixer);
			}
		}
	}
}


/* Setup the output stack. */
int CController::output_set_stack (CVideoMixer* pVideoMixer, struct controller_type *ctr, char *str)
{
	int	new_stack [MAX_STACK];
	int	slot;
	int	x;
	int	id;
	char	stack_str [MAX_STACK * 5];

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in output_select_stack\n");
		return -1;
	}
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "m_pVideoFeed for pVideoMixer was NULL in "
			"output_select_stack\n");
		return -1;
	}

	/* Clear the buffer. */
	for (slot = 0; slot < MAX_STACK; slot++) {
		new_stack [slot] = -1;
	}
	strcpy (stack_str, "");

	/* Parse the input. */
	for (slot = 0; slot < MAX_STACK; slot++) {
		struct feed_type	*feed;

		/* Advance *str past any whitespace. */
		while (*str == ' ') str++;

		x = sscanf (str, "%d", &id);
		if (x != 1) break;

		/* Check if the feed is valid. */
		feed = pVideoMixer->m_pVideoFeed->FindFeed (id);
		if (feed == NULL) {
			controller_write_msg (ctr, "MSG: Unknown feed: %d\n", id);
			return -1;
		}
		if (slot == 0 && (feed->width != pVideoMixer->m_geometry_width || feed->height != pVideoMixer->m_geometry_height)) {
			controller_write_msg (ctr, "MSG: The bottom layer is not a fullscreen feed\n");
			return -1;
		}
		new_stack [slot] = id;
		sprintf (stack_str + strlen (stack_str), " %d", id);

		/* Advance *str past the value. */
		while (*str >= '0' && *str <= '9') str++;
	}

	if (new_stack [0] == -1) {
		/* Empty stack. */
		controller_write_msg (ctr, "MSG: Empty stackup.\n");
		return -1;
	}

	memcpy (pVideoMixer->m_output_stack, new_stack, sizeof (pVideoMixer->m_output_stack));

	/* Let everyone know that a new stack have been selected. */
	controller_write_msg (NULL, "Stack%s\n", stack_str);

	return 0;
}

/* Return a descriptor for a free frame buffer. */
struct output_memory_list_type* CController::output_get_free_block (CVideoMixer* pVideoMixer)
{
	struct output_memory_list_type		*buf;

	for (buf = pVideoMixer->m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		if (buf->use == 0) {
			buf->use++;	/* Increase use count and return the descriptor pointer. */
			return buf;
		}
	}

	fprintf (stderr, "Out of output buffer memory\n");
	return NULL;
}

/* Release the use count for a memory buffer. */
void CController::output_ack_buffer (CVideoMixer* pVideoMixer, int area_id, unsigned long offset)
{
	struct output_memory_list_type	*buf;

	for (buf = pVideoMixer->m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		if (buf->offset == offset) {
			buf->use--;		/* Found you. */
			return;
		}
	}

	fprintf (stderr, "Invalid free by output_ack_buffer. offset: %lu\n", offset);
	return;
}


/* Reset the output stream. */
void CController::output_reset (CVideoMixer* pVideoMixer)
{
	struct output_memory_list_type	*buf;

	close (pVideoMixer->m_output_client_socket);
	pVideoMixer->m_output_client_socket = -1;

	/* Clear the use count on all elements in the output_memory_list. */
	for (buf = pVideoMixer->m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		buf->use = 0;
	}
}

/* Initialize the feed subsystem. */
int CController::feed_init_basic (CVideoMixer* pVideoMixer, const int width, const int height, const char *pixelformat)
{
	//int	w_mod;
	//int	h_mod;
	//VideoType video_type = VIDEO_NONE;

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer for feed_init_basic was NULL.\n");
		return -1;
	}

	if (pVideoMixer->SetGeometry(width, height, pixelformat)) {
		fprintf(stderr, "Failed to set geometry %ux%u for VideoMixer pixelformat "
			"<%s>\n", width, height, pixelformat);
		return -1;
	}

	//system_geometry_width  = width;
	//system_geometry_height = height;
	//strncpy (system_pixelformat, pixelformat, sizeof (system_pixelformat) - 1);
	//system_block_size = system_pixel_bytes * system_geometry_width * system_geometry_height;

	// fprintf(stderr, "System geometry %dx%d pixel format string %s with %d bytes "
	// 	"per pixel %d bytes per frame.\n", system_geometry_width,
	// 	system_geometry_height, system_pixelformat, system_pixel_bytes,
	// 	system_block_size);

	/* Create the system feed. */
	m_pSystem_feed = (feed_type*) calloc (1, sizeof (*m_pSystem_feed));
	m_pSystem_feed->previous_state   = SETUP;
	m_pSystem_feed->state            = STALLED;		/* Is always marked stalled. */
	m_pSystem_feed->id               = 0;
	m_pSystem_feed->feed_name        = (char*) "Internal";
	m_pSystem_feed->oneshot          = 0;
	m_pSystem_feed->is_live          = 0;
	m_pSystem_feed->control_socket   = -1;	/* Does not have a control socket. */
	m_pSystem_feed->width            = width;
	m_pSystem_feed->height           = height;
	m_pSystem_feed->cut_start_column = 0;
	m_pSystem_feed->cut_start_row    = 0;
	m_pSystem_feed->cut_columns      = width;
	m_pSystem_feed->cut_rows         = height;
	m_pSystem_feed->shift_columns    = 0;
	m_pSystem_feed->shift_rows       = 0;
	m_pSystem_feed->par_w            = 1;
	m_pSystem_feed->par_h            = 1;
	m_pSystem_feed->scale_1          = 1;
	m_pSystem_feed->scale_2          = 1;

	/* Add the m_pSystem_feed to the list of feeds. */
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed == NULL in feed_init_basic.\n");
		return -1;
	}
	struct feed_type* pFeedList = pVideoMixer->m_pVideoFeed->GetFeedList();
	if (pFeedList) {
		//fprintf(stderr, "Adding to feed list\n");
		list_add_tail (pFeedList, m_pSystem_feed);
	}
	else {
		//fprintf(stderr, "Adding to empty feed list\n");
		pVideoMixer->m_pVideoFeed->SetFeedList(m_pSystem_feed);
	}

	return 0;
}
