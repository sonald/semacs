
/**
 * BaseView - 
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

#ifndef _semacs_view_h
#define _semacs_view_h

#include "util.h"
#include "editor.h"


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

enum {
    SE_XVIEW,
    SE_QVIEW
};

DEF_CLS(se_text_viewer); // viewer of text editor
struct se_text_viewer
{
    se_world *world;
    int viewType; // determine type of private member
};

#ifdef __cplusplus
extern "C" {
#endif

void setup_language();
const char* XEventTypeString(int type);
    
#ifdef __cplusplus
}
#endif

#endif


