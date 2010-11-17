
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

#include "env.h"
#include "view.h"

se_position se_cursor_to_pixels_pos(se_env* env, se_cursor cur);

DEF_CLS(se_text_xviewer);

typedef void (*event_handler_t)( se_text_xviewer* viewer, XEvent* ev);
#define SE_VIEW_HANDLER(_name) void _name(se_text_xviewer* viewer, XEvent* ev );

struct se_text_xviewer
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

    void (*show)( se_text_xviewer* viewer );
    void (*repaint)( se_text_xviewer* viewer );
    void (*redisplay)( se_text_xviewer* viewer );    
    void (*updateSize)( se_text_xviewer* viewer, int width, int height );

    event_handler_t key_handler;
    event_handler_t mouse_handler;
    event_handler_t configure_change_handler;
};

#ifdef __cplusplus
extern "C" {
#endif

se_text_xviewer* se_text_xviewer_create( se_env*);
int main_loop(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif

