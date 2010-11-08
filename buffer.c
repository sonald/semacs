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
#include "editor.h"

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
    gboolean fullLine; // line ended with '\n'
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
 * address.  caller should make sure that data contains no '\n' at all if lp is
 * already nl ended.
 */
static se_line* se_line_insert(se_line* lp, int start, char* data, int len)
{
    g_assert( lp && lp->content );

    se_chunk *chunk = lp->content;
    se_debug("B: content(%d): [%s]", chunk->used, chunk->data );
    gboolean nl_ended = chunk->fullLine;

    start = MIN(start, chunk->used-(nl_ended?1:0) );

    // sanity check
    char* nl_pos = strchr(data, '\n');
    g_assert( nl_pos == NULL ||
              ( !nl_ended && (nl_pos == data + len - 1) && start == chunk->used) );
    
    int new_len = chunk->size;
    if ( chunk->used + len > chunk->size ) {
        int new_len = ROUND_TO_BLOCK( chunk->used + len );
        lp->content = g_realloc( lp->content, new_len );
        se_debug( "realloc %d to %d", chunk->size, new_len );
    } 

    chunk = lp->content;
    memmove( chunk->data+start, chunk->data+start+len, len );
    memcpy( chunk->data+start, data, len );
    chunk->used += len;
    chunk->size = new_len;
    if ( !nl_ended )
        chunk->fullLine = (nl_pos != NULL);
    
    se_debug("A: content(%d): [%s]", chunk->used, chunk->data );
    return lp;
}


static se_line* se_line_clear(se_line* lp)
{
    se_debug( "clear line" );
    if ( lp->content->fullLine == TRUE ) {
        lp->content->data[0] = '\n';
        lp->content->used = 1;        
    } else
        lp->content->used = 0;
    return lp;
}

/**
 * just return original lp for sequencial call
 * note that one can not delete trialing '\n' in a line
 */
SE_UNUSED static se_line* se_line_delete(se_line* lp, int start, int len)
{
    g_assert( lp );
    int used = lp->content->used;
    if ( lp->content->fullLine == TRUE )
        used--;
    
    if ( len <= 0 || start < 0 || start >= used )
        return lp;
    
    len = MIN( len, used-start );
    
    if ( start == 0 && len >= used )
        return se_line_clear( lp );

    char *data = lp->content->data;
    memmove( data+start+len, data+start, used - start - len );
    lp->content->used -= len;
    return lp;
}

SE_UNUSED static void se_line_destroy(se_line* lp)
{
    g_assert( lp );
    g_free( lp->content );
    g_free( lp );
}

/**
 * copy data into se_line struct
 */
static se_line* se_line_alloc(const char* data, int len)
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
    lp->content->fullLine = (data[len-1] == '\n');
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


////////////////////////////////////////////////////////////////////////////////

/**
 * use this is sync with optional members( curLine )
 * 
 */
/* static void se_buffer_update_point(se_buffer* bufp, int incr) */
/* { */
/*     se_debug( "B:incr:%d, point:%d, curLine: %d, col: %d", incr, */
/*               bufp->position, bufp->curLine, bufp->curColumn ); */
/*     g_assert( bufp && bufp->lines ); */
/*     bufp->position += incr; */

/*     if ( bufp->position == bufp->charCount ) { */
/*         se_line *last_line = bufp->lines->previous; */
/*         bufp->curLine = bufp->lineCount-1; */
/*         bufp->curColumn = se_line_getLineLength( last_line ); */
/*         if ( last_line->content->fullLine ) { */
/*             bufp->curLine++; */
/*             bufp->curColumn = 0; */
/*         } */
/*         se_debug( "A:point:%d, curLine: %d, col: %d", bufp->position, bufp->curLine, */
/*                   bufp->curColumn ); */
/*         return; */
/*     } */
    
/*     se_line *lp = bufp->lines; */
/*     int total = 0, n = -1; */

/*     do { */
/*         if ( lp->content->fullLine ) */
/*             n++; */
/*         total += se_line_getLineLength(lp); */
/*         if ( bufp->position < total ) { */
/*             total -= se_line_getLineLength(lp);             */
/*             break; */
/*         } else if ( bufp->position == total ) { */
/*             if ( lp->content->fullLine ) { */
/*                 n++; */
/*             } else */
/*                 total -= se_line_getLineLength(lp);             */
/*             break; */
/*         } */
        
/*         lp = lp->next; */
/*     } while (1); */

/*     bufp->curLine = n; */
/*     bufp->curColumn = bufp->position - total; */
    
/*     se_debug( "A:point:%d, curLine: %d, col: %d", bufp->position, bufp->curLine, */
/*               bufp->curColumn ); */
/* } */

#define BETWEEN(mem, min, max)  ((mem >= (min)) && (mem <= (max)))
#define BETWEEN_EX(mem, min, max)  ((mem > (min)) && (mem < (max)))

static void se_buffer_update_point(se_buffer* bufp, int incr)
{
    se_debug( "B:incr:%d, point:%d, lines: %d,curLine: %d, col: %d", incr,
              bufp->position, bufp->lineCount, bufp->curLine, bufp->curColumn );
    g_assert( bufp && bufp->lines );
    bufp->position += incr;

    se_line *lp = bufp->lines;
    int total = 0, n = 0;
    while (1) {
        int pad = se_line_getLineLength(lp) - (lp->content->fullLine?1:0);
        if ( BETWEEN(bufp->position, total, total + pad) ) {
            bufp->curLine = n;
            bufp->curColumn = bufp->position - total;
            break;
        } 
        // else
        total += pad;
        if ( lp->content->fullLine ) {
            n++;
            total++;
        }
        lp = lp->next;
    }
    se_debug( "A:incr:%d, point:%d, lines: %d,curLine: %d, col: %d", incr,
              bufp->position, bufp->lineCount, bufp->curLine, bufp->curColumn );
}

/**
 * check if at End-Of-Line, based on point
 */
static gboolean se_buffer_eol(se_buffer* bufp)
{
    g_assert( bufp );
    se_line *lp = bufp->getCurrentLine( bufp );
    if ( !lp )
        return TRUE;
    
    gboolean nl_ended = lp->content->fullLine;
    return bufp->curColumn == (lp->content->used - (nl_ended?1:0));
}

// Beginning-Of-Line
SE_UNUSED static gboolean se_buffer_bol(se_buffer* bufp)
{
    g_assert( bufp );
    se_line *lp = bufp->getCurrentLine( bufp );
    if ( !lp )
        return TRUE;

    return bufp->curColumn == 0;
}

/**
 * insert lp after current line
 */
static void se_buffer_insert_line_after( se_buffer* bufp, se_line *pos, se_line* lp )
{
    lp->next = pos->next;
    pos->next->previous = lp;
    pos->next = lp;
    lp->previous = pos;
}

/* SE_UNUSED static void se_buffer_delete_line( se_buffer* bufp, se_line* lp ) */
/* { */
/*     if ( lp->next == lp ) { // this is only line, we must keep at one line exists. */
/*                                // clear content only */
/*         g_assert( bufp->lineCount == 1 ); */
/*         bufp->lines = NULL; */
/*         bufp->charCount = 0; */
/*         bufp->lineCount = 0; */

/*         // special case: set point to zero directly */
/*         bufp->position = 0; */
/*         bufp->curLine = 0; */
/*         bufp->curColumn = 0; */
/*         se_line_destroy( lp ); */
/*         return; */
/*     } */

/*     lp->next->previous = lp->previous;         */
/*     lp->previous->next = lp->next; */
/*     se_line_destroy( lp ); */
/* } */

int se_buffer_init(se_buffer* bufp)
{
    
    return 0;
}

int se_buffer_release(se_buffer* bufp)
{
    return 0;
}

int se_buffer_isModified(se_buffer* bufp)
{
    return bufp->modified;
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

static inline int se_buffer_getLineCount(se_buffer* bufp)
{
    assert( bufp );
    return bufp->lineCount;
}

static inline int se_buffer_getPoint(se_buffer* bufp)
{
    assert( bufp );
    return bufp->position;
}

static inline int se_buffer_getLine(se_buffer* bufp)
{
    assert( bufp );
    return bufp->curLine;
}

static inline int se_buffer_getCurrentColumn(se_buffer* bufp)
{
    g_assert( bufp );
    return bufp->curColumn;
}

static se_line* se_buffer_getCurrentLine(se_buffer* bufp)
{
    g_assert( bufp );

    if ( !bufp->lines )
        return NULL;
    se_debug( "point: %d, charCount: %d", bufp->position, bufp->charCount );
        
    se_line *lp = bufp->lines;
    if ( bufp->position == bufp->charCount && bufp->curColumn == 0 ) {
        // end of buffer
        se_debug( "EOB" );
        return NULL;
    }
    
    int nr = bufp->curLine;
    while ( nr-- ) lp = lp->next;
    se_debug("No.%d(%d): [%s]", bufp->curLine, lp->content->used, lp->content->data);
    
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
    if ( lstat(canon_name, &statbuf) < 0 ) {
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

    //se_buffer_clear_content( bufp );
    if ( str_len == 0 ) {
        return TRUE;
    }

    int line_incr = 0, char_incr = 0;
    char* sp = str;
    do {
        char* endp = strchr( sp, '\n' );
        if ( !endp && (sp - str >= str_len) )
            break;

        int buf_len = 0;
        if ( endp ) {
            buf_len = ++endp - sp; // take '\n' into account
            
        } else if ( sp - str < str_len ) {
            // means this is last line and no trailing '\n'
            buf_len = sp - str;
            endp = str + 1;
        }

        se_line *lp = se_line_alloc( sp, buf_len );
        
        sp = endp;        
        char_incr += buf_len;
        ++line_incr;
        
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

        if ( sp - str >= str_len )
            break;
        
    } while (1);
    
    bufp->lineCount = line_incr;
    bufp->charCount = char_incr;

    se_buffer_update_point( bufp, char_incr );
    bufp->modified = TRUE;

    se_debug( "read file: lines %d, chars: %d", bufp->lineCount, bufp->charCount );
    return TRUE;
}

int se_buffer_insertFile(se_buffer* bufp, const char* fileName)
{
    assert( bufp && fileName );
    
    char * canon_name = realpath( fileName, NULL );
    if ( !canon_name ) {
        se_warn( "%s does not exists", fileName );
        return FALSE;
    }
    se_debug( "canon_name: %s", canon_name );
    
    struct stat statbuf;
    if ( lstat(canon_name, &statbuf) < 0 ) {
        se_error( "lstat failed: %s", strerror(errno) );
        return FALSE;
    }

    //FIXME: check size in a more elegant way
    if ( statbuf.st_size > (1<<28) ) {
        se_debug( "file is too large, so read a big block each time" );
        //TODO: 
    }
    
    GError *error = NULL;
    size_t data_len = 0;
    char *data = NULL;
    if ( g_file_get_contents(canon_name, &data, &data_len, &error) == FALSE ) {
        se_error( "read file failed: %s", error->message );
        g_error_free( error );
        return FALSE;
    }
    free( canon_name );
    
    g_clear_error( &error );
    g_assert( data != NULL && data_len >= 0 );

    gchar** lines = g_strsplit_set( data, "\n", -1 );
    while ( *lines ) {

    }
    g_strfreev( lines );
    
    return TRUE;
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

int se_buffer_setMajorMode(se_buffer* bufp, const char* mode)
{
    g_assert( bufp && mode );
    bufp->majorMode = bufp->world->getMajorMode(bufp->world, mode);
    g_assert( bufp->majorMode );
    se_debug( "set major mode: %s", mode );
    return TRUE;
}

void se_buffer_insertChar(se_buffer* bufp, int c)
{
    /* se_debug( "B:No.%d, point: %d, col: %d, lines: %d", bufp->curLine, bufp->position, */
    /*           bufp->curColumn, bufp->lineCount ); */
    bufp->modified = TRUE;
    
    se_line *lp = bufp->getCurrentLine( bufp );
    int col = bufp->curColumn;

    char buf[2] = { c, 0 };
    if ( lp == NULL ) {
        se_line *lp_new = se_line_alloc( buf, 1 );
        se_buffer_insert_line_after( bufp, bufp->lines->previous, lp_new );
        bufp->lineCount++;
        
    } else if ( c == '\n' ) { // split cur line into 2 lines
        gboolean nl_ended = lp->content->fullLine;
        
        if ( col == 0 ) {
            se_line *lp_new = se_line_alloc( buf, 1 );
            if ( bufp->curLine == 0 ) {
                bufp->lines = lp_new;
            }
            se_buffer_insert_line_after( bufp, lp->previous, lp_new );
            
        } else if ( se_buffer_eol(bufp) ) {
            if ( nl_ended ) {
                se_line *lp_new = se_line_alloc( buf, 1 );
                se_buffer_insert_line_after( bufp, lp, lp_new );
            } else {
                se_line_insert( lp, col, buf, 1 );
                bufp->lineCount--; // small fix: here no new line added acutally
            }
            
        } else {
            const char *orig = se_line_getData(lp);
            se_line *lp_new = se_line_alloc( orig+col, se_line_getLineLength(lp)-col );
            se_line_delete( lp, col, se_line_getLineLength(lp) );
            se_buffer_insert_line_after( bufp, lp, lp_new );
        }

        bufp->lineCount++;
        
    } else {
        se_line_insert( lp, col, buf, 1 );
    }

    bufp->charCount++;    
    se_buffer_update_point( bufp, 1 );

    /* se_debug( "A:No.%d, point: %d, col: %d, lines: %d", bufp->curLine, bufp->position, */
    /*           bufp->curColumn, bufp->lineCount ); */
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
    bufp->charCount += char_incr;    

    se_buffer_update_point( bufp, char_incr );

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

se_buffer* se_buffer_create(se_world* world, const char* buf_name)
{
    se_buffer *bufp = g_malloc0( sizeof(se_buffer) );
    bufp->world = world;
    
    bufp->init = se_buffer_init;
    bufp->release = se_buffer_release;

    bufp->isModified = se_buffer_isModified;
    bufp->setPoint = se_buffer_setPoint;
    bufp->getChar = se_buffer_getChar;
    bufp->getStr = se_buffer_getStr;
    bufp->getBufferName = se_buffer_getBufferName;
    bufp->getCharCount = se_buffer_getCharCount;
    bufp->getLineCount = se_buffer_getLineCount;
    bufp->getPoint = se_buffer_getPoint;
    bufp->getLine = se_buffer_getLine;
    bufp->getCurrentLine = se_buffer_getCurrentLine;
    bufp->getCurrentColumn = se_buffer_getCurrentColumn;
    
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
    bufp->setMajorMode = se_buffer_setMajorMode;
    
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
