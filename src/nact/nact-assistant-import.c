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

#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <string.h>

#include <api/fma-object-api.h>
#include <api/fma-core-utils.h>

#include <core/fma-import-mode.h>
#include <core/fma-importer.h>
#include <core/fma-ioptions-list.h>
#include <core/fma-gtk-utils.h>
#include <core/na-settings.h>

#include "nact-application.h"
#include "nact-assistant-import.h"
#include "nact-main-window.h"
#include "nact-tree-ieditable.h"

/* Import Assistant
 *
 * pos.  type     title
 * ---   -------  --------------------------------------------------
 *   0   Intro    Introduction
 *   1   Content  Selection of the files
 *   2   Content  Duplicate management: what to do with duplicates ?
 *   3   Confirm  Display the selected files before import
 *   4   Summary  Import is done: summary of the done operations
 */
enum {
	ASSIST_PAGE_INTRO = 0,
	ASSIST_PAGE_FILES_SELECTION,
	ASSIST_PAGE_DUPLICATES,
	ASSIST_PAGE_CONFIRM,
	ASSIST_PAGE_DONE
};

/* column ordering in the duplicates treeview
 */
enum {
	IMAGE_COLUMN = 0,
	LABEL_COLUMN,
	TOOLTIP_COLUMN,
	MODE_COLUMN,
	INDEX_COLUMN,
	N_COLUMN
};

/* private instance data
 */
struct _NactAssistantImportPrivate {
	gboolean     dispose_has_run;
	GtkWidget   *file_chooser;
	GtkTreeView *duplicates_listview;
	FMAIOption   *mode;
	GList       *results;
	GList       *overriden;
};

static const gchar        *st_xmlui_filename = PKGUIDIR "/nact-assistant-import.ui";
static const gchar        *st_toplevel_name  = "ImportAssistant";
static const gchar        *st_wsp_name       = NA_IPREFS_IMPORT_ASSISTANT_WSP;

static BaseAssistantClass *st_parent_class   = NULL;

static GType         register_type( void );
static void          class_init( NactAssistantImportClass *klass );
static void          ioptions_list_iface_init( FMAIOptionsListInterface *iface, void *user_data );
static GList        *ioptions_list_get_modes( const FMAIOptionsList *instance, GtkWidget *container );
static void          ioptions_list_free_modes( const FMAIOptionsList *instance, GtkWidget *container, GList *modes );
static FMAIOption    *ioptions_list_get_ask_option( const FMAIOptionsList *instance, GtkWidget *container );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_dispose( GObject *application );
static void          instance_finalize( GObject *application );

static void          on_base_initialize_gtk( NactAssistantImport *dialog );
static void          create_duplicates_treeview_model( NactAssistantImport *dialog );
static void          on_base_initialize_base_window( NactAssistantImport *dialog );
static void          runtime_init_intro( NactAssistantImport *window, GtkAssistant *assistant );
static void          runtime_init_file_selector( NactAssistantImport *window, GtkAssistant *assistant );
static void          on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data );
static gboolean      has_loadable_files( GSList *uris );
static void          runtime_init_duplicates( NactAssistantImport *window, GtkAssistant *assistant );

static void          assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page );
static void          prepare_confirm( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page );
static void          assistant_apply( BaseAssistant *window, GtkAssistant *assistant );
static FMAObjectItem *check_for_existence( const FMAObjectItem *, NactMainWindow *window );
static void          prepare_importdone( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page );
static void          free_results( GList *list );

static GtkWidget    *find_widget_from_page( GtkWidget *page, const gchar *name );
static GtkTreeView  *get_duplicates_treeview_from_assistant_import( NactAssistantImport *window );
static GtkTreeView  *get_duplicates_treeview_from_page( GtkWidget *page );

GType
nact_assistant_import_get_type( void )
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
	static const gchar *thisfn = "nact_assistant_import_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAssistantImportClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistantImport ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo ioptions_list_iface_info = {
		( GInterfaceInitFunc ) ioptions_list_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_TYPE_ASSISTANT, "NactAssistantImport", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_IOPTIONS_LIST, &ioptions_list_iface_info );

	return( type );
}

static void
class_init( NactAssistantImportClass *klass )
{
	static const gchar *thisfn = "nact_assistant_import_class_init";
	GObjectClass *object_class;
	BaseAssistantClass *assist_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	assist_class = BASE_ASSISTANT_CLASS( klass );
	assist_class->apply = assistant_apply;
	assist_class->prepare = assistant_prepare;
}

static void
ioptions_list_iface_init( FMAIOptionsListInterface *iface, void *user_data )
{
	static const gchar *thisfn = "nact_assistant_import_ioptions_list_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->get_options = ioptions_list_get_modes;
	iface->free_options = ioptions_list_free_modes;
	iface->get_ask_option = ioptions_list_get_ask_option;
}

static GList *
ioptions_list_get_modes( const FMAIOptionsList *instance, GtkWidget *container )
{
	GList *modes;

	g_return_val_if_fail( NACT_IS_ASSISTANT_IMPORT( instance ), NULL );

	modes = fma_importer_get_modes();

	return( modes );
}

static void
ioptions_list_free_modes( const FMAIOptionsList *instance, GtkWidget *container, GList *modes )
{
	fma_importer_free_modes( modes );
}

static FMAIOption *
ioptions_list_get_ask_option( const FMAIOptionsList *instance, GtkWidget *container )
{
	return( fma_importer_get_ask_mode());
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_import_instance_init";
	NactAssistantImport *self;

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_ASSISTANT_IMPORT( instance );

	self->private = g_new0( NactAssistantImportPrivate, 1 );

	self->private->results = NULL;

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_SIGNAL_INITIALIZE_GTK,
			G_CALLBACK( on_base_initialize_gtk ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_SIGNAL_INITIALIZE_WINDOW,
			G_CALLBACK( on_base_initialize_base_window ));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_import_instance_dispose";
	NactAssistantImport *self;

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( window ));

	self = NACT_ASSISTANT_IMPORT( window );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_assistant_import_instance_finalize";
	NactAssistantImport *self;

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_ASSISTANT_IMPORT( window );

	free_results( self->private->results );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * nact_assistant_import_run:
 * @main: the #NactMainWindow parent window of this assistant.
 *
 * Run the assistant.
 */
void
nact_assistant_import_run( NactMainWindow *main_window )
{
	NactAssistantImport *assistant;
	gboolean esc_quit, esc_confirm;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( main_window ));

	esc_quit = na_settings_get_boolean( NA_IPREFS_ASSISTANT_ESC_QUIT, NULL, NULL );
	esc_confirm = na_settings_get_boolean( NA_IPREFS_ASSISTANT_ESC_CONFIRM, NULL, NULL );

	assistant = g_object_new( NACT_TYPE_ASSISTANT_IMPORT,
			BASE_PROP_MAIN_WINDOW,     main_window,
			BASE_PROP_HAS_OWN_BUILDER, TRUE,
			BASE_PROP_XMLUI_FILENAME,  st_xmlui_filename,
			BASE_PROP_TOPLEVEL_NAME,   st_toplevel_name,
			BASE_PROP_WSP_NAME,        st_wsp_name,
			BASE_PROP_QUIT_ON_ESCAPE,  esc_quit,
			BASE_PROP_WARN_ON_ESCAPE,  esc_confirm,
			NULL );

	base_window_run( BASE_WINDOW( assistant ));
}

static void
on_base_initialize_gtk( NactAssistantImport *dialog )
{
	static const gchar *thisfn = "nact_assistant_import_on_base_initialize_gtk";

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

#if !GTK_CHECK_VERSION( 3,0,0 )
		guint padder = 8;
		GtkAssistant *assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( dialog )));
		/* selecting files */
		GtkWidget *page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );
		GtkWidget *container = find_widget_from_page( page, "p1-l2-alignment1" );
		g_object_set( G_OBJECT( container ), "top_padding", padder, NULL );
		/* managing duplicates */
		page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_DUPLICATES );
		container = find_widget_from_page( page, "p2-l2-alignment1" );
		g_object_set( G_OBJECT( container ), "border_width", padder, NULL );
		/* summary */
		page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_CONFIRM );
		container = find_widget_from_page( page, "p3-l2-alignment1" );
		g_object_set( G_OBJECT( container ), "border_width", padder, NULL );
		/* import is done */
		page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_DONE );
		container = find_widget_from_page( page, "p4-l2-alignment1" );
		g_object_set( G_OBJECT( container ), "border_width", padder, NULL );
#endif

		create_duplicates_treeview_model( dialog );
	}
}

static void
create_duplicates_treeview_model( NactAssistantImport *dialog )
{
	static const gchar *thisfn = "nact_assistant_import_create_duplicates_treeview_model";

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( dialog ));
	g_return_if_fail( !dialog->private->dispose_has_run );

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	dialog->private->duplicates_listview = get_duplicates_treeview_from_assistant_import( dialog );
	g_return_if_fail( GTK_IS_TREE_VIEW( dialog->private->duplicates_listview ));

	fma_ioptions_list_gtk_init( FMA_IOPTIONS_LIST( dialog ), GTK_WIDGET( dialog->private->duplicates_listview ), TRUE );
}

static void
on_base_initialize_base_window( NactAssistantImport *dialog )
{
	static const gchar *thisfn = "nact_assistant_import_on_base_initialize_base_window";
	GtkAssistant *assistant;

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( dialog )));

		runtime_init_intro( dialog, assistant );
		runtime_init_file_selector( dialog, assistant );
		runtime_init_duplicates( dialog, assistant );
	}
}

static void
runtime_init_intro( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_intro";
	GtkWidget *page;

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_INTRO );

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

/*
 * Starting with Gtk 3.2, the widgets of the page are no more attached
 * to the GtkAssistant, but only to the page.
 */
static void
runtime_init_file_selector( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_file_selector";
	GtkWidget *page;
	GtkWidget *chooser;
	gchar *uri;

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_FILES_SELECTION );
	g_return_if_fail( GTK_IS_CONTAINER( page ));

	chooser = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( page ), "ImportFileChooser" );
	g_return_if_fail( GTK_IS_FILE_CHOOSER( chooser ));

	g_debug( "%s: window=%p, assistant=%p, page=%p, chooser=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page, ( void * ) chooser );


	uri = na_settings_get_string( NA_IPREFS_IMPORT_ASSISTANT_URI, NULL, NULL );
	if( uri && strlen( uri )){
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( chooser ), uri );
	}
	g_free( uri );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( chooser ),
			"selection-changed",
			G_CALLBACK( on_file_selection_changed ));

	window->private->file_chooser = chooser;

	gtk_assistant_set_page_complete( assistant, page, FALSE );
}

static void
on_file_selection_changed( GtkFileChooser *chooser, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_import_on_file_selection_changed";
	GtkAssistant *assistant;
	gint pos;
	GSList *uris;
	gboolean enabled;
	gchar *folder;
	GtkWidget *content;

	g_assert( NACT_IS_ASSISTANT_IMPORT( user_data ));
	assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( user_data )));
	pos = gtk_assistant_get_current_page( assistant );
	if( pos == ASSIST_PAGE_FILES_SELECTION ){

		uris = gtk_file_chooser_get_uris( chooser );
		enabled = has_loadable_files( uris );

		if( enabled ){
			/*
			 * if user has selected the 'Recently used' place in the file chooser,
			 * then the current folder uri is null
			 * (Gtk+ 3.2.0, don't know before...)
			 */
			folder = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( chooser ));
			g_debug( "%s: current folder uri=%s", thisfn, folder );
			if( folder && strlen( folder )){
				na_settings_set_string( NA_IPREFS_IMPORT_ASSISTANT_URI, folder );
			}
			g_free( folder );
		}

		fma_core_utils_slist_free( uris );

		content = gtk_assistant_get_nth_page( assistant, pos );
		gtk_assistant_set_page_complete( assistant, content, enabled );
		gtk_assistant_update_buttons_state( assistant );
	}
}

/*
 * enable forward button if current selection has at least one loadable file
 */
static gboolean
has_loadable_files( GSList *uris )
{
	GSList *iuri;
	gchar *uri;
	int loadables = 0;

	for( iuri = uris ; iuri ; iuri = iuri->next ){
		uri = ( gchar * ) iuri->data;

		if( !strlen( uri )){
			continue;
		}

		if( fma_core_utils_file_is_loadable( uri )){
			loadables += 1;
		}
	}

	return( loadables > 0 );
}

static void
runtime_init_duplicates( NactAssistantImport *window, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_runtime_init_duplicates";
	gchar *import_mode;
	GtkWidget *page;
	gboolean mandatory;

	g_return_if_fail( GTK_IS_TREE_VIEW( window->private->duplicates_listview ));

	g_debug( "%s: window=%p, assistant=%p",
			thisfn, ( void * ) window, ( void * ) assistant );

	import_mode = na_settings_get_string( NA_IPREFS_IMPORT_PREFERRED_MODE, NULL, &mandatory );
	fma_ioptions_list_set_editable(
			FMA_IOPTIONS_LIST( window ), GTK_WIDGET( window->private->duplicates_listview ),
			!mandatory );
	fma_ioptions_list_set_default(
			FMA_IOPTIONS_LIST( window ), GTK_WIDGET( window->private->duplicates_listview ),
			import_mode );
	g_free( import_mode );

	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_DUPLICATES );
	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

static void
assistant_prepare( BaseAssistant *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_assistant_prepare";
	GtkAssistantPageType type;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	type = gtk_assistant_get_page_type( assistant, page );

	switch( type ){
		case GTK_ASSISTANT_PAGE_CONFIRM:
			prepare_confirm( NACT_ASSISTANT_IMPORT( window ), assistant, page );
			break;

		case GTK_ASSISTANT_PAGE_SUMMARY:
			prepare_importdone( NACT_ASSISTANT_IMPORT( window ), assistant, page );
			break;

		default:
			break;
	}
}

static void
prepare_confirm( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_prepare_confirm";
	gchar *text, *tmp;
	GSList *uris, *is;
	GtkWidget *label;
	gchar *mode_label, *label2, *mode_description;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

#if !GTK_CHECK_VERSION( 3,0,0 )
	/* Note that, at least, in Gtk 2.20 (Ubuntu 10) and 2.22 (Fedora 14), GtkLabel
	 * queues its resize (when the text is being set), but the actual resize does
	 * not happen immediately - We have to wait until Gtk 3.0, most probably due
	 * to the new width-for-height and height-for-width features...
	 */
	GtkWidget *vbox = find_widget_from_page( page, "p3-ConfirmVBox" );
	gtk_container_set_resize_mode( GTK_CONTAINER( vbox ), GTK_RESIZE_IMMEDIATE );
#endif

	/* adding list of uris to import
	 */
	text = NULL;
	uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( window->private->file_chooser ));
	for( is = uris ; is ; is = is->next ){
		g_debug( "%s: uri=%s", thisfn, ( const gchar * ) is->data );

		if( text ){
			tmp = g_strdup_printf( "%s\n%s", text, ( const gchar * ) is->data );
			g_free( text );
			text = tmp;

		} else {
			text = g_strdup(( const gchar * ) is->data );
		}
	}
	label = find_widget_from_page( page, "p3-ConfirmFilesList" );
	g_return_if_fail( GTK_IS_LABEL( label ));
	gtk_label_set_text( GTK_LABEL( label ), text );
	g_free( text );

	/* adding import mode
	 */
	label = find_widget_from_page( page, "p3-ConfirmImportMode" );
	g_return_if_fail( GTK_IS_LABEL( label ));
	window->private->mode = fma_ioptions_list_get_selected(
			FMA_IOPTIONS_LIST( window ), GTK_WIDGET( window->private->duplicates_listview ));
	g_return_if_fail( FMA_IS_IMPORT_MODE( window->private->mode ));
	mode_label = fma_ioption_get_label( window->private->mode );
	label2 = fma_core_utils_str_remove_char( mode_label, "_" );
	mode_description = fma_ioption_get_description( window->private->mode );
	text = g_markup_printf_escaped( "%s\n\n<span style=\"italic\">%s</span>", label2, mode_description );
	gtk_label_set_markup( GTK_LABEL( label ), text );
	g_free( text );
	g_free( mode_description );
	g_free( mode_label );
	g_free( label2 );

	gtk_assistant_set_page_complete( assistant, page, TRUE );
}

/*
 * do import here
 */
static void
assistant_apply( BaseAssistant *wnd, GtkAssistant *assistant )
{
	static const gchar *thisfn = "nact_assistant_import_assistant_apply";
	NactAssistantImport *window;
	FMAImporterParms importer_parms;
	BaseWindow *main_window;
	GList *import_results, *it;
	GList *insertable_items, *overriden_items;
	FMAImporterResult *result;
	NactApplication *application;
	NAUpdater *updater;
	NactTreeView *items_view;

	g_return_if_fail( NACT_IS_ASSISTANT_IMPORT( wnd ));

	g_debug( "%s: window=%p, assistant=%p", thisfn, ( void * ) wnd, ( void * ) assistant );
	window = NACT_ASSISTANT_IMPORT( wnd );
	g_object_get( G_OBJECT( window ), BASE_PROP_MAIN_WINDOW, &main_window, NULL );
	application = NACT_APPLICATION( base_window_get_application( main_window ));
	updater = nact_application_get_updater( application );

	memset( &importer_parms, '\0', sizeof( FMAImporterParms ));
	importer_parms.uris = gtk_file_chooser_get_uris( GTK_FILE_CHOOSER( window->private->file_chooser ));
	importer_parms.check_fn = ( FMAImporterCheckFn ) check_for_existence;
	importer_parms.check_fn_data = main_window;
	importer_parms.preferred_mode = fma_import_mode_get_id( FMA_IMPORT_MODE( window->private->mode ));
	importer_parms.parent_toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( wnd ));

	import_results = fma_importer_import_from_uris( FMA_PIVOT( updater ), &importer_parms );

	insertable_items = NULL;
	overriden_items = NULL;

	for( it = import_results ; it ; it = it->next ){
		result = ( FMAImporterResult * ) it->data;
		if( result->imported ){

			if( !result->exist || result->mode == IMPORTER_MODE_RENUMBER ){
				insertable_items = g_list_prepend( insertable_items, result->imported );

			} else if( result->mode == IMPORTER_MODE_OVERRIDE ){
				overriden_items = g_list_prepend( overriden_items, result->imported );
			}
		}
	}

	fma_core_utils_slist_free( importer_parms.uris );
	window->private->results = import_results;

	/* then insert the list
	 * assuring that actions will be inserted in the same order as uris
	 *
	 * the tree view (and its underlying tree store) takes a new reference
	 * on the inserted objects; the pointers so remain valid even after
	 * having released the imported_items list
	 */
	if( insertable_items ){
		insertable_items = g_list_reverse( insertable_items );
		items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( main_window ));
		nact_tree_ieditable_insert_items( NACT_TREE_IEDITABLE( items_view ), insertable_items, NULL );
		fma_object_free_items( insertable_items );
	}

	/* contrarily, the tree store may or not take a new reference on overriding
	 * items, so do not release it here
	 */
	if( overriden_items ){
		items_view = nact_main_window_get_items_view( NACT_MAIN_WINDOW( main_window ));
		nact_tree_ieditable_set_items( NACT_TREE_IEDITABLE( items_view ), overriden_items );
		window->private->overriden = overriden_items;
	}
}

static FMAObjectItem *
check_for_existence( const FMAObjectItem *item, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_assistant_import_check_for_existence";
	NactTreeView *items_view;
	FMAObjectItem *exists;
	gchar *importing_id;

	importing_id = fma_object_get_id( item );
	g_debug( "%s: item=%p (%s), importing_id=%s",
			thisfn, ( void * ) item, G_OBJECT_TYPE_NAME( item ), importing_id );

	items_view = nact_main_window_get_items_view( window );
	exists = nact_tree_view_get_item_by_id( items_view, importing_id );

	g_free( importing_id );

	return( exists );
}

/*
 * summary page is a vbox inside of a scrolled window
 * each line in this vbox is a GtkLabel
 * Starting with 3.1.6, uri is displayed in red if an error has occured, or
 * in blue.
 */
static void
prepare_importdone( NactAssistantImport *window, GtkAssistant *assistant, GtkWidget *page )
{
	static const gchar *thisfn = "nact_assistant_import_prepare_importdone";
	guint width;
	GtkWidget *vbox;
	GtkWidget *file_vbox, *file_uri, *file_report;
	GList *is;
	GSList *im;
	FMAImporterResult *result;
	gchar *text, *id, *item_label, *text2, *tmp;
	const gchar *color;
	gchar *mode_id;

	g_debug( "%s: window=%p, assistant=%p, page=%p",
			thisfn, ( void * ) window, ( void * ) assistant, ( void * ) page );

	width = 15;
	vbox = find_widget_from_page( page, "p4-SummaryVBox" );
	g_return_if_fail( GTK_IS_BOX( vbox ));

#if !GTK_CHECK_VERSION( 3,0,0 )
	/* Note that, at least, in Gtk 2.20 (Ubuntu 10) and 2.22 (Fedora 14), GtkLabel
	 * queues its resize (when the text is being set), but the actual resize does
	 * not happen immediately - We have to wait until Gtk 3.0, most probably due
	 * to the new width-for-height and height-for-width features...
	 */
	gtk_container_set_resize_mode( GTK_CONTAINER( vbox ), GTK_RESIZE_IMMEDIATE );
#endif

	/* for each uri
	 * 	- display the uri
	 *  - display a brief import log
	 */
	for( is = window->private->results ; is ; is = is->next ){
		result = ( FMAImporterResult * ) is->data;
		g_debug( "%s: uri=%s", thisfn, result->uri );

		/* display the uri
		 */
#if GTK_CHECK_VERSION( 3,0,0 )
		file_vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 4 );
#else
		file_vbox = gtk_vbox_new( FALSE, 4 );
#endif
		gtk_box_pack_start( GTK_BOX( vbox ), file_vbox, FALSE, FALSE, 0 );

		color = result->imported ? "blue" : "red";
		text = g_markup_printf_escaped( "<span foreground=\"%s\">%s</span>", color, result->uri );
		file_uri = gtk_label_new( NULL );
		gtk_label_set_markup( GTK_LABEL( file_uri ), text );
		g_free( text );
		g_object_set( G_OBJECT( file_uri ), "xalign", 0, NULL );
		g_object_set( G_OBJECT( file_uri ), "xpad", width, NULL );
		gtk_box_pack_start( GTK_BOX( file_vbox ), file_uri, FALSE, FALSE, 0 );

		/* display the import log
		 */
		if( result->imported ){
			/* i18n: indicate that the file has been successfully imported */
			text = g_strdup( _( "Import OK" ));
			id = fma_object_get_id( result->imported );
			item_label = fma_object_get_label( result->imported );
			/* i18n: this is the globally unique identifier and the label of the newly imported action */
			text2 = g_strdup_printf( _( "Id.: %s\t%s" ), id, item_label);
			g_free( item_label );
			g_free( id );
			tmp = g_strdup_printf( "%s\n%s", text, text2 );
			g_free( text );
			g_free( text2 );
			text = tmp;

		} else {
			/* i18n: indicate that the file was not imported */
			text = g_strdup( _( "Not imported" ));
		}

		/* add messages if any
		 */
		for( im = result->messages ; im ; im = im->next ){
			tmp = g_strdup_printf( "%s\n%s", text, ( const char * ) im->data );
			g_free( text );
			text = tmp;
		}

		file_report = gtk_label_new( text );
		gtk_label_set_line_wrap( GTK_LABEL( file_report ), TRUE );
		gtk_label_set_line_wrap_mode( GTK_LABEL( file_report ), PANGO_WRAP_WORD );
		g_object_set( G_OBJECT( file_report ), "xalign", 0, NULL );
		g_object_set( G_OBJECT( file_report ), "xpad", 2*width, NULL );
		gtk_box_pack_start( GTK_BOX( file_vbox ), file_report, FALSE, FALSE, 0 );
	}

	mode_id = fma_ioption_get_id( window->private->mode );
	na_settings_set_string( NA_IPREFS_IMPORT_PREFERRED_MODE, mode_id );
	g_free( mode_id );

	/* release here our reference on overriding items
	 */
	if( window->private->overriden ){
		fma_object_free_items( window->private->overriden );
	}

	g_object_set( G_OBJECT( window ), BASE_PROP_WARN_ON_ESCAPE, FALSE, NULL );
	gtk_assistant_set_page_complete( assistant, page, TRUE );
	gtk_widget_show_all( page );
}

static void
free_results( GList *list )
{
	GList *it;

	for( it = list ; it ; it = it->next ){
		fma_importer_free_result(( FMAImporterResult * ) it->data );
	}

	g_list_free( list );
}

static GtkWidget *
find_widget_from_page( GtkWidget *page, const gchar *name )
{
	GtkWidget *widget;

	g_return_val_if_fail( GTK_IS_CONTAINER( page ), NULL );

	widget = fma_gtk_utils_find_widget_by_name( GTK_CONTAINER( page ), name );

	return( widget );
}

static GtkTreeView *
get_duplicates_treeview_from_assistant_import( NactAssistantImport *window )
{
	GtkAssistant *assistant;
	GtkWidget *page;

	g_return_val_if_fail( NACT_IS_ASSISTANT_IMPORT( window ), NULL );

	assistant = GTK_ASSISTANT( base_window_get_gtk_toplevel( BASE_WINDOW( window )));
	page = gtk_assistant_get_nth_page( assistant, ASSIST_PAGE_DUPLICATES );

	return( get_duplicates_treeview_from_page( page ));
}

static GtkTreeView *
get_duplicates_treeview_from_page( GtkWidget *page )
{
	GtkWidget *listview;

	listview = find_widget_from_page( page, "p2-AskTreeView" );

	g_return_val_if_fail( GTK_IS_TREE_VIEW( listview ), NULL );

	return( GTK_TREE_VIEW( listview ));
}
