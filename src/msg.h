/*
 * Message Declaration
 *
 * gui.h
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

#ifndef HAVE_MSG_H
#define HAVE_MSG_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

struct RCMusicMetaData;

/**
 * RCMsgAsyncQueueWatchFunc:
 * @item: queue item
 * @data: user data
 *
 * The watch function type which executes when the message async 
 * queue changed.
 */

typedef void (*RCMsgAsyncQueueWatchFunc)(gpointer item, gpointer data);

/**
 * RCMsgPlistData:
 * @list2_index: the insert index in list2, only used in insert operation
 * @reference: the GtkTreeRowReference, used in list2 refresh operation
 * @store: the GtkListStore, used in list2 refresh operation
 * @mmd: the metadata
 *
 * The playlist item message data used in playlist operation.
 */

typedef struct RCMsgPlistData
{
    gint list2_index;
    GtkTreeRowReference *reference;
    GtkListStore *store;
    struct RCMusicMetaData *mmd;
}RCMsgPlistData;

/**
 * RCMsgType:
 * @MSG_TYPE_EMPTY: empty message
 * @MSG_TYPE_TEST: test message
 * @MSG_TYPE_PL_INSERT: playlist insertion message
 * @MSG_TYPE_PL_REFRESH: playlist refresh message
 * @MSG_TYPE_PL_REMOVE: playlist remove message
 * @MSG_TYPE_PL_INVALID: the message to mark the invalid music in the playlist
 *
 * Types of the message.
 */

typedef enum RCMsgType {
    MSG_TYPE_EMPTY = 0,
    MSG_TYPE_TEST = 1,
    MSG_TYPE_PL_INSERT = 2,
    MSG_TYPE_PL_REFRESH = 3,
    MSG_TYPE_PL_REMOVE = 4,
    MSG_TYPE_PL_INVALID = 5
}RCMsgType;

/**
 * RCMsgData:
 * @type: message type
 * @data: message data
 *
 * Custom struct to store message data.
 */

typedef struct RCMsgData {
    RCMsgType type;
    gpointer data;
}RCMsgData;

/* Functions */
guint rc_msg_async_queue_watch_new(GAsyncQueue *queue, gint priority,
    RCMsgAsyncQueueWatchFunc callback, gpointer data, GDestroyNotify notify,
    GMainContext *context);
void rc_msg_init();
void rc_msg_push(RCMsgType type, gpointer data);

G_END_DECLS

#endif

