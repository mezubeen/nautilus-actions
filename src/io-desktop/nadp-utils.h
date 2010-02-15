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

#ifndef __NADP_UTILS_H__
#define __NADP_UTILS_H__

G_BEGIN_DECLS

GSList  *nadp_utils_split_path_list( const gchar *path_list );

GSList  *nadp_utils_to_slist( const gchar **list );

void     nadp_utils_gslist_free( GSList *list );
GSList  *nadp_utils_gslist_remove_from( GSList *list, const gchar *string );

gchar   *nadp_utils_remove_suffix( const gchar *string, const gchar *suffix );

gboolean nadp_utils_is_writable_dir( const gchar *path );

gchar   *nadp_utils_path2id( const gchar *path );

gboolean nadp_utils_is_writable_file( const gchar *path );

gboolean nadp_utils_delete_file( const gchar *path );

G_END_DECLS

#endif /* __NADP_UTILS_H__ */