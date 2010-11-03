
/**
 * Editor Abstraction - 
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

#ifndef _semacs_editor_h
#define _semacs_editor_h

#include "util.h"
#include "modemap.h"
#include "buffer.h"

DEF_CLS(se_world);
struct se_world
{
    se_buffer *bufferList;
    se_buffer *current;

    // mode map name <-> mode map obj
    se_modemap_hash *modemap_hash;
    
    int (*init)(se_world*);
    int (*finish)(se_world*);

    int (*saveFile)(se_world*, const char*);
    int (*loadFile)(se_world*, const char*);

    int (*bufferCreate)(se_world*, const char* buf_name);
    int (*bufferClear)(se_world*, const char* buf_name);
    int (*bufferDelete)(se_world*, const char* buf_name);
    int (*bufferSetCurrent)(se_world*, const char* buf_name);
    char* (*bufferSetNext)(se_world*); // return next bufffer's name
    int (*bufferChangeName)(se_world*, const char* buf_name);
    char* (*bufferGetName)(se_world*);

    se_mode* (*getMajorMode)(se_world*, const char* mode_name);

    void (*registerModemap)(se_world*, se_modemap*);
    
    int (*dispatchCommand)(se_world*, se_command_args*, se_key);
};

extern se_world* se_world_create();
extern void se_world_free(se_world*);

#endif



