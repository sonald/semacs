
/**
 * Editor Impl - 
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

#include "buffer.h"
#include "editor.h"

#include <libgen.h>

static int se_world_init(se_world* world)
{
    assert( world );
    assert( world->bufferList == NULL && world->current == NULL );

    /* const char init_str[] = "this is by default created buffer just for demo"; */
    /* if ( world->bufferCreate( world, "*scratch*" ) == TRUE ) { */
    /*     assert( world->bufferList && world->current ); */
    /*     world->current->insertString( world->current, init_str ); */
    /* } */

    if ( world->bufferCreate(world, "*scratch*") == TRUE ) {
        assert( world->bufferList && world->current );
        world->current->setFileName( world->current, "readme.txt" );
        world->current->readFile( world->current );
    }
    return TRUE;
}

static int se_world_finish(se_world* world)
{
    if ( world->bufferList ) {
        while ( world->current ) {
            se_buffer* bufp = world->current;
            if ( world->bufferDelete(world,  bufp->getBufferName(bufp)) == FALSE ) {
                se_error( "delete buffer %s failed", bufp->getBufferName(bufp) );
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

static int se_world_saveFile(se_world* world, const char* file_name)
{
    return 0;
    
}

static int se_world_loadFile(se_world* world, const char* file_name)
{
    //TODO: check if file_name has been loaded by a buffer
    size_t siz = strlen( file_name );
    char buf[siz+1];
    strncpy( buf, file_name, siz );
    char *buf_name = basename( buf );
    if ( strlen(buf_name) >= SE_MAX_BUF_NAME_SIZE ) {
        buf_name[SE_MAX_BUF_NAME_SIZE] = '\0';
    }
    
    if ( world->bufferCreate( world, buf_name ) == FALSE ) {
        se_error( "failed to load file" );
        return FALSE;
    }

    se_buffer* bufp = world->current;
    assert( bufp );
    bufp->setFileName( bufp, file_name );
    return TRUE;
}

static int se_world_bufferCreate(se_world* world, const char* buf_name)
{
    se_buffer *bufp = se_buffer_create( buf_name );
    if ( !world->bufferList ) {
        world->bufferList = bufp;
    } else {
        bufp->nextBuffer = world->bufferList;
        world->bufferList = bufp;
    }
    
    world->bufferSetCurrent( world, buf_name );
    return TRUE;    
}

static int se_world_bufferClear(se_world* world, const char* buf_name)
{
    return 0;
}

static int se_world_bufferDelete(se_world* world, const char* buf_name)
{
    return 0;
}

static int se_world_bufferSetCurrent(se_world* world, const char* buf_name)
{
    se_buffer *bufp = world->bufferList;
    while ( bufp ) {
        if ( strcmp(buf_name, bufp->getBufferName(bufp)) == 0 ) {
            world->current = bufp;
            se_debug( "set current as %s", buf_name );
            //TODO: set modes etc
            return TRUE;
        }
    }

    se_debug( "can not find buf named [%s]", buf_name );
    return FALSE;
}

static char* se_world_bufferSetNext(se_world* world)
{
    return 0;    
}

static int se_world_bufferChangeName(se_world* world, const char* buf_name)
{
    return 0;
}

static char* se_world_bufferGetName(se_world* world)
{
    return 0;
}

se_world* se_world_create()
{
    se_world* world = g_malloc0( sizeof(se_world) );

    world->init = se_world_init;
    world->finish = se_world_finish;
    world->saveFile = se_world_saveFile;
    world->loadFile = se_world_loadFile;
    world->bufferCreate = se_world_bufferCreate;
    world->bufferClear = se_world_bufferClear;
    world->bufferDelete = se_world_bufferDelete;
    world->bufferSetCurrent = se_world_bufferSetCurrent;
    world->bufferSetNext = se_world_bufferSetNext;
    world->bufferChangeName = se_world_bufferChangeName;
    world->bufferGetName = se_world_bufferGetName;

    world->init( world );
    
    return world;
}

void se_world_free(se_world* world)
{
    assert( world );
    world->finish( world );
    free( world );
}
