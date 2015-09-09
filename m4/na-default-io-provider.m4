# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

# serial 2 remove the input parameter

dnl --with-default-io-provider=gconf|desktop
dnl   Defines the default I/O Provider when creating a new action
dnl   Default to 'io-desktop'
dnl
dnl configure.ac usage:  NA_SET_DEFAULT_IO_PROVIDER
dnl
dnl ac_define NA_DEFAULT_IO_PROVIDER variable

AC_DEFUN([NA_SET_DEFAULT_IO_PROVIDER],[
	_AC_ARG_NA_WITH_DEFAULT_IO_PROVIDER([io-desktop])
	_CHECK_FOR_DEFAULT_IO_PROVIDER
])

AC_DEFUN([_AC_ARG_NA_WITH_DEFAULT_IO_PROVIDER],[
	AC_ARG_WITH(
		[default-io-provider],
		AS_HELP_STRING(
			[--with-default-io-provider<provider>],
			[define default I/O provider  @<:@$1@:>@]),
		[with_default_io_provider=$withval],
		[with_default_io_provider="$1"])
])

AC_DEFUN([_CHECK_FOR_DEFAULT_IO_PROVIDER],[
	AC_MSG_CHECKING([for default I/O provider on new items])
	AC_MSG_RESULT([${with_default_io_provider}])
	if test "${with_default_io_provider}" != "na-gconf"; then
		if test "${with_default_io_provider}" != "io-desktop"; then
			AC_MSG_ERROR([a default I/O provider must be specified, must be 'na-gconf' or 'io-desktop'])
		fi
	fi

	AC_DEFINE_UNQUOTED([NA_DEFAULT_IO_PROVIDER],["${with_default_io_provider}"],[Default I/O Provider])
])
