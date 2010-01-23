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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include <api/na-object-api.h>

#include <runtime/na-iprefs.h>
#include <runtime/na-utils.h>

#include "nact-application.h"
#include "nact-gtk-utils.h"
#include "nact-main-tab.h"
#include "nact-providers-list.h"

/* column ordering
 */
enum {
	PROVIDERS_READABLE_COLUMN = 0,
	PROVIDERS_WRITABLE_COLUMN,
	PROVIDERS_LIBELLE_COLUMN,
	PROVIDERS_N_COLUMN
};

#define PROVIDERS_LIST_TREEVIEW			"nact-providers-list-treeview"

static gboolean st_on_selection_change = FALSE;

static void       init_view_setup_defaults( GtkTreeView *treeview, BaseWindow *window );
static void       init_view_connect_signals( GtkTreeView *treeview, BaseWindow *window );
static void       init_view_select_first_row( GtkTreeView *treeview );

/*static gboolean   iter_for_reset( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data );*/
static void       iter_for_setup( gchar *scheme, GtkTreeModel *model );
static gboolean   iter_for_get( GtkTreeModel* scheme_model, GtkTreePath *path, GtkTreeIter* iter, GSList **schemes_list );
static GSList    *get_list_schemes( GtkTreeView *treeview );
static gboolean   get_list_schemes_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList **list );

static gboolean   on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window );
static void       on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void       on_add_clicked( GtkButton *button, BaseWindow *window );
static void       on_remove_clicked( GtkButton *button, BaseWindow *window );
static void       on_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window );
static void       on_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window );
static void       on_active_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window );

static void       edit_cell( BaseWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );
static void       edit_inline( BaseWindow *window );
static void       insert_new_row( BaseWindow *window );
static void       delete_row( BaseWindow *window );

static GtkButton *get_add_button( BaseWindow *window );
static GtkButton *get_remove_button( BaseWindow *window );
/*static GSList    *get_gconf_subdirs( GConfClient *gconf, const gchar *path );
static void       free_gconf_subdirs( GSList *subdirs );
static void       free_gslist( GSList *list );*/

/**
 * nact_providers_list_create_providers_list:
 * @treeview: the #GtkTreeView.
 *
 * Create the treeview model when initially loading the widget from
 * the UI manager.
 */
void
nact_providers_list_create_model( GtkTreeView *treeview )
{
	static const char *thisfn = "nact_providers_list_create_model";
	GtkListStore *model;
	GtkCellRenderer *toggled_cell;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;
	GtkTreeSelection *selection;

	g_debug( "%s: treeview=%p", thisfn, ( void * ) treeview );
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	model = gtk_list_store_new( PROVIDERS_N_COLUMN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING );
	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	toggled_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "To be read" ),
			toggled_cell,
			"active", PROVIDERS_READABLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	toggled_cell = gtk_cell_renderer_toggle_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "Writable" ),
			toggled_cell,
			"active", PROVIDERS_WRITABLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	text_cell = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			_( "I/O Provider" ),
			text_cell,
			"text", PROVIDERS_LIBELLE_COLUMN,
			NULL );
	gtk_tree_view_append_column( treeview, column );

	gtk_tree_view_set_headers_visible( treeview, TRUE );

	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
}

/**
 * nact_providers_list_init_view:
 * @treeview: the #GtkTreeView.
 * @window: the parent #BaseWindow which embeds the view.
 *
 * Connects signals at runtime initialization of the widget, and setup
 * current default values.
 */
void
nact_providers_list_init_view( GtkTreeView *treeview, BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_init_view";
	GtkButton *button;

	g_debug( "%s: treeview=%p, window=%p", thisfn, ( void * ) treeview, ( void * ) window );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	g_object_set_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW, treeview );

	init_view_setup_defaults( treeview, window );
	init_view_connect_signals( treeview, window );

	init_view_select_first_row( treeview );
}

static void
init_view_setup_defaults( GtkTreeView *treeview, BaseWindow *window )
{
	GtkListStore *model;
	GSList *schemes, *iter;
	GtkTreeIter row;
	gchar **tokens;

	model = GTK_LIST_STORE( gtk_tree_view_get_model( treeview ));

	schemes = get_default_providers_list( window );

	for( iter = schemes ; iter ; iter = iter->next ){

		tokens = g_strsplit(( gchar * ) iter->data, "|", 2 );
		gtk_list_store_append( model, &row );
		gtk_list_store_set( model, &row,
				PROVIDERS_CHECKBOX_COLUMN, FALSE,
				PROVIDERS_KEYWORD_COLUMN, tokens[0],
				PROVIDERS_DESC_COLUMN, tokens[1],
				-1 );
		g_strfreev( tokens );
	}

	na_utils_free_string_list( schemes );
}

static void
init_view_connect_signals( GtkTreeView *treeview, BaseWindow *window )
{
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkButton *add_button, *remove_button;

	column = gtk_tree_view_get_column( treeview, PROVIDERS_CHECKBOX_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect(
			window,
			G_OBJECT( renderers->data ),
			"toggled",
			G_CALLBACK( on_active_toggled ));

	column = gtk_tree_view_get_column( treeview, PROVIDERS_KEYWORD_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect(
			window,
			G_OBJECT( renderers->data ),
			"edited",
			G_CALLBACK( on_keyword_edited ));

	column = gtk_tree_view_get_column( treeview, PROVIDERS_DESC_COLUMN );
	renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( column ));
	base_window_signal_connect(
			window,
			G_OBJECT( renderers->data ),
			"edited",
			G_CALLBACK( on_desc_edited ));

	add_button = get_add_button( window );
	base_window_signal_connect(
			window,
			G_OBJECT( add_button ),
			"clicked",
			G_CALLBACK( on_add_clicked ));

	remove_button = get_remove_button( window );
	base_window_signal_connect(
			window,
			G_OBJECT( remove_button ),
			"clicked",
			G_CALLBACK( on_remove_clicked ));

	base_window_signal_connect(
			window,
			G_OBJECT( gtk_tree_view_get_selection( treeview )),
			"changed",
			G_CALLBACK( on_selection_changed ));

	base_window_signal_connect(
			window,
			G_OBJECT( treeview ),
			"key-press-event",
			G_CALLBACK( on_key_pressed_event ));
}

static void
init_view_select_first_row( GtkTreeView *treeview )
{
	GtkTreeSelection *selection;
	GtkTreePath *path;

	path = gtk_tree_path_new_first();
	selection = gtk_tree_view_get_selection( treeview );
	gtk_tree_selection_select_path( selection, path );
	gtk_tree_path_free( path );
}

/**
 * nact_providers_list_save:
 * @window: the #BaseWindow which embeds this treeview.
 *
 * Save the I/O provider status as a GConf preference.
 */
void
nact_providers_list_save( BaseWindow *window )
{
	GtkTreeView *treeview;
	GSList *schemes;
	NactApplication *application;
	NAPivot *pivot;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	schemes = get_list_schemes( treeview );
	application = NACT_APPLICATION( base_window_get_application( window ));
	pivot = nact_application_get_pivot( application );

	na_iprefs_write_string_list( NA_IPREFS( pivot ), "schemes", schemes );

	na_utils_free_string_list( schemes );
}

static GSList *
get_list_schemes( GtkTreeView *treeview )
{
	GSList *list = NULL;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model( treeview );
	gtk_tree_model_foreach( model, ( GtkTreeModelForeachFunc ) get_list_providers_iter, &list );

	return( list );
}

static gboolean
get_list_providers_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList **list )
{
	gchar *keyword;
	gchar *description;
	gchar *scheme;

	gtk_tree_model_get( model, iter, PROVIDERS_KEYWORD_COLUMN, &keyword, PROVIDERS_DESC_COLUMN, &description, -1 );
	scheme = g_strdup_printf( "%s|%s", keyword, description );
	g_free( description );
	g_free( keyword );

	( *list ) = g_slist_append(( *list ), scheme );

	return( FALSE ); /* don't stop looping */
}

/**
 * nact_providers_list_dispose:
 * @treeview: the #GtkTreeView.
 */
void
nact_providers_list_dispose( BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_dispose";
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	selection = gtk_tree_view_get_selection( treeview );

	gtk_tree_selection_unselect_all( selection );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static gboolean
on_key_pressed_event( GtkWidget *widget, GdkEventKey *event, BaseWindow *window )
{
	gboolean stop;
	GtkTreeView *treeview;
	gboolean editable;

	/*g_debug( "nact_providers_list_on_key_pressed_event" );*/

	stop = FALSE;
	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( treeview ), PROVIDERS_LIST_EDITABLE ));

	if( editable ){

		if( event->keyval == GDK_F2 ){
			edit_inline( window );
			stop = TRUE;
		}

		if( event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert ){
			insert_new_row( window );
			stop = TRUE;
		}

		if( event->keyval == GDK_Delete || event->keyval == GDK_KP_Delete ){
			delete_row( window );
			stop = TRUE;
		}
	}

	return( stop );
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	/*static const gchar *thisfn = "nact_providers_list_on_selection_changed";*/
	GtkTreeView *treeview;
	gboolean editable;
	GtkButton *button;

	/*g_debug( "%s: selection=%p, window=%p", thisfn, ( void * ) selection, ( void * ) window );*/

	/*g_debug( "%s: getting data on window=%p", thisfn, ( void * ) window );*/
	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));

	/*g_debug( "%s: getting data on treeview=%p", thisfn, ( void * ) treeview );*/
	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( treeview ), PROVIDERS_LIST_EDITABLE ));
	/*g_debug( "%s: editable=%s, selected_rows=%d",
			thisfn, editable ? "True":"False", gtk_tree_selection_count_selected_rows( selection ));*/

	button = get_remove_button( window );
	gtk_widget_set_sensitive( GTK_WIDGET( button ), editable && gtk_tree_selection_count_selected_rows( selection ) > 0);
}

static void
on_add_clicked( GtkButton *button, BaseWindow *window )
{
	insert_new_row( window );
}

static void
on_remove_clicked( GtkButton *button, BaseWindow *window )
{
	delete_row( window );
}

/*
 * do not allow edition of scheme description when editing an action
 */
static void
on_desc_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_on_desc_edited";

	g_debug( "%s: renderer=%p, path=%s, text=%s, window=%p",
			thisfn, ( void * ) renderer, path, text, ( void * ) window );

	edit_cell( window, path, text, PROVIDERS_DESC_COLUMN, NULL, NULL );
}

static void
on_keyword_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, BaseWindow *window )
{
	gboolean state = FALSE;
	gchar *old_text = NULL;
	NAObjectProfile *edited;

	edit_cell( window, path, text, PROVIDERS_KEYWORD_COLUMN, &state, &old_text );

	if( state ){
		/*g_debug( "%s: old_scheme=%s", thisfn, old_text );*/
		if( g_object_class_find_property( G_OBJECT_GET_CLASS( window ), TAB_UPDATABLE_PROP_EDITED_PROFILE )){
			g_object_get(
					G_OBJECT( window ),
					TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
					NULL );
			if( edited ){
				na_object_profile_set_scheme( edited, old_text, FALSE );
				na_object_profile_set_scheme( edited, text, TRUE );
				g_signal_emit_by_name( G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
			}
		}
	}

	g_free( old_text );
}

static void
on_active_toggled( GtkCellRendererToggle *renderer, gchar *path, BaseWindow *window )
{
	GtkTreeView *treeview;
	gboolean editable;
	NAObjectProfile *edited;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gboolean state;
	gchar *scheme;

	if( !st_on_selection_change ){

		treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
		editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( treeview ), PROVIDERS_LIST_EDITABLE ));
		model = gtk_tree_view_get_model( treeview );
		tree_path = gtk_tree_path_new_from_string( path );
		gtk_tree_model_get_iter( model, &iter, tree_path );
		gtk_tree_path_free( tree_path );
		gtk_tree_model_get( model, &iter, PROVIDERS_CHECKBOX_COLUMN, &state, PROVIDERS_KEYWORD_COLUMN, &scheme, -1 );

			/* gtk_tree_model_get: returns the previous state
			g_debug( "%s: gtk_tree_model_get returns keyword=%s state=%s", thisfn, scheme, state ? "True":"False" );*/

		if( !editable ){
			g_signal_handlers_block_by_func(( gpointer ) renderer, on_active_toggled, window );
			gtk_cell_renderer_toggle_set_active( renderer, state );
			g_signal_handlers_unblock_by_func(( gpointer ) renderer, on_active_toggled, window );

		} else {
			gtk_list_store_set( GTK_LIST_STORE( model ), &iter, PROVIDERS_CHECKBOX_COLUMN, !state, -1 );
			if( g_object_class_find_property( G_OBJECT_GET_CLASS( window ), TAB_UPDATABLE_PROP_EDITED_PROFILE )){
				g_object_get(
						G_OBJECT( window ),
						TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
						NULL );
				if( edited ){
					na_object_profile_set_scheme( edited, scheme, !state );
					g_signal_emit_by_name( G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
				}
			}
		}

		g_free( scheme );
	}
}

static void
edit_cell( BaseWindow *window, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text )
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;

	treeview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( treeview );
	path = gtk_tree_path_new_from_string( path_string );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	if( state && old_text ){
		gtk_tree_model_get( model, &iter, PROVIDERS_CHECKBOX_COLUMN, state, PROVIDERS_KEYWORD_COLUMN, old_text, -1 );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, column, text, -1 );
}

/*
 * do not allow edition of scheme description when editing an action
 */
static void
edit_inline( BaseWindow *window )
{
	static const gchar *thisfn = "nact_providers_list_edit_inline";
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeViewColumn *column;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	selection = gtk_tree_view_get_selection( listview );
	listrows = gtk_tree_selection_get_selected_rows( selection, NULL );

	if( g_list_length( listrows ) == 1 ){
		gtk_tree_view_get_cursor( listview, &path, &column );
		gtk_tree_view_set_cursor( listview, path, column, TRUE );
		gtk_tree_path_free( path );
	}

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );
}

static void
insert_new_row( BaseWindow *window )
{
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *listrows;
	GtkTreePath *path;
	GtkTreeIter iter, sibling;
	gboolean inserted;
	GtkTreeViewColumn *column;

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	model = gtk_tree_view_get_model( listview );
	selection = gtk_tree_view_get_selection( listview );
	listrows = gtk_tree_selection_get_selected_rows( selection, NULL );
	inserted = FALSE;
	column = NULL;

	if( g_list_length( listrows ) == 1 ){
		gtk_tree_view_get_cursor( listview, &path, &column );
		if( gtk_tree_model_get_iter( model, &sibling, path )){
			/* though the path of sibling is correct, the new row is always
			 * inserted at path=0 !
			 */
			/*g_debug( "insert_new_row: sibling=%s", gtk_tree_model_get_string_from_iter( &sibling ));*/
			gtk_list_store_insert_before( GTK_LIST_STORE( model ), &iter, &sibling );
			inserted = TRUE;
		}
		gtk_tree_path_free( path );
	}

	if( !inserted ){
		gtk_list_store_append( GTK_LIST_STORE( model ), &iter );
	}

	if( !column || column == gtk_tree_view_get_column( listview, PROVIDERS_CHECKBOX_COLUMN )){
		column = gtk_tree_view_get_column( listview, PROVIDERS_KEYWORD_COLUMN );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter,
			PROVIDERS_CHECKBOX_COLUMN, FALSE,
			/* i18n notes : scheme name set for a new entry in the scheme list */
			PROVIDERS_KEYWORD_COLUMN, _( "new-scheme" ),
			PROVIDERS_DESC_COLUMN, _( "New scheme description" ),
			-1 );

	g_list_foreach( listrows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( listrows );

	path = gtk_tree_model_get_path( model, &iter );
	gtk_tree_view_set_cursor( listview, path, column, TRUE );
	gtk_tree_path_free( path );
}

static void
delete_row( BaseWindow *window )
{
	NAObjectProfile *edited;
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreeIter iter;
	GtkTreePath *path;
	gboolean toggle_state;
	gchar *scheme;

	listview = GTK_TREE_VIEW( g_object_get_data( G_OBJECT( window ), PROVIDERS_LIST_TREEVIEW ));
	selection = gtk_tree_view_get_selection( listview );
	model = gtk_tree_view_get_model( listview );

	rows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter,
				PROVIDERS_CHECKBOX_COLUMN, &toggle_state,
				PROVIDERS_KEYWORD_COLUMN, &scheme, -1 );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		if( toggle_state ){
			if( g_object_class_find_property( G_OBJECT_GET_CLASS( window ), TAB_UPDATABLE_PROP_EDITED_PROFILE )){
				g_object_get(
						G_OBJECT( window ),
						TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
						NULL );
				if( edited ){
					na_object_profile_set_scheme( edited, scheme, FALSE );
					g_signal_emit_by_name( G_OBJECT( window ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
				}
			}
		}

		g_free( scheme );

		if( gtk_tree_model_get_iter( model, &iter, path ) ||
			gtk_tree_path_prev( path )){

			gtk_tree_view_set_cursor( listview, path, NULL, FALSE );
		}
	}

	g_list_foreach( rows, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( rows );
}

static GtkButton *
get_add_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "AddSchemeButton" ));

	return( button );
}

static GtkButton *
get_remove_button( BaseWindow *window )
{
	GtkButton *button;

	button = GTK_BUTTON( base_window_get_widget( window, "RemoveSchemeButton" ));

	return( button );
}

#if 0
/*
 * get_subdirs:
 * @gconf: a  #GConfClient instance.
 * @path: a full path to be readen.
 *
 * Loads the subdirs of the given path.
 *
 * Returns: a GSList of full path subdirectories.
 *
 * The returned list should be free_subdirs() by the
 * caller.
 */
static GSList *
get_gconf_subdirs( GConfClient *gconf, const gchar *path )
{
	static const gchar *thisfn = "get_subdirs";
	GError *error = NULL;
	GSList *list_subdirs;

	list_subdirs = gconf_client_all_dirs( gconf, path, &error );

	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		g_error_free( error );
		return(( GSList * ) NULL );
	}

	return( list_subdirs );
}

/*
 * free_subdirs:
 * @subdirs: a list of subdirs as returned by get_subdirs().
 *
 * Release the list of subdirs.
 */
static void
free_gconf_subdirs( GSList *subdirs )
{
	free_gslist( subdirs );
}

/*
 * free_gslist:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
static void
free_gslist( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}
#endif
