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

#include "api/fma-core-utils.h"
#include "api/fma-object-api.h"

#include "core/fma-gtk-utils.h"

#include "base-gtk-utils.h"
#include "fma-main-tab.h"
#include "fma-main-window.h"
#include "fma-match-list.h"
#include "fma-add-scheme-dialog.h"
#include "fma-ischemes-tab.h"

/* private interface data
 */
struct _FMAISchemesTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* the identifier of this notebook page in the Match dialog
 */
#define ITAB_NAME						"schemes"

static guint st_initializations = 0;	/* interface initialization count */

static GType   register_type( void );
static void    interface_base_init( FMAISchemesTabInterface *klass );
static void    interface_base_finalize( FMAISchemesTabInterface *klass );
static void    initialize_gtk( FMAISchemesTab *instance );
static void    initialize_window( FMAISchemesTab *instance );
static void    on_tree_selection_changed( FMATreeView *tview, GList *selected_items, FMAISchemesTab *instance );
static void    on_add_from_defaults( GtkButton *button, FMAISchemesTab *instance );
static GSList *get_schemes( void *context );
static void    set_schemes( void *context, GSList *filters );
static void    on_instance_finalized( gpointer user_data, FMAISchemesTab *instance );

GType
fma_ischemes_tab_get_type( void )
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
	static const gchar *thisfn = "fma_ischemes_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAISchemesTabInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAISchemesTab", &info, 0 );

	g_type_interface_add_prerequisite( type, GTK_TYPE_APPLICATION_WINDOW );

	return( type );
}

static void
interface_base_init( FMAISchemesTabInterface *klass )
{
	static const gchar *thisfn = "fma_ischemes_tab_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( FMAISchemesTabInterfacePrivate, 1 );
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAISchemesTabInterface *klass )
{
	static const gchar *thisfn = "fma_ischemes_tab_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

/**
 * fma_ischemes_tab_init:
 * @instance: this #FMAISchemesTab instance.
 *
 * Initialize the interface
 * Connect to #BaseWindow signals
 */
void
fma_ischemes_tab_init( FMAISchemesTab *instance )
{
	static const gchar *thisfn = "fma_ischemes_tab_init";

	g_return_if_fail( FMA_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn,
			( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_main_tab_init( FMA_MAIN_WINDOW( instance ), TAB_SCHEMES );
	initialize_gtk( instance );
	initialize_window( instance );

	g_object_weak_ref( G_OBJECT( instance ), ( GWeakNotify ) on_instance_finalized, NULL );
}

static void
initialize_gtk( FMAISchemesTab *instance )
{
	static const gchar *thisfn = "fma_ischemes_tab_initialize_gtk";

	g_return_if_fail( FMA_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	fma_match_list_init_with_args(
			FMA_MAIN_WINDOW( instance ),
			ITAB_NAME,
			TAB_SCHEMES,
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "SchemesTreeView" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddSchemeButton" ),
			fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "RemoveSchemeButton" ),
			( pget_filters ) get_schemes,
			( pset_filters ) set_schemes,
			NULL,
			NULL,
			MATCH_LIST_MUST_MATCH_ONE_OF,
			_( "Scheme filter" ),
			TRUE );
}

static void
initialize_window( FMAISchemesTab *instance )
{
	static const gchar *thisfn = "fma_ischemes_tab_initialize_window";
	FMATreeView *tview;

	g_return_if_fail( FMA_IS_ISCHEMES_TAB( instance ));

	g_debug( "%s: instance=%p (%s)",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	tview = fma_main_window_get_items_view( FMA_MAIN_WINDOW( instance ));

	g_signal_connect(
			tview, TREE_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_tree_selection_changed ), instance );

	fma_gtk_utils_connect_widget_by_name(
			GTK_CONTAINER( instance ), "AddFromDefaultButton",
			"clicked", G_CALLBACK( on_add_from_defaults ), instance );
}

static void
on_tree_selection_changed( FMATreeView *tview, GList *selected_items, FMAISchemesTab *instance )
{
	FMAIContext *context;
	gboolean editable;
	gboolean enable_tab;
	GtkWidget *button;

	g_object_get( G_OBJECT( instance ),
			MAIN_PROP_CONTEXT, &context, MAIN_PROP_EDITABLE, &editable,
			NULL );

	enable_tab = ( context != NULL );
	fma_main_tab_enable_page( FMA_MAIN_WINDOW( instance ), TAB_SCHEMES, enable_tab );

	button = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( instance ), "AddFromDefaultButton" );
	base_gtk_utils_set_editable( G_OBJECT( button ), editable );
}

static void
on_add_from_defaults( GtkButton *button, FMAISchemesTab *instance )
{
	GSList *schemes;
	gchar *new_scheme;
	FMAIContext *context;

	g_object_get( G_OBJECT( instance ), MAIN_PROP_CONTEXT, &context, NULL );
	g_return_if_fail( context );

	schemes = fma_match_list_get_rows( FMA_MAIN_WINDOW( instance ), ITAB_NAME );
	new_scheme = fma_add_scheme_dialog_run( FMA_MAIN_WINDOW( instance ), schemes );
	fma_core_utils_slist_free( schemes );

	if( new_scheme ){
		fma_match_list_insert_row( FMA_MAIN_WINDOW( instance ), ITAB_NAME, new_scheme, FALSE, FALSE );
		g_free( new_scheme );
	}
}

static GSList *
get_schemes( void *context )
{
	return( fma_object_get_schemes( context ));
}

static void
set_schemes( void *context, GSList *filters )
{
	fma_object_set_schemes( context, filters );
}

static void
on_instance_finalized( gpointer user_data, FMAISchemesTab *instance )
{
	static const gchar *thisfn = "fma_ischemes_tab_on_instance_finalized";

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
}
