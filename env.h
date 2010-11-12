
/**
 * Environment - 
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

#ifndef _semacs_env_h
#define _semacs_env_h

#include "util.h"
#include "editor.h"

DEF_CLS(se_font);
struct se_font
{
    XftFont *xftFont;
    int glyphMaxHeight;
    int glyphMaxWidth;
    int glyphAscent;
};

DEF_CLS(se_env);  // environment 
struct se_env 
{
    Display *display;
    int numScreens;
    int screen;  // which screen the editor is showed
    Colormap colormap;
    Visual *visual;
    Window root;

    se_font monoFont;
    se_font sansFont;
    se_font serifFont;
    
    XftFont *xftFont;
    int glyphMaxHeight;
    int glyphMaxWidth;
    int glyphAscent;

    XIM xim;
    se_world *world;
    
    gboolean exitLoop;

};

extern se_env* se_env_init();
extern void se_env_release(se_env* env);
extern void se_env_quit(se_env* env);
extern gboolean se_env_change_font( se_env* env, const char* name );

#endif

