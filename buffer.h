
/**
 * Buffer API - 
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

#ifndef _semacs_buffer_h
#define _semacs_buffer_h

#include "util.h"
#include "modemap.h"

DEF_CLS(se_mark);

DEF_CLS(se_chunk);
DEF_CLS(se_line);
struct se_line
{
    se_line *next;
    se_line *previous;
    se_chunk *content;

    int version;  // used for redisplay
    se_mark *marks;  // marks on this line
};

extern const char* se_line_getData( se_line* );
extern int se_line_getLineLength( se_line* );

#define SE_MAX_NAME_SIZE  1023
#define SE_MAX_BUF_NAME_SIZE  ((SE_MAX_NAME_SIZE/4)-1)

struct se_world;

DEF_CLS(se_buffer);
struct se_buffer
{
    se_buffer *nextBuffer;

    char bufferName[SE_MAX_NAME_SIZE+1];
    char fileName[SE_MAX_BUF_NAME_SIZE+1];
    time_t fileTime;
    int modified;
    
    int position;  // logical offset of cursor
    int curLine;   // calculated from point
    int curColumn; // calculated from point

    int charCount;  // length of buffer in chars
    int lineCount;  // total lines
    
    se_line *lines;
    se_mark *marks;
    se_mode *modes;
    se_mode *majorMode;

    struct se_world *world;
    
    int (*init)(se_buffer*);
    int (*release)(se_buffer*);
    
    int (*setPoint)(se_buffer*, int);
    int (*getPoint)(se_buffer*);

    int (*isModified)(se_buffer*);
    
    int (*getChar)(se_buffer*);
    char* (*getStr)(se_buffer*, int max);
    const char* (*getBufferName)(se_buffer*);
    void (*setFileName)(se_buffer*, const char* file_name);

    int (*getCharCount)(se_buffer*);
    int (*getLineCount)(se_buffer*);    
    int (*getLine)(se_buffer*);
    se_line* (*getCurrentLine)(se_buffer*);
    int (*getCurrentColumn)(se_buffer*);    
    
    int (*forwardChar)(se_buffer*, int);
    int (*forwardLine)(se_buffer*, int);
    int (*beginingOfLine)(se_buffer*);
    int (*endOfLine)(se_buffer*);
    
    // affects by narrowing, right now don't consider that
    int (*bufferStart)(se_buffer*);
    int (*bufferEnd)(se_buffer*);

    int (*createMark)(se_buffer*, const char* name, int flags);
    void (*deleteMark)(se_buffer*, const char* name);
    // set mark at current point
    int (*markToPoint)(se_buffer*, const char* name);
    // set point to the specified mark
    int (*pointToMark)(se_buffer*, const char* name);
    // get mark's position
    int (*getMark)(se_buffer*, const char* name);
    // set existed mark to pos
    int (*setMark)(se_buffer*, const char* name, int pos);

    // predicate point's position relative to mark `name`
    int (*pointAtMark)(se_buffer*, const char* name);
    int (*pointBeforeMark)(se_buffer*, const char* name);
    int (*pointAfterMark)(se_buffer*, const char* name);

    int (*swapPointAndMark)(se_buffer*, const char* name);

    int (*writeBack)(se_buffer*); // write buffer into file
    int (*readFile)(se_buffer*); // clear and reread file into buffer
    int (*insertFile)(se_buffer*, const char* fileName);

    // isFront: if true, mode it insert at the front of mode list
    int (*appendMode)(se_buffer*, const char* mode,
                      int (*init_proc)(se_mode*), int isFront );
    int (*deleteMode)(se_buffer*, const char* mode);
    // call mode->init()
    int (*invokeMode)(se_buffer*, const char* mode);
    int (*setMajorMode)(se_buffer*, const char* mode);
    
    int (*insertChar)(se_buffer*, int c);
    int (*replaceChar)(se_buffer*, int c);
    int (*insertString)(se_buffer*, const char*);
    int (*replaceString)(se_buffer*, const char*);
    int (*deleteChars)(se_buffer*, int count);
    // delete region between point and mark
    int (*deleteRegion)(se_buffer*, const char* markName);
    int (*copyRegion)(se_buffer*, se_buffer* other, const char* markName);
};

extern se_buffer* se_buffer_create(struct se_world*, const char* buf_name);

#endif

