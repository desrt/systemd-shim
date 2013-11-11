/*
 * Copyright © 2013 Canonical Limited
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
 */

#include <gio/gio.h>

#include "unit.h"
#include "virt.h"

#include "systemd-iface.h"

#include <stdlib.h>

static gboolean
exit_on_inactivity (gpointer user_data)
{
  extern gboolean in_shutdown;

  if (!in_shutdown)
    {
      GDBusConnection *session_bus;

      session_bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
      g_dbus_connection_flush_sync (session_bus, NULL, NULL);
      g_object_unref (session_bus);

      exit (0);
    }

  return FALSE;
}

static void
had_activity (void)
{
  static gint inactivity_timeout;

  if (inactivity_timeout)
    g_source_remove (inactivity_timeout);

  inactivity_timeout = g_timeout_add (10000, exit_on_inactivity, NULL);
}

static void
shim_method_call (GDBusConnection       *connection,
                  const gchar           *sender,
                  const gchar           *object_path,
                  const gchar           *interface_name,
                  const gchar           *method_name,
                  GVariant              *parameters,
                  GDBusMethodInvocation *invocation,
                  gpointer               user_data)
{
  GError *error = NULL;

  if (g_str_equal (method_name, "GetUnitFileState"))
    {
      Unit *unit;

      unit = lookup_unit (parameters, &error);

      if (unit)
        {
          g_dbus_method_invocation_return_value (invocation,
                                                 g_variant_new ("(s)", unit_get_state (unit)));
          g_object_unref (unit);
          goto success;
        }
    }

  else if (g_str_equal (method_name, "DisableUnitFiles"))
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a(sss))", NULL));
      goto success;
    }

  else if (g_str_equal (method_name, "EnableUnitFiles"))
    {
      g_dbus_method_invocation_return_value (invocation, g_variant_new ("(ba(sss))", TRUE, NULL));
      goto success;
    }

  else if (g_str_equal (method_name, "Reload"))
    {
      g_dbus_method_invocation_return_value (invocation, NULL);
      goto success;
    }

  else if (g_str_equal (method_name, "StopUnit"))
    {
      Unit *unit;

      unit = lookup_unit (parameters, &error);

      if (unit)
        {
          unit_stop (unit);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(o)", "/"));
          g_object_unref (unit);
          goto success;
        }
    }

  else if (g_str_equal (method_name, "StartUnit"))
    {
      Unit *unit;

      unit = lookup_unit (parameters, &error);

      if (unit)
        {
          unit_start (unit);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(o)", "/"));
          g_dbus_connection_emit_signal (connection, sender, "/org/freedesktop/systemd1",
                                         "org.freedesktop.systemd1.Manager", "JobRemoved",
                                         g_variant_new ("(uoss)", 0, "/", "", ""), NULL);
          g_object_unref (unit);
          goto success;
        }
    }

  else
    g_assert_not_reached ();

  g_dbus_method_invocation_return_gerror (invocation, error);
  g_error_free (error);

success:
  had_activity ();
}

static GVariant *
shim_get_property (GDBusConnection  *connection,
                   const gchar      *sender,
                   const gchar      *object_path,
                   const gchar      *interface_name,
                   const gchar      *property_name,
                   GError          **error,
                   gpointer          user_data)
{
  const gchar *id = "";

  had_activity ();

  g_assert_cmpstr (property_name, ==, "Virtualization");

  detect_virtualization (&id);

  return g_variant_new ("s", id);
}

static void
shim_bus_acquired (GDBusConnection *connection,
                   const gchar     *name,
                   gpointer         user_data)
{
  GDBusInterfaceVTable vtable = {
    shim_method_call,
    shim_get_property,
  };
  GDBusInterfaceInfo *iface;
  GDBusNodeInfo *node;

  node = g_dbus_node_info_new_for_xml (systemd_iface, NULL);
  iface = g_dbus_node_info_lookup_interface (node, "org.freedesktop.systemd1.Manager");
  g_dbus_connection_register_object (connection, "/org/freedesktop/systemd1", iface, &vtable, NULL, NULL, NULL);
  g_dbus_node_info_unref (node);
}

static void
shim_name_lost (GDBusConnection *connection,
                const gchar     *name,
                gpointer         user_data)
{
  g_critical ("Unable to acquire bus name '%s'.  Quitting.", name);
  exit (1);
}

int
main (void)
{
  g_bus_own_name (G_BUS_TYPE_SYSTEM,
                  "org.freedesktop.systemd1",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  shim_bus_acquired,
                  NULL, /* name_acquired */
                  shim_name_lost,
                  NULL, NULL);

  while (1)
    g_main_context_iteration (NULL, TRUE);
}
