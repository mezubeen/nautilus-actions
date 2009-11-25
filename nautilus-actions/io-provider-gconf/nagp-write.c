/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

/* only possible because we are an internal plugin */
#include <runtime/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-write.h"
#include "nagp-keys.h"

static gboolean       write_item_action( NagpGConfProvider *gconf, const NAObjectAction *action, GSList **message );
static gboolean       write_item_menu( NagpGConfProvider *gconf, const NAObjectMenu *menu, GSList **message );
static gboolean       write_object_item( NagpGConfProvider *gconf, const NAObjectItem *item, GSList **message );

static gboolean       write_str( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **message );
static gboolean       write_bool( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **message );
static gboolean       write_list( NagpGConfProvider *gconf, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **message );
static void           free_gslist( GSList *list );

/*
 * always returns TRUE because user _should_ be able to write its own
 * configurations into ~/.gconf
 */
gboolean
nagp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	static const gchar *thisfn = "nagp_iio_provider_is_willing_to_write";
	NagpGConfProvider *self;
	gboolean willing_to = FALSE;

	g_debug( "%s: provider=%p", thisfn, ( void * ) provider );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );
	self = NAGP_GCONF_PROVIDER( provider );

	/* TODO: nagp_gconf_provider_iio_provider_is_willing_to_write */
	if( !self->private->dispose_has_run ){
		willing_to = TRUE;
	}

	return( willing_to );
}

/*
 *
 */
gboolean
nagp_iio_provider_is_writable( const NAIIOProvider *provider, const NAObjectItem *item )
{
	NagpGConfProvider *self;
	gboolean willing_to = FALSE;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), FALSE );
	self = NAGP_GCONF_PROVIDER( provider );

	/* TODO: nagp_gconf_provider_iio_provider_is_writable */
	if( !self->private->dispose_has_run ){
		willing_to = TRUE;
	}

	return( willing_to );
}

/*
 *
 */
guint
nagp_iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_write_item";
	NagpGConfProvider *self;

	g_debug( "%s: provider=%p, item=%p (%s), messages=%p",
			thisfn, ( void * ) provider,
			( void * ) item, G_OBJECT_TYPE_NAME( item ), ( void * ) messages );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	if( NA_IS_OBJECT_ACTION( item )){
		if( !write_item_action( self, NA_OBJECT_ACTION( item ), messages )){
			return( NA_IIO_PROVIDER_WRITE_ERROR );
		}
	}

	if( NA_IS_OBJECT_MENU( item )){
		if( !write_item_menu( self, NA_OBJECT_MENU( item ), messages )){
			return( NA_IIO_PROVIDER_WRITE_ERROR );
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( NA_IIO_PROVIDER_WRITE_OK );
}

static gboolean
write_item_action( NagpGConfProvider *provider, const NAObjectAction *action, GSList **messages )
{
	gchar *uuid, *name;
	gboolean ret;
	GList *profiles, *ip;
	NAObjectProfile *profile;

	uuid = na_object_get_id( action );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( action ), messages ) &&
		write_str( provider, uuid, NULL, ACTION_VERSION_ENTRY, na_object_action_get_version( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_SELECTION_ENTRY, na_object_action_is_target_selection( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_BACKGROUND_ENTRY, na_object_action_is_target_background( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TARGET_TOOLBAR_ENTRY, na_object_action_is_target_toolbar( action ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_TOOLBAR_SAME_LABEL_ENTRY, na_object_action_toolbar_use_same_label( action ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TOOLBAR_LABEL_ENTRY, na_object_action_toolbar_get_label( action ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TYPE_ENTRY, g_strdup( OBJECT_ITEM_TYPE_ACTION ), messages );

	profiles = na_object_get_items_list( action );

	for( ip = profiles ; ip && ret ; ip = ip->next ){

		profile = NA_OBJECT_PROFILE( ip->data );
		name = na_object_get_id( profile );

		ret =
			write_str( provider, uuid, name, ACTION_PROFILE_LABEL_ENTRY, na_object_get_label( profile ), messages ) &&
			write_str( provider, uuid, name, ACTION_PATH_ENTRY, na_object_profile_get_path( profile ), messages ) &&
			write_str( provider, uuid, name, ACTION_PARAMETERS_ENTRY, na_object_profile_get_parameters( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_BASENAMES_ENTRY, na_object_profile_get_basenames( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_MATCHCASE_ENTRY, na_object_profile_get_matchcase( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_MIMETYPES_ENTRY, na_object_profile_get_mimetypes( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_ISFILE_ENTRY, na_object_profile_get_is_file( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_ISDIR_ENTRY, na_object_profile_get_is_dir( profile ), messages ) &&
			write_bool( provider, uuid, name, ACTION_MULTIPLE_ENTRY, na_object_profile_get_multiple( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_SCHEMES_ENTRY, na_object_profile_get_schemes( profile ), messages ) &&
			write_list( provider, uuid, name, ACTION_FOLDERS_ENTRY, na_object_profile_get_folders( profile ), messages );

		g_free( name );
	}

	g_free( uuid );

	return( ret );
}

static gboolean
write_item_menu( NagpGConfProvider *provider, const NAObjectMenu *menu, GSList **messages )
{
	gboolean ret;
	gchar *uuid;

	uuid = na_object_get_id( menu );

	ret =
		write_object_item( provider, NA_OBJECT_ITEM( menu ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TYPE_ENTRY, g_strdup( OBJECT_ITEM_TYPE_MENU ), messages );

	g_free( uuid );

	return( ret );
}

static gboolean
write_object_item( NagpGConfProvider *provider, const NAObjectItem *item, GSList **messages )
{
	gchar *uuid;
	gboolean ret;

	uuid = na_object_get_id( NA_OBJECT( item ));

	ret =
		write_str( provider, uuid, NULL, OBJECT_ITEM_LABEL_ENTRY, na_object_get_label( NA_OBJECT( item )), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_TOOLTIP_ENTRY, na_object_get_tooltip( item ), messages ) &&
		write_str( provider, uuid, NULL, OBJECT_ITEM_ICON_ENTRY, na_object_get_icon( item ), messages ) &&
		write_bool( provider, uuid, NULL, OBJECT_ITEM_ENABLED_ENTRY, na_object_is_enabled( item ), messages ) &&
		write_list( provider, uuid, NULL, OBJECT_ITEM_LIST_ENTRY, na_object_item_rebuild_items_list( item ), messages );

	g_free( uuid );
	return( ret );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
guint
nagp_iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_delete_item";
	NagpGConfProvider *self;
	guint ret;
	gchar *uuid, *path;
	GError *error = NULL;

	g_debug( "%s: provider=%p, item=%p (%s), messages=%p",
			thisfn, ( void * ) provider,
			( void * ) item, G_OBJECT_TYPE_NAME( item ), ( void * ) messages );

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NA_IIO_PROVIDER_PROGRAM_ERROR );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), NA_IIO_PROVIDER_PROGRAM_ERROR );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE );
	}

	ret = NA_IIO_PROVIDER_WRITE_OK;
	uuid = na_object_get_id( NA_OBJECT( item ));

	path = g_strdup_printf( "%s%s/%s", NAUTILUS_ACTIONS_GCONF_SCHEMASDIR, NA_GCONF_CONFIG_PATH, uuid );
	gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		*messages = g_slist_append( *messages, g_strdup( error->message ));
		g_error_free( error );
		error = NULL;
	}
	g_free( path );


	path = g_strdup_printf( "%s/%s", NA_GCONF_CONFIG_PATH, uuid );
	if( !gconf_client_recursive_unset( self->private->gconf, path, 0, &error )){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		*messages = g_slist_append( *messages, g_strdup( error->message ));
		g_error_free( error );
		error = NULL;
		ret = NA_IIO_PROVIDER_WRITE_ERROR;

	} else {
		gconf_client_suggest_sync( self->private->gconf, NULL );
	}
	g_free( path );

	g_free( uuid );

	return( ret );
}

/*
 * gconf_client_key_is_writable doesn't work as I expect: it returns
 * FALSE without error for our keys !
 * So I have to actually try a fake write to the key to get the real
 * writability status...
 *
 * TODO: having a NAPivot as Nautilus extension and another NAPivot in
 * the UI leads to a sort of notification loop: extension's pivot is
 * not aware of UI's one, and so get notifications from GConf, reloads
 * the list of actions, retrying to test the writability.
 * In the meanwhile, UI's pivot has reauthorized notifications, and is
 * notified of extension's pivot tests, and so on......
 *
 * -> Nautilus extension's pivot should not test for writability, as it
 *    uses actions as read-only, reloading the whole list when one
 *    action is modified ; this can break the loop
 *
 * -> the UI may use the pivot inside of Nautilus extension via a sort
 *    of API, falling back to its own pivot, when the extension is not
 *    present.
 *
 * 2009-11-25 - it appears that gconf_client_key_is_writable() returns
 * - FALSE for an existant dir (provided 'path')
 * - FALSE for a non-existant entry
 * - TRUE for an existant entry
 */
gboolean
nagp_key_is_writable( NagpGConfProvider *gconf, const gchar *path )
{
	static const gchar *thisfn = "nagp_gconf_provider_key_is_writable";
	GError *error = NULL;
	gboolean is_writable;
	gchar *key;

	g_debug( "%s: gconf=%p, path=%s", thisfn, ( void * ) gconf, path );

	/*remove_gconf_watched_dir( gconf );*/

	key = gconf_concat_dir_and_key( path, "foo" );
	is_writable = gconf_client_key_is_writable( gconf->private->gconf, key, &error );
	g_debug( "%s: gconf_client_key_is_writable returns is_writable=%s", thisfn, is_writable ? "True":"False" );
	if( error ){
		g_warning( "%s: gconf_client_key_is_writable: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
		is_writable = FALSE;
	}
	g_free( key );

	/*gboolean ret_try = FALSE;
	gchar *path_try = g_strdup_printf( "%s/%s", path, "fake_key" );
	ret_try = gconf_client_set_string( gconf->private->gconf, path_try, "fake_value", &error );
	if( error ){
		g_warning( "%s: gconf_client_set_string: %s", thisfn, error->message );
		g_error_free( error );
		error = NULL;
	}
	if( ret_try ){
		gconf_client_unset( gconf->private->gconf, path_try, NULL );
	}
	g_free( path_try );
	g_debug( "%s: ret_gconf=%s, ret_try=%s", thisfn, ret_gconf ? "True":"False", ret_try ? "True":"False" );

	install_gconf_watched_dir( gconf );
	return( ret_try );*/

	return( is_writable );
}

static gboolean
write_str( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gchar *value, GSList **messages )
{
	gchar *path;
	gboolean ret;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( value );
	g_free( path );

	return( ret );
}

static gboolean
write_bool( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, gboolean value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_bool( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	g_free( path );

	return( ret );
}

static gboolean
write_list( NagpGConfProvider *provider, const gchar *uuid, const gchar *name, const gchar *key, GSList *value, GSList **messages )
{
	gboolean ret;
	gchar *path;
	gchar *msg;

	if( name && strlen( name )){
		path = g_strdup_printf( "%s/%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, name, key );
	} else {
		path = g_strdup_printf( "%s/%s/%s", NA_GCONF_CONFIG_PATH, uuid, key );
	}

	msg = NULL;
	ret = na_gconf_utils_write_string_list( provider->private->gconf, path, value, &msg );
	if( msg ){
		*messages = g_slist_append( *messages, g_strdup( msg ));
		g_free( msg );
	}

	free_gslist( value );
	g_free( path );

	return( ret );
}

/*
 * free_gslist:
 * @list: the GSList to be freed.
 *
 * Frees a GSList of strings.
 */
static void
free_gslist( GSList *list )
{
	g_slist_foreach( list, ( GFunc ) g_free, NULL );
	g_slist_free( list );
}
