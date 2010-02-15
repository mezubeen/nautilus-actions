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

#ifndef __NAUTILUS_ACTIONS_API_NA_CORE_UTILS_H__
#define __NAUTILUS_ACTIONS_API_NA_CORE_UTILS_H__

/**
 * SECTION: na_core_utils
 * @short_description: Core library utilities.
 * @include: nautilus-action/na-core-utils.h
 */

#include <glib.h>

G_BEGIN_DECLS

/* boolean manipulation
 */
gboolean na_core_utils_boolean_from_string( const gchar *string );

/* string manipulation
 */
gchar   *na_core_utils_str_remove_suffix( const gchar *string, const gchar *suffix );
gchar   *na_core_utils_str_add_prefix( const gchar *prefix, const gchar *str );

/* some functions to get or set GSList list of strings
 */
GSList  *na_core_utils_slist_duplicate( GSList *list );
void     na_core_utils_slist_dump( GSList *list );
GSList  *na_core_utils_slist_from_split( const gchar *string, const gchar *separator );
GSList  *na_core_utils_slist_from_str_array( const gchar **str_array );
GSList  *na_core_utils_slist_remove_string( GSList *list, const gchar *string );
gboolean na_core_utils_slist_find( GSList *list, const gchar *str );
gboolean na_core_utils_slist_are_equal( GSList *a, GSList *b );
void     na_core_utils_slist_free( GSList *slist );

/* directory management
 */
gboolean na_core_utils_dir_is_writable( const gchar *path );

/* file management
 */
gboolean na_core_utils_file_delete( const gchar *path );

/* miscellaneous
 */
void     na_core_utils_print_version( void );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_CORE_UTILS_H__ */