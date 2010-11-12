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

const char gFundamentalModeName[] = "Fundamental";
const char gFundamentalModeMapName[] = "fundamental-mode-map";

static se_modemap* se_world_init_fundamentalModeMap()
{
    g_assert( !fundamentalModeMap );
    fundamentalModeMap = se_modemap_full_create( gFundamentalModeMapName );
    // do sanity check
    se_modemap_dump( fundamentalModeMap );
    return fundamentalModeMap;
}

static int se_world_create_fundamentalMode(se_world* world)
{
    g_assert( !fundamentalMode );
    fundamentalMode = se_mode_create( gFundamentalModeName,
                                      se_world_init_fundamentalModeMap );
    world->registerMode( world, fundamentalMode );
    return TRUE;
}

void se_world_registerMode(se_world* world, se_mode* mode)
{
    g_assert( world && mode );
    if ( !world->mode_hash ) {
        world->mode_hash = g_hash_table_new( g_str_hash, g_str_equal );
    }
    g_hash_table_insert( world->mode_hash,
                         strdup(gFundamentalModeName), fundamentalMode );
}

static int se_world_init(se_world* world)
{
    assert( world );
    assert( world->bufferList == NULL && world->current == NULL );
    se_world_create_fundamentalMode( world );
    g_assert( fundamentalMode );

    //FIXME: create two buffers
    const char init_str[] = "# this is by default created buffer just for demo\n"
        "you can test some Emacs keybindings here";
    if ( world->bufferCreate( world, "*scratch*" ) == TRUE ) {
        assert( world->bufferList && world->current );
        world->current->insertString( world->current, init_str );
        world->current->setMajorMode( world->current, gFundamentalModeName );
    }

    world->loadFile( world, "readme.txt" );
    return TRUE;
}

static int se_world_finish(se_world* world)
{
    g_assert( world );
    if ( world->bufferList ) {
        while ( world->current ) {
            se_buffer* bufp = world->current;
            if ( world->bufferDelete(world,  bufp->getBufferName(bufp)) == FALSE ) {
                se_error( "delete buffer %s failed", bufp->getBufferName(bufp) );
                return FALSE;
            }
        }
        
        world->bufferList = NULL;
    }
    
    return TRUE;
}

static int se_world_quit(se_world* world)
{
    world->finish( world );
    exit(0);
    return TRUE;
}

se_mode* se_world_getMajorMode(se_world* world, const char* mode_name)
{
    g_assert( world && mode_name );
    return (se_mode*) g_hash_table_lookup( world->mode_hash, mode_name );
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
    memcpy( buf, file_name, siz+1 );
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
    bufp->setMajorMode( world->current, gFundamentalModeName );
    return bufp->readFile( bufp );
}

static int se_world_bufferCreate(se_world* world, const char* buf_name)
{
    se_buffer *bufp = se_buffer_create( world, buf_name );
    if ( !world->bufferList ) {
        world->bufferList = bufp;
        bufp->nextBuffer = NULL;
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
    se_buffer *bufp = world->bufferList;
    while ( bufp ) {
        if ( strcmp(bufp->getBufferName(bufp), buf_name) == 0 ) {
            if ( bufp == world->current )
                world->current = bufp->nextBuffer;

            if ( bufp == world->bufferList )
                world->bufferList = bufp->nextBuffer;
            
            bufp->release( bufp );
            g_free( bufp );
            return TRUE;
        }

        bufp = bufp->nextBuffer;
    }
    
    return FALSE;
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

static const char* se_world_bufferSetPrevious(se_world* world)
{
    g_assert( world );
    if ( world->bufferList == NULL )
        return NULL;
    
    se_buffer *bufp = world->bufferList;
    while ( bufp->nextBuffer ) {
        if ( bufp->nextBuffer == world->current ){
            world->current = bufp;
            return bufp->getBufferName( bufp );
        }
        bufp = bufp->nextBuffer;
    }
    if ( world->current == world->bufferList ) {
        world->current = bufp;
    }
    return world->current->getBufferName( world->current );
}

static const char* se_world_bufferSetNext(se_world* world)
{
    g_assert( world );
    if ( world->bufferList == NULL )
        return NULL;
    
    if ( world->current->nextBuffer == NULL ) { // loop back to first
        world->current = world->bufferList;
    } else
        world->current = world->current->nextBuffer;
    
    return world->current->getBufferName( world->current );
}

static int se_world_bufferChangeName(se_world* world, const char* buf_name)
{
    return 0;
}

static char* se_world_bufferGetName(se_world* world)
{
    return 0;
}


static int se_world_dispatchCommand(se_world* world, se_command_args* args, se_key key)
{
    se_buffer *bufp = world->current;
    se_modemap *map = bufp->majorMode->modemap;
    g_assert( map );
    se_key_seq keyseq = se_key_seq_null_init();
    
    if ( args->flags & SE_IM_ARG ) {
        se_debug( "SE_IM_ARG set " );
        return se_self_insert_command( world, args, key );
    }

    if ( args->flags & SE_UNIVERSAL_ARG ) {
        se_debug( "SE_UNIVERSAL_ARG set");
        if ( BETWEEN(key.ascii, '0', '9') ) {
            g_string_append_printf(args->universalArg, "%c", key.ascii);
            return FALSE;
        }
        // else loop cmd according to universalArg
    }

    if ( args->flags & SE_PREFIX_ARG )
        keyseq = args->keyseq;
    keyseq.keys[keyseq.len++] = key;

    se_debug( "search keybinding for %s", se_key_seq_to_string(keyseq) );
    se_key_command_t cmd = se_modemap_lookup_command_ex( map, keyseq );
    if ( !cmd ) {
        se_msg( "key seq [%s] is not binded", se_key_seq_to_string(keyseq) );
        return TRUE;
    }

    int nr_execution = 1;
    if ( args->flags & SE_UNIVERSAL_ARG )
        nr_execution = strtol( args->universalArg->str, NULL, 10 );
    //TODO: if nr_execution == 0, it should mean something according to Emacs C-u
    nr_execution = nr_execution?:4;
    se_debug("execute cmd %d times", nr_execution );

    gboolean ret = TRUE;    
    for (int i = 0; i < nr_execution; ++i) {
        if ( (ret = cmd( bufp->world, args, key)) == FALSE )
            break;
    }
    return ret;
}

se_world* se_world_create()
{
    se_world* world = g_malloc0( sizeof(se_world) );

    world->init = se_world_init;
    world->finish = se_world_finish;
    world->quit = se_world_quit;
    
    world->saveFile = se_world_saveFile;
    world->loadFile = se_world_loadFile;
    world->bufferCreate = se_world_bufferCreate;
    world->bufferClear = se_world_bufferClear;
    world->bufferDelete = se_world_bufferDelete;
    world->bufferSetCurrent = se_world_bufferSetCurrent;
    world->bufferSetNext = se_world_bufferSetNext;
    world->bufferSetPrevious = se_world_bufferSetPrevious;
    
    world->bufferChangeName = se_world_bufferChangeName;
    world->bufferGetName = se_world_bufferGetName;

    world->getMajorMode = se_world_getMajorMode;
    world->registerMode = se_world_registerMode;
    
    world->dispatchCommand = se_world_dispatchCommand;
    
    world->init( world );
    
    return world;
}

void se_world_free(se_world* world)
{
    assert( world );
    world->finish( world );
    free( world );
}
