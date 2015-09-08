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

bin_PROGRAMS = \
	fma-config-tool										\
	$(NULL)

ui_datadir = $(pkgdatadir)/ui

egg_platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_NACT}\"				\
	-DPKGUIDIR=\"$(ui_datadir)\"						\
	$(egg_platform_defines)								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

BUILT_SOURCES = \
	$(NULL)

fma_config_tool_SOURCES = \
	base-assistant.c									\
	base-assistant.h									\
	base-builder.c										\
	base-builder.h										\
	base-dialog.c										\
	base-dialog.h										\
	base-gtk-utils.c									\
	base-gtk-utils.h									\
	base-isession.c										\
	base-isession.h										\
	base-keysyms.h										\
	base-window.c										\
	base-window.h										\
	egg-desktop-file.c									\
	egg-desktop-file.h									\
	egg-sm-client.c										\
	egg-sm-client.h										\
	egg-sm-client-private.h								\
	egg-sm-client-xsmp.c								\
	egg-tree-multi-dnd.c								\
	egg-tree-multi-dnd.h								\
	main.c												\
	fma-add-capability-dialog.c						\
	fma-add-capability-dialog.h						\
	fma-add-scheme-dialog.c							\
	fma-add-scheme-dialog.h							\
	fma-application.c									\
	fma-application.h									\
	fma-assistant-export.c								\
	fma-assistant-export.h								\
	fma-assistant-import.c								\
	fma-assistant-import.h								\
	fma-clipboard.c									\
	fma-clipboard.h									\
	fma-confirm-logout.c								\
	fma-confirm-logout.h								\
	fma-export-ask.c									\
	fma-export-ask.h									\
	fma-iaction-tab.c									\
	fma-iaction-tab.h									\
	fma-ibasenames-tab.c								\
	fma-ibasenames-tab.h								\
	fma-icapabilities-tab.c							\
	fma-icapabilities-tab.h							\
	fma-icommand-tab.c									\
	fma-icommand-tab.h									\
	fma-icon-chooser.c									\
	fma-icon-chooser.h									\
	fma-ienvironment-tab.c								\
	fma-ienvironment-tab.h								\
	fma-iexecution-tab.c								\
	fma-iexecution-tab.h								\
	fma-ifolders-tab.c									\
	fma-ifolders-tab.h									\
	fma-imimetypes-tab.c								\
	fma-imimetypes-tab.h								\
	fma-iproperties-tab.c								\
	fma-iproperties-tab.h								\
	fma-ischemes-tab.c									\
	fma-ischemes-tab.h									\
	fma-main-tab.c										\
	fma-main-tab.h										\
	fma-main-window.c									\
	fma-main-window.h									\
	fma-main-window-def.h								\
	nact-match-list.c									\
	nact-match-list.h									\
	nact-menu.c											\
	nact-menu.h											\
	nact-menu-edit.c									\
	nact-menu-edit.h									\
	nact-menu-file.c									\
	nact-menu-file.h									\
	nact-menu-view.c									\
	nact-menu-view.h									\
	nact-preferences-editor.c							\
	nact-preferences-editor.h							\
	nact-providers-list.c								\
	nact-providers-list.h								\
	nact-schemes-list.c									\
	nact-schemes-list.h									\
	nact-sort-buttons.c									\
	nact-sort-buttons.h									\
	nact-statusbar.c									\
	nact-statusbar.h									\
	nact-tree-ieditable.c								\
	nact-tree-ieditable.h								\
	nact-tree-model.c									\
	nact-tree-model.h									\
	nact-tree-model-priv.h								\
	nact-tree-model-dnd.c								\
	nact-tree-view.c									\
	nact-tree-view.h									\
	$(BUILT_SOURCES)									\
	$(NULL)

fma_config_tool_LDADD = \
	$(top_builddir)/src/core/libfma-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

actions_files = \
	nautilus-actions-config-tool.actions				\
	$(NULL)

ui_files = \
	fma-add-capability.ui								\
	fma-add-scheme.ui									\
	fma-assistant-export.ui							\
	fma-assistant-import.ui							\
	fma-confirm-logout.ui								\
	fma-export-ask.ui									\
	fma-icon-chooser.ui								\
	nact-legend.ui										\
	fma-main-window.ui									\
	nact-preferences.ui									\
	nact-toolbar.ui										\
	$(NULL)

images_files = \
	gtk-convert.png										\
	gtk-index.png										\
	locked.png											\
	transparent.png										\
	$(NULL)

ui_data_DATA = \
	$(actions_files)									\
	$(ui_files)											\
	$(images_files)										\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

applications_in_files = nact.desktop.in

applications_files = $(applications_in_files:.desktop.in=.desktop)

applicationsdir = $(datadir)/applications

applications_DATA = $(applications_files)

CLEANFILES = \
	$(applications_files)								\
	$(BUILT_SOURCES)									\
	$(NULL)

EXTRA_DIST = \
	$(applications_in_files)							\
	$(ui_data_DATA)										\
	base-marshal.def									\
	nact-marshal.def									\
	$(NULL)