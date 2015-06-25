# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# Nautilus-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Nautilus-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Nautilus-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

# serial 2 change NACT_ prefix to NA_ (Nautilus-Actions)

# let the user specify an alternate nautilus-extension dir
# --with-nautilus-extdir=<dir>

AC_DEFUN([NA_NAUTILUS_EXTDIR],[
	AC_REQUIRE([_AC_ARG_NA_NAUTILUS_EXTDIR])dnl
	AC_REQUIRE([_AC_NA_CHECK_NAUTILUS_EXTDIR])dnl
	if test "${with_nautilus_extdir}" = ""; then
		AC_MSG_ERROR([Unable to determine nautilus extension folder, please use --with-nautilus-extdir option])
	else
		AC_MSG_NOTICE([installing plugin in ${with_nautilus_extdir}])
		AC_SUBST([NAUTILUS_EXTENSIONS_DIR],[${with_nautilus_extdir}])
		AC_DEFINE_UNQUOTED([NA_NAUTILUS_EXTENSIONS_DIR],[${with_nautilus_extdir}],[Nautilus extensions directory])
	fi
])

AC_DEFUN([_AC_ARG_NA_NAUTILUS_EXTDIR],[
	AC_ARG_WITH(
		[nautilus-extdir],
		AC_HELP_STRING(
			[--with-nautilus-extdir=DIR],
			[nautilus plugins extension directory @<:@auto@:>@]
		),
	[with_nautilus_extdir=$withval],
	[with_nautilus_extdir=""]
	)
])

AC_DEFUN([_AC_NA_CHECK_NAUTILUS_EXTDIR],[
	if test "${with_nautilus_extdir}" = ""; then
		if test "{PKG_CONFIG}" != ""; then
			with_nautilus_extdir=`${PKG_CONFIG} --variable=extensiondir libnautilus-extension`
		fi
	fi
])
