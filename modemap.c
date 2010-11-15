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

struct se_modemap_data
{
    se_key key;
    se_key_command_t cmd;  // Null of the node is a non-leaf node
};

static GNode* se_modemap_search_next_level(se_modemap *map, GNode *node, se_key key)
{
    g_assert( map && node );
   /* se_debug( "search for %s", se_key_to_string(key) ); */
    
    GNode *np = g_node_first_child ( node );
    while ( np ) {
        se_modemap_data *data = np->data;
        if ( se_key_is_equal(data->key, key) ) {
           /* se_debug( "find %s at level %d", se_key_to_string(key), g_node_depth(np) ); */
            return np;
        }
        np = g_node_next_sibling( np );
    }

    return NULL;
}

static GNode* se_modemap_find_match_node(se_modemap* map, se_key_seq keyseq)
{
    g_assert( map );

    int i = 0;
    GNode *np = se_modemap_search_next_level(map, map->root, keyseq.keys[i++]);
    while ( np ) {
        if ( i == keyseq.len )
            break;
        np = se_modemap_search_next_level(map, np, keyseq.keys[i++]);
    }

    return np;
}

se_modemap* se_modemap_simple_create(const char* map_name )
{
    g_assert( map_name && map_name[0] );
    
    se_modemap *map = g_malloc0( sizeof(se_modemap) );
    strncpy( map->mapName, map_name, sizeof map->mapName - 1 );
    se_modemap_data *root_data = g_malloc0( sizeof(se_modemap_data) );
    root_data->key = se_key_null_init();
    map->root = g_node_new( root_data );
    return map;
}

se_modemap* se_modemap_full_create(const char* map_name )
{
    g_assert( map_name && map_name[0] );
    
    se_modemap *map = se_modemap_simple_create( map_name );
    for (int i = 0x20; i <= 0x7e; ++i) {
        g_assert( isprint(i) );
        se_key_command_t cmd = se_self_insert_command;
        se_key_seq keyseq = {
            1, {{0, i}}
        };
        se_modemap_insert_keybinding( map, keyseq, cmd );
    }
    /*
     * XK_BackSpace                     0xff08
     * XK_Tab                           0xff09
     * XK_Return                        0xff0d
     * XK_Scroll_Lock                   0xff14
     * XK_Escape                        0xff1b
     * XK_Delete                        0xffff
     */    
    se_modemap_insert_keybinding_str( map, "C-x C-c", se_editor_quit_command );
    se_modemap_insert_keybinding_str( map, "C-u", se_universal_arg_command );    
    se_modemap_insert_keybinding_str( map, "C-g", se_kbd_quit_command );
    se_modemap_insert_keybinding_str( map, "C-j", se_newline_and_indent_command );
    se_modemap_insert_keybinding_str( map, "C-i", se_indent_for_tab_command );
    se_modemap_insert_keybinding_str( map, "Tab",
                                      se_indent_for_tab_command );
    se_modemap_insert_keybinding_str( map, "Return",
                                      se_newline_command );
    se_modemap_insert_keybinding_str( map, "BackSpace",
                                      se_backspace_command );
    se_modemap_insert_keybinding_str( map, "C-d",
                                      se_delete_forward_command );
    
    se_modemap_insert_keybinding_str( map, "C-f", se_forward_char_command );
    se_modemap_insert_keybinding_str( map, "C-b", se_backward_char_command );
    se_modemap_insert_keybinding_str( map, "C-n", se_forward_line_command );
    se_modemap_insert_keybinding_str( map, "C-p", se_backward_line_command );
    
    se_modemap_insert_keybinding_str( map, "C-a", se_move_beginning_of_line_command );
    se_modemap_insert_keybinding_str( map, "C-e", se_move_end_of_line_command );
    
    se_modemap_insert_keybinding_str( map, "Home", se_move_beginning_of_line_command );
    se_modemap_insert_keybinding_str( map, "End", se_move_end_of_line_command );

    se_modemap_insert_keybinding_str( map, "C--", se_previous_buffer_command );
    se_modemap_insert_keybinding_str( map, "C-=", se_next_buffer_command );
    
    return map;
}

se_key_command_t se_modemap_lookup_command_ex( se_modemap *map, se_key_seq keyseq )
{
    GNode *np = se_modemap_find_match_node( map, keyseq );
    if ( np ) {
        if ( G_NODE_IS_LEAF(np) ) {
            se_modemap_data *data = np->data;
            return data->cmd;
        }
        return se_second_dispatch_command;
    }

    return NULL;    
}

se_key_command_t se_modemap_lookup_command( se_modemap *map, se_key key )
{
    GNode *np = se_modemap_search_next_level( map, map->root, key );
    if ( np ) {
        if ( G_NODE_IS_LEAF(np) ) {
            se_modemap_data *data = np->data;
            g_assert( data->cmd );            
            return data->cmd;
        }
        return se_second_dispatch_command;        
    }
    return NULL;
}

se_key_command_t se_modemap_lookup_keybinding( se_modemap *map, se_key_seq keyseq )
{
    GNode *np = se_modemap_find_match_node( map, keyseq );
    if ( np && G_NODE_IS_LEAF(np) ) {
        se_modemap_data *data = np->data;
        g_assert( data->cmd );
        return data->cmd;
    }

    return NULL;
}

int se_modemap_keybinding_exists_str(se_modemap* map, const char* keys_rep)
{
    g_assert( map && keys_rep );
    se_key_seq keyseq = se_key_seq_from_string( keys_rep );
    return se_modemap_keybinding_exists( map, keyseq );
    
}

int se_modemap_keybinding_exists(se_modemap* map, se_key_seq keyseq)
{
    GNode *np = se_modemap_find_match_node( map, keyseq );
    if ( np ) {
        if ( G_NODE_IS_LEAF(np) ) {
            se_modemap_data *data = np->data;
            g_assert( data->cmd );
            return TRUE;
        }
    }
    return FALSE;
}

void se_modemap_insert_keybinding(se_modemap* map, se_key_seq keyseq,
                                  se_key_command_t cmd)
{
    se_modemap_insert_keybinding_str( map, se_key_seq_to_string(keyseq), cmd );
}

// remove the whole sub tree under parent, parent is excluded
static void se_delete_subtree( GNode *parent )
{
    g_assert( parent );
    GNode *np = g_node_first_child( parent );
    while ( np ) {
        g_node_unlink( np );
        g_node_destroy( np );
        np = g_node_first_child( parent );
    }
}

void se_modemap_insert_keybinding_str( se_modemap *map, const char* keys_rep,
                                   se_key_command_t cmd )
{
    g_assert( map && keys_rep && cmd );
    se_key_seq keyseq = se_key_seq_from_string( keys_rep );
    if ( keyseq.len <= 0 ) {
        se_warn( "bad key seq" );
        return;
    }
    
    if ( se_modemap_keybinding_exists(map, keyseq) ) {
        /* se_msg("remap an existed keybinding" ); */
    }

    GNode *node = map->root;
    int need_create_node = FALSE; // trace from which level we need to create nodes
    for (int i = 0; i < keyseq.len; ++i) {
        se_key key = keyseq.keys[i];
        /* se_debug( "current key %s", se_key_to_string(key) ); */
        
        if ( !need_create_node ) {
            GNode *key_node = se_modemap_search_next_level(map, node, key);
            if ( !key_node ) {
                need_create_node = TRUE;
                --i;
                /* se_debug( "set need_create_node and i = %d", i ); */
                continue;
            }
            
            /* se_debug( "key %s exists at level %d", se_key_to_string(key), */
            /*           g_node_depth(node) ); */
            node = key_node;
            se_modemap_data *data = key_node->data;
            g_assert( data );
            g_assert( se_key_is_equal(key, data->key) );
            
            if ( i+1 == keyseq.len ) { // bind to a cmd
                if ( !G_NODE_IS_LEAF(key_node) )  {
                    // delete sub tree
                    /* se_debug( "delete sub tree" ); */
                    se_delete_subtree( key_node );
                }

                g_assert( G_NODE_IS_LEAF(key_node) );
                data->cmd = cmd;
                /* se_debug( "rebind key %s", se_key_to_string(key) ); */
                
            } else {
                if ( G_NODE_IS_LEAF(key_node) ) {
                    g_assert( data->cmd );
                    data->cmd = NULL;
                    need_create_node = TRUE; // create nodes from next level
                    /* se_debug( "set need_create_node, reset key %s to Null", */
                    /*           se_key_to_string(key) ); */
                    
                } else {
                    g_assert( data->cmd == NULL );
                }
            }
            
        } else {
            se_modemap_data *data = g_malloc0( sizeof(se_modemap_data) );
            data->key = key;
            GNode *key_node = g_node_append_data( node, data );
            if ( i+1 == keyseq.len ) { // bind to a cmd
                /* se_debug( "leaf node (level %d) of key %s: bind cmd", */
                /*           g_node_depth(key_node), se_key_to_string(data->key) ); */
                data->cmd = cmd;
            }
            node = key_node;
        }
    }
}

void se_modemap_free(se_modemap* modemap)
{
    g_assert( modemap );
    g_node_destroy( modemap->root );
}

void se_modemap_dump(se_modemap* modemap) 
{
    g_assert( modemap );
    GNode *np = modemap->root;
    g_assert( np );
    

    /* se_debug( "total %d keys", count ); */
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


