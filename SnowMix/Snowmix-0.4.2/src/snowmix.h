/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __SNOWMIX_H__
#define __SNOWMIX_H__

#include <sys/types.h>


#include "version.h"
#define BANNER "Snowmix version "SNOWMIX_VERSION"\n"
#define STATUS "STAT: "
#define MESSAGE "MSG: "

// FIXME - MacOSX doesn't seem to have MSG_NOSIGNAL
// SG_NOSIGNAL (since Linux 2.2)
// Requests  not  to  send SIGPIPE on errors on stream oriented sockets when the other end breaks the
// connection.  The EPIPE error is still returned.
// OSX doesn't seem to have MSG_NOSIGNAL
#ifdef HAVE_MACOSX
#define MSG_NOSIGNAL 0
#endif

// Forward declaration
class CVideoMixer;
struct feed_type;

/* Define the max number of feeds in a single output. */
#define MAX_STACK       10

// Define the max backlog for listening for control connections
#define MAX_LISTEN_BACKLOG 10

enum feed_state_enum {
	UNDEFINED,
	SETUP,
	READY,
	PENDING,
	RUNNING,
	STALLED,
	DISCONNECTED
};

#define feed_state_string(state)	(state == RUNNING ? "RUNNING" : state == PENDING ? "PENDING" : state == READY ? "READY" : state == SETUP ? "SETUP" : state == STALLED ? "STALLED" : state == DISCONNECTED ? "DISCONNECTED" : "UNKNOWN")

extern int exit_program;
bool set_fd_nonblocking(int fd);

/********************************************************************************
*										*
*	Stuff for handling gstreamer shmsink/shmsrc data.			*
*										*
********************************************************************************/

//struct area_list_type;

/* Struct holding use count for buffers of an area. */
struct buffer_type {
	struct buffer_type	*prev;
	struct buffer_type	*next;

	struct area_list_type	*area_list;	/* Pointer to parrent area_list struct. */
	unsigned long		offset;		/* Offset relative to the base. */
	unsigned long		bsize;		/* Size of the buffer. */
	unsigned int		use;		/* Use count. */
};


/* Struct describing a list of areas used by a feed. */
struct area_list_type {
	struct area_list_type	*prev;
	struct area_list_type	*next;

	int			area_id;		/* Area ID as received from the feed. */
	char			*area_handle_name;	/* Pointer to the area handle name. */
	int			area_handle;		/* File handle for the mapped area. */
	unsigned long		area_size;		/* Size of the shared memory area. */
	u_int8_t		*buffer_base;		/* Offset of where the shm buffer is mapped into memory. */
	struct feed_type	*feed;			/* Pointer to the feed owning this area. */

	struct buffer_type	*buffers;		/* List of buffers in use by this area. */
};

#define MAX_FIFO_DEPTH	10
struct frame_fifo_type {
	struct buffer_type	*buffer;
};

struct shm_commandbuf_type {
	unsigned int	type;
	int		area_id;

	union {
		struct {
			size_t		size;
			unsigned int	path_size;
		} new_shm_area;
		struct {
			unsigned long	offset;
			unsigned long	bsize;
		} buffer;
		struct {
			unsigned long	offset;
		} ack_buffer;
	} payload;
};

// Forward decleration (for CVideoMixer)
//struct feed_type *find_feed (int id);
struct output_memory_list_type *output_get_free_block (CVideoMixer* pVideoMixer);

/************************************************************************
*                                                                       *
*       Output handling functions.                                      *
*                                                                       *
************************************************************************/

struct output_memory_list_type {
        struct output_memory_list_type  *prev;
        struct output_memory_list_type  *next;

        unsigned long                   offset;         /* Offset inside the memory_base. */
        int                             use;            /* Use counter. */
};

void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len);
void output_reset (CVideoMixer* pVideoMixer);

/* Function macro to add a element to the head of a list. */
#define list_add_head(list,new) {	\
	new->prev = NULL;		\
	new->next = list;		\
	if (list != NULL) {		\
		list->prev = new;	\
	}				\
	list = new;			\
} while (0)

/* Function macro to add a element to the tail of a list (somewhat slow). */
#define list_add_tail(list,new) {		\
	if (list == NULL) {			\
		list = new;			\
	} else {				\
		typeof (new) e = list;		\
		while (e->next != NULL) {	\
			e = e->next;		\
		}				\
		e->next = new;			\
		new->prev = e;			\
	}					\
	new->next = NULL;			\
} while (0)


/* Function macro to remove a element from a list. */
#define list_del_element(list,element) {				\
	/* Remove from memory list. */					\
	if (element->prev == NULL) {					\
		/* This is the first element in the chain. */		\
		list = element->next;					\
		if (element->next != NULL) {				\
			/* Not the last. */				\
			element->next->prev = NULL;			\
		}							\
	} else {							\
		/* This is not the first element. */			\
		element->prev->next = element->next;			\
		if (element->next != NULL) {				\
			/* Not the last. */				\
			element->next->prev = element->prev;		\
		}							\
	}								\
} while (0)



#endif	// SNOWMIX
