/**
 * View with Xlib Impl - 
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

#include "xview.h"

SE_VIEW_HANDLER( se_text_viewer_key_event );
SE_VIEW_HANDLER( se_text_viewer_mouse_event );
SE_VIEW_HANDLER( se_text_viewer_configure_change_handler );

static void se_text_viewer_draw_create( se_text_viewer* viewer );
static void se_text_viewer_move_cursor( se_text_viewer* viewer, int col, int row );
static void se_text_viewer_forward_cursor( se_text_viewer* viewer, int nchars );

static void se_draw_char_utf8( se_text_viewer* viewer, XftColor* color, int c );
static void se_draw_text_utf8( se_text_viewer*, XftColor*, const char*, int );

// if nchars > 0, move forward, else move backward
void se_text_viewer_forward_cursor( se_text_viewer* viewer, int nchars )
{
    if ( !nchars ) return;
    
    assert( viewer );
    /* se_debug( "origin: (%d, %d)", viewer->cursor.column, viewer->cursor.row ); */

    if ( nchars > 0 ) {
        while ( nchars > 0 ) {
            if ( viewer->cursor.column + 1 == viewer->columns ) {
                viewer->cursor.column = 0;
                ++viewer->cursor.row;
                --nchars;
                continue;
            }

            ++viewer->cursor.column;
            --nchars;
        }

    } else {
        while ( nchars < 0 ) {
            if ( viewer->cursor.column == 0 ) {
                if ( viewer->cursor.row == 0 )
                    break;

                viewer->cursor.column = viewer->columns-1;
                --viewer->cursor.row;
                ++nchars;
                continue;
            }

            --viewer->cursor.column;
            ++nchars;
        }
    }
    
    viewer->physicalCursor.x = viewer->cursor.column * viewer->env->glyphMaxWidth;
    viewer->physicalCursor.y = viewer->cursor.row * viewer->env->glyphMaxHeight;
    
    /* se_debug( "after: (%d, %d)", viewer->cursor.column, viewer->cursor.row ); */
}

void se_text_viewer_move_cursor( se_text_viewer* viewer, int col, int row )
{
    assert( viewer );
    assert( col >= 0 && col < viewer->columns );
    assert( row >= 0 && row < viewer->rows );
    
    viewer->cursor.column = col;
    viewer->cursor.row = row;
    viewer->physicalCursor.x = viewer->cursor.column * viewer->env->glyphMaxWidth;
    viewer->physicalCursor.y = viewer->cursor.row * viewer->env->glyphMaxHeight;
}

static void se_text_viewer_updateSize( se_text_viewer* viewer, int width, int height )
{
    assert( viewer );
    assert( width && height );
    
    viewer->physicalWidth = width;
    viewer->physicalHeight = height;

    const se_env* env = viewer->env;
    viewer->columns = width / env->glyphMaxWidth;
    viewer->rows = height / env->glyphMaxHeight;

    se_debug( "new size in pixels (%d, %d), in chars (%d, %d)",
              width, height, viewer->columns, viewer->rows );    
    //TODO: check if cursor if out of display
}

static void se_text_viewer_show( se_text_viewer* viewer )
{
    XMapWindow( viewer->env->display, viewer->view );
    XFlush( viewer->env->display );
}

// send draw event and do real update in redisplay routine
static void se_text_viewer_repaint( se_text_viewer* viewer )
{
    se_debug ( "enter expose event" );
    XWindowAttributes attrs;
    XGetWindowAttributes( viewer->env->display, viewer->view, &attrs);
    viewer->updateSize( viewer, attrs.width, attrs.height );

    //refill text viewer
    se_world *world = viewer->env->world;
    g_assert( world );

    se_buffer *cur_buf = world->current;
    g_assert( cur_buf );

    //FIXME: this is slow algo just as a demo, change it later
    //int size = SE_MAX_COLUMNS * SE_MAX_ROWS;

    se_line* lp = cur_buf->getCurrentLine( cur_buf );
    int nr_lines = cur_buf->getLineCount( cur_buf );
    
    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        memcpy( (viewer->content + r*SE_MAX_ROWS), se_line_getData(lp),
                se_line_getLineLength(lp) );
        lp = lp->next;
    }
    
    /* XExposeEvent ev; */
    /* XSendEvent( viewer->env->display, viewer->view, False, ExposureMask, &ev ); */
}

static void se_text_viewer_redisplay(se_text_viewer* viewer )
{
    se_debug( "redisplay" );
    
    se_env *env = viewer->env;
    XRenderColor renderClr = {
        .red = 0x0,
        .green = 0x0,
        .blue = 0x0,
        .alpha = 0xeeee
    };
        
    XftColor clr;
    XftColorAllocValue( env->display, env->visual, env->colormap, &renderClr, &clr );

    se_world *world = viewer->env->world;
    g_assert( world );
    se_buffer *cur_buf = world->current;
    g_assert( cur_buf );
    int nr_lines = cur_buf->getLineCount( cur_buf );

    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        char *data = viewer->content + r*SE_MAX_ROWS;
        int data_len = strlen( data );
        /* se_draw_text_utf8( viewer, &clr, data, MIN(data_len, viewer->columns) ); */
        /* se_text_viewer_move_cursor( viewer, 0, viewer->cursor.row + 1 ); */

        for (int i = 0; i < data_len; ++i) {
            se_draw_char_utf8( viewer, &clr, data[i] );
            se_text_viewer_forward_cursor( viewer, 1 );
        }
        se_text_viewer_move_cursor( viewer, 0, viewer->cursor.row + 1 );
        
        char tmp[MIN(data_len, viewer->columns)];
        snprintf( tmp, sizeof(tmp), "%s", data);
        se_debug( "draw: %s", tmp );
    }
    
    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        char *data = viewer->content + r*SE_MAX_ROWS;
        int data_len = strlen( data );
        se_draw_text_utf8( viewer, &clr, data, MIN(data_len, viewer->columns) );
        se_text_viewer_move_cursor( viewer, 0, viewer->cursor.row + 1 );
    }

    XftColorFree( env->display, env->visual, env->colormap, &clr );
}

void se_text_viewer_configure_change_handler(se_text_viewer* viewer, XEvent* ev )
{
    se_debug( "enter configure_change_handler" );
    gboolean need_update = FALSE;
    
    if ( ev->type == ConfigureRequest ) {
        XConfigureEvent configEv = ev->xconfigure;
        if ( viewer->physicalWidth != configEv.width
             || viewer->physicalHeight != configEv.height ) {
            need_update = TRUE;
        }
        
    } else if ( ev->type == ResizeRequest ) {
        XResizeRequestEvent resizeEv = ev->xresizerequest;
        if ( viewer->physicalWidth != resizeEv.width
             || viewer->physicalHeight != resizeEv.height ) {
            need_update = TRUE;
        }
    }

    if ( need_update ) {
        XWindowAttributes attrs;
        XGetWindowAttributes( viewer->env->display, viewer->view, &attrs);
        viewer->updateSize( viewer, attrs.width, attrs.height );
    }
}

void se_text_viewer_key_event(se_text_viewer* viewer, XEvent* ev )
{
    assert( viewer && ev );

    XKeyEvent kev = ev->xkey;
    KeySym key_sym = XKeycodeToKeysym( viewer->env->display, kev.keycode, 0 );
    
    if ( kev.type == KeyPress ) {
        se_debug( "enter key press event, keysym(0x%x): %s",
                  (guint)key_sym, XKeysymToString(key_sym) );

        switch( key_sym ) {
        case XK_A...XK_Z:
            se_debug( "upper case" );
            
        case XK_a...XK_z:
        {
            XRenderColor renderClr = {
                .red = 0x0,
                .green = 0x0,
                .blue = 0x0,
                .alpha = 0xeeee
            };
        
            XftColor clr;
            XftColorAllocValue( viewer->env->display, viewer->env->visual, 
                                viewer->env->colormap, &renderClr, &clr );

            char *str = XKeysymToString(key_sym);
            int c = str[0];
            if ( kev.state & ShiftMask )
                c = toupper( c );
            
            se_draw_char_utf8( viewer, &clr, c );
            XftColorFree( viewer->env->display, viewer->env->visual,
                          viewer->env->colormap, &clr);
            
            break;
        }

        case XK_space:
        {
            // do testing round: randomly draw chars or strings this test give
            // an idea that each single char's boundary can be calculated
            // independently from XftTextExtents*, it's just as the same effect
            // as we do for a long string
            XRenderColor renderClr = {
                .red = 0x0,
                .green = 0x0,
                .blue = 0x0,
                .alpha = 0xeeee
            };
        
            XftColor clr;
            XftColorAllocValue( viewer->env->display, viewer->env->visual, 
                                viewer->env->colormap, &renderClr, &clr );

            char buf[] = "HWjJlLkQqTpP";
            int len = strlen(buf);
            
            se_text_viewer_move_cursor( viewer, 0, 0 );
            for (int i = 0; i < len; ++i) {
                se_draw_char_utf8( viewer, &clr, buf[i] );                
            }

            se_text_viewer_move_cursor( viewer, 0, 1 );
            se_draw_text_utf8( viewer, &clr, buf, len );
            
            XftColorFree( viewer->env->display, viewer->env->visual,
                          viewer->env->colormap, &clr);
            break;
        }
        
        default:
            break;
        }

    } else {
        switch( key_sym ) {
        case XK_Escape:
            se_env_quit( viewer->env );
            break;
        }
    }
}

void se_text_viewer_mouse_event(se_text_viewer* viewer, XEvent* ev )
{
    se_debug( "enter mouse event" );
}

void se_text_viewer_draw_create( se_text_viewer* viewer )
{
    assert ( viewer->env->xftFont );

    if ( viewer->xftDraw ) {
        XftDrawDestroy( viewer->xftDraw );
        viewer->xftDraw = NULL;
    }
    
    viewer->xftDraw = XftDrawCreate(
        viewer->env->display, viewer->view, viewer->env->visual,
        viewer->env->colormap );
}

void se_draw_char_utf8( se_text_viewer* viewer, XftColor* color, int c )
{
    assert( viewer && viewer->xftDraw );

    const se_env* env = viewer->env;
    se_position cur_pos = viewer->physicalCursor;
    se_position start_pos = {
        .x = cur_pos.x,
        .y = cur_pos.y + env->glyphAscent,
    };
    XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont,
                       start_pos.x, start_pos.y, (const XftChar8*)&c, 1 );
//    se_text_viewer_forward_cursor( viewer, 1 );
}

void se_draw_text_utf8( se_text_viewer* viewer, XftColor* color,
                        const char* utf8, int utf8_len )
{
    assert( viewer && viewer->xftDraw );

    const se_env* env = viewer->env;
    se_position cur_pos = viewer->physicalCursor;

    /* XGlyphInfo metric; */
    /* XftTextExtentsUtf8( env->display, env->xftFont,  */
    /*                     (const XftChar8*)utf8, utf8_len, &metric); */
    /* se_debug( "metric: (x %d, y %d, w %d, h %d, xoff %d, yoff %d)", */
    /*           metric.x, metric.y, metric.width, metric.height, */
    /*           metric.xOff, metric.yOff ); */

    se_position start_pos = {
        .x = cur_pos.x,
        .y = cur_pos.y + env->glyphAscent,
    };
    XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont,
                       start_pos.x, start_pos.y,
                       (const XftChar8*)utf8, utf8_len );
}


se_text_viewer* se_text_viewer_create( se_env* env )
{
    assert( env );

    se_text_viewer* viewer = g_malloc0( sizeof(se_text_viewer) );
    if ( !viewer ) {
        se_error( "can not malloc a viewer obj" );
    }
    
    viewer->env = env;    
    int width = 800;
    int height = 600;
    
    viewer->view = XCreateSimpleWindow(
        env->display, env->root, 0, 0, width, height, 0,
        BlackPixel(env->display, env->screen),
        WhitePixel(env->display, env->screen) );
    
    viewer->key_handler = se_text_viewer_key_event;
    viewer->mouse_handler = se_text_viewer_mouse_event;
    viewer->configure_change_handler = se_text_viewer_configure_change_handler;

    viewer->show = se_text_viewer_show;
    viewer->repaint = se_text_viewer_repaint;
    viewer->redisplay = se_text_viewer_redisplay;
    viewer->updateSize = se_text_viewer_updateSize;
    
    se_text_viewer_draw_create( viewer );

    viewer->content = g_malloc0( SE_MAX_COLUMNS * SE_MAX_ROWS );
    
    return viewer;
}
