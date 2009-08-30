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

#include "na-iduplicable.h"

/* private interface data
 */
struct NAIDuplicableInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* data set against NAIDuplicable-implementated instance
 */
#define PROP_IDUPLICABLE_ORIGIN			"na-iduplicable-origin"
#define PROP_IDUPLICABLE_ISMODIFIED		"na-iduplicable-is-modified"
#define PROP_IDUPLICABLE_ISVALID		"na-iduplicable-is-valid"

static GType          register_type( void );
static void           interface_base_init( NAIDuplicableInterface *klass );
static void           interface_base_finalize( NAIDuplicableInterface *klass );

static void           v_copy( NAIDuplicable *target, const NAIDuplicable *source );
static NAIDuplicable *v_new( const NAIDuplicable *object );
static gboolean       v_are_equal( const NAIDuplicable *a, const NAIDuplicable *b );
static gboolean       v_is_valid( const NAIDuplicable *object );

static NAIDuplicable *get_origin( const NAIDuplicable *object );
static void           set_origin( const NAIDuplicable *object, const NAIDuplicable *origin );
static gboolean       get_modified( const NAIDuplicable *object );
static void           set_modified( const NAIDuplicable *object, gboolean is_modified );
static gboolean       get_valid( const NAIDuplicable *object );
static void           set_valid( const NAIDuplicable *object, gboolean is_valid );

GType
na_iduplicable_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_iduplicable_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIDuplicableInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIDuplicable", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NAIDuplicableInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIDuplicableInterface *klass )
{
	static const gchar *thisfn = "na_iduplicable_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

/**
 * na_iduplicable_init:
 * @object: the #NAIDuplicable object to be initialized.
 *
 * Initializes the properties of a IDuplicable object.
 *
 * This function should be called by the implementor when creating the
 * object, e.g. from instance_init().
 */
void
na_iduplicable_init( NAIDuplicable *object )
{
	g_assert( G_IS_OBJECT( object ));
	g_assert( NA_IS_IDUPLICABLE( object ));

	set_origin( object, NULL );
	set_modified( object, FALSE );
	set_valid( object, TRUE );
}

/**
 * na_iduplicable_dump:
 * @object: the #NAIDuplicable object to be dumped.
 *
 * Dumps via g_debug the properties of the object.
 *
 * We ouput here only the data we set ourselves againt the
 * #NAIDuplicable-implemented object.
 */
void
na_iduplicable_dump( const NAIDuplicable *object )
{
	static const gchar *thisfn = "na_iduplicable_dump";
	NAIDuplicable *origin = NULL;
	gboolean modified = FALSE;
	gboolean valid = FALSE;				/* may a NULL object be valid ? */

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		origin = get_origin( object );
		modified = get_modified( object );
		valid = get_valid( object );
	}

	g_debug( "%s:   origin=%p", thisfn, ( void * ) origin );
	g_debug( "%s: modified=%s", thisfn, modified ? "True" : "False" );
	g_debug( "%s:    valid=%s", thisfn, valid ? "True" : "False" );
}

/**
 * na_iduplicable_duplicate:
 * @object: the #NAIDuplicable object to be duplicated.
 *
 * Exactly duplicates a #NAIDuplicable-implemented object.
 * Properties %PROP_IDUPLICABLE_ORIGIN, %PROP_IDUPLICABLE_ISMODIFIED
 * and %PROP_IDUPLICABLE_ISVALID are initialized to their default
 * values.
 *
 * As %PROP_IDUPLICABLE_ISVALID property is set to %TRUE without any
 * further check, this suppose that only valid objects are duplicated.
 *
 * Returns: a new #NAIDuplicable.
 */
NAIDuplicable *
na_iduplicable_duplicate( const NAIDuplicable *object )
{
	static const gchar *thisfn = "na_iduplicable_duplicate";
	NAIDuplicable *dup = NULL;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		dup = v_new( object );

		if( dup ){
			v_copy( dup, object );
			set_origin( dup, object );
			set_modified( dup, FALSE );
			set_valid( dup, TRUE );
		}
	}

	return( dup );
}

/**
 * na_iduplicable_check_edited_status:
 * @object: the #NAIDuplicable object to be checked.
 *
 * Checks the edition status of the #NAIDuplicable object, and set up
 * the corresponding %PROP_IDUPLICABLE_ISMODIFIED and
 * %PROP_IDUPLICABLE_ISVALID properties.
 *
 * This function is supposed to be called each time the object may have
 * been modified in order to set these properties. Helper functions
 * na_iduplicable_is_modified() and na_iduplicable_is_valid() will
 * then only return the current value of the properties.
 */
void
na_iduplicable_check_edited_status( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_check_edited_status";
	g_debug( "%s: object=%p", thisfn, object );*/
	gboolean modified = TRUE;
	NAIDuplicable *origin;
	gboolean valid;

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		origin = get_origin( object );
		if( origin ){
			modified = !v_are_equal( object, origin );
		}
		set_modified( object, modified );

		valid = v_is_valid( object );
		set_valid( object, valid );
	}
}

/**
 * na_iduplicable_is_modified:
 * @object: the #NAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISMODIFIED
 * property without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object has been modified regarding of
 * the original one.
 */
gboolean
na_iduplicable_is_modified( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_modified";
	g_debug( "%s: object=%p", thisfn, object );*/
	gboolean is_modified = FALSE;

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		is_modified = get_modified( object );
	}

	return( is_modified );
}

/**
 * na_iduplicable_is_valid:
 * @object: the #NAIDuplicable object whose status is to be returned.
 *
 * Returns the current value of the %PROP_IDUPLICABLE_ISVALID property
 * without rechecking the edition status itself.
 *
 * Returns: %TRUE is the provided object is valid.
 */
gboolean
na_iduplicable_is_valid( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/
	gboolean is_valid = FALSE;

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		is_valid = get_valid( object );
	}

	return( is_valid );
}

/**
 * na_iduplicable_get_origin:
 * @object: the #NAIDuplicable object whose origin is to be returned.
 *
 * Returns the origin of a duplicated #NAIDuplicable.
 *
 * Returns: the original #NAIDuplicable, or NULL.
 */
NAIDuplicable *
na_iduplicable_get_origin( const NAIDuplicable *object )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/
	NAIDuplicable *origin = NULL;

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		origin = get_origin( object );
	}

	return( origin );
}

/**
 * na_iduplicable_set_origin:
 * @object: the #NAIDuplicable object whose origin is to be returned.
 * @origin: the new original #NAIDuplicable.
 *
 * Sets the new origin of a duplicated #NAIDuplicable.
 */
void
na_iduplicable_set_origin( NAIDuplicable *object, const NAIDuplicable *origin )
{
	/*static const gchar *thisfn = "na_iduplicable_is_valid";
	g_debug( "%s: object=%p", thisfn, object );*/

	if( object ){
		g_assert( G_IS_OBJECT( object ));
		g_assert( NA_IS_IDUPLICABLE( object ));

		set_origin( object, origin );
	}
}

static void
v_copy( NAIDuplicable *target, const NAIDuplicable *source )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( target )->copy ){
		NA_IDUPLICABLE_GET_INTERFACE( target )->copy( target, source );
	}
}

static NAIDuplicable *
v_new( const NAIDuplicable *object )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( object )->new ){
		return( NA_IDUPLICABLE_GET_INTERFACE( object )->new( object ));
	}

	return( NULL );
}

static gboolean
v_are_equal( const NAIDuplicable *a, const NAIDuplicable *b )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( a )->are_equal ){
		return( NA_IDUPLICABLE_GET_INTERFACE( a )->are_equal( a, b ));
	}

	return( TRUE );
}

static gboolean
v_is_valid( const NAIDuplicable *object )
{
	if( NA_IDUPLICABLE_GET_INTERFACE( object )->is_valid ){
		return( NA_IDUPLICABLE_GET_INTERFACE( object )->is_valid( object ));
	}

	return( TRUE );
}

static NAIDuplicable *
get_origin( const NAIDuplicable *object )
{
	return( NA_IDUPLICABLE( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ORIGIN )));
}

static void
set_origin( const NAIDuplicable *object, const NAIDuplicable *origin )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ORIGIN, ( gpointer ) origin );
}

static gboolean
get_modified( const NAIDuplicable *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISMODIFIED )));
}

static void
set_modified( const NAIDuplicable *object, gboolean is_modified )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISMODIFIED, GUINT_TO_POINTER( is_modified ));
}

static gboolean
get_valid( const NAIDuplicable *object )
{
	return(( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISVALID )));
}

static void
set_valid( const NAIDuplicable *object, gboolean is_valid )
{
	g_object_set_data( G_OBJECT( object ), PROP_IDUPLICABLE_ISVALID, GUINT_TO_POINTER( is_valid ));
}
