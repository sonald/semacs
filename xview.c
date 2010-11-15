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
    /* se_debug( "utf8: %s, utf8_len: %d", utf8, utf8_len ); */
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
        *(viewer->content + r*SE_MAX_ROWS+cols) = '\0';
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
        .alpha = 0xffff
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

    se_key sekey = se_key_null_init();
    char *key_str = XKeysymToString( key_sym );

    // FIXME: this is a simplified version, since KeySym actually follow latin-1
    // except for some terminal control keys
    if ( g_str_has_prefix(key_str, "Alt")
         || g_str_has_prefix(key_str, "Meta")
         || g_str_has_prefix(key_str, "Super")
         || g_str_has_prefix(key_str, "Control")
         || g_str_has_prefix(key_str, "Shift") ) {
        return sekey;
    }
    sekey.ascii = key_sym;
    
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
    
    se_debug( "sekey(%s): %s", key_str, se_key_to_string(sekey) );
    return sekey;
}

static se_key se_delayed_wait_key(se_text_viewer* viewer)
{
    se_debug( "waiting for a key..." );    
    XEvent ev;
    while ( XCheckWindowEvent( viewer->env->display, viewer->view,
                               KeyPressMask, &ev ) == False ) {
        usleep( 100 );
    }

    /* g_assert( ev.type == KeyPress ); */
    se_debug( "wait for a key done" );
    return se_key_event_process( viewer->env->display, ev.xkey );
}

// return TRUE if input is a composed str from IM, and returned from arg str
static gboolean se_text_viewer_lookup_string(se_text_viewer* viewer, XKeyEvent* kev,
                                             GString *str)
{
    char buf[64] = "";
    int buf_len = sizeof buf - 1;
    KeySym keysym;
    Status status;
    Xutf8LookupString( viewer->xic, kev, buf, buf_len, &keysym, &status );
    gboolean composed_input = FALSE;
    switch (status) {
    case XBufferOverflow:
        se_warn("XBufferOverflow");
        break;
        
    case XLookupNone:
        se_debug("XLookupNone");        
        break;
                
    case XLookupKeySym:
        if (status == XLookupKeySym) {
            se_debug( "XLookupKeySym: sym: %s", XKeysymToString(keysym) );
        }
        break;
        
    case XLookupBoth:
        se_debug( "XLookupBoth: sym: %s", XKeysymToString(keysym) );
        se_debug( "XLookupBoth: buf: [%s]", buf );
        if ((keysym == XK_Delete) || (keysym == XK_BackSpace)) {
            se_debug("delete");
            break;
        }
        break;
                
    case XLookupChars:
        se_debug( "XLookupChars: buf: [%s]", buf );
        g_string_printf( str, "%s", buf );
        composed_input = TRUE;
        break;
    }

    return composed_input;
}

void se_text_viewer_key_event(se_text_viewer* viewer, XEvent* ev )
{
    assert( viewer && ev );

    XKeyEvent kev = ev->xkey;
    if ( kev.type == KeyPress ) {
        se_key sekey = se_key_null_init();
        
        GString *chars = g_string_new("");
        gboolean composed_input = se_text_viewer_lookup_string(viewer, &kev, chars );
        //FIXME: right now, do not handle Unicode before I finish basic facilities
        composed_input = FALSE; 
        if ( !composed_input ) {
            sekey = se_key_event_process( viewer->env->display, kev );
            if ( se_key_is_null(sekey) )
                return;
    
            if ( sekey.ascii == XK_Escape )
                se_env_quit( viewer->env );
        }
        
        se_world *world = viewer->env->world;
        se_command_args args;
        bzero( &args, sizeof args );
        args.composedStr = g_string_new("");
        args.universalArg = g_string_new("");
        
        if ( composed_input ) {
            args.flags |= SE_IM_ARG;
            g_string_assign( args.composedStr, chars->str );
            g_string_free( chars, TRUE );
        }
        
        while ( world->dispatchCommand( world, &args, sekey ) != TRUE ) {
            sekey = se_delayed_wait_key( viewer );
            if ( sekey.ascii == XK_Escape )
                se_env_quit( viewer->env );
        }

        g_string_free( args.composedStr, TRUE );
        g_string_free( args.universalArg, TRUE );
        
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


/*
 * This function chooses the "more desirable" of two input styles.  The
 * style with the more complicated Preedit style is returned, and if the
 * styles have the same Preedit styles, then the style with the more
 * complicated Status style is returned.  There is no "official" way to
 * order interaction styles; this one seems reasonable, though.
 * This is a long procedure for a simple heuristic.
 */
static XIMStyle ChooseBetterStyle(XIMStyle style1, XIMStyle style2)
{
    XIMStyle s,t;
    XIMStyle preedit = XIMPreeditArea | XIMPreeditCallbacks |
        XIMPreeditPosition | XIMPreeditNothing | XIMPreeditNone;
    XIMStyle status = XIMStatusArea | XIMStatusCallbacks |
        XIMStatusNothing | XIMStatusNone;
    if (style1 == 0) {
        se_debug("use style2");
        return style2;
    }
    
    if (style2 == 0) {
        se_debug("use style1");
        return style1;
    }
    
    if ((style1 & (preedit | status)) == (style2 & (preedit | status)))
        return style1;
    s = style1 & preedit;
    t = style2 & preedit;
    if (s != t) {
        if (s | t | XIMPreeditCallbacks)
            return (s == XIMPreeditCallbacks)?style1:style2;
        else if (s | t | XIMPreeditPosition)
            return (s == XIMPreeditPosition)?style1:style2;
        else if (s | t | XIMPreeditArea)
            return (s == XIMPreeditArea)?style1:style2;
        else if (s | t | XIMPreeditNothing)
            return (s == XIMPreeditNothing)?style1:style2;
    }
    else { /* if preedit flags are the same, compare status flags */
        s = style1 & status;
        t = style2 & status;
        if (s | t | XIMStatusCallbacks)
            return (s == XIMStatusCallbacks)?style1:style2;
        else if (s | t | XIMStatusArea)
            return (s == XIMStatusArea)?style1:style2;
        else if (s | t | XIMStatusNothing)
            return (s == XIMStatusNothing)?style1:style2;
    }

    return 0;
}

static void dump_style( XIMStyle best_style )
{
    GString *style_str = g_string_new("");
    if ( best_style & XIMPreeditCallbacks)
        g_string_append_printf( style_str, ",%s", "XIMPreeditCallbacks");
    if ( best_style & XIMPreeditPosition )
        g_string_append_printf( style_str, ",%s", "XIMPreeditPosition");
    if ( best_style & XIMPreeditArea     )
        g_string_append_printf( style_str, ",%s", "XIMPreeditArea");
    if ( best_style & XIMPreeditNothing  )
        g_string_append_printf( style_str, ",%s", "XIMPreeditNothing");
    if ( best_style & XIMPreeditNone     )
        g_string_append_printf( style_str, ",%s", "XIMPreeditNone");
    if ( best_style & XIMStatusCallbacks )
        g_string_append_printf( style_str, ",%s", "XIMStatusCallbacks");
    if ( best_style & XIMStatusArea      )
        g_string_append_printf( style_str, ",%s", "XIMStatusArea");
    if ( best_style & XIMStatusNothing   )
        g_string_append_printf( style_str, ",%s", "XIMStatusNothing");
    if ( best_style & XIMStatusNone      )
        g_string_append_printf( style_str, ",%s", "XIMStatusNone");
    se_debug("best_style: %s", style_str->str );
    g_string_free( style_str, True );
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

    XIMStyles *im_supported_styles;
    XIMStyle app_supported_styles;
    XIMStyle style;
    XIMStyle best_style;

    XGetIMValues( env->xim, XNQueryInputStyle, &im_supported_styles, NULL );
    app_supported_styles = XIMPreeditNone | XIMPreeditNothing | XIMPreeditArea
        | XIMStatusNone | XIMStatusNothing | XIMStatusArea;
    best_style = 0;
    se_debug( "styles count: %d", im_supported_styles->count_styles );
    for(int i=0; i < im_supported_styles->count_styles; i++) {
        style = im_supported_styles->supported_styles[i];
        dump_style( style );
        if ((style & app_supported_styles) == style) /* if we can handle it */
            best_style = ChooseBetterStyle(style, best_style);
    }

    /* if we couldn't support any of them, print an error and exit */
    if (best_style == 0) {
        se_error("commonly supported interaction style.");
        exit(1);
    }
    XFree(im_supported_styles);
    dump_style( best_style );
    
    viewer->xic = XCreateIC(env->xim, XNInputStyle, best_style,
                            XNClientWindow, viewer->view,
                            NULL);
    if ( viewer->xic == NULL ) {
        se_debug(" create xic failed" );
        exit(1);        
    }
    se_debug( "XLocaleOfIM: %s", XLocaleOfIM( env->xim ) );
    
    return viewer;
}
