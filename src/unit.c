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

#include "unit.h"

#include "gsd-datetime-mechanism-debian.h"

Unit *
lookup_unit (GVariant  *parameters,
             GError   **error)
{
  const gchar *unit_name;

  g_variant_get_child (parameters, 0, "&s", &unit_name);

  if (!g_str_equal (unit_name, "ntpd.service") || !_get_can_use_ntp_debian ())
    {
      g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_FILE_NOT_FOUND,
                   "Unknown unit: %s", unit_name);
      return NULL;
    }

  return (void *) 0x1;
}

const gchar *
unit_get_state (Unit *unit)
{
  g_return_val_if_fail (unit != NULL, NULL);

  return _get_using_ntp_debian () ? "enabled" : "disabled";
}

void
unit_start (Unit *unit)
{
  g_return_if_fail (unit != NULL);

  _set_using_ntp_debian (TRUE, NULL);
}

void
unit_stop (Unit *unit)
{
  g_return_if_fail (unit != NULL);

  _set_using_ntp_debian (FALSE, NULL);
}
