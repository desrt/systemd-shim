AM_CFLAGS = $(gio_CFLAGS)

systemd_imports = \
	macro.h		\
	util.h		\
	util.c		\
	virt.h		\
	virt.c

libexec_PROGRAMS = systemd-shim
systemd_shim_LDADD = $(gio_LIBS)
systemd_shim_SOURCES = \
	$(systemd_imports)	\
	cgmanager.h		\
	cgmanager.c		\
	unit.h			\
	unit.c			\
	ntp-unit.c		\
	power-unit.c		\
	cgroup-unit.c		\
	state.h			\
	state.c			\
	systemd-iface.h		\
	systemd-shim.c
