#
# GST_LIBS= -pthread -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lxml2 -lglib-2.0


INCLUDES	= -I.		\
	@GTK_PLUS_CFLAGS@
	@PANGO_CFLAGS@
	@OSMESA_CFLAGS@

#	@GSTREAMER_CFLAGS@

include_HEADERS = snowmix.h

#noinst_LTLIBRARIES = libsnowcup.la libvideotest.la
noinst_LTLIBRARIES = libsnowmix.la

### Program Part
# bin_PROGRAMS = snowcupd videotest
bin_PROGRAMS = snowmix bars
#

bars_SOURCES = bars.cpp

snowmix_SOURCES = main.cpp	\
	audio_feed.h		\
	audio_mixer.h		\
	audio_sink.h		\
	audio_util.h		\
	cairo_graphic.h		\
	monitor.h		\
	snowmix.h		\
	version.h		\
	video.h			\
	video_shape.h		\
	video_feed.h		\
	video_mixer.h		\
	video_scaler.h		\
	video_text.h		\
	virtual_feed.h

snowmix_LDADD = libsnowmix.la	\
	-lrt			\
	-lpthread		\
	-ltcl8.5		\
	@GTK_PLUS_LIBS@		\
	@PANGO_LIBS@		\
	@OSMESA_LIBS@		\
	@SDL_LIBS@

#	@GSTREAMER_LIBS@


libsnowmix_la_SOURCES =		\
	add_by.h		\
	audio_feed.cpp		\
	audio_feed.h		\
	audio_mixer.cpp		\
	audio_mixer.h		\
	audio_sink.cpp		\
	audio_sink.h		\
	audio_util.cpp		\
	audio_util.h		\
	cairo_graphic.cpp	\
	cairo_graphic.h		\
	controller.h		\
	controller.cpp		\
	command.h		\
	command.cpp		\
	monitor.cpp		\
	monitor.h		\
	snowmix.h		\
	tcl_interface.h		\
	tcl_interface.cpp	\
	video_shape.h		\
	video_shape.cpp		\
	video_feed.h		\
	video_feed.cpp		\
	video_image.h		\
	video_image.cpp		\
	video_mixer.cpp		\
	video_mixer.h		\
	video_scaler.cpp	\
	video_scaler.h		\
	video_text.cpp		\
	video_text.h		\
	virtual_feed.cpp	\
	virtual_feed.h


#videotest_SOURCES = videotest.cpp video.h
#
#videotest_LDADD = libvideotest.la	\
#	-lpthread		\
#	@SDL_LIBS@
#
#libvideotest_la_SOURCES =	\
#	video.h			\
#	video.cpp
#
#snowcupd_SOURCES = main.cpp	\
#	snowcup.h
#
#snowcupd_LDADD = libsnowcup.la	\
#	-lpthread		\
#	@SDL_LIBS@
#
#	@GST_LIBS@		\
#
#
## Library Part
#libsnowcup_la_SOURCES =		\
#	snowcup.h		\
#	config_tools.h		\
#	config_tools.cpp	\
#	queue.h			\
#	queue.cpp		\
#	request.h		\
#	request.cpp		\
#	oldserver.h		\
#	oldserver.cpp		\
#	oldsession.h		\
##	oldsession.cpp		\
#	server.h		\
#	server.cpp		\
#	session.h		\
#	session.cpp		\
#	snowcup.h		\
#	source.h		\
#	source.cpp		\
#	stream.h		\
#	stream.cpp		\
##	util.h			\
#	util.cpp		\
#	video.h			\
#	video.cpp
#
##
#
#
#
##$(LIBOBJS)

# WITH_SDL
# WITH_CAIRO
# WITH_PANGO
# WITH_GSTREAMER
# WITH_X264

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -Wall -fexceptions -DWITH_SDL -DWITH_CAIRO -DWITH_PANGO -DWITH_TCL -DHAVE_TCL

