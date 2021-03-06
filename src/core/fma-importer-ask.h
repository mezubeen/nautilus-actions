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

#ifndef __CORE_FMA_IMPORTER_ASK_H__
#define __CORE_FMA_IMPORTER_ASK_H__

/* @title: FMAImporterAsk
 * @short_description: The #FMAImporterAsk Class Definition
 * @include: core/fma-importer-ask.h
 *
 * This class creates and manages a dialog. It is ran each time an
 * imported action has the same Id as an existing one, and the user
 * want to be asked to know what to do with it.
 */

#include <gtk/gtk.h>

#include <api/fma-object-item.h>

#include "fma-pivot.h"

G_BEGIN_DECLS

#define FMA_TYPE_IMPORTER_ASK                ( fma_importer_ask_get_type())
#define FMA_IMPORTER_ASK( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_IMPORTER_ASK, FMAImporterAsk ))
#define FMA_IMPORTER_ASK_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_IMPORTER_ASK, FMAImporterAskClass ))
#define FMA_IS_IMPORTER_ASK( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_IMPORTER_ASK ))
#define FMA_IS_IMPORTER_ASK_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_IMPORTER_ASK ))
#define FMA_IMPORTER_ASK_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_IMPORTER_ASK, FMAImporterAskClass ))

typedef struct _FMAImporterAskPrivate        FMAImporterAskPrivate;

typedef struct {
	/*< private >*/
	GObject                parent;
	FMAImporterAskPrivate *private;
}
	FMAImporterAsk;

typedef struct _FMAImporterAskClassPrivate   FMAImporterAskClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                parent;
	FMAImporterAskClassPrivate *private;
}
	FMAImporterAskClass;

typedef struct {
	GtkWindow      *parent;
	gchar          *uri;
	guint           count;
	gboolean        keep_choice;
	const FMAPivot *pivot;
}
	FMAImporterAskUserParms;

GType fma_importer_ask_get_type( void );

guint fma_importer_ask_user    ( const FMAObjectItem *importing, const FMAObjectItem *existing, FMAImporterAskUserParms *parms );

G_END_DECLS

#endif /* __CORE_FMA_IMPORTER_ASK_H__ */
