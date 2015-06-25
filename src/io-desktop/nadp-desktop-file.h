/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NADP_DESKTOP_FILE_H__
#define __NADP_DESKTOP_FILE_H__

/**
 * SECTION: nadp_desktop_file
 * @short_description: #NadpDesktopFile class definition.
 * @include: nadp-desktop-file.h
 *
 * This class encapŝulates the EggDesktopFile structure, adding some
 * private properties. An instance of this class is associated with
 * every #NAObjectItem for this provider.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NADP_TYPE_DESKTOP_FILE                ( nadp_desktop_file_get_type())
#define NADP_DESKTOP_FILE( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NADP_TYPE_DESKTOP_FILE, NadpDesktopFile ))
#define NADP_DESKTOP_FILE_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NADP_TYPE_DESKTOP_FILE, NadpDesktopFileClass ))
#define NADP_IS_DESKTOP_FILE( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NADP_TYPE_DESKTOP_FILE ))
#define NADP_IS_DESKTOP_FILE_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NADP_TYPE_DESKTOP_FILE ))
#define NADP_DESKTOP_FILE_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NADP_TYPE_DESKTOP_FILE, NadpDesktopFileClass ))

typedef struct _NadpDesktopFilePrivate        NadpDesktopFilePrivate;

typedef struct {
	/*< private >*/
	GObject                 parent;
	NadpDesktopFilePrivate *private;
}
	NadpDesktopFile;

typedef struct _NadpDesktopFileClassPrivate   NadpDesktopFileClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                 parent;
	NadpDesktopFileClassPrivate *private;
}
	NadpDesktopFileClass;

/* standard suffix for desktop files
 */
#define NADP_DESKTOP_FILE_SUFFIX		".desktop"

GType            nadp_desktop_file_get_type         ( void );

NadpDesktopFile *nadp_desktop_file_new              ( void );
NadpDesktopFile *nadp_desktop_file_new_from_path    ( const gchar *path );
NadpDesktopFile *nadp_desktop_file_new_from_uri     ( const gchar *uri );
NadpDesktopFile *nadp_desktop_file_new_for_write    ( const gchar *path );

GKeyFile        *nadp_desktop_file_get_key_file     ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_key_file_uri ( const NadpDesktopFile *ndf );
gboolean         nadp_desktop_file_write            ( NadpDesktopFile *ndf );

gchar           *nadp_desktop_file_get_file_type    ( const NadpDesktopFile *ndf );
gchar           *nadp_desktop_file_get_id           ( const NadpDesktopFile *ndf );
GSList          *nadp_desktop_file_get_profiles     ( const NadpDesktopFile *ndf );

gboolean         nadp_desktop_file_has_profile      ( const NadpDesktopFile *ndf, const gchar *profile_id );

void             nadp_desktop_file_remove_key       ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key );
void             nadp_desktop_file_remove_profile   ( const NadpDesktopFile *ndf, const gchar *profile_id );

gboolean         nadp_desktop_file_get_boolean      ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, gboolean default_value );
gchar           *nadp_desktop_file_get_locale_string( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
gchar           *nadp_desktop_file_get_string       ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
GSList          *nadp_desktop_file_get_string_list  ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, const gchar *default_value );
guint            nadp_desktop_file_get_uint         ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean *key_found, guint default_value );

void             nadp_desktop_file_set_boolean      ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, gboolean value );
void             nadp_desktop_file_set_locale_string( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, const gchar *value );
void             nadp_desktop_file_set_string       ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, const gchar *value );
void             nadp_desktop_file_set_string_list  ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, GSList *value );
void             nadp_desktop_file_set_uint         ( const NadpDesktopFile *ndf, const gchar *group, const gchar *key, guint value );

G_END_DECLS

#endif /* __NADP_DESKTOP_FILE_H__ */
