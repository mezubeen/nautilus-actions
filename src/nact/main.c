/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
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

#include <core/na-gconf-migration.h>

#include "nact-application.h"

/*
 * The 'configure' script may define a NA_MAINTAINER_MODE variable when
 * the application is compiled for/in a development environment. When
 * this variable is defined, debug messages are printed on stdout.
 *
 * The NAUTILUS_ACTIONS_DEBUG environment variable may be defined at
 * execution time to display debug messages. Else, debug messages are only
 * displayed when in maintainer mode.
 */

static void set_log_handler( void );
static void log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data );

static GLogFunc st_default_log_func = NULL;

int
main( int argc, char *argv[] )
{
	NactApplication *appli;
	int ret;

	set_log_handler();

	/* pwi 2011-01-05
	 * run GConf migration tools before doing anything else
	 * above all before allocating a new NAPivot
	 */
	na_gconf_migration_run();

	/* create and run the application
	 */
	appli = nact_application_new();
	ret = nact_application_run_with_args( appli, argc, argv );
	g_object_unref( appli );

	return( ret );
}

static void
set_log_handler( void )
{
	st_default_log_func = g_log_set_default_handler(( GLogFunc ) log_handler, NULL );
}

static void
log_handler( const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data )
{
#ifdef NA_MAINTAINER_MODE
	( *st_default_log_func )( log_domain, log_level, message, user_data );
#else
	if( g_getenv( NAUTILUS_ACTIONS_DEBUG )){
		( *st_default_log_func )( log_domain, log_level, message, user_data );
	}
#endif
}
