AC_INIT([systemd-shim], [9])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-dist-gzip dist-xz])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES([yes])

# Set default CFLAGS before AC_PROG_CC does
if test "${CFLAGS+yes}" != "yes"; then
  CFLAGS='-O2 -g -Wall -Wwrite-strings -Wmissing-prototypes -fno-common'
fi
AC_PROG_CC

PKG_CHECK_MODULES(gio, gio-2.0)
PKG_CHECK_MODULES(systemd, systemd)
AC_DEFINE_UNQUOTED([SYSTEMD_VERSION], [`$PKG_CONFIG --modversion systemd`], [systemd version])
AC_CONFIG_FILES([Makefile
                 data/Makefile
                 src/Makefile])
AC_OUTPUT
