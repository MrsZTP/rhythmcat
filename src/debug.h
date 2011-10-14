/*
 * Debug Declaration
 *
 * debug.h
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

#ifndef HAVE_DEBUG_H
#define HAVE_DEBUG_H

#include <glib.h>
#include <glib/gprintf.h>

G_BEGIN_DECLS

#ifndef DEBUG_MODE
#define DEBUG_MODE FALSE
#endif

gboolean rc_debug_get_flag();
void rc_debug_set_mode(gboolean mode);
gint rc_debug_print(const gchar *format, ...);
gint rc_debug_perror(const gchar *format, ...);
gint rc_debug_pmsg(const gchar *format, ...);
gint rc_debug_module_pmsg(const gchar *module_name, const gchar *format,
    ...);
gint rc_debug_module_print(const gchar *module_name, const gchar *format,
    ...);
gint rc_debug_module_perror(const gchar *module_name, const gchar *format,
    ...);
void rc_debug_print_mem_profile();

G_END_DECLS

#endif

