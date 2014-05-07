AC_INIT([gnome-flip-screen], [0.1])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/gnome-flip-screen.c])

AC_PROG_CC

PKG_CHECK_MODULES([DEPS], [glib-2.0 gio-2.0 gtk+-3.0 gdk-3.0 gnome-desktop-3.0])

AC_SEARCH_LIBS([gnome_rr_screen_new], [gnome-desktop-3])
AC_CHECK_FUNCS([gnome_rr_config_apply gnome_rr_config_apply_with_time]) 

if test "x${ac_cv_func_gnome_rr_config_apply}" != xyes && \ 
  test "x${ac_cv_func_gnome_rr_config_apply_with_time}" != xyes;
then
  AC_MSG_ERROR([No gnome-rr config. application function found. Check your gnome-desktop library configuration.])
fi 

AC_SUBST([AM_CFLAGS], ['-Wall -Wextra -Werror'])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
