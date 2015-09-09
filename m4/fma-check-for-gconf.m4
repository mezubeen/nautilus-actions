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

# serial 4 renamed as FMA_CHECK_FOR_GCONF

dnl let the user choose whether to compile with GConf enabled
dnl --enable-gconf
dnl
dnl defaults to disabling GConf, only searching for a GConf subsystem
dnl when the '--enable-gconf' option is specified.
dnl
dnl Please note that, from the packager point of view, we should have
dnl both GConf2 and GConf2-devel package, in order to be able to build
dnl and distribute the 'na-gconf' I/O provider
dnl
dnl Defined AM_CONDITIONAL conditionals:
dnl - HAVE_GCONF
dnl - GCONF_SCHEMAS_INSTALL
dnl
dnl Defined AC_SUBST variables:
dnl - GCONF_SCHEMA_FILE_DIR
dnl - GCONF_SCHEMA_CONFIG_SOURCE

AC_DEFUN([FMA_CHECK_FOR_GCONF],[
	AC_REQUIRE([_AC_FMA_GCONF_ARG])dnl
	AC_REQUIRE([_AC_FMA_GCONF_CHECK])dnl
])

AC_DEFUN([_AC_FMA_GCONF_ARG],[
	AC_ARG_ENABLE(
		[gconf],
		AC_HELP_STRING(
			[--enable-gconf],
			[whether to enable GConf subsystem @<:@no@:>@]),
		[enable_gconf=$enableval],
		[enable_gconf="no"])
])

AC_DEFUN([_AC_FMA_GCONF_CHECK],[
	AC_MSG_CHECKING([whether GConf is required])
	AC_MSG_RESULT([${enable_gconf}])
	compile_with_gconf="no"

	dnl as of 3.4, GConf is disabled if --enable-gconf is not specified
	if test "${enable_gconf}" = "yes"; then
		AC_PATH_PROG([have_gconftool],[gconftool-2],[yes],[no])

		dnl this is not fatal to not have GConf (even if required)
		dnl just the GConf I/O provider will not be built
		if test "${have_gconftool}" != "yes"; then
			AC_MSG_WARN([GConf2 subsystem (gconftool-2) is missing])

		dnl if want to compile with GConf, then check that we have the
		dnl development libraries
		else
			PKG_CHECK_MODULES([GCONF],
					[gconf-2.0 >= 2.8.0],[have_gconf_devel="yes"],[have_gconf_devel="no"])
			if test "${have_gconf_devel}" = "no"; then
				AC_MSG_WARN([GConf2 development libraries are missing. Please install GConf2-devel package])

			else
				compile_with_gconf="yes"
			fi
		fi
	fi

	if test "${compile_with_gconf}" = "yes"; then
		gconf_msg_version=$(pkg-config --modversion gconf-2.0)
		AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} -DHAVE_GCONF"])
		NAUTILUS_ACTIONS_CFLAGS="${NAUTILUS_ACTIONS_CFLAGS} ${GCONF_CFLAGS}"
		NAUTILUS_ACTIONS_LIBS="${NAUTILUS_ACTIONS_LIBS} ${GCONF_LIBS}"
		AC_DEFINE_UNQUOTED([HAVE_GCONF],[1],[Whether we compile against the GConf library (and build the GConf I/O Provider)])
	fi

	_FMA_GCONF_SOURCE_2(["${compile_with_gconf}"])

	AM_CONDITIONAL([HAVE_GCONF], [test "${compile_with_gconf}" = "yes"])
])

dnl pwi 2011-02-14
dnl this is a copy of the original AM_GCONF_SOURCE_2 which is just a bit hacked
dnl in order to define the conditionals event when we want disabled GConf
dnl syntax: FMA_GCONF_SOURCE_2([have_gconf])
dnl
dnl AM_GCONF_SOURCE_2
dnl Defines GCONF_SCHEMA_CONFIG_SOURCE which is where you should install schemas
dnl  (i.e. pass to gconftool-2)
dnl Defines GCONF_SCHEMA_FILE_DIR which is a filesystem directory where
dnl  you should install foo.schemas files

AC_DEFUN([_FMA_GCONF_SOURCE_2],
[
	if test "$1" = "yes"; then
		if test "x$GCONF_SCHEMA_INSTALL_SOURCE" = "x"; then
			GCONF_SCHEMA_CONFIG_SOURCE=`gconftool-2 --get-default-source`
		else
			GCONF_SCHEMA_CONFIG_SOURCE=$GCONF_SCHEMA_INSTALL_SOURCE
		fi
	fi

  AC_ARG_WITH([gconf-source],
              AC_HELP_STRING([--with-gconf-source=sourceaddress],
                             [Config database for installing schema files.]),
              [GCONF_SCHEMA_CONFIG_SOURCE="$withval"],)

  AC_SUBST(GCONF_SCHEMA_CONFIG_SOURCE)

	if test "$1" = "yes"; then
		AC_MSG_RESULT([Using config source $GCONF_SCHEMA_CONFIG_SOURCE for schema installation])
	fi

  if test "x$GCONF_SCHEMA_FILE_DIR" = "x"; then
    GCONF_SCHEMA_FILE_DIR='$(sysconfdir)/gconf/schemas'
  fi

  AC_ARG_WITH([gconf-schema-file-dir],
              AC_HELP_STRING([--with-gconf-schema-file-dir=dir],
                             [Directory for installing schema files.]),
              [GCONF_SCHEMA_FILE_DIR="$withval"],)

  AC_SUBST(GCONF_SCHEMA_FILE_DIR)

	if test "$1" = "yes"; then
		AC_MSG_RESULT([Using $GCONF_SCHEMA_FILE_DIR as install directory for schema files])
	fi

  AC_ARG_ENABLE(schemas-install,
        AC_HELP_STRING([--disable-schemas-install],
                       [Disable the schemas installation]),
     [case ${enableval} in
       yes|no) ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-schemas-install]) ;;
      esac])

	if test "x${enable_schemas_install}" = "xno" -o "$1" != "yes"; then
		msg_schemas_install="disabled"; else
		msg_schemas_install="enabled in ${GCONF_SCHEMA_FILE_DIR}"
	fi
      
  AM_CONDITIONAL([GCONF_SCHEMAS_INSTALL], [test "$enable_schemas_install" != "no" -a "$1" = "yes"])
])
