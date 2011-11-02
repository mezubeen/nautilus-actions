/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <string.h>

#include <core/na-iprefs.h>
#include <core/na-updater.h>

#include "base-gtk-utils.h"

#define NACT_PROP_TOGGLE_BUTTON				"nact-prop-toggle-button"
#define NACT_PROP_TOGGLE_HANDLER			"nact-prop-toggle-handler"
#define NACT_PROP_TOGGLE_USER_DATA			"nact-prop-toggle-user-data"

#define DEFAULT_WIDTH		22
#define DEFAULT_HEIGHT		22

static void        int_list_to_position( const BaseWindow *window, GList *list, gint *x, gint *y, gint *width, gint *height );
static GList      *position_to_int_list( const BaseWindow *window, gint x, gint y, gint width, gint height );
static void        free_int_list( GList *list );

/**
 * base_gtk_utils_position_window:
 * @window: this #BaseWindow-derived window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Position the specified window on the screen.
 *
 * A window position is stored as a list of integers "x,y,width,height".
 */
void
base_gtk_utils_restore_window_position( const BaseWindow *window, const gchar *wsp_name )
{
	static const gchar *thisfn = "base_gtk_utils_restore_window_position";
	GtkWindow *toplevel;
	GList *list;
	gint x=0, y=0, width=0, height=0;
	GdkDisplay *display;
	GdkScreen *screen;
	gint screen_width, screen_height;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	toplevel = base_window_get_gtk_toplevel( window );

	g_debug( "%s: window=%p (%s), toplevel=%p (%s), wsp_name=%s",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ),
			( void * ) toplevel, G_OBJECT_TYPE_NAME( toplevel ), wsp_name );

	list = na_settings_get_uint_list( wsp_name, NULL, NULL );

	if( list ){
		int_list_to_position( window, list, &x, &y, &width, &height );
		g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );
		free_int_list( list );
	}

	if( width > 0 && height > 0 ){
		display = gdk_display_get_default();
		screen = gdk_display_get_screen( display, 0 );
		screen_width = gdk_screen_get_width( screen );
		screen_height = gdk_screen_get_height( screen );

		if(( x+width < screen_width ) && ( y+height < screen_height )){
			gtk_window_move( toplevel, x, y );
			gtk_window_resize( toplevel, width, height );
		}
	}
}

/**
 * base_gtk_utils_save_window_position:
 * @window: this #BaseWindow-derived window.
 * @wsp_name: the string which handles the window size and position in user preferences.
 *
 * Save the size and position of the specified window.
 */
void
base_gtk_utils_save_window_position( const BaseWindow *window, const gchar *wsp_name )
{
	static const gchar *thisfn = "base_gtk_utils_save_window_position";
	GtkWindow *toplevel;
	gint x, y, width, height;
	GList *list;

	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( wsp_name && strlen( wsp_name ));

	toplevel = base_window_get_gtk_toplevel( window );
	g_return_if_fail( GTK_IS_WINDOW( toplevel ));

	gtk_window_get_position( toplevel, &x, &y );
	gtk_window_get_size( toplevel, &width, &height );
	g_debug( "%s: wsp_name=%s, x=%d, y=%d, width=%d, height=%d", thisfn, wsp_name, x, y, width, height );

	list = position_to_int_list( window, x, y, width, height );
	na_settings_set_uint_list( wsp_name, list );
	free_int_list( list );
}

/*
 * extract the position of the window from the list of unsigned integers
 */
static void
int_list_to_position( const BaseWindow *window, GList *list, gint *x, gint *y, gint *width, gint *height )
{
	GList *it;
	int i;

	g_assert( x );
	g_assert( y );
	g_assert( width );
	g_assert( height );

	for( it=list, i=0 ; it ; it=it->next, i+=1 ){
		switch( i ){
			case 0:
				*x = GPOINTER_TO_UINT( it->data );
				break;
			case 1:
				*y = GPOINTER_TO_UINT( it->data );
				break;
			case 2:
				*width = GPOINTER_TO_UINT( it->data );
				break;
			case 3:
				*height = GPOINTER_TO_UINT( it->data );
				break;
		}
	}
}

static GList *
position_to_int_list( const BaseWindow *window, gint x, gint y, gint width, gint height )
{
	GList *list = NULL;

	list = g_list_append( list, GUINT_TO_POINTER( x ));
	list = g_list_append( list, GUINT_TO_POINTER( y ));
	list = g_list_append( list, GUINT_TO_POINTER( width ));
	list = g_list_append( list, GUINT_TO_POINTER( height ));

	return( list );
}

/*
 * free the list of int
 */
static void
free_int_list( GList *list )
{
	g_list_free( list );
}

/**
 * base_gtk_utils_set_editable:
 * @widget: the #GtkWdiget.
 * @editable: whether the @widget is editable or not.
 *
 * Try to set a visual indication of whether the @widget is editable or not.
 *
 * Having a GtkWidget should be enough, but we also deal with a GtkTreeViewColumn.
 * So the most-bottom common ancestor is just GObject (since GtkObject having been
 * deprecated in Gtk+-3.0)
 */
void
base_gtk_utils_set_editable( GObject *widget, gboolean editable )
{
	GList *renderers, *irender;

/* GtkComboBoxEntry is deprecated from Gtk+3
 * see. http://git.gnome.org/browse/gtk+/commit/?id=9612c648176378bf237ad0e1a8c6c995b0ca7c61
 * while 'has_entry' property exists since 2.24
 */
#if GTK_CHECK_VERSION( 2, 24, 0 )
	if( GTK_IS_COMBO_BOX( widget ) && gtk_combo_box_get_has_entry( GTK_COMBO_BOX( widget ))){
#else
	if( GTK_IS_COMBO_BOX_ENTRY( widget )){
#endif
		/* idem as GtkEntry */
		gtk_editable_set_editable( GTK_EDITABLE( gtk_bin_get_child( GTK_BIN( widget ))), editable );
		g_object_set( G_OBJECT( gtk_bin_get_child( GTK_BIN( widget ))), "can-focus", editable, NULL );
		/* disable the listbox button itself */
		gtk_combo_box_set_button_sensitivity( GTK_COMBO_BOX( widget ), editable ? GTK_SENSITIVITY_ON : GTK_SENSITIVITY_OFF );

	} else if( GTK_IS_COMBO_BOX( widget )){
		/* disable the listbox button itself */
		gtk_combo_box_set_button_sensitivity( GTK_COMBO_BOX( widget ), editable ? GTK_SENSITIVITY_ON : GTK_SENSITIVITY_OFF );

	} else if( GTK_IS_ENTRY( widget )){
		gtk_editable_set_editable( GTK_EDITABLE( widget ), editable );
		/* removing the frame leads to a disturbing modification of the
		 * height of the control */
		/*g_object_set( G_OBJECT( widget ), "has-frame", editable, NULL );*/
		/* this prevents the caret to be displayed when we click in the entry */
		g_object_set( G_OBJECT( widget ), "can-focus", editable, NULL );

	} else if( GTK_IS_TEXT_VIEW( widget )){
		g_object_set( G_OBJECT( widget ), "can-focus", editable, NULL );
		gtk_text_view_set_editable( GTK_TEXT_VIEW( widget ), editable );

	} else if( GTK_IS_TOGGLE_BUTTON( widget )){
		/* transforms to a quasi standard GtkButton */
		/*g_object_set( G_OBJECT( widget ), "draw-indicator", editable, NULL );*/
		/* this at least prevent the keyboard focus to go to the button
		 * (which is better than nothing) */
		g_object_set( G_OBJECT( widget ), "can-focus", editable, NULL );

	} else if( GTK_IS_TREE_VIEW_COLUMN( widget )){
		renderers = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( GTK_TREE_VIEW_COLUMN( widget )));
		for( irender = renderers ; irender ; irender = irender->next ){
			if( GTK_IS_CELL_RENDERER_TEXT( irender->data )){
				g_object_set( G_OBJECT( irender->data ), "editable", editable, "editable-set", TRUE, NULL );
			}
		}
		g_list_free( renderers );

	} else if( GTK_IS_BUTTON( widget )){
		gtk_widget_set_sensitive( GTK_WIDGET( widget ), editable );
	}
}

/**
 * base_gtk_utils_radio_set_initial_state:
 * @button: the #GtkRadioButton button which is initially active.
 * @handler: the corresponding "toggled" handler.
 * @user_data: the user data associated to the handler.
 * @editable: whether this radio button group is editable.
 * @sensitive: whether this radio button group is sensitive.
 *
 * This function should be called for the button which is initially active
 * inside of a radio button group when the radio group may happen to not be
 * editable.
 * This function should be called only once for the radio button group.
 *
 * It does the following operations:
 * - set the button as active
 * - set other buttons of the radio button group as inactive
 * - set all buttons of radio button group as @editable
 *
 * The initially active @button, along with its @handler, are recorded
 * as properties of the radio button group (actually as properties of each
 * radio button of the group), so that they can later be used to reset the
 * initial state.
 */
void
base_gtk_utils_radio_set_initial_state( GtkRadioButton *button,
		GCallback handler, void *user_data, gboolean editable, gboolean sensitive )
{
	GSList *group, *ig;
	GtkRadioButton *other;

	group = gtk_radio_button_get_group( button );

	for( ig = group ; ig ; ig = ig->next ){
		other = GTK_RADIO_BUTTON( ig->data );
		g_object_set_data( G_OBJECT( other ), NACT_PROP_TOGGLE_BUTTON, button );
		g_object_set_data( G_OBJECT( other ), NACT_PROP_TOGGLE_HANDLER, handler );
		g_object_set_data( G_OBJECT( other ), NACT_PROP_TOGGLE_USER_DATA, user_data );
		g_object_set_data( G_OBJECT( other ), NACT_PROP_TOGGLE_EDITABLE, GUINT_TO_POINTER( editable ));
		base_gtk_utils_set_editable( G_OBJECT( other ), editable );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( other ), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET( other ), sensitive );
	}

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );
}

/**
 * base_gtk_utils_radio_reset_initial_state:
 * @button: the #GtkRadioButton being toggled.
 * @handler: the corresponding "toggled" handler.
 * @data: data associated with the @handler callback.
 *
 * When clicking on a read-only radio button, this function ensures that
 * the radio button is not modified. It may be called whether the radio
 * button group is editable or not (does nothing if group is actually
 * editable).
 */
void
base_gtk_utils_radio_reset_initial_state( GtkRadioButton *button, GCallback handler )
{
	GtkToggleButton *initial_button;
	GCallback initial_handler;
	gboolean active;
	gboolean editable;
	gpointer user_data;

	active = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( active && !editable ){
		initial_button = GTK_TOGGLE_BUTTON( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_BUTTON ));
		initial_handler = G_CALLBACK( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_HANDLER ));
		user_data = g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_USER_DATA );

		if( handler ){
			g_signal_handlers_block_by_func(( gpointer ) button, handler, user_data );
		}
		g_signal_handlers_block_by_func(( gpointer ) initial_button, initial_handler, user_data );

		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), FALSE );
		gtk_toggle_button_set_active( initial_button, TRUE );

		g_signal_handlers_unblock_by_func(( gpointer ) initial_button, initial_handler, user_data );
		if( handler ){
			g_signal_handlers_unblock_by_func(( gpointer ) button, handler, user_data );
		}
	}
}

/**
 * base_gtk_utils_toggle_set_initial_state:
 * @button: the #GtkToggleButton button.
 * @handler: the corresponding "toggled" handler.
 * @window: the toplevel #BaseWindow which embeds the button;
 *  it will be passed as user_data when connecting the signal.
 * @active: whether the check button is initially active (checked).
 * @editable: whether this radio button group is editable.
 * @sensitive: whether this radio button group is sensitive.
 *
 * This function should be called for a check button which may happen to be
 * read-only..
 *
 * It does the following operations:
 * - connect the 'toggled' handler to the button
 * - set the button as active or inactive depending of @active
 * - set the button as editable or not depending of @editable
 * - set the button as sensitive or not depending of @sensitive
 * - explictely triggers the 'toggled' handler
 */
void
base_gtk_utils_toggle_set_initial_state( BaseWindow *window,
		const gchar *button_name, GCallback handler,
		gboolean active, gboolean editable, gboolean sensitive )
{
	typedef void ( *toggle_handler )( GtkToggleButton *, BaseWindow * );
	GtkToggleButton *button;

	button = GTK_TOGGLE_BUTTON( base_window_get_widget( window, button_name ));

	if( button ){
		base_window_signal_connect( window, G_OBJECT( button ), "toggled", handler );

		g_object_set_data( G_OBJECT( button ), NACT_PROP_TOGGLE_HANDLER, handler );
		g_object_set_data( G_OBJECT( button ), NACT_PROP_TOGGLE_USER_DATA, window );
		g_object_set_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE, GUINT_TO_POINTER( editable ));

		base_gtk_utils_set_editable( G_OBJECT( button ), editable );
		gtk_widget_set_sensitive( GTK_WIDGET( button ), sensitive );
		gtk_toggle_button_set_active( button, active );

		( *( toggle_handler ) handler )( button, window );
	}
}

/**
 * base_gtk_utils_toggle_reset_initial_state:
 * @button: the #GtkToggleButton check button.
 *
 * When clicking on a read-only check button, this function ensures that
 * the check button is not modified. It may be called whether the button
 * is editable or not (does nothing if button is actually editable).
 */
void
base_gtk_utils_toggle_reset_initial_state( GtkToggleButton *button )
{
	gboolean editable;
	GCallback handler;
	gpointer user_data;
	gboolean active;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( !editable ){
		active = gtk_toggle_button_get_active( button );
		handler = G_CALLBACK( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_HANDLER ));
		user_data = g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_USER_DATA );

		g_signal_handlers_block_by_func(( gpointer ) button, handler, user_data );
		gtk_toggle_button_set_active( button, !active );
		g_signal_handlers_unblock_by_func(( gpointer ) button, handler, user_data );
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
base_gtk_utils_get_pixbuf( const gchar *name, GtkWidget *widget, GtkIconSize size )
{
	static const gchar *thisfn = "base_gtk_utils_get_pixbuf";
	GdkPixbuf* pixbuf;
	GError *error;
	gint width, height;
	GtkIconTheme *icon_theme;

	error = NULL;
	pixbuf = NULL;

	if( !gtk_icon_size_lookup( size, &width, &height )){
		width = DEFAULT_WIDTH;
		height = DEFAULT_HEIGHT;
	}

	if( name && strlen( name )){
		if( g_path_is_absolute( name )){
			pixbuf = gdk_pixbuf_new_from_file_at_size( name, width, height, &error );
			if( error ){
				if( error->code != G_FILE_ERROR_NOENT ){
					g_warning( "%s: gdk_pixbuf_new_from_file_at_size: name=%s, error=%s (%d)", thisfn, name, error->message, error->code );
				}
				g_error_free( error );
				error = NULL;
				pixbuf = NULL;
			}

		} else {
/* gtk_widget_render_icon() is deprecated since Gtk+ 3.0
 * see http://library.gnome.org/devel/gtk/unstable/GtkWidget.html#gtk-widget-render-icon
 * and http://git.gnome.org/browse/gtk+/commit/?id=07eeae15825403037b7df139acf9bfa104d5559d
 */
#if GTK_CHECK_VERSION( 2, 91, 7 )
			pixbuf = gtk_widget_render_icon_pixbuf( widget, name, size );
#else
			pixbuf = gtk_widget_render_icon( widget, name, size, NULL );
#endif
			if( !pixbuf ){
				icon_theme = gtk_icon_theme_get_default();
				pixbuf = gtk_icon_theme_load_icon(
								icon_theme, name, width, GTK_ICON_LOOKUP_GENERIC_FALLBACK, &error );
				if( error ){
					/* it happens that the message "Icon 'xxxx' not present in theme"
					 * is generated with a domain of 'gtk-icon-theme-error-quark' and
					 * an error code of zero - it seems difficult to just test zero
					 * so does not display warning, but just debug
					 */
					g_debug( "%s: %s (%s:%d)",
							thisfn, error->message, g_quark_to_string( error->domain ), error->code );
					g_error_free( error );
				}
			}
		}
	}

	if( !pixbuf ){
		g_debug( "%s: null pixbuf, loading transparent image", thisfn );
		pixbuf = gdk_pixbuf_new_from_file_at_size( PKGDATADIR "/transparent.png", width, height, NULL );
	}

	return( pixbuf );
}

/**
 * nact_utils_render:
 * @name: the name of the file or an icon, or %NULL.
 * widget: the widget on which the image should be rendered.
 * size: the desired size.
 *
 * Displays the (maybe themed) image on the given widget.
 */
void
base_gtk_utils_render( const gchar *name, GtkImage *widget, GtkIconSize size )
{
	static const gchar *thisfn = "base_gtk_utils_render";
	GdkPixbuf* pixbuf;
	gint width, height;

	g_debug( "%s: name=%s, widget=%p, size=%d", thisfn, name, ( void * ) widget, size );

	if( name ){
		pixbuf = base_gtk_utils_get_pixbuf( name, GTK_WIDGET( widget ), size );

	} else {
		if( !gtk_icon_size_lookup( size, &width, &height )){
			width = DEFAULT_WIDTH;
			height = DEFAULT_HEIGHT;
		}
		pixbuf = gdk_pixbuf_new_from_file_at_size( PKGDATADIR "/transparent.png", width, height, NULL );
	}

	if( pixbuf ){
		gtk_image_set_from_pixbuf( widget, pixbuf );
		g_object_unref( pixbuf );
	}
}

/**
 * base_gtk_utils_select_file:
 * @window: the #BaseWindow which will be the parent of the dialog box.
 * @title: the title of the dialog box.
 * @wsp_name: the name of the dialog box in Preferences to read/write
 *  its size and position.
 * @entry: the #GtkEntry which is associated with the selected file.
 * @entry_name: the name of the entry in Preferences to be read/written.
 *
 * Opens a #GtkFileChooserDialog and let the user choose an existing file
 * -> choose and display an existing file name
 * -> record the dirname URI.
 *
 * If the user validates its selection, the chosen file pathname will be
 * written in the @entry #GtkEntry, while the corresponding dirname
 * URI will be written as @entry_name in Preferences.
 */
void
base_gtk_utils_select_file( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name )
{
	base_gtk_utils_select_file_with_preview(
			window, title, wsp_name, entry, entry_name, NULL );
}

/**
 * base_gtk_utils_select_file_with_preview:
 * @window: the #BaseWindow which will be the parent of the dialog box.
 * @title: the title of the dialog box.
 * @wsp_name: the name of the dialog box in Preferences to read/write
 *  its size and position.
 * @entry: the #GtkEntry which is associated with the selected file.
 * @entry_name: the name of the entry in Preferences to be read/written.
 * @update_preview_cb: the callback function in charge of updating the
 *  preview widget. May be NULL.
 *
 * Opens a #GtkFileChooserDialog and let the user choose an existing file
 * -> choose and display an existing file name
 * -> record the dirname URI.
 *
 * If the user validates its selection, the chosen file pathname will be
 * written in the @entry #GtkEntry, while the corresponding dirname
 * URI will be written as @entry_name in Preferences.
 */
void
base_gtk_utils_select_file_with_preview( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name,
				GCallback update_preview_cb )
{
	GtkWindow *toplevel;
	GtkWidget *dialog;
	const gchar *text;
	gchar *filename, *uri;
	GtkWidget *preview;

	toplevel = base_window_get_gtk_toplevel( window );

	dialog = gtk_file_chooser_dialog_new(
			title,
			toplevel,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	if( update_preview_cb ){
		preview = gtk_image_new();
		gtk_file_chooser_set_preview_widget( GTK_FILE_CHOOSER( dialog ), preview );
		g_signal_connect( dialog, "update-preview", update_preview_cb, preview );
	}

	base_gtk_utils_restore_window_position( window, wsp_name );

	text = gtk_entry_get_text( GTK_ENTRY( entry ));

	if( text && strlen( text )){
		gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), text );

	} else {
		uri = na_settings_get_string( entry_name, NULL, NULL );
		if( uri ){
			gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
			g_free( uri );
		}
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( entry ), filename );
	    g_free( filename );
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	na_settings_set_string( entry_name, uri );
	g_free( uri );

	base_gtk_utils_save_window_position( window, wsp_name );

	gtk_widget_destroy( dialog );
}

/**
 * base_gtk_utils_select_dir:
 * @window: the #BaseWindow which will be the parent of the dialog box.
 * @title: the title of the dialog box.
 * @wsp_name: the name of the dialog box in Preferences to read/write
 *  its size and position.
 * @entry: the #GtkEntry which is associated with the field.
 * @entry_name: the name of the entry in Preferences to be read/written.
 * @default_dir_uri: the URI of the directory which should be set in there is
 *  not yet any preference (see @entry_name)
 *
 * Opens a #GtkFileChooserDialog and let the user choose an existing directory
 * -> choose and display an existing dir name
 * -> record the dirname URI of this dir name.
 *
 * If the user validates its selection, the chosen file pathname will be
 * written in the @entry #GtkEntry, while the corresponding dirname
 * URI will be written as @entry_name in Preferences.
 */
void
base_gtk_utils_select_dir( BaseWindow *window,
				const gchar *title, const gchar *wsp_name,
				GtkWidget *entry, const gchar *entry_name )
{
	GtkWindow *toplevel;
	GtkWidget *dialog;
	const gchar *text;
	gchar *dir, *uri;

	toplevel = base_window_get_gtk_toplevel( window );

	dialog = gtk_file_chooser_dialog_new(
			title,
			toplevel,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL
			);

	base_gtk_utils_restore_window_position( window, wsp_name );

	text = gtk_entry_get_text( GTK_ENTRY( entry ));

	if( text && strlen( text )){
		gtk_file_chooser_set_filename( GTK_FILE_CHOOSER( dialog ), text );

	} else {
		uri = na_settings_get_string( entry_name, NULL, NULL );
		if( uri ){
			gtk_file_chooser_set_current_folder_uri( GTK_FILE_CHOOSER( dialog ), uri );
			g_free( uri );
		}
	}

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		dir = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ));
		gtk_entry_set_text( GTK_ENTRY( entry ), dir );
	    g_free( dir );
	  }

	uri = gtk_file_chooser_get_current_folder_uri( GTK_FILE_CHOOSER( dialog ));
	na_settings_set_string( entry_name, uri );
	g_free( uri );

	base_gtk_utils_save_window_position( window, wsp_name );

	gtk_widget_destroy( dialog );
}
