
/**
 * Utils - 
 * Copyright (C) 2010 Sian Cao <sycao@redflag-linux.com>
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *  
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _semacs_util_h
#define _semacs_util_h

#include <X11/Xlib.h>
#include <glib.h>
#include <X11/Xft/Xft.h>

#define DEF_CLS(_name) typedef struct _name _name

DEF_CLS(se_env);  // environment 
DEF_CLS(se_text_viewer); // viewer of text editor

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "semacs"
#endif

#define se_error(fmt, ...) g_error( fmt, ##__VA_ARGS__ )
#define se_msg(fmt, ...)   g_message( fmt, ##__VA_ARGS__ )
#define se_warn(fmt, ...)  g_warning( fmt, ##__VA_ARGS__ )
#define se_debug(fmt, ...) g_debug( fmt, ##__VA_ARGS__ )




struct se_env 
{
    Display *display;
    int numScreens;
    int screen;  // which screen the editor is showed
    Colormap colormap;
    Visual *visual;
    Window root;

    XftFont *xftFont;
    int fontMaxHeight;
    int glyphMaxWidth;
    
    gboolean exitLoop;
};

se_env* se_env_init();
void se_env_release(se_env* env);
void se_env_quit(se_env* env);
gboolean se_env_change_font( se_env* env, const char* name );

DEF_CLS(se_curosr);
struct se_curosr
{
    int x;
    int y;
};

DEF_CLS(se_size);
struct se_size
{
    int width;
    int height;
};

DEF_CLS(se_rect);
struct se_rect
{
    se_curosr start;
    se_size size;
};

typedef void (*event_handler_t)( se_text_viewer* viewer, XEvent* ev);
#define SE_VIEW_HANDLER(_name) void _name(se_text_viewer* viewer, XEvent* ev );

struct se_text_viewer
{
    se_env* env;
    Window view;
    int columns; // viewable width in cols
    int rows; // viewable height in rows
    
    int physicalWidth;  // real width of view
    int physicalHeight; // real height of view
    
    XftDraw *xftDraw;

    se_curosr cursor;  // where cursor is, size in pixels
    se_curosr logicalCursor; // where cursor is, pos in logical (row, col)
    
    event_handler_t key_handler;
    event_handler_t mouse_handler;
    event_handler_t configure_change_handler;
};

se_text_viewer* se_text_viewer_create( se_env* );
void se_text_viewer_show( se_text_viewer* viewer );
void se_text_viewer_update( se_text_viewer* viewer );
void se_text_viewer_draw_create( se_text_viewer* viewer );

SE_VIEW_HANDLER( se_text_viewer_key_event );
SE_VIEW_HANDLER( se_text_viewer_mouse_event );
SE_VIEW_HANDLER( se_text_viewer_configure_change_handler );

void se_text_viewer_size_update( se_text_viewer* viewer, int width, int height );
void se_text_viewer_update_cursor( se_text_viewer* viewer, int nchars, se_size incr );

void se_draw_text_utf8( se_text_viewer* viewer,
                        XftColor* color, const char* utf8 );


#endif

