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

#include <gtk/gtk.h>
#include <string.h>

#include "api/fma-object-api.h"

#include "core/fma-exporter.h"
#include "core/fma-export-format.h"

#include "nact-application.h"
#include "nact-export-ask.h"
#include "nact-main-window.h"
#include "nact-tree-model.h"
#include "nact-clipboard.h"

/* private class data
 */
struct _NactClipboardClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
typedef struct {
	guint    target;
	gchar   *folder;
	GList   *rows;
	gboolean copy;
}
	NactClipboardDndData;

typedef struct {
	GList *items;
	gint   mode;
	guint  nb_actions;
	guint  nb_profiles;
	guint  nb_menus;
}
	PrimaryData;

struct _NactClipboardPrivate {
	gboolean        dispose_has_run;
	NactMainWindow *window;
	GtkClipboard   *dnd;
	GtkClipboard   *primary;
	PrimaryData    *primary_data;
	gboolean        primary_got;
};

#define NACT_CLIPBOARD_ATOM				gdk_atom_intern( "_NACT_CLIPBOARD", FALSE )
#define NACT_CLIPBOARD_NACT_ATOM		gdk_atom_intern( "ClipboardNautilusActions", FALSE )

enum {
	NACT_CLIPBOARD_FORMAT_NACT = 0,
	NACT_CLIPBOARD_FORMAT_APPLICATION_XML,
	NACT_CLIPBOARD_FORMAT_TEXT_PLAIN
};

/* clipboard formats
 * - a special ClipboardNautilusAction format for internal move/copy
 *   and also used by drag and drop operations
 * - a XdndDirectSave, suitable for exporting to a file manager
 *   (note that Nautilus recognized the "XdndDirectSave0" format as XDS
 *   protocol)
 * - a text (xml) format, to go to clipboard or a text editor
 */
static GtkTargetEntry clipboard_formats[] = {
	{ "ClipboardNautilusActions", 0, NACT_CLIPBOARD_FORMAT_NACT },
	{ "application/xml",          0, NACT_CLIPBOARD_FORMAT_APPLICATION_XML },
	{ "text/plain",               0, NACT_CLIPBOARD_FORMAT_TEXT_PLAIN },
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NactClipboardClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *application );
static void   instance_finalize( GObject *application );

static void   get_from_dnd_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data );
static void   clear_dnd_clipboard_callback( GtkClipboard *clipboard, NactClipboardDndData *data );
static gchar *export_rows( NactClipboard *clipboard, GList *rows, const gchar *dest_folder );
static gchar *export_objects( NactClipboard *clipboard, GList *objects );
static gchar *export_row_object( NactClipboard *clipboard, FMAObject *object, const gchar *dest_folder, GList **exported, gboolean first );

static void   get_from_primary_clipboard_callback( GtkClipboard *gtk_clipboard, GtkSelectionData *selection_data, guint info, NactClipboard *clipboard );
static void   clear_primary_clipboard( NactClipboard *clipboard );
static void   clear_primary_clipboard_callback( GtkClipboard *gtk_clipboard, NactClipboard *clipboard );
static void   dump_primary_clipboard( NactClipboard *clipboard );

static gchar *clipboard_mode_to_string( gint mode );

GType
nact_clipboard_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_clipboard_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactClipboardClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactClipboard ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NactClipboard", &info, 0 );

	return( type );
}

static void
class_init( NactClipboardClass *klass )
{
	static const gchar *thisfn = "nact_clipboard_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactClipboardClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_clipboard_instance_init";
	NactClipboard *self;
	GdkDisplay *display;

	g_return_if_fail( NACT_IS_CLIPBOARD( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_CLIPBOARD( instance );

	self->private = g_new0( NactClipboardPrivate, 1 );

	self->private->dispose_has_run = FALSE;

	display = gdk_display_get_default();
	self->private->dnd = gtk_clipboard_get_for_display( display, NACT_CLIPBOARD_ATOM );
	self->private->primary = gtk_clipboard_get_for_display( display, GDK_SELECTION_CLIPBOARD );
	self->private->primary_data = NULL;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_clipboard_instance_dispose";
	NactClipboard *self;

	g_return_if_fail( NACT_IS_CLIPBOARD( object ));

	self = NACT_CLIPBOARD( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		gtk_clipboard_clear( self->private->dnd );
		gtk_clipboard_clear( self->private->primary );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *instance )
{
	static const gchar *thisfn = "nact_clipboard_instance_finalize";
	NactClipboard *self;

	g_return_if_fail( NACT_IS_CLIPBOARD( instance ));

	g_debug( "%s: instance=%p (%s)", thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ));

	self = NACT_CLIPBOARD( instance );

	if( self->private->primary_data ){
		clear_primary_clipboard( self );
		g_free( self->private->primary_data );
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( instance );
	}
}

/**
 * nact_clipboard_new:
 *
 * Returns: a new #NactClipboard object.
 */
NactClipboard *
nact_clipboard_new( NactMainWindow *window )
{
	NactClipboard *clipboard;

	clipboard = g_object_new( NACT_TYPE_CLIPBOARD, NULL );

	clipboard->private->window = window;

	return( clipboard );
}

/**
 * nact_clipboard_dnd_set:
 * @clipboard: this #NactClipboard instance.
 * @rows: the list of row references of dragged items.
 * @folder: the URI of the target folder if any (XDS protocol to outside).
 * @copy_data: %TRUE if data is to be copied, %FALSE else
 *  (only relevant when drag and drop occurs inside of the tree view).
 *
 * Set the selected items into our dnd clipboard.
 */
void
nact_clipboard_dnd_set( NactClipboard *clipboard, guint target, GList *rows, const gchar *folder, gboolean copy_data )
{
	static const gchar *thisfn = "nact_clipboard_dnd_set";
	NactClipboardDndData *data;
	GList *it;

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));
	g_return_if_fail( rows && g_list_length( rows ));

	if( !clipboard->private->dispose_has_run ){

		data = g_new0( NactClipboardDndData, 1 );

		data->target = target;
		data->folder = g_strdup( folder );
		data->rows = NULL;
		data->copy = copy_data;

		for( it = rows ; it ; it = it->next ){
			data->rows = g_list_append(
					data->rows,
					gtk_tree_row_reference_copy(( GtkTreeRowReference * ) it->data ));
		}

		gtk_clipboard_set_with_data( clipboard->private->dnd,
				clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
				( GtkClipboardGetFunc ) get_from_dnd_clipboard_callback,
				( GtkClipboardClearFunc ) clear_dnd_clipboard_callback,
				data );

		g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );
	}
}

/**
 * nact_clipboard_dnd_get_data:
 * @clipboard: this #NactClipboard instance.
 * @copy_data: will be set to the original value of the drag and drop.
 *
 * Returns the list of rows references privously stored.
 *
 * The returned list should be gtk_tree_row_reference_free() by the
 * caller.
 */
GList *
nact_clipboard_dnd_get_data( NactClipboard *clipboard, gboolean *copy_data )
{
	static const gchar *thisfn = "nact_clipboard_dnd_get_data";
	GList *rows = NULL;
	GtkSelectionData *selection;
	NactClipboardDndData *data;
	GList *it;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );

	if( copy_data ){
		*copy_data = FALSE;
	}

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->dnd, NACT_CLIPBOARD_NACT_ATOM );
		if( selection ){
			data = ( NactClipboardDndData * ) gtk_selection_data_get_data( selection );

			if( data->target == NACT_XCHANGE_FORMAT_NACT ){
				for( it = data->rows ; it ; it = it->next ){
					rows = g_list_append( rows,
							gtk_tree_row_reference_copy(( GtkTreeRowReference * ) it->data ));
				}
				*copy_data = data->copy;
			}
		}
		gtk_selection_data_free( selection );
	}

	return( rows );
}

/*
 * Get text/plain from selected actions.
 *
 * This is called when we drop or paste a selection onto an application
 * willing to deal with Xdnd protocol, for text/plain or application/xml
 * mime types.
 *
 * Selected items may include menus, actions and profiles.
 * For now, we only exports actions (and profiles) as XML files.
 *
 * FIXME: na_xml_writer_get_xml_buffer() returns a valid XML document,
 * which includes the <?xml ...?> header. Concatenating several valid
 * XML documents doesn't provide a valid global XML doc, because of
 * the presence of several <?xml ?> headers inside.
 */
gchar *
nact_clipboard_dnd_get_text( NactClipboard *clipboard, GList *rows )
{
	static const gchar *thisfn = "nact_clipboard_dnd_get_text";
	gchar *buffer;

	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );

	g_debug( "%s: clipboard=%p, rows=%p (count=%u)",
			thisfn, ( void * ) clipboard, ( void * ) rows, g_list_length( rows ));

	buffer = NULL;

	if( !clipboard->private->dispose_has_run ){

		buffer = export_rows( clipboard, rows, NULL );
		g_debug( "%s: returning buffer=%p (length=%lu)", thisfn, ( void * ) buffer, g_utf8_strlen( buffer, -1 ));
	}

	return( buffer );
}

/**
 * nact_clipboard_dnd_drag_end:
 * @clipboard: this #NactClipboard instance.
 *
 * On drag-end, exports the objects if needed.
 */
void
nact_clipboard_dnd_drag_end( NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_dnd_drag_end";
	GtkSelectionData *selection;
	NactClipboardDndData *data;
	gchar *buffer;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->dnd, NACT_CLIPBOARD_NACT_ATOM );
		g_debug( "%s: selection=%p", thisfn, ( void * ) selection );

		if( selection ){
			data = ( NactClipboardDndData * ) gtk_selection_data_get_data( selection );
			g_debug( "%s: data=%p (NactClipboardDndData)", thisfn, ( void * ) data );

			if( data->target == NACT_XCHANGE_FORMAT_XDS ){
				g_debug( "%s: folder=%s", thisfn, data->folder );
				buffer = export_rows( clipboard, data->rows, data->folder );
				g_free( buffer );
			}

			gtk_selection_data_free( selection );
		}
	}
}

/**
 * nact_clipboard_dnd_clear:
 * @clipboard: this #NactClipboard instance.
 *
 * Clears the drag-and-drop clipboard.
 *
 * At least called on drag-begin.
 */
void
nact_clipboard_dnd_clear( NactClipboard *clipboard )
{
	g_debug( "nact_clipboard_dnd_clear: clipboard=%p", ( void * ) clipboard );

	gtk_clipboard_clear( clipboard->private->dnd );
}

static void
get_from_dnd_clipboard_callback( GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, guchar *data )
{
	static const gchar *thisfn = "nact_clipboard_get_from_dnd_clipboard_callback";
	GdkAtom selection_data_target;

	selection_data_target = gtk_selection_data_get_target( selection_data );

	g_debug( "%s: clipboard=%p, selection_data=%p, target=%s, info=%d, data=%p",
			thisfn, ( void * ) clipboard,
			( void * ) selection_data, gdk_atom_name( selection_data_target ), info, ( void * ) data );

	gtk_selection_data_set( selection_data,
			selection_data_target, 8, data, sizeof( NactClipboardDndData ));
}

static void
clear_dnd_clipboard_callback( GtkClipboard *clipboard, NactClipboardDndData *data )
{
	static const gchar *thisfn = "nact_clipboard_clear_dnd_clipboard_callback";

	g_debug( "%s: clipboard=%p, data=%p", thisfn, ( void * ) clipboard, ( void * ) data );

	g_free( data->folder );
	g_list_foreach( data->rows, ( GFunc ) gtk_tree_row_reference_free, NULL );
	g_list_free( data->rows );
	g_free( data );
}

/*
 * returns a buffer which contains all exported items if dest_folder is null
 * else export items as files to target directory, returning an empty string
 */
static gchar *
export_rows( NactClipboard *clipboard, GList *rows, const gchar *dest_folder )
{
	static const gchar *thisfn = "nact_clipboard_export_rows";
	GString *data;
	GtkTreeModel *model;
	GList *exported, *irow;
	GtkTreePath *path;
	GtkTreeIter iter;
	FMAObject *object;
	gchar *buffer;
	gboolean first;

	g_debug( "%s: clipboard=%p, rows=%p (count=%d), dest_folder=%s",
			thisfn, ( void * ) clipboard, ( void * ) rows, g_list_length( rows ), dest_folder );

	first = TRUE;
	buffer = NULL;
	exported = NULL;
	data = g_string_new( "" );
	model = gtk_tree_row_reference_get_model(( GtkTreeRowReference * ) rows->data );

	for( irow = rows ; irow ; irow = irow->next ){
		path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) irow->data );
		if( path ){
			gtk_tree_model_get_iter( model, &iter, path );
			gtk_tree_path_free( path );
			gtk_tree_model_get( model, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );
			buffer = export_row_object( clipboard, object, dest_folder, &exported, first );
			if( buffer && strlen( buffer )){
				data = g_string_append( data, buffer );
				g_free( buffer );
			}
			g_object_unref( object );
		}
		first = FALSE;
	}

	g_list_free( exported );
	return( g_string_free( data, FALSE ));
}

static gchar *
export_objects( NactClipboard *clipboard, GList *objects )
{
	gchar *buffer;
	GString *data;
	GList *exported;
	GList *iobj;
	FMAObject *object;
	gboolean first;

	first = TRUE;
	buffer = NULL;
	exported = NULL;
	data = g_string_new( "" );

	for( iobj = objects ; iobj ; iobj = iobj->next ){
		object = FMA_OBJECT( iobj->data );
		buffer = export_row_object( clipboard, object, NULL, &exported, first );
		if( buffer && strlen( buffer )){
			data = g_string_append( data, buffer );
			g_free( buffer );
		}
		g_object_unref( object );
		first = FALSE;
	}

	g_list_free( exported );
	return( g_string_free( data, FALSE ));
}

/*
 * export to a buffer if dest_folder is null, and returns this buffer
 * else export to a new file in the target directory (returning an empty string)
 *
 * exported maintains a list of exported items, so that the same item is not
 * exported twice
 */
static gchar *
export_row_object( NactClipboard *clipboard, FMAObject *object, const gchar *dest_folder, GList **exported, gboolean first )
{
	static const gchar *thisfn = "nact_clipboard_export_row_object";
	GList *subitems, *isub;
	NactApplication *application;
	NAUpdater *updater;
	FMAObjectItem *item;
	gchar *item_label;
	gint index;
	GString *data;
	gchar *buffer;
	gchar *format;
	gchar *fname;
	GSList *msgs;

	data = g_string_new( "" );

	/* if we have a menu, first export the subitems
	 */
	if( FMA_IS_OBJECT_MENU( object )){
		subitems = fma_object_get_items( object );

		for( isub = subitems ; isub ; isub = isub->next ){
			buffer = export_row_object( clipboard, isub->data, dest_folder, exported, first );
			if( buffer && strlen( buffer )){
				data = g_string_append( data, buffer );
				g_free( buffer );
			}
			first = FALSE;
		}
	}

	/* only export FMAObjectItem type
	 * here, object may be a menu, an action or a profile
	 */
	msgs = NULL;
	item = ( FMAObjectItem * ) object;
	if( FMA_IS_OBJECT_PROFILE( object )){
		item = FMA_OBJECT_ITEM( fma_object_get_parent( object ));
	}

	application = NACT_APPLICATION(
			gtk_window_get_application( GTK_WINDOW( clipboard->private->window )));
	updater = nact_application_get_updater( application );

	index = g_list_index( *exported, ( gconstpointer ) item );
	if( index == -1 ){

		item_label = fma_object_get_label( item );
		g_debug( "%s: exporting %s", thisfn, item_label );
		g_free( item_label );

		*exported = g_list_prepend( *exported, ( gpointer ) item );
		format = fma_settings_get_string( IPREFS_EXPORT_PREFERRED_FORMAT, NULL, NULL );
		g_return_val_if_fail( format && strlen( format ), NULL );

		if( !strcmp( format, EXPORTER_FORMAT_ASK )){
			g_free( format );
			format = nact_export_ask_user( item, first );
			g_return_val_if_fail( format && strlen( format ), NULL );
		}

		if( strcmp( format, EXPORTER_FORMAT_NOEXPORT ) != 0 ){
			if( dest_folder ){
				fname = fma_exporter_to_file( FMA_PIVOT( updater ), item, dest_folder, format, &msgs );
				g_free( fname );

			} else {
				buffer = fma_exporter_to_buffer( FMA_PIVOT( updater ), item, format, &msgs );
				if( buffer && strlen( buffer )){
					data = g_string_append( data, buffer );
					g_free( buffer );
				}
			}
		}

		g_free( format );
	}

	return( g_string_free( data, FALSE ));
}

/**
 * nact_clipboard_primary_set:
 * @clipboard: this #NactClipboard object.
 * @items: a list of #FMAObject items
 * @mode: where do these items come from ?
 *  Or what is the operation which has led the items to the clipboard?
 *
 * Installs a copy of provided items in the clipboard.
 *
 * Rationale: when cutting an item to the clipboard, the next paste
 * will keep its same original id, and it is safe because this is
 * actually what we want when we cut/paste.
 *
 * Contrarily, when we copy/paste, we are waiting for a new element
 * which has the same characteristics that the previous one ; we so
 * have to renumber actions/menus items when copying into the clipboard.
 *
 * Note that we use FMAIDuplicable interface without actually taking care
 * of what is origin or so, as origin will be reinitialized when getting
 * data out of the clipboard.
 */
void
nact_clipboard_primary_set( NactClipboard *clipboard, GList *items, gint mode )
{
	static const gchar *thisfn = "nact_clipboard_primary_set";
	PrimaryData *user_data;
	GList *it;

	g_debug( "%s: clipboard=%p, items=%p (count=%d), mode=%d",
			thisfn, ( void * ) clipboard, ( void * ) items, g_list_length( items ), mode );
	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		user_data = clipboard->private->primary_data;

		if( user_data == NULL ){
			user_data = g_new0( PrimaryData, 1 );
			clipboard->private->primary_data = user_data;
			g_debug( "%s: allocating PrimaryData=%p", thisfn, ( void * ) user_data );

		} else {
			clear_primary_clipboard( clipboard );
		}

		fma_object_item_count_items( items,
				( gint * ) &user_data->nb_menus,
				( gint * ) &user_data->nb_actions,
				( gint * ) &user_data->nb_profiles,
				FALSE );

		for( it = items ; it ; it = it->next ){
			user_data->items =
					g_list_prepend( user_data->items, fma_object_duplicate( it->data, DUPLICATE_REC ));
		}
		user_data->items = g_list_reverse( user_data->items );

		user_data->mode = mode;

		gtk_clipboard_set_with_data( clipboard->private->primary,
				clipboard_formats, G_N_ELEMENTS( clipboard_formats ),
				( GtkClipboardGetFunc ) get_from_primary_clipboard_callback,
				( GtkClipboardClearFunc ) clear_primary_clipboard_callback,
				clipboard );

		clipboard->private->primary_got = FALSE;
	}
}

/**
 * nact_clipboard_primary_get:
 * @clipboard: this #NactClipboard object.
 *
 * Returns: a copy of the list of items previously referenced in the
 * internal clipboard.
 *
 * We allocate a new id for items in order to be ready to paste another
 * time.
 */
GList *
nact_clipboard_primary_get( NactClipboard *clipboard, gboolean *relabel )
{
	static const gchar *thisfn = "nact_clipboard_primary_get";
	GList *items = NULL;
	GtkSelectionData *selection;
	PrimaryData *user_data;
	GList *it;
	FMAObject *obj;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );
	g_return_val_if_fail( NACT_IS_CLIPBOARD( clipboard ), NULL );
	g_return_val_if_fail( relabel, NULL );

	if( !clipboard->private->dispose_has_run ){

		selection = gtk_clipboard_wait_for_contents( clipboard->private->primary, NACT_CLIPBOARD_NACT_ATOM );

		if( selection ){
			user_data = ( PrimaryData * ) gtk_selection_data_get_data( selection );
			g_debug( "%s: retrieving PrimaryData=%p", thisfn, ( void * ) user_data );

			if( user_data ){
				for( it = user_data->items ; it ; it = it->next ){
					obj = FMA_OBJECT( fma_object_duplicate( it->data, DUPLICATE_REC ));
					fma_object_set_origin( obj, NULL );
					items = g_list_prepend( items, obj );
				}
				items = g_list_reverse( items );

				*relabel = (( user_data->mode == CLIPBOARD_MODE_CUT && clipboard->private->primary_got ) ||
								user_data->mode == CLIPBOARD_MODE_COPY );

				clipboard->private->primary_got = TRUE;
			}

			gtk_selection_data_free( selection );
		}
	}

	return( items );
}

/**
 * nact_clipboard_primary_counts:
 * @clipboard: this #NactClipboard object.
 *
 * Returns some counters on content of primary clipboard.
 */
void
nact_clipboard_primary_counts( NactClipboard *clipboard, guint *actions, guint *profiles, guint *menus )
{
	PrimaryData *user_data;

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));
	g_return_if_fail( actions && profiles && menus );

	if( !clipboard->private->dispose_has_run ){

		*actions = 0;
		*profiles = 0;
		*menus = 0;

		user_data = clipboard->private->primary_data;
		if( user_data ){

			*actions = user_data->nb_actions;
			*profiles = user_data->nb_profiles;
			*menus = user_data->nb_menus;
		}
	}
}

static void
get_from_primary_clipboard_callback( GtkClipboard *gtk_clipboard, GtkSelectionData *selection_data, guint info, NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_get_from_primary_clipboard_callback";
	PrimaryData *user_data;
	gchar *buffer;
	GdkAtom selection_data_target;

	selection_data_target = gtk_selection_data_get_target( selection_data );

	g_debug( "%s: gtk_clipboard=%p, selection_data=%p, target=%s, info=%d, clipboard=%p",
			thisfn, ( void * ) gtk_clipboard,
			( void * ) selection_data, gdk_atom_name( selection_data_target ), info, ( void * ) clipboard );

	user_data = clipboard->private->primary_data;

	if( info == NACT_CLIPBOARD_FORMAT_TEXT_PLAIN ){
		buffer = export_objects( clipboard, user_data->items );
		gtk_selection_data_set( selection_data,
				selection_data_target, 8, ( const guchar * ) buffer, strlen( buffer ));
		g_free( buffer );

	} else {
		gtk_selection_data_set( selection_data,
				selection_data_target, 8, ( const guchar * ) user_data, sizeof( PrimaryData ));
	}
}

static void
clear_primary_clipboard( NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_clear_primary_clipboard";
	PrimaryData *user_data;

	g_debug( "%s: clipboard=%p", thisfn, ( void * ) clipboard );

	user_data = clipboard->private->primary_data;
	g_return_if_fail( user_data != NULL );

	g_list_foreach( user_data->items, ( GFunc ) g_object_unref, NULL );
	g_list_free( user_data->items );
	user_data->items = NULL;
	user_data->nb_menus = 0;
	user_data->nb_actions = 0;
	user_data->nb_profiles = 0;

	clipboard->private->primary_got = FALSE;
}

static void
clear_primary_clipboard_callback( GtkClipboard *gtk_clipboard, NactClipboard *clipboard )
{
}

/**
 * nact_clipboard_dump:
 * @clipboard: this #NactClipboard instance.
 *
 * Dumps the content of the primary clipboard.
 */
void
nact_clipboard_dump( NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_dump";

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		g_debug( "%s:       window=%p (%s)", thisfn, ( void * ) clipboard->private->window, G_OBJECT_TYPE_NAME( clipboard->private->window ));
		g_debug( "%s:          dnd=%p", thisfn, ( void * ) clipboard->private->dnd );
		g_debug( "%s:      primary=%p", thisfn, ( void * ) clipboard->private->primary );
		g_debug( "%s: primary_data=%p", thisfn, ( void * ) clipboard->private->primary_data );

		if( clipboard->private->primary_data ){
			dump_primary_clipboard( clipboard );
		}
	}
}

static void
dump_primary_clipboard( NactClipboard *clipboard )
{
	static const gchar *thisfn = "nact_clipboard_dump_primary";
	PrimaryData *user_data;
	gchar *mode;
	GList *it;

	g_return_if_fail( NACT_IS_CLIPBOARD( clipboard ));

	if( !clipboard->private->dispose_has_run ){

		user_data = clipboard->private->primary_data;

		if( user_data ){
			g_debug( "%s:           user_data->nb_actions=%d", thisfn, user_data->nb_actions );
			g_debug( "%s:          user_data->nb_profiles=%d", thisfn, user_data->nb_profiles );
			g_debug( "%s:             user_data->nb_menus=%d", thisfn, user_data->nb_menus );
			g_debug( "%s:                user_data->items=%p (count=%d)",
					thisfn,
					( void * ) user_data->items,
					user_data->items ? g_list_length( user_data->items ) : 0 );
			mode = clipboard_mode_to_string( user_data->mode );
			g_debug( "%s:                 user_data->mode=%d (%s)", thisfn, user_data->mode, mode );
			g_free( mode );
			for( it = user_data->items ; it ; it = it->next ){
				fma_object_object_dump( FMA_OBJECT( it->data ));
			}
		}

		g_debug( "%s: clipboard->private->primary_got=%s", thisfn, clipboard->private->primary_got ? "True":"False" );
	}
}

static gchar *
clipboard_mode_to_string( gint mode )
{
	gchar *mode_str;

	switch( mode ){
		case CLIPBOARD_MODE_CUT:
			mode_str = g_strdup( "CutMode" );
			break;

		case CLIPBOARD_MODE_COPY:
			mode_str = g_strdup( "CopyMode" );
			break;

		default:
			mode_str = g_strdup( "unknown mode" );
			break;
	}

	return( mode_str );
}
