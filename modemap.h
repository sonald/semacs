
/**
 * Mode Map Definition - 
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

#ifndef _semacs_modemap_h
#define _semacs_modemap_h

#include "util.h"

enum {
    SE_PREFIX_ARG,
    SE_EXIT_ARG,
};

DEF_CLS(se_command_args);
struct se_command_args
{
    int flags;
    int prefix_arg;
};

enum {
    ControlDown = 0x01,
    ShiftDown   = 0x02,
    MetaDown    = 0x04, // Meta or Alt
    SuperDown   = 0x08,  // Super or Hyper
    EscapeDown  = 0x10,
    Button1Down = 0x20,
    Button2Down = 0x40,
    Button3Down = 0x80,
    // double-click?
    // triple-click?

    /* SeKeyUp     = 0x1000,   */
    /* SeKeyDown   = 0x2000,     */
};

DEF_CLS(se_key);
struct se_key
{
    unsigned short  modifiers;
    unsigned short ascii; // > 0 if se_key is a isprint, else -1
};

extern se_key se_key_null_init();
extern char* se_key_to_string(se_key);
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

struct se_world;
typedef int (*se_key_command_t)(struct se_world* world, se_command_args* args, se_key key);


extern int se_self_insert_command(struct se_world*, se_command_args*, se_key);
extern int se_self_silent_command(struct se_world*, se_command_args*, se_key);

DEF_CLS(se_modemap);
struct se_modemap
{
    char mapName[64];
    GHashTable *keyToCmds;
};

extern se_modemap* se_modemap_full_create(const char*);
extern void se_modemap_free(se_modemap*);
extern se_key_command_t se_modemap_lookup_command(se_modemap*, se_key);
extern void se_modemap_dump(se_modemap*);

////////////////////////////////////////////////////////////////////////////////

// struct to cached all created modemap 
typedef GHashTable se_modemap_hash;



DEF_CLS(se_mode);
struct se_mode
{
    se_mode *next;
    
    char modeName[32];
    se_modemap* modemap;

    int (*bindMap)(se_mode*, se_modemap*);
    se_modemap* (*initMap)(se_mode*);
    int (*init)(se_mode*); // setup mode such as keybindings
};

extern se_mode* se_mode_create(const char* mode_name,
                               se_modemap* (*initMapProc)());
extern void se_mode_delete(const char* mode_name);

// globally defined mode as default
extern se_mode *fundamentalMode;
extern se_modemap *fundamentalModeMap;

#endif

