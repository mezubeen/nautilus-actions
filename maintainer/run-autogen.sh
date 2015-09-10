#!/bin/sh
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
#
# pwi 2011-11-28
# Goal is to have a single source tree, being able to easily build it
# in several virtual guests which all have a read access to this source
# tree
# -> run-distcheck.sh and run-autogen.sh are only executed on the
#    maintainer development box (and thus are installed in maintainer/
#    directory), while run-configure.sh is meant to build
#    from anywhere (thus simulating the packager machines, and is so
#    built at the root of the source tree)
# -> on the maintainer development box, _build and _install are
#    subdirectories of the root source tree
# -> on the virtual guests simulating packager machines, _build and
#    _install are subdirectories of the current working directory when
#    running run-configure.sh
#
# pwi 2013- 9-26
# As a recall, I use two configurations for two distinct uses:
# - first, and the most often, i.e. when developing the code, I try to
#   not use any deprecated function; I so ask to configure to just
#   disable them..
# - but, when updating the developer reference manual, I want deprecated
#   functions to be at least listed as deprecated, and I so to enable
#   there deprecated functions with configure.
#
# pwi 2013- 9-26
# Get rid of 'target' environment variable, as doc generation is a
# configure option.
#
# pwi 2015- 9-10
# gnome-common is deprecated
# see: https://wiki.gnome.org/Projects/GnomeCommon/Migration

if [ ! -f configure.ac ]; then
	echo "> This script is only meant to be run from the top source directory." 1>&2
	exit 1
fi

maintainer_dir=$(cd ${0%/*}; pwd)
top_srcdir="${maintainer_dir%/*}"

PkgName=`autoconf --trace 'AC_INIT:$1' configure.ac`
pkgname=$(echo $PkgName | tr '[[:upper:]]' '[[:lower:]]')

# a filemanager-actions-x.y may remain after an aborted make distcheck
# such a directory breaks gnome-autogen.sh generation
# so clean it here
for d in $(find ${top_srcdir} -maxdepth 2 -type d -name "${pkgname}-*"); do
	echo "> Removing $d"
	chmod -R u+w $d
	rm -fr $d
done

# tools required version
source ${top_srcdir}/fma_required_versions

echo "> Running aclocal"
aclocal --install || exit 1

# requires gtk-doc package
# used for Developer Reference Manual generation (devhelp)
echo "> Running gtkdocize"
gtkdocize || exit 1

echo "> Running autoreconf"
autoreconf --verbose --force --install -Wno-portability || exit 1

# pwi 2012-10-12
# starting with FileManager-Actions 3.2.3, we let the GNOME-DOC-PREPARE do
# its stuff, but get rid of the gnome-doc-utils.make standard file, as
# we are using our own hacked version
# (see full rationale in docs/nact/gnome-doc-utils-na.make)
#rm -f gnome-doc-utils.make

runconf="${top_srcdir}/run-configure.sh"
echo "> Generating ${runconf}"
cat <<EOF >${runconf}
#!/bin/sh
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
#
# WARNING
#   This file has been automatically generated by $0
#   on $(date) - Please do not manually modify it.

# top_srcdir here is the root of the source directory
top_srcdir="\$(cd \${0%/*}; pwd)"

# heredir is the root of the _build/_install directories
heredir=\$(pwd)

mkdir -p \${heredir}/_build
cd \${heredir}/_build

conf_cmd="\${top_srcdir}/configure"
conf_args="${conf_args}"
conf_args="\${conf_args} --prefix=\${heredir}/_install"
conf_args="\${conf_args} --sysconfdir=/etc"
conf_args="\${conf_args} --with-nautilus-extdir=\${heredir}/_install/lib/nautilus"
conf_args="\${conf_args} --with-nemo-extdir=\${heredir}/_install/lib/nemo"
conf_args="\${conf_args} --enable-maintainer-mode"
conf_args="\${conf_args} --disable-scrollkeeper"
conf_args="\${conf_args} $*"
conf_args="\${conf_args} \$*"

tput bold
echo "\${conf_cmd} \${conf_args}
"
tput sgr0

\${conf_cmd} \${conf_args}
EOF

echo "> Executing ${runconf}
"
chmod a+x ${runconf}
${runconf} &&
make -C _build &&
make -C _build install
