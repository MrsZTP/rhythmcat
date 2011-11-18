/*
 * GUI Treeview
 * Build the Treeviews in the player. 
 *
 * gui_treeview.c
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

#include "gui_treeview.h"
#include "gui.h"
#include "core.h"
#include "player.h"
#include "playlist.h"
#include "debug.h"

/**
 * SECTION: gui_treeview
 * @Short_description: The playlist views of the player.
 * @Title: Playlist Views UI
 * @Include: gui_treeview.h
 *
 * Show the playlist views in the player.
 */

/* Variables */
static const gchar *module_name = "GUI";
static RCGuiData *rc_ui;
static GtkTreeViewColumn *list1_column;
static GtkTreeViewColumn *list2_index_column;
static GtkTreeViewColumn *list2_title_column;
static GtkTreeViewColumn *list2_time_column;

/*
 * Process the multi-drag of selections in list2.
 */

static gboolean rc_gui_list2_mdrag_selection_block(GtkTreeSelection *s,
    GtkTreeModel *model, GtkTreePath *path, gboolean pcs, gpointer data)
{
    return *(const gboolean *)data;
}

/*
 * Block the selection in list2.
 */

static void rc_gui_list2_block_selection(GtkWidget *widget, gboolean block,
    gint x, gint y)
{
    static const gboolean which[] = {FALSE, TRUE};
    gtk_tree_selection_set_select_function(rc_ui->list2_selection,
        rc_gui_list2_mdrag_selection_block, (gboolean *)&which[!!block],
        NULL);
    gint *where = g_object_get_data(G_OBJECT(rc_ui->list2_tree_view),
        "multidrag-where");
    if(where==NULL)
    {
        where = g_new(gint, 2);
        g_object_set_data_full(G_OBJECT(rc_ui->list2_tree_view),
            "multidrag-where", where, g_free);
    }
    where[0] = x;
    where[1] = y;  
}

/*
 * Set how the item in the list1_tree_view can be draged. 
 */

static void rc_gui_list1_tree_view_set_drag()
{
    static GtkTargetEntry entry[2];   
    entry[0].target = "RhythmCat/ListItem";
    entry[0].flags = GTK_TARGET_SAME_WIDGET;
    entry[0].info = 0;
    entry[1].target = "RhythmCat/MusicItem";
    entry[1].flags = GTK_TARGET_SAME_APP;
    entry[1].info = 1;
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(
        rc_ui->list1_tree_view), GDK_BUTTON1_MASK, entry, 1,
        GDK_ACTION_COPY);
    gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(
        rc_ui->list1_tree_view), entry, 2, GDK_ACTION_COPY |
        GDK_ACTION_MOVE | GDK_ACTION_LINK);
}

/*
 * Set how the item in the list2_tree_view can be draged. 
 */

static void rc_gui_list2_tree_view_set_drag()
{
    static GtkTargetEntry entry[4];
    entry[0].target = "RhythmCat/MusicItem";
    entry[0].flags = GTK_TARGET_SAME_APP;
    entry[0].info = 1;
    entry[1].target = "STRING";
    entry[1].flags = 0;
    entry[1].info = 6;
    entry[2].target = "text/plain";
    entry[2].flags = 0;
    entry[2].info = 6;
    entry[3].target = "text/uri-list";
    entry[3].flags = 0;
    entry[3].info = 7;
    gtk_drag_source_set(rc_ui->list2_tree_view, GDK_BUTTON1_MASK, entry,
        1, GDK_ACTION_MOVE);
    gtk_drag_dest_set(rc_ui->list2_tree_view, GTK_DEST_DEFAULT_ALL, entry,
        4, GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);
}

/*
 * Get the index number by the iter of list1. 
 */

static gint rc_gui_list1_get_index(GtkTreeIter *iter)
{
    gint *indices = NULL;
    gint index = 0;
    GtkTreePath *path = NULL;
    path = gtk_tree_model_get_path(rc_ui->list1_tree_model, iter);
    indices = gtk_tree_path_get_indices(path);
    if(indices!=NULL)
    {
        index =  indices[0];
    }
    else index = -1;
    gtk_tree_path_free(path);
    return index;
}

/*
 * Rename an existing list.
 */

static void rc_gui_list1_edited(GtkCellRendererText *renderer, gchar *path_str,
    gchar *new_text, gpointer data)
{
    GtkTreeIter iter;
    gint index = 0;
    if(new_text==NULL || strlen(new_text)<=0) return;
    if(!gtk_tree_model_get_iter_from_string(rc_ui->list1_tree_model,
        &iter, path_str))
        return;
    index = rc_gui_list1_get_index(&iter);
    if(index<0) return;
    rc_plist_set_list1_name(index, new_text);
}


static gint rc_gui_list2_comp_func(const gint a, const gint b,
    gpointer data)
{
    if(a<b) return -1;
    else if(a==b) return 0;
    else return 1;
}

/*
 * Receive the data of the DnD of the play list.
 */

static void rc_gui_list2_dnd_data_received(GtkWidget *widget,
    GdkDragContext *context, gint x, gint y, GtkSelectionData *seldata,
    guint info, guint time, gpointer data)
{
    guint length = 0;
    gint i, j, k;
    gint *reorder_array = NULL;
    gint *indices = NULL;
    gint *index = NULL;
    gint target = 0;
    guint insert_num = 0;
    GList *path_list_foreach = NULL;
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_start = NULL;
    GtkTreePath *path_drop = NULL;
    gint list_length = 0;
    gboolean insert_flag = FALSE;
    GList *path_list = NULL;
    gchar *uris = NULL;
    gchar **uri_array = NULL;
    gchar *uri = NULL;
    guint count = 0;
    gboolean flag = FALSE;
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(
        rc_ui->list2_tree_view), x,y, &path_drop, &pos);
    if(path_drop!=NULL)
    {
        index = gtk_tree_path_get_indices(path_drop);
        target = index[0];
        gtk_tree_path_free(path_drop);
    }
    else target = -2;
    switch(info)
    {
        case 1: 
        {
            memcpy(&path_list, gtk_selection_data_get_data(seldata),
                sizeof(path_list));
            if(path_list==NULL) break;
            length = g_list_length(path_list);
            indices = g_new(gint, length);
            for(path_list_foreach=path_list;path_list_foreach!=NULL;
                path_list_foreach=g_list_next(path_list_foreach))
            {
                path_start = path_list_foreach->data;
                index = gtk_tree_path_get_indices(path_start);
                indices[count] = index[0];
                count++;
            }
            g_qsort_with_data(indices, length, sizeof(gint),
                (GCompareDataFunc)rc_gui_list2_comp_func, NULL);
            if(pos==GTK_TREE_VIEW_DROP_AFTER ||
                pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER) target++;
            list_length = gtk_tree_model_iter_n_children(
                rc_ui->list2_tree_model, NULL);
            if(target<0) target = list_length;
            reorder_array = g_new0(gint, list_length);
            i = 0;
            j = 0;
            count = 0;
            while(i<list_length)
            {
                if((j>=length || count!=indices[j]) && count!=target)
                {
                    reorder_array[i] = count;
                    count++;
                    i++;
                }
                else if(count==target && !insert_flag)
                {
                    for(k=0;k<length;k++)
                    {
                        if(target==indices[k])
                        {
                            target++;
                            count++;
                        }
                        reorder_array[i] = indices[k];
                        i++;
                    }
                    reorder_array[i] = target;
                    i++;
                    count++;
                    insert_flag = TRUE;
                }
                else if(j<length && count==indices[j])
                {
                    count++;             
                    j++;
                }
                else break;
            }
            gtk_list_store_reorder(GTK_LIST_STORE(rc_ui->list2_tree_model),
                reorder_array);
            g_free(reorder_array);
            g_free(indices);
            break;
        }
        case 6:
        {
            uris = (gchar *)gtk_selection_data_get_data(seldata);
            if(uris==NULL) break;
            if(pos==GTK_TREE_VIEW_DROP_AFTER ||
                pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER) target++;
            list_length = gtk_tree_model_iter_n_children(
                rc_ui->list2_tree_model, NULL);
            if(target<0) target = list_length;
            uri_array = g_uri_list_extract_uris(uris);
            insert_num = 0;
            for(count=0;uri_array[count]!=NULL;count++)
            {
                uri = uri_array[count];
                if(rc_player_check_supported_format(uri))
                {
                    flag = rc_plist_insert_music(uri, 
                        rc_gui_list1_get_selected_index(), target);
                    target++;
                    insert_num++;
                }
            }
            g_strfreev(uri_array);
            if(insert_num>0)
                rc_gui_status_task_set(1, insert_num);
            break;
        }
        case 7:
        {
            rc_debug_module_perror(module_name,
                "Unknown dnd data in list2: %s",
                gtk_selection_data_get_data(seldata));
        }
        default: break;
    }
}

/*
 * Send the data of the DnD of the play list.
 */

static void rc_gui_list2_dnd_data_get(GtkWidget *widget,
    GdkDragContext *context, GtkSelectionData *sdata, guint info, guint time,
    gpointer data)
{
    static GList *path_list = NULL;
    if(path_list!=NULL)
    {
        g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(path_list);
        path_list = NULL;
    }
    path_list = gtk_tree_selection_get_selected_rows(
        rc_ui->list2_selection, NULL);
    if(path_list==NULL) return;
    gtk_selection_data_set(sdata,gdk_atom_intern("Playlist index array",
        FALSE),8,(void *)&path_list,sizeof(GList *));
}

/*
 * Set the motion action of the play list.
 */

static void rc_gui_list2_dnd_motion(GtkWidget *widget, GdkDragContext *context,
    gint x, gint y, guint time, gpointer data)
{
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_drop = NULL;
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(
        rc_ui->list2_tree_view), x, y, &path_drop, &pos);
    if(pos==GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) pos=GTK_TREE_VIEW_DROP_BEFORE;
    if(pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER) pos=GTK_TREE_VIEW_DROP_AFTER;
    if(path_drop)
    {
        gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(
            rc_ui->list2_tree_view), path_drop, pos);
        if(pos==GTK_TREE_VIEW_DROP_AFTER) gtk_tree_path_next(path_drop);
        else gtk_tree_path_prev(path_drop);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(
            rc_ui->list2_tree_view), path_drop, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path_drop);
    }
}

/*
 * Receive the data of the DnD of the list.
 */

static void rc_gui_list1_dnd_data_received(GtkWidget *widget,
    GdkDragContext *context, gint x, gint y, GtkSelectionData *seldata,
    guint info, guint time, gpointer data)
{
    gint source = -1;
    gint target = 0;
    gint *index = NULL;
    gint i = 0;
    gint length = 0;
    gboolean end_flag = FALSE;
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_start = NULL, *path_drop = NULL;
    GtkTreeIter iter_start, iter_drop;
    GList *path_list = NULL;
    GList *list_foreach = NULL;
    GtkTreePath **path_array;
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(
        rc_ui->list1_tree_view), x,y, &path_drop, &pos);
    if(path_drop!=NULL)
    {
        gtk_tree_model_get_iter(rc_ui->list1_tree_model, &iter_drop,
            path_drop);
        index = gtk_tree_path_get_indices(path_drop);
        target = index[0];
        gtk_tree_path_free(path_drop);
    }
    else end_flag = TRUE;
    switch(info)
    {
        case 0:
        {
            source = *(gtk_selection_data_get_data(seldata));
            if(pos==GTK_TREE_VIEW_DROP_AFTER ||
                pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
            {
                target++;
                if(!gtk_tree_model_iter_next(rc_ui->list1_tree_model,
                    &iter_drop))
                    end_flag = TRUE;
            }
            path_start = gtk_tree_path_new_from_indices(source, -1);
            gtk_tree_model_get_iter(rc_ui->list1_tree_model, &iter_start,
                path_start);
            gtk_tree_path_free(path_start);
            if(!end_flag)
                gtk_list_store_move_before(GTK_LIST_STORE(
                    rc_ui->list1_tree_model), &iter_start, &iter_drop);
            else
                gtk_list_store_move_before(GTK_LIST_STORE(
                    rc_ui->list1_tree_model), &iter_start, NULL);
            break;
        }
        case 1:
        {
            if(target==rc_gui_list1_get_selected_index()) break;
            memcpy(&path_list, gtk_selection_data_get_data(seldata),
                sizeof(path_list));
            if(path_list==NULL) break;
            length = g_list_length(path_list);
            path_list = g_list_sort_with_data(path_list, (GCompareDataFunc)
                gtk_tree_path_compare, NULL);
            path_array = g_new0(GtkTreePath *, length);
            for(list_foreach=path_list, i=0;list_foreach!=NULL;
                list_foreach=g_list_next(list_foreach), i++)
            {
                path_array[i] = list_foreach->data;
            }
            rc_plist_plist_move2(rc_gui_list1_get_selected_index(), path_array,
                length, target);
            break;
        }
        default: break;
    }
}

/*
 * Send the data of the DnD of the list.
 */

static void rc_gui_list1_dnd_data_get(GtkWidget *widget, GdkDragContext *context,
    GtkSelectionData *sdata, guint info, guint time, gpointer data)
{
    GtkTreeIter iter;
    static gint index;
    index = 0;
    if(gtk_tree_selection_get_selected(rc_ui->list1_selection, NULL,
        &iter))
    {
        index = rc_gui_list1_get_index(&iter);
        if(index==-1) return;
    }
    else return;
    gtk_selection_data_set(sdata, gdk_atom_intern("Playlist list index",
        FALSE), 8, (void *)&index, sizeof(index));
}


/*
 * Detect if the playlist in the list is selected.
 */

static void rc_gui_list1_row_selected(GtkTreeView *list, gpointer data)
{
    GtkTreeIter iter;
    GtkTreePath *path, *path_old;
    gint same_flag = 1;
    gint index = 0;
    if(!gtk_tree_selection_get_selected(rc_ui->list1_selection, NULL, &iter))
        return;
    if(gtk_tree_row_reference_valid(rc_ui->list1_selected_reference))
    {
        path_old = gtk_tree_row_reference_get_path(
            rc_ui->list1_selected_reference);
        path = gtk_tree_model_get_path(rc_ui->list1_tree_model, &iter);
        if(path!=NULL)
        {
            if(path_old!=NULL)
            {
                same_flag = gtk_tree_path_compare(path_old, path);
                gtk_tree_path_free(path_old);
            }
            gtk_tree_path_free(path);
            if(same_flag==0) return;
        }
    }
    if(rc_ui->list1_selected_reference!=NULL)
    {
        gtk_tree_row_reference_free(rc_ui->list1_selected_reference);
        rc_ui->list1_selected_reference = NULL;
    }
    index = rc_gui_list1_get_index(&iter);
    if(index==-1) return;
    rc_ui->list2_tree_model = GTK_TREE_MODEL(
        rc_plist_get_list_store(index));
    gtk_tree_view_set_model(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        rc_ui->list2_tree_model);
    rc_ui->list2_selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(rc_ui->list2_tree_view));
    path = gtk_tree_model_get_path(rc_ui->list1_tree_model, &iter);
    if(path!=NULL)
    {
        rc_ui->list1_selected_reference = gtk_tree_row_reference_new(
            rc_ui->list1_tree_model, path);
        gtk_tree_path_free(path);
    }
}

/*
 * Detect if the music in the playlist is double-clicked.
 */

static void rc_gui_list2_row_activated(GtkTreeView *list, GtkTreePath *path, 
    GtkTreeViewColumn *column, gpointer data)
{
    gint *indices = NULL;
    gint list1_index, list2_index;
    if(path==NULL) return;
    list1_index = rc_gui_list1_get_selected_index();
    if(list1_index==-1) return;
    indices = gtk_tree_path_get_indices(path);
    list2_index = indices[0];
    rc_plist_play_by_index(list1_index, list2_index);
    rc_core_play();
}

/*
 * Popup the menu of the list.
 */

static gboolean rc_gui_list1_popup_menu(GtkWidget *widget,
    GdkEventButton *event, gpointer data)
{
    if(event->button!=3) return FALSE;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(rc_ui->main_ui,
        "/List1PopupMenu")), NULL, NULL, NULL, NULL, 3,
        gtk_get_current_event_time());
    return FALSE;
}

/*
 * Popup the menu of playlist.
 */

static gboolean rc_gui_list2_popup_menu(GtkWidget *widget,
    GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    rc_gui_list2_block_selection(widget, TRUE, -1, -1);
    if(event->button!=3 && event->button!=1) return FALSE;
    if(event->button==1)
    {
        if(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) return FALSE;
        if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(
            rc_ui->list2_tree_view), event->x, event->y, &path, NULL,
            NULL, NULL))
            return FALSE;
        if(gtk_tree_selection_path_is_selected(rc_ui->list2_selection,
            path))
        {
            rc_gui_list2_block_selection(rc_ui->list2_tree_view, FALSE, 
                event->x, event->y);
        }
        if(path!=NULL) gtk_tree_path_free(path);
        return FALSE;
    }
    if(gtk_tree_selection_count_selected_rows(rc_ui->list2_selection)>1)
        return TRUE;
    else return FALSE;
}

/*
 * Process the event of play list when the button released.
 */

static gboolean rc_gui_list2_button_release_event(GtkWidget *widget,
    GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *col;
    gint x, y;
    gint *where = g_object_get_data(G_OBJECT(rc_ui->list2_tree_view),
        "multidrag-where");
    if(where && where[0] != -1)
    {
        x = where[0];
        y = where[1];
        rc_gui_list2_block_selection(widget, TRUE, -1, -1);
        if(x==event->x && y==event->y)
        {

            if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(
                rc_ui->list2_tree_view), event->x, event->y, &path, &col,
                NULL, NULL))
            {
                gtk_tree_view_set_cursor(GTK_TREE_VIEW(
                    rc_ui->list2_tree_view), path, col, FALSE);
            }
            if(path) gtk_tree_path_free(path);
        }
    }
    if(event->button==3)
    {
        gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(rc_ui->main_ui,
            "/List2PopupMenu")), NULL, NULL, NULL, NULL, 3,
            gtk_get_current_event_time());
    }
    return FALSE;
}

/**
 * rc_gui_treeview_init:
 *
 * Initialize the tree views in the main window. Can be used only once.
 */

void rc_gui_treeview_init()
{
    rc_ui = rc_gui_get_data();
    gint count = 0;
    rc_ui->list2_tree_model = NULL;
    rc_ui->list1_tree_model = NULL;
    rc_ui->list1_tree_view = gtk_tree_view_new();
    rc_ui->list2_tree_view = gtk_tree_view_new();
    gtk_widget_set_name(rc_ui->list1_tree_view, "RCListView1");
    gtk_widget_set_name(rc_ui->list2_tree_view, "RCListView2");
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(rc_ui->list1_tree_view));
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(rc_ui->list2_tree_view));
    for(count=0;count<5;count++)
        rc_ui->renderer_text[count] = gtk_cell_renderer_text_new();
    for(count=0;count<2;count++)
        rc_ui->renderer_pixbuf[count] = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size(rc_ui->renderer_text[0], 80, -1);
    gtk_cell_renderer_set_fixed_size(rc_ui->renderer_text[1], 120, -1);
    gtk_cell_renderer_set_fixed_size(rc_ui->renderer_text[4], 55, -1);
    g_object_set(G_OBJECT(rc_ui->renderer_text[0]), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    g_object_set(G_OBJECT(rc_ui->renderer_text[1]), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    g_object_set(G_OBJECT(rc_ui->renderer_text[2]), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    g_object_set(G_OBJECT(rc_ui->renderer_text[3]),"ellipsize",
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, NULL);
    g_object_set(G_OBJECT(rc_ui->renderer_text[4]), "xalign", 1.0, "width-chars", 5,
        NULL);
    gtk_cell_renderer_set_fixed_size(rc_ui->renderer_pixbuf[0], 16, -1);
    gtk_cell_renderer_set_fixed_size(rc_ui->renderer_pixbuf[1], 16, -1);
    list1_column = gtk_tree_view_column_new();
    list2_index_column = gtk_tree_view_column_new_with_attributes(
        "#", rc_ui->renderer_pixbuf[1], "stock-id", PLIST2_STATE, NULL);
    list2_title_column = gtk_tree_view_column_new_with_attributes(
        _("Title"), rc_ui->renderer_text[2], "text", PLIST2_TITLE, NULL);
    list2_time_column = gtk_tree_view_column_new_with_attributes(
        _("Length"), rc_ui->renderer_text[4], "text", PLIST2_LENGTH, NULL);
    gtk_tree_view_column_set_title(list1_column, "Playlist");
    gtk_tree_view_column_pack_start(list1_column, rc_ui->renderer_pixbuf[0],
        FALSE);
    gtk_tree_view_column_pack_start(list1_column, rc_ui->renderer_text[0],
        TRUE);
    gtk_tree_view_column_add_attribute(list1_column, 
        rc_ui->renderer_pixbuf[0], "stock-id", PLIST1_STATE);
    gtk_tree_view_column_add_attribute(list1_column,
        rc_ui->renderer_text[0], "text", PLIST1_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(rc_ui->list1_tree_view),
        list1_column);
    gtk_tree_view_column_set_expand(list1_column, TRUE);
    gtk_tree_view_column_set_expand(list2_title_column, TRUE);
    gtk_tree_view_column_set_sizing(list2_time_column,
        GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(list2_index_column ,30);
    gtk_tree_view_column_set_min_width(list2_index_column, 30);
    gtk_tree_view_column_set_max_width(list2_index_column, 30);
    gtk_tree_view_column_set_fixed_width(list2_time_column, 55);
    gtk_tree_view_column_set_alignment(list2_time_column, 1.0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        list2_index_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        list2_title_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        list2_time_column);
    rc_ui->list1_selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(rc_ui->list1_tree_view));
    rc_ui->list2_selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(rc_ui->list2_tree_view));
    gtk_tree_selection_set_mode(rc_ui->list1_selection,
        GTK_SELECTION_BROWSE);
    gtk_tree_selection_set_mode(rc_ui->list2_selection,
        GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(
        rc_ui->list1_tree_view), FALSE);
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(
        rc_ui->list2_tree_view), FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(rc_ui->list1_tree_view),
        FALSE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(rc_ui->list2_tree_view),
        TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(rc_ui->list1_tree_view),
        TRUE);
    rc_gui_list1_tree_view_set_drag();
    rc_gui_list2_tree_view_set_drag();
    g_signal_connect(G_OBJECT(rc_ui->renderer_text[0]), "edited",
        G_CALLBACK(rc_gui_list1_edited), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list1_tree_view), "cursor-changed",
        G_CALLBACK(rc_gui_list1_row_selected), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view), "row-activated",
        G_CALLBACK(rc_gui_list2_row_activated), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view),
        "button-press-event", G_CALLBACK(rc_gui_list2_popup_menu), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view),
        "button-release-event",
        G_CALLBACK(rc_gui_list2_button_release_event), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list1_tree_view),
        "button-release-event", G_CALLBACK(rc_gui_list1_popup_menu), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view),
        "drag_data_received", G_CALLBACK(rc_gui_list2_dnd_data_received),
        NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view), "drag-data-get",
        G_CALLBACK(rc_gui_list2_dnd_data_get), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list2_tree_view), "drag-motion",
        G_CALLBACK(rc_gui_list2_dnd_motion), NULL);
    g_signal_connect(G_OBJECT(rc_ui->list1_tree_view), 
        "drag-data-received", G_CALLBACK(rc_gui_list1_dnd_data_received),
        NULL);
    g_signal_connect(G_OBJECT(rc_ui->list1_tree_view), "drag-data-get",
        G_CALLBACK(rc_gui_list1_dnd_data_get), NULL);
}

/**
 * rc_gui_list_tree_reset_list_store:
 *
 * Reset the playlist views.
 */

void rc_gui_list_tree_reset_list_store()
{
    GtkListStore *store;
    store = rc_plist_get_list_head();
    rc_ui->list1_tree_model = GTK_TREE_MODEL(store);
    gtk_tree_view_set_model(GTK_TREE_VIEW(rc_ui->list1_tree_view),
        rc_ui->list1_tree_model);
    rc_ui->list1_selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(rc_ui->list1_tree_view));
}

/**
 * rc_gui_select_list1:
 * @list_index: the index of the item
 *
 * Make the cursor select one item in the playlist by index.
 */

void rc_gui_select_list1(gint list_index)
{
    GtkTreePath *path;
    if(list_index<0 || list_index>=rc_plist_get_list1_length()) return;
    path = gtk_tree_path_new_from_indices(list_index,-1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(rc_ui->list1_tree_view), path, 
        NULL, FALSE);
    gtk_tree_path_free(path);
}

/**
 * rc_gui_select_list2:
 * @list_index: the index of the item
 *
 * Make the cursor select one music in the playlist.
 */

void rc_gui_select_list2(gint list_index)
{
    GtkTreePath *path;
    if(list_index<0) return;
    path = gtk_tree_path_new_from_indices(list_index, -1);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(rc_ui->list2_tree_view), path, 
        NULL, FALSE);
    gtk_tree_path_free(path);
}

/**
 * rc_gui_list1_new_list:
 *
 * Create a new list with the name the user inputs.
 */

void rc_gui_list1_new_list()
{
    static guint count = 1;
    GtkTreeIter iter;
    gint length = 0;
    gint index;
    gchar new_name[64];
    snprintf(new_name, 63, _("Playlist %u"), count);
    count++;
    length = rc_plist_get_list1_length();
    if(gtk_tree_selection_get_selected(rc_ui->list1_selection ,NULL, &iter))
    {
        index = rc_gui_list1_get_index(&iter);
        if(index==-1) index = length;
    }
    else index = length;
    if(index>length) index = length;
    rc_plist_insert_list(new_name, index);
    rc_gui_select_list1(index);
    rc_gui_list1_rename_list();
}

/**
 * rc_gui_list1_delete_list:
 *
 * Delete the playlist the user selected.
 */

void rc_gui_list1_delete_list()
{
    GtkTreeIter iter;
    int index = 0, list1_index = 0;
    if(rc_plist_get_list1_length()<=1) return;
    if(rc_ui->list1_selection==NULL) return;
    if(gtk_tree_selection_get_selected(rc_ui->list1_selection, NULL, &iter))
    {
        index = rc_gui_list1_get_index(&iter);
        if(index==-1) return;
    }
    else return;
    if(index<0) return;
    rc_plist_remove_list(index);
    rc_gui_select_list1(index);
    if(rc_plist_play_get_index(&list1_index, NULL) && list1_index==index)
    {
        rc_plist_play_by_index(0, 0);
    }
}

/**
 * rc_gui_list1_get_selected_index:
 *
 * Return the index of the selected playlist.
 *
 * Returns: The index of the selected playlist.
 */

gint rc_gui_list1_get_selected_index()
{
    GtkTreePath *list1_path;
    gint *indices = NULL;
    gint list1_index;
    if(!gtk_tree_row_reference_valid(rc_ui->list1_selected_reference))
        return -1;
    list1_path = gtk_tree_row_reference_get_path(
        rc_ui->list1_selected_reference);
    indices = gtk_tree_path_get_indices(list1_path);
    list1_index = indices[0];
    gtk_tree_path_free(list1_path);
    return list1_index;
}

/**
 * rc_gui_list2_delete_lists:
 *
 * Delete the selected item(s) in the playlist.
 */

void rc_gui_list2_delete_lists()
{
    static GList *path_list = NULL;
    GList *list_foreach;
    GtkTreeIter iter;
    if(path_list!=NULL)
    {
        g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(path_list);
        path_list = NULL;
    }
    path_list = gtk_tree_selection_get_selected_rows(
        rc_ui->list2_selection, NULL);
    if(path_list==NULL) return;
    path_list = g_list_sort(path_list, (GCompareFunc)gtk_tree_path_compare);
    for(list_foreach=g_list_last(path_list);list_foreach!=NULL;
        list_foreach=g_list_previous(list_foreach))
    {
        gtk_tree_model_get_iter(rc_ui->list2_tree_model, &iter,
            list_foreach->data);
        gtk_list_store_remove(GTK_LIST_STORE(rc_ui->list2_tree_model),
            &iter);
    }
    if(path_list!=NULL)
    {
        g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(path_list);
        path_list = NULL;
    }
}

/**
 * rc_gui_list2_select_all:
 *
 * Select all items in the playlist.
 */

void rc_gui_list2_select_all()
{
    gtk_tree_selection_select_all(rc_ui->list2_selection);
}

/**
 * rc_gui_list1_rename_list:
 *
 * Rename a list (make the name of the selected playlist editable).
 */

void rc_gui_list1_rename_list()
{
    GtkTreeIter iter;
    GtkTreePath *path;
    if(gtk_tree_selection_get_selected(rc_ui->list1_selection, NULL,
        &iter))
    {
        path = gtk_tree_model_get_path(rc_ui->list1_tree_model, &iter);
        if(path==NULL) return;
    }
    else return;
    g_object_set(G_OBJECT(rc_ui->renderer_text[0]), "editable", TRUE,
        "editable-set", TRUE, NULL);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(rc_ui->list1_tree_view), path,
        list1_column, TRUE);
    g_object_set(G_OBJECT(rc_ui->renderer_text[0]), "editable", FALSE,
        "editable-set", FALSE, NULL);
    gtk_tree_path_free(path);
}

/**
 * rc_gui_list1_get_model:
 *
 * Return the GtkTreeModel of list1 in player.
 *
 * Returns: The GtkTreeModel.
 */

GtkTreeModel *rc_gui_list1_get_model()
{
    return rc_ui->list1_tree_model;
}

/**
 * rc_gui_list2_get_model:
 *
 * Return the GtkTreeModel of list2 in player.
 *
 * Returns: The GtkTreeModel.
 */

GtkTreeModel *rc_gui_list2_get_model()
{
    return rc_ui->list2_tree_model;
}

/**
 * rc_gui_list1_get_cursor:
 * @iter: the uninitialized GtkTreeIter
 *
 * Get the GtkTreeIter of the selected item.
 *
 * Returns: TRUE, if iter was set.
 */

gboolean rc_gui_list1_get_cursor(GtkTreeIter *iter)
{
    GtkTreePath *path;
    gboolean flag;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(rc_ui->list1_tree_view), &path,
        NULL);
    flag = gtk_tree_model_get_iter(GTK_TREE_MODEL(rc_ui->list1_tree_model),
        iter, path);
    gtk_tree_path_free(path);
    return flag;
}

/**
 * rc_gui_list2_get_cursor:
 * @iter: the uninitialized GtkTreeIter
 *
 * Get the GtkTreeIter of the selected item.
 *
 * Returns: TRUE, if iter was set.
 */

gboolean rc_gui_list2_get_cursor(GtkTreeIter *iter)
{
    GtkTreePath *path;
    gboolean flag;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(rc_ui->list2_tree_view), &path,
        NULL);
    if(path==NULL) return FALSE;
    flag = gtk_tree_model_get_iter(GTK_TREE_MODEL(rc_ui->list2_tree_model),
        iter, path);
    gtk_tree_path_free(path);
    return flag;
}

/**
 * rc_gui_list1_scroll_to_index:
 * @list_index: the index of the item
 *
 * Make the list scrolled to the given index.
 */

void rc_gui_list1_scroll_to_index(gint list_index)
{
    GtkTreePath *path;
    if(list_index<0 || list_index>=rc_plist_get_list1_length()) return;
    path = gtk_tree_path_new_from_indices(list_index,-1);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(rc_ui->list1_tree_view), path,
        NULL, FALSE, 0.0, 0.0);
    gtk_tree_path_free(path);
}

/**
 * rc_gui_list2_scroll_to_index:
 * @list_index: the index of the item
 *
 * Make the playlist scrolled to the given index
 */

void rc_gui_list2_scroll_to_index(gint list_index)
{
    GtkTreePath *path;
    if(list_index<0) return;
    path = gtk_tree_path_new_from_indices(list_index, -1);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(rc_ui->list2_tree_view), path,
        NULL, FALSE, 0.0, 0.0);
    gtk_tree_path_free(path);
}

