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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syslog.h>

#include <nautilus-actions/api/na-api.h>

#include "nadp-desktop-provider.h"

/*
 * na_api_module_init:
 *
 * mandatory starting with API v. 1.
 */
/* TODO: remove this when we will be ready to release the desktop provider */
#ifdef NA_MAINTAINER_MODE
gboolean
na_api_module_init( GTypeModule *module )
{
	static const gchar *thisfn = "nadp_module_na_api_module_initialize";

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	nadp_desktop_provider_register_type( module );

	return( TRUE );
}
#endif

/*
 * na_api_module_get_version:
 *
 * optional, defaults to 1.
 */
guint
na_api_module_get_version( void )
{
	static const gchar *thisfn = "nadp_module_na_api_module_get_version";
	guint version;

	version = 1;

	g_debug( "%s: version=%d", thisfn, version );

	return( version );
}

/*
 * na_api_module_list_types:
 *
 * mandatory starting with v. 1.
 */
guint
na_api_module_list_types( const GType **types )
{
	static const gchar *thisfn = "nadp_module_na_api_module_list_types";
	#define count 1
	static GType type_list[count];

	g_debug( "%s: types=%p", thisfn, ( void * ) types );

	type_list[0] = NADP_DESKTOP_PROVIDER_TYPE;
	*types = type_list;

	return( count );
}

/*
 * na_api_module_shutdown:
 *
 * mandatory starting with v. 1.
 */
void
na_api_module_shutdown( void )
{
	static const gchar *thisfn = "nadp_module_na_api_module_shutdown";

	g_debug( "%s", thisfn );
}
