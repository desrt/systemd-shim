/*
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
 * Copyright (C) 2011 Bastien Nocera <hadess@hadess.net>
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

#include <stdio.h>

#define NTPDATE_ENABLED   "/etc/network/if-up.d/ntpdate"
#define NTPDATE_DISABLED  "/etc/network/if-up.d/ntpdate.disabled"
#define NTPDATE_AVAILABLE "/usr/sbin/ntpdate-debian"
#define NTPD_AVAILABLE    "/usr/sbin/ntpd"

static gboolean
ntp_unit_get_can_use_ntpdate (void)
{
  return g_file_test (NTPDATE_AVAILABLE, G_FILE_TEST_EXISTS);
}

static gboolean
ntp_unit_get_using_ntpdate (void)
{
  if (!ntp_unit_get_can_use_ntpdate ())
    return FALSE;

  return g_file_test (NTPDATE_ENABLED, G_FILE_TEST_EXISTS);
}

static gboolean
ntp_unit_get_can_use_ntpd (void)
{
  return g_file_test (NTPD_AVAILABLE, G_FILE_TEST_EXISTS);
}

static gboolean
ntp_unit_get_using_ntpd (void)
{
  int exit_status;

  if (!ntp_unit_get_can_use_ntpd ())
    return FALSE;

  if (!g_spawn_command_line_sync ("/usr/sbin/service ntp status", NULL, NULL, &exit_status, NULL))
    return FALSE;

  return exit_status == 0;
}

static void
ntp_unit_set_using_ntpdate (gboolean using_ntp)
{
  if (using_ntp == ntp_unit_get_using_ntpdate ())
    return;

  if (using_ntp)
    {
      rename (NTPDATE_DISABLED, NTPDATE_ENABLED);

      /* Kick start ntpdate to sync time immediately */
      g_spawn_command_line_sync ("/etc/network/if-up.d/ntpdate", NULL, NULL, NULL, NULL);
    }
  else
    rename (NTPDATE_ENABLED, NTPDATE_DISABLED);
}

static void
ntp_unit_set_using_ntpd (gboolean using_ntp)
{
  char *cmd;

  cmd = g_strconcat ("/usr/sbin/update-rc.d ntp ", using_ntp ? "enable" : "disable", NULL);
  g_spawn_command_line_sync (cmd, NULL, NULL, NULL, NULL);
  g_free (cmd);

  cmd = g_strconcat ("/usr/sbin/service ntp ", using_ntp ? "restart" : "stop", NULL);;
  g_spawn_command_line_sync (cmd, NULL, NULL, NULL, NULL);
  g_free (cmd);
}

typedef Unit NtpUnit;
typedef UnitClass NtpUnitClass;
static GType ntp_unit_get_type (void);

G_DEFINE_TYPE (NtpUnit, ntp_unit, UNIT_TYPE)

static void
ntp_unit_start (Unit *unit)
{
  if (ntp_unit_get_can_use_ntpdate ())
    ntp_unit_set_using_ntpdate (TRUE);

  if (ntp_unit_get_can_use_ntpd ())
    ntp_unit_set_using_ntpd (TRUE);
}

static void
ntp_unit_stop (Unit *unit)
{
  if (ntp_unit_get_can_use_ntpdate ())
    ntp_unit_set_using_ntpdate (FALSE);

  if (ntp_unit_get_can_use_ntpd ())
    ntp_unit_set_using_ntpd (FALSE);
}

static const gchar *
ntp_unit_get_state (Unit *unit)
{
  if (ntp_unit_get_using_ntpdate () || ntp_unit_get_using_ntpd ())
    return "enabled";
  else
    return "disabled";
}

Unit *
ntp_unit_get (void)
{
  if (ntp_unit_get_can_use_ntpdate () || ntp_unit_get_can_use_ntpd ())
    return g_object_new (ntp_unit_get_type (), NULL);

  return NULL;
}

static void
ntp_unit_init (Unit *unit)
{
}

static void
ntp_unit_class_init (UnitClass *class)
{
  class->start = ntp_unit_start;
  class->stop = ntp_unit_stop;
  class->get_state = ntp_unit_get_state;
}
