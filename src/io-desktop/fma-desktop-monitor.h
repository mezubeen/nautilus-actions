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

#ifndef __IO_DESKTOP_FMA_DESKTOP_MONITOR_H__
#define __IO_DESKTOP_FMA_DESKTOP_MONITOR_H__

/**
 * SECTION: fma_desktop_monitor
 * @short_description: #FMADesktopMonitor class definition.
 * @include: fma-desktop-monitor.h
 *
 * This class manages monitoring on .desktop files and directories.
 * We also put a monitor on directories which do not exist, to be
 * triggered when a file is dropped there.
 *
 * During tests of GIO monitoring, we don't have found any case where a
 * file monitor would be triggered without the parent directory monitor
 * has been itself triggered. We, so only monitor directories (not files).
 * More, as several events may be triggered for one user modification,
 * we try to factorize all monitor events before advertizing FMAPivot.
 */

#include "fma-desktop-provider.h"

G_BEGIN_DECLS

#define FMA_TYPE_DESKTOP_MONITOR                ( fma_desktop_monitor_get_type())
#define FMA_DESKTOP_MONITOR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_DESKTOP_MONITOR, FMADesktopMonitor ))
#define FMA_DESKTOP_MONITOR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_DESKTOP_MONITOR, FMADesktopMonitorClass ))
#define FMA_IS_DESKTOP_MONITOR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_DESKTOP_MONITOR ))
#define FMA_IS_DESKTOP_MONITOR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_DESKTOP_MONITOR ))
#define FMA_DESKTOP_MONITOR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_DESKTOP_MONITOR, FMADesktopMonitorClass ))

typedef struct _FMADesktopMonitorPrivate        FMADesktopMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject                   parent;
	FMADesktopMonitorPrivate *private;
}
	FMADesktopMonitor;

typedef struct _FMADesktopMonitorClassPrivate   FMADesktopMonitorClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                   parent;
	FMADesktopMonitorClassPrivate *private;
}
	FMADesktopMonitorClass;

GType              fma_desktop_monitor_get_type( void );

FMADesktopMonitor *fma_desktop_monitor_new     ( const FMADesktopProvider *provider, const gchar *path );

G_END_DECLS

#endif /* __IO_DESKTOP_FMA_DESKTOP_MONITOR_H__ */
