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

#include "cgmanager.h"
#include "state.h"
#include "unit.h"
#include "virt.h"

#include "systemd-iface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static gboolean
exit_on_inactivity (gpointer user_data)
{
  extern gboolean in_shutdown;

  if (!in_shutdown)
    {
      GDBusConnection *system_bus;

      system_bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, NULL);
      g_dbus_connection_flush_sync (system_bus, NULL, NULL);
      g_object_unref (system_bus);

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
      const gchar *unit_name;
      Unit *unit;

      g_variant_get_child (parameters, 0, "&s", &unit_name);
      unit = lookup_unit (unit_name, &error);

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

  else if (g_str_equal (method_name, "Subscribe"))
    {
      g_dbus_method_invocation_return_value (invocation, NULL);
      goto success;
    }

  else if (g_str_equal (method_name, "Unsubscribe"))
    {
      g_dbus_method_invocation_return_value (invocation, NULL);
      goto success;
    }

  else if (g_str_equal (method_name, "StopUnit"))
    {
      const gchar *unit_name;
      Unit *unit;

      g_variant_get_child (parameters, 0, "&s", &unit_name);
      unit = lookup_unit (unit_name, &error);

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
      const gchar *unit_name;
      Unit *unit;

      g_variant_get_child (parameters, 0, "&s", &unit_name);
      unit = lookup_unit (unit_name, &error);

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
  else if (g_str_equal (method_name, "StartTransientUnit"))
    {
      const gchar *unit_name;
      Unit *unit;

      g_variant_get_child (parameters, 0, "&s", &unit_name);
      unit = lookup_unit (unit_name, &error);

      if (unit)
        {
          GVariant *properties;

          properties = g_variant_get_child_value (parameters, 2);
          unit_start_transient (unit, properties);
          g_dbus_method_invocation_return_value (invocation, g_variant_new ("(o)", "/"));
          g_dbus_connection_emit_signal (connection, sender, "/org/freedesktop/systemd1",
                                         "org.freedesktop.systemd1.Manager", "JobRemoved",
                                         g_variant_new ("(uoss)", 0, "/", unit_get_state(unit), "done"), NULL);
          g_variant_unref (properties);
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

static gchar *
unescape_object_path (const gchar *path)
{
  gchar *result;
  gint i, j;

  result = g_malloc (strlen (path) + 1);
  for (i = 0, j = 0; path[i]; i++)
    {
      if (path[i] == '_')
        {
          if (g_ascii_isxdigit (path[i + 1]) && g_ascii_isxdigit (path[i + 2]))
            {
              gint val = g_ascii_xdigit_value(path[i + 1]) * 16 + g_ascii_xdigit_value (path[i + 2]);

              if (g_ascii_isgraph (val))
                result[j++] = val;

              i += 2;
            }
        }

      else
        result[j++] = path[i];
    }

  result[j] = '\0';

  return result;
}

static gchar *
escape_object_path (const gchar *path)
{
  gchar *result;
  gint i, j;

  result = g_malloc (3 * strlen (path) + 1);
  for (i = 0, j = 0; path[i]; i++)
    {
      if (g_ascii_isalnum (path[i]))
        result[j++] = path[i];
      else
        {
          snprintf (&result[j], 4, "_%02x", (guint) path[i]);
          j += 3;
        }
    }

  result[j] = '\0';

  return result;
}

static void
shim_unit_method_call (GDBusConnection       *connection,
                       const gchar           *sender,
                       const gchar           *object_path,
                       const gchar           *interface_name,
                       const gchar           *method_name,
                       GVariant              *parameters,
                       GDBusMethodInvocation *invocation,
                       gpointer               user_data)
{
  const gchar *node = user_data;

  had_activity ();

  if (g_str_equal (method_name, "Abandon"))
    {
      GError *error = NULL;
      gchar *unit_name;
      Unit *unit;

      unit_name = unescape_object_path (node);
      unit = lookup_unit (unit_name, &error);

      if (unit)
        {
          unit_abandon (unit);

          g_dbus_method_invocation_return_value (invocation, NULL);
          g_object_unref (unit);
        }
      else
        {
          g_dbus_method_invocation_return_gerror (invocation, error);
          g_error_free (error);
        }

      g_free (unit_name);
    }

  else
    g_assert_not_reached ();
}

static gchar **
shim_units_enumerate (GDBusConnection *connection,
                      const gchar     *sender,
                      const gchar     *object_path,
                      gpointer         user_data)
{
  gchar **cgroups;
  guint i;

  had_activity ();

  cgroups = state_list_units ();

  for (i = 0; cgroups[i]; i++)
    {
      gchar *unescaped;

      unescaped = cgroups[i];
      cgroups[i] = escape_object_path (unescaped);
      g_free (unescaped);
    }

  return cgroups;
}

static GDBusInterfaceInfo* shim_units_iface;

static GDBusInterfaceInfo **
shim_units_introspect (GDBusConnection *connection,
                       const gchar     *sender,
                       const gchar     *object_path,
                       const gchar     *node,
                       gpointer         user_data)
{
  GDBusInterfaceInfo *result[2] = { g_dbus_interface_info_ref (shim_units_iface) };

  had_activity ();

  return g_memdup (result, sizeof result);
}

static const GDBusInterfaceVTable *
shim_units_dispatch (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *object_path,
                     const gchar     *interface_name,
                     const gchar     *node,
                     gpointer        *out_user_data,
                     gpointer         user_data)
{
  static const GDBusInterfaceVTable vtable = {
    shim_unit_method_call
  };

  had_activity ();

  *out_user_data = (gpointer) node;

  return &vtable;
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
  GDBusSubtreeVTable sub_vtable = {
    shim_units_enumerate,
    shim_units_introspect,
    shim_units_dispatch
  };
  GDBusInterfaceInfo *iface;
  GDBusNodeInfo *node;

  node = g_dbus_node_info_new_for_xml (systemd_iface, NULL);
  shim_units_iface = g_dbus_node_info_lookup_interface (node, "org.freedesktop.systemd1.Scope");
  g_assert (shim_units_iface);
  g_dbus_interface_info_ref (shim_units_iface);

  iface = g_dbus_node_info_lookup_interface (node, "org.freedesktop.systemd1.Manager");

  g_dbus_connection_register_object (connection, "/org/freedesktop/systemd1", iface, &vtable, NULL, NULL, NULL);
  g_dbus_connection_register_subtree (connection, "/org/freedesktop/systemd1/unit", &sub_vtable,
                                      G_DBUS_SUBTREE_FLAGS_DISPATCH_TO_UNENUMERATED_NODES, NULL, NULL, NULL);

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

  cgmanager_move_self ();

  while (1)
    g_main_context_iteration (NULL, TRUE);
}
