/*
 * Copyright © 2013 Canonical Limited
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

typedef UnitClass PowerUnitClass;
static GType power_unit_get_type (void);

const gchar *power_cmds[] = {
  [POWER_OFF] = "/sbin/poweroff",
  [POWER_REBOOT] = "/sbin/reboot",
  [POWER_SUSPEND] = "/usr/sbin/pm-suspend",
  [POWER_HIBERNATE] = "/usr/sbin/pm-hibernate"
};

typedef struct
{
  Unit parent_instance;
  PowerAction action;
} PowerUnit;

G_DEFINE_TYPE (PowerUnit, power_unit, UNIT_TYPE)

gboolean in_shutdown;

static void
power_unit_start (Unit *unit)
{
  PowerUnit *pu = (PowerUnit *) unit;
  static gint64 last_suspend_time;

  /* If we request power off or reboot actions then we should ignore any
   * suspend or hibernate actions that come after this.
   */
  if (pu->action == POWER_OFF || pu->action == POWER_REBOOT)
    {
      GError *error = NULL;
      gchar *pid_str;
      gboolean success;

      in_shutdown = TRUE;

      /* avoid being killed during shutdown, so that we can keep our
       * in_shutdown state */
      pid_str = g_strdup_printf ("%u", (unsigned) getpid ());
      success = g_file_set_contents ("/run/sendsigs.omit.d/systemd-shim.pid", pid_str, -1, &error);
      g_free (pid_str);
      if (!success)
        {
          g_warning ("Unable to write sendsigs.omit.d pid file: %s", error->message);
          g_error_free (error);
        }

      if (system (power_cmds[pu->action]) != 0)
        g_warning ("Error while running '%s'", power_cmds[pu->action]);
    }
  else
    {
      if (in_shutdown)
        return;

      /* This is pretty ugly: if we are being asked to perform a suspend
       * or hibernate action within 1 second of the previous one, don't
       * do it.
       *
       * We will be able to do this properly once we forward the
       * timestamp of the event that caused the suspend all the way
       * down.
       */
      if (pu->action == POWER_SUSPEND && last_suspend_time + G_TIME_SPAN_SECOND > g_get_monotonic_time ())
        return;

      /* pm-utils might not have been installed, so go the direct route
       * if we find that we don't have it...
       */
      if (g_file_test (power_cmds[pu->action], G_FILE_TEST_IS_EXECUTABLE))
        {
          if (system (power_cmds[pu->action]) != 0)
            g_warning ("Error while running '%s'", power_cmds[pu->action]);
        }
      else
        {
          const gchar *kind;
          gint fd;

          fd = open ("/sys/power/state", O_WRONLY);
          if (fd == -1)
            {
              g_warning ("Could not open /sys/power/state");
              return;
            }

          kind = (pu->action == POWER_SUSPEND) ? "mem" : "disk";
          if (write (fd, kind, strlen (kind)) != strlen (kind))
            g_warning ("Failed to write() to /sys/power/state?!?");
          close (fd);
        }

      if (pu->action == POWER_SUSPEND)
        last_suspend_time = g_get_monotonic_time ();
    }
}

static void
power_unit_stop (Unit *unit)
{
}

static const gchar *
power_unit_get_state (Unit *unit)
{
  return "static";
}

Unit *
power_unit_new (PowerAction action)
{
  PowerUnit *unit;

  g_return_val_if_fail (action < N_POWER_ACTIONS, NULL);

  unit = g_object_new (power_unit_get_type (), NULL);
  unit->action = action;

  return (Unit *) unit;
}

static void
power_unit_init (PowerUnit *unit)
{
}

static void
power_unit_class_init (UnitClass *class)
{
  class->start = power_unit_start;
  class->stop = power_unit_stop;
  class->get_state = power_unit_get_state;
}
