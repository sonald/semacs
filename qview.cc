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

    _content = (gchar*)g_malloc0( SE_MAX_COLUMNS * SE_MAX_ROWS );
    _cmdArgs = se_command_args_init();
    
    _composingState = SE_IM_NORMAL;
    setAttribute( Qt::WA_InputMethodEnabled );
}

bool SEView::x11Event( XEvent *event )
{
    _cachedEvent = *event;
    bool flip = false;
    int orig_state = _composingState;
    
    if ( event->type == KeyPress ) {
        orig_state = _composingState;
        qDebug() << "XEvent: " << XEventTypeString(event->type);        
    }
    
    bool ret = QWidget::x11Event( event );

    if ( event->type == KeyPress ) {
        if ( orig_state != _composingState && _composingState == SE_IM_COMMIT ) {
            flip = true;
            se_debug("flip");
        }
    }
    
    return ret;
}

void SEView::inputMethodEvent( QInputMethodEvent *event ) 
{
    qDebug() << "preedit: " << event->preeditString()
             << ", commit: " << event->commitString()
             << ", replace len: " << event->replacementLength()
             << ", replace start: " << event->replacementStart();
    if ( event->preeditString().isEmpty() ) {
        if ( event->commitString().isEmpty() ) {
            if ( _composingState == SE_IM_COMMIT )
                dispatchCommand( se_key_null_init() );
            _composingState = SE_IM_NORMAL;
        } else
            _composingState = SE_IM_COMMIT;
    } else {
        g_assert ( event->commitString().isEmpty() );
        _composingState = SE_IM_COMPOSING;
    }
    
    switch( _composingState ) {
    case SE_IM_NORMAL: break;
    case SE_IM_COMPOSING: break;
    case SE_IM_COMMIT:
        if ( !event->commitString().isEmpty() ) {
            _cmdArgs.flags |= SE_IM_ARG;
            g_string_assign( _cmdArgs.composedStr,
                             event->commitString().toUtf8().constData() );
            qDebug() << "commit im";
        }
        break;        
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

void SEView::dispatchCommand( se_key sekey )
{
    if ( _world->dispatchCommand( _world, &_cmdArgs, sekey ) ) {
        qDebug() << "cmd finished";
        se_command_args_clear( &_cmdArgs );
        _composingState = SE_IM_NORMAL;
        
        if ( _world->current->isModified(_world->current) ) {
            se_debug("pending update");
            _world->current->modified = FALSE;
            updateViewContent();
            return;
        }
    }

    updateViewContent();
    // not complete key seq, continue ...

}

void SEView::keyPressEvent( QKeyEvent * event )
{
    qDebug() << ("KeyPress: ") << event->text();
    g_assert( _cachedEvent.type == KeyPress );
    
    XKeyEvent kev = _cachedEvent.xkey;
    se_key sekey = se_key_null_init();
        
    sekey = se_key_event_process( QX11Info::display(), kev );
    //qDebug() << se_key_to_string( sekey );
    if ( se_key_is_null(sekey) ) {
        return;
    }
    
    if ( sekey.ascii == XK_Escape )
        qApp->quit();

    dispatchCommand( sekey );
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

    update();
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
    updateViewContent();
}

void SEView::drawBufferPoint( QPainter *p )
{
    se_buffer *cur_buf = _world->current;
    g_assert( cur_buf );

    se_cursor point_cur = {
        cur_buf->getCurrentColumn( cur_buf ),
        cur_buf->getLine( cur_buf ),
    };
    
    se_position point_pos = (se_position){ point_cur.column * _glyphMaxWidth,
                                           point_cur.row * _glyphMaxHeight };
    point_pos.y -= (_glyphMaxHeight - _glyphAscent)/2;

    QColor clr( 0xff, 0x00, 0x00, 128 );
        
    p->setPen( clr );
    p->fillRect( point_pos.x, point_pos.y, 2, _glyphMaxHeight, clr );

}

void SEView::drawTextUtf8( QPainter *p, se_cursor cur, const char* utf8, int utf8_len )
{
    se_position cur_pos = (se_position){ cur.column * _glyphMaxWidth,
                                         cur.row * _glyphMaxHeight };
    se_position start_pos = {
        cur_pos.x,
        cur_pos.y + _glyphAscent,
    };

    QColor clr( 0, 0, 0, 0xff );    
    p->setPen( clr );
    p->drawText( start_pos.x, start_pos.y, QString::fromUtf8(utf8) );
}

void SEView::paintEvent( QPaintEvent * event )
{
    QWidget::paintEvent( event );
    
    se_buffer *cur_buf = _world->current;
    g_assert( cur_buf );

    QPainter p;
    p.begin( this );
    
    int nr_lines = cur_buf->getLineCount( cur_buf );
    se_debug( "repaint buf %s [%d, %d]", cur_buf->getBufferName(cur_buf),
              _columns, MIN(_rows, nr_lines) );    

    _cursor = (se_cursor){ 0, 0 };
    for (int r = 0; r < MIN(_rows, nr_lines); ++r) {
        char *data = _content + r*SE_MAX_ROWS;
        int data_len = strlen( data );
        drawTextUtf8( &p, _cursor, data, MIN(data_len, _columns) );
        _cursor = (se_cursor){0, _cursor.row + 1};
    }

    drawBufferPoint( &p );
    p.end();
    
}

int start_qview()
{
    
    return true;
}
