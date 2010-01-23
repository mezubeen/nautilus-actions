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

#ifndef __NACT_ASSISTANT_EXPORT_ASK_H__
#define __NACT_ASSISTANT_EXPORT_ASK_H__

/**
 * SECTION: nact_assistant_export_ask
 * @short_description: #NactAssistantExportAsk class definition.
 * @include: nact/nact-assistant-export-ask.h
 *
 * This class is derived from BaseDialog.
 * It is ran each time an action is to be exported, and the user want
 * to be ask to choose the export format.
 */

#include <private/na-object-action-class.h>

#include "base-dialog.h"

G_BEGIN_DECLS

#define NACT_ASSISTANT_EXPORT_ASK_TYPE					( nact_assistant_export_ask_get_type())
#define NACT_ASSISTANT_EXPORT_ASK( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ASSISTANT_EXPORT_ASK_TYPE, NactAssistantExportAsk ))
#define NACT_ASSISTANT_EXPORT_ASK_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ASSISTANT_EXPORT_ASK_TYPE, NactAssistantExportAskClass ))
#define NACT_IS_ASSISTANT_EXPORT_ASK( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ASSISTANT_EXPORT_ASK_TYPE ))
#define NACT_IS_ASSISTANT_EXPORT_ASK_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ASSISTANT_EXPORT_ASK_TYPE ))
#define NACT_ASSISTANT_EXPORT_ASK_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ASSISTANT_EXPORT_ASK_TYPE, NactAssistantExportAskClass ))

typedef struct NactAssistantExportAskPrivate NactAssistantExportAskPrivate;

typedef struct {
	BaseDialog                     parent;
	NactAssistantExportAskPrivate *private;
}
	NactAssistantExportAsk;

typedef struct NactAssistantExportAskClassPrivate NactAssistantExportAskClassPrivate;

typedef struct {
	BaseDialogClass                     parent;
	NactAssistantExportAskClassPrivate *private;
}
	NactAssistantExportAskClass;

GType nact_assistant_export_ask_get_type( void );

gint  nact_assistant_export_ask_user( BaseWindow *window, NAObjectAction *action );

G_END_DECLS

#endif /* __NACT_ASSISTANT_EXPORT_ASK_H__ */
