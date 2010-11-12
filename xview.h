
/**
 * Viewer Implementation with xlib - 
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

#ifndef _semacs_xview_h
#define _semacs_xview_h

#include "util.h"
#include "env.h"

DEF_CLS(se_cursor);
struct se_cursor
{
    int column;
    int row;
};

DEF_CLS(se_position);
struct se_position
{
    int x;
    int y;
};

se_position se_cursor_to_pixels_pos(se_env* env, se_cursor cur);

DEF_CLS(se_size);
struct se_size
{
    int width;
    int height;
};

DEF_CLS(se_rect);
struct se_rect
{
    se_cursor start;
    se_size size;
};

// this limits make decision a lot easier now, limit policy gonna change later
#define SE_MAX_COLUMNS 512   // longest line allowed
#define SE_MAX_ROWS    128   // most visible lines of text

DEF_CLS(se_text_viewer); // viewer of text editor

typedef void (*event_handler_t)( se_text_viewer* viewer, XEvent* ev);
#define SE_VIEW_HANDLER(_name) void _name(se_text_viewer* viewer, XEvent* ev );

struct se_text_viewer
{
    se_env* env;
    Window view;
    XIC xic; // one ic per window
    
    int columns; // viewable width in cols
    int rows; // viewable height in rows
    
    int physicalWidth;  // real width of view
    int physicalHeight; // real height of view

    gchar *content;  // which contains columns * rows chars, may utf8 next version
    XftDraw *xftDraw;
    se_cursor cursor; // where cursor is, pos in logical (row, col),
                      // this is not point of editor

    void (*show)( se_text_viewer* viewer );
    void (*repaint)( se_text_viewer* viewer );
    void (*redisplay)( se_text_viewer* viewer );    
    void (*updateSize)( se_text_viewer* viewer, int width, int height );

    event_handler_t key_handler;
    event_handler_t mouse_handler;
    event_handler_t configure_change_handler;
};

extern se_text_viewer* se_text_viewer_create( se_env* );

#endif

