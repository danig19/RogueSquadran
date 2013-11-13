AC_INIT([snowmix], [0.1])
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_LIBTOOL

# SDL
dnl Get SDL cflags and SDL libraries to include
SDL_LIBS="`sdl-config --libs` "
AC_SUBST(SDL_LIBS)

# GTK_PLUS
dnl *** pango ***
GTK_PLUS_LIBS="`echo  $( pkg-config --libs gtk+-3.0)` "
AC_SUBST(GTK_PLUS_LIBS)
GTK_PLUS_CFLAGS="`echo  $(pkg-config --cflags gtk+-3.0)` "
AC_SUBST(GTK_PLUS_CFLAGS)

# Pango
dnl *** pango ***
PANGO_LIBS="`echo  $(pkg-config --libs pangocairo)` "
AC_SUBST(PANGO_LIBS)
PANGO_CFLAGS="`echo  $(pkg-config --cflags pangocairo)` "
AC_SUBST(PANGO_CFLAGS)

# Gstreamer
dnl *** Gstreamer ***
GSTREAMER_LIBS="`echo  $(pkg-config --libs gstreamer-0.10)` "
AC_SUBST(GSTREAMER_LIBS)
GSTREAMER_CFLAGS="`echo  $(pkg-config --cflags gstreamer-0.10)` "
AC_SUBST(GSTREAMER_CFLAGS)

# OSMesa - Off Screen Mesa (OpenGL)
dnl *** osmesa ***
OSMESA_LIBS="`echo  $(pkg-config --libs osmesa)` "
AC_SUBST(OSMESA_LIBS)
OSMESA_CFLAGS="`echo  $(pkg-config --cflags osmesa)` "
AC_SUBST(OSMESA_CFLAGS)

AC_CHECK_HEADER(tcl8.5/tcl.h, , [AC_MSG_ERROR(
[Unable to find tcl header files, install tcl development package.] )])
AC_CONFIG_FILES([Makefile
	src/Makefile
])
AC_OUTPUT
