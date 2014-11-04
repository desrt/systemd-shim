/*
 * Copyright © 2014 Canonical Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the licence, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * Authors:
 *   Serge Hallyn <serge.hallyn@canonical.com>
 *   Stéphane Graber <stephane.graber@canonical.com>
 *   Ryan Lortie <desrt@desrt.ca>
 */

#include "cgmanager.h"

#include <gio/gio.h>

#define CGM_DBUS_ADDRESS          "unix:path=/sys/fs/cgroup/cgmanager/sock"
#define CGM_REQUIRED_VERSION      8

static GDBusConnection *
cgmanager_connect (GError **error)
{
  GDBusConnection *connection;
  GVariant *reply;
  GVariant *version;

  connection = g_dbus_connection_new_for_address_sync (CGM_DBUS_ADDRESS,
                                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                       NULL, NULL, error);

  if (!connection)
    return NULL;

  reply = g_dbus_connection_call_sync (connection, NULL, "/org/linuxcontainers/cgmanager",
                                       "org.freedesktop.DBus.Properties", "Get",
                                       g_variant_new ("(ss)", "org.linuxcontainers.cgmanager0_0", "api_version"),
                                       G_VARIANT_TYPE ("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, error);

  if (!reply)
    {
      g_object_unref (connection);
      return NULL;
    }

  g_variant_get (reply, "(v)", &version);
  g_variant_unref (reply);

  if (!g_variant_is_of_type (version, G_VARIANT_TYPE_INT32) || g_variant_get_int32 (version) < CGM_REQUIRED_VERSION)
    {
      g_set_error_literal (error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "Incorrect cgmanager API version");
      g_object_unref (connection);
      g_variant_unref (version);
      return NULL;
    }

  g_variant_unref (version);

  return connection;
}

static gboolean
cgmanager_call (const gchar         *method_name,
                GVariant            *parameters,
                const GVariantType  *reply_type,
                GVariant           **reply)
{
  static GDBusConnection *connection;
  static gboolean initialised;
  GVariant *my_reply = NULL;
  GError *error = NULL;

  /* Use a separate bool to prevent repeated attempts to connect to a
   * defunct cgmanager...
   */
  if (!initialised)
    {
      GError *error = NULL;

      connection = cgmanager_connect (&error);

      if (!connection)
        {
          g_warning ("Could not connect to cgmanager: %s", error->message);
          g_error_free (error);
        }

      initialised = TRUE;
    }

  if (!connection)
    return FALSE;

  if (!reply)
    reply = &my_reply;

  /* We do this sync because we need to ensure that the calls finish
   * before we return to _our_ caller saying that this is done.
   */
  *reply = g_dbus_connection_call_sync (connection, NULL, "/org/linuxcontainers/cgmanager",
                                        "org.linuxcontainers.cgmanager0_0", method_name,
                                        parameters, reply_type, G_DBUS_CALL_FLAGS_NONE,
                                        -1, NULL, &error);

  if (!*reply)
    {
      if (reply_type)
        g_warning ("cgmanager method call org.linuxcontainers.cgmanager0_0.%s failed: %s.  "
                   "Use G_DBUS_DEBUG=message for more info.", method_name, error->message);
      g_error_free (error);

      return FALSE;
    }

  if (my_reply)
    g_variant_unref (my_reply);

  return TRUE;
}

void
cgmanager_create (const gchar *path,
                  gint         uid,
                  const guint *pids,
                  guint        n_pids)
{
  guint i;

  if (path[0] == '/')
    path++;

  cgmanager_call ("Create", g_variant_new ("(ss)", "all", path), G_VARIANT_TYPE ("(i)"), NULL);

  if (uid != -1)
    cgmanager_call ("Chown", g_variant_new ("(ssii)", "all", path, uid, -1), G_VARIANT_TYPE_UNIT, NULL);

  for (i = 0; i < n_pids; i++)
    cgmanager_call ("MovePid", g_variant_new ("(ssi)", "all", path, pids[i]), G_VARIANT_TYPE_UNIT, NULL);

  cgmanager_call ("SetValue", g_variant_new ("(ssss)", "systemd", path, "notify_on_release", "1"), G_VARIANT_TYPE_UNIT, NULL);
}

gboolean
cgmanager_remove (const gchar *path)
{
  if (path[0] == '/')
    path++;

  return cgmanager_call ("Remove", g_variant_new ("(ssi)", "all", path, 1), G_VARIANT_TYPE ("(i)"), NULL);
}

void
cgmanager_move_self (void)
{
  cgmanager_call ("MovePidAbs", g_variant_new ("(ssi)", "all", "/", getpid ()), G_VARIANT_TYPE_UNIT, NULL);

  /* install our systemd cgroup release handler */
  g_debug ("Installing cgroup release handler " LIBEXECDIR "/systemd-shim-cgroup-release-agent");
  cgmanager_call ("SetValue",
                  g_variant_new ("(ssss)", "systemd", "/", "release_agent", LIBEXECDIR "/systemd-shim-cgroup-release-agent"),
                  G_VARIANT_TYPE_UNIT,
                  NULL);
}

void
cgmanager_prune (const gchar *path)
{
  cgmanager_call ("Prune", g_variant_new ("(ss)", "all", path), G_VARIANT_TYPE_UNIT, NULL);
}

void
cgmanager_kill (const gchar *path)
{
  GVariant *reply;

  if (cgmanager_call ("GetTasksRecursive", g_variant_new ("(ss)", "all", path), G_VARIANT_TYPE ("(ai)"), &reply))
    {
      GVariantIter *iter;
      guint32 pid;

      g_variant_get (reply, "(ai)", &iter);

      while (g_variant_iter_next (iter, "i", &pid))
        kill (pid, SIGKILL);

      g_variant_iter_free (iter);
      g_variant_unref (reply);
    }
}
