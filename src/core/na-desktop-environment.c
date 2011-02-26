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

#include <glib/gi18n.h>

#include "na-desktop-environment.h"

static const NADesktopEnv st_desktops[] = {
	{ DESKTOP_GNOME, N_( "GNOME desktop" ) },
	{ DESKTOP_KDE,   N_( "KDE desktop" ) },
	{ DESKTOP_LXDE,  N_( "LXDE desktop" ) },
	{ DESKTOP_ROX,   N_( "ROX desktop" ) },
	{ DESKTOP_XFCE,  N_( "XFCE desktop" ) },
	{ DESKTOP_OLD,   N_( "Legacy systems" ) },
	{ NULL }
};

/*
 * na_desktop_environment_get_known_list:
 *
 * Returns the list of known desktop environments as defined by the
 * corresponding XDG specification.
 */
const NADesktopEnv *
na_desktop_environment_get_known_list( void )
{
	return(( const NADesktopEnv * ) st_desktops );
}


/*
 * Have asked on xdg-list how to identify the currently running desktop environment
 * (see http://standards.freedesktop.org/menu-spec/latest/apb.html)
 * For now, just reproduce the xdg-open algorythm from xdg-utils 1.0
 */

const gchar *
na_desktop_environment_detect_running_desktop( void )
{
	static const gchar *thisfn = "na_desktop_environment_detect_running_desktop";
	const gchar *value;
	gchar *output_str, *error_str;
	gint exit_status;
	GError *error;
	gboolean ok;

	value = g_getenv( "KDE_FULL_SESSION" );
	if( value && !strcmp( value, "true" )){
		return( DESKTOP_KDE );
	}

	value = g_getenv( "GNOME_DESKTOP_SESSION_ID" );
	if( value && strlen( value )){
		return( DESKTOP_GNOME );
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager",
			&output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_GNOME );
		}
	}
	if( error ){
		g_warning( "%s: dbus-send: %s", thisfn, error->message );
		g_error_free( error );
	}

	output_str = NULL;
	error_str = NULL;
	error = NULL;
	if( g_spawn_command_line_sync(
			"xprop -root _DT_SAVE_MODE", &output_str, &error_str, &exit_status, &error )){
		ok = ( exit_status == 0 && output_str && strlen( output_str ) && ( !error_str || !strlen( error_str )));
		if( ok ){
			ok = ( g_strstr_len( output_str, -1, "xfce" ) != NULL );
		}
		g_free( output_str );
		g_free( error_str );
		if( ok ){
			return( DESKTOP_XFCE );
		}
	}
	if( error ){
		g_warning( "%s: xprop: %s", thisfn, error->message );
		g_error_free( error );
	}

	/* do not know how to identify ROX or LXDE (Hong Jen Yee <pcman.tw (at) gmail.com>)
	 * environments; so other desktops are just identified as 'Old' (legacy systems)
	 */
	return( DESKTOP_OLD );
}