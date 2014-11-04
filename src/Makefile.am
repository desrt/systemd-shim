AM_CFLAGS = $(gio_CFLAGS)

systemd_imports = \
	macro.h		\
	util.h		\
	util.c		\
	virt.h		\
	virt.c

libexec_PROGRAMS = systemd-shim systemd-shim-cgroup-release-agent
systemd_shim_LDADD = $(gio_LIBS)
systemd_shim_CPPFLAGS = \
	-DLIBEXECDIR=\"$(libexecdir)\"	\
	$(NULL)
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

systemd_shim_cgroup_release_agent_LDADD = $(gio_LIBS)
systemd_shim_cgroup_release_agent_SOURCES = \
	cgroup-release-agent.c	\
	$(NULL)
