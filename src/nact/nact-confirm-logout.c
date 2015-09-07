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

#include "core/na-settings.h"

#include "nact-confirm-logout.h"
#include "nact-main-window-def.h"
#include "nact-menu-file.h"

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

static const gchar     *st_xmlui_filename = PKGUIDIR "/nact-confirm-logout.ui";
static const gchar     *st_toplevel_name  = "ConfirmLogoutDialog";
static const gchar     *st_wsp_name       = NA_IPREFS_CONFIRM_LOGOUT_WSP;

static BaseDialogClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NactConfirmLogoutClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_constructed( GObject *dialog );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static void     on_base_initialize_window( NactConfirmLogout *editor, gpointer user_data );
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

	type = g_type_register_static( BASE_TYPE_DIALOG, "NactConfirmLogout", &info, 0 );

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
	object_class->constructed = instance_constructed;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
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

	self->private->dispose_has_run = FALSE;
}

static void
instance_constructed( GObject *dialog )
{
	static const gchar *thisfn = "nact_confirm_logout_instance_constructed";
	NactConfirmLogoutPrivate *priv;

	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( dialog ));

	priv = NACT_CONFIRM_LOGOUT( dialog )->private;

	if( !priv->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( dialog );
		}

		g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

		base_window_signal_connect(
				BASE_WINDOW( dialog ),
				G_OBJECT( dialog ),
				BASE_SIGNAL_INITIALIZE_WINDOW,
				G_CALLBACK( on_base_initialize_window ));
	}
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

	dialog = g_object_new( NACT_TYPE_CONFIRM_LOGOUT,
			BASE_PROP_MAIN_WINDOW,        parent,
			BASE_PROP_XMLUI_FILENAME, st_xmlui_filename,
			BASE_PROP_TOPLEVEL_NAME, st_toplevel_name,
			BASE_PROP_WSP_NAME,      st_wsp_name,
			NULL );

	base_window_run( BASE_WINDOW( dialog ));

	willing_to = dialog->private->willing_to_quit;

	g_object_unref( dialog );

	return( willing_to );
}

static void
on_base_initialize_window( NactConfirmLogout *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_confirm_logout_on_base_initialize_window";

	g_return_if_fail( NACT_IS_CONFIRM_LOGOUT( dialog ));

	if( !dialog->private->dispose_has_run ){

		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"QuitNoSaveButton",
				"clicked",
				G_CALLBACK( on_quit_without_saving_clicked ));

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"CancelQuitButton",
				"clicked",
				G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"SaveQuitButton",
				"clicked",
				G_CALLBACK( on_save_and_quit_clicked ));
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

	main_window = NACT_MAIN_WINDOW( base_window_get_main_window( BASE_WINDOW( editor )));
	nact_menu_file_save_items( main_window );

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
