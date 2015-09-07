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
#include <libintl.h>
#include <string.h>

#include "api/na-object-api.h"

#include "core/na-gtk-utils.h"
#include "core/na-io-provider.h"

#include "base-gtk-utils.h"
#include "nact-iproperties-tab.h"
#include "nact-main-tab.h"
#include "nact-main-window.h"

/* private interface data
 */
struct _NactIPropertiesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* i18n: label of the push button when there is not yet any shortcut */
#define NO_SHORTCUT						N_( "None" )

/* data set against the instance
 */
typedef struct {
	gboolean on_selection_change;
}
	IPropertiesData;

#define IPROPERTIES_TAB_PROP_DATA		"nact-iproperties-tab-data"

static guint st_initializations = 0;	/* interface initialization count */

static GType            register_type( void );
static void             interface_base_init( NactIPropertiesTabInterface *klass );
static void             interface_base_finalize( NactIPropertiesTabInterface *klass );
static void             initialize_window( NactIPropertiesTab *instance );
static void             on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIPropertiesTab *instance );
static void             on_main_item_updated( NactIPropertiesTab *instance, FMAIContext *context, guint data, void *empty );
static GtkButton       *get_enabled_button( NactIPropertiesTab *instance );
static void             on_enabled_toggled( GtkToggleButton *button, NactIPropertiesTab *instance );
static void             on_readonly_toggled( GtkToggleButton *button, NactIPropertiesTab *instance );
static void             on_description_changed( GtkTextBuffer *buffer, NactIPropertiesTab *instance );
static void             on_shortcut_clicked( GtkButton *button, NactIPropertiesTab *instance );
static void             display_provider_name( NactIPropertiesTab *instance, NAObjectItem *item );
static IPropertiesData *get_iproperties_data( NactIPropertiesTab *instance );
static void             on_instance_finalized( gpointer user_data, NactIPropertiesTab *instance );

GType
nact_iproperties_tab_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_iproperties_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIPropertiesTabInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIPropertiesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( NactIPropertiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_iproperties_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIPropertiesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( NactIPropertiesTabInterface *klass )
{
	static const gchar *thisfn = "nact_iproperties_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * nact_iproperties_tab_init:
 * @instance: this #NactIPropertiesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
nact_iproperties_tab_init( NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_init";
	IPropertiesData *data;

	g_return_if_fail( NACT_IS_IPROPERTIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	nact_main_tab_init( NACT_MAIN_WINDOW( instance ), TAB_PROPERTIES );
	initialize_window( instance );

	data = get_iproperties_data( instance );
	data->on_selection_change = FALSE;

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_window( NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_initialize_window";
	GtkButton *enabled_button;
	GtkWidget *label_widget;
	GtkTextBuffer *buffer;
	NactTreeView *tview;

	g_return_if_fail( NACT_IS_IPROPERTIES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = nact_main_window_get_items_view( NACT_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	g_signal_connect(
			instance, MAIN_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_main_item_updated ), NULL );

	enabled_button = get_enabled_button( instance );
	g_signal_connect(
			enabled_button, "toggled", G_CALLBACK( on_enabled_toggled ), instance );

	label_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionDescriptionText" );
	buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( label_widget ));
	g_signal_connect(
			buffer, "changed", G_CALLBACK( on_description_changed ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "SuggestedShortcutButton",
			"clicked", G_CALLBACK( on_shortcut_clicked ), instance );

	na_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "ActionReadonlyButton",
			"toggled", G_CALLBACK( on_readonly_toggled ), instance );
}

static void
on_tree_selection_changed( NactTreeView *tview, GList *selected_items, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_tree_selection_changed";
	guint count_selected;
	NAObjectItem *item;
	gboolean editable;
	gboolean enable_tab;
	GtkNotebook *notebook;
	GtkWidget *page;
	GtkWidget *title_widget, *label_widget, *shortcut_button;
	GtkButton *enabled_button;
	gboolean enabled_item;
	GtkToggleButton *readonly_button;
	GtkTextBuffer *buffer;
	gchar *label, *shortcut;
	IPropertiesData *data;

	g_return_if_fail( NACT_IS_IPROPERTIES_TAB( instance ));

	count_selected = g_list_length( selected_items );
	g_debug( "%s: tview=%p, count_selected=%d, instance=%p (%s)",
			thisfn, tview, count_selected, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			MAIN_PROP_EDITABLE, &editable,
			NULL );

	g_return_if_fail( !item || NA_IS_OBJECT_ITEM( item ));

	enable_tab = ( count_selected == 1 );
	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_PROPERTIES, enable_tab );

	data = get_iproperties_data( instance );
	data->on_selection_change = TRUE;

	notebook = GTK_NOTEBOOK( na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "main-notebook" ));
	page = gtk_notebook_get_nth_page( notebook, TAB_ACTION );
	title_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionPropertiesTitle" );
	label_widget = gtk_notebook_get_tab_label( notebook, page );

	if( item && NA_IS_OBJECT_MENU( item )){
		gtk_label_set_label( GTK_LABEL( label_widget ), _( "Me_nu" ));
		gtk_label_set_markup( GTK_LABEL( title_widget ), _( "<b>Menu editable properties</b>" ));
	} else {
		gtk_label_set_label( GTK_LABEL( label_widget ), _( "_Action" ));
		gtk_label_set_markup( GTK_LABEL( title_widget ), _( "<b>Action editable properties</b>" ));
	}

	enabled_button = get_enabled_button( instance );
	enabled_item = item ? na_object_is_enabled( NA_OBJECT_ITEM( item )) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( enabled_button ), enabled_item );
	base_gtk_utils_set_editable( G_OBJECT( enabled_button ), editable );

	label_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionDescriptionText" );
	buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( label_widget ));
	label = item ? na_object_get_description( item ) : g_strdup( "" );
	gtk_text_buffer_set_text( buffer, label, -1 );
	g_free( label );
	base_gtk_utils_set_editable( G_OBJECT( label_widget ), editable );

	shortcut_button = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SuggestedShortcutButton" );
	shortcut = item ? na_object_get_shortcut( item ) : g_strdup( "" );
	if( !shortcut || !strlen( shortcut )){
		g_free( shortcut );
		shortcut = g_strdup( gettext( NO_SHORTCUT ));
	}
	gtk_button_set_label( GTK_BUTTON( shortcut_button ), shortcut );
	g_free( shortcut );
	base_gtk_utils_set_editable( G_OBJECT( shortcut_button ), editable );

	/* TODO: don't know how to edit a shortcut for now */
	gtk_widget_set_sensitive( shortcut_button, FALSE );

	/* read-only toggle only indicates the intrinsic writability status of this item
	 * _not_ the writability status of the provider
	 */
	readonly_button = GTK_TOGGLE_BUTTON( na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionReadonlyButton" ));
	gtk_toggle_button_set_active( readonly_button, item ? na_object_is_readonly( item ) : FALSE );
	base_gtk_utils_set_editable( G_OBJECT( readonly_button ), FALSE );

	label_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionItemID" );
	label = item ? na_object_get_id( item ) : g_strdup( "" );
	gtk_label_set_text( GTK_LABEL( label_widget ), label );
	g_free( label );

	display_provider_name( instance, item );

	data->on_selection_change = FALSE;
}

static void
on_main_item_updated( NactIPropertiesTab *instance, FMAIContext *context, guint data, void *empty )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_main_item_updated";

	if( data & MAIN_DATA_PROVIDER ){

		g_debug( "%s: instance=%p, item=%p (%s), data=%u, empty=%p",
				thisfn, ( void * ) instance,
				( void * ) context, G_OBJECT_TYPE_NAME( context ), data, empty );

		display_provider_name( instance, NA_OBJECT_ITEM( context ));
	}
}

static GtkButton *
get_enabled_button( NactIPropertiesTab *instance )
{
	return( GTK_BUTTON( na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionEnabledButton" )));
}

static void
on_enabled_toggled( GtkToggleButton *button, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_enabled_toggled";
	NAObjectItem *item;
	gboolean enabled;
	gboolean editable;
	IPropertiesData *data;

	data = get_iproperties_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p, on_selection_change=%s",
				thisfn, ( void * ) button, ( void * ) instance, data->on_selection_change ? "True":"False" );

		g_object_get(
				G_OBJECT( instance ),
				MAIN_PROP_ITEM, &item,
				MAIN_PROP_EDITABLE, &editable,
				NULL );

		if( item && NA_IS_OBJECT_ITEM( item )){
			enabled = gtk_toggle_button_get_active( button );

			if( editable ){
				na_object_set_enabled( item, enabled );
				g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );

			} else {
				g_signal_handlers_block_by_func(( gpointer ) button, on_enabled_toggled, instance );
				gtk_toggle_button_set_active( button, !enabled );
				g_signal_handlers_unblock_by_func(( gpointer ) button, on_enabled_toggled, instance );
			}
		}
	}
}

/*
 * prevent the user to click on the button
 * - draw-indicator property: transform the check button (TRUE) into a toggle button (FALSE)
 * - toggled signal is of type run first
 *   so we can only execute our user signal handler after the object signal handler
 *   has already been executed
 * - overriding the class handler does not work: the overriding handler is called, we
 *   are able to distinguish between our button and others, but even stopping the signal
 *   emission does not prevent the checkbox to be checked/unchecked
 * - last trying to set an emission hook, but:
 *   emission of signal "toggled" for instance `0x9ceace0' cannot be stopped from emission hook
 *
 * so the solution to re-toggle the button inside of the signal handler, if it is not
 * the most elegant, seems at least working without too drawbacks
 */
static void
on_readonly_toggled( GtkToggleButton *button, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_readonly_toggled";
	gboolean active;
	IPropertiesData *data;

	data = get_iproperties_data( instance );

	if( !data->on_selection_change ){
		g_debug( "%s: button=%p, instance=%p, on_selection_change=%s",
				thisfn, ( void * ) button, ( void * ) instance, data->on_selection_change ? "True":"False" );

		active = gtk_toggle_button_get_active( button );

		g_signal_handlers_block_by_func(( gpointer ) button, on_readonly_toggled, instance );
		gtk_toggle_button_set_active( button, !active );
		g_signal_handlers_unblock_by_func(( gpointer ) button, on_readonly_toggled, instance );
	}
}

#if 0
static void
on_readonly_toggle_cb( GtkToggleButton *button, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_readonly_toggle_cb";
	g_debug( "%s: button=%p, instance=%p", thisfn, ( void * ) button, ( void * ) instance );

	if( rdbtn != GTK_WIDGET( button )){
		g_debug( "%s: not called for our button, calling the default handler", thisfn );
		g_signal_chain_from_overridden_handler( button, instance );
		return;
	}

	g_debug( "%s: called for our button, stop emission", thisfn );
	g_signal_stop_emission_by_name( button, "toggled" );
}

static gboolean
on_readonly_toggle_hook( GSignalInvocationHint *ihint,
        guint n_param_values, const GValue *param_values, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_readonly_toggle_hook";
	GtkWidget *button;
	GtkWidget *signaled_object;

	g_debug( "%s: n_param=%d, instance=%p", thisfn, n_param_values, ( void * ) instance );

	if( instance ){
		signaled_object = ( GtkWidget * ) g_value_get_object( param_values );
		if( signaled_object ){
			button = base_window_get_widget( BASE_WINDOW( instance ), "ActionReadonlyButton" );
			if( button == signaled_object ){
				g_debug( "%s: called for our button, stop emission", thisfn );
				g_signal_stop_emission_by_name( button, "toggled" );
			}
		}
	}

	/* stay connected */
	return( TRUE );
}
#endif

static void
on_description_changed( GtkTextBuffer *buffer, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_description_changed";
	NAObjectItem *item;
	GtkTextIter start, end;
	gchar *text;

	g_debug( "%s: buffer=%p, instance=%p", thisfn, ( void * ) buffer, ( void * ) instance );

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			NULL );

	if( item ){
		gtk_text_buffer_get_start_iter( buffer, &start );
		gtk_text_buffer_get_end_iter( buffer, &end );
		text = gtk_text_buffer_get_text( buffer, &start, &end, TRUE );
		na_object_set_description( item, text );
		g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_ITEM_UPDATED, item, 0 );
	}
}

static void
on_shortcut_clicked( GtkButton *button, NactIPropertiesTab *instance )
{
	NAObjectItem *item;

	g_object_get(
			G_OBJECT( instance ),
			MAIN_PROP_ITEM, &item,
			NULL );

	if( item ){
		/*g_signal_emit_by_name( G_OBJECT( instance ), MAIN_SIGNAL_TAB_UPDATED, edited, 0 );*/
	}
}

static void
display_provider_name( NactIPropertiesTab *instance, NAObjectItem *item )
{
	GtkWidget *label_widget;
	gchar *label;
	NAIOProvider *provider;

	label_widget = na_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "ActionItemProvider" );
	label = NULL;
	if( item ){
		provider = na_object_get_provider( item );
		if( provider ){
			label = na_io_provider_get_name( provider );
		}
	}
	if( !label ){
		label = g_strdup( "" );
	}
	gtk_label_set_text( GTK_LABEL( label_widget ), label );
	g_free( label );
	gtk_widget_set_sensitive( label_widget, item != NULL );
}

static IPropertiesData *
get_iproperties_data( NactIPropertiesTab *instance )
{
	IPropertiesData *data;

	data = ( IPropertiesData * ) g_object_get_data( G_OBJECT( instance ), IPROPERTIES_TAB_PROP_DATA );

	if( !data ){
		data = g_new0( IPropertiesData, 1 );
		g_object_set_data( G_OBJECT( instance ), IPROPERTIES_TAB_PROP_DATA, data );
	}

	return( data );
}

static void
on_instance_finalized( gpointer user_data, NactIPropertiesTab *instance )
{
	static const gchar *thisfn = "nact_iproperties_tab_on_instance_finalized";
	IPropertiesData *data;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );

	data = get_iproperties_data( instance );

	g_free( data );
}
