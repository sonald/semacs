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

#define ROUND_TO_BLOCK(size)  ((((size)>>8)<<8) + (1<<8))

/**
 * insert data start from start, and if neccesary, realloc chunk and return new
 * address.  caller should make sure that data contains no '\n' at all if lp is
 * already nl ended.
 */
static se_line* se_line_insert(se_line* lp, int start, const char* data, int len)
{
    g_assert( lp && lp->content );

    se_chunk *chunk = lp->content;
    /* se_debug("B: content(%d): [%s], data(%d): [%s]", chunk->used, chunk->data, */
    /*          len, data ); */
    gboolean nl_ended = chunk->fullLine;

    start = MIN(start, chunk->used-(nl_ended?1:0) );

    // sanity check: this is not strong enough
    char* nl_pos = strrchr(data, '\n');
    if ( nl_pos >= data + len )
        nl_pos = NULL;
    g_assert( nl_pos == NULL ||
              ((nl_pos == data + len - 1) && (start == chunk->used)) );
    
    int new_len = chunk->size;
    if ( chunk->used + len > chunk->size ) {
        int new_len = ROUND_TO_BLOCK( chunk->used + len );
        lp->content = g_realloc( lp->content, new_len );
        se_debug( "realloc %d to %d", chunk->size, new_len );
    } 

    chunk = lp->content;
    memmove( chunk->data+start+len, chunk->data+start, new_len-start-len );
    memcpy( chunk->data+start, data, len );
    chunk->used += len;
    chunk->size = new_len;
    if ( !nl_ended )
        chunk->fullLine = (nl_pos != NULL);
    
    /* se_debug("A: content(%d): [%s]", chunk->used, chunk->data ); */
    return lp;
}

static se_line* se_line_clear(se_line* lp)
{
    // lp should nl-ended, if not, it should be destroyed not cleared
    g_assert( lp->content->fullLine );
    lp->content->data[0] = '\n';
    lp->content->used = 1;        
    return lp;
}

static void se_line_remove_nl( se_line* lp )
{
    se_chunk* chunk = lp->content;
    if ( chunk->fullLine ) {
        g_assert( chunk->data[chunk->used-1] == '\n' );
        chunk->data[chunk->used-1] = 0;
        chunk->used--;
        chunk->fullLine = FALSE;
    } 
}

/**
 * just return original lp for sequencial call
 * note that one can not delete trialing '\n' in a line
 */
static se_line* se_line_delete(se_line* lp, int start, int len)
{
    g_assert( lp );
    int used = lp->content->used;
    
    if ( lp->content->fullLine )
        used--;    
    if ( len <= 0 || start < 0 || start >= used )
        return lp;
    
    len = MIN( len, used-start );
    
    if ( start == 0 && len >= used )
        return se_line_clear( lp );

    char *data = lp->content->data;
    used += (lp->content->fullLine?1:0);
    memmove( data+start, data+start+len, used - start - len );
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
    /* se_debug( "round size %d to %d", len, new_len ); */
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
 * if count is positive, move forward, else move backward
 */
static void se_buffer_update_point(se_buffer* bufp, int incr)
{
    se_debug( "B:incr:%d, point:%d, chars:%d, lines: %d,curLine: %d, col: %d", incr,
              bufp->position, bufp->charCount, bufp->lineCount, bufp->curLine, bufp->curColumn );
    g_assert( bufp && bufp->lines );
    
    bufp->position += incr;
    if ( bufp->position > bufp->charCount )
        bufp->position = bufp->charCount;
    bufp->position =  bufp->position? :0;
    
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
    se_debug( "A:incr:%d, point:%d, chars:%d, lines: %d,curLine: %d, col: %d", incr,
              bufp->position, bufp->charCount, bufp->lineCount, bufp->curLine, bufp->curColumn );
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
static gboolean se_buffer_bol(se_buffer* bufp)
{
    g_assert( bufp );
    return bufp->curColumn == 0;
}

static gboolean se_buffer_bob(se_buffer* bufp)
{
    g_assert( bufp );
    return ( bufp->position == 0 || bufp->charCount == 0 );
}

static gboolean se_buffer_eob(se_buffer* bufp)
{
    g_assert( bufp );
    return (bufp->position == bufp->charCount);
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

static void se_buffer_delete_line( se_buffer* bufp, se_line* lp )
{
    lp->next->previous = lp->previous;
    lp->previous->next = lp->next;
}

int se_buffer_init(se_buffer* bufp)
{
    
    return 0;
}

int se_buffer_release(se_buffer* bufp)
{
    
    return TRUE;
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
    if ( se_buffer_eob( bufp ) && se_buffer_bol( bufp ) ) {
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
    se_debug( "" );
    if ( count )
        se_buffer_update_point( bufp, count );
    return TRUE;
}

/**
 * when moving up/down, try best to keep column remaining
 */
int se_buffer_forwardLine(se_buffer* bufp, int nr_lines)
{
    se_debug( "forward %d lines", nr_lines );
    g_assert( bufp && bufp->lines );
    int orig_col = bufp->curColumn;

    if ( nr_lines > 0 && (nr_lines + bufp->curLine > bufp->lineCount) )
        nr_lines = bufp->lineCount - bufp->curLine;
    else if ( nr_lines < 0 && (bufp->curLine + nr_lines <= 0) )
        nr_lines = - MIN(bufp->curLine, bufp->lineCount);
    
    if ( !nr_lines || (nr_lines > 0 && bufp->curLine == bufp->lineCount)
         || (nr_lines < 0 && bufp->curLine == 0))
        return TRUE;

    se_line *lp = bufp->getCurrentLine( bufp );
    int count = 0;

    if ( nr_lines > 0 ) {
        if ( !lp ) {  // EOB
            return TRUE;
        }

        count = se_line_getLineLength(lp) - orig_col;
        for (int i = 0; i < nr_lines-1; ++i) {
            g_assert( lp->next == bufp->lines );
            lp = lp->next;
            count += se_line_getLineLength( lp );            
        }

        if ( lp->next != bufp->lines ) {
            lp = lp->next;
            gboolean nl_ended = lp->content->fullLine;
            count += MIN(orig_col, se_line_getLineLength(lp) - (nl_ended?1:0));
        }
        
    } else {
        count = lp? orig_col: 0;
        lp = lp? : bufp->lines;
        for (int i = nr_lines; i < -1; ++i) {
            lp = lp->previous;
            count += se_line_getLineLength( lp );            
        }

        lp = lp->previous;
        count += MAX(0, se_line_getLineLength(lp) - orig_col);
    }
    
    se_buffer_update_point( bufp, nr_lines>0?count:-count );
    return TRUE;    
}

static int se_buffer_beginingOfLine(se_buffer* bufp)
{
    g_assert( bufp );
    if ( !bufp->lines || se_buffer_bob(bufp) || se_buffer_bol(bufp) )
        return TRUE;

    int orig_col = bufp->curColumn;
    if ( orig_col == 0 )
        return TRUE;

    se_buffer_update_point( bufp, -orig_col );
    g_assert( bufp->curColumn == 0 );
    
    return TRUE;
}

static int se_buffer_endOfLine(se_buffer* bufp)
{
    if ( !bufp->lines || se_buffer_eob(bufp) || se_buffer_eol(bufp) )
        return TRUE;

    se_line *lp = bufp->getCurrentLine( bufp );
    gboolean nl_ended = lp->content->fullLine;
    int trail = se_line_getLineLength(lp) - bufp->curColumn - (nl_ended?1:0);
    se_buffer_update_point( bufp, trail );
    g_assert( bufp->curColumn == se_line_getLineLength(lp) - (nl_ended?1:0) );
    
    return TRUE;
}

static int se_buffer_bufferStart(se_buffer* bufp)
{
    return TRUE;
}

static int se_buffer_bufferEnd(se_buffer* bufp)
{
    return bufp->charCount;
}

static int se_buffer_createMark(se_buffer* bufp, const char* name, int flags)
{
    return 0;
}

static void se_buffer_deleteMark(se_buffer* bufp, const char* name)
{

}

static int se_buffer_markToPoint(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_pointToMark(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_getMark(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_setMark(se_buffer* bufp, const char* name, int pos)
{
    return 0;
}

static int se_buffer_pointAtMark(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_pointBeforeMark(se_buffer* bufp, const char* name)
{

    return 0;
}

static int se_buffer_pointAfterMark(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_swapPointAndMark(se_buffer* bufp, const char* name)
{
    return 0;
}

static int se_buffer_writeBack(se_buffer* bufp)
{
    return 0;
}

static void se_buffer_setFileName(se_buffer* bufp, const char* file_name)
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

int se_buffer_insertChar(se_buffer* bufp, int c)
{
    /* se_debug( "B:No.%d, point: %d, col: %d, lines: %d", bufp->curLine, bufp->position, */
    /*           bufp->curColumn, bufp->lineCount ); */
    bufp->modified = TRUE;
    char buf[2] = { c, 0 };
    
    if ( !bufp->lines ) {
        bufp->lines = se_line_alloc( buf, 1 );
        bufp->lines->next = bufp->lines;
        bufp->lines->previous = bufp->lines;
        bufp->lineCount++;
        bufp->charCount++;    
        se_buffer_update_point( bufp, 1 );
        return TRUE;
    }
    
    se_line *lp = bufp->getCurrentLine( bufp );
    int col = bufp->curColumn;
    if ( lp == NULL ) {
        se_line *lp_new = se_line_alloc( buf, 1 );
        se_buffer_insert_line_after( bufp, bufp->lines->previous, lp_new );
        bufp->lineCount++;
        
    } else if ( c == '\n' ) { // split cur line into 2 lines
        gboolean nl_ended = lp->content->fullLine;
        
        if ( se_buffer_bol(bufp) ) {        
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
    return TRUE;
}

/**
 * split str into lines by '\n' and construct se_lines to store each.
 * all of them are appended to the tail.
 * TODO: 
 *   auto split long lines into small ( auto wrap or what )
 */
int se_buffer_insertString(se_buffer* bufp, const char* str)
{
    const char* sp = str;
    int str_bytes = strlen( str );
    int nr_chars = g_utf8_strlen( str, -1 );
    se_debug( "str: %s, bytes: %d, chars: %d", str, str_bytes, nr_chars );
    
    do {
        const char* endp = strchr( sp, '\n' );
        if ( !endp && (sp - str >= str_bytes) )
            break;

        int buf_len = 0;
        gboolean nl_ended = TRUE;
        if ( endp ) {
            buf_len = ++endp - sp; // take '\n' into account
            
        } else if ( sp - str < str_bytes ) {
            // means this is last line and no trailing '\n'
            endp = str + str_bytes;
            buf_len = endp - sp;
            nl_ended = FALSE;
        }

        se_debug("process: endp - sp: %d, sp(%d): [%s], nl_ended: %d",
                 (endp - sp), buf_len, strndup(sp, buf_len), nl_ended );

        se_line *cur_lp = bufp->getCurrentLine( bufp );
        if ( !cur_lp ) {
            /* se_debug("1st switch"); */
            // eob and bol
            se_line *lp_new = se_line_alloc( sp, buf_len );
            if ( bufp->lines ) {
                cur_lp = bufp->lines->previous;
                se_buffer_insert_line_after( bufp, cur_lp, lp_new );
            } else {
                bufp->lines = lp_new;
                lp_new->next = lp_new;
                lp_new->previous = lp_new;
            }
            
            // a little fix, whatever if sp is nl-ended, we should increment lines
            if ( !nl_ended ) bufp->lineCount++;
            
        } else if ( nl_ended ) {
            /* se_debug("2nd switch"); */
            if ( se_buffer_bol(bufp) ) {
                se_line *lp_new = se_line_alloc( sp, buf_len );
                se_buffer_insert_line_after( bufp, cur_lp->previous, lp_new );
                if ( bufp->lines == cur_lp ) {
                    bufp->lines = lp_new;
                }

            } else {
                gboolean nl_ended2 = cur_lp->content->fullLine;
                int lp_len = se_line_getLineLength(cur_lp);
                
                if ( se_buffer_eol(bufp) && !nl_ended2 ) {
                    g_assert( bufp->curLine == bufp->lineCount - 1 );
                    se_line_insert( cur_lp, lp_len, sp, buf_len );
                    
                } else {
                    const char *orig = se_line_getData( cur_lp );
                    int col = bufp->curColumn;
                    
                    se_line *lp_new = se_line_alloc( orig+col, lp_len - col );
                    se_line_delete( cur_lp, col, lp_len );
                    se_line_insert( cur_lp, col, sp, buf_len );
                    se_buffer_insert_line_after( bufp, cur_lp, lp_new ); 
                }
            }

        } else {
            /* se_debug("3rd switch"); */
            int lp_len = se_line_getLineLength(cur_lp);
            se_line_insert( cur_lp, lp_len, sp, buf_len );
        }
        
        if ( nl_ended ) bufp->lineCount++;
        bufp->charCount += buf_len;
        se_buffer_update_point( bufp, buf_len );

        sp = endp;
        if ( sp - str >= str_bytes )
            break;
        
    } while (1);

    bufp->modified = TRUE;
    return TRUE;
}

static int se_buffer_replaceChar(se_buffer* bufp, int c)
{
    return TRUE;
}

static int se_buffer_replaceString(se_buffer* bufp, const char* str)
{
    return TRUE;
}

static int se_buffer_deleteChar( se_buffer* bufp )
{
    if ( se_buffer_eob(bufp) )
        return TRUE;

    bufp->modified = TRUE;
    se_line *cur_lp = bufp->getCurrentLine( bufp );
    g_assert( cur_lp );

    gboolean nl_ended = cur_lp->content->fullLine;
    int ln_size = se_line_getLineLength( cur_lp );
    int col = bufp->getCurrentColumn( bufp );
    
    if ( nl_ended ) {
        ln_size--; // remove '\n'
        g_assert( BETWEEN(col, 0, ln_size) );

        if ( ln_size == 0 ) {
            // delete empty line

            if ( bufp->lineCount == 1 ) {
                bufp->lines = NULL;
            } else if ( cur_lp == bufp->lines ) {
                bufp->lines = cur_lp->next;
            }

            se_buffer_delete_line( bufp, cur_lp );
            se_line_destroy( cur_lp );
            bufp->lineCount--;
            bufp->charCount--;
            return TRUE;
        }
        
        if ( col == ln_size ) {
            // merge with next line
            se_debug( "merge with next line" );
            if ( cur_lp->next == bufp->lines || bufp->lineCount == 1 ) {
                se_line_remove_nl( cur_lp );
                bufp->charCount--;
                se_debug("tail line, remove '\n' only");
                return TRUE;
            }
            
            se_line *to_merge = cur_lp->next;
            se_buffer_delete_line( bufp, to_merge );
            int merge_size = se_line_getLineLength( to_merge );
            /* if ( to_merge->content->fullLine ) */
            /*     merge_size--; */
            
            se_line_remove_nl( cur_lp );
            se_line_insert( cur_lp, ln_size, se_line_getData(to_merge), merge_size );
            bufp->lineCount--;
            bufp->charCount--;
            return TRUE;
        } 

        // else
        se_line_delete( cur_lp, col, 1 );
        bufp->charCount--;
        return TRUE;
    }

    // else !nl_ended, so this should be the last line
    g_assert( bufp->curLine == bufp->lineCount - 1 );
    if ( ln_size == 1 ) {
        se_buffer_delete_line( bufp, cur_lp );
        if ( bufp->lineCount == 1 ) {
            g_assert( bufp->lines == cur_lp );
            bufp->lines = NULL;
            se_debug("empty buffer");
        }            
        se_line_destroy( cur_lp );
        bufp->lineCount--;
    } else
        se_line_delete( cur_lp, col, 1 );
    
    bufp->charCount--;
    
    return TRUE;
}

static int se_buffer_deleteChars(se_buffer* bufp, int count)
{
    g_assert( bufp );
    if ( count <= 0 )
        return TRUE;

    for (int i = 0; i < count; ++i) {
        if ( !se_buffer_deleteChar( bufp ) )
            return FALSE;
    }
    return TRUE;
}

static int se_buffer_deleteRegion(se_buffer* bufp, const char* markName)
{
    return 0;
}

static int se_buffer_copyRegion(se_buffer* bufp, se_buffer* other, const char* markName)
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
    bufp->beginingOfLine = se_buffer_beginingOfLine;
    bufp->endOfLine = se_buffer_endOfLine;
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
