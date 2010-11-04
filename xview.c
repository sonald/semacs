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

/* static void se_text_viewer_draw_create( se_text_viewer* viewer ); */
/* static void se_text_viewer_move_cursor( se_text_viewer* viewer, int col, int row ); */
static void se_draw_text_utf8( se_text_viewer*, XftColor*, const char*, int );

// if nchars > 0, move forward, else move backward
/* static void se_text_viewer_forward_cursor( se_text_viewer* viewer, int nchars ) */
/* { */
/*     if ( !nchars ) return; */
    
/*     assert( viewer ); */
/*     /\* se_debug( "origin: (%d, %d)", viewer->cursor.column, viewer->cursor.row ); *\/ */

/*     if ( nchars > 0 ) { */
/*         while ( nchars > 0 ) { */
/*             if ( viewer->cursor.column + 1 == viewer->columns ) { */
/*                 viewer->cursor.column = 0; */
/*                 ++viewer->cursor.row; */
/*                 --nchars; */
/*                 continue; */
/*             } */

/*             ++viewer->cursor.column; */
/*             --nchars; */
/*         } */

/*     } else { */
/*         while ( nchars < 0 ) { */
/*             if ( viewer->cursor.column == 0 ) { */
/*                 if ( viewer->cursor.row == 0 ) */
/*                     break; */

/*                 viewer->cursor.column = viewer->columns-1; */
/*                 --viewer->cursor.row; */
/*                 ++nchars; */
/*                 continue; */
/*             } */

/*             --viewer->cursor.column; */
/*             ++nchars; */
/*         } */
/*     } */
    
/*     viewer->physicalCursor.x = viewer->cursor.column * viewer->env->glyphMaxWidth; */
/*     viewer->physicalCursor.y = viewer->cursor.row * viewer->env->glyphMaxHeight; */
    
/*     /\* se_debug( "after: (%d, %d)", viewer->cursor.column, viewer->cursor.row ); *\/ */
/* } */

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
    se_line* lp = cur_buf->lines;
    int nr_lines = cur_buf->getLineCount( cur_buf );
    
    for (int r = 0; r < MIN(viewer->rows, nr_lines); ++r) {
        memcpy( (viewer->content + r*SE_MAX_ROWS), se_line_getData(lp),
                se_line_getLineLength(lp) );
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

    se_text_viewer_move_cursor( viewer, 0, 0 );
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
    
    se_key sekey = se_key_null_init();
    /* if ( key_sym == XK_Control_L */
    /*      || key_sym == XK_Control_R */
    /*      || key_sym == XK_Meta_L */
    /*      || key_sym == XK_Meta_R */
    /*      || key_sym == XK_Super_L */
    /*      || key_sym == XK_Super_R */
    /*      || key_sym == XK_Super_L */
    /*      || key_sym == XK_Super_R ) */
    /*     return sekey; */
    
    char *key_str = XKeysymToString( key_sym );
    if ( key_sym == XK_Escape ) {
        sekey.modifiers = EscapeDown;
    }
    
    if ( strlen(key_str) > 1 ) {
        se_debug( "sekey: %s", se_key_to_string(sekey) );        
        return sekey;
    }
    
    sekey.ascii = key_str[0];
    
    if ( kev.state & ControlMask ) {
        sekey.modifiers |= ControlDown;
    }
    if ( kev.state & ShiftMask ) {
        sekey.modifiers |= ShiftDown;
    }
    if ( kev.state & Mod1Mask ) {
        sekey.modifiers |= MetaDown;
    }

    /* if ( key_sym == XK_Super_L || key_sym == XK_Super_R ) { */
    /*     sekey.modifiers |= SuperDown; */
    /* } */
    if ( kev.state & Mod4Mask ) {
        // this is no good, Mod4 can be map to other keysym
        sekey.modifiers |= SuperDown;
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
    
        if ( se_key_is_only( sekey, EscapeDown )  )
            se_env_quit( viewer->env );
    
        se_world *world = viewer->env->world;
        se_command_args args;
        bzero( &args, sizeof args );
        while ( world->dispatchCommand( world, &args, sekey ) != TRUE ) {
            sekey = se_delayed_wait_key( viewer );
            if ( se_key_is_only( sekey, EscapeDown ) )
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

/* void se_draw_char_utf8( se_text_viewer* viewer, XftColor* color, int c ) */
/* { */
/*     assert( viewer && viewer->xftDraw ); */

/*     const se_env* env = viewer->env; */
/*     se_position cur_pos = viewer->physicalCursor; */
/*     se_position start_pos = { */
/*         .x = cur_pos.x, */
/*         .y = cur_pos.y + env->glyphAscent, */
/*     }; */
/*     XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont, */
/*                        start_pos.x, start_pos.y, (const XftChar8*)&c, 1 ); */
/* } */

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
