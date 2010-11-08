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
#include "key.h"

SE_VIEW_HANDLER( se_text_viewer_key_event );
SE_VIEW_HANDLER( se_text_viewer_mouse_event );
SE_VIEW_HANDLER( se_text_viewer_configure_change_handler );

static se_position se_text_cursor_to_physical( se_text_viewer* viewer, se_cursor cur )
{
    return (se_position){ cur.column * viewer->env->glyphMaxWidth,
            cur.row * viewer->env->glyphMaxHeight };
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

SE_UNUSED void se_draw_char_utf8( se_text_viewer* viewer, XftColor* color,
                                  se_cursor cur, int c )
{
    assert( viewer && viewer->xftDraw );

    const se_env* env = viewer->env;
    se_position cur_pos = se_text_cursor_to_physical(viewer, cur);
    se_position start_pos = {
        .x = cur_pos.x,
        .y = cur_pos.y + env->glyphAscent,
    };
    XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont,
                       start_pos.x, start_pos.y, (const XftChar8*)&c, 1 );
}

static void se_draw_text_utf8( se_text_viewer* viewer, XftColor* color,
                        se_cursor cur, const char* utf8, int utf8_len )
{
    assert( viewer && viewer->xftDraw );
    const se_env* env = viewer->env;
    se_position cur_pos = se_text_cursor_to_physical(viewer, cur);
    se_position start_pos = {
        .x = cur_pos.x,
        .y = cur_pos.y + env->glyphAscent,
    };
    XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont,
                       start_pos.x, start_pos.y,
                       (const XftChar8*)utf8, utf8_len );
}

static void se_draw_buffer_point( se_text_viewer* viewer )
{
    se_env *env = viewer->env;
    se_world *world = viewer->env->world;
    g_assert( world );

    se_buffer *cur_buf = world->current;
    g_assert( cur_buf );

    se_cursor point_cur = {
        .row = cur_buf->getLine( cur_buf ),
        .column = cur_buf->getCurrentColumn( cur_buf )
    };
    se_position point_pos = se_text_cursor_to_physical( viewer, point_cur );
    point_pos.y -= (env->glyphMaxHeight - env->glyphAscent)/2;
    

    XRenderColor renderClr = {
        .red = 0x00,
        .green = 0xffff,
        .blue = 0x00,
        .alpha = 0x00
    };
        
    XftColor clr;
    XftColorAllocValue( env->display, env->visual, env->colormap, &renderClr, &clr );
    
    XftDrawRect( viewer->xftDraw, &clr, point_pos.x, point_pos.y, 2, env->glyphMaxHeight );

    XftColorFree( env->display, env->visual, env->colormap, &clr );
}

// send draw event and do real update in redisplay routine
static void se_text_viewer_repaint( se_text_viewer* viewer )
{
    XWindowAttributes attrs;
    XGetWindowAttributes( viewer->env->display, viewer->view, &attrs);
    viewer->updateSize( viewer, attrs.width, attrs.height );

    //refill text viewer
    se_world *world = viewer->env->world;
    g_assert( world );

    se_buffer *cur_buf = world->current;
    g_assert( cur_buf );

    //FIXME: this is slow algo just as a demo, change it later
    se_line* lp = cur_buf->lines;
    int nr_lines = cur_buf->getLineCount( cur_buf );
    se_debug( "paint rect: rows: %d", nr_lines );
    
    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        const char* buf = se_line_getData( lp );
        int cols = se_line_getLineLength( lp );
        if ( buf[cols-1] == '\n' ) cols--;
        memcpy( (viewer->content + r*SE_MAX_ROWS), buf, cols );
        /* se_debug( "draw No.%d: [%s]", r, buf ); */
        lp = lp->next;
    }

    
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
    se_debug( "update buf %s [%d, %d]", cur_buf->getBufferName(cur_buf),
              viewer->columns, MIN(viewer->rows, nr_lines) );    

    // this clear the whole window ( slow )
    XClearArea( env->display, viewer->view, 0, 0, 0, 0, False );
    
    viewer->cursor = (se_cursor){ 0, 0 };
    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        char *data = viewer->content + r*SE_MAX_ROWS;
        int data_len = strlen( data );
        se_draw_text_utf8( viewer, &clr, viewer->cursor,
                           data, MIN(data_len, viewer->columns) );
        viewer->cursor = (se_cursor){0, viewer->cursor.row + 1};
    }

    se_draw_buffer_point( viewer );
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

static se_key se_key_event_process( Display *display, XKeyEvent kev )
{
    int index = 0;
    int mask = 0;
    
    if ( kev.state & ShiftMask ) {
        mask = 0x01;
    }
    if ( kev.state & LockMask ) {
        mask |= 0x02;
    }
    if ( mask ) index = 1;
    
    KeySym key_sym = XKeycodeToKeysym( display, kev.keycode, index );
    if ( key_sym == NoSymbol ) {
        se_debug( "NoSymbol for keycode %d", kev.keycode );
    }
    se_debug( "keysym: %s", XKeysymToString(key_sym) );
    se_key sekey = se_key_null_init();
    
    char *key_str = XKeysymToString( key_sym );

    // FIXME: this is a simplified version, since KeySym actually follow latin-1
    // except for some terminal control keys
    sekey.ascii = key_sym;
    if ( strlen(key_str) > 1 ) {
        se_debug( "sekey: %s", se_key_to_string(sekey) );        
        return sekey;
    }
    
    /* sekey.ascii = key_str[0]; */
    
    if ( kev.state & ControlMask ) {
        sekey.modifiers |= ControlDown;
    }
    if ( kev.state & ShiftMask ) {
        sekey.modifiers |= ShiftDown;
    }
    if ( kev.state & Mod1Mask ) {
        sekey.modifiers |= MetaDown;
    }

    if ( kev.state & Mod4Mask ) {
        // this is no good, Mod4 can be map to other keysym
        sekey.modifiers |= SuperDown;
    }

    if ( sekey.modifiers == ShiftDown ) { // here Shift is not part of combination
                                        // only change case of char
        sekey.modifiers = 0;
    }
    
    se_debug( "sekey: %s", se_key_to_string(sekey) );
    return sekey;
}

static se_key se_delayed_wait_key(se_text_viewer* viewer)
{
    se_debug( "waiting for a key..." );    
    XEvent ev;
    while ( XCheckWindowEvent( viewer->env->display, viewer->view,
                               KeyPressMask | KeyReleaseMask, &ev ) == False ) {
        usleep( 100 );
    }

    g_assert( ev.type == KeyRelease || ev.type == KeyPress );
    se_debug( "wait for a key done" );
    return se_key_event_process( viewer->env->display, ev.xkey );
}

void se_text_viewer_key_event(se_text_viewer* viewer, XEvent* ev )
{
    assert( viewer && ev );

    XKeyEvent kev = ev->xkey;
    if ( kev.type == KeyPress ) {
        se_key sekey = se_key_event_process( viewer->env->display, kev );
        if ( se_key_is_null(sekey) )
            return;
    
        if ( sekey.ascii == XK_Escape )
            se_env_quit( viewer->env );
    
        se_world *world = viewer->env->world;
        se_command_args args;
        bzero( &args, sizeof args );
        while ( world->dispatchCommand( world, &args, sekey ) != TRUE ) {
            sekey = se_delayed_wait_key( viewer );
            if ( sekey.ascii == XK_Escape )
                se_env_quit( viewer->env );
        }

        if ( world->current->isModified(world->current) ) {
            viewer->repaint( viewer );
            viewer->redisplay( viewer );
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
