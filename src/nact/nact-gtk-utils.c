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

#include <glib.h>
#include <string.h>

#include <core/na-iprefs.h>
#include <core/na-updater.h>

#include "base-iprefs.h"
#include "nact-iprefs.h"
#include "nact-gtk-utils.h"
#include "nact-application.h"

/**
 * nact_gtk_utils_set_editable:
 * @widget: the #GtkWdiget.
 * @editable: whether the @widget is editable or not.
 *
 * Try to set a visual indication of whether the @widget is editable or not.
 */
void
nact_gtk_utils_set_editable( GtkObject *widget, gboolean editable )
{
	GList *renderers, *irender;
	GtkTextBuffer *buffer;
	GtkTextTag *tag;

	if( GTK_IS_ENTRY( widget )){
		gtk_editable_set_editable( GTK_EDITABLE( widget ), editable );
		/* removing the frame leads to a disturbing modification of the
		 * height of the control */
		/*g_object_set( G_OBJECT( widget ), "has-frame", editable, NULL );*/
		/* this prevents the caret to be displayed when we click in the entry */
		g_object_set( G_OBJECT( widget ), "can-focus", editable, NULL );

	} else if( GTK_IS_TEXT_VIEW( widget )){
		buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( widget ));
		tag = gtk_text_buffer_create_tag( buffer,
				"nact-tag",
				"editable", editable,
				"editable-set", TRUE,
				NULL );

	} else if( GTK_IS_TOGGLE_BUTTON( widget )){
		/* transforms to a quasi standard GtkButton */
		/*g_object_set( G_OBJECT( widget ), "draw-indicator", editable, NULL );*/
		/* this at least prevent the keyboard focus to go to the button
		 * (which is better than nothing) */
		g_object_set( G_OBJECT( widget ), "can-focus", editable, NULL );

	} else if( GTK_IS_BUTTON( widget )){
		gtk_widget_set_sensitive( GTK_WIDGET( widget ), editable );

	} else if( GTK_IS_COMBO_BOX_ENTRY( widget )){
		/* idem as GtkEntry */
		gtk_editable_set_editable( GTK_EDITABLE( gtk_bin_get_child( GTK_BIN( widget ))), editable );
		g_object_set( G_OBJECT( gtk_bin_get_child( GTK_BIN( widget ))), "can-focus", editable, NULL );
		/* disable the listbox button itself */
		gtk_combo_box_set_button_sensitivity( GTK_COMBO_BOX( widget ), editable ? GTK_SENSITIVITY_ON : GTK_SENSITIVITY_OFF );

	} else if( GTK_IS_TREE_VIEW_COLUMN( widget )){
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( GTK_TREE_VIEW_COLUMN( widget )));
		for( irender = renderers ; irender ; irender = irender->next ){
			if( GTK_IS_CELL_RENDERER_TEXT( irender->data )){
				g_object_set( G_OBJECT( irender->data ), "editable", editable, "editable-set", TRUE, NULL );
			}
		}
		g_list_free( renderers );
	}
}

/**
 * nact_utils_get_pixbuf:
 * @name: the name of the file or an icon.
 * widget: the widget on which the imagecshould be rendered.
 * size: the desired size.
 *
 * Returns a pixbuf for the given widget.
 */
GdkPixbuf *
nact_gtk_utils_get_pixbuf( const gchar *name, GtkWidget *widget, gint size )
{
	static const gchar *thisfn = "nact_gtk_utils_get_pixbuf";
	GdkPixbuf* pixbuf;
	GError *error;

	error = NULL;
	pixbuf = NULL;

	if( name && strlen( name )){
		if( g_path_is_absolute( name )){
			pixbuf = gdk_pixbuf_new_from_file_at_size( name, size, size, &error );
			if( error ){
				if( error->code == G_FILE_ERROR_NOENT ){
					g_debug( "%s: gdk_pixbuf_new_from_file_at_size: name=%s, error=%s", thisfn, name, error->message );
				} else {
					g_warning( "%s: gdk_pixbuf_new_from_file_at_size: name=%s, error=%s", thisfn, name, error->message );
				}
				g_error_free( error );
				error = NULL;
				pixbuf = NULL;
			}

		} else {
			pixbuf = gtk_widget_render_icon( widget, name, size, NULL );
		}
	}

	if( !pixbuf ){
		g_debug( "%s: null pixbuf, loading transparent image", thisfn );
		pixbuf = gdk_pixbuf_new_from_file_at_size( PKGDATADIR "/transparent.png", size, size, NULL );
	}

	return( pixbuf );
}

/**
 * nact_utils_render:
 * @name: the name of the file or an icon.
 * widget: the widget on which the image should be rendered.
 * size: the desired size.
 *
 * Displays the (maybe themed) image on the given widget.
 */
void
nact_gtk_utils_render( const gchar *name, GtkImage *widget, gint size )
{
	GdkPixbuf* pixbuf;

	pixbuf = nact_gtk_utils_get_pixbuf( name, GTK_WIDGET( widget ), size );

	if( pixbuf ){
		gtk_image_set_from_pixbuf( widget, pixbuf );
		g_object_unref( pixbuf );
	}
}

/**
 * nact_gtk_utils_select_file:
 * @window: the #BaseWindow which will be the parent of the dialog box.
 * @title: the title of the dialog box.
 * @dialog_name: the name of the dialog box in Preferences to read/write
 *  its size and position.
 * @entry: the #GtkEntry which is associated with the selected file.
 * @entry_name: the name of the entry in Preferences to be readen/written.
 * @default_dir_uri: the URI of the directory which should be set in there is
 *  not yet any preference (see @entry_name)
 *
 * Opens a #GtkFileChooserDialog and let the user choose an existing file
 * -> choose and display an existing file name
 * -> record the dirname URI.
 *
 * If the user validates its selection, the choosen file pathname will be
 * written in the @entry #GtkEntry, while the corresponding dirname
 * URI will be written as @entry_name in Preferences.
 */
void
nact_gtk_utils_select_file( BaseWindow *window,
				const gchar *title, const gchar *dialog_name,
				GtkWidget *entry, const gchar *entry_name,
				const gchar *default_dir_uri )
{
	NactApplication *application;
	NAUpdater *updater;
	GtkWindow *toplevel;
	GtkWidget *dialog;
	const gchar *text;
	gchar *filename, *uri;

	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	toplevel = base_window_get_toplevel( window );

	dialog = gtk_file_chooser_dialog_new(
			title,
			toplevel,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	base_iprefs_position_named_window( window, GTK_WINDOW( dialog ), dialog_name );

	text = gtk_entry_get_text( GTK_ENTRY( entry ));

	if( text && strlen( text )){
		gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), text );

	} else {
		uri = na_iprefs_read_string( NA_IPREFS( updater ), entry_name, default_dir_uri );
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
		g_free( uri );
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( entry ), filename );
	    g_free( filename );
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	nact_iprefs_write_string( window, entry_name, uri );
	g_free( uri );

	base_iprefs_save_named_window_position( window, GTK_WINDOW( dialog ), dialog_name );

	gtk_widget_destroy( dialog );
}

/**
 * nact_gtk_utils_select_dir:
 * @window: the #BaseWindow which will be the parent of the dialog box.
 * @title: the title of the dialog box.
 * @dialog_name: the name of the dialog box in Preferences to read/write
 *  its size and position.
 * @entry: the #GtkEntry which is associated with the field.
 * @entry_name: the name of the entry in Preferences to be readen/written.
 * @default_dir_uri: the URI of the directory which should be set in there is
 *  not yet any preference (see @entry_name)
 *
 * Opens a #GtkFileChooserDialog and let the user choose an existing directory
 * -> choose and display an existing dir name
 * -> record the dirname URI of this dir name.
 *
 * If the user validates its selection, the choosen file pathname will be
 * written in the @entry #GtkEntry, while the corresponding dirname
 * URI will be written as @entry_name in Preferences.
 */
void
nact_gtk_utils_select_dir( BaseWindow *window,
				const gchar *title, const gchar *dialog_name,
				GtkWidget *entry, const gchar *entry_name,
				const gchar *default_dir_uri )
{
	NactApplication *application;
	NAUpdater *updater;
	GtkWindow *toplevel;
	GtkWidget *dialog;
	const gchar *text;
	gchar *dir, *uri;

	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	toplevel = base_window_get_toplevel( window );

	dialog = gtk_file_chooser_dialog_new(
			title,
			toplevel,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	base_iprefs_position_named_window( window, GTK_WINDOW( dialog ), dialog_name );

	text = gtk_entry_get_text( GTK_ENTRY( entry ));

	if( text && strlen( text )){
		gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), text );

	} else {
		uri = na_iprefs_read_string( NA_IPREFS( updater ), entry_name, default_dir_uri );
		gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
		g_free( uri );
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		dir = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( entry ), dir );
	    g_free( dir );
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	nact_iprefs_write_string( window, entry_name, uri );
	g_free( uri );

	base_iprefs_save_named_window_position( window, GTK_WINDOW( dialog ), dialog_name );

	gtk_widget_destroy( dialog );
}
