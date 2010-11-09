
/**
 * Keys - 
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

//TODO: I should auto convert XKeysyms into my key defs
//this can help me use other than X ( like in console ) facilities to get key
//events

#ifndef _semacs_key_h
#define _semacs_key_h

#include "util.h"

enum {
    ControlDown = 0x01,
    ShiftDown   = 0x02,
    MetaDown    = 0x04, // Meta or Alt
    SuperDown   = 0x08,  // Super or Hyper
//    EscapeDown  = 0x10,
    Button1Down = 0x20,
    Button2Down = 0x40,
    Button3Down = 0x80,
    // double-click?
    // triple-click?
};

DEF_CLS(se_key);
struct se_key
{
    unsigned short  modifiers;
    unsigned short ascii; // > 0 if se_key is a isprint, else -1
};

extern se_key se_key_null_init();
extern char* se_key_to_string(se_key);
extern se_key se_key_from_string( const char* rep );
extern se_key se_key_from_keysym( KeySym keysym );
extern se_key se_keycode_to_sekey(KeyCode);
extern int se_key_is_control(se_key);
extern int se_key_is_null(se_key);
extern int se_key_is_only(se_key, unsigned short modifier);
// this will compare both ascii and modifiers
extern int se_key_is_equal(se_key key1, se_key key2);

static inline int se_key_to_int(se_key sekey)
{
    int ikey;
    memmove( &ikey, &sekey, sizeof sekey );
    return ikey;
}
    
static inline se_key int_to_se_key(int ikey)
{
    se_key sekey;
    memmove( &sekey, &ikey, sizeof sekey );
    return sekey;
}

// FIXME: remove this!
#define SE_MAX_KEY_SEQ_LEN  10

DEF_CLS(se_key_seq);
struct se_key_seq
{
    int len;
    se_key keys[SE_MAX_KEY_SEQ_LEN];
};

/**
 * string rep of key seq define as below:
 * if isprint(key): as it is
 * Control + isprint(key): C-`as it is`
 * Shift + non_print(key): S-
 * Meta/Alt + : M-
 * e.g.  b, C-M-a, S-B1, C-a M-d, C-c S-B2
 */
extern se_key_seq se_key_seq_null_init();
extern se_key_seq se_key_seq_from_string( const char* );
/**
 * convenient constuctors 
 */
extern se_key_seq se_key_seq_from_keysym1( KeySym );
extern se_key_seq se_key_seq_from_keysym2( KeySym keysyms[2] );
extern se_key_seq se_key_seq_from_keysyms( int nr_keysym, ... );

extern char* se_key_seq_to_string( se_key_seq );
extern int se_key_seq_is_equal( se_key_seq, se_key_seq );


#endif

