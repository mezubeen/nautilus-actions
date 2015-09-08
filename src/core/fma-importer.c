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

#include <glib/gi18n.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-iimporter.h>
#include <api/fma-object-api.h>

#include "fma-import-mode.h"
#include "fma-importer.h"
#include "fma-importer-ask.h"

typedef struct {
	guint        id;				/* the import mode used in switch statement in the code */
	const gchar *mode;				/* the import mode saved in user's preferences */
	const gchar *label;				/* short label */
	const gchar *description;		/* full description */
	gchar       *image;				/* associated image */
}
	FMAImportModeStr;

static FMAImportModeStr st_import_modes[] = {

	{ IMPORTER_MODE_NO_IMPORT,
			"NoImport",
			N_( "Do _not import the item" ),
			N_( "This used to be the historical behavior.\n" \
				"The selected file will be marked as \"NOT OK\" in the Summary page.\n" \
				"The existing item will not be modified." ),
			"fma-import-mode-no-import.png" },

	{ IMPORTER_MODE_RENUMBER,
			"Renumber",
			N_( "Import the item, _allocating it a new identifier" ),
			N_( "The selected file will be imported with a slightly modified label " \
				"indicating the renumbering.\n" \
				"The existing item will not be modified." ),
			"fma-import-mode-renumber.png" },

	{ IMPORTER_MODE_OVERRIDE,
			"Override",
			N_( "_Override the existing item" ),
			N_( "The item found in the selected file will silently override the " \
				"current one which has the same identifier.\n" \
				"Be warned: this mode may be dangerous. You will not be prompted another time." ),
			"fma-import-mode-override.png" },

	{ 0 }
};

static FMAImportModeStr st_import_ask_mode = {

	IMPORTER_MODE_ASK,
			"Ask",
			N_( "_Ask me" ),
			N_( "You will be asked each time an imported ID already exists." ),
			"fma-import-mode-ask.png"
};

static FMAImporterResult *import_from_uri( const FMAPivot *pivot, GList *modules, const gchar *uri );
static void               manage_import_mode( FMAImporterParms *parms, GList *results, FMAImporterAskUserParms *ask_parms, FMAImporterResult *result );
static FMAObjectItem     *is_importing_already_exists( FMAImporterParms *parms, GList *results, FMAImporterResult *result );
static void               renumber_label_item( FMAObjectItem *item );
static guint              ask_user_for_mode( const FMAObjectItem *importing, const FMAObjectItem *existing, FMAImporterAskUserParms *parms );
static guint              get_id_from_string( const gchar *str );
static FMAIOption        *get_mode_from_struct( const FMAImportModeStr *str );

/* i18n: '%s' stands for the file URI */
#define ERR_NOT_LOADABLE	_( "%s is not loadable (empty or too big or not a regular file)" )

/*
 * fma_importer_import_from_uris:
 * @pivot: the #FMAPivot pivot for this application.
 * @parms: a #FMAImporterParms structure.
 *
 * Imports a list of URIs.
 *
 * For each URI to import, we search through the available #FMAIImporter
 * providers until the first which returns with something different from
 * "not_willing_to" code.
 *
 * #parms.uris contains a list of URIs to import.
 *
 * Each import operation will have its corresponding newly allocated
 * #FMAImporterResult structure which will contain:
 * - the imported URI
 * - the #FMAIImporter provider if one has been found, or %NULL
 * - a #FMAObjectItem item if import was successful, or %NULL
 * - a list of error messages, or %NULL.
 *
 * Returns: a #GList of #FMAImporterResult structures
 * (was the last import operation code up to 3.2).
 *
 * Since: 2.30
 */
GList *
fma_importer_import_from_uris( const FMAPivot *pivot, FMAImporterParms *parms )
{
	static const gchar *thisfn = "fma_importer_import_from_uris";
	GList *results, *ires;
	GList *modules;
	GSList *uri;
	FMAImporterResult *import_result;
	FMAImporterAskUserParms ask_parms;
	gchar *mode_str;

	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );
	g_return_val_if_fail( parms != NULL, NULL );

	results = NULL;

	g_debug( "%s: pivot=%p, parms=%p", thisfn, ( void * ) pivot, ( void * ) parms );

	/* first phase: just try to import the uris into memory
	 */
	modules = fma_pivot_get_providers( pivot, FMA_TYPE_IIMPORTER );

	for( uri = parms->uris ; uri ; uri = uri->next ){
		import_result = import_from_uri( pivot, modules, ( const gchar * ) uri->data );
		results = g_list_prepend( results, import_result );
	}

	fma_pivot_free_providers( modules );

	results = g_list_reverse( results );

	memset( &ask_parms, '\0', sizeof( FMAImporterAskUserParms ));
	ask_parms.parent = parms->parent_toplevel;
	ask_parms.count = 0;
	ask_parms.keep_choice = FALSE;
	ask_parms.pivot = pivot;

	/* set the default import mode
	 */
	if( !parms->preferred_mode ){
		mode_str = fma_settings_get_string( IPREFS_IMPORT_PREFERRED_MODE, NULL, NULL );
		parms->preferred_mode = get_id_from_string( mode_str );
		g_free( mode_str );
	}

	/* second phase: check for their pre-existence
	 */
	for( ires = results ; ires ; ires = ires->next ){
		import_result = ( FMAImporterResult * ) ires->data;

		if( import_result->imported ){
			g_return_val_if_fail( FMA_IS_OBJECT_ITEM( import_result->imported ), NULL );
			g_return_val_if_fail( FMA_IS_IIMPORTER( import_result->importer ), NULL );

			ask_parms.uri = import_result->uri;
			manage_import_mode( parms, results, &ask_parms, import_result );
		}
	}

	return( results );
}

/*
 * fma_importer_free_result:
 * @result: the #FMAImporterResult structure to be released.
 *
 * Release the structure.
 */
void
fma_importer_free_result( FMAImporterResult *result )
{
	g_free( result->uri );
	fma_core_utils_slist_free( result->messages );

	g_free( result );
}

/*
 * Each FMAIImporter interface may return some messages, specially if it
 * recognized but is not able to import the provided URI. But as long
 * we do not have yet asked to all available interfaces, we are not sure
 * of whether this URI is eventually importable or not.
 *
 * We so let each interface push its messages in the list, but be ready to
 * only keep the messages provided by the interface which has successfully
 * imported the item.
 */
static FMAImporterResult *
import_from_uri( const FMAPivot *pivot, GList *modules, const gchar *uri )
{
	FMAImporterResult *result;
	FMAIImporterImportFromUriParmsv2 provider_parms;
	GList *im;
	guint code;
	GSList *all_messages;
	FMAIImporter *provider;

	result = NULL;
	all_messages = NULL;
	provider = NULL;
	code = IMPORTER_CODE_NOT_WILLING_TO;

	memset( &provider_parms, '\0', sizeof( FMAIImporterImportFromUriParmsv2 ));
	provider_parms.version = 2;
	provider_parms.content = 1;
	provider_parms.uri = uri;

	for( im = modules ;
			im && ( code == IMPORTER_CODE_NOT_WILLING_TO || code == IMPORTER_CODE_NOT_LOADABLE ) ;
			im = im->next ){

		code = fma_iimporter_import_from_uri( FMA_IIMPORTER( im->data ), &provider_parms );

		if( code == IMPORTER_CODE_NOT_WILLING_TO ){
			all_messages = g_slist_concat( all_messages, provider_parms.messages );
			provider_parms.messages = NULL;

		} else if( code == IMPORTER_CODE_NOT_LOADABLE ){
			fma_core_utils_slist_free( all_messages );
			all_messages = NULL;
			fma_core_utils_slist_free( provider_parms.messages );
			provider_parms.messages = NULL;
			fma_core_utils_slist_add_message( &all_messages, ERR_NOT_LOADABLE, ( const gchar * ) uri );

		} else {
			fma_core_utils_slist_free( all_messages );
			all_messages = provider_parms.messages;
			provider = FMA_IIMPORTER( im->data );
		}
	}

	result = g_new0( FMAImporterResult, 1 );
	result->uri = g_strdup( uri );
	result->imported = provider_parms.imported;
	result->importer = provider;
	result->messages = all_messages;

	return( result );
}

/*
 * check for existence of the imported item
 * ask for the user if needed
 */
static void
manage_import_mode( FMAImporterParms *parms, GList *results, FMAImporterAskUserParms *ask_parms, FMAImporterResult *result )
{
	static const gchar *thisfn = "fma_importer_manage_import_mode";
	FMAObjectItem *exists;
	guint mode;
	gchar *id;

	exists = NULL;
	result->exist = FALSE;
	result->mode = parms->preferred_mode;
	mode = 0;

	/* if no check function is provided, then we systematically allocate
	 * a new identifier to the imported item
	 */
	if( !parms->check_fn ){
		renumber_label_item( result->imported );
		fma_core_utils_slist_add_message(
				&result->messages,
				"%s",
				_( "Item was renumbered because the caller did not provide any check function." ));
		result->mode = IMPORTER_MODE_RENUMBER;

	} else {
		exists = is_importing_already_exists( parms, results, result );
	}

	g_debug( "%s: exists=%p", thisfn, exists );

	if( exists ){
		result->exist = TRUE;

		if( parms->preferred_mode == IMPORTER_MODE_ASK ){
			mode = ask_user_for_mode( result->imported, exists, ask_parms );

		} else {
			mode = parms->preferred_mode;
		}
	}

	/* mode is only set if asked mode was "ask me" and an ask function was provided
	 * or if asked mode was not "ask me"
	 */
	if( mode ){
		result->mode = mode;

		switch( mode ){
			case IMPORTER_MODE_RENUMBER:
				renumber_label_item( result->imported );
				if( parms->preferred_mode == IMPORTER_MODE_ASK ){
					fma_core_utils_slist_add_message(
							&result->messages,
							"%s",
							_( "Item was renumbered due to user request." ));
				}
				break;

			case IMPORTER_MODE_OVERRIDE:
				if( parms->preferred_mode == IMPORTER_MODE_ASK ){
					fma_core_utils_slist_add_message(
							&result->messages,
							"%s",
							_( "Existing item was overriden due to user request." ));
				}
				break;

			case IMPORTER_MODE_NO_IMPORT:
			default:
				id = fma_object_get_id( result->imported );
				fma_core_utils_slist_add_message(
						&result->messages,
						_( "Item %s already exists." ),
						id );
				if( parms->preferred_mode == IMPORTER_MODE_ASK ){
					fma_core_utils_slist_add_message(
							&result->messages,
							"%s",
							_( "Import was canceled due to user request." ));
				}
				g_free( id );
				g_object_unref( result->imported );
				result->imported = NULL;
		}
	}
}

/*
 * First check here for duplicates inside of imported population,
 * then delegates to the caller-provided check function the rest of work...
 */
static FMAObjectItem *
is_importing_already_exists( FMAImporterParms *parms, GList *results, FMAImporterResult *result )
{
	static const gchar *thisfn = "fma_importer_is_importing_already_exists";
	FMAObjectItem *exists;
	GList *ip;

	exists = NULL;

	gchar *importing_id = fma_object_get_id( result->imported );
	g_debug( "%s: importing=%p, id=%s", thisfn, ( void * ) result->imported, importing_id );

	/* is the importing item already in the current importation list ?
	 * (only tries previous items of the list)
	 */
	for( ip = results ; ip && !exists && ip->data != result ; ip = ip->next ){
		FMAImporterResult *try_result = ( FMAImporterResult * ) ip->data;

		if( try_result->imported ){
			g_return_val_if_fail( FMA_IS_OBJECT_ITEM( try_result->imported ), NULL );

			gchar *id = fma_object_get_id( try_result->imported );
			if( !strcmp( importing_id, id )){
				exists = FMA_OBJECT_ITEM( try_result->imported );
			}
			g_free( id );
		}
	}

	g_free( importing_id );

	/* if not found in our current importation list,
	 * then check the existence via provided function and data
	 */
	if( !exists ){
		exists = parms->check_fn( result->imported, parms->check_fn_data );
	}

	return( exists );
}

/*
 * renumber the item, and set a new label
 */
static void
renumber_label_item( FMAObjectItem *item )
{
	gchar *label, *tmp;

	fma_object_set_new_id( item, NULL );

	label = fma_object_get_label( item );

	/* i18n: the action has been renumbered during import operation */
	tmp = g_strdup_printf( "%s %s", label, _( "(renumbered)" ));

	fma_object_set_label( item, tmp );

	g_free( tmp );
	g_free( label );
}

static guint
ask_user_for_mode( const FMAObjectItem *importing, const FMAObjectItem *existing, FMAImporterAskUserParms *parms )
{
	guint mode;
	gchar *mode_str;

	if( parms->count == 0 || !parms->keep_choice ){
		mode = fma_importer_ask_user( importing, existing, parms );

	} else {
		mode_str = fma_settings_get_string( IPREFS_IMPORT_ASK_USER_LAST_MODE, NULL, NULL );
		mode = get_id_from_string( mode_str );
		g_free( mode_str );
	}

	return( mode );
}

static guint
get_id_from_string( const gchar *str )
{
	int i;

	/* search in standard import modes
	 */
	for( i = 0 ; st_import_modes[i].id ; ++i ){
		if( !strcmp( st_import_modes[i].mode, str )){
			return( st_import_modes[i].id );
		}
	}

	/* else, is it ask option ?
	 */
	if( !strcmp( st_import_ask_mode.mode, str )){
		return( st_import_ask_mode.id );
	}

	return( 0 );
}

/*
 * fma_importer_get_modes:
 *
 * Returns: the list of available import modes.
 * This list should later be released by calling fma_importer_free_modes();
 */
GList *
fma_importer_get_modes( void )
{
	static const gchar *thisfn = "fma_importer_get_modes";
	GList *modes;
	FMAIOption *mode;
	guint i;

	g_debug( "%s", thisfn );

	modes = NULL;

	for( i = 0 ; st_import_modes[i].id ; ++i ){
		mode = get_mode_from_struct( st_import_modes+i );
		modes = g_list_prepend( modes, mode );
	}

	return( modes );
}

static FMAIOption *
get_mode_from_struct( const FMAImportModeStr *str )
{
	FMAImportMode *mode;
	gint width, height;
	gchar *fname;
	GdkPixbuf *pixbuf;

	if( !gtk_icon_size_lookup( GTK_ICON_SIZE_DIALOG, &width, &height )){
		width = height = 48;
	}

	mode = fma_import_mode_new( str->id );
	pixbuf = NULL;

	if( str->image && g_utf8_strlen( str->image, -1 )){
		fname = g_strdup_printf( "%s/%s", PKGIMPORTMODEDIR, str->image );
		pixbuf = gdk_pixbuf_new_from_file_at_size( fname, width, height, NULL );
		g_free( fname );
	}
	g_object_set( G_OBJECT( mode ),
		FMA_IMPORT_PROP_MODE,        str->mode,
		FMA_IMPORT_PROP_LABEL,       gettext( str->label ),
		FMA_IMPORT_PROP_DESCRIPTION, gettext( str->description ),
		FMA_IMPORT_PROP_IMAGE,       pixbuf,
		NULL );

	return( FMA_IOPTION( mode ));
}

/*
 * fma_importer_free_modes:
 * @modes: a #GList of #FMAImportMode items, as returned by fma_importer_get_modes().
 *
 * Releases the resources allocated to the @modes list.
 */
void
fma_importer_free_modes( GList *modes )
{
	static const gchar *thisfn = "fma_importer_free_modes";

	g_debug( "%s: modes=%p", thisfn, ( void * ) modes );

	g_list_foreach( modes, ( GFunc ) g_object_unref, NULL );
	g_list_free( modes );
}

/*
 * fma_importer_get_ask_mode:
 *
 * Returns: a #FMAImportMode object which describes the 'Ask me' option.
 */
FMAIOption *
fma_importer_get_ask_mode( void )
{
	static const gchar *thisfn = "fma_importer_get_ask_mode";

	g_debug( "%s", thisfn );

	return( get_mode_from_struct( &st_import_ask_mode ));
}