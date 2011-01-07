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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>

#include <api/na-core-utils.h>
#include <api/na-object-api.h>

#include <core/na-exporter.h>
#include <core/na-export-format.h>

#include "console-utils.h"

static gchar     *id               = "";
static gchar     *format           = "";
static gboolean   version          = FALSE;

static GOptionEntry entries[] = {

	{ "id"                   , 'i', 0, G_OPTION_ARG_STRING        , &id,
			N_( "The identifiant of the menu or the action to be printed" ), N_( "<STRING>" ) },
	{ "format"               , 'f', 0, G_OPTION_ARG_STRING,     &format,
			N_( "An export format [Desktop1]" ), N_( "<STRING>" ) },
	{ NULL }
};

static GOptionEntry misc_entries[] = {

	{ "version"              , 'v', 0, G_OPTION_ARG_NONE        , &version,
			N_( "Output the version number" ), NULL },
	{ NULL }
};

static NAPivot *pivot = NULL;

static GOptionContext  *init_options( void );
static NAObjectItem    *get_item( const gchar *id );
static void             export_item( const NAObjectItem *item, const gchar *format );
static void             exit_with_usage( void );

int
main( int argc, char** argv )
{
	int status = EXIT_SUCCESS;
	GOptionContext *context;
	GError *error = NULL;
	gchar *help;
	gint errors;
	NAObjectItem *item;
	GList *formats_list, *it;
	gboolean format_found;

	g_type_init();
	console_init_log_handler();

	context = init_options();

	if( argc == 1 ){
		g_set_prgname( argv[0] );
		help = g_option_context_get_help( context, FALSE, NULL );
		g_print( "\n%s", help );
		g_free( help );
		exit( status );
	}

	if( !g_option_context_parse( context, &argc, &argv, &error )){
		g_printerr( _( "Syntax error: %s\n" ), error->message );
		g_error_free (error);
		exit_with_usage();
	}

	g_option_context_free( context );

	if( version ){
		na_core_utils_print_version();
		exit( status );
	}

	errors = 0;

	if( !id || !strlen( id )){
		g_printerr( _( "Error: a menu or action id is mandatory.\n" ));
		errors += 1;
	}

	item = get_item( id );
	if( !item ){
		errors += 1;
	}

	if( !format || !strlen( format )){
		format = "Desktop1";
	}

	formats_list = na_exporter_get_formats( pivot );
	format_found = FALSE;

	for( it = formats_list ; it && !format_found ; it = it->next ){
		NAExportFormat *export = NA_EXPORT_FORMAT( it->data );
		gchar *export_id = na_export_format_get_id( export );
		format_found = !strcmp( export_id, format );
		g_free( export_id );
	}

	na_exporter_free_formats( formats_list );

	if( !format_found ){
		/* i18n: %s stands for the id of the export format, and is not translatable */
		g_printerr( _( "Error: %s: unknown export format.\n" ), format );
		errors += 1;
	}

	if( errors ){
		exit_with_usage();
	}

	export_item( item, format );

	exit( status );
}

/*
 * init options context
 */
static GOptionContext *
init_options( void )
{
	GOptionContext *context;
	gchar* description;
	GOptionGroup *misc_group;

	context = g_option_context_new( _( "Print a menu or an action to stdout." ));

#ifdef ENABLE_NLS
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR );
# ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
# endif
	textdomain( GETTEXT_PACKAGE );
	g_option_context_add_main_entries( context, entries, GETTEXT_PACKAGE );
#else
	g_option_context_add_main_entries( context, entries, NULL );
#endif

	description = console_cmdline_get_description();
	g_option_context_set_description( context, description );
	g_free( description );

	misc_group = g_option_group_new(
			"misc", _( "Miscellaneous options" ), _( "Miscellaneous options" ), NULL, NULL );
	g_option_group_add_entries( misc_group, misc_entries );
	g_option_context_add_group( context, misc_group );

	return( context );
}

/*
 * search for the action in the repository
 */
static NAObjectItem *
get_item( const gchar *id )
{
	NAObjectItem *item = NULL;

	pivot = na_pivot_new();
	na_pivot_set_loadable( pivot, PIVOT_LOAD_ALL );
	na_pivot_load_items( pivot );

	item = na_pivot_get_item( pivot, id );

	if( !item ){
		g_printerr( _( "Error: item '%s' doesn't exist.\n" ), id );
	}

	return( item );
}

/*
 * displays the specified item on stdout, in the specified export format
 */
static void
export_item( const NAObjectItem *item, const gchar *format )
{
	GSList *messages = NULL;
	GSList *it;
	GQuark q_format = g_quark_from_string( format );

	gchar *buffer = na_exporter_to_buffer( pivot, item, q_format, &messages );

	for( it = messages ; it ; it = it->next ){
		g_printerr( "%s\n", ( const gchar * ) it->data );
	}
	na_core_utils_slist_free( messages );

	if( buffer ){
		g_printf( "%s\n", buffer );
		g_free( buffer );
	}
}

/*
 * print a help message and exit with failure
 */
static void
exit_with_usage( void )
{
	g_printerr( _( "Try %s --help for usage.\n" ), g_get_prgname());
	exit( EXIT_FAILURE );
}