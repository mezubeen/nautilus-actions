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

#include <api/fma-data-def.h>
#include <api/fma-data-types.h>
#include <api/fma-iio-provider.h>
#include <api/fma-ifactory-provider.h>
#include <api/fma-object-api.h>
#include <api/fma-core-utils.h>
#include <api/fma-gconf-utils.h>

#include "fma-gconf-provider.h"
#include "nagp-writer.h"
#include "fma-gconf-keys.h"

#ifdef NA_ENABLE_DEPRECATED
static void write_start_write_type( FMAGConfProvider *provider, FMAObjectItem *item );
static void write_start_write_version( FMAGConfProvider *provider, FMAObjectItem *item );
#endif

/*
 * API function: should only be called through FMAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_willing_to_write( const FMAIIOProvider *provider )
{
#ifdef NA_ENABLE_DEPRECATED
	return( TRUE );
#else
	return( FALSE );
#endif
}

/*
 * Rationale: gconf reads its storage path from /etc/gconf/2/path ;
 * there is there a 'xml:readwrite:$(HOME)/.gconf' line, but I do not
 * known any way to get it programatically, so an admin may have set a
 * readwrite space elsewhere..
 *
 * So, we try to write a 'foo' key somewhere: if it is ok, then the
 * provider is supposed able to write...
 *
 * API function: should only be called through FMAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_able_to_write( const FMAIIOProvider *provider )
{
#ifdef NA_ENABLE_DEPRECATED
	static const gchar *thisfn = "nagp_iio_provider_is_able_to_write";
	static const gchar *path = "/apps/filemanager-actions/foo";
	FMAGConfProvider *self;
	gboolean able_to = FALSE;

	/*g_debug( "%s: provider=%p", thisfn, ( void * ) provider );*/
	g_return_val_if_fail( FMA_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), FALSE );

	self = FMA_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		if( !fma_gconf_utils_write_string( self->private->gconf, path, "foo", NULL )){
			able_to = FALSE;

		} else {
			gchar *str = fma_gconf_utils_read_string( self->private->gconf, path, FALSE, NULL );
			if( strcmp( str, "foo" )){
				able_to = FALSE;

			} else if( !gconf_client_recursive_unset( self->private->gconf, path, 0, NULL )){
				able_to = FALSE;

			} else {
				able_to = TRUE;
			}

			g_free( str );
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	g_debug( "%s: provider=%p, able_to=%s", thisfn, ( void * ) provider, able_to ? "True":"False" );
	return( able_to );
#else
	return( FALSE );
#endif
}

#ifdef NA_ENABLE_DEPRECATED
/*
 * update an existing item or write a new one
 * in all cases, it is much more easy to delete the existing  entries
 * before trying to write the new ones
 */
guint
nagp_iio_provider_write_item( const FMAIIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "fma_gconf_provider_iio_provider_write_item";
	FMAGConfProvider *self;
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );

	self = FMA_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = nagp_iio_provider_delete_item( provider, item, messages );

	if( ret == IIO_PROVIDER_CODE_OK ){
		fma_ifactory_provider_write_item( FMA_IFACTORY_PROVIDER( provider ), NULL, FMA_IFACTORY_OBJECT( item ), messages );
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( ret );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
guint
nagp_iio_provider_delete_item( const FMAIIOProvider *provider, const FMAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "fma_gconf_provider_iio_provider_delete_item";
	FMAGConfProvider *self;
	guint ret;
	gchar *uuid, *path;
	GError *error = NULL;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( FMA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), ret );

	self = FMA_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = IIO_PROVIDER_CODE_OK;
	uuid = fma_object_get_id( FMA_OBJECT( item ));

	/* GCONF_UNSET_INCLUDING_SCHEMA_NAMES seems mean: including the name
	 * of the schemas which is embedded in the GConfEntry - this doesn't
	 * mean including the schemas themselves
	 */
	if( ret == IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( FMA_GCONF_CONFIGURATIONS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, GCONF_UNSET_INCLUDING_SCHEMA_NAMES, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR;
		}
		gconf_client_suggest_sync( self->private->gconf, NULL );
		g_free( path );
	}

	if( ret == IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( FMA_GCONF_SCHEMAS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR;
		}
		g_free( path );
		gconf_client_suggest_sync( self->private->gconf, NULL );
	}

	g_free( uuid );

	return( ret );
}

guint
nagp_writer_write_start( const FMAIFactoryProvider *writer, void *writer_data,
							const FMAIFactoryObject *object, GSList **messages  )
{
	if( FMA_IS_OBJECT_ITEM( object )){
		write_start_write_type( FMA_GCONF_PROVIDER( writer ), FMA_OBJECT_ITEM( object ));
		write_start_write_version( FMA_GCONF_PROVIDER( writer ), FMA_OBJECT_ITEM( object ));
	}

	return( IIO_PROVIDER_CODE_OK );
}

static void
write_start_write_type( FMAGConfProvider *provider, FMAObjectItem *item )
{
	gchar *id, *path;

	id = fma_object_get_id( item );
	path = g_strdup_printf( "%s/%s/%s", FMA_GCONF_CONFIGURATIONS_PATH, id, FMA_GCONF_ENTRY_TYPE );

	fma_gconf_utils_write_string(
			provider->private->gconf,
			path,
			FMA_IS_OBJECT_ACTION( item ) ? FMA_GCONF_VALUE_TYPE_ACTION : FMA_GCONF_VALUE_TYPE_MENU,
			NULL );

	g_free( path );
	g_free( id );
}

static void
write_start_write_version( FMAGConfProvider *provider, FMAObjectItem *item )
{
	gchar *id, *path;
	guint iversion;

	id = fma_object_get_id( item );
	path = g_strdup_printf( "%s/%s/%s", FMA_GCONF_CONFIGURATIONS_PATH, id, FMA_GCONF_ENTRY_IVERSION );

	iversion = fma_object_get_iversion( item );
	fma_gconf_utils_write_int( provider->private->gconf, path, iversion, NULL );

	g_free( path );
	g_free( id );
}

guint
nagp_writer_write_data( const FMAIFactoryProvider *provider, void *writer_data,
									const FMAIFactoryObject *object, const FMADataBoxed *boxed,
									GSList **messages )
{
	static const gchar *thisfn = "nagp_writer_write_data";
	guint code;
	const FMADataDef *def;
	gchar *this_id;
	gchar *this_path, *path;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;
	GConfClient *gconf;

	/*g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

	msg = NULL;
	code = IIO_PROVIDER_CODE_OK;
	def = fma_data_boxed_get_data_def( boxed );

	if( !fma_data_boxed_is_default( boxed ) || def->write_if_default ){

		if( FMA_IS_OBJECT_PROFILE( object )){
			FMAObjectItem *parent = FMA_OBJECT_ITEM( fma_object_get_parent( object ));
			gchar *parent_id = fma_object_get_id( parent );
			gchar *id = fma_object_get_id( object );
			this_id = g_strdup_printf( "%s/%s", parent_id, id );
			g_free( id );
			g_free( parent_id );

		} else {
			this_id = fma_object_get_id( object );
		}

		this_path = gconf_concat_dir_and_key( FMA_GCONF_CONFIGURATIONS_PATH, this_id );
		path = gconf_concat_dir_and_key( this_path, def->gconf_entry );

		gconf = FMA_GCONF_PROVIDER( provider )->private->gconf;

		switch( def->type ){

			case FMA_DATA_TYPE_STRING:
				str_value = fma_boxed_get_string( FMA_BOXED( boxed ));
				fma_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				g_free( str_value );
				break;

			case FMA_DATA_TYPE_LOCALE_STRING:
				str_value = fma_boxed_get_string( FMA_BOXED( boxed ));
				fma_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				g_free( str_value );
				break;

			case FMA_DATA_TYPE_BOOLEAN:
				bool_value = GPOINTER_TO_UINT( fma_boxed_get_as_void( FMA_BOXED( boxed )));
				fma_gconf_utils_write_bool( gconf, path, bool_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				break;

			case FMA_DATA_TYPE_STRING_LIST:
				slist_value = ( GSList * ) fma_boxed_get_as_void( FMA_BOXED( boxed ));
				fma_gconf_utils_write_string_list( gconf, path, slist_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				fma_core_utils_slist_free( slist_value );
				break;

			case FMA_DATA_TYPE_UINT:
				uint_value = GPOINTER_TO_UINT( fma_boxed_get_as_void( FMA_BOXED( boxed )));
				fma_gconf_utils_write_int( gconf, path, uint_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = IIO_PROVIDER_CODE_WRITE_ERROR;
				}
				break;

			default:
				g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
				code = IIO_PROVIDER_CODE_PROGRAM_ERROR;
		}

		/*g_debug( "%s: gconf=%p, code=%u, path=%s", thisfn, ( void * ) gconf, code, path );*/

		g_free( msg );
		g_free( path );
		g_free( this_path );
		g_free( this_id );
	}

	return( code );
}

guint
nagp_writer_write_done( const FMAIFactoryProvider *writer, void *writer_data,
									const FMAIFactoryObject *object,
									GSList **messages  )
{
	return( IIO_PROVIDER_CODE_OK );
}
#endif /* NA_ENABLE_DEPRECATED */
