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

#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <libnautilus-extension/nautilus-extension-types.h>

#include <core/na-gconf-migration.h>
#include <core/na-settings.h>

#include "nautilus-actions.h"

static void set_log_handler( void );
static void log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );

static GLogFunc st_default_log_func = NULL;

/*
 * A nautilus extension must implement three functions :
 *
 * - nautilus_module_initialize
 * - nautilus_module_list_types
 * - nautilus_module_shutdown
 *
 * The first two functions are called at nautilus startup.
 *
 * The prototypes for these functions are defined in nautilus-extension-types.h
 */

void
nautilus_module_initialize( GTypeModule *module )
{
	static const gchar *thisfn = "nautilus_module_initialize";

	syslog( LOG_USER | LOG_INFO, "[N-A] %s Menu Extender %s initializing...", PACKAGE_NAME, PACKAGE_VERSION );

	set_log_handler();

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	g_type_module_set_name( module, PACKAGE_STRING );

	/* pwi 2011-01-05
	 * run GConf migration tools before doing anything else
	 * above all before allocating a new NAPivot
	 */
	na_gconf_migration_run();

	nautilus_actions_register_type( module );
}

void
nautilus_module_list_types( const GType **types, int *num_types )
{
	static const gchar *thisfn = "nautilus_module_list_types";
	static GType type_list[1];

	g_debug( "%s: types=%p, num_types=%p", thisfn, ( void * ) types, ( void * ) num_types );

	type_list[0] = NAUTILUS_ACTIONS_TYPE;
	*types = type_list;
	*num_types = 1;

	/* this may let us some time to attach nautilus to the debugger :) */
	/*sleep( 60 ); */
}

void
nautilus_module_shutdown( void )
{
	static const gchar *thisfn = "nautilus_module_shutdown";

	g_debug( "%s", thisfn );

	/* remove the log handler
	 * almost useless as the process is nonetheless terminating at this time
	 * but this is the art of coding...
	 */
	if( st_default_log_func ){
		g_log_set_default_handler( st_default_log_func, NULL );
		st_default_log_func = NULL;
	}
}

/*
 * a log handler that we install when in development mode in order to be
 * able to log plugin runtime
 *
 * enabling log in the plugin menu at runtime requires a Nautilus restart
 * (because we need to run in the code, which embeds g_debug instructions,
 *  and we have to do so before the log handler be set, or we will run
 *  into a deep stack recursion)
 */
static void
set_log_handler( void )
{
	gboolean is_log_enabled;

#ifdef NA_MAINTAINER_MODE
	is_log_enabled = TRUE;
#else
	is_log_enabled =
			g_getenv( NAUTILUS_ACTIONS_DEBUG ) ||
			na_settings_get_boolean( NA_IPREFS_PLUGIN_MENU_LOG, NULL, NULL );
#endif

	st_default_log_func = g_log_set_default_handler(( GLogFunc ) log_handler, GUINT_TO_POINTER( is_log_enabled ));
}

/*
 * we used to install a log handler for each and every log domain used
 * in FileManager-Actions ; this led to a fastidious enumeration
 * instead we install a default log handler which will receive all
 * debug messages, i.e. not only from N-A, but also from other code
 * in the Nautilus process
 */
static void
log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
	gchar *tmp;
	gboolean is_log_enabled;

	is_log_enabled = ( gboolean ) GPOINTER_TO_UINT( user_data );

	if( is_log_enabled ){
		tmp = g_strdup( "" );

		if( log_domain && strlen( log_domain )){
			g_free( tmp );
			tmp = g_strdup_printf( "[%s] ", log_domain );
		}

		syslog( LOG_USER | LOG_DEBUG, "%s%s", tmp, message );
		g_free( tmp );
	}
}
