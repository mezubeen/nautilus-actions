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
#include <stdlib.h>

#include "api/na-object-api.h"
#include "api/na-timeout.h"

#include "core/na-about.h"
#include "core/na-gtk-utils.h"
#include "core/na-iprefs.h"
#include "core/na-pivot.h"

#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-ibasenames-tab.h"
#include "nact-imimetypes-tab.h"
#include "nact-ifolders-tab.h"
#include "nact-ischemes-tab.h"
#include "nact-icapabilities-tab.h"
#include "nact-ienvironment-tab.h"
#include "nact-iexecution-tab.h"
#include "nact-iproperties-tab.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"
#include "nact-menu.h"
#include "nact-statusbar.h"
#include "nact-tree-view.h"
#include "nact-confirm-logout.h"
#include "nact-sort-buttons.h"

/* private instance data
 */
struct _NactMainWindowPrivate {
	gboolean         dispose_has_run;

	NAUpdater       *updater;

	/**
	 * Current action or menu.
	 *
	 * This is the action or menu which is displayed in tabs Action/Menu
	 * and Properties ; it may be different of the exact row being currently
	 * selected, e.g. when a sub-profile is edited.
	 *
	 * Can be null, and this implies that @current_profile is also null,
	 * e.g. when the list is empty or in the case of a multiple selection.
	 *
	 * 'editable' property is set on selection change;
	 * This is the actual current writability status of the item at this time.
	 */
	NAObjectItem    *current_item;
	gboolean         editable;
	guint            reason;

	/**
	 * Current profile.
	 *
	 * This is the profile which is displayed in tab Command;
	 * it may be different of the exact row being currently selected,
	 * e.g. when an action with only one profile is selected.
	 *
	 * Can be null if @current_item is a menu, or an action with more
	 * than one profile is selected, or the list is empty, or in the
	 * case of a multiple selection.
	 *
	 * In other words, it is not null if:
	 * a) a profile is selected,
	 * b) an action is selected and it has exactly one profile.
	 */
	NAObjectProfile *current_profile;

	/**
	 * Current context.
	 *
	 * This is the #NAIContext data which corresponds to @current_profile
	 * or @current_item, depending of which one is actually selected.
	 */
	NAIContext      *current_context;

	/**
	 * Some convenience objects and data.
	 */
	NactTreeView    *items_view;
	gboolean         is_tree_modified;
	NactClipboard   *clipboard;
	NactStatusbar   *statusbar;
	NactSortButtons *sort_buttons;

	gulong           pivot_handler_id;
	NATimeout        pivot_timeout;
};

/* properties set against the main window
 * these are set on selection changes
 */
enum {
	MAIN_PROP_0 = 0,

	MAIN_PROP_ITEM_ID,
	MAIN_PROP_PROFILE_ID,
	MAIN_PROP_CONTEXT_ID,
	MAIN_PROP_EDITABLE_ID,
	MAIN_PROP_REASON_ID,

	MAIN_PROP_N_PROPERTIES
};

/* signals
 */
enum {
	ITEM_UPDATED,
	UPDATE_SENSITIVITIES,
	LAST_SIGNAL
};

static const gchar     *st_xmlui_filename         = PKGUIDIR "/nact-main-window.ui";
static const gchar     *st_toplevel_name          = "MainWindow";
static const gchar     *st_wsp_name               = NA_IPREFS_MAIN_WINDOW_WSP;

static gint             st_burst_timeout          = 2500;		/* burst timeout in msec */
static BaseWindowClass *st_parent_class           = NULL;
static guint            st_signals[ LAST_SIGNAL ] = { 0 };

static GType      register_type( void );
static void       class_init( NactMainWindowClass *klass );
static void       iaction_tab_iface_init( NactIActionTabInterface *iface, void *user_data );
static void       icommand_tab_iface_init( NactICommandTabInterface *iface, void *user_data );
static void       ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface, void *user_data );
static void       imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface, void *user_data );
static void       ifolders_tab_iface_init( NactIFoldersTabInterface *iface, void *user_data );
static void       ischemes_tab_iface_init( NactISchemesTabInterface *iface, void *user_data );
static void       icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface, void *user_data );
static void       ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface, void *user_data );
static void       iexecution_tab_iface_init( NactIExecutionTabInterface *iface, void *user_data );
static void       iproperties_tab_iface_init( NactIPropertiesTabInterface *iface, void *user_data );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void       instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void       instance_dispose( GObject *window );
static void       instance_finalize( GObject *window );
static void       setup_main_ui( NactMainWindow *main_window );
static void       setup_treeview( NactMainWindow *main_window );
static void       setup_monitor_pivot( NactMainWindow *main_window );
static void       on_block_items_changed_timeout( NactMainWindow *window );
static void       on_tree_view_modified_status_changed( NactTreeView *treeview, gboolean is_modified, NactMainWindow *window );
static void       on_tree_view_selection_changed( NactTreeView *treeview, GList *selected_items, NactMainWindow *window );
static void       on_tab_item_updated( NactMainWindow *window, NAIContext *context, guint data, void *empty );
static void       raz_selection_properties( NactMainWindow *window );
static void       setup_current_selection( NactMainWindow *window, NAObjectId *selected_row );
static void       setup_dialog_title( NactMainWindow *window );
static void       setup_writability_status( NactMainWindow *window );

/* items have changed */
static void       on_pivot_items_changed( NAUpdater *updater, NactMainWindow *window );
static gboolean   confirm_for_giveup_from_pivot( const NactMainWindow *window );
static gboolean   confirm_for_giveup_from_menu( const NactMainWindow *window );
static void       load_or_reload_items( NactMainWindow *window );

/* application termination */
static gboolean   on_delete_event( GtkWidget *toplevel, GdkEvent *event, void *empty );
static gboolean   warn_modified( NactMainWindow *window );

GType
nact_main_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_main_window_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactMainWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactMainWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iaction_tab_iface_info = {
		( GInterfaceInitFunc ) iaction_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icommand_tab_iface_info = {
		( GInterfaceInitFunc ) icommand_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ibasenames_tab_iface_info = {
		( GInterfaceInitFunc ) ibasenames_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo imimetypes_tab_iface_info = {
		( GInterfaceInitFunc ) imimetypes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ifolders_tab_iface_info = {
		( GInterfaceInitFunc ) ifolders_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ischemes_tab_iface_info = {
		( GInterfaceInitFunc ) ischemes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icapabilities_tab_iface_info = {
		( GInterfaceInitFunc ) icapabilities_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ienvironment_tab_iface_info = {
		( GInterfaceInitFunc ) ienvironment_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iexecution_tab_iface_info = {
		( GInterfaceInitFunc ) iexecution_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iproperties_tab_iface_info = {
		( GInterfaceInitFunc ) iproperties_tab_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_APPLICATION_WINDOW, "NactMainWindow", &info, 0 );

	g_type_add_interface_static( type, NACT_TYPE_IACTION_TAB, &iaction_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_ICOMMAND_TAB, &icommand_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IBASENAMES_TAB, &ibasenames_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IMIMETYPES_TAB, &imimetypes_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IFOLDERS_TAB, &ifolders_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_ISCHEMES_TAB, &ischemes_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_ICAPABILITIES_TAB, &icapabilities_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IENVIRONMENT_TAB, &ienvironment_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IEXECUTION_TAB, &iexecution_tab_iface_info );

	g_type_add_interface_static( type, NACT_TYPE_IPROPERTIES_TAB, &iproperties_tab_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	g_object_class_install_property( object_class, MAIN_PROP_ITEM_ID,
			g_param_spec_pointer(
					MAIN_PROP_ITEM,
					_( "Current NAObjectItem" ),
					_( "A pointer to the currently edited NAObjectItem, an action or a menu" ),
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_PROFILE_ID,
			g_param_spec_pointer(
					MAIN_PROP_PROFILE,
					_( "Current NAObjectProfile" ),
					_( "A pointer to the currently edited NAObjectProfile" ),
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_CONTEXT_ID,
			g_param_spec_pointer(
					MAIN_PROP_CONTEXT,
					_( "Current NAIContext" ),
					_( "A pointer to the currently edited NAIContext" ),
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_EDITABLE_ID,
			g_param_spec_boolean(
					MAIN_PROP_EDITABLE,
					_( "Editable item ?" ),
					_( "Whether the item will be able to be updated against its I/O provider" ),
					FALSE,
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, MAIN_PROP_REASON_ID,
			g_param_spec_int(
					MAIN_PROP_REASON,
					_( "No edition reason" ),
					_( "Why is this item not editable" ),
					0, 255, 0,
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	/**
	 * NactMainWindow::main-item-updated:
	 *
	 * This signal is emitted on the main window when an item is
	 * modified from the user interface
	 *
	 * Args:
	 * - an OR-ed list of the modified data, or 0 if not relevant.
	 *
	 * Handler:
	 *   void handler( NactMainWindow *main_window,
	 *   				NAIContext *updated_context,
	 *   				guint       updated_data,
	 *   				void       *user_data );
	 */
	st_signals[ ITEM_UPDATED ] = g_signal_new(
			MAIN_SIGNAL_ITEM_UPDATED,
			NACT_TYPE_MAIN_WINDOW,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_UINT );

	/**
	 * NactMainWindow::main-signal-update-sensitivities
	 *
	 * This signal is emitted on the NactMainWindow when any menu item
	 *  sensitivity has to be refreshed.
	 *
	 * Signal arg.: None
	 *
	 * Handler prototype:
	 * void handler( NactMainWindow *main_window, gpointer user_data );
	 */
	st_signals[ UPDATE_SENSITIVITIES ] = g_signal_new(
			MAIN_SIGNAL_UPDATE_SENSITIVITIES,
			NACT_TYPE_MAIN_WINDOW,
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,					/* no default handler */
			NULL,
			NULL,
			NULL,
			G_TYPE_NONE,
			0 );
}

static void
iaction_tab_iface_init( NactIActionTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_iaction_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_ibasenames_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_imimetypes_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
ifolders_tab_iface_init( NactIFoldersTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_ifolders_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
ischemes_tab_iface_init( NactISchemesTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_ischemes_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_icapabilities_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_ienvironment_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
iexecution_tab_iface_init( NactIExecutionTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_iexecution_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
iproperties_tab_iface_init( NactIPropertiesTabInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_main_window_iproperties_tab_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	NactMainWindow *self;
	NactMainWindowPrivate *priv;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_MAIN_WINDOW( instance );
	self->private = g_new0( NactMainWindowPrivate, 1 );
	priv = self->private;
	priv->dispose_has_run = FALSE;

	/* initialize timeout parameters when blocking 'pivot-items-changed' handler
	 */
	priv->pivot_timeout.timeout = st_burst_timeout;
	priv->pivot_timeout.handler = ( NATimeoutFunc ) on_block_items_changed_timeout;
	priv->pivot_timeout.user_data = self;
	priv->pivot_timeout.source_id = 0;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case MAIN_PROP_ITEM_ID:
				g_value_set_pointer( value, self->private->current_item );
				break;

			case MAIN_PROP_PROFILE_ID:
				g_value_set_pointer( value, self->private->current_profile );
				break;

			case MAIN_PROP_CONTEXT_ID:
				g_value_set_pointer( value, self->private->current_context );
				break;

			case MAIN_PROP_EDITABLE_ID:
				g_value_set_boolean( value, self->private->editable );
				break;

			case MAIN_PROP_REASON_ID:
				g_value_set_int( value, self->private->reason );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case MAIN_PROP_ITEM_ID:
				self->private->current_item = g_value_get_pointer( value );
				break;

			case MAIN_PROP_PROFILE_ID:
				self->private->current_profile = g_value_get_pointer( value );
				break;

			case MAIN_PROP_CONTEXT_ID:
				self->private->current_context = g_value_get_pointer( value );
				break;

			case MAIN_PROP_EDITABLE_ID:
				self->private->editable = g_value_get_boolean( value );
				break;

			case MAIN_PROP_REASON_ID:
				self->private->reason = g_value_get_int( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	NactMainWindow *self;
	GtkWidget *pane;
	gint pos;
	GtkNotebook *notebook;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );

		pane = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( window ), "main-paned" );
		pos = gtk_paned_get_position( GTK_PANED( pane ));
		na_settings_set_uint( NA_IPREFS_MAIN_PANED, pos );

		notebook = GTK_NOTEBOOK( na_gtk_utils_find_widget_by_name( GTK_CONTAINER( window ), "main-notebook" ));
		pos = gtk_notebook_get_tab_pos( notebook );
		na_iprefs_set_tabs_pos( pos );

		na_gtk_utils_save_window_position( GTK_WINDOW( window ), st_wsp_name );
	}

	/* chain up to the parent class */
	G_OBJECT_CLASS( st_parent_class )->dispose( window );
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_finalize";
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_MAIN_WINDOW( window );

	g_free( self->private );

	/* chain up to the parent class */
	G_OBJECT_CLASS( st_parent_class )->finalize( window );
}

/**
 * Returns a newly allocated NactMainWindow object.
 */
NactMainWindow *
nact_main_window_new( NactApplication *application )
{
	NactMainWindow *window;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	window = g_object_new( NACT_TYPE_MAIN_WINDOW,
			"application", application,		/* GtkWindow property */
			NULL );

	setup_main_ui( window );
	setup_treeview( window );
	nact_menu_win( window );

	window->private->clipboard = nact_clipboard_new( window );
	window->private->sort_buttons = nact_sort_buttons_new( window );

	/* initialize the notebook interfaces
	 *  and monitor the updates which originates from each property tab
	 */
	nact_iaction_tab_init( NACT_IACTION_TAB( window ));
	nact_icommand_tab_init( NACT_ICOMMAND_TAB( window ));
	nact_ibasenames_tab_init( NACT_IBASENAMES_TAB( window ));
	nact_imimetypes_tab_init( NACT_IMIMETYPES_TAB( window ));
	nact_ifolders_tab_init( NACT_IFOLDERS_TAB( window ));
	nact_ischemes_tab_init( NACT_ISCHEMES_TAB( window ));
	nact_icapabilities_tab_init( NACT_ICAPABILITIES_TAB( window ));
	nact_ienvironment_tab_init( NACT_IENVIRONMENT_TAB( window ));
	nact_iexecution_tab_init( NACT_IEXECUTION_TAB( window ));
	nact_iproperties_tab_init( NACT_IPROPERTIES_TAB( window ));

	g_signal_connect( window, MAIN_SIGNAL_ITEM_UPDATED, G_CALLBACK( on_tab_item_updated ), NULL );

	na_gtk_utils_restore_window_position( GTK_WINDOW( window ), st_wsp_name );
	g_signal_connect( window, "delete-event", G_CALLBACK( on_delete_event ), NULL );

	setup_monitor_pivot( window );
	load_or_reload_items( window );
	gtk_widget_show_all( GTK_WIDGET( window ));

	return( window );
}

/*
 * Load and initialize the user interface
 */
static void
setup_main_ui( NactMainWindow *main_window )
{
	GtkBuilder *builder;
	GObject *top_window;
	GtkWidget *top_widget, *alignment;
	GtkWidget *notebook;
	guint pos;
	GtkWidget *pane;
	NactStatusbar *bar;

	/* no error condition here:
	 * If there is an error opening the file or parsing the description
	 * then the program will be aborted. */
	builder = gtk_builder_new_from_file( st_xmlui_filename );
	top_window = gtk_builder_get_object( builder, st_toplevel_name );
	g_return_if_fail( top_window && GTK_IS_WINDOW( top_window ));

	top_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( top_window ), "top" );
	g_return_if_fail( top_widget && GTK_IS_CONTAINER( top_widget ));

	/* reparent
	 */
	g_object_ref( top_widget );
	gtk_container_remove( GTK_CONTAINER( top_window ), top_widget );
	gtk_container_add( GTK_CONTAINER( main_window ), top_widget );
	g_object_unref( top_widget );
	gtk_widget_destroy( GTK_WIDGET( top_window ));
	g_object_unref( builder );

	/* restore the notebook tabs position, and enable its popup menu
	 */
	notebook = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( top_widget ), "main-notebook" );
	g_return_if_fail( notebook && GTK_IS_NOTEBOOK( notebook ));
	pos = na_iprefs_get_tabs_pos( NULL );
	gtk_notebook_set_tab_pos( GTK_NOTEBOOK( notebook ), pos );
	gtk_notebook_popup_enable( GTK_NOTEBOOK( notebook ));

	/* restore pane position
	 */
	pos = na_settings_get_uint( NA_IPREFS_MAIN_PANED, NULL, NULL );
	if( pos ){
		pane = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( top_widget ), "main-paned" );
		g_return_if_fail( pane && GTK_IS_PANED( pane ));
		gtk_paned_set_position( GTK_PANED( pane ), pos );
	}

	/* setup statusbar
	 */
	bar = nact_statusbar_new();
	alignment = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( top_widget ), "main-statusbar" );
	g_return_if_fail( alignment && GTK_IS_CONTAINER( alignment ));
	gtk_container_add( GTK_CONTAINER( alignment ), GTK_WIDGET( bar ));
	main_window->private->statusbar = bar;

	/* application icon
	 */
	gtk_window_set_default_icon_from_file( na_about_get_icon_name(), NULL );
}

/*
 * setup the ActionsList treeview, and connect to management signals
 */
static void
setup_treeview( NactMainWindow *main_window )
{
	NactMainWindowPrivate *priv;
	GtkWidget *top_widget;

	priv = main_window->private;
	priv->items_view = nact_tree_view_new( main_window );

	top_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( main_window ), "ActionsList" );
	g_return_if_fail( top_widget && GTK_IS_CONTAINER( top_widget ));
	gtk_container_add( GTK_CONTAINER( top_widget ), GTK_WIDGET( priv->items_view ));

	nact_tree_view_set_mnemonic( priv->items_view, GTK_CONTAINER( main_window ), "ActionsListLabel" );
	nact_tree_view_set_edition_mode( priv->items_view, TREE_MODE_EDITION );

	g_signal_connect(
			priv->items_view,
			TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_view_selection_changed ), main_window );

	g_signal_connect(
			priv->items_view,
			TREE_SIGNAL_MODIFIED_STATUS_CHANGED,
			G_CALLBACK( on_tree_view_modified_status_changed ), main_window );
}

/*
 * monitor the items stored on the disk for modifications
 *  from outside of this application
 */
static void
setup_monitor_pivot( NactMainWindow *main_window )
{
	NactMainWindowPrivate *priv;
	GtkApplication *application;

	priv = main_window->private;

	application = gtk_window_get_application( GTK_WINDOW( main_window ));
	g_return_if_fail( application && NACT_IS_APPLICATION( application ));

	priv->updater = nact_application_get_updater( NACT_APPLICATION ( application ));

	priv->pivot_handler_id = g_signal_connect( priv->updater,
			PIVOT_SIGNAL_ITEMS_CHANGED, G_CALLBACK( on_pivot_items_changed ), main_window );
}

/**
 * nact_main_window_get_clipboard:
 * @window: this #NactMainWindow instance.
 *
 * Returns: the #NactClipboard convenience object.
 */
NactClipboard *
nact_main_window_get_clipboard( const NactMainWindow *window )
{
	NactClipboard *clipboard;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), NULL );

	clipboard = NULL;

	if( !window->private->dispose_has_run ){

		clipboard = window->private->clipboard;
	}

	return( clipboard );
}

/**
 * nact_main_window_get_sort_buttons:
 * @window: this #NactMainWindow instance.
 *
 * Returns: the #NactSortButtons object.
 */
NactSortButtons *
nact_main_window_get_sort_buttons( const NactMainWindow *window )
{
	NactSortButtons *buttons;

	g_return_val_if_fail( window && NACT_IS_MAIN_WINDOW( window ), NULL );

	buttons = NULL;

	if( !window->private->dispose_has_run ){

		buttons = window->private->sort_buttons;
	}

	return( buttons );
}

/**
 * nact_main_window_get_statusbar:
 * @window: this #NactMainWindow instance.
 *
 * Returns: the #NactStatusbar object.
 */
NactStatusbar *
nact_main_window_get_statusbar( const NactMainWindow *window )
{
	NactStatusbar *bar;

	g_return_val_if_fail( window && NACT_IS_MAIN_WINDOW( window ), NULL );

	bar = NULL;

	if( !window->private->dispose_has_run ){

		bar = window->private->statusbar;
	}

	return( bar );
}

/**
 * nact_main_window_get_items_view:
 * @window: this #NactMainWindow instance.
 *
 * Returns: The #NactTreeView convenience object.
 */
NactTreeView *
nact_main_window_get_items_view( const NactMainWindow *window )
{
	NactTreeView *view;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), NULL );

	view = NULL;

	if( !window->private->dispose_has_run ){

		view = window->private->items_view;
	}

	return( view );
}

/**
 * nact_main_window_reload:
 * @window: this #NactMainWindow instance.
 *
 * Refresh the list of items.
 * If there is some non-yet saved modifications, a confirmation is
 * required before giving up with them.
 */
void
nact_main_window_reload( NactMainWindow *window )
{
	gboolean reload_ok;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		reload_ok = confirm_for_giveup_from_menu( window );

		if( reload_ok ){
			load_or_reload_items( window );
		}
	}
}

/**
 * nact_main_window_block_reload:
 * @window: this #NactMainWindow instance.
 *
 * Temporarily blocks the handling of pivot-items-changed signal.
 */
void
nact_main_window_block_reload( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_block_reload";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		g_debug( "%s: blocking %s signal", thisfn, PIVOT_SIGNAL_ITEMS_CHANGED );
		g_signal_handler_block( window->private->updater, window->private->pivot_handler_id );
		na_timeout_event( &window->private->pivot_timeout );
	}
}

static void
on_block_items_changed_timeout( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_block_items_changed_timeout";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: unblocking %s signal", thisfn, PIVOT_SIGNAL_ITEMS_CHANGED );
	g_signal_handler_unblock( window->private->updater, window->private->pivot_handler_id );
}

/*
 * the modification status of the items view has changed
 */
static void
on_tree_view_modified_status_changed( NactTreeView *treeview, gboolean is_modified, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_tree_view_modified_status_changed";

	g_debug( "%s: treeview=%p, is_modified=%s, window=%p",
			thisfn, ( void * ) treeview, is_modified ? "True":"False", ( void * ) window );

	if( !window->private->dispose_has_run ){

		window->private->is_tree_modified = is_modified;
		setup_dialog_title( window );
	}
}

/*
 * tree view selection has changed
 */
static void
on_tree_view_selection_changed( NactTreeView *treeview, GList *selected_items, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_tree_view_selection_changed";
	guint count;

	count = g_list_length( selected_items );

	if( !window->private->dispose_has_run ){
		g_debug( "%s: treeview=%p, selected_items=%p (count=%d), window=%p",
				thisfn, ( void * ) treeview,
				( void * ) selected_items, count, ( void * ) window );

		raz_selection_properties( window );

		if( count == 1 ){
			g_return_if_fail( NA_IS_OBJECT_ID( selected_items->data ));
			setup_current_selection( window, NA_OBJECT_ID( selected_items->data ));
			setup_writability_status( window );
		}

		setup_dialog_title( window );
	}
}

static void
on_tab_item_updated( NactMainWindow *window, NAIContext *context, guint data, void *empty )
{
	static const gchar *thisfn = "nact_main_window_on_tab_item_updated";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, context=%p (%s), data=%u, empty=%p",
				thisfn, ( void * ) window, ( void * ) context, G_OBJECT_TYPE_NAME( context ),
				data, ( void * ) empty );

		if( context ){
			na_object_check_status( context );
		}
	}
}

static void
raz_selection_properties( NactMainWindow *window )
{
	window->private->current_item = NULL;
	window->private->current_profile = NULL;
	window->private->current_context = NULL;
	window->private->editable = FALSE;
	window->private->reason = 0;

	nact_statusbar_set_locked( window->private->statusbar, FALSE, 0 );
}

/*
 * enter after raz_properties
 * only called when only one selected row
 */
static void
setup_current_selection( NactMainWindow *window, NAObjectId *selected_row )
{
	guint nb_profiles;
	GList *profiles;

	if( NA_IS_OBJECT_PROFILE( selected_row )){
		window->private->current_profile = NA_OBJECT_PROFILE( selected_row );
		window->private->current_context = NA_ICONTEXT( selected_row );
		window->private->current_item = NA_OBJECT_ITEM( na_object_get_parent( selected_row ));

	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( selected_row ));
		window->private->current_item = NA_OBJECT_ITEM( selected_row );
		window->private->current_context = NA_ICONTEXT( selected_row );

		if( NA_IS_OBJECT_ACTION( selected_row )){
			nb_profiles = na_object_get_items_count( selected_row );

			if( nb_profiles == 1 ){
				profiles = na_object_get_items( selected_row );
				window->private->current_profile = NA_OBJECT_PROFILE( profiles->data );
				window->private->current_context = NA_ICONTEXT( profiles->data );
			}
		}
	}
}

/*
 * the title bar of the main window brings up three informations:
 * - the name of the application
 * - the name of the currently selected item if there is only one
 * - an asterisk if anything has been modified
 */
static void
setup_dialog_title( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_setup_dialog_title";
	NactMainWindowPrivate *priv;
	GtkApplication *application;
	gchar *title;
	gchar *label;
	gchar *tmp;
	gboolean is_modified;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	priv = window->private;
	application = gtk_window_get_application( GTK_WINDOW( window ));
	g_return_if_fail( application && NACT_IS_APPLICATION( application ));
	title = nact_application_get_application_name( NACT_APPLICATION( application ));

	if( priv->current_item ){
		label = na_object_get_label( priv->current_item );
		is_modified = na_object_is_modified( priv->current_item );
		tmp = g_strdup_printf( "%s%s - %s", is_modified ? "*" : "", label, title );
		g_free( label );
		g_free( title );
		title = tmp;
	}

	gtk_window_set_title( GTK_WINDOW( window ), title );
	g_free( title );
}

static void
setup_writability_status( NactMainWindow *window )
{
	g_return_if_fail( NA_IS_OBJECT_ITEM( window->private->current_item ));

	window->private->editable = na_object_is_finally_writable( window->private->current_item, &window->private->reason );
	nact_statusbar_set_locked( window->private->statusbar, !window->private->editable, window->private->reason );
}

/*
 * The handler of the signal sent by NAPivot when items have been modified
 * in the underlying storage subsystems
 */
static void
on_pivot_items_changed( NAUpdater *updater, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_pivot_items_changed";
	gboolean reload_ok;

	g_return_if_fail( NA_IS_UPDATER( updater ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: updater=%p (%s), window=%p (%s)", thisfn,
				( void * ) updater, G_OBJECT_TYPE_NAME( updater ),
				( void * ) window, G_OBJECT_TYPE_NAME( window ));

		reload_ok = confirm_for_giveup_from_pivot( window );

		if( reload_ok ){
			load_or_reload_items( window );
		}
	}
}

/*
 * informs the user that the actions in underlying storage subsystem
 * have changed, and propose for reloading
 *
 */
static gboolean
confirm_for_giveup_from_pivot( const NactMainWindow *window )
{
	gboolean reload_ok;
	gchar *first, *second;

	first = g_strdup(
				_( "One or more actions have been modified in the filesystem.\n"
					"You could keep to work with your current list of actions, "
					"or you may want to reload a fresh one." ));

	if( window->private->is_tree_modified){

		gchar *tmp = g_strdup_printf( "%s\n\n%s", first,
				_( "Note that reloading a fresh list of actions requires "
					"that you give up with your current modifications." ));
		g_free( first );
		first = tmp;
	}

	second = g_strdup( _( "Do you want to reload a fresh list of actions ?" ));

	reload_ok = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

	g_free( second );
	g_free( first );

	return( reload_ok );
}

/*
 * requires a confirmation from the user when is has asked for reloading
 * the actions via the Edit menu
 */
static gboolean
confirm_for_giveup_from_menu( const NactMainWindow *window )
{
	gboolean reload_ok = TRUE;
	gchar *first, *second;

	if( window->private->is_tree_modified ){

		first = g_strdup(
					_( "Reloading a fresh list of actions requires "
						"that you give up with your current modifications." ));

		second = g_strdup( _( "Do you really want to do this ?" ));

		reload_ok = base_window_display_yesno_dlg( BASE_WINDOW( window ), first, second );

		g_free( second );
		g_free( first );
	}

	return( reload_ok );
}

static void
load_or_reload_items( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_load_or_reload_items";
	GList *tree;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	raz_selection_properties( window );
	tree = na_updater_load_items( window->private->updater );
	nact_tree_view_fill( window->private->items_view, tree );

	g_debug( "%s: end of tree view filling", thisfn );
}

/**
 * nact_main_window_dispose_has_run:
 * @window: the #NactMainWindow main window.
 *
 * Returns: %TRUE if the main window is terminating.
 */
gboolean
nact_main_window_dispose_has_run( const NactMainWindow *window )
{
	g_return_val_if_fail( window && NACT_IS_MAIN_WINDOW( window ), TRUE );

	return( window->private->dispose_has_run );
}

/**
 * nact_main_window_quit:
 * @window: the #NactMainWindow main window.
 *
 * Quit the window, thus terminating the application.
 *
 * Returns: %TRUE if the application will terminate, and the @window object
 * is no more valid; %FALSE if the application will continue to run.
 */
gboolean
nact_main_window_quit( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_quit";
	gboolean terminated;

	g_return_val_if_fail( window && NACT_IS_MAIN_WINDOW( window ), FALSE );

	terminated = FALSE;

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		if( !window->private->is_tree_modified  || warn_modified( window )){
			gtk_widget_destroy( GTK_WIDGET( window ));
			terminated = TRUE;
		}
	}

	return( terminated );
}

#if 0
/*
 * signal handler
 * should return %FALSE if it is not willing to quit
 * this will also stop the emission of the signal (i.e. the first FALSE wins)
 */
static gboolean
on_base_quit_requested( NactApplication *application, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_on_base_quit_requested";
	gboolean willing_to;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), TRUE );

	willing_to = TRUE;

	if( !window->private->dispose_has_run ){

		g_debug( "%s: application=%p, window=%p", thisfn, ( void * ) application, ( void * ) window );

		if( window->private->is_tree_modified ){
			willing_to = nact_confirm_logout_run( window );
		}
	}

	return( willing_to );
}
#endif

/*
 * triggered when the user clicks on the top right [X] button
 * returns %TRUE to stop the signal to be propagated (which would cause the
 * window to be destroyed); instead we gracefully quit the application
 */
static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, void *empty )
{
	static const gchar *thisfn = "nact_main_window_on_delete_event";

	g_debug( "%s: toplevel=%p, event=%p, empty=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) empty );

	g_return_val_if_fail( toplevel && NACT_IS_MAIN_WINDOW( toplevel ), FALSE );

	nact_main_window_quit( NACT_MAIN_WINDOW( toplevel ));

	return( TRUE );
}

/*
 * warn_modified:
 * @window: this #NactWindow instance.
 *
 * Emits a warning if at least one item has been modified.
 *
 * Returns: %TRUE if the user confirms he wants to quit.
 */
static gboolean
warn_modified( NactMainWindow *window )
{
	gboolean confirm = FALSE;
	gchar *first;
	gchar *second;

	first = g_strdup_printf( _( "Some items have been modified." ));
	second = g_strdup( _( "Are you sure you want to quit without saving them ?" ));

	confirm = base_window_display_yesno_dlg( NULL, first, second );

	g_free( second );
	g_free( first );

	return( confirm );
}
