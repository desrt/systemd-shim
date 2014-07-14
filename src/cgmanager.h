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
 *   Ryan Lortie <desrt@desrt.ca>
 */

#ifndef _cgmanager_h_
#define _cgmanager_h_

#include <glib.h>

void cgmanager_create (const gchar *path,
                       gint         uid,
                       guint       *pids,
                       guint        n_pids);

void cgmanager_remove (const gchar *path);

void cgmanager_moveself (void);

#endif /* _cgmanager_h_ */
