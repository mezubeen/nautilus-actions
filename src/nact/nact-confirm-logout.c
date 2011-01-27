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

#include "nact-confirm-logout.h"
#include "nact-main-menubar-file.h"

/* private class data
 */
struct _NactConfirmLogoutClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NactConfirmLogoutPrivate {
	gboolean dispose_has_run;
	gboolean willing_to_quit;
};

enum {
	BTN_QUIT_WITHOUT_SAVING = 1,
	BTN_CANCEL,
	BTN_SAVE_AND_QUIT
};

static const gchar     *st_toplevel_name  = "ConfirmLogoutDialog";

static BaseDialogClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NactConfirmLogoutClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static void     on_base_initialize_base_window( NactConfirmLogout *editor );
static void     on_quit_without_saving_clicked( GtkButton *button, NactConfirmLogout *editor );
static void     on_cancel_clicked( GtkButton *button, NactConfirmLogout *editor );
static void     on_save_and_quit_clicked( GtkButton *button, NactConfirmLogout *editor );
static void     close_dialog( NactConfirmLogout *editor, gboolean willing_to );

GType
nact_confirm_logout_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_confirm_logout_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactConfirmLogoutClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactConfirmLogout ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactConfirmLogout", &info, 0 );

	return( type );
}

static void
class_init( NactConfirmLogoutClass *klass )
{
	static const gchar *thisfn = "nact_confirm_logout_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactConfirmLogoutClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_confirm_logout_instance_init";
	NactConfirmLogout *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( instance ));
	self = NACT_CONFIRM_LOGOUT( instance );

	self->private = g_new0( NactConfirmLogoutPrivate, 1 );

	base_window_signal_connect( BASE_WINDOW( instance ),
			G_OBJECT( instance ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_base_initialize_base_window ));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_confirm_logout_instance_dispose";
	NactConfirmLogout *self;

	g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));
	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( dialog ));
	self = NACT_CONFIRM_LOGOUT( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_confirm_logout_instance_finalize";
	NactConfirmLogout *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( dialog ));
	self = NACT_CONFIRM_LOGOUT( dialog );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/**
 * nact_confirm_logout_run:
 * @parent: the NactMainWindow parent of this dialog
 * (usually the NactMainWindow).
 *
 * Initializes and runs the dialog.
 */
gboolean
nact_confirm_logout_run( NactMainWindow *parent )
{
	static const gchar *thisfn = "nact_confirm_logout_run";
	NactConfirmLogout *dialog;
	gboolean willing_to;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( parent ), TRUE );

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );

	dialog = g_object_new( NACT_CONFIRM_LOGOUT_TYPE,
			BASE_PROP_PARENT,        parent,
			BASE_PROP_TOPLEVEL_NAME, st_toplevel_name,
			NULL );

	base_window_run( BASE_WINDOW( dialog ));

	willing_to = dialog->private->willing_to_quit;

	g_object_unref( dialog );

	return( willing_to );
}

static void
on_base_initialize_base_window( NactConfirmLogout *dialog )
{
	static const gchar *thisfn = "nact_confirm_logout_on_base_initialize_base_window";

	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

		base_window_signal_connect_by_name( BASE_WINDOW( dialog ),
				"QuitNoSaveButton", "clicked", G_CALLBACK( on_quit_without_saving_clicked ));

		base_window_signal_connect_by_name( BASE_WINDOW( dialog ),
				"CancelQuitButton", "clicked", G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name( BASE_WINDOW( dialog ),
				"SaveQuitButton", "clicked", G_CALLBACK( on_save_and_quit_clicked ));
	}
}

static void
on_quit_without_saving_clicked( GtkButton *button, NactConfirmLogout *editor )
{
	static const gchar *thisfn = "nact_confirm_logout_on_quit_without_saving_clicked";

	g_debug( "%s: button=%p, editor=%p", thisfn, ( void * ) button, ( void * ) editor );

	close_dialog( editor, TRUE );
}

static void
on_cancel_clicked( GtkButton *button, NactConfirmLogout *editor )
{
	static const gchar *thisfn = "nact_confirm_logout_on_cancel_clicked";

	g_debug( "%s: button=%p, editor=%p", thisfn, ( void * ) button, ( void * ) editor );

	close_dialog( editor, FALSE );
}

static void
on_save_and_quit_clicked( GtkButton *button, NactConfirmLogout *editor )
{
	static const gchar *thisfn = "nact_confirm_logout_on_cancel_clicked";
	NactMainWindow *main_window;

	g_debug( "%s: button=%p, editor=%p", thisfn, ( void * ) button, ( void * ) editor );

	main_window = NACT_MAIN_WINDOW( base_window_get_parent( BASE_WINDOW( editor )));
	nact_main_menubar_file_save_items( main_window );

	close_dialog( editor, TRUE );
}

static void
close_dialog( NactConfirmLogout *editor, gboolean willing_to )
{
	static const gchar *thisfn = "nact_confirm_logout_close_dialog";
	GtkWindow *toplevel;

	g_debug( "%s: editor=%p, willing_to=%s", thisfn, ( void * ) editor, willing_to ? "True":"False" );

	editor->private->willing_to_quit = willing_to;

	toplevel = base_window_get_gtk_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}
