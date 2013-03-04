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

#include "gsd-datetime-mechanism-debian.h"

static gboolean
_get_using_ntpdate (gboolean *can_use, gboolean *is_using)
{
  if (!g_file_test ("/usr/sbin/ntpdate-debian", G_FILE_TEST_EXISTS))
    return TRUE;

  *can_use = TRUE;

  if (g_file_test ("/etc/network/if-up.d/ntpdate", G_FILE_TEST_EXISTS))
    *is_using = TRUE;

  return TRUE;
}

static gboolean
_get_using_ntpd (gboolean *can_use, gboolean *is_using)
{
  int exit_status;

  if (!g_file_test ("/usr/sbin/ntpd", G_FILE_TEST_EXISTS))
    return TRUE;

  *can_use = TRUE;

  if (!g_spawn_command_line_sync ("/usr/sbin/service ntp status", NULL, NULL, &exit_status, NULL))
    return FALSE;

  if (exit_status == 0)
    *is_using = TRUE;

  return TRUE;
}

gboolean
_get_using_ntp_debian (void)
{
  gboolean can_use_ntp = FALSE;
  gboolean is_using_ntp = FALSE;
  gboolean success;

  /* In Debian, ntpdate is used whenever the network comes up. So if
     either ntpdate or ntpd is installed and available, can_use is true.
     If either is active, is_using is true. */
  success = _get_using_ntpdate (&can_use_ntp, &is_using_ntp) &&
            _get_using_ntpd (&can_use_ntp, &is_using_ntp);

  return success && is_using_ntp;
}

static gboolean
_set_using_ntpdate (gboolean   using_ntp,
                    GError   **error)
{
  const gchar *cmd = NULL;

  /* Debian uses an if-up.d script to sync network time when an interface
     comes up.  This is a separate mechanism from ntpd altogether. */

#define NTPDATE_ENABLED  "/etc/network/if-up.d/ntpdate"
#define NTPDATE_DISABLED "/etc/network/if-up.d/ntpdate.disabled"

  if (using_ntp && g_file_test (NTPDATE_DISABLED, G_FILE_TEST_EXISTS))
    cmd = "/bin/mv -f "NTPDATE_DISABLED" "NTPDATE_ENABLED;
  else if (!using_ntp && g_file_test (NTPDATE_ENABLED, G_FILE_TEST_EXISTS))
    cmd = "/bin/mv -f "NTPDATE_ENABLED" "NTPDATE_DISABLED;
  else
    return TRUE;

  if (!g_spawn_command_line_sync (cmd, NULL, NULL, NULL, error))
    return FALSE;

  /* Kick start ntpdate to sync time immediately */
  if (using_ntp && !g_spawn_command_line_sync ("/etc/network/if-up.d/ntpdate", NULL, NULL, NULL, error))
    return FALSE;

  return TRUE;
}

static gboolean
_set_using_ntpd (gboolean   using_ntp,
                 GError   **error)
{
  int exit_status;
  char *cmd;

  if (!g_file_test ("/usr/sbin/ntpd", G_FILE_TEST_EXISTS))
    return TRUE;

  cmd = g_strconcat ("/usr/sbin/update-rc.d ntp ", using_ntp ? "enable" : "disable", NULL);

  if (!g_spawn_command_line_sync (cmd, NULL, NULL, &exit_status, error))
    {
      g_free (cmd);
      return FALSE;
    }

  g_free (cmd);

  cmd = g_strconcat ("/usr/sbin/service ntp ", using_ntp ? "restart" : "stop", NULL);;

  if (!g_spawn_command_line_sync (cmd, NULL, NULL, &exit_status, error))
    {
      g_free (cmd);
      return FALSE;
    }

  g_free (cmd);

  return TRUE;
}

gboolean
_set_using_ntp_debian (gboolean   using_ntp,
                       GError   **error)
{
  /* In Debian, ntpdate and ntpd may be installed separately, so don't
     assume both are valid. */

  return _set_using_ntpdate (using_ntp, error) &&
         _set_using_ntpd (using_ntp, error);
}
