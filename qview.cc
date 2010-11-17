/**
 * Qt Based View - 
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


#include "qview.h"
#include "editor.h"

#include <QX11Info>

SEView::SEView()
{
    _world = se_world_create();
    setFont( QFont( "Monaco", 11 ) ) ;
    QFontMetrics fm = fontMetrics();
    _glyphMaxWidth = fm.width("W");
    _glyphMaxHeight = fm.height();
    _glyphAscent = fm.ascent();
    
    qDebug() << "get font: " << font().toString();
    updateSize();
    _content = (gchar*)g_malloc0( SE_MAX_COLUMNS * SE_MAX_ROWS );

    _composingState = SE_IM_COMMIT;
    setAttribute( Qt::WA_InputMethodEnabled );
    update();
}

bool SEView::x11Event( XEvent *event )
{
    _cachedEvent = *event;
    return QWidget::x11Event( event );
}

void SEView::inputMethodEvent( QInputMethodEvent *event ) 
{
    qDebug() << "preedit: " << event->preeditString()
             << ", commit: " << event->commitString()
             << ", replace len: " << event->replacementLength()
             << ", replace start: " << event->replacementStart();
    if ( event->preeditString().isEmpty() ) {
        if ( event->commitString().isEmpty() )
            _composingState = SE_IM_NORMAL;
        else
            _composingState = SE_IM_COMMIT;
    } else {
        g_assert ( event->commitString().isEmpty() );
        _composingState = SE_IM_COMPOSING;
    }
    
    switch( _composingState ) {
    case SE_IM_NORMAL: break;
    case SE_IM_COMPOSING: break;
    case SE_IM_COMMIT: _composedText = event->commitString(); break;
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

se_key SEView::se_delayed_wait_key()
{
    se_debug( "waiting for a key..." );
    XEvent ev;
    while ( XCheckWindowEvent( QX11Info::display(), this->winId(),
                               KeyPressMask, &ev ) == False ) {
        usleep( 100 );
    }

    se_debug( "wait for a key done" );
    return se_key_event_process( QX11Info::display(), ev.xkey );
}

void SEView::keyPressEvent( QKeyEvent * event )
{
    se_debug("");
    
    g_assert( _cachedEvent.type == KeyPress );
    
    XKeyEvent kev = _cachedEvent.xkey;
    se_key sekey = se_key_null_init();
        
    sekey = se_key_event_process( QX11Info::display(), kev );
    if ( se_key_is_null(sekey) )
        return;
    
    if ( sekey.ascii == XK_Escape )
        qApp->quit();

    se_command_args args;
    bzero( &args, sizeof args );
    args.composedStr = g_string_new("");
    args.universalArg = g_string_new("");
        
    // if ( composed_input ) {
    //     args.flags |= SE_IM_ARG;
    //     g_string_assign( args.composedStr, chars->str );
    //     g_string_free( chars, TRUE );
    // }
        
    while ( _world->dispatchCommand( _world, &args, sekey ) != TRUE ) {
        sekey = se_delayed_wait_key();
        if ( sekey.ascii == XK_Escape )
            qApp->quit();
    }

    g_string_free( args.composedStr, TRUE );
    g_string_free( args.universalArg, TRUE );
        
    if ( _world->current->isModified(_world->current) ) {
        updateViewContent();
        update();
        
    }

}

void SEView::keyReleaseEvent( QKeyEvent * event )
{

}

void SEView::updateViewContent()
{
    se_buffer *cur_buf = _world->current;
    g_assert( cur_buf );

    //FIXME: this is slow algo just as a demo, change it later
    se_line* lp = cur_buf->lines;
    int nr_lines = cur_buf->getLineCount( cur_buf );
    se_debug( "paint rect: rows: %d", nr_lines );
    
    for (int r = 0; r < qMin(_rows, nr_lines); ++r) {
        const char* buf = se_line_getData( lp );
        int cols = se_line_getLineLength( lp );
        if ( buf[cols-1] == '\n' ) cols--;
        memcpy( (_content + r*SE_MAX_ROWS), buf, cols );
        *(_content + r*SE_MAX_ROWS+cols) = '\0';
        /* se_debug( "draw No.%d: [%s]", r, buf ); */
        lp = lp->next;
    }
}

void SEView::resizeEvent( QResizeEvent * event )
{
    updateSize();
}

void SEView::updateSize()
{
    _physicalWidth = size().width();
    _physicalHeight = size().height();

    _columns = _physicalWidth / _glyphMaxWidth;
    _rows = _physicalHeight / _glyphMaxHeight;

    se_debug( "new size in pixels (%d, %d), in chars (%d, %d)",
              _physicalWidth, _physicalHeight, _columns, _rows );
}

// void SEView::drawBufferPoint()
// {
//     se_buffer *cur_buf = _world->current;
//     g_assert( cur_buf );

//     se_cursor point_cur = {
//         .row = cur_buf->getLine( cur_buf ),
//         .column = cur_buf->getCurrentColumn( cur_buf )
//     };
//     se_position point_pos = se_text_cursor_to_physical( viewer, point_cur );
//     point_pos.y -= (env->glyphMaxHeight - env->glyphAscent)/2;
    
//     XRenderColor renderClr = {
//         .red = 0x00,
//         .green = 0xffff,
//         .blue = 0x00,
//         .alpha = 0x00
//     };
        
//     XftColor clr;
//     XftColorAllocValue( env->display, env->visual, env->colormap, &renderClr, &clr );
    
//     XftDrawRect( viewer->xftDraw, &clr, point_pos.x, point_pos.y, 2, env->glyphMaxHeight );

//     XftColorFree( env->display, env->visual, env->colormap, &clr );
// }

void SEView::drawTextUtf8( QColor color,
                        se_cursor cur, const char* utf8, int utf8_len )
{
    se_position cur_pos = (se_position){ cur.column * _glyphMaxWidth,
                                         cur.row * _glyphMaxHeight };
    se_position start_pos = {
        cur_pos.x,
        cur_pos.y + _glyphAscent,
    };

    QPainter p( this );
    p.setPen( color );
    p.drawText( start_pos.x, start_pos.y, QString::fromUtf8(utf8) );
}

void SEView::paintEvent( QPaintEvent * event )
{
    QWidget::paintEvent( event );
    
    se_buffer *cur_buf = _world->current;
    g_assert( cur_buf );

    QColor clr( 0x00, 0xffff, 0x00, 128 );
    
    int nr_lines = cur_buf->getLineCount( cur_buf );
    se_debug( "repaint buf %s [%d, %d]", cur_buf->getBufferName(cur_buf),
              _columns, MIN(_rows, nr_lines) );    

    // // this clear the whole window ( slow )
    // XClearArea( env->display, viewer->view, 0, 0, 0, 0, False );
    
    _cursor = (se_cursor){ 0, 0 };
    for (int r = 0; r < MIN(_rows, nr_lines); ++r) {
        char *data = _content + r*SE_MAX_ROWS;
        int data_len = strlen( data );
        drawTextUtf8( clr, _cursor, data, MIN(data_len, _columns) );
        _cursor = (se_cursor){0, _cursor.row + 1};
    }

    // se_draw_buffer_point( viewer );
}

int start_qview()
{
    
    return true;
}
