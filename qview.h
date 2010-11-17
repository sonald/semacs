
/**
 * View Based On Qt  - 
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

#ifndef _semacs_qview_h
#define _semacs_qview_h

#include <Qt/QtGui>
#include "view.h"
#include "editor.h"

class SEView: public QWidget
{
    Q_OBJECT
public:
    SEView();

protected:    
    bool x11Event( XEvent *event );
    void paintEvent( QPaintEvent * event );
    void keyPressEvent( QKeyEvent * event );
	void keyReleaseEvent( QKeyEvent * event );
    void resizeEvent( QResizeEvent * event );
    void inputMethodEvent( QInputMethodEvent * event );
    
private:
    int _columns; // viewable width in cols
    int _rows; // viewable height in rows
    
    int _physicalWidth;  // real width of view
    int _physicalHeight; // real height of view

    int _glyphMaxHeight;
    int _glyphMaxWidth;
    int _glyphAscent;
    
    gchar *_content;  // which contains columns * rows chars, may utf8 next version
    se_cursor _cursor; // where cursor is, pos in logical (row, col),
                      // this is not point of editor
    XEvent _cachedEvent;
    se_world *_world;

    enum {
        SE_IM_NORMAL,
        SE_IM_COMPOSING,
        SE_IM_COMMIT,
    };
    int _composingState;
    QString _composedText;
    
    se_key se_delayed_wait_key();
    void updateViewContent(); // update data in view area
    void updateSize();
    void drawTextUtf8( QColor, se_cursor, const char*, int utf8_len );
    void drawBufferPoint();
    
};

extern int start_qview();

#endif

