/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011, 2012 Pierre Wieser and others (see AUTHORS)
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

#ifndef __BASE_WINDOW_H__
#define __BASE_WINDOW_H__

/**
 * SECTION: base-window
 * @title: BaseWindow
 * @short_description: the BaseWindow base window class definition
 * @include: base-window.h
 *
 * This is a base class which manages a Gtk+ toplevel.
 *
 * One global UI manager is allocated at #BaseWindow class level.
 * Each window may have its own, provided that it is willing to
 * reinstanciate a new builder each time the window is opened.
 *
 *   Cf. http://bugzilla.gnome.org/show_bug.cgi?id=589746 against
 *   Gtk+ 2.16 : a GtkFileChooserWidget embedded in a GtkAssistant is
 *   not displayed when run more than once. As a work-around, reload
 *   the XML ui in a new builder each time we run an assistant !
 *
 * Note that having its own builder implies loading in it the required
 * XML file which holds the needed UI definition, and so even if this
 * same XML file has already been load in the common builder.
 *
 * The common #BaseBuilder is never g_object_unref(), so the
 * embedded Gtk toplevels are only destroyed when quitting the
 * program.
 */

#include <gtk/gtk.h>

#include "base-application.h"

G_BEGIN_DECLS

#define BASE_WINDOW_TYPE                ( base_window_get_type())
#define BASE_WINDOW( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, BASE_WINDOW_TYPE, BaseWindow ))
#define BASE_WINDOW_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, BASE_WINDOW_TYPE, BaseWindowClass ))
#define BASE_IS_WINDOW( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, BASE_WINDOW_TYPE ))
#define BASE_IS_WINDOW_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), BASE_WINDOW_TYPE ))
#define BASE_WINDOW_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), BASE_WINDOW_TYPE, BaseWindowClass ))

typedef struct _BaseWindowPrivate       BaseWindowPrivate;

typedef struct {
	/*< private >*/
	GObject            parent;
	BaseWindowPrivate *private;
}
	BaseWindow;

typedef struct _BaseWindowClassPrivate  BaseWindowClassPrivate;

/**
 * BaseWindowClass:
 * @initialize_gtk_toplevel: initialize the toplevel GtkWindow
 * @initialize_base_window:  initialize the BaseWindow
 * @all_widgets_showed:      all widgets have been showed
 * @run:                     run the dialog box loop
 * @is_willing_to_quit:      asks if the window is willing to quit
 *
 * This defines the virtual method a derived class may, should or must implement.
 */
typedef struct {
	/*< private >*/
	GObjectClass            parent;
	BaseWindowClassPrivate *private;

	/*< public >*/
	/**
	 * initialize_gtk_toplevel:
	 * @window: this #BaseWindow instance.
	 * @toplevel: the GtkWindow being initialized.
	 *
	 * This virtual method is invoked after the toplevel GtkWindow has been
	 * allocated and built for the firt time by the GtkBuilder, after all
	 * connected signal handlers have themselves run.
	 *
	 * The derived class should invoke the virtual method of its parent class
	 * at the end of its processing.
	 *
	 * The BaseWindow class ensures that each created instance has its Gtk
	 * toplevel - if it has been successfully loaded - initialized when
	 * returning from instanciation.
	 * The instance has nonetheless still to check if the Gtk toplevel has
	 * actually been built, and to initialize and show it before anything
	 * useful may occur (see base_window_init()).
	 *
	 * Note that initialization process may fall if the XML UI definition cannot
	 * be loaded in memory, or if the required Gtk toplevel cannot be found.
	 * Derived class has so to make sure that a Gtk toplevel actually exists
	 * before continuing. Calling base_window_init() on the instance may
	 * do this check.
	 */
	void     ( *initialize_gtk_toplevel )( BaseWindow *window, GtkWindow *toplevel );

	/**
	 * initialize_base_window:
	 * @window: this #BaseWindow instance.
	 *
	 * This virtual method is invoked as the first phase of base_window_init(),
	 * after having checked for the presence of a GtkWindow toplevel, before
	 * actually displaying the widget, and after all connected signal handlers
	 * have themselves run.
	 *
	 * The derived class should invoke the virtual method of its parent class
	 * at the end of its processing.
	 *
	 * The BaseWindow base class implementation of this method, which is
	 * so called last, just does nothing.
	 */
	void     ( *initialize_base_window ) ( BaseWindow *window );

	/**
	 * all_widgets_showed:
	 * @window: this #BaseWindow instance.
	 *
	 * This virtual method is invoked at the end of initialization process,
	 * after all connected signal handlers have themselves run.
	 *
	 * The derived class should invoke the virtual method of its parent class
	 * at the end of its processing.
	 *
	 * The BaseWindow base class implementation of this method, which is
	 * so called last, will call gtk_widget_show_all() on the Gtk toplevel.
	 */
	void     ( *all_widgets_showed )     ( BaseWindow *window );

	/**
	 * run:
	 * @window: this #BaseWindow instance.
	 * @dialog: the toplevel #GtkWindow.
	 *
	 * Invoked when it is time to run the main loop for the toplevel.
	 *
	 * The #BaseWindow class makes sure that the #GtkWindow toplevel and
	 * the #BaseWindow window have both been initialized.
	 *
	 * The #BaseWindow class defaults to do nothing.
	 *
	 * Returns: the exit code of the program if it is the main window.
	 */
	int      ( *run )                    ( BaseWindow *window, GtkWindow *toplevel );
}
	BaseWindowClass;

/**
 * Properties defined by the BaseWindow class.
 * They should be provided at object instanciation time.
 *
 * Instanciation time requires:
 * - either PARENT or APPLICATION
 * - XMLUI_FILENAME
 * - TOPLEVEL_NAME
 * - HAS_OWN_BUILDER
 */
#define BASE_PROP_PARENT						"base-prop-window-parent"
#define BASE_PROP_APPLICATION					"base-prop-window-application"
#define BASE_PROP_XMLUI_FILENAME				"base-prop-window-xmlui-filename"
#define BASE_PROP_HAS_OWN_BUILDER				"base-prop-window-has-own-builder"
#define BASE_PROP_TOPLEVEL_NAME					"base-prop-window-toplevel-name"
#define BASE_PROP_WSP_NAME						"base-prop-window-wsp-name"

/**
 * Signals defined by the BaseWindow class.
 *
 * All signals of this class share the same behavior:
 *
 * - the message is sent to all derived classes, which are free to
 *   connect to the signal in order to implement their own code;
 *
 * - finally, the default class handler invokes the corresponding
 *   virtual method of the derived class. The derived class should
 *   call the parent class method at the end of its implementation.
 *
 * This way, each class is free to choose to implement the action, either
 * as a signal handler or as a virtual method if it is a class derived from
 * BaseWindow.
 */
#define BASE_SIGNAL_INITIALIZE_GTK				"base-signal-window-initialize-gtk"
#define BASE_SIGNAL_INITIALIZE_WINDOW			"base-signal-window-initialize-window"
#define BASE_SIGNAL_SHOW_WIDGETS				"base-signal-window-show-widgets"

GType            base_window_get_type( void );

gboolean         base_window_init( BaseWindow *window );
int              base_window_run ( BaseWindow *window );

#ifdef NA_MAINTAINER_MODE
void             base_window_dump_children           ( const BaseWindow *window );
#endif

BaseApplication *base_window_get_application         ( const BaseWindow *window );
BaseWindow      *base_window_get_parent              ( const BaseWindow *window );
GtkWindow       *base_window_get_gtk_toplevel        ( const BaseWindow *window );
GtkWindow       *base_window_get_gtk_toplevel_by_name( const BaseWindow *window, const gchar *name );
GtkWidget       *base_window_get_widget              ( const BaseWindow *window, const gchar *name );

void             base_window_display_error_dlg       ( const BaseWindow *parent, const gchar *primary, const gchar *secondary );
gboolean         base_window_display_yesno_dlg       ( const BaseWindow *parent, const gchar *primary, const gchar *secondary );
void             base_window_display_message_dlg     ( const BaseWindow *parent, GSList *message );

gulong           base_window_signal_connect          ( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_after    ( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_by_name  ( BaseWindow *window, const gchar *name, const gchar *signal, GCallback fn );
gulong           base_window_signal_connect_with_data( BaseWindow *window, GObject *instance, const gchar *signal, GCallback fn, void *user_data );
void             base_window_signal_disconnect       ( BaseWindow *window, gulong handler_id );

G_END_DECLS

#endif /* __BASE_WINDOW_H__ */
