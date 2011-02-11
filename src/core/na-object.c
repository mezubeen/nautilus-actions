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

#include <api/na-object-api.h>

#include "na-factory-object.h"

/* private class data
 */
struct _NAObjectClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NAObjectPrivate {
	gboolean   dispose_has_run;
};

/* while iterating on the class hierarchy for are_equal() and is_valid()
 */
typedef struct {
	NAObject *object;
	gboolean  result;
}
	HierarchyIter;

/* while iterating on the class hierarchy for object_copy()
 */
typedef struct {
	NAObject *target;
	gboolean  recursive;
}
	CopyIter;

typedef gboolean ( *HierarchyIterFunc )( GObjectClass *class, const NAObject *object, void *user_data );

static GObjectClass *st_parent_class   = NULL;

static GType    register_type( void );
static void     class_init( NAObjectClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     object_dump( const NAObject *object );

static void     iduplicable_iface_init( NAIDuplicableInterface *iface );
static void     iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source );
static gboolean iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b );
static gboolean iduplicable_are_equal_iter( GObjectClass *class, const NAObject *a, HierarchyIter *str );
static gboolean iduplicable_is_valid( const NAIDuplicable *object );
static gboolean iduplicable_is_valid_iter( GObjectClass *class, const NAObject *a, HierarchyIter *str );

static void     check_status_down_rec( const NAObject *object );
static void     check_status_up_rec( const NAObject *object, gboolean was_modified, gboolean was_valid );
static gboolean object_copy_iter( GObjectClass *class, const NAObject *source, CopyIter *data );
static void     dump_tree( GList *tree, gint level );
static void     iter_on_class_hierarchy( const NAObject *object, HierarchyIterFunc pfn, void *user_data );
static GList   *build_class_hierarchy( const NAObject *object );

GType
na_object_object_get_type( void )
{
	static GType item_type = 0;

	if( item_type == 0 ){
		item_type = register_type();
	}

	return( item_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_object_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAObjectClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObject ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iduplicable_iface_info = {
		( GInterfaceInitFunc ) iduplicable_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAObject", &info, 0 );

	g_type_add_interface_static( type, NA_IDUPLICABLE_TYPE, &iduplicable_iface_info );

	return( type );
}

static void
class_init( NAObjectClass *klass )
{
	static const gchar *thisfn = "na_object_class_init";
	GObjectClass *object_class;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->copy = NULL;
	naobject_class->are_equal = NULL;
	naobject_class->is_valid = NULL;

	klass->private = g_new0( NAObjectClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_object_instance_init";
	NAObject *self;

	g_return_if_fail( NA_IS_OBJECT( instance ));

	self = NA_OBJECT( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( NAObjectPrivate, 1 );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_object_instance_dispose";
	NAObject *self;

	g_return_if_fail( NA_IS_OBJECT( object ));

	self = NA_OBJECT( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		na_iduplicable_dispose( NA_IDUPLICABLE( object ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_object_instance_finalize";
	NAObject *self;

	g_return_if_fail( NA_IS_OBJECT( object ));

	self = NA_OBJECT( object );

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_free( self->private );

	if( NA_IS_IFACTORY_OBJECT( object )){
		na_factory_object_finalize( NA_IFACTORY_OBJECT( object ));
	}

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_dump( const NAObject *object )
{
	if( !object->private->dispose_has_run ){

		na_iduplicable_dump( NA_IDUPLICABLE( object ));

		if( NA_IS_IFACTORY_OBJECT( object )){
			na_factory_object_dump( NA_IFACTORY_OBJECT( object ));
		}
	}
}

static void
iduplicable_iface_init( NAIDuplicableInterface *iface )
{
	static const gchar *thisfn = "na_object_iduplicable_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->copy = iduplicable_copy;
	iface->are_equal = iduplicable_are_equal;
	iface->is_valid = iduplicable_is_valid;
}

/*
 * implementation of na_iduplicable::copy interface virtual function
 * it recursively copies @source to @target
 */
static void
iduplicable_copy( NAIDuplicable *target, const NAIDuplicable *source )
{
	na_object_copy( NA_OBJECT( target ), NA_OBJECT( source ), TRUE );
}

static gboolean
iduplicable_are_equal( const NAIDuplicable *a, const NAIDuplicable *b )
{
	static const gchar *thisfn = "na_object_iduplicable_are_equal";
	gboolean are_equal;
	HierarchyIter *str;

	g_return_val_if_fail( NA_IS_OBJECT( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT( b ), FALSE );

	are_equal = FALSE;

	if( !NA_OBJECT( a )->private->dispose_has_run &&
		!NA_OBJECT( b )->private->dispose_has_run ){

		g_debug( "%s: a=%p (%s), b=%p", thisfn, ( void * ) a, G_OBJECT_TYPE_NAME( a ), ( void * ) b );

		if( NA_IS_IFACTORY_OBJECT( a )){
			are_equal = na_factory_object_are_equal( NA_IFACTORY_OBJECT( a ), NA_IFACTORY_OBJECT( b ));

		} else {
			str = g_new0( HierarchyIter, 1 );
			str->object = NA_OBJECT( b );
			str->result = FALSE;

			iter_on_class_hierarchy( NA_OBJECT( a ), ( HierarchyIterFunc ) &iduplicable_are_equal_iter, str );

			are_equal = str->result;

			g_free( str );
		}
	}

	return( are_equal );
}

static gboolean
iduplicable_are_equal_iter( GObjectClass *class, const NAObject *a, HierarchyIter *str )
{
	gboolean stop = FALSE;

	if( NA_OBJECT_CLASS( class )->are_equal ){
		str->result = NA_OBJECT_CLASS( class )->are_equal( a, str->object );
		stop = !str->result;
	}

	return( stop );
}

static gboolean
iduplicable_is_valid( const NAIDuplicable *object )
{
	static const gchar *thisfn = "na_object_iduplicable_is_valid";
	gboolean is_valid;
	HierarchyIter *str;

	g_return_val_if_fail( NA_IS_OBJECT( object ), FALSE );

	is_valid = FALSE;

	if( !NA_OBJECT( object )->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		if( NA_IS_IFACTORY_OBJECT( object )){
			is_valid = na_factory_object_is_valid( NA_IFACTORY_OBJECT( object ));

		} else {
			g_debug( "%s: object=%p (%s): iterating on class hierarchy",
					thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

			str = g_new0( HierarchyIter, 1 );
			str->result = TRUE;
			iter_on_class_hierarchy( NA_OBJECT( object ), ( HierarchyIterFunc ) &iduplicable_is_valid_iter, str );
			is_valid = str->result;
			g_free( str );
		}
	}

	return( is_valid );
}

static gboolean
iduplicable_is_valid_iter( GObjectClass *class, const NAObject *a, HierarchyIter *str )
{
	gboolean stop = FALSE;

	if( NA_OBJECT_CLASS( class )->is_valid ){

		str->result = NA_OBJECT_CLASS( class )->is_valid( a );
		stop = !str->result;
	}

	return( stop );
}

/**
 * na_object_object_check_status_rec:
 * @object: the #NAObject -derived object to be checked.
 *
 * Recursively checks for the edition status of @object and its children
 * (if any).
 *
 * Internally set some properties which may be requested later. This
 * two-steps check-request let us optimize some work in the UI.
 *
 * na_object_object_check_status_rec( object )
 *  +- na_iduplicable_check_status( object )
 *      +- get_origin( object )
 *      +- modified_status = v_are_equal( origin, object ) -> interface <structfield>NAObjectClass::are_equal</structfield>
 *      +- valid_status = v_is_valid( object )             -> interface <structfield>NAObjectClass::is_valid</structfield>
 *
 * Note that the recursivity is managed here, so that we can be sure
 * that edition status of children is actually checked before those of
 * the parent.
 *
 * As of 3.1.0:
 * - when the modification status of a NAObjectProfile changes, then its
 *   NAObjectAction parent is rechecked;
 * - when the validity status of an object is changed, then its parent is
 *   also rechecked.
 *
 * Since: 2.30
 */
void
na_object_object_check_status_rec( const NAObject *object )
{
	static const gchar *thisfn = "na_object_object_check_status_rec";
	gboolean was_modified, was_valid;

	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		was_modified = na_object_is_modified( object );
		was_valid = na_object_is_valid( object );
		check_status_down_rec( object );
		check_status_up_rec( object, was_modified, was_valid );
	}
}

/*
 * recursively checks the status downstream
 */
static void
check_status_down_rec( const NAObject *object )
{
	if( NA_IS_OBJECT_ITEM( object )){
		g_list_foreach( na_object_get_items( object ), ( GFunc ) check_status_down_rec, NULL );
	}

	na_iduplicable_check_status( NA_IDUPLICABLE( object ));
}

/*
 * if the status appears changed, then rechecks the parent
 * recurse upstream while there is a parent, and its status changes
 */
static void
check_status_up_rec( const NAObject *object, gboolean was_modified, gboolean was_valid )
{
	gboolean is_modified, is_valid;
	NAObjectItem *parent;

	is_modified = na_object_is_modified( object );
	is_valid = na_object_is_valid( object );

	if(( NA_IS_OBJECT_PROFILE( object ) && was_modified != is_modified ) ||
			was_valid != is_valid ){

			parent = na_object_get_parent( object );

			if( parent ){
				was_modified = na_object_is_modified( parent );
				was_valid = na_object_is_valid( parent );
				na_iduplicable_check_status( NA_IDUPLICABLE( parent ));
				check_status_up_rec( NA_OBJECT( parent ), was_modified, was_valid );
			}
	}
}

/**
 * na_object_object_copy:
 * @target: the target #NAObject -derived object.
 * @source: the source #NAObject -derived object.
 * @recursive: whether the copy should be recursive.
 *
 * Copies @source to @target.
 *
 * Since: 2.30
 */
void
na_object_object_copy( NAObject *target, const NAObject *source, gboolean recursive )
{
	CopyIter *data;

	g_return_if_fail( NA_IS_OBJECT( target ));
	g_return_if_fail( NA_IS_OBJECT( source ));

	if( !NA_OBJECT( source )->private->dispose_has_run &&
		!NA_OBJECT( target )->private->dispose_has_run ){

		data = g_new0( CopyIter, 1 );
		data->target = target;
		data->recursive = recursive;

		iter_on_class_hierarchy( NA_OBJECT( source ),
					( HierarchyIterFunc ) object_copy_iter, ( void * ) data );

		g_free( data );
	}
}

static gboolean
object_copy_iter( GObjectClass *class, const NAObject *source, CopyIter *data )
{
	gboolean stop = FALSE;

	if( NA_OBJECT_CLASS( class )->copy ){
		NA_OBJECT_CLASS( class )->copy( data->target, source, data->recursive );
	}

	return( stop );
}

/**
 * na_object_object_dump:
 * @object: the #NAObject -derived object to be dumped.
 *
 * Dumps via g_debug() the actual content of the object.
 *
 * The recursivity is dealt with here because, if we would let
 * #NAObjectItem do this, the dump of #NAObjectItem -derived object
 * would be splitted, children being inserted inside.
 *
 * na_object_dump() doesn't modify the reference count of the dumped
 * object.
 *
 * Since: 2.30
 */
void
na_object_object_dump( const NAObject *object )
{
	GList *children, *ic;

	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){

		na_object_dump_norec( object );

		if( NA_IS_OBJECT_ITEM( object )){
			children = na_object_get_items( object );

			for( ic = children ; ic ; ic = ic->next ){
				na_object_dump( ic->data );
			}
		}
	}
}

/**
 * na_object_object_dump_norec:
 * @object: the #NAObject -derived object to be dumped.
 *
 * Dumps via g_debug the actual content of the object.
 *
 * This function is not recursive.
 *
 * Since: 2.30
 */
void
na_object_object_dump_norec( const NAObject *object )
{
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run ){
		if( NA_OBJECT_GET_CLASS( object )->dump ){
			NA_OBJECT_GET_CLASS( object )->dump( object );
		}
	}
}

/**
 * na_object_object_dump_tree:
 * @tree: a hierarchical list of #NAObject -derived objects.
 *
 * Outputs a brief, hierarchical dump of the provided list.
 *
 * Since: 2.30
 */
void
na_object_object_dump_tree( GList *tree )
{
	dump_tree( tree, 0 );
}

static void
dump_tree( GList *tree, gint level )
{
	GString *prefix;
	gint i;
	GList *it;
	const NAObject *object;
	gchar *label;

	prefix = g_string_new( "" );
	for( i = 0 ; i < level ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	for( it = tree ; it ; it = it->next ){
		object = ( const NAObject * ) it->data;
		label = na_object_get_label( object );
		g_debug( "na_object_dump_tree: %s%p (%s, ref_count=%u) '%s'", prefix->str,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count, label );
		g_free( label );

		if( NA_IS_OBJECT_ITEM( object )){
			dump_tree( na_object_get_items( object ), level+1 );
		}
	}

	g_string_free( prefix, TRUE );
}

/**
 * na_object_object_reset_origin:
 * @object: a #NAObject -derived object.
 * @origin: must be a duplication of @object.
 *
 * Recursively reset origin of @object and its children to @origin (and
 * its children), so that @origin appears as the actual origin of @object.
 *
 * The origin of @origin itself is set to NULL.
 *
 * This only works if @origin has just been duplicated from @object,
 * and thus we do not have to check if children lists are equal.
 *
 * Since: 2.30
 */
void
na_object_object_reset_origin( NAObject *object, const NAObject *origin )
{
	GList *origin_children, *iorig;
	GList *object_children, *iobj;

	g_return_if_fail( NA_IS_OBJECT( origin ));
	g_return_if_fail( NA_IS_OBJECT( object ));

	if( !object->private->dispose_has_run && !origin->private->dispose_has_run ){

		origin_children = na_object_get_items( origin );
		object_children = na_object_get_items( object );

		for( iorig = origin_children, iobj = object_children ; iorig && iobj ; iorig = iorig->next, iobj = iobj->next ){
			na_object_reset_origin( iobj->data, iorig->data );
		}

		na_iduplicable_set_origin( NA_IDUPLICABLE( object ), NA_IDUPLICABLE( origin ));
		na_iduplicable_set_origin( NA_IDUPLICABLE( origin ), NULL );
	}
}

/**
 * na_object_object_ref:
 * @object: a #NAObject -derived object.
 *
 * Recursively ref the @object and all its children, incrementing their
 * reference_count by 1.
 *
 * Returns: a reference on the @object.
 *
 * Since: 2.30
 */
NAObject *
na_object_object_ref( NAObject *object )
{
	NAObject *ref;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	ref = NULL;

	if( !object->private->dispose_has_run ){

		if( NA_IS_OBJECT_ITEM( object )){
			g_list_foreach( na_object_get_items( object ), ( GFunc ) na_object_object_ref, NULL );
		}

		ref = g_object_ref( object );
	}

	return( ref );
}

/**
 * na_object_object_unref:
 * @object: a #NAObject -derived object.
 *
 * Recursively unref the @object and all its children, decrementing their
 * reference_count by 1.
 *
 * Note that we may want to free a copy+ref of a list of items whichy have
 * had already disposed (which is probably a bug somewhere). So first test
 * is the object is still alive.
 *
 * Since: 2.30
 */
void
na_object_object_unref( NAObject *object )
{
	if( NA_IS_OBJECT( object )){
		if( !object->private->dispose_has_run ){
			if( NA_IS_OBJECT_ITEM( object )){
				g_list_foreach( na_object_get_items( object ), ( GFunc ) na_object_object_unref, NULL );
			}
			g_object_unref( object );
		}
	}
}

/*
 * iter on the whole class hierarchy
 * starting with NAObject, and to the most derived up to NAObjectAction..
 */
static void
iter_on_class_hierarchy( const NAObject *object, HierarchyIterFunc pfn, void *user_data )
{
	gboolean stop;
	GObjectClass *class;
	GList *hierarchy, *ih;

	stop = FALSE;
	hierarchy = build_class_hierarchy( object );

	for( ih = hierarchy ; ih && !stop ; ih = ih->next ){
		class = ( GObjectClass * ) ih->data;
		/*g_debug( "iter_on_class_hierarchy: class=%s", G_OBJECT_CLASS_NAME( class ));*/
		stop = ( *pfn )( class, object, user_data );
	}

	g_list_free( hierarchy );
}

/*
 * build the class hierarchy
 * returns a list of GObjectClass, which starts with NAObject,
 * and to with the most derived class (e.g. NAObjectAction or so)
 */
static GList *
build_class_hierarchy( const NAObject *object )
{
	GObjectClass *class;
	GList *hierarchy;

	hierarchy = NULL;
	class = G_OBJECT_GET_CLASS( object );

	while( G_OBJECT_CLASS_TYPE( class ) != NA_OBJECT_TYPE ){

		hierarchy = g_list_prepend( hierarchy, class );
		class = g_type_class_peek_parent( class );
	}

	hierarchy = g_list_prepend( hierarchy, class );

	return( hierarchy );
}

/**
 * na_object_object_get_hierarchy:
 * @object: the #NAObject -derived object.
 *
 * Returns: the class hierarchy,
 * from the topmost base class, to the most-derived one.
 *
 * Since: 2.30
 */
GList *
na_object_object_get_hierarchy( const NAObject *object )
{
	GList *hierarchy;

	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	hierarchy = NULL;

	if( !object->private->dispose_has_run ){

		hierarchy = build_class_hierarchy( object );
	}

	return( hierarchy );
}

/**
 * na_object_free_hierarchy:
 * @hierarchy: the #GList of hierarchy, as returned from
 *  na_object_get_hierarchy().
 *
 * Releases the #NAObject hierarchy.
 *
 * Since: 2.30
 */
void
na_object_free_hierarchy( GList *hierarchy )
{
	g_list_free( hierarchy );
}

/**
 * na_object_object_debug_invalid:
 * @object: the #NAObject -derived object which is invalid.
 * @reason: the reason.
 *
 * Dump the object with the invalidity reason.
 *
 * Since: 2.30
 */
void
na_object_object_debug_invalid( const NAObject *object, const gchar *reason )
{
	g_debug( "na_object_object_debug_invalid: object %p (%s) is marked invalid for reason \"%s\"",
			( void * ) object, G_OBJECT_TYPE_NAME( object ), reason );
}
