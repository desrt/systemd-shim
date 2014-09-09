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

#include "state.h"

#define STATE_FILENAME "/run/systemd-shim-state"

static GKeyFile *
state_get_key_file (void)
{
  static GKeyFile *key_file;

  if (!key_file)
    {
      key_file = g_key_file_new ();
      g_key_file_load_from_file (key_file, STATE_FILENAME, G_KEY_FILE_NONE, NULL);
    }

  return key_file;
}

static void
state_sync (void)
{
  GError *error = NULL;
  GKeyFile *key_file;

  key_file = state_get_key_file ();

  /* This will be world-readable but that's OK */
  if (!g_key_file_save_to_file (key_file, STATE_FILENAME, &error))
    {
      g_warning ("cannot save systemd-shim state: %s", error->message);
      g_error_free (error);
    }
}

gchar **
state_list_units (void)
{
  GKeyFile *key_file = state_get_key_file ();

  return g_key_file_get_groups (key_file, NULL);
}

gchar *
state_get_string (const gchar *unit,
                  const gchar *key)
{
  GKeyFile *key_file = state_get_key_file ();

  return g_key_file_get_string (key_file, unit, key, NULL);
}

void
state_set_string (const gchar *unit,
                  const gchar *key,
                  const gchar *value)
{
  GKeyFile *key_file = state_get_key_file ();

  g_key_file_set_string (key_file, unit, key, value);
  state_sync ();
}

void
state_remove_unit (const gchar *unit)
{
  GKeyFile *key_file = state_get_key_file ();

  g_key_file_remove_group (key_file, unit, NULL);
  state_sync ();
}
