/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#include <unistd.h>
#include <errno.h>
//#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
//#include <stdlib.h>
//#include <png.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include "cairo/cairo.h"
#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "audio_sink.h"

CAudioSink::CAudioSink(CVideoMixer* pVideoMixer, u_int32_t max_sinks)
{
	m_max_sinks = 0;
	m_sink_count = 0;
	m_sinks = (audio_sink_t**) calloc(
		sizeof(audio_sink_t*), max_sinks);
	//for (unsigned int i=0; i< m_max_sinks; i++) m_sinks[i] = NULL;
	if (m_sinks) {
		m_max_sinks = max_sinks;
	}
	m_pVideoMixer = pVideoMixer;
	m_verbose = 0;
	m_animation = 0;

	m_monitor_fd = -1; //open("monitor_samples", O_RDWR | O_TRUNC);
	//if (m_monitor_fd < 0) fprintf(stderr, "ERROR in opening monitor write file\n");
}

CAudioSink::~CAudioSink()
{
	if (m_sinks) {
		for (unsigned int id=0; id<m_max_sinks; id++) DeleteSink(id);
		free(m_sinks);
		m_sinks = NULL;
	}
	m_max_sinks = 0;
	if (m_monitor_fd > -1) close(m_monitor_fd);
}

// audio sink ....
int CAudioSink::ParseCommand(struct controller_type* ctr, const char* str)
{
	//if (!m_pVideoMixer) return 0;
	// audio sink status
	if (!strncasecmp (str, "status ", 7)) {
		return set_sink_status(ctr, str+7);
	}
	// audio sink info
	else if (!strncasecmp (str, "info ", 5)) {
		return list_info(ctr, str+5);
	}
	// audio sink add silence <sink id> <ms>]
	else if (!strncasecmp (str, "add silence ", 12)) {
		return set_add_silence(ctr, str+12);
	}
	// audio sink drop <sink id> <ms>]
	else if (!strncasecmp (str, "drop ", 5)) {
		return set_drop(ctr, str+5);
	}
	// audio sink add [<sink id> [<sink name>]]
	else if (!strncasecmp (str, "add ", 4)) {
		return set_sink_add(ctr, str+4);
	}
	// audio sink start [<sink id>]
	else if (!strncasecmp (str, "start ", 6)) {
		return set_sink_start(ctr, str+6);
	}
	// audio sink stop [<sink id>]
	else if (!strncasecmp (str, "stop ", 5)) {
		return set_sink_stop(ctr, str+5);
	}
	// audio sink source [(feed | mixer) <sink id> <source id>]
	else if (!strncasecmp (str, "source ", 7)) {
		return set_sink_source(ctr, str+7);
	}
	// audio sink channels [<sink id> <channels>]
	else if (!strncasecmp (str, "channels ", 9)) {
		return set_sink_channels(ctr, str+9);
	}
	// audio sink rate [<sink id> <rate>]
	else if (!strncasecmp (str, "rate ", 5)) {
		return set_sink_rate(ctr, str+5);
	}
	// audio sink format [<sink id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
	else if (!strncasecmp (str, "format ", 7)) {
		return set_sink_format(ctr, str+7);
	}
	// audio sink file <sink id> <file name>
	else if (!strncasecmp (str, "file ", 5)) {
		return set_file(ctr, str+5);
	}
	// audio sink queue maxdelay <sink id> <max delay in ms
	else if (!strncasecmp (str, "queue maxdelay ", 15)) {
		return set_queue_maxdelay(ctr, str+15);
	}
	// audio sink queue <sink id> 
	else if (!strncasecmp (str, "queue ", 6)) {
		return set_queue(ctr, str+6);
	}
	//// audio sink monitor (on | off) <sink id> 
	//else if (!strncasecmp (str, "monitor ", 8)) {
	//	return set_monitor(ctr, str+8);
	//}
	// audio sink ctr isaudio <sink id> // set control channel to write out
	else if (!strncasecmp (str, "ctr isaudio ", 12)) {
		return set_ctr_isaudio(ctr, str+12);
	}
	// audio sink volume [<sink id> <volume>] // volume = 0..255
	else if (!strncasecmp (str, "volume ", 7)) {
		return set_volume(ctr, str+7);
	}
        // audio sink move volume [<sink id> <delta volume> <steps>]
        else if (!strncasecmp (str, "move volume ", 12)) {
                return set_move_volume(ctr, str+12);
        }
	// audio sink mute [(on | off) <sink id>]
	else if (!strncasecmp (str, "mute ", 5)) {
		return set_mute(ctr, str+5);
	}
	// audio sink verbose [<verbose level>]
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose(ctr, str+8);
	}
	// audio sink help
	else if (!strncasecmp (str, "help ", 5)) {
		return list_help(ctr, str+5);
	}
	return 1;
}

// list audio sink help
int CAudioSink::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Commands:\n"
			MESSAGE"audio sink add [<sink id> [<sink name>]]\n"
			MESSAGE"audio sink add silence <sink id> <ms>\n"
			MESSAGE"audio sink channels [<sink id> <channels>]\n"
			MESSAGE"audio sink ctr isaudio <sink id> "
				"// set control channel to write out audio\n"
			MESSAGE"audio sink drop <sink id> <ms>]\n"
			MESSAGE"audio sink file <sink id> <file name>\n"
			MESSAGE"audio sink format [<sink id> (8 | 16 | 24 | 32 | 64) "
				"(signed | unsigned | float) ]\n"
			MESSAGE"audio sink info\n"
			MESSAGE"audio sink move volume [<sink id> <delta volume> <delta steps>]\n"
			MESSAGE"audio sink mute [(on | off) <sink id>]\n"
			MESSAGE"audio sink queue <sink id> \n"
			MESSAGE"audio sink queue maxdelay <sink id> <max delay in ms\n"
			MESSAGE"audio sink rate [<sink id> <rate>]\n"
			MESSAGE"audio sink source [(feed | mixer) <sink id> <source id>]\n"
			MESSAGE"audio sink start [<sink id>]\n"
			MESSAGE"audio sink status\n"
			MESSAGE"audio sink stop [<sink id>]\n"
			MESSAGE"audio sink verbose [<verbose level>]\n"
			MESSAGE"audio sink volume [<sink id> <volume>] // volume = 0..255\n"
			MESSAGE"audio sink help\n"
			MESSAGE"\n");
	return 0;
}


// audio sink info
int CAudioSink::list_info(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" audio sink info\n"
		STATUS" audio sinks       : %u\n"
		STATUS" max audio sinks   : %u\n"
		STATUS" verbose level     : %u\n",
		m_sink_count,
		m_max_sinks,
		m_verbose
		);
	if (m_sink_count) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" audio sink id : state, rate, channels, bytespersample, "
			"signess, volume, mute, buffersize, delay, queues\n");
		for (u_int32_t id=0; id < m_max_sinks; id++) {
			if (!m_sinks[id]) continue;
			u_int32_t queue_count = 0;
			audio_queue_t* pQueue = m_sinks[id]->pAudioQueues;
			while (pQueue) {
				queue_count++;
				pQueue = pQueue->pNext;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" - audio sink %u : %s, %u, %u, %u, %ssigned, %s", id,
				feed_state_string(m_sinks[id]->state),
				m_sinks[id]->rate,
				m_sinks[id]->channels,
				m_sinks[id]->bytespersample,
				m_sinks[id]->is_signed ? "" : "un",
				m_sinks[id]->pVolume ? "" : "0,");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++)
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f,", *p);
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, " %smuted, %u, %u, %u\n",
				m_sinks[id]->mute ? "" : "un",
				m_sinks[id]->buffer_size,
				m_sinks[id]->delay,
				queue_count);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}
// audio sink verbose [<level>] // set control channel to write out
int CAudioSink::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"audio sink verbose %s\n"STATUS"\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"audio sink verbose level set to %u for CAudioSink\n", m_verbose);
	return 0;
}

// audio sink status
int CAudioSink::set_sink_status(struct controller_type* ctr, const char* str)
{
	//u_int32_t id;
	//int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"sink_id : state samples samplespersecond avg_samplespersecond silence dropped clipped rms");
		for (unsigned id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
			audio_queue_t* pQueue = m_sinks[id]->pAudioQueue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"\n"STATUS"audio sink %u : %s %u %.0lf %.0lf %u %u %u %s",
				id, feed_state_string(m_sinks[id]->state),
				m_sinks[id]->samples,
				m_sinks[id]->samplespersecond,
				m_sinks[id]->total_avg_sps/MAX_AVERAGE_SAMPLES,
				m_sinks[id]->silence,
				m_sinks[id]->dropped,
				m_sinks[id]->clipped,
                                pQueue ? "" : "0 ");
                        while (pQueue) {
                                m_pVideoMixer->m_pController->controller_write_msg (ctr, "%u%s",
                                        1000*pQueue->sample_count/m_sinks[id]->calcsamplespersecond,
                                        pQueue->pNext ? "," : " ");
                                pQueue = pQueue->pNext;
                        }

			for (u_int32_t i=0; i < m_sinks[id]->channels; i++) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%.1lf%s", (double) (sqrt(m_sinks[id]->rms[i])/327.67),
					i+1 < m_sinks[id]->channels ? "," : "");
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"\n"STATUS"\n");
		return 0;

	}
	return -1;
}

// audio sink ctr isaudio <sink id> // set control channel to write out
int CAudioSink::set_ctr_isaudio(struct controller_type* ctr, const char* str)
{

	u_int32_t id;
	int n;
	// make some checks
	if (!str || !ctr) return -1;
	if (!m_sinks || ctr->write_fd == -1) return 1;

	// skip whitespace and find the sink id
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &id) != 1 || id < 1 || !m_sinks[id]) return -1;
	ctr->is_audio = true;
	ctr->sink_id = id;
	m_sinks[id]->sink_fd = ctr->write_fd;
	if (m_verbose) fprintf(stderr,
		"Frame %u - controller output changed to audio sink %u. Starting audio sink %u\n",
				system_frame_no, id, id);
	if ((n = StartSink(id))) {
		if (m_verbose) fprintf(stderr, "audio sink %u failed to start sink\n", id);
		ctr->is_audio = false;
		ctr->sink_id = id;
		m_sinks[id]->sink_fd = -1;
		return n;
	}
	//gettimeofday(&(m_sinks[id]->start_time), NULL);
	//SetSinkBufferSize(id);
	return 0;
}

// audio sink queue maxdelay <sink id> <max delay>
int CAudioSink::set_queue_maxdelay(struct controller_type* ctr, const char* str)
{
	u_int32_t id, maxdelay;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned id=0 ; id < m_max_sinks; id++) {
			
			if (m_sinks[id] && m_sinks[id]->pAudioQueue) {
				//u_int32_t bytespersecond = m_sinks[id]->bytespersample*
				//	m_sinks[id]->channels*m_sinks[id]->rate;
				u_int32_t n_total = sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0])-1;
				int32_t total = m_sinks[id]->queue_samples[n_total];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %2u queue : average buffer bytes %d = %d ms\n"
					"MSG:                     : current buffer bytes %u = %u ms\n"
					"MSG:                     : buffers %u\n"
					"MSG:                     : maxdelay set to %u bytes = %u ms\n",
					id, total/n_total, (1000*total)/(n_total*m_sinks[id]->bytespersecond),
					m_sinks[id]->pAudioQueue->bytes_count,
					(1000*m_sinks[id]->pAudioQueue->bytes_count)/m_sinks[id]->bytespersecond,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->max_bytes,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// Get the sink id.
	if (sscanf(str, "%u %u", &id, &maxdelay) != 2) return -1;
	//maxdelay = maxdelay*m_sinks[id]->bytespersample*m_sinks[id]->channels*m_sinks[id]->rate/1000;
	maxdelay = maxdelay*m_sinks[id]->bytespersecond/1000;
	return SetQueueMaxDelay(id, maxdelay);
}

// audio sink queue <sink id> 
int CAudioSink::set_queue(struct controller_type* ctr, const char* str)
{
	u_int32_t id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (id=0 ; id < m_max_sinks; id++) {
			
			if (m_sinks[id] && m_sinks[id]->pAudioQueue) {
				//u_int32_t bytespersecond = m_sinks[id]->bytespersample*
			//		m_sinks[id]->channels*m_sinks[id]->rate;
				u_int32_t n_total = sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0])-1;
				int32_t total = m_sinks[id]->queue_samples[n_total];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %2u queue : average buffer bytes %d = %d ms\n"
					"MSG:                     : current buffer bytes %u = %u ms\n"
					"MSG:                     : buffers %u\n"
					"MSG:                     : maxdelay set to %u bytes = %u ms\n",
					id, total/n_total, (1000*total)/(n_total*m_sinks[id]->bytespersecond),
					m_sinks[id]->pAudioQueue->bytes_count,
					(1000*m_sinks[id]->pAudioQueue->bytes_count)/m_sinks[id]->bytespersecond,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->max_bytes,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond);
				/*m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %u queue : buffers %u bytes %u maxdelay "
					"%u ms current delay %u ms\n", id,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->bytes_count,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond,
					1000 * m_sinks[id]->pAudioQueue->bytes_count / m_sinks[id]->bytespersecond);
*/
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// Get the sink id.
	if (scanf(str, "on %u", &id) != 1) return -1;

	// Do something
	return 0;
}

/*
// audio sink monitor (on | off) <sink id> 
int CAudioSink::set_monitor(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Audio monitor\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "on %u", &id)) != 1) return -1;
fprintf(stderr, "Monitor on\n");
	m_sinks[id]->monitor_queue = AddQueue(id);
	if (m_sinks[id]->monitor_queue) {
		MonitorSink(id);
		SDL_PauseAudio(0);
	} else fprintf(stderr, "Failed to set audio monitor\n");
	return m_sinks[id]->monitor_queue ? 0 : 1;
}
*/

// audio sink file [<sink id> <file name>]
int CAudioSink::set_file(struct controller_type* ctr, const char* str)
{
	u_int32_t id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (unsigned n=0 ; n < m_max_sinks; n++)
			if (m_sinks[n] && m_sinks[n]->file_name)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: audio sink %2d file name %s\n", n, m_sinks[n]->file_name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;

	}
	// Get the sink id.
	if (sscanf(str, "%u", &id) != 1 || id < 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	return SetFile(id, str);
}

// audio sink move volume [<sink_id> <delta volume> <steps>]
int CAudioSink::set_move_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, steps;
	float volume_delta;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) {
			if (!m_sinks[id]) continue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio sink %2d volume %s", id,
				m_sinks[id]->pVolume ? "" : "0");
			if (m_sinks[id]->pVolume) {
				float* p = m_sinks[id]->pVolume;
				for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%smuted delta %.4f steps %u\n",
				m_sinks[id]->mute ? " " : " un",
				m_sinks[id]->volume_delta, m_sinks[id]->volume_delta_steps);
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id.
	if ((n = sscanf(str, "%u %f %u", &id, &volume_delta, &steps)) == 3) {
		n = SetMoveVolume(id, volume_delta, steps);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u delta volume %.4f steps %u\n",
				id, volume_delta, steps);
		return n;
	}
	else return -1;
}

// audio sink volume <sink id> <volume>
int CAudioSink::set_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channel;
	float volume;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
                        if (!m_sinks[id]) continue;
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                STATUS"audio sink %2d volume %s", id,
                                m_sinks[id]->pVolume ? "" : "0");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_sinks[id]->mute ? " muted" : "");
                }
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id.
        if ((n = sscanf(str, "%u", &id)) == 1) {
                while (*str && !isspace(*str)) str++;
                while (isspace(*str)) str++;
                const char* volume_list = str;
                channel=0;
                if (isdigit(*str) || (*str == '-' && isspace(str[1]))) {
                        while (*str) {
                                if ((*str == '-' && (isspace(str[1]) || !str[1]))) {
                                        str++;
                                        channel++;
                                        while (isspace(*str)) str++;
                                        continue;
                                }
                                if ((n = sscanf(str, "%f", &volume)) == 1) {
                                        if ((n=SetVolume(id,channel++,volume))) return n;
                                        while (*str && !isspace(*str)) str++;
                                        while (isspace(*str)) str++;
                                } else return -1;
                        }
                } else return -1;
                if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                MESSAGE"audio sink %u volume %s\n", id, volume_list);
                return 0;
        }
        else return -1;
}

// audio sink mute [(on | off) <sink id>]
int CAudioSink::set_mute(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
                        if (!m_sinks[id]) continue;
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                STATUS"audio sink %2d volume %s", id,
                                m_sinks[id]->pVolume ? "" : "0");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_sinks[id]->mute ? " muted" : "");
                }
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id and mute state
	if ((n = sscanf(str, "on %u", &id)) == 1) {
		n = SetMute(id, true);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u mute\n", id);
		return n;
	}
	else if ((n = sscanf(str, "off %u", &id)) == 1) {
		n = SetMute(id, false);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u unmute\n", id);
		return n;
	}
	else return -1;
}

// audio sink add silence <sink id> <ms>
int CAudioSink::set_add_silence(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the sink id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_sinks && m_sinks[id] && (m_sinks[id]->state == RUNNING ||
			m_sinks[id]->state == STALLED)) {
				if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						MESSAGE"Frame %u - audio sink %u adding %u ms of silence per channel\n",
						system_frame_no, id, ms);
			m_sinks[id]->write_silence_bytes = ms * m_sinks[id]->bytespersample *
				m_sinks[id]->channels*m_sinks[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio sink drop output <sink id> <ms>
int CAudioSink::set_drop(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the sink id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_sinks && m_sinks[id] && (m_sinks[id]->state == RUNNING ||
			m_sinks[id]->state == STALLED)) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"Frame %u - audio sink %u dropping %u ms of silence per channel\n",
					system_frame_no, id, ms);
			m_sinks[id]->drop_bytes = ms * sizeof(int32_t) *
				m_sinks[id]->channels * m_sinks[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio sink source [(feed | mixer) <sink id> <source id>]
int CAudioSink::set_sink_source(struct controller_type* ctr, const char* str)
{
	u_int32_t sink_id, source_id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (u_int32_t id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %u source %u\n", id,
					m_sinks[id]->source_id);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG:\n");
		return 0;

	}
	int n = -1;
	// Get the sink id and source
	if (sscanf(str, "feed %u %u", &sink_id, &source_id) == 2) {
		n = SinkSource(sink_id, source_id, 1);
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sourced by audio feed %u\n",
				sink_id, source_id);
	} else if (sscanf(str, "mixer %u %u", &sink_id, &source_id) == 2) {
		n = SinkSource(sink_id, source_id, 2);
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sourced by audio mixer %u\n",
				sink_id, source_id);
	}
	return n;
}


// audio sink stop [<sink id>] // Stop a sink
int CAudioSink::set_sink_stop(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %u state %s\n", id,
					m_sinks[id]->state == RUNNING ? "running" :
					  m_sinks[id]->state == PENDING ? "pending" :
					    m_sinks[id]->state == READY ? "ready" :
					      m_sinks[id]->state == SETUP ? "setup" :
					        m_sinks[id]->state == STALLED ? "stalled" :
					          m_sinks[id]->state == DISCONNECTED ? "disconnected" :
						    "unknown");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG:\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	return StopSink(id);
}

// audio sink start [<sink id>] // Start a sink
int CAudioSink::set_sink_start(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: audio sink %u state %s\n", id,
					m_sinks[id]->state == RUNNING ? "running" :
					  m_sinks[id]->state == PENDING ? "pending" :
					    m_sinks[id]->state == READY ? "ready" :
					      m_sinks[id]->state == SETUP ? "setup" :
					        m_sinks[id]->state == STALLED ? "stalled" :
					          m_sinks[id]->state == DISCONNECTED ? "disconnected" :
						    "unknown");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG:\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	return StartSink(id);
}

// audio sink add [<sink id> [<sink name>]] // Create/delete/list an audio sink
int CAudioSink::set_sink_add(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u <%s>\n", id,
					m_sinks[id]->name ? m_sinks[id]->name : "");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	while (isdigit(*str)) str++; while (isspace(*str)) str++;

	// Now if eos, only id was given and we delete the sink
	if (!(*str)) {
		if ((n = DeleteSink(id))) return n;
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u deleted\n");
	} else {

		// Now we know there is more after the id and we can create the feed
		if ((n = CreateSink(id))) return n;
		m_sink_count++;
		if ((n = SetSinkName(id, str))) {
			DeleteSink(id);
			return n;
		}
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u <%s> added\n", id, m_sinks[id]->name);
	}
	return n;
}

// audio sink channels [<sink id> <channels>]] // Set number of channels for an audio sink
int CAudioSink::set_sink_channels(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channels;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list channels
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u channels %u\n", id,
					m_sinks[id]->channels);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id and channels
	if ((n = sscanf(str, "%u %u", &id, &channels)) != 2) return -1;
	if (!(n = SetSinkChannels(id, channels)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u channels %u\n", id, channels);
	return n;
}

// audio sink rate [<sink id> <rate in Hz>]] // Set sample rate for an audio sink
int CAudioSink::set_sink_rate(struct controller_type* ctr, const char* str)
{
	u_int32_t id, rate;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u rate %u Hz\n", id,
					m_sinks[id]->rate);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id and rate
	if ((n = sscanf(str, "%u %u", &id, &rate)) != 2) return -1;
	if (!(n = SetSinkRate(id, rate)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u rate %u\n", id, rate);
	return n;
}

// audio sink format [<sink id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
// Set format for audio sink
int CAudioSink::set_sink_format(struct controller_type* ctr, const char* str)
{
	u_int32_t id, bits;
	int n;
	bool is_signed = false;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u %s and %u bytes per "
					"sample\n", id,
					m_sinks[id]->is_signed ? "signed" : "unsigned",
					m_sinks[id]->bytespersample);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id.
	n = sscanf(str, "%u %u", &id, &bits);
	if (n != 2) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (strncasecmp(str, "signed ", 8) == 0) {
		is_signed = true;
		if (bits != 8 && bits != 16 && bits != 24) return -1;
	} else if (strncasecmp(str, "unsigned ", 10) == 0) {
		is_signed = false;
		if (bits != 8 && bits != 16 && bits != 24) return -1;
	} else if (strncasecmp(str, "float ", 6) == 0) {
		is_signed = true;
		if (bits != 32 && bits != 64) return -1;
	} else return -1;
	if (!(n = SetSinkFormat(id, bits >> 3, is_signed)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sample is %u bytes per "
				"sample and %s\n", id, bits >> 3, is_signed ?
				"signed" : "unsigned");
	return n;
}


int CAudioSink::CreateSink(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks) return -1;
	audio_sink_t* p = m_sinks[id];
	if (!p) p = (audio_sink_t*) calloc(sizeof(audio_sink_t),1);
	if (!p) return -1;
	m_sinks[id]		= p;
	p->name			= NULL;
	p->id			= id;
	p->state		= SETUP;
	p->previous_state	= SETUP;
	p->channels		= 0;
	p->rate			= 0;
	p->is_signed		= true;
	p->bytespersample	= 0;
	p->bytespersecond	= 0;
	p->calcsamplespersecond	= 0;
	p->mute			= true;
	p->delay		= 0;
	p->start_time.tv_sec	= 0;
	p->start_time.tv_usec	= 0;
	p->buffer_size		= 0;
	p->pAudioQueues		= NULL;
	p->monitor_queue	= NULL;
	p->buf_seq_no		= 0;
	p->pVolume		= NULL;
	p->file_name		= NULL;
	p->sink_fd		= -1;
	p->sample_count		= 0;
	p->write_silence_bytes	= 0;
	p->drop_bytes		= 0;
	p->no_rate_limit	= true;

	p->samples		= 0;
	p->silence		= 0;
	p->dropped		= 0;
	p->clipped		= 0;
	p->rms			= NULL;


	p->source_type		= 0;
	p->source_id		= 0;
	p->pAudioQueue		= NULL;

	p->update_time.tv_sec   = 0;
        p->update_time.tv_usec  = 0;
        p->samplespersecond     = 0.0;
        p->last_samples         = 0;
        p->total_avg_sps        = 0.0;
        for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
                p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index        = 0;

	
	return 0;
}

// Delete a sink and allocated memory associated.
int CAudioSink::DeleteSink(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING) return -1;
	m_sink_count--;
	if (m_sinks[id]->name) free(m_sinks[id]->name);
	if (m_sinks[id]->file_name) free(m_sinks[id]->file_name);
        if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
        if (m_sinks[id]->rms) free(m_sinks[id]->rms);
	if (m_sinks[id]) free(m_sinks[id]);
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	m_sinks[id] = NULL;
	if (m_verbose) fprintf(stderr, "Deleted audio sink %u\n",id);
	return 0;
}

int CAudioSink::SetSinkName(u_int32_t id, const char* sink_name)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !sink_name)
		return -1;
	m_sinks[id]->name = strdup(sink_name);
	trim_string(m_sinks[id]->name);
	return m_sinks[id]->name ? 0 : -1;
}

int CAudioSink::SetSinkChannels(u_int32_t id, u_int32_t channels)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->channels = channels;
	if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
	if (m_sinks[id]->rms) free(m_sinks[id]->rms);
        m_sinks[id]->pVolume = (float*) calloc(channels,sizeof(float));
	m_sinks[id]->rms = (u_int64_t*) calloc(channels,sizeof(u_int64_t));
        if (!m_sinks[id]->pVolume || !m_sinks[id]->rms) {
                if (m_sinks[id]->rms) free(m_sinks[id]->rms);
                if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
                m_sinks[id]->rms = NULL;
		m_sinks[id]->pVolume = NULL;
                return 1;
        }
        for (u_int32_t i=0 ; i < channels ; i++) m_sinks[id]->pVolume[i] = DEFAULT_VOLUME_FLOAT;


	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkRate(u_int32_t id, u_int32_t rate)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->rate = rate;
	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkFormat(u_int32_t id, u_int32_t bytes, bool is_signed)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->bytespersample = bytes;
	//m_sinks[id]->bytespersecond = bytes*m_sinks[id]->channels*m_sinks[id]->rate;
	m_sinks[id]->is_signed = is_signed;
	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkBufferSize(u_int32_t id)
{
	// make some checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_pVideoMixer ||
		m_pVideoMixer->m_frame_rate == 0 || m_sinks[id]->channels < 1 ||
		m_sinks[id]->bytespersample < 1 || m_sinks[id]->rate < 1) {
		m_sinks[id]->buffer_size = 0;
		m_sinks[id]->state = SETUP;
		return -1;
	}

	m_sinks[id]->calcsamplespersecond = m_sinks[id]->channels*m_sinks[id]->rate;
	m_sinks[id]->bytespersecond = m_sinks[id]->bytespersample*
		m_sinks[id]->calcsamplespersecond;
	int32_t size = m_sinks[id]->channels*m_sinks[id]->bytespersample*
		m_sinks[id]->rate/ m_pVideoMixer->m_frame_rate;
	int32_t n = 0;
	for (n=0 ; size ; n++) size >>=  1;
	m_sinks[id]->buffer_size = (1 << n);
	if (m_verbose) fprintf(stderr, "audio sink %u buffersize set to %u bytes\n",
		id, m_sinks[id]->buffer_size);
	if (m_sinks[id]->buffer_size > 0) {
		if (m_verbose && m_sinks[id]->state != READY)
			fprintf(stderr, "audio sink %u changed state from %s to %s\n",
				id, feed_state_string(m_sinks[id]->state),
				feed_state_string(READY));
		m_sinks[id]->state = READY;
	} else {
		if (m_sinks[id]->state != SETUP && m_verbose)
			fprintf(stderr, "audio sink %u changed state from %s to %s\n",
				id, feed_state_string(m_sinks[id]->state),
				feed_state_string(SETUP));
		m_sinks[id]->state = SETUP;
	}
	return m_sinks[id]->buffer_size;
}

audio_buffer_t* CAudioSink::GetEmptyBuffer(u_int32_t id, u_int32_t* size)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_sinks[id]->buffer_size)
		return NULL;
	audio_buffer_t* p = (audio_buffer_t*)
		malloc(sizeof(audio_buffer_t)+m_sinks[id]->buffer_size);
	if (p) {
		if (size) *size = m_sinks[id]->buffer_size;
		p->data = ((u_int8_t*)p)+sizeof(audio_buffer_t);
		p->len = 0;
		p->next = NULL;
//fprintf(stderr, "Get empty buffer <%lu> data <%lu>\n", (u_int64_t) p, (u_int64_t) p->data);
	}
	return p;
}

// Set volume level for sink
int CAudioSink::SetMoveVolume(u_int32_t sink_id, float volume_delta, u_int32_t steps)
{
	if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id]) return -1;
	m_sinks[sink_id]->volume_delta = volume_delta;
	m_sinks[sink_id]->volume_delta_steps = steps;
	if (m_animation < steps) m_animation = steps;
	return 0;
}

// Set volume level for sink
int CAudioSink::SetVolume(u_int32_t sink_id, u_int32_t channel, float volume)
{
        if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id] ||
                channel >= m_sinks[sink_id]->channels) return -1;
        if (volume < 0.0) volume = 0.0;
        if (volume > MAX_VOLUME_FLOAT) volume = MAX_VOLUME_FLOAT;
        m_sinks[sink_id]->pVolume[channel] = volume;
        return 0;
}
/*
int CAudioSink::SetVolume(u_int32_t id, u_int32_t volume)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return -1;
	// Set volume. Legal value is between 0-255
	m_sinks[id]->volume = volume > 255 ? 255 : volume;
	return 0;
}
*/

int CAudioSink::SetFile(u_int32_t id, const char* file_name)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state == RUNNING)
		return -1;
	// Close fd if open
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	m_sinks[id]->sink_fd = -1;
	// Free name if any
	if (m_sinks[id]->file_name) free(m_sinks[id]->file_name);
	// copy new name
	m_sinks[id]->file_name = strdup(file_name);
	trim_string(m_sinks[id]->file_name);
	if (!m_sinks[id]->file_name) return 1;
	SetSinkBufferSize(id);
	if (m_verbose) fprintf(stderr, "audio sink %u will write to file <%s>\n",
		id, m_sinks[id]->file_name);
	return 0;
}
int CAudioSink::SetQueueMaxDelay(u_int32_t id, u_int32_t maxdelay)
{
	// make checks
fprintf(stderr, "SINK %u delay %u\n", id, maxdelay);
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_sinks[id]->pAudioQueue)
		return -1;
	m_sinks[id]->pAudioQueue->max_bytes = maxdelay;
	return 0;
}


// Calculate samples lacking using start_time and sample_count.
int32_t CAudioSink::SamplesNeeded(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state != RUNNING)
		return -1;
	struct timeval timenow, timediff;
	if (gettimeofday(&timenow,NULL)) return -1;
	timersub(&timenow, &m_sinks[id]->start_time, &timediff);
	u_int32_t samples = timediff.tv_sec*m_sinks[id]->rate +
		m_sinks[id]->rate * (timediff.tv_usec/1000000.0);
	if (samples < m_sinks[id]->sample_count) {
		fprintf(stderr, "audio sink %u samples %u less than sample count %u\n",
			id, samples, m_sinks[id]->sample_count);
		return 0;
		// We may have wrapping - FIXME
	}
	if (m_verbose > 2) fprintf(stderr,
		"audio sink %u samples %u current sample count %u lacking %u\n", id,
		samples, m_sinks[id]->sample_count, samples - m_sinks[id]->sample_count);
	return samples - m_sinks[id]->sample_count;
}

int CAudioSink::SinkSource(u_int32_t sink_id, u_int32_t source_id, int source_type)
{
	// make checks
	if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id] || !m_pVideoMixer)
		return -1;

	// Now if type is 1 - audio feed
	if (source_type == 1 || source_type == 2) {
		if (!m_pVideoMixer->m_pAudioFeed ||
			source_id >= m_pVideoMixer->m_pAudioFeed->MaxFeeds()) return -1;

		if (m_sinks[sink_id]->pAudioQueue) {
			if (m_verbose) fprintf(stderr,
				"Frame %u - audio sink %u dropping sourcing by audio %s %u\n",
				system_frame_no, sink_id,
				m_sinks[sink_id]->source_type == 1 ? "feed" : "mixer",
				m_sinks[sink_id]->source_id);

			// Remove the current queue to audio feed or mixer
			if (m_sinks[sink_id]->source_type == 1)
				m_pVideoMixer->m_pAudioFeed->RemoveQueue(m_sinks[sink_id]->
					source_id, m_sinks[sink_id]->pAudioQueue);
			else if (m_sinks[sink_id]->source_type == 2)
				m_pVideoMixer->m_pAudioMixer->RemoveQueue(m_sinks[sink_id]->
					source_id, m_sinks[sink_id]->pAudioQueue);
			if (m_sinks[sink_id]->state == RUNNING ||
				m_sinks[sink_id]->state == STALLED) {
				m_sinks[sink_id]->state = PENDING;
				if (m_verbose) fprintf(stderr,
					"Frame %u - audio sink %u changed state to PENDING\n",
					system_frame_no, sink_id);
					
			}
		}
/*
		// Adding an audio queue to audio feed (audio feed will start fill it
		// if something is feeding it.
		if (!(m_sinks[sink_id]->pAudioQueue =
			m_pVideoMixer->m_pAudioFeed->AddQueue(source_id))) {
			if (m_verbose) fprintf(stderr,
				"Failed to add source queue from audio feed %u "
				"to audio sink %u\n", source_id, sink_id);
			m_sinks[sink_id]->source_id = 0;
			return -1;
		}
*/
		m_sinks[sink_id]->source_id = source_id;
		m_sinks[sink_id]->source_type = source_type;
	} else return -1;

/*
	// check if sink was started
	if (m_sinks[id]->sink_fd > -1) {
		m_sinks[id]->state = PENDING;
	}
*/
	return 0;
}

int CAudioSink::StartSink(u_int32_t id)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state != READY)
		return -1;

	audio_sink_t* p = m_sinks[id];

	// Now lets see what we should start
	if (p->file_name && *p->file_name) {
		// Close if open
		if (p->sink_fd > -1) close(p->sink_fd);
		// Open file
		p->sink_fd = open(p->file_name, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRWXU);
		if (p->sink_fd < 0) return 1;
		else if (m_verbose) fprintf(stderr, "audio sink %u opened file <%sd> for writing\n",
			id, p->file_name);



	// Check if fd is already open
	} else if (m_verbose) fprintf(stderr, "Frame %u - audio sink %u sink fd was already opened\n",
		system_frame_no, id);

	// If sink_fd is -1, we assume we have a dummy sink
/*
	if (p->sink_fd < 0) {
		fprintf(stderr, "Frame %u - audio sink %u sink opening fd failed\n",
			system_frame_no, id);
		return -1;
	}
*/

	// Set fd to non blocking
	if (p->sink_fd >= 0 && set_fd_nonblocking(p->sink_fd)) {
		fprintf(stderr, "audio sink %u failed to set file <%s> to non blocking\n",
			id, p->file_name);
		close(p->sink_fd);
		p->sink_fd = -1;
		return 1;
	}
	if (p->sink_fd >= 0 && m_verbose) fprintf(stderr, "Frame %u - audio sink %u fd %d set to nonblock\n",
		system_frame_no, id, p->sink_fd);

	// Adding an audio queue to audio feed (audio feed will start fill it
	// if something is feeding it.
	if (p->source_type == 1 || p->source_type == 2) {
		if (p->source_type == 1)
			p->pAudioQueue = m_pVideoMixer->m_pAudioFeed->
				AddQueue(p->source_id);
		else p->pAudioQueue = m_pVideoMixer->m_pAudioMixer->
			AddQueue(p->source_id);
		if (!(p->pAudioQueue)) {
			if (m_verbose) fprintf(stderr,
				"Failed to add source queue from audio %s %u "
				"to audio sink %u\n",
				p->source_type == 1 ? "feed" : "mixer",
				p->source_id, id);
			close(p->sink_fd);
			p->sink_fd = -1;
			return -1;
		}
		if (m_verbose) {
			fprintf(stderr, "Frame %u - audio sink %u added audio queue to "
				"audio %s %u\n", system_frame_no, id,
				p->source_type == 1 ? "feed" : "mixer",
				p->source_id);
			fprintf(stderr,
				"Frame %u - audio sink %u started. Changing state from %s to %s\n",
				system_frame_no, id, feed_state_string(p->state),
				feed_state_string(PENDING));
		}
		p->state = PENDING;
	} else if (m_verbose)
			fprintf(stderr, "audio sink %u started but has no audio queue source\n", id);

	// Reset counter
	p->sample_count = 0;
	p->samples	= 0;
	p->silence	= 0;
	p->dropped	= 0;
	p->clipped	= 0;

	p->start_time.tv_sec           = 0;
        p->start_time.tv_usec          = 0;
	p->update_time.tv_sec           = 0;
        p->update_time.tv_usec          = 0;
        p->samplespersecond             = 0.0;
        p->last_samples                 = 0;
        p->total_avg_sps                = 0.0;
        for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
                p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index                = 0;

	return 0;
}

// Returns the number of channels for sink id
u_int32_t CAudioSink::GetChannels(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->channels);
}

// Returns the sample rate for sink id
u_int32_t CAudioSink::GetRate(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->rate);
}

// Returns the number bytes per sample for sink id
u_int32_t CAudioSink::GetBytesPerSample(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->bytespersample);
}


// Get Audio Sink state
feed_state_enum CAudioSink::GetState(u_int32_t id) {
        if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return UNDEFINED;
        return m_sinks[id]->state;
}


int CAudioSink::StopSink(u_int32_t id)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || (
	m_sinks[id]->state != RUNNING &&
	m_sinks[id]->state != PENDING &&
	m_sinks[id]->state != STALLED)) return -1;
	// Should be true, but lets make a check anyway;
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	if (m_sinks[id]->source_type == 1)
		m_pVideoMixer->m_pAudioFeed->RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pAudioQueue);
	else if (m_sinks[id]->source_type == 2)
		m_pVideoMixer->m_pAudioMixer->RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pAudioQueue);
	else fprintf(stderr, "audio sink %u can not recognize source type while removing queue\n", id);
	m_sinks[id]->pAudioQueue = NULL;
	// We don't check on close.
	m_sinks[id]->start_time.tv_sec = m_sinks[id]->start_time.tv_usec = 0;
	m_sinks[id]->sink_fd = -1;
	m_sinks[id]->state = READY;
	fprintf(stderr, "audio sink %u changed state to READY\n", id);
	return 0;
}

int CAudioSink::SetMute(u_int32_t id, bool mute)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return -1;
	m_sinks[id]->mute = mute;
	return 0;
}
void CAudioSink::QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples)
{
	// Make checks
	if (!pQueue || !drop_samples) return;
	u_int32_t drop_bytes = drop_samples*sizeof(int32_t);
	while (drop_bytes > 0 && pQueue->sample_count > 0) {
		audio_buffer_t* buf = GetAudioBuffer(pQueue);
		if (!buf) break;
		if (drop_bytes >= buf->len) {
			drop_bytes -= buf->len;
			free(buf);
			continue;
		}
		buf->len -= drop_bytes;
		buf->data += drop_bytes;
		AddAudioBufferToHeadOfQueue(pQueue, buf);
		drop_bytes = 0;
	}
}

audio_queue_t* CAudioSink::AddQueue(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return NULL;
	audio_queue_t* pQueue = (audio_queue_t*) calloc(sizeof(audio_queue_t),1);
	if (pQueue) {
		pQueue->buf_count	= 0;
		pQueue->bytes_count     = 0;
		pQueue->sample_count    = 0;
		pQueue->async_access    = false;
		//pQueue->queue_samples[]       = 0;    // this is zeroed by calloc
		pQueue->qp	      = 0;
		pQueue->queue_total     = 0;
		pQueue->queue_max       = DEFAULT_QUEUE_MAX_SECONDS*m_sinks[id]->rate*m_sinks[id]->channels;

		//pQueue->wait = 4;
		pQueue->pNext = m_sinks[id]->pAudioQueues;
		m_sinks[id]->pAudioQueues = pQueue;
if (m_verbose) fprintf(stderr, "audio sink %u adding queue\n", id);
	}
	return pQueue;
}

// Remove an audio queue from feed
bool CAudioSink::RemoveQueue(u_int32_t sink_id, audio_queue_t* pQueue)
{
	if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id] ||
		!pQueue || !m_sinks[sink_id]->pAudioQueues) return false;
	audio_queue_t* p = m_sinks[sink_id]->pAudioQueues;
	bool found = false;
	if (p == pQueue) {
		m_sinks[sink_id]->pAudioQueues = pQueue->pNext;
		found = true;
		if (m_verbose) fprintf(stderr, "audio sink %u removing first audio queue\n", sink_id);
	} else {
		while (p) {
			if (p->pNext == pQueue) {
				p->pNext = pQueue->pNext;
				found = true;
				if (m_verbose) fprintf(stderr, "audio sink %u removing "
					"audio queue\n", sink_id);
				break;
			}
		}
	}
	if (found) {
		if (m_verbose > 1) fprintf(stderr, "audio sink %u freeing %u buffers and %u bytes\n",
			sink_id, pQueue->buf_count, pQueue->bytes_count);
		audio_buffer_t* pBuf = pQueue->pAudioHead;
		while (pBuf) {
			audio_buffer_t* next = pBuf->next;
			free(pBuf);
			pBuf = next;
		}
		free(pQueue);
	}
	if (!m_sinks[sink_id]->pAudioQueues) {
		if (m_sinks[sink_id]->state == RUNNING || m_sinks[sink_id]->state == STALLED) {
			if (m_verbose > 1) fprintf(stderr, "Frame %u - audio sink %u changing state from %s to %s\n",
				system_frame_no, sink_id,
				feed_state_string(m_sinks[sink_id]->state),
				feed_state_string(PENDING));
			m_sinks[sink_id]->state = PENDING;
		}
	}
	return found;
}

void CAudioSink::AddAudioToSink(u_int32_t id, audio_buffer_t* buf)
{
//fprintf(stderr, "Add buffer to queue for sink %u len %u ", id, buf->len);
//fprintf(stderr, "buf <%lu> data %lu\n", (u_int64_t) buf, (u_int64_t) buf->data);
	if (!buf) return;
	if (id < m_max_sinks && m_sinks && m_sinks[id] &&
		!m_sinks[id]->mute && m_sinks[id]->pAudioQueues) {
			buf->seq_no = m_sinks[id]->buf_seq_no++;
			SDL_LockAudio();
			audio_queue_t* paq = m_sinks[id]->pAudioQueues;
			buf->next = NULL;
			while (paq) {
				// Check if the audio queues have more members
				if (paq->pNext) {
fprintf(stderr, "paq->pNext was not NULL\n");
exit(1);
					// we have more consumers and need to copy data
					audio_buffer_t* newbuf = GetEmptyBuffer(id, NULL);
					if (newbuf) {
						memcpy(newbuf, buf, sizeof(audio_buffer_t)+buf->len);
						if (paq->pAudioTail) {
							paq->pAudioTail->next = newbuf;
							paq->pAudioTail = newbuf;
						} else paq->pAudioTail = newbuf;
						if (!paq->pAudioHead) paq->pAudioHead = newbuf;
					}
				// This is the last member and we can use the buffer as is
				} else {
					if (paq->pAudioTail) {
						paq->pAudioTail->next = buf;
						paq->pAudioTail = buf;
					} else paq->pAudioTail = buf;
					if (!paq->pAudioHead) paq->pAudioHead = buf;
				}
				//if (paq->wait > 0) paq->wait--;
				paq = paq->pNext;
			}
		if (m_verbose)
			fprintf(stderr, " - Frame no %u - audio sink %u got buffer "
				"seq no %u with %u bytes\n",
				system_frame_no,
				id, buf->seq_no, buf->len);
		SDL_UnlockAudio();
	} else {
		free(buf);
	}
}

/*
// Deliver an audio buffer from a specific queue.
// Update bytes and buffer count for feed.
audio_buffer_t* CAudioSink::GetAudioBuffer(audio_queue_t* pQueue) {

	// First we check we have a queue and that it has buffer(s)
	if (!pQueue || !pQueue->pAudioHead) return NULL;
	audio_buffer_t* pBuf = pQueue->pAudioHead;
	if (pBuf) {
		pQueue->pAudioHead = pBuf->next;
		if (!pQueue->pAudioHead) {
			pQueue->pAudioTail = NULL;
		}
		pQueue->buf_count--;
		pQueue->bytes_count -= pBuf->len;
		pQueue->sample_count -= (pBuf->len/sizeof(int32_t));
	}
	return pBuf;
}

void fill_audio_sink(void *udata, Uint8 *stream, int len)
{	CAudioSink* p = (CAudioSink*) udata;
	if (!udata) return;
	p->MonitorSinkCallBack(1, stream, len);
}

void CAudioSink::MonitorSinkCallBack(u_int32_t id, Uint8 *stream, int len)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || len < 1) return;
	fprintf(stderr, "Frame %u - Monitor call back len %d\n",
		system_frame_no, len);
	if (m_sinks[id]->monitor_queue) {
		audio_buffer_t* p = m_sinks[id]->monitor_queue->pAudioHead;
		//if (m_sinks[id]->monitor_queue->wait)
			//fprintf(stderr, " - wait %u\n", m_sinks[id]->monitor_queue->wait);
		int i=0;
		while(p) {
			fprintf(stderr, " - seq no %u len %u\n", p->seq_no, p->len);
			p = p->next;
			if (i++ > 16) break;
		}
	}
	audio_buffer_t* pBuf = NULL;
	while (len > 0) {
		if (!pBuf) pBuf = GetAudioBuffer(m_sinks[id]->monitor_queue);
		if (pBuf) {
			if (pBuf->len > (unsigned) len) {
				fprintf(stderr, "Frame %u - buf %u - Audio mix %d. Have %u buf "
					"<%lu> data <%lu>\n",
					system_frame_no,
					pBuf->seq_no, len, pBuf->len, (u_int64_t) pBuf,
					(u_int64_t) pBuf->data);
				SDL_MixAudio(stream, pBuf->data, len, SDL_MIX_MAXVOLUME);
if (m_monitor_fd > -1) write(m_monitor_fd, pBuf->data, len);
				pBuf->data = pBuf->data + len;
				pBuf->len -= len;
				len = 0;
				pBuf->next = m_sinks[id]->monitor_queue->pAudioHead;
				m_sinks[id]->monitor_queue->pAudioHead = pBuf;
				if (!m_sinks[id]->monitor_queue->pAudioTail)
					m_sinks[id]->monitor_queue->pAudioTail = pBuf;
				//if (m_sinks[id]->monitor_queue->wait > 0)
					//m_sinks[id]->monitor_queue->wait = 0;
			} else {
				fprintf(stderr, "Frame %u - buf %u - Audio mix %u but only have "
					"%d.\n",
					system_frame_no,
					pBuf->seq_no, len, pBuf->len);
				SDL_MixAudio(stream, pBuf->data, pBuf->len,
					SDL_MIX_MAXVOLUME);
if (m_monitor_fd > -1) write(m_monitor_fd, pBuf->data, pBuf->len);
				len -= pBuf->len;
				free(pBuf);
				pBuf = NULL;
			}
		} else {
fprintf(stderr, "Frame %u - We need to fill with silence %d\n",
system_frame_no, len);
			u_int8_t* buf = (u_int8_t*) calloc(len, 1);
			if (buf) {
				memset(buf, 0, len);
				SDL_MixAudio(stream, buf, len, SDL_MIX_MAXVOLUME);
				free(buf);
			}
			len = 0;
		}
	}
	//u_int8_t buf[2*8192];
	//memset(buf, 0, 2*8192);
	//SDL_MixAudio(stream, buf, len, SDL_MIX_MAXVOLUME);
	return;
	//if (!m_sinks[id]->pAudioDataHead)) {
		fprintf(stderr, "Monitor call back had no data to fill\n");
		return;
	//}
	while (len) {
		
	}
	//SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME)
}

//extern void fill_audio(void *udata, Uint8 *stream, int len);
void CAudioSink::MonitorSink(u_int32_t id)
{
	SDL_AudioSpec wanted;
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		!m_sinks[id]->rate || !m_sinks[id]->channels) {
		fprintf(stderr, "Can not monitor audio sink\n");
		return;
	}
	// Set the audio format
	wanted.freq = m_sinks[id]->rate;
	wanted.channels = m_sinks[id]->channels;    // 1 = mono, 2 = stereo 
	if (m_sinks[id]->is_signed) {
		if (m_sinks[id]->bytespersample == 1) wanted.format = AUDIO_S8;
		else wanted.format = AUDIO_S16LSB;
		// FIXME. What to do for 24 bit int or 32 float or 64 double
	} else {
		if (m_sinks[id]->bytespersample == 1) wanted.format = AUDIO_U8;
		else wanted.format = AUDIO_U16;
		// FIXME. What to do for 24 bit int or 32 float or 64 double
	}
	// Good low-latency value for callback
	wanted.samples = 2048;
	wanted.callback = fill_audio_sink;
	wanted.userdata = this;
	fprintf(stderr, "SDL Audio format rate %u channels %u format %u <%u>\n",
		wanted.freq, wanted.channels, wanted.format, AUDIO_S16);

	// Open the audio device, forcing the desired format
	if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return;
	}
	fprintf(stderr, "Monitor set to on\n");
	return;
}
*/

void CAudioSink::Update()
{
	u_int32_t id;
	timeval timenow, timedif;

	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update start\n",
		system_frame_no);
	if (!m_sinks) {
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update done\n",
			system_frame_no);
		return;
	}

	// Check to see if animation is needed
	if (m_animation) {
		for (id = 0; id < m_max_sinks ; id++) {
			if (m_sinks[id] && m_sinks[id]->volume_delta_steps) {
				if (m_sinks[id]->pVolume) {
					float* p = m_sinks[id]->pVolume;
					for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
						*p += m_sinks[id]->volume_delta;
						if (*p < 0.0) *p = 0;
						else if(*p > MAX_VOLUME_FLOAT) *p = MAX_VOLUME_FLOAT;
					}
				}
				m_sinks[id]->volume_delta_steps--;
			}
		}
		m_animation--;
	}


	gettimeofday(&timenow, NULL);

	// Process all existing sinks
	for (id = 0; id < m_max_sinks ; id++) {
		
		if (!m_sinks[id]) continue;
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u update\n",
			system_frame_no, id);

		// Do some accounting on clipping and rms
		if (m_sinks[id]->clipped) m_sinks[id]->clipped--;
		if (m_sinks[id]->rms) for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++)
			m_sinks[id]->rms[i] *= 0.95;

		// Find time diff, sample diff and calculate samplespersecond
		if (m_sinks[id]->update_time.tv_sec > 0 || m_sinks[id]->update_time.tv_usec > 0) {
                        timersub(&timenow, &m_sinks[id]->update_time, &timedif);
                        double time = timedif.tv_sec + ((double) timedif.tv_usec)/1000000.0;
                        if (time != 0.0) m_sinks[id]->samplespersecond =
				(m_sinks[id]->samples - m_sinks[id]->last_samples)/time;
                        m_sinks[id]->total_avg_sps -= m_sinks[id]->avg_sps_arr[m_sinks[id]->avg_sps_index];
                        m_sinks[id]->total_avg_sps += m_sinks[id]->samplespersecond;
                        m_sinks[id]->avg_sps_arr[m_sinks[id]->avg_sps_index] = m_sinks[id]->samplespersecond;
                        m_sinks[id]->avg_sps_index++;
                        m_sinks[id]->avg_sps_index %= MAX_AVERAGE_SAMPLES;
                }

                // Update values for next time
                m_sinks[id]->last_samples = m_sinks[id]->samples;
                m_sinks[id]->update_time.tv_sec = timenow.tv_sec;
                m_sinks[id]->update_time.tv_usec = timenow.tv_usec;


		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u accounting done\n",
			system_frame_no, id);

		// Now we check if we have a sink pending start but we
		// will not start it until we have data in the audio queue
		if (m_sinks[id]->state == PENDING && m_sinks[id]->pAudioQueue &&
			m_sinks[id]->pAudioQueue->bytes_count > 0) {
				if (m_sinks[id]->pAudioQueue->sample_count <
					m_sinks[id]->calcsamplespersecond/m_pVideoMixer->m_frame_rate) {
					if (m_verbose > 1)
						fprintf(stderr, "Frame %u - audio sink "
							"%u is pending. queue has %u samples. Sink needs %u samples\n",
							system_frame_no, 
							id, m_sinks[id]->pAudioQueue->sample_count,
							(u_int32_t)(m_sinks[id]->calcsamplespersecond/
							m_pVideoMixer->m_frame_rate));
					continue;
				}

			// If start_time == 0 we set the start time, else the start_time has been set
			// previous (perhaps for another audio source and has already run
			if (m_sinks[id]->start_time.tv_sec == 0 && m_sinks[id]->start_time.tv_usec == 0)
				gettimeofday(&m_sinks[id]->start_time, NULL);

			if (m_verbose)
				fprintf(stderr, "\nFrame %u - audio sink %u changed state from %s to %s\n"
					"Frame %u - audio sink %u audio queue has %u buffers and %u samples\n",
					system_frame_no, id, feed_state_string(m_sinks[id]->state),
					feed_state_string(RUNNING),
					system_frame_no, id, m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->sample_count);

			// state is now running and we will write date next time around
			m_sinks[id]->state = RUNNING;

			continue;
		}

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if stalled \n",
			system_frame_no, id);

		// We skip the sink, if the sink is not running or stalled
		if (m_sinks[id]->state != RUNNING &&
			m_sinks[id]->state != STALLED) continue;

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if queue is present \n",
			system_frame_no, id);

		// Now we check we have an audio queue for the sink.
		if (!m_sinks[id]->pAudioQueue) {
			fprintf(stderr, "Frame %u - Ooops. Audio queue was gone for sink %u\n"
				" - audio sink %u will be stopped and change state to SETUP.\n",
				system_frame_no,
				id, id);
			m_sinks[id]->state = SETUP;
			continue;
		}

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if rate limited \n",
			system_frame_no, id);

		// Lets check if we need to add silence
		if (m_sinks[id]->write_silence_bytes) {
			u_int32_t size;
			int32_t n = 0;
			audio_buffer_t* newbuf = GetEmptyBuffer(id, &size);
			memset(newbuf->data, 0, size);
			while (newbuf && m_sinks[id]->write_silence_bytes) {
				int32_t n = m_sinks[id]->write_silence_bytes > size ? size :
					m_sinks[id]->write_silence_bytes;
				newbuf->len = n;

				// Write to fd if present or else ...
				if (m_sinks[id]->sink_fd > -1)
					n = write(m_sinks[id]->sink_fd, newbuf->data, newbuf->len);
				// .... or else just dump the data
				else n = newbuf->len;
				if (n < 1) break;
				m_sinks[id]->silence += (n/m_sinks[id]->bytespersample);
				if ((unsigned) n <= m_sinks[id]->write_silence_bytes) m_sinks[id]->write_silence_bytes -= n;
				else m_sinks[id]->write_silence_bytes = 0;
			}
			if (newbuf) free(newbuf);

			//if we failed to write, we skip to next sink
			if (n < 0 && (errno == EAGAIN || EWOULDBLOCK)) continue;
		}

		// If we don't have a rate limit, we don't check how much time has passed, we don't
		// check how much we should write. We just write whatever we have in the queue
		if (m_sinks[id]->no_rate_limit) {
			// if we have no data in queue, we skip
			if (!m_sinks[id]->pAudioQueue->sample_count) continue;
			// we have data in the queue
			u_int32_t size, samples;
			audio_buffer_t* newbuf = NULL;
			audio_buffer_t* samplebuf = GetEmptyBuffer(id, &size);

			// Check that we have a sample buffer for conversion
			if (!samplebuf) {
				fprintf(stderr, "Frame %u - audio sink %u failed to "
					"get sample buffer\n", system_frame_no, id);
				continue;
			}

			// If the sink gets stopped while in loop, the queue will be removed
			// and GetAudioBuffer will return NULL
			while ((newbuf = GetAudioBuffer(m_sinks[id]->pAudioQueue))) {

if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u got buffer seq_no %u with %u bytes, sink mute %s buf mute %s\n",
	system_frame_no, id, newbuf->seq_no, newbuf->len, m_sinks[id]->mute ? "true" : "false", newbuf->mute ? "true" : "false");
				// Now check if we should drop samples
				if (m_sinks[id]->drop_bytes) {
					if (m_verbose > 1) fprintf(stderr,
						"Frame %u - audio sink %u dropped %u samples\n", system_frame_no, id,
						(newbuf->len <= m_sinks[id]->drop_bytes ? newbuf->len :
						m_sinks[id]->drop_bytes) / m_sinks[id]->bytespersample);
					if (newbuf->len >= m_sinks[id]->drop_bytes) {
						newbuf->len -= m_sinks[id]->drop_bytes;
						newbuf->data += m_sinks[id]->drop_bytes;
						m_sinks[id]->dropped += (m_sinks[id]->drop_bytes/
							m_sinks[id]->bytespersample);
						m_sinks[id]->drop_bytes = 0;
					} else {
						m_sinks[id]->dropped += (newbuf->len/
							m_sinks[id]->bytespersample);
						m_sinks[id]->drop_bytes -= newbuf->len;
						free(newbuf);
						continue;
					}
						
				}
				if (newbuf->mute || m_sinks[id]->mute)
					memset(newbuf->data, 0, newbuf->len);
				while (newbuf->len) {

if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u buffer with %u bytes\n",
	system_frame_no, id, newbuf->len);
					if (newbuf->len/sizeof(int32_t) > size/m_sinks[id]->bytespersample)
						samples = size/m_sinks[id]->bytespersample;
					else
						samples = newbuf->len/sizeof(int32_t);

					// Then we set the volume
					if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
						"setting volume\n", system_frame_no, id);
					if (newbuf->channels == m_sinks[id]->channels && m_sinks[id]->pVolume)
						SetVolumeForAudioBuffer(newbuf, m_sinks[id]->pVolume);
					else if (m_verbose > 2) fprintf(stderr,
						"Frame %u - audio sink %u volume error : %s\n",
						system_frame_no, id,
						newbuf->channels == m_sinks[id]->channels ?
						  "missing volume" : "channel mismatch");

					if (newbuf->clipped) m_sinks[id]->clipped = 100;

					if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
						"setting rms\n", system_frame_no, id);

					// Now we need to set RMS for source from buffer
					if (!newbuf->mute) for (u_int32_t i=0 ; i < m_sinks[id]->channels &&
						i < newbuf->channels; i++) {
						float vol = m_sinks[id]->pVolume ?
							m_sinks[id]->pVolume[i] * m_sinks[id]->pVolume[i] :
							  1.0;
						if (vol != 1.0) {
							if (m_sinks[id]->rms[i] < newbuf->rms[i]*vol)
								m_sinks[id]->rms[i] = newbuf->rms[i]*vol;
						}
						else if (m_sinks[id]->rms[i] < newbuf->rms[i])
							m_sinks[id]->rms[i] = newbuf->rms[i];
					}

					if (m_verbose > 4) fprintf(stderr,
						"Frame %u - audio sink %u convert\n", system_frame_no, id);

					if (m_sinks[id]->bytespersample == 2) {
						if (m_sinks[id]->is_signed) {
							convert_int32_to_si16((u_int16_t*)samplebuf->data,
								(int32_t*)newbuf->data, samples);
						} else {
							convert_int32_to_ui16((u_int16_t*)samplebuf->data,
								(int32_t*)newbuf->data, samples);
						}
					} else {
						fprintf(stderr, "audio sink %u unsupported audio format\n", id);
						break;
					}

					samplebuf->len = samples*m_sinks[id]->bytespersample;
					newbuf->len -= (samples*sizeof(int32_t));
					u_int32_t written = 0;
					while (written < samplebuf->len) {
						if (m_verbose > 4) fprintf(stderr, "Frame %u - audio "
							"sink %u write->fd %d buf->len %u written %u "
							"writing %u\n", system_frame_no, id,
							m_sinks[id]->sink_fd, samplebuf->len, written,
							samplebuf->len-written);

						// Write samplebuf->len-written bytes to output fd
						int n;
						// if sink_fd > -1, then write data else ...
						if (m_sinks[id]->sink_fd > -1)
							n = write(m_sinks[id]->sink_fd, samplebuf->data +
								written, samplebuf->len-written);
						// .... or else just dump the data
						else {
							n = samplebuf->len-written;
							m_sinks[id]->dropped += (samplebuf->len-written);
						}
						
						// Check for EAGAIN and EWOULDBLOCK
						if ((n <= 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
							n = samplebuf->len-written;
							m_sinks[id]->dropped += (samplebuf->len-written);
							written += n;
							//n = 0;
							if (m_verbose) fprintf(stderr, "Frame %u - audio sink %u "
								"failed writing. reason %s\n", system_frame_no, id,
								errno == EAGAIN ? "EAGAIN" : "EWOULDBLOCK");
// FIXME. We need to break the loop and skip to next sink. We also need to add the buffer back to the queue which is bad because it is converted.
							continue;
						} else if (n <= 0) {
							perror("audio sink write failed");
						}

						// if n < 0 we failed and we will drop the buffer and stop the sink
						if (n < 0) {
							perror ("Could not write data for audio sink");
							fprintf(stderr, "audio sink %u failed writing buffer "
								"to fd %d.\n", id, m_sinks[id]->sink_fd);
							StopSink(id);	// StopSInk will remove queue
							newbuf->len = 0;
							break;
						}
						written += n;
if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u wrote %d bytes written %u\n",
	system_frame_no, id, n, written);
					}
					m_sinks[id]->samples += (written/m_sinks[id]->bytespersample);
				}
				if (newbuf) free(newbuf);
			}
			if (samplebuf) free(samplebuf);
			continue;
		}
fprintf(stderr, "WE SHOULD NOT BE HERE\n");


		// We have rate limit and must see if we need to calculate how much we should write,
		// insert silence if needed and drop data in queue if it becomes too long 

		// The sink is running or stalled. Lets calculate how many
		// bytes we need to write
		int32_t needed_samples = SamplesNeeded(id)*m_sinks[id]->channels;

		// Lets check if we should write any samples at all
		if (needed_samples < 1) {
			if (m_verbose)
				fprintf(stderr, "Frame %u - audio sink %u does not need any samples. "
				"Skipping\n", id, system_frame_no);
			continue;
		}
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -7 sink %u %u\n", id, m_sinks[id]->drop_bytes);

		// Now lets see how many samples we in the queue.
		u_int32_t queue_samples = m_sinks[id]->pAudioQueue->sample_count;

		int32_t average_samples = QueueUpdateAndAverage(m_sinks[id]->pAudioQueue);
		if (average_samples > m_sinks[id]->pAudioQueue->queue_max) {
			if (m_verbose) fprintf(stderr, "Frame %u - "
				"audio sink %u av. samp. %d (%d ms) exceeds max. queue = %u (%u ms) max = %d (%u ms)\n",
				system_frame_no, id,
				average_samples, 1000*average_samples/(m_sinks[id]->rate*m_sinks[id]->channels),
				queue_samples, 1000*queue_samples/(m_sinks[id]->rate*m_sinks[id]->channels),
				m_sinks[id]->pAudioQueue->queue_max,
				1000*m_sinks[id]->pAudioQueue->queue_max/(m_sinks[id]->rate*m_sinks[id]->channels));
			if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u queue bytes %u queue buffers %u\n",
				system_frame_no, id,
				m_sinks[id]->pAudioQueue->bytes_count, m_sinks[id]->pAudioQueue->buf_count);

			// Now we will check if we should drop samples
			if ((int32_t) queue_samples > m_sinks[id]->pAudioQueue->queue_max) {
				u_int32_t drop_samples = queue_samples - m_sinks[id]->pAudioQueue->queue_max;
				if (m_sinks[id]->channels > 1) {
					drop_samples /= m_sinks[id]->channels;
					drop_samples *= m_sinks[id]->channels;
				}
				if (m_verbose)  fprintf(stderr, "Frame %u - audio %s %u dropping samples %u (%u ms)\n",
					system_frame_no,
					m_sinks[id]->source_type == 1 ? "feed" :
					  m_sinks[id]->source_type == 2 ? "mixer" :
					    m_sinks[id]->source_type == 3 ? "sink" : "unknown",
					id, drop_samples,
					1000*drop_samples/(m_sinks[id]->channels*m_sinks[id]->rate));

				if (m_sinks[id]->source_type == 1)
					m_pVideoMixer->m_pAudioFeed->QueueDropSamples(m_sinks[id]->pAudioQueue,
						drop_samples);
				else if (m_sinks[id]->source_type == 2)
					m_pVideoMixer->m_pAudioMixer->QueueDropSamples(m_sinks[id]->pAudioQueue,
						drop_samples);
				else if (m_sinks[id]->source_type == 3)
					QueueDropSamples(m_sinks[id]->pAudioQueue, drop_samples);
				else fprintf(stderr, "Frame %u - audio sink %u source is unknown\n",
					system_frame_no, id);
				queue_samples = m_sinks[id]->pAudioQueue->sample_count;
			}
				
	
		}
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -6 sink %u %u\n", id, m_sinks[id]->drop_bytes);

/*
		// Lets calculate the average surplus after subtracting needed samples
		// queue_samples[] = [count1, count2 ... countn, total]. First n counts in round robin
		int32_t* total = &m_sinks[id]->queue_samples[sizeof(m_sinks[id]->queue_samples)/
			sizeof(m_sinks[id]->queue_samples[0])-1];
		*total -= m_sinks[id]->queue_samples[m_sinks[id]->nqb];
		m_sinks[id]->queue_samples[m_sinks[id]->nqb] = queue_samples - needed_samples;
		*total += m_sinks[id]->queue_samples[m_sinks[id]->nqb];
		m_sinks[id]->nqb++;
		m_sinks[id]->nqb %= ((sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0]))-1);
		if (m_verbose > 2 && !m_sinks[id]->nqb)
			fprintf(stderr, "audio sink %u average(%2u) %d queue samples %u missing %u\n",
				id, m_sinks[id]->nqb,
				*total/(int32_t)(sizeof(m_sinks[id]->queue_samples)/
					sizeof(m_sinks[id]->queue_samples[0])-1),
				queue_samples, needed_samples);

		if (m_verbose > 3) 
			fprintf(stderr, "Frame %u - audio sink %u has %u samples in queue "
				"and miss %u samples%s\n",
				system_frame_no,
				id, queue_samples, needed_samples,
				(u_int32_t)needed_samples > queue_samples ? " Adding silence needed" : "");

		// total / (120 / 4 - 1) = total / 39
		if (*total/(int32_t)(sizeof(m_sinks[id]->queue_samples)/
			sizeof(m_sinks[id]->queue_samples[0])-1) >
			(2*sizeof(int32_t)*m_sinks[id]->buffer_size/m_sinks[id]->bytespersample) &&
			m_sinks[id]->pAudioQueue->sample_count > m_sinks[id]->buffer_size) {
			if (m_verbose)
				fprintf(stderr, "Frame %u - audio sink %u has %u buffers and %u samples in queue. "
					"Dropping 1 buffer with %u samples\n", 
					system_frame_no, id,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->sample_count,
					(u_int32_t)(m_sinks[id]->pAudioQueue->pAudioHead->len/sizeof(int32_t)));
			audio_buffer_t* buf = GetAudioBuffer(m_sinks[id]->pAudioQueue);
u_int32_t no_samples = buf->len/(m_sinks[id]->channels*m_sinks[id]->bytespersample);
	if (no_samples*m_sinks[id]->channels*m_sinks[id]->bytespersample != buf->len)
		fprintf(stderr, "Audio sink drops MISMATCH for buffer size %u\n",buf->len);
			if (buf) {
				free(buf);
			}
			queue_samples = m_sinks[id]->pAudioQueue->sample_count;
		}
*/
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -5 sink %u %u\n", id, m_sinks[id]->drop_bytes);

		// Lets check if we have enough bytes to write
		while ((u_int32_t)needed_samples > queue_samples) {
			// First we calculate the samples we need (and add one extra)
			u_int32_t samples = needed_samples - queue_samples;

			// Then we get a buffer
			u_int32_t size = 0;
			audio_buffer_t* newbuf = GetEmptyBuffer(id, &size);

			// If we dont get a buffer we can't do much about it
			if (!newbuf) break;

			// The we figure out how much silence to fill in
			size = size / sizeof(int32_t) > samples ? samples*sizeof(int32_t) : size;

			// Then we fill it with silence and add it to head of queue
			// The sequence number will be zero.
			memset(newbuf->data, 0, size);
			newbuf->len = size;
			newbuf->seq_no = 0;
			AddAudioBufferToHeadOfQueue(m_sinks[id]->pAudioQueue, newbuf);
			queue_samples = m_sinks[id]->pAudioQueue->sample_count;
			if (m_verbose > 1) {
				fprintf(stderr, "Frame %u - audio sink %u added %u samples of silence\n",
					system_frame_no, id, (u_int32_t)(newbuf->len/sizeof(int32_t)));
			}
		}

if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -4 sink %u %u\n", id, m_sinks[id]->drop_bytes);
		// Now if the sink is a file write
		if (m_sinks[id]->sink_fd >= 0) {

			// We need to convert internal format int32_t to si16 or ui16
			// and we need a buffer for that.
			u_int32_t size;
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -3 sink %u %u\n", id, m_sinks[id]->drop_bytes);
			audio_buffer_t* samplebuffer = GetEmptyBuffer(id, &size);
			while (needed_samples > 0) {
				if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u "
					"needed samples %u queue samples %u\n",
					system_frame_no, id, needed_samples, queue_samples);
				audio_buffer_t* p = m_sinks[id]->pAudioQueue->pAudioHead;
				unsigned count = 0;
				if (m_verbose > 3) fprintf(stderr,
					"Frame %u - audio sink %u counting\n", system_frame_no, id);
				while (p) {
					count++;
					p = p->next;
				}
				if (m_verbose > 3) fprintf(stderr,
					"Frame %u - Audio queue said %u counted %u\n",
					system_frame_no, m_sinks[id]->pAudioQueue->buf_count, count);
				if (count != m_sinks[id]->pAudioQueue->buf_count) {
					fprintf(stderr, "Frame %u - audio sink %u queue "
						"count mismatch. Counted %u queue says %u\n",
						system_frame_no, id,
						count, m_sinks[id]->pAudioQueue->buf_count);
				}
				audio_buffer_t* newbuf = GetAudioBuffer(m_sinks[id]->pAudioQueue);
u_int32_t no_samples = newbuf->len/(m_sinks[id]->channels*m_sinks[id]->bytespersample);
	if (no_samples*m_sinks[id]->channels*m_sinks[id]->bytespersample != newbuf->len)
		fprintf(stderr, "Audio sink drops MISMATCH for buffer size %u\n",newbuf->len);
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -2 sink %u %u\n", id, m_sinks[id]->drop_bytes);
				if (!newbuf) {
					fprintf(stderr, "Frame %u - audio sink %u failed to "
						"get audio buffer\n"
						" - audio queue has %u samples and %u buffers\n",
						system_frame_no, id,
						m_sinks[id]->pAudioQueue->sample_count,
						m_sinks[id]->pAudioQueue->buf_count);
					break;
				}
				u_int32_t samples = newbuf->len/sizeof(int32_t) >
					size/m_sinks[id]->bytespersample ?
					size/m_sinks[id]->bytespersample :
					newbuf->len/sizeof(int32_t);
				if (samples > (u_int32_t)needed_samples) samples = needed_samples;
				if (m_sinks[id]->bytespersample == 2) {
					if (m_sinks[id]->is_signed) {
						convert_int32_to_si16((u_int16_t*)samplebuffer->data,
							(int32_t*)newbuf->data, samples);
					} else {
						convert_int32_to_ui16((u_int16_t*)samplebuffer->data,
							(int32_t*)newbuf->data, samples);
					}
				} else {
					fprintf(stderr, "audio sink %u unsupported audio format\n", id);
					break;
				}
				samplebuffer->len = samples*m_sinks[id]->bytespersample;

				if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u will "
					"write %u samples = %u bytes\n"
					" - the queue has %u bytes and %u buffers\n",
					system_frame_no, id, samples, samplebuffer->len,
					m_sinks[id]->pAudioQueue->bytes_count,
					m_sinks[id]->pAudioQueue->buf_count);

if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -1 sink %u %u\n", id, m_sinks[id]->drop_bytes);
				// This adds silence samples to output without affecting
				// SamplesNeeded()
				if (m_sinks[id]->write_silence_bytes) {
					u_int8_t* buf = (u_int8_t *)calloc(m_sinks[id]->write_silence_bytes,1);
					if (buf) {
						int n=write(m_sinks[id]->sink_fd, buf, m_sinks[id]->write_silence_bytes);
						if (n == (signed) m_sinks[id]->write_silence_bytes)
							fprintf(stderr, "wrote %u bytes of silence\n",
								m_sinks[id]->write_silence_bytes);
						else
							fprintf(stderr, "wrote %u bytes of silence\n",
								m_sinks[id]->write_silence_bytes);
						m_sinks[id]->write_silence_bytes = 0;
						free(buf);
					}

				}

				// Check and see if we should drop some output bytes
				int dropped_bytes = 0;
				if (m_sinks[id]->drop_bytes) {
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING sink %u %u\n", id, m_sinks[id]->drop_bytes);
					if (m_sinks[id]->drop_bytes < samplebuffer->len) {
						samplebuffer->data += m_sinks[id]->drop_bytes;
						samplebuffer->len -= m_sinks[id]->drop_bytes;
						dropped_bytes = m_sinks[id]->drop_bytes;
						m_sinks[id]->drop_bytes = 0;
					} else {
						samplebuffer->data += samplebuffer->len;
						m_sinks[id]->drop_bytes -= samplebuffer->len;
						dropped_bytes = samplebuffer->len;
						samplebuffer->len = 0;
					}
					m_sinks[id]->dropped = dropped_bytes/m_sinks[id]->bytespersample;
					if (m_verbose) fprintf(stderr, "audio mixer %u dropping %u samples per channel on output\n",
						id, dropped_bytes/(m_sinks[id]->channels*m_sinks[id]->bytespersample));
				}
				int n = 0;
				if (samplebuffer->len) {
					n = write(m_sinks[id]->sink_fd, samplebuffer->data, samplebuffer->len);
					if ((n <= 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) n = 0;
					// if n < 0 we failed and we will drop the buffer and stop the sink
					if (n < 0) {
						perror ("Could not write data for audio sink");
						fprintf(stderr, "audio sink %u failed writing buffer to fd %d.\n",
							id, m_sinks[id]->sink_fd);
						if (newbuf) free(newbuf); newbuf = NULL;
						if (samplebuffer) free(samplebuffer); samplebuffer = NULL;
						
						StopSink(id);
						break;
					}
				}
				n += dropped_bytes;
				newbuf->len -= ((sizeof(int32_t)*n)/m_sinks[id]->bytespersample);

				m_sinks[id]->sample_count += (n/(m_sinks[id]->bytespersample*m_sinks[id]->channels));

				if (newbuf->len > 0) {
					newbuf->data += ((sizeof(int32_t)*n)/m_sinks[id]->bytespersample);
					AddAudioBufferToHeadOfQueue(m_sinks[id]->pAudioQueue,newbuf);
					if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u added back buf seq "
						"no %u to queue newbuf addr=%lu\n",
						system_frame_no,
						id, newbuf->seq_no, (u_int64_t)newbuf);
				} else free(newbuf);
				newbuf = NULL;
				if (n == 0) break;
				needed_samples -= (n/m_sinks[id]->bytespersample);
			}
			if (samplebuffer) free(samplebuffer); samplebuffer=NULL;
		} else {
			fprintf(stderr, "audio sink was not a file write sink\n");
		}
	}
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update done\n",
		system_frame_no);
}
