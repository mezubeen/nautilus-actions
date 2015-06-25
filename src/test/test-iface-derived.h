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

#ifndef __TEST_IFACE_DERIVED_H__
#define __TEST_IFACE_DERIVED_H__

/**
 * SECTION: test_iface_derived
 * @short_description: #TestDerived class definition.
 * @include: test-iface-derived.h
 *
 * Derivation of TestBae class.
 * Are we able to define our own implementation of testIFace interface ?
 * Also, the derived class is it recognized as implementing the interface ?
 */

#include "test-iface-base.h"

G_BEGIN_DECLS

#define TEST_DERIVED_TYPE				( test_derived_get_type())
#define TEST_DERIVED( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, TEST_DERIVED_TYPE, TestDerived ))
#define TEST_DERIVED_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, TEST_DERIVED_TYPE, TestDerivedClass ))
#define TEST_IS_DERIVED( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, TEST_DERIVED_TYPE ))
#define TEST_IS_DERIVED_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), TEST_DERIVED_TYPE ))
#define TEST_DERIVED_GET_CLASS( object )( G_TYPE_INSTANCE_GET_CLASS(( object ), TEST_DERIVED_TYPE, TestDerivedClass ))

typedef struct TestDerivedPrivate TestDerivedPrivate;

typedef struct {
	TestBase            parent;
	TestDerivedPrivate *private;
}
	TestDerived;

typedef struct TestDerivedClassPrivate TestDerivedClassPrivate;

typedef struct {
	TestBaseClass            parent;
	TestDerivedClassPrivate *private;
}
	TestDerivedClass;

GType        test_derived_get_type( void );

TestDerived *test_derived_new( void );

G_END_DECLS

#endif /* __TEST_IFACE_DERIVED_H__ */
