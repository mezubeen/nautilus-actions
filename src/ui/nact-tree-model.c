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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>

#include "api/fma-object-api.h"

#include "core/fma-iprefs.h"

#include "fma-application.h"
#include "fma-clipboard.h"
#include "base-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-tree-model.h"
#include "nact-tree-model-priv.h"

/* iter on tree store
 */
typedef gboolean ( *FnIterOnStore )( const NactTreeModel *, GtkTreeStore *, GtkTreePath *, FMAObject *, gpointer );

/* getting the list of items
 * - mode is the indicator of the wished content
 * - list is the returned list
 */
typedef struct {
	guint  mode;
	GList *items;
}
	ntmGetItems;

/* when iterating while searching for an object by id
 * setting the iter if found
 */
typedef struct {
	gchar       *id;
	FMAObject    *object;
	GtkTreeIter *iter;
}
	ntmFindId;

/* when iterating while searching for an object by its address
 * setting the iter if found
 */
typedef struct {
	const FMAObject *object;
	GtkTreeIter    *iter;
	GtkTreePath    *path;
}
	ntmFindObject;

/* dump the content of the tree
 */
typedef struct {
	gchar *fname;
	gchar *prefix;
}
	ntmDumpStruct;

/* drop formats, as defined in nact-tree-model-dnd.c
 */
extern GtkTargetEntry tree_model_dnd_dest_formats[];
extern guint          tree_model_dnd_dest_formats_count;

#define TREE_MODEL_ORDER_MODE			"nact-tree-model-order-mode"

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactTreeModelClass *klass );
static void     imulti_drag_source_init( EggTreeMultiDragSourceIface *iface, void *user_data );
static void     idrag_dest_init( GtkTreeDragDestIface *iface, void *user_data );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *model );
static void     instance_finalize( GObject *model );
static void     connect_item_updated_signal( NactTreeModel *tmodel );
static void     on_settings_order_mode_changed( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, NactTreeModel *model );
static void     on_main_item_updated( BaseWindow *window, FMAIContext *context, guint data, NactTreeModel *model );
static void     setup_dnd_edition( NactTreeModel *tmodel );
static void     append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const FMAObject *object );
static void     display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const FMAObject *object );
static void     display_order_change( NactTreeModel *model, gint order_mode );
#if 0
static void     dump( NactTreeModel *model );
static gboolean dump_store( NactTreeModel *model, GtkTreePath *path, FMAObject *object, ntmDumpStruct *ntm );
#endif
static void     fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, FMAObject *object, GtkTreeIter *parent );
static gboolean filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model );
static gboolean find_item_iter( NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmFindId *nfo );
static gboolean find_object_iter( NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmFindObject *nfo );
static gboolean get_items_iter( const NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmGetItems *ngi );
static void     iter_on_store( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data );
static gboolean iter_on_store_item( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data );
static void     remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const FMAObject *object );
static gboolean delete_items_rec( GtkTreeStore *store, GtkTreeIter *iter );
static gint     sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data );

GType
nact_tree_model_get_type( void )
{
	static GType model_type = 0;

	if( !model_type ){
		model_type = register_type();
	}

	return( model_type );
}

static GType
register_type (void)
{
	static const gchar *thisfn = "nact_tree_model_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactTreeModelClass ),
		NULL,							/* base_init */
		NULL,							/* base_finalize */
		( GClassInitFunc ) class_init,
		NULL,							/* class_finalize */
		NULL,							/* class_data */
		sizeof( NactTreeModel ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo imulti_drag_source_info = {
		( GInterfaceInitFunc ) imulti_drag_source_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo idrag_dest_info = {
		( GInterfaceInitFunc ) idrag_dest_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_TREE_MODEL_FILTER, "NactTreeModel", &info, 0 );

	g_type_add_interface_static( type, EGG_TYPE_TREE_MULTI_DRAG_SOURCE, &imulti_drag_source_info );

	g_type_add_interface_static( type, GTK_TYPE_TREE_DRAG_DEST, &idrag_dest_info );

	return( type );
}

static void
class_init( NactTreeModelClass *klass )
{
	static const gchar *thisfn = "nact_tree_model_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
}

static void
imulti_drag_source_init( EggTreeMultiDragSourceIface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->row_draggable = nact_tree_model_dnd_imulti_drag_source_row_draggable;
	iface->drag_data_get = nact_tree_model_dnd_imulti_drag_source_drag_data_get;
	iface->drag_data_delete = nact_tree_model_dnd_imulti_drag_source_drag_data_delete;
	iface->get_target_list = nact_tree_model_dnd_imulti_drag_source_get_format_list;
	iface->free_target_list = NULL;
	iface->get_drag_actions = nact_tree_model_dnd_imulti_drag_source_get_drag_actions;
}

static void
idrag_dest_init( GtkTreeDragDestIface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->drag_data_received = nact_tree_model_dnd_idrag_dest_drag_data_received;
	iface->row_drop_possible = nact_tree_model_dnd_idrag_dest_row_drop_possible;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_tree_model_instance_init";
	NactTreeModel *self;

	g_return_if_fail( NACT_IS_TREE_MODEL( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

#if 0
/*
 * Initializes the tree model.
 *
 * We use drag and drop:
 * - inside of treeview, for duplicating items, or moving items between menus
 * - from treeview to the outside world (e.g. Nautilus) to export actions
 * - from outside world (e.g. Nautilus) to import actions
 */
static void
instance_constructed( GObject *model )
{
	static const gchar *thisfn = "nact_tree_model_instance_constructed";
	NactTreeModelPrivate *priv;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	priv = NACT_TREE_MODEL( model )->private;

	if( !priv->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( model );
		}

		g_debug( "%s: model=%p (%s)", thisfn, ( void * ) model, G_OBJECT_TYPE_NAME( model ));

		/* nact_tree_model_set_main_window */
		priv->clipboard = fma_clipboard_new( priv->window );

		if( priv->mode == TREE_MODE_EDITION ){

			egg_tree_multi_drag_add_drag_support(
					EGG_TREE_MULTI_DRAG_SOURCE( model ),
					priv->treeview );

			gtk_tree_view_enable_model_drag_dest(
					priv->treeview,
					tree_model_dnd_dest_formats,
					tree_model_dnd_dest_formats_count,
					nact_tree_model_dnd_imulti_drag_source_get_drag_actions( EGG_TREE_MULTI_DRAG_SOURCE( model )));

			g_signal_connect(
					priv->treeview, "drag-begin",
					G_CALLBACK( nact_tree_model_dnd_on_drag_begin ), priv->window );

			/* connect: implies that we have to do all hard stuff
			 * connect_after: no more triggers drag-drop signal
			 */
			/*base_window_signal_connect_after( window,
					G_OBJECT( model->private->treeview ), "drag-motion", G_CALLBACK( on_drag_motion ));*/

			/*base_window_signal_connect( window,
					G_OBJECT( model->private->treeview ), "drag-drop", G_CALLBACK( on_drag_drop ));*/

			g_signal_connect(
					priv->treeview, "drag-end",
					G_CALLBACK( nact_tree_model_dnd_on_drag_end ), priv->window );

			/* set_edition_mode */
			fma_settings_register_key_callback(
					IPREFS_ITEMS_LIST_ORDER_MODE,
					G_CALLBACK( on_settings_order_mode_changed ),
					model );

			/* connect_item_updated_signal */
			g_signal_connect(
					priv->window,
					MAIN_SIGNAL_TAB_UPDATED, G_CALLBACK( on_main_item_updated ), model );
		}

	}
}
#endif

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_dispose";
	NactTreeModel *self;
	GtkTreeStore *ts_model;

	g_return_if_fail( NACT_IS_TREE_MODEL( object ));

	self = NACT_TREE_MODEL( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( self )));
		gtk_tree_store_clear( ts_model );
		g_debug( "%s: tree store cleared", thisfn );

		g_object_unref( self->private->clipboard );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_finalize";
	NactTreeModel *self;

	g_return_if_fail( NACT_IS_TREE_MODEL( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = NACT_TREE_MODEL( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * nact_tree_model_new:
 *
 * Returns: a newly created NactTreeModel object.
 */
NactTreeModel *
nact_tree_model_new( GtkTreeView *treeview )
{
	static const gchar *thisfn = "nact_tree_model_new";
	GtkTreeStore *ts_model;
	NactTreeModel *model;
	gint order_mode;

	g_return_val_if_fail( treeview && GTK_IS_TREE_VIEW( treeview ), NULL );

	g_debug( "%s: treeview=%p", thisfn, ( void * ) treeview );

	/* create the underlying tree store */
	ts_model = gtk_tree_store_new(
			TREE_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, FMA_TYPE_OBJECT );

	/* create our filter model */
	model = g_object_new( NACT_TYPE_TREE_MODEL,
			"child-model", ts_model,
			NULL );
	g_object_unref( ts_model );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, model, NULL );

	model->private->treeview = treeview;

	/* initialize the sortable interface */
	order_mode = fma_iprefs_get_order_mode( NULL );
	display_order_change( model, order_mode );

	return( model );
}

/**
 * nact_tree_model_set_main_window:
 * @tmodel: this #NactTreeModel instance
 * @main_window: the #NactMainWindow.
 *
 * Attach the main window to the tree model, which is required to get
 * a clipboard object, which is required to have a functional drag and
 * drop.
 */
void
nact_tree_model_set_main_window( NactTreeModel *tmodel, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_set_main_window";
	NactTreeModelPrivate *priv;

	g_return_if_fail( tmodel && NACT_IS_TREE_MODEL( tmodel ));
	g_return_if_fail( window && NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: tmodel=%p, window=%p", thisfn, ( void * ) tmodel, ( void * ) window );

	priv = tmodel->private;

	if( !priv->dispose_has_run ){

		priv->window = window;
		priv->clipboard = fma_clipboard_new( window );

		/* depends of window and edition mode */
		connect_item_updated_signal( tmodel );
	}
}

/**
 * nact_tree_model_set_edition_mode:
 * @tmodel: this #NactTreeModel instance
 * @mode: the edition mode
 *
 * Set the edition mode which is required in order to rightly initialize
 * the drag and drop code.
 */
void
nact_tree_model_set_edition_mode( NactTreeModel *tmodel, guint mode )
{
	static const gchar *thisfn = "nact_tree_model_set_edition_mode";
	NactTreeModelPrivate *priv;

	g_return_if_fail( tmodel && NACT_IS_TREE_MODEL( tmodel ));

	g_debug( "%s: tmodel=%p, mode=%u", thisfn, ( void * ) tmodel, mode );

	priv = tmodel->private;

	if( !priv->dispose_has_run ){

		priv->mode = mode;

		if( priv->mode == TREE_MODE_EDITION ){

			fma_settings_register_key_callback(
					IPREFS_ITEMS_LIST_ORDER_MODE,
					G_CALLBACK( on_settings_order_mode_changed ),
					tmodel );

			/* depends of window and edition mode */
			connect_item_updated_signal( tmodel );

			/* depend of treeview and edition mode set */
			setup_dnd_edition( tmodel );
		}
	}
}

/*
 * connect_item_updated_signal:
 *
 * Monitor the item updates when we have set the main window and if we
 * are in edition mode
 */
static void
connect_item_updated_signal( NactTreeModel *tmodel )
{
	NactTreeModelPrivate *priv;

	priv = tmodel->private;

	if( priv->window && priv->mode == TREE_MODE_EDITION && !priv->item_updated_connected ){

		g_signal_connect( priv->window,
				MAIN_SIGNAL_ITEM_UPDATED, G_CALLBACK( on_main_item_updated ), tmodel );

		priv->item_updated_connected = TRUE;
	}
}

/*
 * FMASettings callback for a change on IPREFS_ITEMS_LIST_ORDER_MODE key
 */
static void
on_settings_order_mode_changed( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_on_settings_order_mode_changed";
	const gchar *order_mode_str;
	guint order_mode;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		order_mode_str = ( const gchar * ) new_value;
		order_mode = fma_iprefs_get_order_mode_by_label( order_mode_str );

		g_debug( "%s: group=%s, key=%s, order_mode=%u (%s), mandatory=%s, model=%p (%s)",
				thisfn, group, key, order_mode, order_mode_str,
				mandatory ? "True":"False", ( void * ) model, G_OBJECT_TYPE_NAME( model ));

		display_order_change( model, order_mode );
	}
}

/*
 * if force_display is true, then refresh the display of the view
 */
static void
on_main_item_updated( BaseWindow *window, FMAIContext *context, guint data, NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_on_main_item_updated";
	GtkTreePath *path;
	GtkTreeStore *store;
	GtkTreeIter iter;

	if( !model->private->dispose_has_run ){

		g_debug( "%s: window=%p, context=%p (%s), data=%u, model=%p",
				thisfn,
				( void * ) window,
				( void * ) context, G_OBJECT_TYPE_NAME( context ),
				data,
				( void * ) model );

		if( data & ( MAIN_DATA_LABEL | MAIN_DATA_ICON )){
			path = nact_tree_model_object_to_path( model, ( FMAObject * ) context );
			if( path ){
				store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
				if( gtk_tree_model_get_iter( GTK_TREE_MODEL( store ), &iter, path )){
					display_item( store, model->private->treeview, &iter, ( FMAObject * ) context );
					gtk_tree_model_row_changed( GTK_TREE_MODEL( store ), path, &iter );
				}
				gtk_tree_path_free( path );
			}
		}
	}
}

static void
setup_dnd_edition( NactTreeModel *tmodel )
{
	NactTreeModelPrivate *priv;

	priv = tmodel->private;

	if( priv->mode == TREE_MODE_EDITION && priv->treeview && !priv->dnd_setup ){

		egg_tree_multi_drag_add_drag_support(
				EGG_TREE_MULTI_DRAG_SOURCE( tmodel ),
				priv->treeview );

		gtk_tree_view_enable_model_drag_dest(
				priv->treeview,
				tree_model_dnd_dest_formats,
				tree_model_dnd_dest_formats_count,
				nact_tree_model_dnd_imulti_drag_source_get_drag_actions( EGG_TREE_MULTI_DRAG_SOURCE( tmodel )));

		g_signal_connect(
				priv->treeview, "drag-begin",
				G_CALLBACK( nact_tree_model_dnd_on_drag_begin ), priv->window );

		/* connect: implies that we have to do all hard stuff
		 * connect_after: no more triggers drag-drop signal
		 */
		/*base_window_signal_connect_after( window,
				G_OBJECT( model->private->treeview ), "drag-motion", G_CALLBACK( on_drag_motion ));*/

		/*base_window_signal_connect( window,
				G_OBJECT( model->private->treeview ), "drag-drop", G_CALLBACK( on_drag_drop ));*/

		g_signal_connect(
				priv->treeview, "drag-end",
				G_CALLBACK( nact_tree_model_dnd_on_drag_end ), priv->window );

		priv->dnd_setup = TRUE;
	}
}

/**
 * nact_tree_model_delete:
 * @model: this #NactTreeModel instance.
 * @object: the #FMAObject to be deleted.
 *
 * Recursively deletes the specified object.
 *
 * Returns: a path which may be suitable for the next selection.
 */
GtkTreePath *
nact_tree_model_delete( NactTreeModel *model, FMAObject *object )
{
	GtkTreePath *path;
	static const gchar *thisfn = "nact_tree_model_delete";
	GtkTreeIter iter;
	GtkTreeStore *store;
	FMAObjectItem *parent;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	path = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, object=%p (%s)",
				thisfn, ( void * ) model, ( void * ) object, object ? G_OBJECT_TYPE_NAME( object ) : "null" );

		path = nact_tree_model_object_to_path( model, object );

		if( path != NULL ){

			/* detaching to-be-deleted object from its current parent
			 */
			parent = fma_object_get_parent( object );
			g_debug( "%s: object=%p, parent=%p", thisfn, ( void * ) object, ( void * ) parent );
			if( parent ){
				fma_object_remove_item( parent, object );
			}

			/* then recursively remove the object and its children from the store
			 */
			store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
			if( gtk_tree_model_get_iter( GTK_TREE_MODEL( store ), &iter, path )){
				delete_items_rec( store, &iter );
			}
		}
	}

	return( path );
}

/**
 * nact_tree_model_fill:
 * @model: this #NactTreeModel instance.
 * @ŧreeview: the #GtkTreeView widget.
 * @items: this list of items, usually from #FMAPivot, which will be used
 *  to fill up the tree store.
 *
 * Fill up the tree store with specified items.
 *
 * We enter with the GSList owned by FMAPivot which contains the ordered
 * list of level-zero items. We want have a duplicate of this list in
 * tree store, so that we are able to freely edit it.
 */
void
nact_tree_model_fill( NactTreeModel *model, GList *items )
{
	static const gchar *thisfn = "nact_tree_model_fill";
	GtkTreeStore *ts_model;
	GList *it;
	FMAObject *duplicate;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	g_debug( "%s: model=%p, items=%p (count=%d)",
			thisfn, ( void * ) model, ( void * ) items, g_list_length( items ));

	if( !model->private->dispose_has_run ){

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		gtk_tree_store_clear( ts_model );

		for( it = items ; it ; it = it->next ){
			duplicate = ( FMAObject * ) fma_object_duplicate( it->data, DUPLICATE_REC );
			fma_object_check_status( duplicate );
			fill_tree_store( ts_model, model->private->treeview, duplicate, NULL );
			fma_object_unref( duplicate );
		}
	}
}

/**
 * nact_tree_model_insert_before:
 * @model: this #NactTreeModel instance.
 * @object: a #FMAObject-derived object to be inserted.
 * @path: the #GtkTreePath which specifies the insertion path.
 * @parent: set to the parent or the object itself.
 *
 * Insert a new row at the given position.
 *
 * Gtk API uses to returns iter ; but at least when inserting a new
 * profile in an action, we may have store_iter_path="0:1" (good), but
 * iter_path="0:0" (bad) - so we'd rather return a string path.
 *
 * Returns: the actual insertion path, which may be different from the
 * asked insertion path if tree is sorted.
 */
GtkTreePath *
nact_tree_model_insert_before( NactTreeModel *model, const FMAObject *object, GtkTreePath *path )
{
	static const gchar *thisfn = "nact_tree_model_insert_before";
	GtkTreeModel *store;
	gchar *path_str;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *parent_path;
	GtkTreePath *inserted_path;
	FMAObject *parent_obj;
	gboolean has_parent;
	GtkTreeIter sibling_iter;
	FMAObject *sibling_obj;
	gboolean has_sibling;

	path_str = gtk_tree_path_to_string( path );
	g_debug( "%s: model=%p, object=%p (%s, ref_count=%d), path=%p (%s)",
			thisfn,
			( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count,
			( void * ) path, path_str );
	g_free( path_str );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( FMA_IS_OBJECT( object ), NULL );

	inserted_path = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		has_parent = FALSE;
		parent_obj = NULL;
		sibling_obj = NULL;

		remove_if_exists( model, store, object );

		/* may be FALSE when store is empty */
		has_sibling = gtk_tree_model_get_iter( store, &sibling_iter, path );
		if( has_sibling ){
			gtk_tree_model_get( store, &sibling_iter, TREE_COLUMN_NAOBJECT, &sibling_obj, -1 );
			g_object_unref( sibling_obj );
		}
		g_debug( "%s: has_sibling=%s, sibling_obj=%p", thisfn, has_sibling ? "True":"False", ( void * ) sibling_obj );

		if( gtk_tree_path_get_depth( path ) > 1 ){

			has_parent = TRUE;
			parent_path = gtk_tree_path_copy( path );
			gtk_tree_path_up( parent_path );
			gtk_tree_model_get_iter( store, &parent_iter, parent_path );
			gtk_tree_path_free( parent_path );

			gtk_tree_model_get( store, &parent_iter, TREE_COLUMN_NAOBJECT, &parent_obj, -1 );
			g_object_unref( parent_obj );

			if( has_sibling ){
				fma_object_insert_item( parent_obj, object, sibling_obj );
			} else {
				fma_object_append_item( parent_obj, object );
			}

			fma_object_set_parent( object, parent_obj );
		}
		g_debug( "%s: has_parent=%s, parent_obj=%p", thisfn, has_parent ? "True":"False", ( void * ) parent_obj );

		gtk_tree_store_insert_before(
				GTK_TREE_STORE( store ), &iter,
				has_parent ? &parent_iter : NULL,
				has_sibling ? &sibling_iter : NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, TREE_COLUMN_NAOBJECT, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );

		inserted_path = gtk_tree_model_get_path( store, &iter );
		path_str = gtk_tree_path_to_string( inserted_path );
		g_debug( "%s: object %p (%s) inserted at path %s",
				thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ), path_str );
		g_free( path_str );
	}

	return( inserted_path );
}

/**
 * nact_tree_model_insert_into:
 * @model: this #NactTreeModel instance.
 * @object: the #FMAObject to be inserted.
 * @path: the wished #GtkTreePath path.
 *
 * Insert the @object at ou near the wished @path, and attaches the object
 * to its new parent.
 *
 * Returns the actual insertion path, wchich should be gtk_tree_path_free()
 * by the caller.
 */
GtkTreePath *
nact_tree_model_insert_into( NactTreeModel *model, const FMAObject *object, GtkTreePath *path )
{
	static const gchar *thisfn = "nact_tree_model_insert_into";
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *new_path;
	gchar *path_str;
	FMAObject *parent;

	path_str = gtk_tree_path_to_string( path );
	g_debug( "%s: model=%p, object=%p (%s, ref_count=%d), path=%p (%s), parent=%p",
			thisfn,
			( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count,
			( void * ) path, path_str, ( void * ) parent );
	g_free( path_str );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( FMA_IS_OBJECT( object ), NULL );

	new_path = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		if( !gtk_tree_model_get_iter( store, &parent_iter, path )){
			path_str = gtk_tree_path_to_string( path );
			g_warning( "%s: unable to get iter at path %s", thisfn, path_str );
			g_free( path_str );
			return( NULL );
		}

		gtk_tree_model_get( store, &parent_iter, TREE_COLUMN_NAOBJECT, &parent, -1 );
		g_object_unref( parent );
		fma_object_insert_item( parent, object, NULL );
		fma_object_set_parent( object, parent );

		gtk_tree_store_insert_after( GTK_TREE_STORE( store ), &iter, &parent_iter, NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, TREE_COLUMN_NAOBJECT, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );

		new_path = gtk_tree_model_get_path( store, &iter );
		path_str = gtk_tree_path_to_string( new_path );
		g_debug( "%s: object %p (%s) inserted at path %s",
				thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ), path_str );
		g_free( path_str );
	}

	return( new_path );
}

/**
 * nact_tree_model_get_item_by_id:
 * @model: this #NactTreeModel object.
 * @id: the searched #FMAObjectItem.
 *
 * Returns: a pointer on the searched #FMAObjectItem if it exists, or %NULL.
 *
 * The returned pointer is owned by the underlying tree store, and should
 * not be released by the caller.
 */
FMAObjectItem *
nact_tree_model_get_item_by_id( const NactTreeModel *model, const gchar *id )
{
	static const gchar *thisfn = "nact_tree_model_get_item_by_id";
	GtkTreeStore *store;
	ntmFindId nfi;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	nfi.object = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, id=%s", thisfn, ( void * ) model, id );

		nfi.id = ( gchar * ) id;
		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, ( FnIterOnStore ) find_item_iter, &nfi );
	}

	return(( FMAObjectItem * ) nfi.object );
}

/**
 * nact_tree_model_get_items:
 * @model: this #NactTreeModel object.
 * @mode: the content indicator for the returned list
 *
 * Returns: the content of the current store as a newly allocated list
 * which should be fma_object_free_items() by the caller.
 */
GList *
nact_tree_model_get_items( const NactTreeModel *model, guint mode )
{
	static const gchar *thisfn = "nact_tree_model_get_items";
	GList *items;
	GtkTreeStore *store;
	ntmGetItems ngi;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	items = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, mode=0x%xh", thisfn, ( void * ) model, mode );

		ngi.mode = mode;
		ngi.items = NULL;
		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, ( FnIterOnStore ) get_items_iter, &ngi );
		items = g_list_reverse( ngi.items );
	}

	return( items );
}

/**
 * nact_tree_model_object_at_path:
 * @model: this #NactTreeModel instance.
 * @path: the #GtkTreePath to be searched.
 *
 * Returns: the #FMAObject at the given @path if any, or NULL.
 *
 * The reference count of the object is not modified. The returned reference
 * is owned by the tree store and should not be released by the caller.
 */
FMAObject *
nact_tree_model_object_at_path( const NactTreeModel *model, GtkTreePath *path )
{
	FMAObject *object;
	GtkTreeModel *store;
	GtkTreeIter iter;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	object = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		if( gtk_tree_model_get_iter( store, &iter, path )){
			gtk_tree_model_get( store, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );
			g_object_unref( object );
		}
	}

	return( object );
}

/**
 * nact_tree_model_object_to_path:
 * @model: this #NactTreeModel.
 * @object: the searched FMAObject.
 *
 * Returns: a newly allocated GtkTreePath which is the current position
 * of @object in the tree store, or %NULL.
 *
 * The returned path should be gtk_tree_path_free() by the caller.
 */
GtkTreePath *
nact_tree_model_object_to_path( const NactTreeModel *model, const FMAObject *object )
{
	static const gchar *thisfn = "nact_tree_model_object_to_path";
	ntmFindObject nfo;
	GtkTreeIter iter;
	GtkTreeStore *store;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	nfo.path = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, object=%p (%s)",
				thisfn, ( void * ) model, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		nfo.object = object;
		nfo.iter = &iter;

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, ( FnIterOnStore ) find_object_iter, &nfo );
	}

	return( nfo.path );
}

static void
append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const FMAObject *object )
{
	/*g_debug( "nact_tree_model_append_item: object=%p (ref_count=%d), parent=%p",
					( void * ) object, G_OBJECT( object )->ref_count, ( void * ) parent );*/

	gtk_tree_store_append( model, iter, parent );
	gtk_tree_store_set( model, iter, TREE_COLUMN_NAOBJECT, object, -1 );
	display_item( model, treeview, iter, object );
}

static void
display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const FMAObject *object )
{
	gchar *label = fma_object_get_label( object );
	gtk_tree_store_set( model, iter, TREE_COLUMN_LABEL, label, -1 );
	g_free( label );

	if( FMA_IS_OBJECT_ITEM( object )){
		gchar *icon_name = fma_object_get_icon( object );
		GdkPixbuf *icon = base_gtk_utils_get_pixbuf( icon_name, GTK_WIDGET( treeview ), GTK_ICON_SIZE_MENU );
		gtk_tree_store_set( model, iter, TREE_COLUMN_ICON, icon, -1 );
		g_object_unref( icon );
	}
}

/*
 * nact_tree_model_display_order_change:
 * @model: this #NactTreeModel.
 * @order_mode: the new order mode.
 *
 * Setup the new order mode.
 */
static void
display_order_change( NactTreeModel *model, gint order_mode )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		g_object_set_data( G_OBJECT( store ), TREE_MODEL_ORDER_MODE, GINT_TO_POINTER( order_mode ));

		switch( order_mode ){

			case IPREFS_ORDER_ALPHA_ASCENDING:

				gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( store ),
						TREE_COLUMN_LABEL, GTK_SORT_ASCENDING );

				gtk_tree_sortable_set_sort_func( GTK_TREE_SORTABLE( store ),
						TREE_COLUMN_LABEL, ( GtkTreeIterCompareFunc ) sort_actions_list, NULL, NULL );
				break;

			case IPREFS_ORDER_ALPHA_DESCENDING:

				gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( store ),
						TREE_COLUMN_LABEL, GTK_SORT_DESCENDING );

				gtk_tree_sortable_set_sort_func( GTK_TREE_SORTABLE( store ),
						TREE_COLUMN_LABEL, ( GtkTreeIterCompareFunc ) sort_actions_list, NULL, NULL );
				break;

			case IPREFS_ORDER_MANUAL:
			default:

				gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( store ),
						GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, 0 );
				break;
		}
	}
}

#if 0
/*
 * dump:
 * @model: this #NactTreeModel instance.
 *
 * Briefly dumps the content of the tree.
 */
static void
dump( NactTreeModel *model )
{
	GList *items;

	items = nact_tree_model_get_items( model, TREE_LIST_ALL );
	fma_object_dump_tree( items );
	fma_object_free_items( items );
}

static gboolean
dump_store( NactTreeModel *model, GtkTreePath *path, FMAObject *object, ntmDumpStruct *ntm )
{
	gint depth;
	gint i;
	GString *prefix;
	gchar *id, *label;
	FMAObjectItem *origin;

	depth = gtk_tree_path_get_depth( path );
	prefix = g_string_new( ntm->prefix );
	for( i=1 ; i<depth ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	id = fma_object_get_id( object );
	label = fma_object_get_label( object );
	origin = ( FMAObjectItem * ) fma_object_get_origin( object );
	g_debug( "%s: %s%s at %p (ref_count=%d) \"[%s] %s\" origin=%p (%s)",
			ntm->fname, prefix->str,
			G_OBJECT_TYPE_NAME( object ), ( void * ) object, G_OBJECT( object )->ref_count, id, label,
			( void * ) origin, origin ? G_OBJECT_TYPE_NAME( object ) : "null" );
	g_free( label );
	g_free( id );

	g_string_free( prefix, TRUE );

	/* don't stop iteration */
	return( FALSE );
}
#endif

static void
fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, FMAObject *object, GtkTreeIter *parent )
{
	static const gchar *thisfn = "nact_tree_model_fill_tree_store";
	GList *subitems, *it;
	GtkTreeIter iter;

	g_debug( "%s entering: object=%p (%s, ref_count=%d)", thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

	/* an action or a menu
	 */
	if( FMA_IS_OBJECT_ITEM( object )){
		append_item( model, treeview, parent, &iter, object );
		subitems = fma_object_get_items( object );
		for( it = subitems ; it ; it = it->next ){
			fill_tree_store( model, treeview, it->data, &iter );
		}

	} else {
		g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));
		append_item( model, treeview, parent, &iter, object );
	}

	/*g_debug( "%s quitting: object=%p (%s, ref_count=%d)", thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );*/
}

/*
 * Only display profiles when we are in edition mode.
 *
 * This function is called as soon as a new row is created in the tree store,
 * so is called the first time _before_ the FMAObject be set on the row.
 */
static gboolean
filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model )
{
	FMAObject *object;
	FMAObjectAction *action;
	gint count;

	gtk_tree_model_get( store, iter, TREE_COLUMN_NAOBJECT, &object, -1 );

	if( object ){
		g_object_unref( object );

		/* an action or a menu are always displayed, whatever the current
		 * management mode may be
		 */
		if( FMA_IS_OBJECT_ITEM( object )){
			return( TRUE );
		}

		/* profiles are just never displayed in selection mode
		 * in edition mode, they are displayed only when the action has
		 * more than one profile
		 */
		g_return_val_if_fail( FMA_IS_OBJECT_PROFILE( object ), FALSE );

		if( NACT_TREE_MODEL( model )->private->mode == TREE_MODE_SELECTION ){
			return( FALSE );
		}

		action = FMA_OBJECT_ACTION( fma_object_get_parent( object ));
		count = fma_object_get_items_count( action );
		return( count > 1 );
	}

	return( FALSE );
}

/*
 * search for an object, given its id
 */
static gboolean
find_item_iter( NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmFindId *nfi )
{
	gchar *id;
	gboolean found = FALSE;

	if( FMA_IS_OBJECT_ITEM( object )){
		id = fma_object_get_id( object );
		found = ( g_ascii_strcasecmp( id, nfi->id ) == 0 );
		g_free( id );

		if( found ){
			nfi->object = object;
		}
	}

	/* stop iteration if found */
	return( found );
}

static gboolean
find_object_iter( NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmFindObject *nfo )
{
	if( object == nfo->object ){
		if( gtk_tree_model_get_iter( GTK_TREE_MODEL( store ), nfo->iter, path )){
			nfo->path = gtk_tree_path_copy( path );
		}
	}

	/* stop iteration when found */
	return( nfo->path != NULL );
}

/*
 * Builds the tree by iterating on the store
 * we may want selected, modified or both, or a combination of these modes
 *
 * This function is called from iter_on_store_item();
 */
static gboolean
get_items_iter( const NactTreeModel *model, GtkTreeStore *store, GtkTreePath *path, FMAObject *object, ntmGetItems *ngi )
{
	if( ngi->mode & TREE_LIST_ALL ){
		if( gtk_tree_path_get_depth( path ) == 1 ){
			ngi->items = g_list_prepend( ngi->items, fma_object_ref( object ));
		}
	}

	/* don't stop iteration */
	return( FALSE );
}

static void
iter_on_store( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeIter iter;
	gboolean stop;

	if( gtk_tree_model_iter_children( store, &iter, parent )){
		stop = iter_on_store_item( model, store, &iter, fn, user_data );

		while( !stop && gtk_tree_model_iter_next( store, &iter )){
			stop = iter_on_store_item( model, store, &iter, fn, user_data );
		}
	}
}

static gboolean
iter_on_store_item( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data )
{
	FMAObject *object;
	GtkTreePath *path;
	gboolean stop;

	/* unreffing as soon as we got the pointer so that the ref count is
	 * unchanged in dump_store
	 */
	gtk_tree_model_get( store, iter, TREE_COLUMN_NAOBJECT, &object, -1 );
	g_object_unref( object );

	/*
	g_debug( "nact_tree_model_iter_on_store_item: object=%p (%s, ref_count=%d)",
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );
			*/

	path = gtk_tree_model_get_path( store, iter );
	stop = ( *fn )( model, GTK_TREE_STORE( store ), path, object, user_data );
	gtk_tree_path_free( path );

	if( !stop ){
		iter_on_store( model, store, iter, fn, user_data );
	}

	return( stop );
}

/*
 * if the object, identified by its id (historically a uuid), already exists,
 * then remove it first
 */
static void
remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const FMAObject *object )
{
	ntmFindId nfi;
	GtkTreeIter iter;

	if( FMA_IS_OBJECT_ITEM( object )){

		nfi.id = fma_object_get_id( object );
		nfi.object = NULL;
		nfi.iter = &iter;

		iter_on_store( model, store, NULL, ( FnIterOnStore ) find_item_iter, &nfi );

		if( nfi.object ){
			g_debug( "nact_tree_model_remove_if_exists: removing %s %p",
					G_OBJECT_TYPE_NAME( object ), ( void * ) object );
			gtk_tree_store_remove( GTK_TREE_STORE( store ), nfi.iter );
		}

		g_free( nfi.id );
	}
}

/*
 * recursively delete child items starting with iter
 * returns TRUE if iter is always valid after the remove
 */
static gboolean
delete_items_rec( GtkTreeStore *store, GtkTreeIter *iter )
{
	GtkTreeIter child;
	gboolean valid;

	while( gtk_tree_model_iter_children( GTK_TREE_MODEL( store ), &child, iter )){
		delete_items_rec( store, &child );
	}
	valid = gtk_tree_store_remove( store, iter );

	return( valid );
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_tree_model_sort_actions_list";*/
	FMAObjectId *obj_a, *obj_b;
	gint ret;

	/*g_debug( "%s: model=%p, a=%p, b=%p, window=%p", thisfn, ( void * ) model, ( void * ) a, ( void * ) b, ( void * ) window );*/

	gtk_tree_model_get( model, a, TREE_COLUMN_NAOBJECT, &obj_a, -1 );
	gtk_tree_model_get( model, b, TREE_COLUMN_NAOBJECT, &obj_b, -1 );

	g_object_unref( obj_b );
	g_object_unref( obj_a );

	if( FMA_IS_OBJECT_PROFILE( obj_a )){
		ret = 0;
	} else {
		ret = fma_object_sort_alpha_asc( obj_a, obj_b );
	}

	/*g_debug( "%s: ret=%d", thisfn, ret );*/
	return( ret );
}
