AC_INIT([gnome-flip-screen], [0.1])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CONFIG_SRCDIR([src/gnome-flip-screen.c])

AC_PROG_CC

PKG_CHECK_MODULES([DEPS], [glib-2.0 gio-2.0 gtk+-3.0 gdk-3.0 gnome-desktop-3.0])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT
