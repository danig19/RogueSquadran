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
#include "audio_mixer.h"

CAudioMixer::CAudioMixer(CVideoMixer* pVideoMixer, u_int32_t max_mixers)
{
	m_mixers = NULL;
	m_max_mixers = 0;
	m_verbose = 0;
	m_pVideoMixer = pVideoMixer;
	if (!(m_pAudioFeed = pVideoMixer->m_pAudioFeed)) {
		fprintf(stderr, "CAudioMixer could not set CAudioFeed\n");
		return;
	}
	m_mixers = (audio_mixer_t**) calloc(
		sizeof(audio_mixer_t*), max_mixers);
	//for (unsigned int i=0; i< m_max_mixers; i++) m_mixers[i] = NULL;
	if (m_mixers) {
		m_max_mixers = max_mixers;
	} else {
		fprintf(stderr, "CAudioMixer failed to allocate space for mixers\n");
	}
	m_mixer_count = 0;
	m_animation = 0;
}

CAudioMixer::~CAudioMixer()
{
	if (m_mixers) {
		for (unsigned int id=0; id<m_max_mixers; id++) DeleteMixer(id);
		free(m_mixers);
		m_mixers = NULL;
	}
	m_max_mixers = 0;
}

// audio mixer ....
int CAudioMixer::ParseCommand(struct controller_type* ctr, const char* str)
{
	//if (!m_pVideoMixer) return 0;
	// audio mixer status
	if (!strncasecmp (str, "status ", 7)) {
		return set_mixer_status(ctr, str+7);
	}
	// audio mixer add silence [<mixer id> <silence in ms>]]
	else if (!strncasecmp (str, "add silence ", 12)) {
		return set_add_silence(ctr, str+12);
	}

	// audio mixer add [<mixer id> [<mixer name>]]
	else if (!strncasecmp (str, "add ", 4)) {
		return set_mixer_add(ctr, str+4);
	}
	// audio mixer channels [<mixer id> <channels>]
	else if (!strncasecmp (str, "channels ", 9)) {
		return set_mixer_channels(ctr, str+9);
	}
	// audio mixer drop <sink id> <ms>]
	else if (!strncasecmp (str, "drop ", 5)) {
		return set_drop(ctr, str+5);
	}
	// audio mixer info
	if (!strncasecmp (str, "info ", 5)) {
		return list_info(ctr, str+5);
	}
	// audio mixer rate [<mixer id> <rate>]
	else if (!strncasecmp (str, "rate ", 5)) {
		return set_mixer_rate(ctr, str+5);
	}
//	// audio mixer format [<mixer id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
//	else if (!strncasecmp (str, "format ", 7)) {
//		return set_mixer_format(ctr, str+7);
//	}
//	else if (!strncasecmp (str, "monitor ", 8)) {
//		return set_monitor(ctr, str+8);
//	}
//	else if (!strncasecmp (str, "ctr isaudio ", 12)) {
//		return set_ctr_isaudio(ctr, str+12);
//	}
	// audio mixer volume [<mixer id> (-|<volume>) [(-|<volume>) ...]]
	else if (!strncasecmp (str, "volume ", 7)) {
		return set_mixer_volume(ctr, str+7);
	}
	// audio mixer move volume [<mixer id> <delta volume> <delta steps>]
	else if (!strncasecmp (str, "move volume ", 12)) {
		return set_mixer_move_volume(ctr, str+12);
	}
	// audio mixer queue <mixer id>
	else if (!strncasecmp (str, "queue ", 6)) {
		return set_mixer_queue(ctr, str+6);
	}

	// audio mixer source [(feed|mixer) <mixer id> <source id>]
	// audio mixer source [mute (on|off) <mixer id> <source no>]
	// audio mixer source [volume <mixer id> <source no> (-|<volume>) [(-|<volume) ...]]
	// audio mixer source [move volume <mixer id> <source no> <delta volume> <delta steps>]
	// audio mixer source [add silence <mixer id> <source no> <ms>]
	// audio mixer source [drop <mixer id> <source no> <ms>]
	// audio mixer source [maxdelay <mixer id> <source no> <ms>]
	// audio mixer source [map <mixer id> <source no> 
	// audio mixer source [invert <mixer id> <source no>]
	else if (!strncasecmp (str, "source ", 6)) {
		return set_mixer_source(ctr, str+6);
	}

	// audio mixer start [[soft ]<mixer id>]
	else if (!strncasecmp (str, "start ", 6)) {
		return set_mixer_start(ctr, str+6);
	}

	// audio mixer stop [<mixer id>]
	else if (!strncasecmp (str, "stop ", 5)) {
		return set_mixer_stop(ctr, str+5);
	}

	// audio mixer mute [(on | off) <mixer id>]
	else if (!strncasecmp (str, "mute ", 5)) {
		return set_mixer_mute(ctr, str+5);
	}

	// audio mixer verbose [<verbose level>]
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_mixer_verbose(ctr, str+8);
	}
	// audio mixer help
	else if (!strncasecmp (str, "help ", 5)) {
		return list_help(ctr, str+5);
	} else return 1;
	return 0;
}

// Create a virtual mixer
int CAudioMixer::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
			MESSAGE"audio mixer add [<mixer id> [<mixer name>]]\n"
			MESSAGE"audio mixer add silence <mixer id> <ms>\n"
			MESSAGE"audio mixer drop <mixer id> <ms>\n"
			MESSAGE"audio mixer channels [<mixer id> <channels>]\n"
			MESSAGE"audio mixer info\n"
			MESSAGE"audio mixer mute [(on | off) <mixer id>]\n"
			MESSAGE"audio mixer queue <mixer id>\n"
			MESSAGE"audio mixer rate [<mixer id> <rate>]\n"
			MESSAGE"audio mixer source [(feed|mixer) <mixer id> <source id>]\n"
			MESSAGE"audio mixer source add silence <mixer id> <source no> <ms>\n"
			//MESSAGE"audio mixer source drop <mixer id> <source no> <ms>\n"
			MESSAGE"audio mixer source [invert <mixer id> <source no>]\n"
			MESSAGE"audio mixer source [map <mixer id> <source no> <maps ...>]\n"
			MESSAGE"audio mixer source [maxdelay <mixer id> <source no> <ms>]\n"
			MESSAGE"audio mixer source [mute (on|off) <mixer id> <source no>]\n"
			MESSAGE"audio mixer source [volume <mixer id> <source no> (-|<volume>) [(-|<volume>) ...]]\n"
			MESSAGE"audio mixer start [[soft ]<mixer id>]\n"
			MESSAGE"audio mixer status\n"
			MESSAGE"audio mixer stop [<mixer id>]\n"
			MESSAGE"audio mixer verbose [<verbose level>]\n"
			MESSAGE"audio mixer volume [<mixer id> (-|<volume>) [(-|<volume>) ...]] "
				"// volume = 0..MAX_VOLUME\n"
			MESSAGE"audio mixer help\n"
			MESSAGE"\n");
	return 0;
}

// audio mixer info
int CAudioMixer::list_info(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" audio mixer info\n"
		STATUS" audio mixers      : %u\n"
		STATUS" max audio mixers  : %u\n"
		STATUS" verbose level     : %u\n",
		m_mixer_count,
		m_max_mixers,
		m_verbose
		);
	if (m_mixer_count) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" audio mixer id : state, rate, channels, bytespersample, "
			"signess, volume, mute, buffersize, delay, queues\n");
		for (u_int32_t id=0; id < m_max_mixers; id++) {
			if (!m_mixers[id]) continue;
			u_int32_t queue_count = 0;
			audio_queue_t* pQueue = m_mixers[id]->pAudioQueues;
			while (pQueue) {
				queue_count++;
				pQueue = pQueue->pNext;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" - audio mixer %u : %s, %u, %u, %u, %ssigned, %s", id,
				feed_state_string(m_mixers[id]->state),
				m_mixers[id]->rate,
				m_mixers[id]->channels,
				m_mixers[id]->bytespersample,
				m_mixers[id]->is_signed ? "" : "un",
				m_mixers[id]->pVolume ? "" : "0,");
			if (m_mixers[id]->pVolume) {
                                float* p = m_mixers[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_mixers[id]->channels; i++, p++)
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f,", *p);
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, " %smuted, %u, %u, %u\n",
				m_mixers[id]->mute ? "" : "un",
				m_mixers[id]->buffer_size,
				0, //m_mixers[id]->delay,
				queue_count);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

// audio mixer verbose [<level>] // set control channel to write out
int CAudioMixer::set_mixer_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio mixer verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	if ((n = sscanf(str, "%u", &m_verbose)) != 1) return -1;
	if (n == 1 && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"audio mixer verbose level set to %u for CAudioMixer\n",
			m_verbose);
	return 0;
}

/*
// audio mixer monitor (on | off) <mixer_id> 
int CAudioMixer::set_monitor(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"audio monitor\n");
		return 0;

	}
	// Get the mixer id.
	if ((n = sscanf(str, "on %u", &id)) != 1) return -1;
fprintf(stderr, "Monitor on\n");
	m_mixers[id]->monitor_queue = AddQueue(id);
	if (m_mixers[id]->monitor_queue) {
		MonitorMixer(id);
		SDL_PauseAudio(0);
	} else fprintf(stderr, "Failed to set audio monitor\n");
	return m_mixers[id]->monitor_queue ? 0 : 1;
}
*/

// audio mixer source (feed|mixer) <mixer id> <source id>
// audio mixer source mute (on|off) <mixer id> <source no>
// audio mixer source volume <mixer id> <source no> (-|<volume>)
// audio mixer source [add silence <mixer id> <source no> <ms>]
// audio mixer source [drop <mixer id> <source no> <ms>]
// audio mixer source [maxdelay <mixer id> <source no> <ms>]
// audio mixer source [map <mixer id> <source no>]
// audio mixer source [invert <mixer id> <source no>]
// audio mixer source [move volume <mixer id> <source no> <delta volume> <delta steps>]
int CAudioMixer::set_mixer_source(struct controller_type* ctr, const char* str)
{
	u_int32_t mixer_id, source_id, source_no, silence, drop, maxdelay, channel, delta_steps;
	float volume, volume_delta;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (u_int32_t id = 0; id < m_max_mixers ; id++) {
			if (m_mixers[id]) {
				audio_source_t* p = m_mixers[id]->pAudioSources;
				while (p) {
					int average_samples = QueueAverage(p->pAudioQueue);
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS"audio mixer %u sourced by %s source id %u "
						"av. samp. %d = %d ms, volume %s", id,
						p->source_type == 1 ? "audio feed" :
						  p->source_type == 2 ? "audio mixer" : "none",
						p->source_id,
						average_samples,
						1000*average_samples/(m_mixers[id]->rate*m_mixers[id]->channels),
						p->pVolume ? "" : "0");
					if (p->pVolume) {
						float* pv = p->pVolume;
						for (u_int32_t i=0 ; i < p->channels; i++, pv++) {
							m_pVideoMixer->m_pController->controller_write_msg (ctr,
							"%.3f%s", *pv, i+1 < p->channels ? "," : "");
                                		}

					}
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						" %smuted, %s", p->mute ? "" : "un",
						p->invert ? "inverted" : "normal");
					if (p->pSourceMap) {
						m_pVideoMixer->m_pController->controller_write_msg (ctr, ", map ");
						source_map_t* pMap = p->pSourceMap;
						while (pMap) {
							m_pVideoMixer->m_pController->controller_write_msg (ctr,
								"%u,%u%s",
								pMap->source_index, pMap->mix_index,
								pMap->next ? " " : "");
							pMap = pMap->next;
						}
					}
					m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n");
					p = p->next;
				}
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	int n = -1;
	// Get the mixer id.
	if (sscanf(str, "feed %u %u", &mixer_id, &source_id) == 2) {
		n = MixerSource(mixer_id, source_id, 1);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u setting audio feed %u as source\n",
				mixer_id, source_id);
	} else if (sscanf(str, "mixer %u %u", &mixer_id, &source_id) == 2) {
		n = MixerSource(mixer_id, source_id, 2);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u setting audio mixer %u as source\n",
				mixer_id, source_id);
	} else if (sscanf(str, "volume %u %u", &mixer_id, &source_no) == 2) {
		str += 7;
		while (*str && !isspace(*str)) str++;
                while (isspace(*str)) str++;
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
                                        if ((n=MixerSourceVolume(mixer_id,source_no,
						channel++,volume))) return n;
                                        while (*str && !isspace(*str)) str++;
                                        while (isspace(*str)) str++;
                                } else return -1;
                        }
                } else return -1;
                if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u setting source no %u volume %s\n",
				mixer_id, source_no, volume_list);
                n=0;

	// audio mixer source [move volume <mixer id> <source no> <delta volume> <delta steps>]
	} else if (sscanf(str, "move volume %u %u %f %u", &mixer_id,
		&source_no, &volume_delta, &delta_steps) == 4) {
		n = MixerSourceMoveVolume(mixer_id, source_no, volume_delta, delta_steps);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u source %u move volume delta %.4f steps %u\n",
				mixer_id, source_no, volume_delta, delta_steps);


	} else if (sscanf(str, "mute off %u %u", &mixer_id, &source_no) == 2) {
		n = MixerSourceMute(mixer_id, source_no, false);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u unmuting source no %u\n",
				mixer_id, source_no);
	} else if (sscanf(str, "mute on %u %u", &mixer_id, &source_no) == 2) {
		n = MixerSourceMute(mixer_id, source_no, true);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u muting source no %u\n",
				mixer_id, source_no);
	} else if (sscanf(str, "add silence %u %u %u", &mixer_id, &source_no, &silence) == 3) {
		n = MixerSourceSilence(mixer_id, source_no, silence);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u adding to source no %u silence %u ms\n",
				mixer_id, source_no, silence);
	} else if (sscanf(str, "drop %u %u %u", &mixer_id, &source_no, &drop) == 3) {
		n = MixerSourceDrop(mixer_id, source_no, drop);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u source no %u dropping %u ms\n",
				mixer_id, source_no, drop);

	} else if (sscanf(str, "maxdelay %u %u %u", &mixer_id, &source_no, &maxdelay) == 3) {
		n = MixerSourceMaxDelay(mixer_id, source_no, maxdelay);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u source no %u maxdelay %u ms\n",
				mixer_id, source_no, maxdelay);

	// audio mixer source map 1 1 1,1 ...
	} else if (sscanf(str, "map %u %u", &mixer_id, &source_no) == 2) {
		str += 3;
		while (*str && isspace(*str)) str++;
		while (*str && isdigit(*str)) str++;
		while (*str && isspace(*str)) str++;
		while (*str && isdigit(*str)) str++;
		while (*str && isspace(*str)) str++;
		if (!(*str)) return -1;
		n = SetSourceMap(mixer_id, source_no, str);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u source map %u %u %s\n",
				mixer_id, source_no, str);
	// audio mixer source invert <mixer id> <source id>
	} else if (sscanf(str, "invert %u %u", &mixer_id, &source_no) == 2) {
		n = SetSourceInvert(mixer_id, source_no);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u source inversion toggled\n",
				mixer_id, source_no);
	}
	return n;
}

// audio mixer status
int CAudioMixer::set_mixer_status(struct controller_type* ctr, const char* str)
{
	//u_int32_t id;
	//int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"mixer_id : state samples sps avg_sps silence dropped clipped delay rms");
		for (unsigned id=0 ; id < m_max_mixers; id++) if (m_mixers[id]) {
			audio_queue_t* pQueue = m_mixers[id]->pAudioQueues;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"\n"STATUS"audio mixer %u : %s %u %.0lf %.0lf %u %u %u %s",
				id, feed_state_string(m_mixers[id]->state),
				m_mixers[id]->samples,
				m_mixers[id]->samplespersecond,
				m_mixers[id]->total_avg_sps/MAX_AVERAGE_SAMPLES,
				m_mixers[id]->silence,
				m_mixers[id]->dropped,
				m_mixers[id]->clipped,
				pQueue ? "" : "0 ");
			while (pQueue) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "%u%s",
					1000*pQueue->sample_count/m_mixers[id]->calcsamplespersecond,
					pQueue->pNext ? "," : " ");
				pQueue = pQueue->pNext;
			}

			// RMS * 100 / 32767 = percentage
			for (u_int32_t i=0; i < m_mixers[id]->channels; i++) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%.1lf%s", (double) (sqrt(m_mixers[id]->rms[i])/327.67),
					i+1 < m_mixers[id]->channels ? "," : "");
			}
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				feed_state_enum state;
				if (pSource->source_type == 1 && m_pAudioFeed)
					state = m_pAudioFeed->GetState(pSource->source_id);
				else if (pSource->source_type == 2)
					state = GetState(pSource->source_id);
				else if (pSource->source_type == 3) {
					state = m_pVideoMixer->m_pAudioSink->GetState(pSource->source_id);
					fprintf(stderr, "sink source not supported yet for mixer<n");
				}
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"\n"STATUS"- source audio %s %2d : %s %u %u 0 %u %u %u %u ",
					pSource->source_type == 1 ? "feed " :
					  pSource->source_type == 2 ? "mixer" : "unknown",
					pSource->source_id,
					feed_state_string(state),
					pSource->samples,
					pSource->source_id,
					pSource->silence,
					pSource->dropped,
					pSource->clipped,
					(u_int32_t)(pSource->pAudioQueue ? 1000*pSource->pAudioQueue->sample_count/m_mixers[id]->calcsamplespersecond : 0));
				if (pSource->channels) {
					for (u_int32_t i=0; i < pSource->channels; i++) {
						m_pVideoMixer->m_pController->controller_write_msg (ctr,
							"%.1lf%s", (double) (sqrt(pSource->rms[i])/327.67),
						i+1 < pSource->channels ? "," : "");
					}
				} else m_pVideoMixer->m_pController->controller_write_msg (ctr, "0");
				pSource = pSource->next;
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"\n"STATUS"\n");
		return 0;

	}
	return -1;
}



// audio mixer drop <feed id> <ms>
int CAudioMixer::set_drop(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the mixer id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_mixers && m_mixers[id] && (m_mixers[id]->state == RUNNING ||
			m_mixers[id]->state == STALLED)) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"Frame %u - audio mixer %u dropping %u ms per channel\n",
					system_frame_no, id, ms);
			m_mixers[id]->drop_bytes = ms * sizeof(int32_t) *
				m_mixers[id]->channels * m_mixers[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio mixer silence <mixer id> <silence in ms>
int CAudioMixer::set_add_silence(struct controller_type* ctr, const char* str)
{
	if (!m_mixers || !str) return -1;
	while (isspace(*str)) str++;
	u_int32_t id, silence;
	int32_t n = -1;
	if ((sscanf(str, "%u %u", &id, &silence) == 2)) {
		if ((n = SetMixerSilence(id, silence))) return n;
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u added %u ms of silence\n",
				id, silence);
	}
	return n;
}

// Set mixer Silence
int CAudioMixer::SetMixerSilence(u_int32_t id, u_int32_t silence)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] || !m_mixers[id]->pAudioQueues)
		return -1;
	audio_queue_t* pQueue = m_mixers[id]->pAudioQueues;
	bool added = false;
	while (pQueue) {
		if (!AddSilence(pQueue, id, silence)) added = true;
		pQueue = pQueue->pNext;
	}
	if (added) m_mixers[id]->silence += (silence*m_mixers[id]->rate*m_mixers[id]->channels/1000);
	return 0;
}

int CAudioMixer::AddSilence(audio_queue_t* pQueue, u_int32_t id, u_int32_t ms)
{
	if (!pQueue || !ms || id >= m_max_mixers || !m_mixers || !m_mixers[id] ||
		!m_mixers[id]->channels || !m_mixers[id]->rate) return -1;
	u_int32_t added = 0;
	u_int32_t samples = ms*m_mixers[id]->channels*m_mixers[id]->rate/1000;
	u_int32_t size;
	while (added < samples) {
		audio_buffer_t* buf = GetEmptyBuffer(id, &size);
		if (!buf) {
			fprintf(stderr, "Failed to get buffer for adding silence\n");
			return 1;
		}
		memset(buf->data, 0, size);
		buf->mute = true;
		if (size >= (samples-added)*sizeof(int32_t)) {
			buf->len = (samples-added)*sizeof(int32_t);
			added += (samples-added);
		} else {
			buf->len = size;
			added += (size/sizeof(int32_t));
		}
		//m_mixers[id]->silence += (buf->len/(m_mixers[id]->channels*sizeof(int32_t)));
		if (m_verbose) fprintf(stderr, "audio mixer %u added %u samples\n",
			id, (u_int32_t)(buf->len/sizeof(int32_t)));
		AddAudioBufferToHeadOfQueue(pQueue, buf);
	}
	return 0;
}

// audio mixer mute (on | off) <mixer_id> 
int CAudioMixer::set_mixer_mute(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
                for (unsigned id=0 ; id < m_max_mixers; id++) {
                        if (!m_mixers[id]) continue;
                        // Write volume for mixer
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                STATUS"audio mixer %2d volume %s", id,
                                m_mixers[id]->pVolume ? "" : "0,");
                        if (m_mixers[id]->pVolume) {
                                float* p = m_mixers[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_mixers[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_mixers[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
                                m_mixers[id]->mute ? " muted" : "");

                        // And write volume for sources of mixer
                        audio_source_t* pSource = m_mixers[id]->pAudioSources;
                        while (pSource) {
                                m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                        STATUS"- source audio %s %2d volume %s",
                                        pSource->source_type == 1 ? "feed " :
                                          pSource->source_type == 2 ? "mixer" : "unknown",
                                        pSource->source_id,
                                        pSource->pVolume ? "" : "0");
                                if (pSource->pVolume) {
                                        float* p = pSource->pVolume;
                                        for (u_int32_t i=0 ; i < pSource->channels; i++, p++) {
                                                m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                        "%.3f%s", *p, i+1 < pSource->channels ? "," : "");
                                        }
                                }
                                m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
                                        pSource->mute ? " muted" : "");
                                pSource = pSource->next;
                        }
                }
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the mixer id.
	if ((n = sscanf(str, "on %u", &id)) == 1) {
		if ((n = SetMute(id, true)) == 0)
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"audio mixer %u mute\n", id);
		return n;
	}
	else if ((n = sscanf(str, "off %u", &id)) == 1) {
		if ((n = SetMute(id, false)) == 0)
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"audio mixer %u unmute\n", id);
		return n;
	}
	else return -1;
}

// audio mixer queue <mixer_id>
int CAudioMixer::set_mixer_queue(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return -1;
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned id=0 ; id < m_max_mixers; id++) if (m_mixers[id]) {
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				if (pSource->pAudioQueue) {
					int average_samples = QueueAverage(pSource->pAudioQueue);
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS"audio mixer %2d source audio %s %d : "
						"av. samp. %d = %d ms, cur. buf. %u, samp. %u = %u ms\n",
						id, pSource->source_type == 1 ? "feed" :
						    pSource->source_type == 2 ? "mixer" : "unknown",
						pSource->source_id,
						average_samples,
						1000*average_samples/(m_mixers[id]->rate*m_mixers[id]->channels),
						pSource->pAudioQueue->buf_count,
						pSource->pAudioQueue->sample_count,
						1000*pSource->pAudioQueue->sample_count/m_mixers[id]->calcsamplespersecond);
				}
				pSource = pSource->next;
			}
			audio_queue_t* pQueue = m_mixers[id]->pAudioQueues;
			while (pQueue) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %2d queue : buffers %u samples %u = %u ms\n",
					id, pQueue->buf_count,
					pQueue->sample_count,
					1000*pQueue->sample_count/m_mixers[id]->calcsamplespersecond);
						
				pQueue = pQueue->pNext;
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the mixer id.
	if ((n = sscanf(str, "%u", &id)) == 2) {
		//if (volume > MAX_VOLUME) volume = MAX_VOLUME;
		//if (m_verbose) fprintf(stderr, "audio mixer %u volume %u\n", id, volume);
		//return SetVolume(id, volume);
		return 0;
	}
	else return -1;
}

// audio mixer volume <mixer_id> <volume>
int CAudioMixer::set_mixer_move_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, steps;
	float volume_delta;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (unsigned id=0 ; id < m_max_mixers; id++) {
			if (!m_mixers[id]) continue;
			// Write volume for mixer
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio mixer %2d volume %s", id,
				m_mixers[id]->pVolume ? "" : "0,");
			if (m_mixers[id]->pVolume) {
                                float* p = m_mixers[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_mixers[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_mixers[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, " %s\n",
				"%smuted delta %.4f steps %u\n",
				m_mixers[id]->mute ? "" : "un",
				m_mixers[id]->volume_delta, m_mixers[id]->volume_delta_steps);

			// And write volume for sources of mixer
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"- source audio %s %2d volume %s",
					pSource->source_type == 1 ? "feed " :
					  pSource->source_type == 2 ? "mixer" : "unknown",
					pSource->source_id,
					pSource->pVolume ? "" : "0");
				if (pSource->pVolume) {
					float* p = pSource->pVolume;
					for (u_int32_t i=0 ; i < pSource->channels; i++, p++) {
                                        	m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                	"%.3f%s", *p, i+1 < pSource->channels ? "," : "");
					}
				}
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
					"%smuted delta %.4f steps %u\n",
					pSource->mute ? " " : " un",
					pSource->volume_delta, pSource->volume_delta_steps);
				pSource = pSource->next;
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the mixer id.
	if ((n = sscanf(str, "%u %f %u", &id, &volume_delta, &steps)) == 3) {
		n = SetMoveVolume(id, volume_delta, steps);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u delta volume %.4f steps %u\n",
				id, volume_delta, steps);
		return n;
	}
	else return -1;
}

// audio mixer volume <mixer_id> <volume>
int CAudioMixer::set_mixer_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channel;
	int n;
	float volume;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (unsigned id=0 ; id < m_max_mixers; id++) {
			if (!m_mixers[id]) continue;
			// Write volume for mixer
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio mixer %2d volume %s", id,
				m_mixers[id]->pVolume ? "" : "0,");
			if (m_mixers[id]->pVolume) {
                                float* p = m_mixers[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_mixers[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_mixers[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_mixers[id]->mute ? " muted" : "");

			// And write volume for sources of mixer
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"- source audio %s %2d volume %s",
					pSource->source_type == 1 ? "feed " :
					  pSource->source_type == 2 ? "mixer" : "unknown",
					pSource->source_id,
					pSource->pVolume ? "" : "0");
				if (pSource->pVolume) {
					float* p = pSource->pVolume;
					for (u_int32_t i=0 ; i < pSource->channels; i++, p++) {
                                        	m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                	"%.3f%s", *p, i+1 < pSource->channels ? "," : "");
					}
				}
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
					pSource->mute ? " muted" : "");
//m_pVideoMixer->m_pController->controller_write_msg (ctr, "Source Channels %u\n", pSource->channels);
				pSource = pSource->next;
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the mixer id.
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
                                MESSAGE"audio mixer %u volume %s\n", id, volume_list);
                return 0;
        }
        else return -1;
}

// audio mixer add [<mixer id> [<mixer name>]] // Create/delete/list an audio mixer
int CAudioMixer::set_mixer_add(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u <%s>\n", id,
					m_mixers[id]->name ? m_mixers[id]->name : "");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the mixer id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;

	// Now if eos, only id was given and we delete the mixer
	if (!(*str)) {
		if ((n = DeleteMixer(id)) == 0) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"Deleted audio mixer %u\n",id);
		}
		return n;
	} 

	// Now we know there is more after the id and we can create the mixer
	if ((n = CreateMixer(id)) == 0) {
		if ((n = SetMixerName(id, str)) == 0) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"audio mixer %u <%s> added\n",
					id, m_mixers[id]->name);
		} else DeleteMixer(id);
	}
	return n;
}

// audio mixer start [<mixer id>] // Start a mixer
int CAudioMixer::set_mixer_start(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u state %s\n", id,
					m_mixers[id]->state == RUNNING ? "running" :
					  m_mixers[id]->state == PENDING ? "pending" :
					    m_mixers[id]->state == READY ? "ready" :
					      m_mixers[id]->state == SETUP ? "setup" :
						m_mixers[id]->state == STALLED ? "stalled" :
						  m_mixers[id]->state == DISCONNECTED ? "disconnected" :
						    "unknown");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the mixer id.
	if ((n = sscanf(str, "soft %u", &id)) == 1) return StartMixer(id, true);
	if ((n = sscanf(str, "%u", &id)) == 1) return StartMixer(id, false);
	return -1;
}

// audio mixer stop [<mixer id>] // Start a mixer
int CAudioMixer::set_mixer_stop(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u state %s\n", id,
					m_mixers[id]->state == RUNNING ? "running" :
					  m_mixers[id]->state == PENDING ? "pending" :
					    m_mixers[id]->state == READY ? "ready" :
					      m_mixers[id]->state == SETUP ? "setup" :
						m_mixers[id]->state == STALLED ? "stalled" :
						  m_mixers[id]->state == DISCONNECTED ? "disconnected" :
						    "unknown");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the mixer id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	return StopMixer(id);
}

// audio mixer channels [<mixer id> <channels>]] // Set number of channels for an audio mixer
int CAudioMixer::set_mixer_channels(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channels;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list channels
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u channels %u\n", id,
					m_mixers[id]->channels);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the mixer id.
	if ((n = sscanf(str, "%u %u", &id, &channels)) != 2) return -1;
	if ((n = SetMixerChannels(id, channels)) == 0) {
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u channels %u\n",
				id, channels);
	}
	return n;
}

// audio mixer rate [<mixer id> <rate in Hz>]] // Set sample rate for an audio mixer
int CAudioMixer::set_mixer_rate(struct controller_type* ctr, const char* str)
{
	u_int32_t id, rate;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u <%s> rate %u Hz\n", id,
					m_mixers[id]->name ? m_mixers[id]->name : "",
					m_mixers[id]->rate);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the mixer id and rate
	if ((n = sscanf(str, "%u %u", &id, &rate)) != 2) return -1;
	if ((n = SetMixerRate(id, rate)) == 0) {
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio mixer %u sample rate %u Hz\n",
				id, rate);
	}
	return n;
}

/*
// audio mixer format [<mixer id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
// Set format for audio mixer
int CAudioMixer::set_mixer_format(struct controller_type* ctr, const char* str)
{
	u_int32_t id, bits;
	int n;
	bool is_signed = false;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_mixers) return 1;
		for (id = 0; id < m_max_mixers ; id++)
			if (m_mixers[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio mixer %u <%s> %s and %u bytes per "
					"sample\n", id,
					m_mixers[id]->name ? m_mixers[id]->name : "",
					m_mixers[id]->is_signed ? "signed" : "unsigned",
					m_mixers[id]->bytespersample);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the mixer id.
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
	} else return 0;
	return SetMixerFormat(id, bits >> 3, is_signed);
}
*/

int CAudioMixer::CreateMixer(u_int32_t id)
{
	if (id >= m_max_mixers || !m_mixers) return -1;
	audio_mixer_t* p = m_mixers[id];
	if (!p) p = (audio_mixer_t*) calloc(sizeof(audio_mixer_t),1);
	if (!p) return -1;
	m_mixers[id]		= p;
	p->name			= NULL;
	p->id			= id;
	p->state		= SETUP;
	p->channels		= 0;
	p->rate			= 0;
	p->is_signed		= true;
	p->bytespersample	= sizeof(int32_t);
	p->bytespersecond	= 0;
	p->calcsamplespersecond	= 0;
	p->mute			= true;
	p->start_time.tv_sec	= 0;
	p->start_time.tv_usec	= 0;
	p->buffer_size		= 0;
	p->pAudioSources	= NULL;
	p->no_sources		= 0;
	p->pAudioQueues		= NULL;
	//p->monitor_queue	= NULL;
	p->buf_seq_no		= 0;
	p->pVolume		= NULL;
	p->sample_count		= 0;
	p->drop_bytes		= 0;
	p->samples		= 0;
	p->silence		= 0;
	p->dropped		= 0;
	p->clipped		= 0;
	p->rms			= NULL;

	p->update_time.tv_sec   = 0;
	p->update_time.tv_usec  = 0;
	p->samplespersecond     = 0.0;
	p->last_samples         = 0;
	p->total_avg_sps        = 0.0;
	for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
        	p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index        = 0;
	m_mixer_count++;
	return 0;
}

int CAudioMixer::DeleteSource(audio_source_t* pSource)
{
	if (!pSource) return 1;
	source_map_t* pMap = pSource->pSourceMap;
	while (pMap) {
		source_map_t* tmp = pMap->next;
		free(pMap);
		pMap = tmp;
	}
	pSource->pSourceMap = NULL;		// not necessary, but lets do it anyway
	if (pSource->rms) free(pSource->rms);
	if (pSource->pVolume) free(pSource->pVolume);
	//pSource->rms = NULL;			// not necessary ....
	//pSource->pVolume = NULL;		// not necessary ....
	//pSource->next = NULL;			// not necessary ....
	free(pSource);
	return 0;
}

int CAudioMixer::DeleteMixer(u_int32_t id)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id]) return -1;
	if (m_mixers[id]->name) free(m_mixers[id]->name);
	if (m_mixers[id]->rms) free(m_mixers[id]->rms);
	if (m_mixers[id]->pVolume) free(m_mixers[id]->pVolume);
        while(m_mixers[id]->pAudioSources) {
		audio_source_t* next = m_mixers[id]->pAudioSources->next;
		DeleteSource(m_mixers[id]->pAudioSources);
		m_mixers[id]->pAudioSources = next;
	}
	free(m_mixers[id]);
	m_mixers[id] = NULL;
	if (m_mixer_count) m_mixer_count--;
	return 0;
}

int CAudioMixer::SetMixerName(u_int32_t id, const char* mixer_name)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] || !mixer_name)
		return -1;
	m_mixers[id]->name = strdup(mixer_name);
	trim_string(m_mixers[id]->name);
	return m_mixers[id]->name ? 0 : -1;
}

int CAudioMixer::SetMixerChannels(u_int32_t id, u_int32_t channels)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id])
		return -1;
	m_mixers[id]->channels = channels;
	if (m_mixers[id]->rms) free(m_mixers[id]->rms);
	if (m_mixers[id]->pVolume) free(m_mixers[id]->pVolume);
	m_mixers[id]->rms = (u_int64_t*) calloc(channels,sizeof(u_int64_t));
	m_mixers[id]->pVolume = (float*) calloc(channels,sizeof(float));
	if (!m_mixers[id]->pVolume || !m_mixers[id]->rms) {
                if (m_mixers[id]->rms) free(m_mixers[id]->rms);
                if (m_mixers[id]->pVolume) free(m_mixers[id]->pVolume);
                m_mixers[id]->rms = NULL;
                m_mixers[id]->pVolume = NULL;
                return 1;
        }
        for (u_int32_t i=0 ; i < channels ; i++) m_mixers[id]->pVolume[i] = DEFAULT_VOLUME_FLOAT;
	SetMixerBufferSize(id);
	return 0;
}

int CAudioMixer::SetMixerRate(u_int32_t id, u_int32_t rate)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id])
		return -1;
	m_mixers[id]->rate = rate;
	SetMixerBufferSize(id);
	return 0;
}

int CAudioMixer::SetMixerBufferSize(u_int32_t id)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] || !m_pVideoMixer ||
		m_pVideoMixer->m_frame_rate == 0)
		return -1;
	int32_t size = m_mixers[id]->channels*m_mixers[id]->bytespersample*
		m_mixers[id]->rate/ m_pVideoMixer->m_frame_rate;
	if (!size) return -1;
	if (m_verbose) fprintf(stderr, "audio mixer %u needs "
		"%u bytes per video frame\n", id, (unsigned) size);
	m_mixers[id]->bytespersecond = m_mixers[id]->bytespersample*
		m_mixers[id]->channels*m_mixers[id]->rate;
	m_mixers[id]->calcsamplespersecond = m_mixers[id]->channels*m_mixers[id]->rate;
	int32_t n = 0;
	for (n=0 ; size ; n++) size >>=  1;
	m_mixers[id]->buffer_size = (1 << n);
	if (m_verbose) fprintf(stderr, "audio mixer %u buffer size set to %u bytes\n",
		id, m_mixers[id]->buffer_size);
	if (m_mixers[id]->buffer_size > 0) {
		if (m_mixers[id]->state != READY && m_verbose)
			fprintf(stderr, "audio mixer %u changed state from %s to %s\n",
				id, feed_state_string(m_mixers[id]->state),
				feed_state_string(READY));
		m_mixers[id]->state = READY;
	} else {
		if (m_mixers[id]->state != SETUP && m_verbose)
			fprintf(stderr, "audio mixer %u changed state from %s to %s\n",
				id, feed_state_string(m_mixers[id]->state),
				feed_state_string(SETUP));
		m_mixers[id]->state = SETUP;
	}

	return m_mixers[id]->buffer_size;
}

// Returns the number of channels for mixer id
u_int32_t CAudioMixer::GetChannels(u_int32_t id)
{
	// Make checks
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id]) return 0;
	return (m_mixers[id]->channels);
}

// Returns the sample rate for mixer id
u_int32_t CAudioMixer::GetRate(u_int32_t id)
{
	// Make checks
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id]) return 0;
	return (m_mixers[id]->rate);
}

// Returns the number bytes per sample for mixer id
u_int32_t CAudioMixer::GetBytesPerSample(u_int32_t id)
{
	// Make checks
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id]) return 0;
	return (m_mixers[id]->bytespersample);
}

audio_buffer_t* CAudioMixer::GetEmptyBuffer(u_int32_t id, u_int32_t* size)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] || !m_mixers[id]->buffer_size)
		return NULL;
	audio_buffer_t* p = (audio_buffer_t*)
		malloc(sizeof(audio_buffer_t)+m_mixers[id]->buffer_size+m_mixers[id]->channels*sizeof(u_int64_t));
	if (p) {
		if (size) *size = m_mixers[id]->buffer_size;
		p->data = ((u_int8_t*)p)+sizeof(audio_buffer_t);
		p->len = 0;
		p->next = NULL;
		p->mute = false;
		p->clipped = false;
		p->channels     = m_mixers[id]->channels;
                p->rate         = m_mixers[id]->rate;
                p->rms          = (u_int64_t*)(p->data+m_mixers[id]->buffer_size);
                for (u_int32_t i=0 ; i < m_mixers[id]->channels ; i++)
                        p->rms[i] = 0;

	}
	return p;
}

// Set volume level for mixer
int CAudioMixer::SetMoveVolume(u_int32_t mixer_id, float volume_delta, u_int32_t steps)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id]) return -1;
	m_mixers[mixer_id]->volume_delta = volume_delta;
	m_mixers[mixer_id]->volume_delta_steps = steps;
	if (m_animation < steps) m_animation = steps;
	return 0;
}

int CAudioMixer::SetVolume(u_int32_t mixer_id, u_int32_t channel, float volume)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		channel >= m_mixers[mixer_id]->channels) return -1;
	if (volume > MAX_VOLUME_FLOAT) volume = MAX_VOLUME_FLOAT;
	else if (volume < 0.0) volume = 0.0;
	m_mixers[mixer_id]->pVolume[channel] = volume;
	return 0;
}

int CAudioMixer::SetMute(u_int32_t mixer_id, bool mute)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id]) return -1;
	m_mixers[mixer_id]->mute = mute;
	return 0;
}

int CAudioMixer::MixerSourceVolume(u_int32_t mixer_id, u_int32_t source_no, u_int32_t channel, float volume)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource || channel >= pSource->channels || !pSource->pVolume) return -1;
	if (volume > MAX_VOLUME_FLOAT) volume = MAX_VOLUME_FLOAT;
	else if (volume < 0.0) volume = 0.0;
	pSource->pVolume[channel] = volume;
	return 0;
}

int CAudioMixer::MixerSourceMoveVolume(u_int32_t mixer_id, u_int32_t source_no, float volume_delta, u_int32_t delta_steps)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource) return -1;
	pSource->volume_delta = volume_delta;
	pSource->volume_delta_steps = delta_steps;
//fprintf(stderr, "Move volume source %f %u set\n", volume_delta, delta_steps);
	if (m_animation < delta_steps) m_animation = delta_steps;
	return 0;
}

int CAudioMixer::MixerSourceSilence(u_int32_t mixer_id, u_int32_t source_no, u_int32_t silence)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource || !pSource->pAudioQueue) return -1;
	return AddSilence(pSource->pAudioQueue, mixer_id, silence);
}

int CAudioMixer::MixerSourceDrop(u_int32_t mixer_id, u_int32_t source_no, u_int32_t drop)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource || !pSource->pAudioQueue) return -1;
	pSource->drop_bytes = drop * sizeof(int32_t) *
				pSource->channels * pSource->rate/1000;
	return 0;
}

int CAudioMixer::SetSourceInvert(u_int32_t mixer_id, u_int32_t source_no)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource || !pSource->pAudioQueue) return -1;
	pSource->invert = !pSource->invert;
	return 0;
}

int CAudioMixer::MixerSourceMaxDelay(u_int32_t mixer_id, u_int32_t source_no, u_int32_t maxdelay)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource) return -1;
	pSource->max_delay_in_bytes = maxdelay * sizeof(int32_t) *
				pSource->channels * pSource->rate/1000;
	if (pSource->pAudioQueue) pSource->pAudioQueue->max_bytes = pSource->max_delay_in_bytes;
	return 0;
}

int CAudioMixer::SetSourceMap(u_int32_t mixer_id, u_int32_t source_no, const char* str)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources || !str || !(*str)) return -1;

	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 0;
	u_int32_t src, dst;
	while (pSource) {
		if (++no == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource) return -1;
	bool set = false;
	while ((sscanf(str, "%u,%u", &src, &dst) == 2)) {
		u_int32_t src_channels = pSource->source_type == 1 ?
			m_pAudioFeed->GetChannels(pSource->source_id) :
			  pSource->source_type == 2 ? GetChannels(pSource->source_id) : 0;
		source_map_t* pMap = (source_map_t*) calloc(sizeof(source_map_t), 1);
		if (pMap && src < src_channels && dst < m_mixers[mixer_id]->channels) {
			pMap->next = pSource->pSourceMap;
			pMap->source_index = src;
			pMap->mix_index = dst;
			pSource->pSourceMap = pMap;
			set = true;
		} else return 1;

		while (*str && isdigit(*str)) str++;
		str++;	// the comma
		while (*str && isdigit(*str)) str++;
		while (*str && isspace(*str)) str++;
	}

	return set ? 0 : -1;
}

int CAudioMixer::MixerSourceMute(u_int32_t mixer_id, u_int32_t source_no, bool mute)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!m_mixers[mixer_id]->pAudioSources) return -1;
	audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
	u_int32_t no = 1;
	while (pSource) {
		if (no++ == source_no) break;
		pSource = pSource->next;
	}
	if (!pSource) return -1;
	pSource->mute = mute;
	return 0;
}

int CAudioMixer::MixerSource(u_int32_t mixer_id, u_int32_t source_id, int source_type)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		source_type < 1 || source_type > 3  ||
		(source_type == 2 && mixer_id == source_id)) return -1;
	if (source_type == 2 && (source_id >= m_max_mixers || !m_mixers[source_id])) return -1;
	audio_source_t* pSource = NULL;
	if (source_type == 1 || source_type == 2) {		// Type is audio feed;
		if (source_type == 1 && !m_pAudioFeed) {
			if (m_verbose) fprintf(stderr, "audio mixer %u has audio feed as source, "
				"but CAudioFeed is null\n", mixer_id);
			return 1;
		}
		if (m_mixers[mixer_id]->rate != (source_type == 1 ?
			m_pAudioFeed->GetRate(source_id) : GetRate(source_id))) {
			if (m_verbose) fprintf(stderr, "Rate mismatch mixer rate %u source rate %u\n",
				m_mixers[mixer_id]->rate,
				  source_type == 1 ? m_pAudioFeed->GetRate(source_id) :
				  GetRate(source_id));
			return -1;
		}

		if (!(pSource = (audio_source_t*) calloc(sizeof(audio_source_t), 1))) {
			if (m_verbose) fprintf(stderr,
				"audio mixer %u could not allocate space for source",
				mixer_id);
			return 1;
		}
		pSource->source_type	= source_type;
		pSource->source_id	= source_id;
		pSource->pVolume	= NULL;
		pSource->mute		= true;
		pSource->rate		= 0;
		pSource->channels	= 0;
		pSource->samples	= 0;
		pSource->got_samples	= false;
		pSource->silence	= 0;
		pSource->dropped	= 0;
		pSource->clipped	= 0;
		pSource->max_delay_in_bytes = 0;
		pSource->pSourceMap	= NULL;
		pSource->next		= NULL;
		
		bool add_queue = (m_mixers[mixer_id]->state == PENDING ||
			m_mixers[mixer_id]->state == RUNNING ||
			m_mixers[mixer_id]->state == STALLED);
		AddQueueToSource(pSource, add_queue);
		if (add_queue) {

			if (!(pSource->pAudioQueue)) {
				if (m_verbose) fprintf(stderr,
					"audio mixer %u could not get audio queue for "
					"audio %s %u", mixer_id, source_id == 1 ?
					"feed" : "mixer", source_id);
				free(pSource);
				return -1;
			}
			pSource->pAudioQueue->invert = pSource->invert;
			pSource->pAudioQueue->max_bytes = pSource->max_delay_in_bytes;
			if (pSource->channels) pSource->rms = (u_int64_t*)
				calloc(pSource->channels, sizeof(u_int64_t));
		}
		if (!m_mixers[mixer_id]->pAudioSources)
			m_mixers[mixer_id]->pAudioSources = pSource;
		else {
			audio_source_t* p = m_mixers[mixer_id]->pAudioSources;
			while (p->next) p = p->next;
			p->next = pSource;
		}
		//pSource->next = m_mixers[mixer_id]->pAudioSources;
		//m_mixers[mixer_id]->pAudioSources = pSource;
		m_mixers[mixer_id]->no_sources++;
	} else return -1;
	if (m_mixers[mixer_id]->state == PENDING || m_mixers[mixer_id]->state == RUNNING ||
			m_mixers[mixer_id]->state == STALLED) {
		if (m_verbose)
			fprintf(stderr, "audio mixer %u added queue to audio %s %u\n",
				mixer_id, source_type == 1 ? "feed" :
				  source_type == 2 ? "mixer" : "none?????",
				source_id);
	}
		
	return 0;
}


int CAudioMixer::StartMixer(u_int32_t id, bool soft)
{
	// make checks
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] || m_mixers[id]->state != READY)
		return -1;
	audio_mixer_t* p = m_mixers[id];
	// We can not start a mixer, if it has no sources
	if (!p->pAudioSources) {
		if (m_verbose) fprintf(stderr, "audio mixer %u can not start "
			"as it has no sources\n", id);
		return -1;
	}
/*
	// We refuse to start a mixer that has no queues (no one using it).
	// It will need a sink or another mixer started succesfully
	if (!p->pAudioQueues) {
		if (m_verbose) fprintf(stderr, "audio mixer %u can not start "
			"as it has no queues (no one using it)\n", id);
		return -1;
	}
*/


	// We will now set the state to pending. State will change to running
	// in Update().
	p->state = PENDING;
	if (m_verbose) fprintf(stderr, "audio mixer %u changed state to PENDING\n", id);
	

	// Reset counter
	p->sample_count = 0;

	p->update_time.tv_sec           = 0;
        p->update_time.tv_usec          = 0;
        p->samplespersecond             = 0.0;
        p->last_samples                 = 0;
        p->total_avg_sps                = 0.0;
        for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
                p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index                = 0;

	if (!soft) {
		timeval timenow;
		gettimeofday(&timenow, NULL);
		ConditionalStart(id, &timenow, true);
	}

/*
audio_source_t* pSource = p->pAudioSources;
int n= 1;
	while (pSource) {
		fprintf(stderr, "Start for source %d\n", n++);
		pSource = pSource->next;
	}
*/

	return 0;
}

int CAudioMixer::StopMixer(u_int32_t id)
{
	// make checks
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] ||
		(m_mixers[id]->state != RUNNING && m_mixers[id]->state != STALLED &&
		m_mixers[id]->state != PENDING))
		return -1;
	audio_mixer_t* p = m_mixers[id];

	// We must drop sources before stopping
	if (p->pAudioSources) {
		audio_source_t* pSource = p->pAudioSources;
		int n = 1;
		while (pSource) {
			if (pSource->pAudioQueue) {
				if (pSource->source_type == 1) {
					if (m_pAudioFeed && m_pAudioFeed->RemoveQueue(pSource->source_id, pSource->pAudioQueue)) {
						if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u "
							"removed source %d queue from audio feed %u\n",
							system_frame_no, id, n, pSource->source_id);
					} else fprintf(stderr, "Frame %u - audio mixer %u "
							"failed removing source %d queue from audio feed %u\n",
							system_frame_no, id, n, pSource->source_id);
				}
				else if (pSource->source_type == 2) {
					if (RemoveQueue(pSource->source_id, pSource->pAudioQueue)) {
						if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u "
							"removed source %d queue from audio mixer %u\n",
							system_frame_no, id, n, pSource->source_id);
					} else fprintf(stderr, "Frame %u - audio mixer %u "
							"failed removing source %d queue from audio mixer %u\n",
							system_frame_no, id, n, pSource->source_id);
				}
				else if (pSource->source_type == 3) {
					if (m_pVideoMixer && m_pVideoMixer->m_pAudioSink &&
						m_pVideoMixer->m_pAudioSink->RemoveQueue(pSource->source_id, pSource->pAudioQueue)) {
						if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u "
							"removed source %d queue from audio sink %u\n",
							system_frame_no, id, n, pSource->source_id);
					} else fprintf(stderr, "Frame %u - audio mixer %u "
							"failed removing source %d queue from audio sink %u\n",
							system_frame_no, id, n, pSource->source_id);
				}
				pSource->pAudioQueue = NULL;
			}
			pSource = pSource->next;
			n++;
		}
	}
	feed_state_enum state = p->state;
	p->state = SETUP;
	if (m_verbose > 1) fprintf(stderr, "Frame %u - audio mixer %u changed state from %s to %s\n",
		system_frame_no, id, feed_state_string(state),
		feed_state_string(p->state));

	// We call SetMixerBufferSize to see if we can change state to READY (done by that)
	SetMixerBufferSize(id);

	return 0;
}

bool CAudioMixer::RemoveQueue(u_int32_t mixer_id, audio_queue_t* pQueue)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id] ||
		!pQueue || !m_mixers[mixer_id]->pAudioQueues) return false;
	audio_queue_t* p = m_mixers[mixer_id]->pAudioQueues;
	bool found = false;
	if (p == pQueue) {
		m_mixers[mixer_id]->pAudioQueues = pQueue->pNext;
		found = true;
		if (m_verbose) fprintf(stderr, "audio mixer %u removing first audio queue\n", mixer_id);
	} else {
		while (p) {
			if (p->pNext == pQueue) {
				p->pNext = pQueue->pNext;
				found = true;
				if (m_verbose) fprintf(stderr, "audio mixer %u removing "
					"audio queue\n", mixer_id);
				break;
			}
		}
	}
	if (found) {
		if (m_verbose > 1) fprintf(stderr, "audio mixer %u freeing %u buffers and %u bytes\n",
			mixer_id, pQueue->buf_count, pQueue->bytes_count);
		audio_buffer_t* pBuf = pQueue->pAudioHead;
		while (pBuf) {
			audio_buffer_t* next = pBuf->next;
			free(pBuf);
			pBuf = next;
		}
		free(pQueue);

		// Now check if we have any queues to write to. If not, remove source queues
		if (!m_mixers[mixer_id]->pAudioQueues && m_mixers[mixer_id]->pAudioSources) {
			audio_source_t* pSource = m_mixers[mixer_id]->pAudioSources;
			while (pSource) {
				if (m_verbose) fprintf(stderr,
					"Frame %u - audio mixer %u removing source queue for audio %s %u\n",
					system_frame_no, mixer_id, pSource->source_type == 1 ? "feed" :
					  pSource->source_type == 2 ? "mixer" : "unknown",
					pSource->source_id);
				if (pSource->source_type == 1)
					m_pAudioFeed->RemoveQueue(pSource->source_id, pSource->pAudioQueue);
				else RemoveQueue(pSource->source_id, pSource->pAudioQueue);
				pSource->pAudioQueue = NULL;
				//if (pSource->state == RUNNING || pSource->state == STALLED) {
				//	if (m_verbose > 1) fprintf(stderr,
				//		"Frame %u - audio mixer %u changing state from %s to %s\n",
				//		system_frame_no, mixer_id,
				//		feed_state_string(pSource->state),
				//		feed_state_string(PENDING));
				//	pSource->state = PENDING;
				//}
				pSource = pSource->next;
			}
			if (m_mixers[mixer_id]->state == RUNNING || m_mixers[mixer_id]->state == STALLED) {
				if (m_verbose) fprintf(stderr, "audio mixer %u changing state to PENDING\n", mixer_id);
				m_mixers[mixer_id]->state = PENDING;
			}
			m_mixers[mixer_id]->start_time.tv_sec	= 0;
			m_mixers[mixer_id]->start_time.tv_usec	= 0;
			m_mixers[mixer_id]->sample_count	= 0;
		}
	}
	if (!m_mixers[mixer_id]->pAudioQueues) {
		if (m_mixers[mixer_id]->state == RUNNING || m_mixers[mixer_id]->state == STALLED) {
			if (m_verbose > 1) fprintf(stderr, "Frame %u - audio mixer %u changing state from %s to %s\n",
				system_frame_no, mixer_id,
				feed_state_string(m_mixers[mixer_id]->state),
				feed_state_string(PENDING));
			m_mixers[mixer_id]->state = PENDING;
		}
	}
	return found;
}

void CAudioMixer::QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples)
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


audio_queue_t* CAudioMixer::AddQueue(u_int32_t mixer_id)
{
	if (mixer_id >= m_max_mixers || !m_mixers || !m_mixers[mixer_id]) return NULL;
	audio_queue_t* pQueue = (audio_queue_t*) calloc(sizeof(audio_queue_t),1);
	if (pQueue) {
		pQueue->buf_count = 0;
		pQueue->bytes_count = 0;
		pQueue->async_access = false;
		//pQueue->queue_samples[]       = 0;    // this is zeroed by calloc
		pQueue->qp	      = 0;
		pQueue->queue_total     = 0;
		pQueue->queue_max       = DEFAULT_QUEUE_MAX_SECONDS*m_mixers[mixer_id]->rate*
			m_mixers[mixer_id]->channels;
		pQueue->pNext = m_mixers[mixer_id]->pAudioQueues;
		m_mixers[mixer_id]->pAudioQueues = pQueue;
		if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u was added an audio queue\n",
			system_frame_no, mixer_id);
	}
	return pQueue;
}

// Get Audio mixer state
feed_state_enum CAudioMixer::GetState(u_int32_t id) {
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id]) return UNDEFINED;
	return m_mixers[id]->state;
}


// AddAudioToMixer()
// Adding a buffer with audio samples to an audio mixer
// The buffer is dropped if the mixer doesn't exist or if the mixer
// has no audio queue(s). The buffer is added to end of the list.
// The volume for the mixer is applied to the sample
//
void CAudioMixer::AddAudioToMixer(u_int32_t id, audio_buffer_t* buf)
{
	if (!buf || id >= m_max_mixers || !m_mixers || !m_mixers[id] || 
		!m_mixers[id]->pAudioQueues) {
		if (buf) {
			free(buf);
		}
		return;
	}

	if (m_mixers[id]->drop_bytes) {
		u_int32_t drop_bytes = buf->len >= m_mixers[id]->drop_bytes ?
			m_mixers[id]->drop_bytes : buf->len;
		u_int32_t samples = drop_bytes/(m_mixers[id]->channels*m_mixers[id]->bytespersample);
		buf->len -= drop_bytes;
		buf->data += drop_bytes;
		m_mixers[id]->drop_bytes -= drop_bytes;
		m_mixers[id]->dropped += samples;
		if (m_verbose) fprintf(stderr,
			"Frame %u - audio mixer %u dropping %u samples per channel\n",
			system_frame_no, id, samples);
	}



	// First we assign a seq no to the buffer.
	buf->seq_no = m_mixers[id]->buf_seq_no++;

	// Then we check if we should mute it
	if (m_mixers[id]->mute) buf->mute = true;

	// Then we set the volume
	if (buf->channels == m_mixers[id]->channels && m_mixers[id]->pVolume)
		SetVolumeForAudioBuffer(buf, m_mixers[id]->pVolume);
	else if (m_verbose > 2) fprintf(stderr,
		"Frame %u - audio sink %u volume error : %s\n",
		system_frame_no, id,
		buf->channels == m_mixers[id]->channels ?
		"missing volume" : "channel mismatch");

	// Check for clipping
	if (buf->clipped) m_mixers[id]->clipped = 100;

	// Now we calculate the RMS sum for the buffer
	MakeRMS(buf);
	if (m_mixers[id]->rms) for (u_int32_t i=0 ; i < m_mixers[id]->channels ; i++) {
		if (buf->rms[i] > m_mixers[id]->rms[i]) m_mixers[id]->rms[i] = buf->rms[i];
	}
	// Now we will add the buffer to the tail of the queue(s),

	audio_queue_t* paq = m_mixers[id]->pAudioQueues;
	buf->next = NULL;
	if (m_verbose > 3) fprintf(stderr,
		"Frame %u - audio mixer %u addaudiotomixer buffer seq no %u with %u buf count %u\n",
		system_frame_no, id, buf->seq_no, (u_int32_t) (buf->len/sizeof(int32_t)), paq->buf_count);
		// we go to next queue, if any
	m_mixers[id]->samples += (buf->len/sizeof(int32_t));

	// Now add buffer to queue or queues. In the latter case a copy is added for each queue.
	AddBufferToQueues(m_mixers[id]->pAudioQueues, buf, 2, id, system_frame_no, m_verbose);
}

void CAudioMixer::AddQueueToSource(audio_source_t* pSource, bool add_queue)
{
	if (!pSource) return;
	u_int32_t channels = pSource->channels;

	// if source is audio feed, then add queue to audio feed
	if (pSource->source_type == 1) {
		pSource->channels = m_pAudioFeed->GetChannels(pSource->source_id);
		pSource->rate = m_pAudioFeed->GetRate(pSource->source_id);
		if (add_queue) pSource->pAudioQueue = m_pAudioFeed->AddQueue(pSource->source_id);

	// else if source is another audio mixer, then add queue to audio mixer
	// Its a bad idea to connect a mixer to it self.
	} else if (pSource->source_type == 2) {
		pSource->channels = GetChannels(pSource->source_id);
		pSource->rate = GetRate(pSource->source_id);
		if (add_queue) pSource->pAudioQueue = AddQueue(pSource->source_id);
	} else if (pSource->source_type == 3) {
		pSource->channels = m_pVideoMixer->m_pAudioSink->GetChannels(pSource->source_id);
		pSource->rate = m_pVideoMixer->m_pAudioSink->GetRate(pSource->source_id);
		if (add_queue) pSource->pAudioQueue =
			m_pVideoMixer->m_pAudioSink->AddQueue(pSource->source_id);
	} else {
		fprintf(stderr, "AddQueueToSource source type error. Wrong type\n");
	}
	if (pSource->channels > 0) {
		if (channels != pSource->channels || !pSource->pVolume) {
			if (pSource->pVolume) free(pSource->pVolume);
			pSource->pVolume = (float*) calloc(sizeof(float),
				pSource->channels);
			if (pSource->pVolume) {
				float* p = pSource->pVolume;
				for (u_int32_t i=0; i < pSource->channels; i++) p[i] = 1.0;
			} else fprintf(stderr, "failed to allocate volume for source\n");
		}

		if (channels != pSource->channels || !pSource->rms) {
			if (pSource->rms) free(pSource->rms);
			pSource->rms = (u_int64_t*) calloc(pSource->channels,
				sizeof(u_int64_t));
		}
	} else {
		if (pSource->pVolume) free(pSource->pVolume);
		if (pSource->rms) free(pSource->rms);
		pSource->pVolume = NULL;
		pSource->rms = NULL;
	}
}

bool CAudioMixer::ConditionalStart(u_int32_t id, timeval* pTimenow, bool hardstart)
{
	if (id >= m_max_mixers || m_mixers[id]->state != PENDING) return false;

	// First we check sources and queues just to be sure
	// If it has no souces, then we postpone
	if (!m_mixers[id]->pAudioSources) {
		if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u state "
			"changed from PENDING to READY. Has no sources\n",
			system_frame_no, id);
			m_mixers[id]->state = READY;
		return false;
	}

	// Then we check for consumers. If it has no consumers yet, then we postpone
	// changing state
	if (!m_mixers[id]->pAudioQueues) return false;
	if (m_verbose > 2) fprintf(stderr,
		"Frame %u - audio mixer %u pending, will check sources. Should have %u sources\n",
		system_frame_no, id, m_mixers[id]->no_sources);

	if (m_verbose>3) fprintf(stderr, "Frame %u - audio mixer %u has queue(s) "
		"(consumers - sinks or mixers\n", system_frame_no, id);

	// Now we have both sources and consumers/sinks/queues
	// Time to add queues to sources 
	audio_source_t* pSource = m_mixers[id]->pAudioSources;
	while (pSource) {
		// skip to next if AudioSource alrady had an AudioQueue attached
		if (pSource->pAudioQueue) {
			if (m_verbose>3) fprintf(stderr,
				"Frame %u - audio mixer %u audio source was "
				"already set to audio %s %u\n", system_frame_no, id,
				pSource->source_type == 1 ? "feed" :
				  pSource->source_type == 2 ? "mixer" : "unknown",
				pSource->source_id);
			pSource = pSource->next;
			continue;
		}
		AddQueueToSource(pSource, true);

		if (!pSource->pAudioQueue) {
			if (m_verbose) fprintf(stderr,
				"Frame %u - audio mixer %u could not add audio "
				"queue to audio %s %u", system_frame_no, id,
				pSource->source_type == 1 ? "feed" :
				  pSource->source_type == 2 ? "mixer" :
				    pSource->source_type == 3 ? "sink" : "unknown", pSource->source_id);
			continue;
		}
		pSource->pAudioQueue->max_bytes = pSource->max_delay_in_bytes;
		if (pSource->channels) pSource->rms = (u_int64_t*)
			calloc(pSource->channels, sizeof(u_int64_t));
		if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u added "
			"queue to audio %s %u as source, channels %u\n", system_frame_no, id,
			pSource->source_type == 1 ? "feed" : "mixer",
			pSource->source_id, pSource->channels);
		pSource = pSource->next;
	}
	// Queues are now added to audio feeds or audio mixers.

	if (m_verbose>3) fprintf(stderr, "Frame %u - audio mixer %u now has sources(s)\n",
		system_frame_no, id);

	pSource = m_mixers[id]->pAudioSources;
	while (pSource) {
		if (m_verbose > 2)
			fprintf(stderr, "Frame %u - audio mixer %u checking source %u\n",
				system_frame_no, id, pSource->source_id);

		// We will change state if we have samples
		if ((hardstart || (pSource->pAudioQueue && pSource->pAudioQueue->sample_count >=
			m_mixers[id]->channels))) {

			if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u changes state from "
				"%s to %s\n"
				"Frame %u - source audio %s %u has enough samples (%u >= %u)\n",
				system_frame_no, id,
				feed_state_string(m_mixers[id]->state), feed_state_string(RUNNING),
				system_frame_no,
				pSource->source_type == 1 ? "feed" :
				  pSource->source_type == 2 ? "mixer" : "unknown",
				pSource->source_id,
				pSource->pAudioQueue->sample_count, m_mixers[id]->channels
				);
			m_mixers[id]->state = RUNNING;

			// If start_time == 0 we set the start time, else
			// the start_time has been set previous (perhaps
			// for another audio source and has already run
			if (m_mixers[id]->start_time.tv_sec == 0 &&
				m_mixers[id]->start_time.tv_usec == 0) {
				m_mixers[id]->start_time.tv_sec = pTimenow->tv_sec;
				m_mixers[id]->start_time.tv_usec = pTimenow->tv_usec;
			}
			else if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u start time "
				"has already been set\n", system_frame_no, id);
			
			// state is now running and we will write date next time around
			return true;
		}
		pSource = pSource->next;
	}
	return false;
}

/*
FROM ConditionalStart

			// First we check sources and queues just to be sure
			if (!m_mixers[id]->pAudioSources) {
				if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u state "
					"changed from PENDING to READY. Has no sources\n",
					system_frame_no, id);
				m_mixers[id]->state = READY;
				continue;
			}

			// Then we check for consumers. If no consumers yet, then we postpone
			// changing state
			if (!m_mixers[id]->pAudioQueues) continue;
			if (m_verbose > 2) fprintf(stderr,
				"Frame %u - audio mixer %u pending, will check sources. Should have %u sources\n",
				system_frame_no, id, m_mixers[id]->no_sources);

			if (m_verbose>3) fprintf(stderr, "Frame %u - audio mixer %u has queue(s) "
				"(consumers - sinks or mixers\n", system_frame_no, id);

			// Now we have both sources and consumers/sinks/queues
			// Time to add queues to sources 
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				// skip to next if AudioSource alrady had an AudioQueue attached
				if (pSource->pAudioQueue) {
					if (m_verbose>3) fprintf(stderr,
						"Frame %u - audio mixer %u audio source was already set to audio %s %u\n",
						system_frame_no, id,
						pSource->source_type == 1 ? "feed" :
						  pSource->source_type == 2 ? "mixer" : "unknown",
						pSource->source_id);
					pSource = pSource->next;
					continue;
				}
				// else if source is audio feed, then add queue to audio feed
				if (pSource->source_type == 1) {
					if (!(pSource->pAudioQueue = m_pAudioFeed->AddQueue(pSource->source_id))) {
						if (m_verbose) fprintf(stderr,
							"Frame %u - audio mixer %u could not add audio queue to audio feed %u",
							system_frame_no, id, pSource->source_id);
						continue;
					} else pSource->channels = m_pAudioFeed->GetChannels(pSource->source_id);

				// else if source is an other audio mixer, then add queue to audio mixer
				// Its a bad idea to connect mixer to it self, but we don't check for it.
				} else if (pSource->source_type == 2) {
					if (!(pSource->pAudioQueue = AddQueue(pSource->source_id))) {
						if (m_verbose) fprintf(stderr,
							"Frame %u - audio mixer %u could not add audio queue to audio mixer %u",
							system_frame_no, id, pSource->source_id);
						continue;
					} else pSource->channels = GetChannels(pSource->source_id);
				}
				if (pSource->channels) pSource->rms = (u_int64_t*)
					calloc(pSource->channels, sizeof(u_int64_t));
				if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u added "
					"queue to audio %s %u as source, channels %u\n", system_frame_no, id,
					pSource->source_type == 1 ? "feed" : "mixer",
					pSource->source_id, pSource->channels);
				pSource = pSource->next;
			}
			// Queues are now added to audio feeds or audio mixers.

			if (m_verbose>3) fprintf(stderr, "Frame %u - audio mixer %u now has sources(s)\n",
						system_frame_no, id);

			//u_int32_t needed_samples = m_mixers[id]->calcsamplespersecond/m_pVideoMixer->m_frame_rate;
			//u_int32_t needed_bytes = m_mixers[id]->bytespersecond/m_pVideoMixer->m_frame_rate;
			pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				if (m_verbose > 2)
					fprintf(stderr, "Frame %u - audio mixer %u checking source %u\n",
						system_frame_no, id, pSource->source_id);

if (m_verbose > 2) fprintf(stderr, "Check source 1\n");
				// We will change state if we have samples
				if (pSource->pAudioQueue && pSource->pAudioQueue->sample_count >=
					m_mixers[id]->channels) {
if (m_verbose > 2) fprintf(stderr, "Check source 1A\n");

					fprintf(stderr, "Frame %u - audio mixer %u changes state from "
						"%s to %s\n"
						"Frame %u - source audio %s %u has enough samples (%u >= %u)\n",
						system_frame_no, id,
						feed_state_string(m_mixers[id]->state), feed_state_string(RUNNING),
						system_frame_no,
						pSource->source_type == 1 ? "feed" :
						  pSource->source_type == 2 ? "mixer" : "unknown",
						pSource->source_id,
						pSource->pAudioQueue->sample_count, m_mixers[id]->channels
						);
					m_mixers[id]->state = RUNNING;

					// If start_time == 0 we set the start time, else
					// the start_time has been set previous (perhaps
					// for another audio source and has already run
					if (m_mixers[id]->start_time.tv_sec == 0 &&
						m_mixers[id]->start_time.tv_usec == 0) {
						m_mixers[id]->start_time.tv_sec = timenow.tv_sec;
						m_mixers[id]->start_time.tv_usec = timenow.tv_usec;
					}
					else if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u start time "
						"has already been set\n", system_frame_no, id);
					
					// state is now running and we will write date next time around
					break;
				}
				pSource = pSource->next;
			}
			continue;
*/

void CAudioMixer::Update()
{
	u_int32_t id;
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio mixer update start\n",
		system_frame_no);

	if (!m_mixers) {
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio mixer update done\n",
			system_frame_no);
		return;
	}


	// Check to see if animation is needed
	if (m_animation) {
		for (id = 0; id < m_max_mixers ; id++) {
			if (m_mixers[id]) {
				if (m_mixers[id]->volume_delta_steps) {
					if (m_mixers[id]->pVolume) {
						float* p = m_mixers[id]->pVolume;
						for (u_int32_t i=0 ; i < m_mixers[id]->channels; i++, p++) {
							*p += m_mixers[id]->volume_delta;
							if (*p < 0.0) *p = 0;
							else if(*p > MAX_VOLUME_FLOAT) *p = MAX_VOLUME_FLOAT;
						}
					}
					m_mixers[id]->volume_delta_steps--;
				}
				audio_source_t* pSource = m_mixers[id]->pAudioSources;
				while (pSource) {
					if (pSource->volume_delta_steps) {
						if (pSource->pVolume) {
							float* p = pSource->pVolume;
							for (u_int32_t i=0 ; i < pSource->channels; i++, p++) {
								*p += pSource->volume_delta;
								if (*p < 0.0) *p = 0;
								else if(*p > MAX_VOLUME_FLOAT)
									*p = MAX_VOLUME_FLOAT;
							}
						}
						pSource->volume_delta_steps--;
					}
					pSource = pSource->next;
				}
			}
		}
		m_animation--;
	}

	timeval timenow, timedif;
	gettimeofday(&timenow,NULL);

	// walk through all the mixers
	for (id = 0; id < m_max_mixers ; id++) {

		// Skip the mixer if it doesn't exist
		if (!m_mixers[id]) continue;

		// Adjust RMS down -fading down towards zero,if no new high values are present
		if (m_mixers[id]->rms) for (u_int32_t i=0 ; i < m_mixers[id]->channels ; i++) {
			if (m_mixers[id]->rms[i]) m_mixers[id]->rms[i] *= 0.95;
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				if (pSource->rms) for (u_int32_t i=0 ; i < m_mixers[id]->channels ; i++) {
					if (pSource->rms[i]) pSource->rms[i] *= 0.95;
				}
				pSource = pSource->next;
			}
		}

		// Adjust clipped down for sources and mixer
		if (m_mixers[id]->pAudioSources) {
			audio_source_t* pSource = m_mixers[id]->pAudioSources;
			while (pSource) {
				if (pSource->clipped) pSource->clipped--;
				pSource = pSource->next;
			}
		}
		if (m_mixers[id]->clipped) m_mixers[id]->clipped--;

                // Find time diff, sample diff and calculate samplespersecond
                if (m_mixers[id]->update_time.tv_sec > 0 || m_mixers[id]->update_time.tv_usec > 0) {
                        timersub(&timenow, &m_mixers[id]->update_time, &timedif);
                        double time = timedif.tv_sec + ((double) timedif.tv_usec)/1000000.0;
                        if (time != 0.0) m_mixers[id]->samplespersecond =
				(m_mixers[id]->samples - m_mixers[id]->last_samples)/time;
                        m_mixers[id]->total_avg_sps -= m_mixers[id]->avg_sps_arr[m_mixers[id]->avg_sps_index];
                        m_mixers[id]->total_avg_sps += m_mixers[id]->samplespersecond;
                        m_mixers[id]->avg_sps_arr[m_mixers[id]->avg_sps_index] = m_mixers[id]->samplespersecond;
                        m_mixers[id]->avg_sps_index++;
                        m_mixers[id]->avg_sps_index %= MAX_AVERAGE_SAMPLES;
                }

                // Update values for next time
                m_mixers[id]->last_samples = m_mixers[id]->samples;
                m_mixers[id]->update_time.tv_sec = timenow.tv_sec;
                m_mixers[id]->update_time.tv_usec = timenow.tv_usec;

		// Check if pending mixers should change state to running
		if (m_mixers[id]->state == PENDING) {

			ConditionalStart(id, &timenow);

			continue;

		}

		// We skip the mixer, if the mixer is not running or stalled
		if (m_mixers[id]->state != RUNNING &&
			m_mixers[id]->state != STALLED) continue;

		// Then we check sources and queues just to be sure
		if (!m_mixers[id]->pAudioQueues || !m_mixers[id]->pAudioSources) {
			if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u changed state to READY in Update()\n",
				system_frame_no, id);
			m_mixers[id]->state = READY;
			continue;
		}

		// The mixer is running or stalled. Lets calculate how many
		// bytes we need to write
		u_int32_t needed_samples = SamplesNeeded(id)*m_mixers[id]->channels;
		//u_int32_t needed_bytes = needed_samples*m_mixers[id]->bytespersample;

		// Lets check if we should write any samples at all
		if (!needed_samples) {
			if (m_verbose)
				fprintf(stderr, "Frame %u - audio mixer %u does not need any samples. "
				"Skipping\n", system_frame_no, id);
			continue;
		}

		if (m_verbose > 3) fprintf(stderr, "Frame %u - audio mixer %u will produce %u samples\n",
			system_frame_no, id, needed_samples);

		// Now we need to get 'needed_samples' samples from all sources and then
		// add samples from all sources (fill with silence if necessary). Then
		// the result has to be copied to every audio queue subscribing
		audio_source_t* pSource = m_mixers[id]->pAudioSources;
		audio_buffer_t* srcbuf = NULL;
		audio_buffer_t* newbuf = NULL;
		u_int32_t n, size;
		while (needed_samples > 0) {

			// First we get a buffer to hold the mixed samples
			if (!newbuf) {
				if (!(newbuf = GetEmptyBuffer(id, &size))) {
					fprintf(stderr, "Failed to get buffer for audio queue for audio mixer %u\n", id);
					break;
				}
			}
			memset(newbuf->data, 0, size);

			// max_samples will be the number of samples we need from each source
			// but not larger than we can have in the buffer
			u_int32_t max_samples = size/sizeof(int32_t);
			if (max_samples > needed_samples) max_samples = needed_samples;

			// mix will point to the start of the buffer of the mixed samples
			int32_t* mix = (int32_t*) newbuf->data;

			// Now we need to mix each source
			pSource = m_mixers[id]->pAudioSources;
			u_int32_t source_no = 0;
			while (pSource) {
				source_no++;
				if (!pSource->pAudioQueue) {
					pSource = pSource->next;
				}
				int32_t average_samples = QueueUpdateAndAverage(pSource->pAudioQueue);
				if (average_samples > pSource->pAudioQueue->queue_max &&
					(int32_t) pSource->pAudioQueue->sample_count > pSource->pAudioQueue->queue_max) {
					u_int32_t drop_samples = pSource->pAudioQueue->sample_count -
						pSource->pAudioQueue->queue_max;
					if (m_mixers[id]->channels > 1) {
						drop_samples /= m_mixers[id]->channels;
						drop_samples *= m_mixers[id]->channels;
					}
					if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u dropping "
						"%u samples (%u ms) from audio %s %u\n",
						system_frame_no, id,
						drop_samples, 1000*drop_samples/(m_mixers[id]->rate*m_mixers[id]->channels),
						pSource->source_type == 1 ? "feed" :
						  pSource->source_type == 2 ? "mixer" :
						    pSource->source_type == 2 ? "sink" : "unknown",
						pSource->source_id);
					if (pSource->source_type == 1)
                                        	m_pVideoMixer->m_pAudioFeed->QueueDropSamples(pSource->pAudioQueue,
                                                	drop_samples);
                                	else if (pSource->source_type == 2)
                                        	QueueDropSamples(pSource->pAudioQueue, drop_samples);
                                	else if (pSource->source_type == 3)
                                        	m_pVideoMixer->m_pAudioSink->QueueDropSamples(pSource->pAudioQueue,
							drop_samples);
					else fprintf(stderr, "Frame %u - audio mixer %u source is unknown\n",
                                        	system_frame_no, id);
				}

				if (m_verbose > 3) {
					fprintf(stderr, "Frame %u - audio mixer %u needed_samples %u,"
						" max_samples = %u\n", system_frame_no, id,
						needed_samples, max_samples);
					fprintf(stderr, "Frame %u - audio mixer %u source %u has "
						"%u samples and %u buffers\n", system_frame_no, id,
						pSource->source_id, pSource->pAudioQueue->sample_count,
						pSource->pAudioQueue->buf_count );
				}
				// n is the number of samples we have mixed for this source in this round
				n = 0;
				while (n < max_samples) {

					// First we get a buffer from source, if any
					if (!(srcbuf = GetAudioBuffer(pSource->pAudioQueue))) {
						if (m_verbose > 1 && pSource->got_samples)
							fprintf(stderr, "Frame %u - audio mixer %u adding %u "
								"samples of silence\n",
								system_frame_no, id, max_samples - n);
						if (m_verbose > 2) fprintf(stderr, "Frame %u - audio mixer %u had no "
							"buffer for source %u\n",
							system_frame_no, id, pSource->source_id);
						// source had no more buffers and we do not need to mix
						// more bytes from this source at this moment
						pSource->silence += (pSource->channels*(max_samples-n)/m_mixers[id]->channels);
						pSource->got_samples = false;
						break;
					
					} else pSource->got_samples = true;

					if (pSource->drop_bytes) {
						u_int32_t drop_bytes = srcbuf->len >= pSource->drop_bytes ?
							pSource->drop_bytes : srcbuf->len;
						srcbuf->len -= drop_bytes;
						srcbuf->data += drop_bytes;
						pSource->drop_bytes -= drop_bytes;
						u_int32_t samples = drop_bytes/sizeof(int32_t);
						pSource->dropped += samples;
						if (m_verbose) fprintf(stderr,
							"Frame %u - audio mixer %u source %s %u dropping %u "
							"samples per channel\n",
							system_frame_no, id,
							pSource->source_type == 1 ? "feed" :
							  pSource->source_type == 2 ? "mixer" :
							    pSource->source_type == 3 ? "sink" : "unknown",
							pSource->source_id,
							samples);
					}

					// We fill the srcbuf with zero if it is muted. This is a possible
					// Fixme as we might drop the buffer instead.
					if (srcbuf->mute) memset(srcbuf->data, 0, srcbuf->len);

					// The mix buffer can contain max_samples, but we may need less
					// as we may already have filled some in it
					u_int32_t mix_samples = set_mix_samples(newbuf,
						srcbuf, max_samples-n, pSource->pSourceMap);

					if (m_verbose > 3) fprintf(stderr,
						" - n=%u, %u samples in buffer seq no %u, mix_samples is %u\n",
						n, (u_int32_t)(srcbuf->len/sizeof(int32_t)), srcbuf->seq_no, mix_samples);

					// src points to the first sample in the srcbuf received from source
					//u_int32_t m = 0;
//fprintf(stderr, "sourceno %u srcseqno %u srcch %u newbufch %u needed_samples %u max_samples %u n %u mix_samples %u \n", source_no, srcbuf->seq_no, srcbuf->channels, newbuf->channels, needed_samples, max_samples, n, mix_samples);
					
					// If volume needs to be set, then apply on the samples
					// we are going to use
					if (pSource->pVolume) {
						u_int32_t len = srcbuf->len;
						if (len > mix_samples*sizeof(int32_t))
							srcbuf->len = mix_samples*sizeof(int32_t);
						if (pSource->channels == srcbuf->channels && pSource->pVolume)
							SetVolumeForAudioBuffer(srcbuf, pSource->pVolume);
						else if (m_verbose > 2) fprintf(stderr,
							"Frame %u - audio mixer %u source volume error : %s\n",
							system_frame_no, id,
							srcbuf->channels == pSource->channels ?
							  "missing volume" : "channel mismatch");
						srcbuf->len = len;
					}

					// Now we need to set RMS for source from buffer
					// FIXME. If we don't consume the entire buffer, RMS will be too
					// high on the next turn as we multiply with vol squared more than
					// one time.
					if (!srcbuf->mute) for (u_int32_t i=0 ; i < pSource->channels &&
						i < srcbuf->channels; i++) {
						float vol = pSource->pVolume ?
							pSource->pVolume[i]*pSource->pVolume[i] : 1.0;
						if (vol != 1.0) {
							if (pSource->rms[i] < srcbuf->rms[i]*vol)
								pSource->rms[i] = srcbuf->rms[i]*vol;
						}
						else if (pSource->rms[i] < srcbuf->rms[i])
							pSource->rms[i] = srcbuf->rms[i];
					}
//fprintf(stderr, "RMS buffer %lu source %lu (%u,%u,%u)\n", srcbuf->rms[0], pSource->rms[0], pSource->channels, m_mixers[id]->channels, srcbuf->channels);

					// The we need to see if we clipped
					// If clipped, we mark mixer as clipped and mark
					// newbuf also as clipped
					if (srcbuf->clipped) {
						pSource->clipped = 100;
						newbuf->clipped = true;
					}

					u_int32_t samples_mixed = 0;

					if (pSource->mute) {
						//if (pSource->pSourceMap) {
							mix_samples /= newbuf->channels;
							samples_mixed = mix_samples*newbuf->channels;
							u_int32_t offset = srcbuf->channels*mix_samples*sizeof(int32_t);
							srcbuf->len -= offset;
							srcbuf->data += offset;
/*
						} else {
							if (newbuf->channels == srcbuf->channels) {
								n += mix_samples;
								m += mix_samples;
							} else if (srcbuf->channels == 1) {
								n += mix_samples;
								m += mix_samples;
							} else if (newbuf->channels == 1) {
							}
						}
*/
					} else {
						samples_mixed = MixSamples(newbuf,
							srcbuf, n, mix_samples, pSource->pSourceMap);
						// src_buf has been updated.

					}
					n += samples_mixed;

					// If the srcbuf have more samples in it, we add it back to the
					// head of the queue, otherwise we delete the buffer.
					if (srcbuf->len > 0) {
						if (srcbuf->len >= sizeof(int32_t)) 
							AddAudioBufferToHeadOfQueue(pSource->pAudioQueue, srcbuf);
						else {
							fprintf(stderr, "Frame %u - audio mixer %u buffer "
								"with less data %u than a single sample (%u). Ooops\n",
								system_frame_no, id, srcbuf->len, (u_int32_t) sizeof(int32_t));
							free(srcbuf);
						}
					} else free(srcbuf);
					srcbuf = NULL;
				}

				// Now all samples either needed or available for this source has been mixed
				// and we go to next source, if any
				pSource = pSource->next;
			}

			// We have now mixed max_sample samples from all sources, if available. Now
			// we need to update the buffer and check for clipping
			newbuf->len = max_samples*sizeof(int32_t);
			for (n=0; n < max_samples; n++) {
				if (mix[n] > 32767) {
					mix[n] = 32767;
					newbuf->clipped = true;
				} else if (mix[n] < -32768) {
					mix[n] = -32768;
					newbuf->clipped = true;
				}
			}
			if (needed_samples < max_samples) {
				fprintf(stderr, "Oooops we mixed too many samples\n");
				needed_samples = 0;
			} else needed_samples -= max_samples;

			AddAudioToMixer(id, newbuf);
//fprintf(stderr, "MIXER queue now has %u buffers and %u samples\n", m_mixers[id]->pAudioQueues->buf_count, m_mixers[id]->pAudioQueues->sample_count);
			newbuf = NULL;
			m_mixers[id]->sample_count += (max_samples/m_mixers[id]->channels);
		}

		if (m_verbose > 2)
			fprintf(stderr, "Frame %u - audio mixer %u updated.\n",
				system_frame_no, id);
	}
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio mixer update done\n",
		system_frame_no);
}

// Calculate samples lacking per channel using start_time and sample_count. 
int32_t CAudioMixer::SamplesNeeded(u_int32_t id)
{
	if (id >= m_max_mixers || !m_mixers || !m_mixers[id] ||
		(m_mixers[id]->state != RUNNING && m_mixers[id]->state != STALLED))
		return -1;
	struct timeval timenow, timediff;
	if (gettimeofday(&timenow,NULL)) return -1;
	timersub(&timenow, &m_mixers[id]->start_time, &timediff);
	u_int32_t samples = timediff.tv_sec*m_mixers[id]->rate +
		m_mixers[id]->rate * (timediff.tv_usec/1000000.0);
	if (samples < m_mixers[id]->sample_count) {

		fprintf(stderr, "audio mixer %u samples %u less than sample count %u. "
			"Assuming wrapping\n",
			id, samples, m_mixers[id]->sample_count);
		m_mixers[id]->start_time.tv_sec = timenow.tv_sec;
		m_mixers[id]->start_time.tv_usec = timenow.tv_usec;
		u_int32_t n = UINT_MAX - m_mixers[id]->sample_count + samples + 1;
		m_mixers[id]->sample_count = 0;
		return n;
		// We may have wrapping - FIXME
	}
	if (m_verbose > 2) fprintf(stderr,
		"Frame %u - audio mixer %u lacking samples %u per channel, current %u should be %u\n",
			system_frame_no, id,
			samples - m_mixers[id]->sample_count,
			m_mixers[id]->sample_count, samples);
	return samples - m_mixers[id]->sample_count;
}
