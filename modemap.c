/**
 * Mode Map Impl - 
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

#include "modemap.h"
#include "env.h"
#include "editor.h"

// do init in se_env_init
se_mode* fundamentalMode = NULL;
se_modemap *fundamentalModeMap = NULL;

char* se_key_to_string(se_key key)
{
    GString *gstr = g_string_new("");

    if ( key.modifiers & ControlDown ) g_string_append_printf( gstr, "C-" );
    if ( key.modifiers & MetaDown    ) g_string_append_printf( gstr, "M-" );
    if ( key.modifiers & SuperDown   ) g_string_append_printf( gstr, "S-" );
    if ( key.modifiers & EscapeDown  ) g_string_append_printf( gstr, "Esc " );
    if ( key.modifiers & Button1Down ) g_string_append_printf( gstr, "B1-" );
    if ( key.modifiers & Button2Down ) g_string_append_printf( gstr, "B2-" );
    if ( key.modifiers & Button3Down ) g_string_append_printf( gstr, "B3-" );

    if ( isprint(key.ascii) )
        g_string_append_printf( gstr, "%c", key.ascii );        
    else if ( key.ascii != 0xffff )
        g_string_append_printf( gstr, "\\x%x", key.ascii );

    return g_string_free( gstr, FALSE );
}

int se_key_is_control(se_key key)
{
    return ( key.ascii == 0xffff && key.modifiers > 0 );
}

int se_key_is_only(se_key key, unsigned short modifier)
{
    return (key.ascii == 0xffff && key.modifiers == modifier );
}

se_key se_key_null_init()
{
    return (se_key) {0, 0};
}

int se_key_is_null(se_key key)
{
    return (key.ascii == 0xffff || key.ascii == 0) && (key.modifiers == 0);
    
}

int se_key_is_equal(se_key key1, se_key key2)
{
    return  (key1.ascii == key2.ascii) && (key1.modifiers == key2.modifiers );
}

se_modemap* se_modemap_full_create(const char* map_name )
{
    g_assert( map_name && map_name[0] );
    
    se_modemap *map = g_malloc0( sizeof(se_modemap) );
    strncpy( map->mapName, map_name, sizeof map->mapName - 1 );
    map->keyToCmds = g_hash_table_new( NULL, NULL );    

    for (int i = 1; i < 128; ++i) {
        se_key sekey = { 0, i };
        int key = se_key_to_int( sekey );
        se_key_command_t val = (gpointer)se_self_silent_command;
        if ( isprint(i) ) {
            val = (gpointer)se_self_insert_command;
        }
        g_hash_table_insert( map->keyToCmds, GUINT_TO_POINTER(key), val );
    }
    return map;
}

void se_modemap_free(se_modemap* modemap)
{
    g_assert( modemap );
    g_hash_table_destroy( modemap->keyToCmds );
}

static se_key_command_t se_modemap_search_cmd(se_modemap* modemap, se_key key) 
{
    g_assert( modemap );
    se_debug( "" );

    GHashTableIter iter;
    gpointer hkey, hval;
    g_hash_table_iter_init( &iter, modemap->keyToCmds );
    
    while( g_hash_table_iter_next(&iter, &hkey, &hval) ) {
        se_key sekey = int_to_se_key( GPOINTER_TO_UINT(hkey) );
        if ( se_key_is_equal(sekey, key) ) {
            se_debug( "find key: %s, val: 0x%x",
                      se_key_to_string(sekey), (guint)hval );
            return (se_key_command_t)hval;
        }
    }

    return NULL;
}

se_key_command_t se_modemap_lookup_command(se_modemap* modemap, se_key key)
{
    g_assert( modemap && modemap->keyToCmds );
    /* se_debug( "find cmd for key [%s] in map %s", se_key_to_string(key), */
    /*           modemap->mapName ); */

    /* guint k; */
    /* memcpy( &k, &key, sizeof key ); */
    /* gpointer p = g_hash_table_lookup( modemap->keyToCmds, GUINT_TO_POINTER(k) ); */

    gpointer p = se_modemap_search_cmd( modemap, key );
    se_debug( "find cmd: 0x%x", (int)p );
    if ( p ) {
        return (se_key_command_t)p;
    }

    return NULL;
}

void se_modemap_dump(se_modemap* modemap) 
{
    g_assert( modemap );
    se_debug( "" );

    GHashTableIter iter;
    gpointer key, val;
    g_hash_table_iter_init( &iter, modemap->keyToCmds );

    int count = 0;
    while( g_hash_table_iter_next(&iter, &key, &val) ) {
        int ik = GPOINTER_TO_UINT(key);
        se_key sekey;
        memcpy( &sekey, &ik, sizeof sekey );
        se_key_command_t cmd = (se_key_command_t)val;
        se_debug( "key: %s, val: 0x%x", se_key_to_string(sekey), (guint)cmd );
        ++count;
    }
    se_debug( "total %d keys", count );
}

////////////////////////////////////////////////////////////////////////////////


static int se_mode_bindMap(se_mode* modep, se_modemap* map)
{
    g_assert( modep && map );
    if ( modep->modemap ) {
        // clean up
    }
    modep->modemap = map;
    return TRUE;
}

static int se_mode_init(se_mode* modep)
{
    modep->bindMap( modep, modep->initMap( modep ) );
    // run modemap hooks

    return TRUE;
}

int se_self_silent_command(se_world* world, se_command_args* args, se_key key)
{
    se_debug( "unprintable char, ignore" );
    return TRUE;
}

int se_self_insert_command(se_world* world, se_command_args* args, se_key key)
{
    if ( isprint(key.ascii) ) {
        world->current->insertChar( world->current, key.ascii );
    }
    
    return TRUE;
}

se_mode* se_mode_create(const char* mode_name, se_modemap* (*initMapProc)() )
{
    se_mode* modep = g_malloc0( sizeof(se_mode) );
    strncpy( modep->modeName, mode_name, sizeof modep->modeName );

    modep->init = se_mode_init;
    modep->bindMap = se_mode_bindMap;
    modep->initMap = initMapProc;
    
    modep->init( modep );
    return modep;
}

void se_mode_delete(const char* mode_name)
{

}


