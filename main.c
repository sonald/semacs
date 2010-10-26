/**
 * Main Entry - 
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

#include "util.h"
#include <X11/Xlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

void se_env_quit(se_env* env)
{
    env->exitLoop = 1;
}

se_env* se_env_init()
{
    se_env* env = (se_env*)g_malloc0( sizeof(se_env) );
    if ( !env )
        se_error( "can not allocate se_env" );

    env->display = XOpenDisplay( NULL );
    if ( env->display == NULL )
        se_error( "cannot connect to X server" );

    env->numScreens = ScreenCount( env->display );
    env->screen = DefaultScreen( env->display );
    env->root = RootWindow( env->display, env->screen );
    env->visual = DefaultVisual(env->display, env->screen);
    env->colormap = DefaultColormap(env->display, env->screen);
    
    if ( !se_env_change_font( env, "Inconsolate-13" ) ) {
        env->xftFont = NULL;
    }
    
    return env;
}

void se_env_release(se_env* env)
{
    if ( env ) {
        XCloseDisplay( env->display );
        g_free( env );
    }
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

    XWindowAttributes attrs;
    XGetWindowAttributes( env->display, viewer->view, &attrs);

    se_text_viewer_size_update( viewer, attrs.width, attrs.height );
    
    viewer->key_handler = se_text_viewer_key_event;
    viewer->mouse_handler = se_text_viewer_mouse_event;
    viewer->configure_change_handler = se_text_viewer_configure_change_handler;
    
    se_text_viewer_draw_create( viewer );
    return viewer;
}

void se_text_viewer_size_update( se_text_viewer* viewer, int width, int height )
{
    assert( viewer );
    assert( width && height );
    
    viewer->physicalWidth = width;
    viewer->physicalHeight = height;

    const se_env* env = viewer->env;
    viewer->columns = width / env->glyphMaxWidth;
    viewer->rows = height / env->fontMaxHeight;

    se_debug( "new size in pixels (%d, %d), in chars (%d, %d)\n",
              width, height, viewer->columns, viewer->rows );    
    //TODO: check if cursor if out of display
}

void se_text_viewer_show( se_text_viewer* viewer )
{
    XMapWindow( viewer->env->display, viewer->view );
    
}

void se_text_viewer_update( se_text_viewer* viewer )
{
    se_debug ( "enter" );
    XFlush( viewer->env->display );
    
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
        se_text_viewer_size_update( viewer, attrs.width, attrs.height );
    }
}

void se_text_viewer_key_event(se_text_viewer* viewer, XEvent* ev )
{
    se_debug( "enter key event" );
    assert( viewer && ev );

    XKeyEvent kev = ev->xkey;
    KeySym key_sym = XKeycodeToKeysym( viewer->env->display, kev.keycode, 0 );
    se_debug( "keysym(0x%x): %s", (guint)key_sym, XKeysymToString(key_sym) );
    
    if ( kev.type == KeyPress ) {

        switch( key_sym ) {
        case XK_A...XK_Z:
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
        
            se_draw_text_utf8( viewer, &clr, XKeysymToString(key_sym) );
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

gboolean se_env_change_font( se_env* env, const char* name )
{
    XftPattern* favourate_pat = XftNameParse( name );
    XftResult result;
    XftPattern* matched_pat = XftFontMatch(
        env->display, env->screen, favourate_pat, &result );
    XftFont *font = XftFontOpenPattern( env->display, matched_pat );
    if ( !font ) {
        se_warn( "can not open font" );
        XftPatternDestroy( favourate_pat );
        XftPatternDestroy( matched_pat );
        return FALSE;
    }
    env->xftFont = font;

    XGlyphInfo metric;
    char buf[] = "lg";
    XftTextExtentsUtf8( env->display, env->xftFont, 
                        (const XftChar8*)buf, strlen(buf), &metric);
    env->fontMaxHeight = metric.height;

    char buf2[] = "WM";
    XftTextExtentsUtf8( env->display, env->xftFont, 
                        (const XftChar8*)buf2, strlen(buf2), &metric);
    env->glyphMaxWidth = metric.width/2;

    XftPatternDestroy( favourate_pat );
    XftPatternDestroy( matched_pat );

    return TRUE;
}

void se_draw_text_utf8( se_text_viewer* viewer, XftColor* color,
                        const char* utf8 )
{
    assert( viewer && viewer->xftDraw );

    const se_env* env = viewer->env;
    
    se_curosr cur = viewer->cursor;
    int utf8_len = strlen( utf8 );

//    int cols_need = ;
    XGlyphInfo metric;
    XftTextExtentsUtf8( env->display, env->xftFont, 
                        (const XftChar8*)utf8, utf8_len, &metric);
    se_debug( "metric: (x %d, y %d, w %d, h %d, xoff %d, yoff %d)",
              metric.x, metric.y, metric.width, metric.height,
              metric.xOff, metric.yOff );
    
    se_size incr = {
        .width = metric.xOff,
        .height = metric.yOff
    };    
    
    XftDrawStringUtf8( viewer->xftDraw, color, viewer->env->xftFont,
                       cur.x, cur.y,
                       (const XftChar8*)utf8, utf8_len );
    se_text_viewer_update_cursor( viewer, utf8_len, incr );
}

void se_text_viewer_update_cursor( se_text_viewer* viewer, int nchars, se_size incr )
{
    assert( viewer );
    viewer->cursor.x += incr.width;
    viewer->cursor.y += incr.height;
}

int main(int argc, char *argv[])
{
    se_env* env = se_env_init();
    se_text_viewer* viewer = se_text_viewer_create( env );
    assert( viewer );
    se_text_viewer_show( viewer );
    XFlush( env->display );

    int mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
        | KeyPressMask | KeyReleaseMask | SubstructureRedirectMask;
    
    XSelectInput( env->display, viewer->view, mask );
    
    while( !env->exitLoop ) {
        XEvent ev;
        XNextEvent( env->display, &ev );
        if ( ev.xany.window != viewer->view )
            continue;

        switch( ev.type ) {
        case Expose:
            if ( ev.xexpose.count > 0 )
                break;

            se_text_viewer_update( viewer );
            break;

        case KeyPress:
        case KeyRelease:
            viewer->key_handler( viewer, &ev );
            break;

        case ConfigureRequest:
        case ResizeRequest:
            viewer->configure_change_handler( viewer, &ev );
                
        default:
            break;
        }
    }

    se_debug( "exit loop" );
    se_env_release( env );
    return 0;
}

