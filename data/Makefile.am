dbusservicedir = $(prefix)/share/dbus-1/system-services
dbusservice_DATA = org.freedesktop.systemd1.service
CLEANFILES = $(dbusservice_DATA)

ntpunitsdir = $(prefix)/lib/systemd/ntp-units.d
dist_ntpunits_DATA = systemd-shim.list

dbuspolicydir = $(sysconfdir)/dbus-1/system.d
dist_dbuspolicy_DATA = org.freedesktop.systemd1.conf

org.freedesktop.systemd1.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
	             echo 'Name=org.freedesktop.systemd1'; \
	             echo 'User=root'; \
	             echo 'Exec=${libexecdir}/systemd-shim') > $@.tmp && \
	            mv $@.tmp $@
