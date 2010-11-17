
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
#include "key.h"
#include "cmd.h"


#ifdef __cplusplus
extern "C" {
#endif

DEF_CLS(se_modemap_data);
DEF_CLS(se_modemap);
struct se_modemap
{
    char mapName[64];
    /**
     * key map to se_modemap_data, it's either a cmd, or a 2nd disptach table
     */
    GNode* root; // root store no key or cmd, the summary info
};

extern se_modemap* se_modemap_simple_create(const char*);
extern se_modemap* se_modemap_full_create(const char*);
extern void se_modemap_free(se_modemap*);
extern void se_modemap_dump(se_modemap*);

/**
 * this two functions will also return cmd which is a 2nd dispatch cmd but not
 * a binding cmd, meaning key is the prefix of a binding, and awaits for the
 * coming keys to complete.
 */
extern se_key_command_t se_modemap_lookup_command( se_modemap *map, se_key key );
extern se_key_command_t se_modemap_lookup_command_ex( se_modemap *map, se_key_seq );

/**
 * this will only search for a full keybinding
 */
extern se_key_command_t se_modemap_lookup_keybinding( se_modemap *map, se_key_seq );

extern void se_modemap_insert_keybinding_str(se_modemap*, const char*, se_key_command_t);
extern void se_modemap_insert_keybinding(se_modemap*, se_key_seq, se_key_command_t);

extern int se_modemap_keybinding_exists_str(se_modemap*, const char*);
extern int se_modemap_keybinding_exists(se_modemap*, se_key_seq);

////////////////////////////////////////////////////////////////////////////////

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

typedef GHashTable se_mode_hash;

extern se_mode* se_mode_create(const char* mode_name,
                               se_modemap* (*initMapProc)());
extern void se_mode_delete(const char* mode_name);

// globally defined mode as default
extern se_mode *fundamentalMode;
extern se_modemap *fundamentalModeMap;

#ifdef __cplusplus
}
#endif

#endif

