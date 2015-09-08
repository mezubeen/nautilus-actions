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

#ifndef __NACT_SORT_BUTTONS_H__
#define __NACT_SORT_BUTTONS_H__

/**
 * SECTION: nact-sort-buttons
 * @title: NactSortButtons
 * @short_description: The Sort Buttons class definition
 * @include: nact-sort-buttons.h
 *
 * A convenience class to manager sort buttons in the user interface.
 *
 * The sort order mode is monitored, so that buttons automatically display
 * the right order mode if it is modified by another way (e.g. from
 * Preferences editor).
 *
 * Modifying the sort order mode requires that:
 * - level zero is writable (see FMAUpdater)
 * - preferences are not locked (see FMAUpdater)
 * - sort order mode is not a mandatory preference.
 */

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_SORT_BUTTONS                ( nact_sort_buttons_get_type())
#define NACT_SORT_BUTTONS( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_SORT_BUTTONS, NactSortButtons ))
#define NACT_SORT_BUTTONS_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_SORT_BUTTONS, NactSortButtonsClass ))
#define NACT_IS_SORT_BUTTONS( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_SORT_BUTTONS ))
#define NACT_IS_SORT_BUTTONS_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_SORT_BUTTONS ))
#define NACT_SORT_BUTTONS_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_SORT_BUTTONS, NactSortButtonsClass ))

typedef struct _NactSortButtonsPrivate        NactSortButtonsPrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	NactSortButtonsPrivate *private;
}
	NactSortButtons;

typedef struct {
	/*< private >*/
	GObjectClass            parent;
}
	NactSortButtonsClass;

GType            nact_sort_buttons_get_type( void );

NactSortButtons *nact_sort_buttons_new     ( NactMainWindow *window );

G_END_DECLS

#endif /* __NACT_SORT_BUTTONS_H__ */
