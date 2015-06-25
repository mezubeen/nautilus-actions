/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_TREE_VIEW_H__
#define __NACT_TREE_VIEW_H__

/*
 * SECTION: nact-tree-view
 * @title: NactTreeView
 * @short_description: The Tree View Base Class Definition
 * @include: nact-tree-view.h
 *
 * This is a convenience class to manage a read-only items tree view.
 *
 * The NactTreeView encapsulates the GtkTreeView which displays the items
 * list on the left of the main pane.
 *
 * It is instanciated from NactMainWindow::on_initialize_gtk().
 *
 * A pointer to this NactTreeView is attached to the NactMainWindow at
 * construction time.
 */

#include "api/na-object-item.h"

#include "base-window.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_TREE_VIEW                ( nact_tree_view_get_type())
#define NACT_TREE_VIEW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_TREE_VIEW, NactTreeView ))
#define NACT_TREE_VIEW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_TREE_VIEW, NactTreeViewClass ))
#define NACT_IS_TREE_VIEW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_TREE_VIEW ))
#define NACT_IS_TREE_VIEW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_TREE_VIEW ))
#define NACT_TREE_VIEW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_TREE_VIEW, NactTreeViewClass ))

typedef struct _NactTreeViewPrivate        NactTreeViewPrivate;

typedef struct {
	/*< private >*/
	GtkBin               parent;
	NactTreeViewPrivate *private;
}
	NactTreeView;

typedef struct {
	/*< private >*/
	GtkBinClass          parent;
}
	NactTreeViewClass;

/**
 * Signals emitted by the NactTreeView instance.
 */
#define TREE_SIGNAL_COUNT_CHANGED				"tree-signal-count-changed"
#define TREE_SIGNAL_FOCUS_IN					"tree-signal-focus-in"
#define TREE_SIGNAL_FOCUS_OUT					"tree-signal-focus-out"
#define TREE_SIGNAL_LEVEL_ZERO_CHANGED			"tree-signal-level-zero-changed"
#define TREE_SIGNAL_MODIFIED_STATUS_CHANGED		"tree-signal-modified-status-changed"
#define TREE_SIGNAL_SELECTION_CHANGED		    "tree-selection-changed"
#define TREE_SIGNAL_CONTEXT_MENU			    "tree-signal-open-popup"

typedef enum {
	TREE_MODE_EDITION = 1,
	TREE_MODE_SELECTION,
	/*< private >*/
	TREE_MODE_N_MODES
}
	NactTreeMode;

/**
 * When getting a list of items; these indicators may be OR-ed.
 */
enum {
	TREE_LIST_SELECTED = 1<<0,
	TREE_LIST_MODIFIED = 1<<1,
	TREE_LIST_ALL      = 1<<7,
	TREE_LIST_DELETED  = 1<<8,
};

GType         nact_tree_view_get_type( void );

NactTreeView *nact_tree_view_new               ( NactMainWindow *main_window );

void          nact_tree_view_set_mnemonic      ( NactTreeView *view,
														GtkContainer *parent,
														const gchar *widget_name );

void          nact_tree_view_set_edition_mode  ( NactTreeView *view,
														NactTreeMode mode );

void          nact_tree_view_fill              ( NactTreeView *view, GList *items );

gboolean      nact_tree_view_are_notify_allowed( const NactTreeView *view );
void          nact_tree_view_set_notify_allowed( NactTreeView *view, gboolean allow );

void          nact_tree_view_collapse_all      ( const NactTreeView *view );
void          nact_tree_view_expand_all        ( const NactTreeView *view );
NAObjectItem *nact_tree_view_get_item_by_id    ( const NactTreeView *view, const gchar *id );
GList        *nact_tree_view_get_items         ( const NactTreeView *view );
GList        *nact_tree_view_get_items_ex      ( const NactTreeView *view, guint mode );

void          nact_tree_view_select_row_at_path( NactTreeView *view, GtkTreePath *path );

G_END_DECLS

#endif /* __NACT_TREE_VIEW_H__ */
