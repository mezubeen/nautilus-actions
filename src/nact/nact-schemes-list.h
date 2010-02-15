/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_SCHEMES_LIST_H__
#define __NACT_SCHEMES_LIST_H__

#include <gtk/gtk.h>

#include "base-window.h"

G_BEGIN_DECLS

void    nact_schemes_list_create_model( GtkTreeView *treeview, gboolean for_action );
void    nact_schemes_list_init_view( GtkTreeView *treeview, BaseWindow *window );
void    nact_schemes_list_setup_values( GtkTreeView *treeview, BaseWindow *window, GSList *schemes, gboolean sensitive, gboolean editable );
GSList *nact_schemes_list_get_schemes( GtkTreeView *treeview );
void    nact_schemes_list_save_defaults( BaseWindow *window );
void    nact_schemes_list_dispose( BaseWindow *window );

G_END_DECLS

#endif /* __NACT_SCHEMES_LIST_H__ */