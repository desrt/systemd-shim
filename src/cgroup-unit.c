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
 */

#include "unit.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef UnitClass CGroupUnitClass;
static GType cgroup_unit_get_type (void);

typedef struct
{
  Unit parent_instance;
  gchar *name;
} CGroupUnit;

G_DEFINE_TYPE (CGroupUnit, cgroup_unit, UNIT_TYPE)

static gchar *
cgroup_unit_get_path_and_uid (const gchar *slice,
                              const gchar *scope,
                              gint        *uid)
{
  GString *path;
  gint i;

  path = g_string_new (NULL);
  for (i = 0; slice[i]; i++)
    if (slice[i] == '-')
      {
        g_string_append_len (path, slice, i);
        g_string_append (path, ".slice/");
      }

  g_string_append_len (path, slice, i);

  *uid = -1;
  if (g_str_has_prefix (slice, "user-"))
    {
      guint64 value;
      gchar *end;

      errno = 0;
      value = g_ascii_strtoull (slice + 5, &end, 10);
      if (errno == 0 && g_str_equal (end, ".slice") && value < G_MAXINT)
        *uid = (gint) value;
    }

  if (scope)
    {
      g_string_append_c (path, '/');
      g_string_append (path, scope);
    }

  return g_string_free (path, FALSE);
}

static void
cgroup_unit_start_transient (Unit     *unit,
                             GVariant *properties)
{
  CGroupUnit *cg_unit = (CGroupUnit *) unit;
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  gchar *slice;
  GArray *pids;

  if (!g_str_has_suffix (cg_unit->name, ".scope"))
    {
      g_warning ("%s: Can only StartTransient for scopes", cg_unit->name);
      return;
    }

  pids = g_array_new (TRUE, FALSE, sizeof (guint));
  slice = NULL;

  g_variant_iter_init (&iter, properties);
  while (g_variant_iter_loop (&iter, "(&sv)", &key, &value))
    {
      if (g_str_equal (key, "Slice") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
          g_free (slice);
          slice = g_variant_dup_string (value, NULL);
        }

      else if (g_str_equal (key, "PIDs") && g_variant_is_of_type (value, G_VARIANT_TYPE ("au")))
        {
          const guint *vals;
          gsize n_vals;

          vals = g_variant_get_fixed_array (value, &n_vals, sizeof (guint));
          g_array_append_vals (pids, vals, n_vals);
        }
    }

  if (slice && g_str_has_suffix (slice, ".slice"))
    {
      gchar *path;
      gint uid;

      path = cgroup_unit_get_path_and_uid (slice, cg_unit->name, &uid);
      cgmanager_create (path, uid, (const guint *) pids->data, pids->len);
      g_free (path);
    }
  else
    g_warning ("%s: StartTransient failed: requires 'Slice' property ending with '.slice'", cg_unit->name);

  g_array_free (pids, TRUE);
}

static void
cgroup_unit_start (Unit *unit)
{
  CGroupUnit *cg_unit = (CGroupUnit *) unit;
  gchar *path;
  gint uid;

  if (!g_str_has_suffix (cg_unit->name, ".slice"))
    {
      g_warning ("%s: Can only Start for slices", cg_unit->name);
      return;
    }

  path = cgroup_unit_get_path_and_uid (cg_unit->name, NULL, &uid);
  cgmanager_create (path, uid, NULL, 0);
  g_free (path);
}

static void
cgroup_unit_stop (Unit *unit)
{
  /* for now, no-op */
}

static const gchar *
cgroup_unit_get_state (Unit *unit)
{
  CGroupUnit *gc = (CGroupUnit *)unit;
  return gc->name;
}

Unit *
cgroup_unit_new (const gchar *name)
{
  CGroupUnit *unit;

  unit = g_object_new (cgroup_unit_get_type (), NULL);
  unit->name = g_strdup (name);

  return (Unit *) unit;
}

static void
cgroup_unit_init (CGroupUnit *unit)
{
}

static void
cgroup_unit_class_init (UnitClass *class)
{
  class->start_transient = cgroup_unit_start_transient;
  class->start = cgroup_unit_start;
  class->stop = cgroup_unit_stop;
  class->get_state = cgroup_unit_get_state;
}
