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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "core/na-about.h"
#include "core/na-io-provider.h"

#include "nact-main-window.h"
#include "nact-menu.h"
#include "nact-menu-edit.h"
#include "nact-menu-file.h"
#include "nact-menu-view.h"
/*
#include "nact-menu-maintainer.h"
#include "nact-menu-tools.h"
*/
#include "nact-preferences-editor.h"
#include "nact-tree-view.h"

static const gchar *st_uixml_actions    = PKGUIDIR "/nautilus-actions-config-tool.actions";
static const gchar *st_ui_app_menu      = "app-menu";
static const gchar *st_ui_menubar       = "menubar";

static void on_app_about( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_app_help( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_app_preferences( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_app_quit( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_brief_tree_store_dump( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_collapse_all( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_copy( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_cut( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_delete( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_dump_clipboard( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_dump_selection( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_duplicate( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_expand_all( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_export( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_import( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_list_modified_items( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_new_action( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_new_menu( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_new_profile( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_paste( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_paste_into( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_reload( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_save( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_test_function( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_toolbar_activate( GSimpleAction *action, GVariant *parameter, gpointer user_data );
static void on_win_toolbar_changed_state( GSimpleAction *action, GVariant *parameter, gpointer user_data );

/* since the deprecation of GtkAction, I no more know how to display
 * menu item tooltips - but they have been translated and I don't want
 * this work to be lost
 */
typedef struct {
	GActionEntry  action_entry;
	const gchar  *tooltip;
}
	sActionEntry;

static sActionEntry st_app_entries[] = {
		{{ "about", on_app_about, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'About' item navigation */
				N_( "Display informations about this program" )},
		{{ "help", on_app_help, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Help' item navigation */
				N_( "Display help about this program" )},
		{{ "preferences", on_app_preferences, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Preferences' item navigation */
				N_( "Edit your preferences" )},
		{{ "quit", on_app_quit, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Quit' item navigation */
				N_( "Quit the application" )},
};

static sActionEntry st_menubar_entries[] = {
		{{ "new-menu", on_win_new_menu, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'New menu' item navigation */
				N_( "Insert a new menu at the current position" )},
		{{ "new-action", on_win_new_action, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'New action' item navigation */
				N_( "Define a new action" )},
		{{ "new-profile", on_win_new_profile, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'New profile' item navigation */
				N_( "Define a new profile attached to the current action" )},
		{{ "save", on_win_save, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Save' item navigation */
				N_( "Record all the modified actions. Invalid actions will be silently ignored" )},
		{{ "cut", on_win_cut, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Cut' item navigation */
				N_( "Cut the selected item(s) to the clipboard" )},
		{{ "copy", on_win_copy, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Copy' item navigation */
				N_( "Copy the selected item(s) to the clipboard" )},
		{{ "paste", on_win_paste, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Paste' item navigation */
				N_( "Insert the content of the clipboard just before the current position" )},
		{{ "paste-into", on_win_paste_into, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Paste into' item navigation */
				N_( "Insert the content of the clipboard as first child of the current item" )},
		{{ "duplicate", on_win_duplicate, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Duplicate' item navigation */
				N_( "Duplicate the selected item(s)" )},
		{{ "delete", on_win_delete, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Delete' item navigation */
				N_( "Delete the selected item(s)" )},
		{{ "reload", on_win_reload, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Reload items' item navigation */
				N_( "Cancel your current modifications and reload the initial list of menus and actions" )},
		{{ "expand", on_win_expand_all, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Expand all' item navigation */
				N_( "Entirely expand the items hierarchy" )},
		{{ "collapse", on_win_collapse_all, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Collapse all' item navigation */
				N_( "Entirely collapse the items hierarchy" )},
		{{ "toolbar-file", on_win_toolbar_activate, NULL, "false", on_win_toolbar_changed_state },
				/* i18n: status bar tooltip displayed on 'Toolbars/File' item navigation */
				N_( "Display the File toolbar" )},
		{{ "toolbar-edit", on_win_toolbar_activate, NULL, "false", on_win_toolbar_changed_state },
				/* i18n: status bar tooltip displayed on 'Toolbars/Edit' item navigation */
				N_( "Display the Edit toolbar" )},
		{{ "toolbar-tools", on_win_toolbar_activate, NULL, "false", on_win_toolbar_changed_state },
				/* i18n: status bar tooltip displayed on 'Toolbars/Tools' item navigation */
				N_( "Display the Tools toolbar" )},
		{{ "toolbar-help", on_win_toolbar_activate, NULL, "false", on_win_toolbar_changed_state },
				/* i18n: status bar tooltip displayed on 'Toolbars/Help' item navigation */
				N_( "Display the Help toolbar" )},
		{{ "import", on_win_import, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Import' item navigation */
				N_( "Import one or more actions from external files into your configuration" )},
		{{ "export", on_win_export, NULL, NULL, NULL },
				/* i18n: status bar tooltip displayed on 'Export' item navigation */
				N_( "Export one or more actions from your configuration to external files" )},
		{{ "dump-selection", on_win_dump_selection, NULL, NULL, NULL },
				"Recursively dump selected items" },
		{{ "tree-store-dump", on_win_brief_tree_store_dump, NULL, NULL, NULL },
				"Briefly dump the tree store" },
		{{ "list-modified", on_win_list_modified_items, NULL, NULL, NULL },
				"List the modified item(s)" },
		{{ "dump-clipboard", on_win_dump_clipboard, NULL, NULL, NULL },
				"Dump the content of the clipboard object" },
		{{ "fntest", on_win_test_function, NULL, NULL, NULL },
				"Test a function (see nact-menubar-maintainer.c" },
};

static void       free_menu_data( sMenuData *sdata );
static void       on_open_context_menu( NactTreeView *treeview, GdkEventButton *event, NactMainWindow *window );
static void       on_popup_selection_done( GtkMenuShell *menushell, NactMainWindow *window );
static void       on_tree_view_count_changed( NactTreeView *treeview, gboolean reset, gint menus, gint actions, gint profiles, NactMainWindow *window );
static void       on_tree_view_focus_in( NactTreeView *treeview, NactMainWindow *window );
static void       on_tree_view_focus_out( NactTreeView *treeview, NactMainWindow *window );
static void       on_tree_view_modified_status_changed( NactTreeView *treeview, gboolean is_modified, NactMainWindow *window );
static void       on_tree_view_selection_changed( NactTreeView *treeview, GList *selected, NactMainWindow *window );
static void       on_update_sensitivities( NactMainWindow *window, void *empty );

/**
 * nact_menu_app:
 * @application: the #NactApplication.
 *
 * Define the application menu and attach it to the application.
 *
 * In Gnome-Shell, the application menu is attached to the icon in the
 * taskbar. Most often, it contains only general options (e.g. About,
 * Quit, etc.)
 */
void
nact_menu_app( NactApplication *application )
{
	static const gchar *thisfn = "nact_menu_app";
	GError *error;
	GtkBuilder *builder;
	GMenuModel *appmenu, *menubar;
	gint i;

	error = NULL;
	builder = gtk_builder_new();
	if( gtk_builder_add_from_file( builder, st_uixml_actions, &error )){

		appmenu = G_MENU_MODEL( gtk_builder_get_object( builder, st_ui_app_menu ));
		if( !appmenu ){
			g_warning( "%s: unable to find '%s' object in '%s' file",
					thisfn, st_ui_app_menu, st_uixml_actions );
		} else {
			gtk_application_set_app_menu( GTK_APPLICATION( application ), appmenu );
		}

		menubar = G_MENU_MODEL( gtk_builder_get_object( builder, st_ui_menubar ));
		if( !menubar ){
			g_warning( "%s: unable to find '%s' object in '%s' file",
					thisfn, st_ui_menubar, st_uixml_actions );
		} else {
			gtk_application_set_menubar( GTK_APPLICATION( application ), menubar );
		}

	} else {
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
	}
	g_object_unref( builder );

	for( i=0 ; i<G_N_ELEMENTS( st_app_entries ) ; ++i ){
		g_action_map_add_action_entries(
				G_ACTION_MAP( application ),
				&st_app_entries[i].action_entry, 1, application );
	}
}

static void
on_app_about( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	GtkWindow *window;

	g_return_if_fail( user_data && NACT_IS_APPLICATION( user_data ));

	window = gtk_application_get_active_window( GTK_APPLICATION( user_data ));
	g_return_if_fail( window && NACT_IS_MAIN_WINDOW( window ));

	na_about_display( window );
}

static void
on_app_help( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	static const gchar *thisfn = "nact_menu_on_app_help";
	GError *error;

	g_return_if_fail( user_data && NACT_IS_APPLICATION( user_data ));

	error = NULL;
	gtk_show_uri( NULL, "ghelp:nautilus-actions-config-tool", GDK_CURRENT_TIME, &error );
	if( error ){
		g_warning( "%s: %s", thisfn, error->message );
		g_error_free( error );
	}
}

static void
on_app_preferences( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	GtkWindow *window;

	g_return_if_fail( user_data && NACT_IS_APPLICATION( user_data ));

	window = gtk_application_get_active_window( GTK_APPLICATION( user_data ));
	g_return_if_fail( window && NACT_IS_MAIN_WINDOW( window ));

	nact_preferences_editor_run( NACT_MAIN_WINDOW( window ));
}

static void
on_app_quit( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	GtkWindow *window;

	g_return_if_fail( user_data && NACT_IS_APPLICATION( user_data ));

	window = gtk_application_get_active_window( GTK_APPLICATION( user_data ));
	g_return_if_fail( window && NACT_IS_MAIN_WINDOW( window ));

	nact_main_window_quit( NACT_MAIN_WINDOW( window ));
}

/**
 * nact_menu_win:
 * @main_window: the #NactMainWindow.
 *
 * Define the window menubar and attach it to the main window.
 */
void
nact_menu_win( NactMainWindow *main_window )
{
	static const gchar *thisfn = "nact_menu_win";
	gint i;
	GtkApplication *application;
	NactTreeView *treeview;
	sMenuData *sdata;

	sdata = nact_menu_get_data( main_window );

	for( i=0 ; i<G_N_ELEMENTS( st_menubar_entries ) ; ++i ){
		g_action_map_add_action_entries(
				G_ACTION_MAP( main_window ),
				&st_menubar_entries[i].action_entry, 1, main_window );
	}

	/* initialize the private data
	 */
	application = gtk_window_get_application( GTK_WINDOW( main_window ));
	g_return_if_fail( application && NACT_IS_APPLICATION( application ));
	sdata->updater = nact_application_get_updater( NACT_APPLICATION( application ));
	sdata->is_level_zero_writable = na_updater_is_level_zero_writable( sdata->updater );
	sdata->has_writable_providers =
			( na_io_provider_find_writable_io_provider( NA_PIVOT( sdata->updater )) != NULL );

	g_debug( "%s: na_updater_is_level_zero_writable=%s, na_io_provider_find_writable_io_provider=%s",
			thisfn,
			sdata->is_level_zero_writable ? "True":"False",
			sdata->has_writable_providers ? "True":"False" );

	/* connect to all signal which may have an influence on the menu
	 * items sensitivity
	 */
	treeview = nact_main_window_get_items_view( main_window );
	g_signal_connect(
			treeview, TREE_SIGNAL_CONTEXT_MENU,
			G_CALLBACK( on_open_context_menu ), main_window );

	g_signal_connect(
			treeview, TREE_SIGNAL_COUNT_CHANGED,
			G_CALLBACK( on_tree_view_count_changed ), main_window );

	g_signal_connect(
			treeview, TREE_SIGNAL_FOCUS_IN,
			G_CALLBACK( on_tree_view_focus_in ), main_window );

	g_signal_connect(
			treeview, TREE_SIGNAL_FOCUS_OUT,
			G_CALLBACK( on_tree_view_focus_out ), main_window );

	g_signal_connect(
			treeview, TREE_SIGNAL_MODIFIED_STATUS_CHANGED,
			G_CALLBACK( on_tree_view_modified_status_changed ), main_window );

	g_signal_connect(
			treeview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_view_selection_changed ), main_window );

	sdata->update_sensitivities_handler_id =
			g_signal_connect(
					main_window, MAIN_SIGNAL_UPDATE_SENSITIVITIES,
					G_CALLBACK( on_update_sensitivities ), NULL );

	/* install autosave */
	nact_menu_file_init( main_window );

	/* install toolbar submenu */
	nact_menu_view_init( main_window );
}

/**
 * nact_menu_get_data:
 * @main_window: the #NactMainWindow main window.
 */
sMenuData *
nact_menu_get_data( NactMainWindow *main_window )
{
	sMenuData *sdata;

	sdata = g_object_get_data( G_OBJECT( main_window ), "menu-data" );
	if( !sdata ){
		sdata = g_new0( sMenuData, 1 );
		g_object_set_data_full( G_OBJECT( main_window ), "menu-data", sdata, ( GDestroyNotify ) free_menu_data );
	}

	return( sdata );
}

static void
free_menu_data( sMenuData *sdata )
{
	g_free( sdata );
}

/*
 * Opens a popup menu.
 */
static void
on_open_context_menu( NactTreeView *treeview, GdkEventButton *event, NactMainWindow *window )
{
	sMenuData *sdata;

	sdata = nact_menu_get_data( window );

	sdata->popup_handler = g_signal_connect(
			sdata->popup, "selection-done", G_CALLBACK( on_popup_selection_done ), window );

	if( event ){
		gtk_menu_popup(
				GTK_MENU( sdata->popup ), NULL, NULL, NULL, NULL, event->button, event->time );
	} else {
		gtk_menu_popup(
				GTK_MENU( sdata->popup ), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}
}

static void
on_popup_selection_done( GtkMenuShell *menushell, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_menu_on_popup_selection_done";
	sMenuData *sdata;

	g_debug( "%s", thisfn );

	sdata = nact_menu_get_data( window );
	g_signal_handler_disconnect( menushell, sdata->popup_handler );
	sdata->popup_handler = ( gulong ) 0;
}

/*
 * when the tree view is refilled, update our internal counters so
 * that we are knowing if we have some exportables
 */
static void
on_tree_view_count_changed( NactTreeView *treeview, gboolean reset, gint menus, gint actions, gint profiles, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_menu_on_tree_view_count_changed";
	sMenuData *sdata;
	NactStatusbar *bar;
	gchar *status;

	g_debug( "%s: treeview=%p, reset=%s, menus=%d, actions=%d, profiles=%d, window=%p",
			thisfn, ( void * ) treeview,
			reset ? "True":"False", menus, actions, profiles, ( void * ) window );

	sdata = nact_menu_get_data( window );

	if( reset ){
		sdata->count_menus = menus;
		sdata->count_actions = actions;
		sdata->count_profiles = profiles;

	} else {
		sdata->count_menus += menus;
		sdata->count_actions += actions;
		sdata->count_profiles += profiles;
	}

	sdata->have_exportables = ( sdata->count_menus + sdata->count_actions > 0 );

	/* i18n: note the space at the beginning of the sentence */
	status = g_strdup_printf(
			_( " %d menu(s), %d action(s), %d profile(s) are currently loaded" ),
			sdata->count_menus, sdata->count_actions, sdata->count_profiles );

	bar = nact_main_window_get_statusbar( window );
	nact_statusbar_display_status( bar, "menu-status-context", status );
	g_free( status );

	g_signal_emit_by_name( window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

static void
on_tree_view_focus_in( NactTreeView *treeview, NactMainWindow *window )
{
	sMenuData *sdata;

	sdata = nact_menu_get_data( window );

	sdata->treeview_has_focus = TRUE;
	g_signal_emit_by_name( window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

static void
on_tree_view_focus_out( NactTreeView *treeview, NactMainWindow *window )
{
	sMenuData *sdata;

	sdata = nact_menu_get_data( window );

	sdata->treeview_has_focus = FALSE;
	g_signal_emit_by_name( window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

/*
 * the count of modified NAObjectItem has changed
 */
static void
on_tree_view_modified_status_changed( NactTreeView *treeview, gboolean is_modified, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_menu_on_tree_view_modified_status_changed";
	sMenuData *sdata;

	g_debug( "%s: treeview=%p, is_modified=%s, window=%p",
			thisfn, ( void * ) treeview, is_modified ? "True":"False", ( void * ) window );

	sdata = nact_menu_get_data( window );

	sdata->is_tree_modified = is_modified;
	g_signal_emit_by_name( window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

/*
 * when the selection changes in the tree view, see what is selected
 *
 * It happens that this function is triggered after all tabs have already
 * dealt with the TREE_SIGNAL_SELECTION_CHANGED signal
 *
 * We are trying to precompute here all indicators which are needed to
 * make actions sensitive. As a multiple selection may have multiple
 * sort of indicators, we assure here that at least one item will be a
 * valid candidate to the target action, the action taking care itself
 * of applying to valid candidates, and rejecting the others.
 */
static void
on_tree_view_selection_changed( NactTreeView *treeview, GList *selected, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_menu_on_tree_view_selection_changed";
	sMenuData *sdata;
	NAObject *first;
	NAObject *selected_action;
	NAObject *row, *parent;
	GList *is;

	g_debug( "%s: treeview=%p, selected_items=%p (count=%d), window=%p",
			thisfn, ( void * ) treeview,
			( void * ) selected, g_list_length( selected ), ( void * ) window );

	sdata = nact_menu_get_data( window );

	/* count the items
	 */
	sdata->count_selected = g_list_length( selected );

	if( selected ){
		na_object_item_count_items( selected, &sdata->selected_menus, &sdata->selected_actions, &sdata->selected_profiles, FALSE );
		g_debug( "%s: selected_menus=%d, selected_actions=%d, selected_profiles=%d",
				thisfn,
				sdata->selected_menus, sdata->selected_actions, sdata->selected_profiles );
	}

	/* take a ref of the list of selected items
	 */
	if( sdata->selected_items ){
		g_list_free( sdata->selected_items );
	}
	sdata->selected_items = g_list_copy( selected );

	/* check if the parent of the first selected item is writable
	 * (File: New menu/New action)
	 * (Edit: Paste menu or action)
	 */
	first = NULL;
	if( selected ){
		first = ( NAObject *) selected->data;
		if( NA_IS_OBJECT_PROFILE( first )){
			first = NA_OBJECT( na_object_get_parent( first ));
		}
		first = ( NAObject * ) na_object_get_parent( first );
	}
	if( first ){
		sdata->is_parent_writable = na_object_is_finally_writable( first, NULL );
		g_debug( "%s: parent of first selected is not null: is_parent_writable=%s",
				thisfn, sdata->is_parent_writable ? "True":"False" );
	} else {
		sdata->is_parent_writable = sdata->is_level_zero_writable;
		g_debug( "%s: first selected is at level zero: is_level_zero_writable=%s",
				thisfn, sdata->is_level_zero_writable ? "True":"False" );
	}

	/* check is only an action is selected, or only profile(s) of a same action
	 * (File: New profile)
	 * (Edit: Paste a profile)
	 */
	sdata->enable_new_profile = TRUE;
	selected_action = NULL;
	for( is = selected ; is ; is = is->next ){

		if( NA_IS_OBJECT_MENU( is->data )){
			sdata->enable_new_profile = FALSE;
			break;

		} else if( NA_IS_OBJECT_ACTION( is->data )){
			if( !selected_action ){
				selected_action = NA_OBJECT( is->data );
			} else {
				sdata->enable_new_profile = FALSE;
				break;
			}

		} else if( NA_IS_OBJECT_PROFILE( is->data )){
			first = NA_OBJECT( na_object_get_parent( is->data ));
			if( !selected_action ){
				selected_action = first;
			} else if( selected_action != first ){
				sdata->enable_new_profile = FALSE;
				break;
			}
		}
	}
	if( selected_action ){
		sdata->is_action_writable = na_object_is_finally_writable( selected_action, NULL );
	} else {
		sdata->enable_new_profile = FALSE;
	}

	/* check that selection is not empty and that each selected item is writable
	 * and that all parents are writable
	 * if some selection is at level zero, then it must be writable
	 * (Edit: Cut/Delete)
	 */
	if( selected ){
		sdata->are_parents_writable = TRUE;
		sdata->are_items_writable = TRUE;
		for( is = selected ; is ; is = is->next ){
			row = ( NAObject * ) is->data;
			if( NA_IS_OBJECT_PROFILE( row )){
				row = NA_OBJECT( na_object_get_parent( row ));
			}
			gchar *label = na_object_get_label( row );
			gboolean writable = na_object_is_finally_writable( row, NULL );
			g_debug( "%s: label=%s, writable=%s", thisfn, label, writable ? "True":"False" );
			g_free( label );
			sdata->are_items_writable &= writable;
			parent = ( NAObject * ) na_object_get_parent( row );
			if( parent ){
				sdata->are_parents_writable &= na_object_is_finally_writable( parent, NULL );
			} else {
				sdata->are_parents_writable &= sdata->is_level_zero_writable;
			}
		}
	}

	g_signal_emit_by_name( window, MAIN_SIGNAL_UPDATE_SENSITIVITIES );
}

static void
on_update_sensitivities( NactMainWindow *window, void *empty )
{
	static const gchar *thisfn = "nact_menu_on_update_sensitivities";

	g_debug( "%s: window=%p, empty=%p", thisfn, ( void * ) window, empty );

	if( !nact_main_window_dispose_has_run( window )){

		nact_menu_file_update_sensitivities( window );
		nact_menu_edit_update_sensitivities( window );
		nact_menu_view_update_sensitivities( window );
		/*nact_menu_tools_update_sensitivities( window );
		nact_menu_maintainer_update_sensitivities( window );
		nact_menu_help_update_sensitivities( window );*/
	}
}

static void
on_win_brief_tree_store_dump( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_collapse_all( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	NactTreeView *items_view;

	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( user_data ));
	nact_tree_view_collapse_all( items_view );
}

static void
on_win_copy( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_copy( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_cut( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_cut( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_delete( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_delete( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_dump_clipboard( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_dump_selection( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_duplicate( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_duplicate( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_expand_all( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	NactTreeView *items_view;

	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));

	items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( user_data ));
	nact_tree_view_expand_all( items_view );
}

static void
on_win_export( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_import( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_list_modified_items( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

static void
on_win_new_action( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_file_new_action( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_new_menu( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_file_new_menu( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_new_profile( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_file_new_profile( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_paste( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_paste( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_paste_into( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_edit_paste_into( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_reload( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_main_window_reload( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_save( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
	nact_menu_file_save_items( NACT_MAIN_WINDOW( user_data ));
}

static void
on_win_test_function( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));
}

/*
 * the menu item is activated
 * just toggle the state of the corresponding action
 */
static void
on_win_toolbar_activate( GSimpleAction *action, GVariant *parameter, gpointer user_data )
{
	GVariant *state;

	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));

	state = g_action_get_state( G_ACTION( action ));
	g_action_change_state( G_ACTION( action ),
			g_variant_new_boolean( !g_variant_get_boolean( state )));
	g_variant_unref( state );
}

/*
 * the state of the action has been toggled, either directly or by
 * activating the menu item
 */
static void
on_win_toolbar_changed_state( GSimpleAction *action, GVariant *state, gpointer user_data )
{
	g_return_if_fail( user_data && NACT_IS_MAIN_WINDOW( user_data ));

	nact_menu_view_toolbar_display(
			NACT_MAIN_WINDOW( user_data ),
			g_action_get_name( G_ACTION( action )),
			g_variant_get_boolean( state ));

	g_simple_action_set_state( action, state );
}

/**
 * nact_menu_enable_item:
 * @main_window: this #NactMainWindow main window.
 * @name: the name of the item in a menu.
 * @enabled: whether this item should be enabled or not.
 *
 * Enable/disable an item in an menu.
 */
void
nact_menu_enable_item( NactMainWindow *main_window, const gchar *name, gboolean enabled )
{
	GAction *action;

	action = g_action_map_lookup_action( G_ACTION_MAP( main_window ), name );
	g_simple_action_set_enabled( G_SIMPLE_ACTION( action ), enabled );
}
