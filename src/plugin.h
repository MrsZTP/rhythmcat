/*
 * Plugin Declaration
 *
 * plugin.h
 * This file is part of <RhythmCat>
 *
 * Copyright (C) 2010 - SuperCat, license: GPL v3
 *
 * <RhythmCat> is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * <RhythmCat> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef HAVE_PLUGIN_H
#define HAVE_PLUGIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define RC_PLUGIN_MAGIC_NUMBER 0x100B0916
#define RC_PLUGIN_OLD_MAGIC_NUMBER 0x100B090B

/**
 * RCPluginType:
 * @PLUGIN_TYPE_MODULE: the plugin is a module
 *
 * The enum type to show the type of the plugin.
 */

typedef enum RCPluginType {
    PLUGIN_TYPE_MODULE = 1
}RCPluginType;

/**
 * RCPluginConfData:
 * @path: the path of the plugin file
 * @name: the name of the plugin
 * @desc: the description of the plugin
 * @author: the author of the plugin
 * @version: the version of the plugin
 * @website: the website of the plugin
 * @type: the type of the plugin
 *
 * The plugin configuration data structure.
 */

typedef struct RCPluginConfData {
    gchar *path;
    gchar *name;
    gchar *desc;
    gchar *author;
    gchar *version;
    gchar *website;
    RCPluginType type;
}RCPluginConfData;

/**
 * RCPluginModuleData:
 * @magic_number: the magic number
 * @group_name: the group name used in plugin configure file
 * @path: the plugin path (can only be accessed when the plugin is running)
 * @resident: whether the plugin can be removed while the player is running
 * @id: the unique ID when the plugin is running
 * @busy_flag: whether the plugin is busy (cannot be interruped)
 *
 * The data structure of module.
 */

typedef struct RCPluginModuleData {
    guint32 magic_number;
    gchar *group_name;
    gchar *path;
    gboolean resident;
    GQuark id;
    gboolean busy_flag;
}RCPluginModuleData;

/* Function */
void rc_plugin_init();
void rc_plugin_exit();
gboolean rc_plugin_search_dir(const gchar *dirname);
const GSList *rc_plugin_get_list();
void rc_plugin_list_free();
void rc_plugin_conf_free(RCPluginConfData *plugin_data);
RCPluginConfData *rc_plugin_conf_load(const gchar *filename);
gboolean rc_plugin_load(RCPluginType type, const gchar *filename);
gboolean rc_plugin_configure(RCPluginType type, const gchar *filename);
void rc_plugin_close(RCPluginType type, const gchar *filename);
gboolean rc_plugin_check_running(RCPluginType type, const gchar *path);
GSList *rc_plugin_check_exist(RCPluginType type, const gchar *name);
const gchar *rc_plugin_get_path(RCPluginType type, const gchar *group_name);

G_END_DECLS

#endif

