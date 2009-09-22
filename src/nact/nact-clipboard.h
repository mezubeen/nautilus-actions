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

#ifndef __NACT_CLIPBOARD_H__
#define __NACT_CLIPBOARD_H__

/*
 * SECTION: nact_clipboard.
 * @short_description: #NactClipboard class definition.
 * @include: nact/nact-clipboard.h
 *
 * This is just a convenience class to extract clipboard functions
 * from main window code. There is a unique object which manages all
 * clipboard buffers.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NACT_CLIPBOARD_TYPE					( nact_clipboard_get_type())
#define NACT_CLIPBOARD( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_CLIPBOARD_TYPE, NactClipboard ))
#define NACT_CLIPBOARD_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_CLIPBOARD_TYPE, NactClipboardClass ))
#define NACT_IS_CLIPBOARD( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_CLIPBOARD_TYPE ))
#define NACT_IS_CLIPBOARD_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_CLIPBOARD_TYPE ))
#define NACT_CLIPBOARD_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_CLIPBOARD_TYPE, NactClipboardClass ))

typedef struct NactClipboardPrivate NactClipboardPrivate;

typedef struct {
	GObject               parent;
	NactClipboardPrivate *private;
}
	NactClipboard;

typedef struct NactClipboardClassPrivate NactClipboardClassPrivate;

typedef struct {
	GObjectClass               parent;
	NactClipboardClassPrivate *private;
}
	NactClipboardClass;

GType          nact_clipboard_get_type( void );

NactClipboard *nact_clipboard_new( void );

void           nact_clipboard_get_data_for_intern_use( GList *selected_items, gboolean copy_data );
char          *nact_clipboard_get_data_for_extern_use( GList *selected_items );

void           nact_clipboard_primary_set( GList *items, gboolean renumber_items );
GList         *nact_clipboard_primary_get( void );
void           nact_clipboard_primary_counts( guint *actions, guint *profiles, guint *menus );

void           nact_clipboard_free_items( GSList *items );

void           nact_clipboard_export_items( const gchar *uri, GList *items );

G_END_DECLS

#endif /* __NACT_CLIPBOARD_H__ */
