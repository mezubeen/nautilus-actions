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

#ifndef __NACT_IIO_PROVIDER_H__
#define __NACT_IIO_PROVIDER_H__

/*
 * NactIIOProvider interface definition.
 *
 * This is the API all storage subsystems should implement in order to
 * provide i/o resources to NautilusActions.
 *
 * In a near or far future, provider subsystems may be extended by
 * creating extension libraries, this class loading the modules at
 * startup time (e.g. on the model of provider interfaces in Nautilus).
 */

#include "glib-object.h"

G_BEGIN_DECLS

#define NACT_IIO_PROVIDER_TYPE						( nact_iio_provider_get_type())
#define NACT_IIO_PROVIDER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_IIO_PROVIDER_TYPE, NactIIOProvider ))
#define NACT_IS_IIO_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_IIO_PROVIDER_TYPE ))
#define NACT_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NACT_IIO_PROVIDER_TYPE, NactIIOProviderInterface ))

typedef struct NactIIOProvider NactIIOProvider;

typedef struct NactIIOProviderInterfacePrivate NactIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                   parent;
	NactIIOProviderInterfacePrivate *private;

	/* i/o api */
	GSList * ( *load_actions )( NactIIOProvider *instance );
}
	NactIIOProviderInterface;

GType   nact_iio_provider_get_type( void );

GSList *nact_iio_provider_load_actions( const GObject *pivot );

G_END_DECLS

#endif /* __NACT_IIO_PROVIDER_H__ */
