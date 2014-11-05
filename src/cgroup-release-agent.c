/*
 * Copyright © 2014 Canonical Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the licence, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * Authors:
 *   Martin Pitt <martin.pitt@ubuntu.com>
 */

#include <glib.h>
#include <gio/gio.h>

#include <unistd.h>
#include <stdio.h>

#define CGMANAGER_AGENT "/run/cgmanager/agents/cgm-release-agent.systemd"

int
main (int argc, char** argv)
{
  gchar* unit_name;
  GDBusConnection *connection;
  GVariant *reply = NULL;
  GError *error = NULL;

  g_assert(argc == 2);
  unit_name = g_path_get_basename (argv[1]);
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL)
    g_error ("Cannot connect to system D-BUS: %s", error->message);
  g_debug ("sending StopUnit(%s) to systemd-shim", unit_name);
  reply = g_dbus_connection_call_sync (connection,
                                       "org.freedesktop.systemd1",
                                       "/org/freedesktop/systemd1",
                                       "org.freedesktop.systemd1.Manager",
                                       "StopUnit",
                                       g_variant_new ("(ss)", unit_name, "/"),
                                       G_VARIANT_TYPE ("(o)"),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1, NULL, &error);
  if (reply != NULL)
    g_debug ("Got reply: %s", g_variant_print (reply, TRUE));
  else
    g_warning ("StopUnit call failed: %s", error->message);

  g_object_unref (connection);

  /* Now chain-call cgmanager's agent */
  if (access (CGMANAGER_AGENT, X_OK) == 0)
    {
      g_debug ("calling " CGMANAGER_AGENT);
      execl (CGMANAGER_AGENT, CGMANAGER_AGENT, argv[1], NULL);
      perror ("failed to run " CGMANAGER_AGENT);
      return 1;
    }
  else
    {
      g_debug (CGMANAGER_AGENT " does not exist");
    }

  return 0;
}
