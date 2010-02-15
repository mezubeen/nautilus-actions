/*
 * Nautilus ObjectActions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-object-api.h>

#include "na-io-factory.h"
#include "na-data-factory.h"

/* private class data
 */
struct NAObjectActionClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NAObjectActionPrivate {
	gboolean dispose_has_run;
};

										/* i18n: default label for a new action */
#define NEW_NAUTILUS_ACTION				N_( "New Nautilus action" )

extern NadfIdGroup action_id_groups [];	/* defined in na-item-action-enum.c */

static NAObjectItemClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NAObjectActionClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     idata_factory_iface_init( NAIDataFactoryInterface *iface );
static guint    idata_factory_get_version( const NAIDataFactory *instance );
static gchar   *idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef );
static void     idata_factory_copy( NAIDataFactory *target, const NAIDataFactory *source );
static gboolean idata_factory_are_equal( const NAIDataFactory *a, const NAIDataFactory *b );
static void     idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages );
static void     idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages );

GType
na_object_action_get_type( void )
{
	static GType action_type = 0;

	if( action_type == 0 ){

		action_type = register_type();
	}

	return( action_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_action_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectActionClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectAction ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo idata_factory_iface_info = {
		( GInterfaceInitFunc ) idata_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NA_OBJECT_ITEM_TYPE, "NAObjectAction", &info, 0 );

	g_type_add_interface_static( type, NA_IDATA_FACTORY_TYPE, &idata_factory_iface_info );

	na_io_factory_register( type, action_id_groups );

	return( type );
}

static void
class_init( NAObjectActionClass *klass )
{
	static const gchar *thisfn = "na_object_action_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = NULL;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( NAObjectActionClassPrivate, 1 );

	na_data_factory_properties( object_class );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_action_instance_init";
	NAObjectAction *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	g_return_if_fail( NA_IS_OBJECT_ACTION( instance ));

	self = NA_OBJECT_ACTION( instance );

	self->private = g_new0( NAObjectActionPrivate, 1 );

	na_data_factory_init( NA_IDATA_FACTORY( instance ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_ACTION( object )->private->dispose_has_run ){

		na_data_factory_set_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));
	g_return_if_fail( NA_IS_IDATA_FACTORY( object ));

	if( !NA_OBJECT_ACTION( object )->private->dispose_has_run ){

		na_data_factory_get_value( NA_IDATA_FACTORY( object ), property_id, value, spec );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_action_instance_dispose";
	NAObjectAction *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));

	self = NA_OBJECT_ACTION( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_object_action_instance_finalize";
	NAObjectAction *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));

	self = NA_OBJECT_ACTION( object );

	g_free( self->private );

	na_data_factory_finalize( NA_IDATA_FACTORY( object ));

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
idata_factory_iface_init( NAIDataFactoryInterface *iface )
{
	static const gchar *thisfn = "na_object_action_idata_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = idata_factory_get_version;
	iface->get_default = idata_factory_get_default;
	iface->copy = idata_factory_copy;
	iface->are_equal = idata_factory_are_equal;
	iface->read_done = idata_factory_read_done;
	iface->write_done = idata_factory_write_done;
}

static guint
idata_factory_get_version( const NAIDataFactory *instance )
{
	return( 1 );
}

static gchar *
idata_factory_get_default( const NAIDataFactory *instance, const NadfIdType *iddef )
{
	gchar *value;

	value = NULL;

	switch( iddef->id ){

		case NADF_DATA_LABEL:
		case NADF_DATA_TOOLBAR_LABEL:
			value = g_strdup( NEW_NAUTILUS_ACTION );
			break;
	}

	return( value );
}

static void
idata_factory_copy( NAIDataFactory *target, const NAIDataFactory *source )
{
	na_object_item_copy( NA_OBJECT_ITEM( target ), NA_OBJECT_ITEM( source ));
}

static gboolean
idata_factory_are_equal( const NAIDataFactory *a, const NAIDataFactory *b )
{
	return( na_object_item_are_equal( NA_OBJECT_ITEM( a ), NA_OBJECT_ITEM( b )));
}

static void
idata_factory_read_done( NAIDataFactory *instance, const NAIIOFactory *reader, void *reader_data, GSList **messages )
{
	g_debug( "na_object_action_idata_factory_read_done: instance=%p", ( void * ) instance );

	na_object_dump( instance );
}

static void
idata_factory_write_done( NAIDataFactory *instance, const NAIIOFactory *writer, void *writer_data, GSList **messages )
{

}

/**
 * na_object_action_new:
 *
 * Allocates a new #NAObjectAction object.
 *
 * The new #NAObjectAction object is initialized with suitable default values,
 * but without any profile.
 *
 * Returns: the newly allocated #NAObjectAction object.
 */
NAObjectAction *
na_object_action_new( void )
{
	NAObjectAction *action;

	action = g_object_new( NA_OBJECT_ACTION_TYPE, NULL );

	return( action );
}

/**
 * na_object_action_new_with_profile:
 *
 * Allocates a new #NAObjectAction object along with a default profile.
 *
 * Returns: the newly allocated #NAObjectAction action.
 */
NAObjectAction *
na_object_action_new_with_profile( void )
{
	NAObjectAction *action;
	NAObjectProfile *profile;

	action = na_object_action_new();

	profile = na_object_profile_new();
	na_object_action_attach_profile( action, profile );

	return( action );
}

/**
 * na_object_action_attach_profile:
 * @action: the #NAObjectAction action to which the profile will be attached.
 * @profile: the #NAObjectProfile profile to be attached to @action.
 *
 * Adds a profile at the end of the list of profiles.
 */
void
na_object_action_attach_profile( NAObjectAction *action, NAObjectProfile *profile )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( action ));
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !action->private->dispose_has_run ){

		na_object_append_item( action, profile );
		na_object_set_parent( profile, action );
	}
}

/**
 * na_object_action_is_candidate:
 * @action: the #NAObjectAction to be tested.
 * @target: the current target.
 *
 * Returns: %TRUE if the @action may be candidate for this @target.
 */
gboolean
na_object_action_is_candidate( const NAObjectAction *action, gint target )
{
	gboolean is_candidate = FALSE;

#if 0
	g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), is_candidate );

	if( !action->private->dispose_has_run ){

		is_candidate =
			( action->private->target_selection && target == ITEM_TARGET_SELECTION ) ||
			( action->private->target_background && target == ITEM_TARGET_BACKGROUND ) ||
			( action->private->target_toolbar && target == ITEM_TARGET_TOOLBAR );
	}
#endif

	return( is_candidate );
}
