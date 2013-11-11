/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include "snowmix.h"

#define MIN_SYSTEM_FRAME_RATE 1
#define MAX_SYSTEM_FRAME_RATE 100


struct controller_type {
	struct controller_type	*prev;
	struct controller_type	*next;

	int		read_fd;	// File descriptor for receiving commands.
	int		write_fd;	// File descriptor for transmitting
					// status/error messages.
	char		linebuf [2048];	// Buffer for retrieving commands.
	int		got_bytes;	// Number of bytes already read into linebuf.
	bool		is_audio;	// Using the connection for audio stream
	bool		close_controller;	// signal to close controller
	u_int32_t	feed_id;	// The feed the ctr will be associated with
	u_int32_t	sink_id;	// The feed the ctr will be associated with
};

struct host_allow_t {
	u_int32_t		ip_from;
	u_int32_t		ip_to;
	struct host_allow_t*	next;
};

void trim_string (char *str);

//#include "video_mixer.h"
//#include "command.h"
//#include "monitor.h"

class CVideoMixer;
class CCommand;
class CMonitor_SDL;

struct feed_type;


class CController {
  public:
	CController(char *init_file);
	~CController();
	int			controller_write_msg (struct controller_type *ctr,
					const char *format, ...);
	void			controller_parse_command (CVideoMixer* pVideoMixer,
					struct controller_type *ctr, char *line);
	char*			SetCommandName(const char* name);
	int			ParseCommandByName (CVideoMixer* pVideoMixer,
					controller_type* ctr, char* command);
	int			controller_set_fds (int maxfd, fd_set *rfds, fd_set *wfds);
	void			controller_check_read (CVideoMixer* pVideoMixer,
					fd_set *rfds);
	void			ControllerListDeleteElement (struct controller_type* ctr);
	struct output_memory_list_type*
				output_get_free_block (CVideoMixer* pVideoMixer);
	void			output_check_read (CVideoMixer* pVideoMixer, fd_set *rfds);
	void			output_reset (CVideoMixer* pVideoMixer);
	//struct feed_type*	find_feed (int id);

	//struct feed_type*	m_pFeed_list;
	struct feed_type*	m_pSystem_feed;
	bool			m_monitor_status;
	CMonitor_SDL*		m_pMonitor;
	CCommand*		m_pCommand;
	bool			m_verbose;
	u_int32_t		m_frame_seq_no;
	u_int32_t		m_lagged_frames;

  private:

	/* private functions */
	int	list_help(struct controller_type *ctr, char*ci);
	int	list_system_info(CVideoMixer* pVideoMixer, struct controller_type* ctr, const char* str);
	int	set_maxplaces (struct controller_type *ctr, char* ci);
	int	set_system_controller_port (struct controller_type *ctr, char* ci);
	int	require_version (struct controller_type *ctr, char* ci );
	int	list_monitor (struct controller_type *ctr);
	int	host_allow (struct controller_type *ctr, const char* ci);
	bool	host_not_allow (u_int32_t ip);
	int	set_monitor_on (CVideoMixer* pVideoMixer, struct controller_type *ctr);
	int	set_monitor_off (CVideoMixer* pVideoMixer, struct controller_type *ctr);


	int	output_init_runtime (CVideoMixer* pVideoMixer, struct controller_type *ctr,
			char *path);
	int	output_set_stack (CVideoMixer* pVideoMixer, struct controller_type *ctr,
			char *str);
	void	output_ack_buffer (CVideoMixer* pVideoMixer, int area_id, unsigned long offset);
	int	feed_init_basic (CVideoMixer* pVideoMixer, const int width, const int height, const char *pixelformat);

	/* Private vars */

	struct controller_type*	m_pController_list;
	int		m_controller_listener;
	host_allow_t*	m_host_allow;
	int		m_control_port;
	int		m_control_connections;

	u_int32_t	m_maxplaces_text;
	u_int32_t	m_maxplaces_font;
	u_int32_t	m_maxplaces_images;
	u_int32_t	m_maxplaces_virtual_feeds;
	u_int32_t	m_maxplaces_shapes;
        u_int32_t	m_maxplaces_shape_places;
        u_int32_t	m_maxplaces_patterns;
	u_int32_t	m_maxplaces_video_feeds;
	u_int32_t	m_maxplaces_audio_feeds;
	u_int32_t	m_maxplaces_audio_mixers;
	u_int32_t	m_maxplaces_audio_sinks;
	u_int32_t	m_maxplaces_text_places;
	u_int32_t	m_maxplaces_image_places;

	bool		m_broadcast;		// broadcast messages to all controllers
	char*		m_command_name;

};
#endif	// CONTROLLER_H
