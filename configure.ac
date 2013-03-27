AC_INIT([systemd-shim], [2])
AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])
AC_PROG_CC
PKG_CHECK_MODULES(gio, gio-2.0)
AC_CONFIG_FILES([Makefile
                 data/Makefile
                 src/Makefile])
AC_OUTPUT
