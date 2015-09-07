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

#ifndef __NA_TRACKER_H__
#define __NA_TRACKER_H__

/**
 * SECTION: na_tracker
 * @short_description: #NATracker class definition.
 * @include: tracker/na-tracker.h
 *
 * The #NATracker object is instanciated when Nautilus file manager loads
 * this plugin (this is the normal behavior of Nautilus to instanciate one
 * object of each plugin type).
 *
 * There is so only one #NATracker object in the process. As any Nautilus
 * extension, it is instantiated when the module is loaded by the file
 * manager, usually at startup time.
 *
 * The #NATracker object instanciates and keeps a new GDBusObjectManagerServer
 * rooted on our D-Bus path.
 * It then allocates an object at this same path, and another object which
 * implements the .Properties1 interface. Last connects to the method signal
 * before connecting the server to the session D-Bus.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_TYPE_TRACKER                ( na_tracker_get_type())
#define NA_TRACKER( object )           ( G_TYPE_CHECK_INSTANCE_CAST(( object ), NA_TYPE_TRACKER, NATracker ))
#define NA_TRACKER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST(( klass ), NA_TYPE_TRACKER, NATrackerClass ))
#define NA_IS_TRACKER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE(( object ), NA_TYPE_TRACKER ))
#define NA_IS_TRACKER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_TYPE_TRACKER ))
#define NA_TRACKER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_TYPE_TRACKER, NATrackerClass ))

typedef struct _NATrackerPrivate       NATrackerPrivate;

typedef struct {
	/*< private >*/
	GObject           parent;
	NATrackerPrivate *private;
}
	NATracker;

typedef struct _NATrackerClassPrivate  NATrackerClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass           parent;
	NATrackerClassPrivate *private;
}
	NATrackerClass;

GType    na_tracker_get_type          ( void );
void     na_tracker_register_type     ( GTypeModule *module );

G_END_DECLS

#endif /* __NA_TRACKER_H__ */
