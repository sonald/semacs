/**
 * Buffer Impl - 
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

#ifndef _BSD_SOURCE
#define _BSD_SOURCE // for lstat
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib/gstdio.h>

struct se_chunk
{
    int size;
    int used;
    char data[0];
};


const char* se_line_getData( se_line* lp )
{
    assert( lp );
    return lp->content->data;
}

int se_line_getLineLength( se_line* lp )
{
    assert( lp );
    return lp->content->used;
}

#define ROUND_TO_BLOCK(size)  ((((size)>>8)<<8) + (1<<9))

/**
 * insert data start from start, and if neccesary, realloc chunk and return new
 * address
 */
static se_line* se_line_insert(se_line* lp, int start, char* data, int len)
{
    assert( lp && lp->content );
    se_chunk *chunk = lp->content;
    assert( start <= chunk->used );
    int new_len = chunk->size;
    
    if ( chunk->used + len > chunk->size ) {
        int new_len = ROUND_TO_BLOCK( chunk->used + len );
        lp->content = g_realloc( lp->content, new_len );
    } 

    chunk = lp->content;
    memmove( chunk->data+start, chunk->data+start+len, len );
    memcpy( chunk->data+start, data, len );
    chunk->used += len;
    chunk->size = new_len;
    
    return lp;
}

/**
 * just return original lp for sequencial call
 */
static se_line* se_line_delete(se_line* lp, int start, int len)
{
    
    return lp;
}

/**
 * copy data into se_line struct
 */
static se_line* se_line_alloc(char* data, int len)
{
    se_line *lp = g_malloc0( sizeof(se_line) );
    if ( !lp ) {
        se_error( "no enough memory" );
        return NULL;
    }
    
    int new_len = ROUND_TO_BLOCK( len );
    se_debug( "round size %d to %d", len, new_len );
    lp->content = g_malloc0( sizeof(se_chunk)+new_len );
    if ( !lp->content ) {
        se_error( "no enough memory" );
        g_free( lp );
        return NULL;
    }
    
    lp->content->size = new_len;
    lp->content->used = len;
    memcpy( lp->content->data, data, len );
    return lp;
}


struct se_mark
{
    se_mark *next;

    char markName[32];
    int position;
    int flags;  // persistent or not
};

struct se_mode
{
    se_mode *next;
    
    char modeName[64];
    int (*init)(se_mode*); // setup mode such as keybindings
};

int se_buffer_init(se_buffer* bufp)
{
    
    return 0;
}

int se_buffer_release(se_buffer* bufp)
{
    return 0;
}

int se_buffer_setPoint(se_buffer* bufp, int new_pos)
{
    return 0;
}

int se_buffer_getChar(se_buffer* bufp)
{
    return 0;
}

char* se_buffer_getStr(se_buffer* bufp, int max)
{
    return 0;
}

const char* se_buffer_getBufferName(se_buffer* bufp)
{
    assert( bufp );
    if ( bufp->bufferName[0] == 0 )
        return NULL;
    return bufp->bufferName;
}

int se_buffer_getCharCount(se_buffer* bufp)
{
    assert( bufp );
    return bufp->charCount;
}

int se_buffer_getLineCount(se_buffer* bufp)
{
    assert( bufp );
    return bufp->lineCount;
}

int se_buffer_getPoint(se_buffer* bufp)
{
    assert( bufp );
    return bufp->position;
}

int se_buffer_getLine(se_buffer* bufp)
{
    assert( bufp );
    return bufp->curLine;
}

se_line* se_buffer_getCurrentLine(se_buffer* bufp)
{
    assert( bufp );
    se_line *lp = bufp->lines;
    int nr = bufp->curLine;
    
    while ( --nr ) lp = lp->next;
    return lp;
}

int se_buffer_forwardChar(se_buffer* bufp, int count)
{
    return 0;
}

int se_buffer_forwardLine(se_buffer* bufp, int count)
{
    return 0;
}

int se_buffer_bufferStart(se_buffer* bufp)
{
    return 0;
}

int se_buffer_bufferEnd(se_buffer* bufp)
{
    return bufp->charCount;
}

int se_buffer_createMark(se_buffer* bufp, const char* name, int flags)
{
    return 0;
}

void se_buffer_deleteMark(se_buffer* bufp, const char* name)
{

}

int se_buffer_markToPoint(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_pointToMark(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_getMark(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_setMark(se_buffer* bufp, const char* name, int pos)
{
    return 0;
}

int se_buffer_pointAtMark(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_pointBeforeMark(se_buffer* bufp, const char* name)
{

    return 0;
}

int se_buffer_pointAfterMark(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_swapPointAndMark(se_buffer* bufp, const char* name)
{
    return 0;
}

int se_buffer_writeBack(se_buffer* bufp)
{
    return 0;
}

void se_buffer_setFileName(se_buffer* bufp, const char* file_name)
{
    //TODO: check
    if ( bufp->fileName[0] ) {
        // clean up
    }

    size_t siz = strlen(file_name);
    strncpy( bufp->fileName, file_name, MIN(siz, SE_MAX_NAME_SIZE) );
}

/**
 * TODO:
 *   check file type
 *   handle Emacs modes
 */
int se_buffer_readFile(se_buffer* bufp)
{
    assert( bufp );
    
    if ( bufp->fileName[0] == 0 ) {
        se_warn( "no file is associated with buffer yet" );
        return FALSE;
    }

    char * canon_name = realpath( bufp->fileName, NULL );
    if ( !canon_name ) {
        // create new buffer
        se_warn( "%s does not exists, create new buffer", bufp->fileName );
        return FALSE;
    }
    se_debug( "canon_name: %s", canon_name );
    
    struct stat statbuf;
    if ( lstat(bufp->fileName, &statbuf) < 0 ) {
        se_error( "lstat failed: %s", strerror(errno) );
        return FALSE;
    }

    //FIXME: check size in a more elegant way
    if ( statbuf.st_size > (1<<28) ) {
        se_debug( "file is too large, so read a big block each time" );
        //TODO: 
    }
    
    GError *error = NULL;
    size_t str_len = 0;
    char *str = NULL;
    if ( g_file_get_contents(canon_name, &str, &str_len, &error) == FALSE ) {
        se_error( "read file failed: %s", error->message );
        g_error_free( error );
        return FALSE;
    }
    free( canon_name );
    
    g_clear_error( &error );
    g_assert( str != NULL && str_len >= 0 );
    
    char *sp = str, *buf = str;
    size_t buf_len = 0;
    int line_incr = 0, char_incr = 0;

    while ( buf - str < str_len  ) {
        sp = strchr( buf, '\n' );
        se_line *lp = NULL;
        
        if ( !sp ) {
            buf_len = strlen( buf );
            lp = se_line_alloc( buf, buf_len );
            char_incr += buf_len;
            
        } else {
            buf_len = sp - buf;
            lp = se_line_alloc( buf, buf_len );
            ++line_incr;
            char_incr += buf_len + 1; // plus 1 is for invisible '\n'
        }
        
        if ( bufp->lines ) {
            se_line *lp1 = bufp->lines->previous;
            lp1->next = lp;
            lp->next = bufp->lines;
            lp->previous = lp1;
            bufp->lines->previous = lp;
            
        } else {
            bufp->lines = lp;
            lp->next = lp;
            lp->previous = lp;
        }

        if ( !sp )
            break;
        
        buf = sp+1;
    }

    bufp->lineCount = line_incr;
    bufp->curLine = line_incr;
    bufp->charCount = char_incr;

    bufp->position = 0;
    bufp->modified = TRUE;

    se_debug( "read file: lines %d, chars: %d", bufp->lineCount, bufp->charCount );
    
    g_free( str );
    return TRUE;
}

int se_buffer_insertFile(se_buffer* bufp, const char* fileName)
{
    return 0;
}

int se_buffer_appendMode(se_buffer* bufp, const char* mode, int se_buffer_init_proc(se_mode*), int isFront )
{
    return 0;
}

int se_buffer_deleteMode(se_buffer* bufp, const char* mode)
{
    return 0;
}

int se_buffer_invokeMode(se_buffer* bufp, const char* mode)
{
    return 0;
}

void se_buffer_insertChar(se_buffer* bufp, int c)
{

}

/**
 * split str into lines by '\n' and construct se_lines to store each.
 * all of them are appended to the tail.
 * TODO: 
 *   auto split long lines into small ( auto wrap or what )
 */
/* FIXME: insert is acutally happens right after pointer, so this could involve
 * cutting cur line into two sep lines.
 */
void se_buffer_insertString(se_buffer* bufp, char* str)
{
    char *sp = str, *buf = str;
    size_t buf_len = 0;
    int line_incr = 0, char_incr = 0;

    while ( *buf ) {
        sp = strchr( buf, '\n' );
        se_line *lp = NULL;
        
        if ( !sp ) {
            buf_len = strlen( buf );
            lp = se_line_alloc( buf, buf_len );
            char_incr += buf_len;
            
        } else {
            buf_len = sp - buf;
            lp = se_line_alloc( buf, buf_len );
            ++line_incr;
            char_incr += buf_len + 1; // plus 1 is for invisible '\n'
        }
        
        if ( bufp->lines ) {
            se_line *lp1 = bufp->lines->previous;
            lp1->next = lp;
            lp->next = bufp->lines;
            lp->previous = lp1;
            bufp->lines->previous = lp;
            
        } else {
            bufp->lines = lp;
            lp->next = lp;
            lp->previous = lp;
        }

        if ( !sp )
            break;
        
        buf = sp+1;
    }

    bufp->lineCount += line_incr;
    bufp->curLine += line_incr;
    bufp->position += char_incr;
    bufp->charCount += char_incr;

    bufp->modified = TRUE;
}

void se_buffer_replaceChar(se_buffer* bufp, int c)
{

}

void se_buffer_replaceString(se_buffer* bufp, const char* str)
{

}

int se_buffer_deleteChars(se_buffer* bufp, int count)
{
    return 0;
}

int se_buffer_deleteRegion(se_buffer* bufp, const char* markName)
{
    return 0;
}

int se_buffer_copyRegion(se_buffer* bufp, se_buffer* other, const char* markName)
{
    return 0;
}




se_buffer* se_buffer_create(const char* buf_name)
{
    se_buffer *bufp = g_malloc0( sizeof(se_buffer) );
    
    bufp->init = se_buffer_init;
    bufp->release = se_buffer_release;
    bufp->setPoint = se_buffer_setPoint;
    bufp->getChar = se_buffer_getChar;
    bufp->getStr = se_buffer_getStr;
    bufp->getBufferName = se_buffer_getBufferName;
    bufp->getCharCount = se_buffer_getCharCount;
    bufp->getLineCount = se_buffer_getLineCount;
    bufp->getPoint = se_buffer_getPoint;
    bufp->getLine = se_buffer_getLine;
    bufp->getCurrentLine = se_buffer_getCurrentLine;
    
    bufp->forwardChar = se_buffer_forwardChar;
    bufp->forwardLine = se_buffer_forwardLine;
    bufp->bufferStart = se_buffer_bufferStart;
    bufp->bufferEnd = se_buffer_bufferEnd;
    bufp->createMark = se_buffer_createMark;
    bufp->deleteMark = se_buffer_deleteMark;
    bufp->markToPoint = se_buffer_markToPoint;
    bufp->pointToMark = se_buffer_pointToMark;
    bufp->getMark = se_buffer_getMark;
    bufp->setMark = se_buffer_setMark;
    bufp->pointAtMark = se_buffer_pointAtMark;
    bufp->pointBeforeMark = se_buffer_pointBeforeMark;
    bufp->pointAfterMark = se_buffer_pointAfterMark;
    bufp->swapPointAndMark = se_buffer_swapPointAndMark;
    
    bufp->writeBack = se_buffer_writeBack;
    bufp->readFile = se_buffer_readFile;
    bufp->insertFile = se_buffer_insertFile;
    bufp->setFileName = se_buffer_setFileName;
    
    bufp->appendMode = se_buffer_appendMode;
    bufp->deleteMode = se_buffer_deleteMode;
    bufp->invokeMode = se_buffer_invokeMode;

    bufp->insertChar = se_buffer_insertChar;
    bufp->insertString = se_buffer_insertString;
    bufp->replaceChar = se_buffer_replaceChar;
    bufp->replaceString = se_buffer_replaceString;
    bufp->deleteChars = se_buffer_deleteChars;
    bufp->deleteRegion = se_buffer_deleteRegion;
    bufp->copyRegion = se_buffer_copyRegion;

    size_t siz = strlen(buf_name);
    se_debug( "MIN: %d", MIN(siz, SE_MAX_BUF_NAME_SIZE) );
    strncpy( bufp->bufferName, buf_name, MIN(siz, SE_MAX_BUF_NAME_SIZE) );
    
    bufp->init( bufp );
    
    return bufp;
}
