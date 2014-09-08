#ifndef _systemd_iface_h_
#define _systemd_iface_h_

#include "config.h"

static const gchar *systemd_iface =
  "<node>"
   "<interface name='org.freedesktop.systemd1.Manager'>"
    "<method name='GetUnitFileState'>"
     "<arg name='file' type='s' direction='in'/>"
     "<arg name='state' type='s' direction='out'/>"
    "</method>"
    "<method name='DisableUnitFiles'>"
     "<arg name='files' type='as' direction='in'/>"
     "<arg name='runtime' type='b' direction='in'/>"
     "<arg name='changes' type='a(sss)' direction='out'/>"
    "</method>"
    "<method name='EnableUnitFiles'>"
     "<arg name='files' type='as' direction='in'/>"
     "<arg name='runtime' type='b' direction='in'/>"
     "<arg name='force' type='b' direction='in'/>"
     "<arg name='carries_install_info' type='b' direction='out'/>"
     "<arg name='changes' type='a(sss)' direction='out'/>"
    "</method>"
    "<method name='Reload'/>"
    "<method name='StartUnit'>"
     "<arg name='name' type='s' direction='in'/>"
     "<arg name='mode' type='s' direction='in'/>"
     "<arg name='job' type='o' direction='out'/>"
    "</method>"
    "<method name='StopUnit'>"
     "<arg name='name' type='s' direction='in'/>"
     "<arg name='mode' type='s' direction='in'/>"
     "<arg name='job' type='o' direction='out'/>"
    "</method>"
    "<method name='StartTransientUnit'>"
     "<arg name='name' type='s' direction='in'/>"
     "<arg name='mode' type='s' direction='in'/>"
     "<arg name='properties' type='a(sv)' direction='in'/>"
#if SYSTEMD_VERSION >= 209
     "<arg name='aux' type='a(sa(sv))' direction='in'/>"
#endif
     "<arg name='job' type='o' direction='out'/>"
    "</method>"
    "<method name='Subscribe'/>"
    "<method name='Unsubscribe'/>"
    "<property name='Virtualization' type='s' access='read'/>"
   "</interface>"
   "<interface name='org.freedesktop.systemd1.Scope'>"
    "<method name='Abandon'/>"
   "</interface>"
  "</node>";

#endif
