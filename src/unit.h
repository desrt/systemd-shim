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

#ifndef _unit_h_
#define _unit_h_

#include <gio/gio.h>

#define UNIT_TYPE (unit_get_type ())
#define UNIT_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst), UNIT_TYPE, UnitClass))

typedef GObject Unit;

typedef struct
{
  GObjectClass parent_class;

  const gchar * (* get_state) (Unit *unit);
  void (* start) (Unit *unit);
  void (* start_transient) (Unit *unit, GVariant *properties);
  void (* stop) (Unit *unit);
  void (* abandon) (Unit *unit);
} UnitClass;

GType unit_get_type (void);
Unit *lookup_unit (const gchar *name, GError **error);
const gchar *unit_get_state (Unit *unit);
void unit_start_transient (Unit *unit, GVariant *properties);
void unit_start (Unit *unit);
void unit_stop (Unit *unit);
void unit_abandon (Unit *unit);

Unit *ntp_unit_get (void);

typedef enum
{
  POWER_OFF,
  POWER_REBOOT,
  POWER_SUSPEND,
  POWER_HIBERNATE,
  N_POWER_ACTIONS
} PowerAction;

Unit *power_unit_new (PowerAction action);

Unit *cgroup_unit_new (const gchar *name);

#endif /* _unit_h_ */
