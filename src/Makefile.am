AM_CFLAGS = $(gio_CFLAGS)

gsd_imports = \
	gsd-datetime-mechanism-debian.h		\
	gsd-datetime-mechanism-debian.c

systemd_imports = \
	macro.h		\
	util.h		\
	util.c		\
	virt.h		\
	virt.c

libexec_PROGRAMS = systemd-shim
systemd_shim_LDADD = $(gio_LIBS)
systemd_shim_SOURCES = \
	$(gsd_imports)		\
	$(systemd_imports)	\
	unit.h			\
	unit.c			\
	systemd-iface.h		\
	systemd-shim.c
