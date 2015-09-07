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

#ifndef __NAXML_WRITER_H__
#define __NAXML_WRITER_H__

/**
 * SECTION: naxml_writer
 * @short_description: #NAXMLWriter class definition.
 * @include: io-xml/naxml-writer.h
 *
 * This class exports FileManager-Actions actions and menus as XML files.
 */

#include <api/fma-data-boxed.h>
#include <api/fma-iexporter.h>
#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

#define NAXML_WRITER_TYPE                ( naxml_writer_get_type())
#define NAXML_WRITER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NAXML_WRITER_TYPE, NAXMLWriter ))
#define NAXML_WRITER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NAXML_WRITER_TYPE, NAXMLWriterClass ))
#define NAXML_IS_WRITER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NAXML_WRITER_TYPE ))
#define NAXML_IS_WRITER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAXML_WRITER_TYPE ))
#define NAXML_WRITER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NAXML_WRITER_TYPE, NAXMLWriterClass ))

typedef struct _NAXMLWriterPrivate       NAXMLWriterPrivate;

typedef struct {
	/*< private >*/
	GObject             parent;
	NAXMLWriterPrivate *private;
}
	NAXMLWriter;

typedef struct _NAXMLWriterClassPrivate  NAXMLWriterClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass             parent;
	NAXMLWriterClassPrivate *private;
}
	NAXMLWriterClass;

GType  naxml_writer_get_type( void );

guint  naxml_writer_export_to_buffer( const FMAIExporter *instance, FMAIExporterBufferParmsv2 *parms );
guint  naxml_writer_export_to_file  ( const FMAIExporter *instance, FMAIExporterFileParmsv2 *parms );

guint  naxml_writer_write_start( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, GSList **messages  );
guint  naxml_writer_write_data ( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, const FMADataBoxed *boxed, GSList **messages );
guint  naxml_writer_write_done ( const FMAIFactoryProvider *writer, void *writer_data, const FMAIFactoryObject *object, GSList **messages  );

G_END_DECLS

#endif /* __NAXML_WRITER_H__ */
