/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __UI_FMA_MATCH_LIST_H__
#define __UI_FMA_MATCH_LIST_H__

/**
 * SECTION: fma_match_list
 * @short_description: Implementation of a list match/does not match.
 * @include: ui/fma-match-list.h
 *
 * In an ideal world, this would be a base interface for FMAISchemesTab,
 * etc. interfaces.
 * In GObject world however, one cannot derived an interface, nor an interface
 * can implement another interface. The GObject solution would be for FMAISchemesTab
 * to requires NactIMatchList, and for FMAMainWindow to implement this same
 * NactIMatchList interface. This is not very practical as FMAMainWindow is
 * already some big bunch of code...
 * And, nonetheless, we need three/four instances of this interface, which
 * is not possible while Schemes, etc. are already interfaces themselves.
 *
 * So we stay with just a piece of helper functions...
 */

#include "base-window.h"
#include "fma-main-window-def.h"

G_BEGIN_DECLS

typedef GSList * ( *pget_filters ) ( void * );
typedef void     ( *pset_filters ) ( void *, GSList * );
typedef void     ( *pon_add_cb )   ( void *, BaseWindow * );
typedef void     ( *pon_remove_cb )( void *, BaseWindow * );

enum {
	MATCH_LIST_MUST_MATCH_ONE_OF = 1,
	MATCH_LIST_MUST_MATCH_ALL_OF,
};

void    fma_match_list_init_with_args( FMAMainWindow *window,
												const gchar  *tab_name,
												guint         tab_id,
												GtkWidget    *listview,
												GtkWidget    *addbutton,
												GtkWidget    *removebutton,
												pget_filters  pget,
												pset_filters  pset,
												pon_add_cb    pon_add,
												pon_remove_cb pon_remove,
												guint         match_header,
												const gchar  *item_header,
												gboolean      editable_filter );

void    fma_match_list_insert_row    ( FMAMainWindow *window,
												const gchar  *tab_name,
												const gchar  *filter,
												gboolean      match,
												gboolean      not_match );

GSList *fma_match_list_get_rows      ( FMAMainWindow *window,
												const gchar  *tab_name );

G_END_DECLS

#endif /* __UI_FMA_MATCH_LIST_H__ */
