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

#ifndef __IO_DESKTOP_FMA_DESKTOP_READER_H__
#define __IO_DESKTOP_FMA_DESKTOP_READER_H__

#include <api/fma-iio-provider.h>
#include <api/fma-iimporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

GList        *fma_desktop_reader_iio_provider_read_items     ( const FMAIIOProvider *provider, GSList **messages );

guint         fma_desktop_reader_iimporter_import_from_uri   ( const FMAIImporter *instance, void *parms_ptr );

void          fma_desktop_reader_ifactory_provider_read_start( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *serializable, GSList **messages );
FMADataBoxed *fma_desktop_reader_ifactory_provider_read_data ( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *serializable, const FMADataDef *iddef, GSList **messages );
void          fma_desktop_reader_ifactory_provider_read_done ( const FMAIFactoryProvider *reader, void *reader_data, const FMAIFactoryObject *serializable, GSList **messages );

G_END_DECLS

#endif /* __IO_DESKTOP_FMA_DESKTOP_READER_H__ */
