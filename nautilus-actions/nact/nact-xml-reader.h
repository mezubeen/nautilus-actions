/*
 * Nautilus Actions
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

#ifndef __NACT_XML_READER_H__
#define __NACT_XML_READER_H__

/**
 * SECTION: nact_xml_reader
 * @short_description: #NactXMLReader class definition.
 * @include: nact/nact-xml-reader.h
 *
 * This is the base class for importing actions from XML files.
 */

#include <private/na-object-action-class.h>

#include "base-assistant.h"

G_BEGIN_DECLS

#define NACT_XML_READER_TYPE				( nact_xml_reader_get_type())
#define NACT_XML_READER( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_XML_READER_TYPE, NactXMLReader ))
#define NACT_XML_READER_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_XML_READER_TYPE, NactXMLReaderClass ))
#define NACT_IS_XML_READER( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_XML_READER_TYPE ))
#define NACT_IS_XML_READER_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_XML_READER_TYPE ))
#define NACT_XML_READER_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_XML_READER_TYPE, NactXMLReaderClass ))

typedef struct NactXMLReaderPrivate NactXMLReaderPrivate;

typedef struct {
	GObject               parent;
	NactXMLReaderPrivate *private;
}
	NactXMLReader;

typedef struct NactXMLReaderClassPrivate NactXMLReaderClassPrivate;

typedef struct {
	GObjectClass               parent;
	NactXMLReaderClassPrivate *private;
}
	NactXMLReaderClass;

GType           nact_xml_reader_get_type( void );

NAObjectAction *nact_xml_reader_import( BaseWindow *window, GList *items, const gchar *uri, gint mode, GSList **msg );

G_END_DECLS

#endif /* __NACT_XML_READER_H__ */
