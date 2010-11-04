/**
 * Aho Corasick's augmented KMP string matching algo - used to find multi
 * keywords in one string
 * 
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

#include "util.h"
#include "submatch.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

static trie_node* trie_node_get_transfer_node( trie_node* node, int c );
static void trie_node_add_transfer( trie_node* node,  trie_node* next );

static trie_node* trie_node_new( int state, int expect, trie_node* parent )
{
    trie_node* node = (trie_node*) malloc( sizeof(trie_node) );
    node->state = state;
    node->expect = expect;
    node->parent = parent;
    node->trans = 0;
    node->trans_size = 0;
    node->accept = 0;  // default value
    return node;
}

static void trie_node_free( trie_node* node )
{
    assert( node );
    bzero( node, sizeof(trie_node) );
    
    free( node );
}

void trie_node_add_transfer( trie_node* node, trie_node* next )
{
    assert( node );
    assert( next );

    if ( node->trans ) {
        trie_node** new_trans = (trie_node**) realloc(
            node->trans, (node->trans_size+1)*sizeof(trie_node*) );
        if ( new_trans != node->trans ) {
            /* se_debug( "node->trans address changed\n" ); */
            node->trans = new_trans;
        }

        *(node->trans + node->trans_size) = next;
        node->trans_size++;

    } else {
        node->trans = (trie_node**) malloc( sizeof(trie_node*) );
        node->trans[0] = next;
        node->trans_size++;
    }
    /* se_debug( "add transfer: %d -> %d\n", node->state, next->state ); */
}

trie_t* trie_init()
{
    trie_t *trie = (trie_t*) malloc( sizeof(trie_t) );
    bzero( trie, sizeof(trie_t) );
    trie->root = trie_node_new( trie->nr_states++, 0, 0 );
    return trie;
}

trie_t* trie_add_keyword(trie_t* trie, const char* keyword)
{
    assert( trie );
    assert( trie->root );

    if ( trie->nr_keywords >= MAX_KEYWORDS ) {
        se_warn( "there is too many keywords\n");
        return trie;
    }
    
    int len = strlen( keyword );
    //TODO: check duplication
    trie->keywords[trie->nr_keywords] = strndup( keyword, len+1 );
    trie->nr_keywords++;

    
    trie_node* cur_state = trie->root;
    assert( cur_state );
    
    for ( int i = 0; i < len; ++i ) {
        trie_node* next_state = trie_node_get_transfer_node( cur_state, keyword[i] );
        if ( !next_state ) {
            // need to build new state
            trie_node* next_state = trie_node_new(
                trie->nr_states++, keyword[i], cur_state );
            next_state->accept =  ( i == len - 1 );
            
            trie->node_list = (trie_node**)realloc(
                trie->node_list, sizeof(trie_node*) * trie->nr_states );
            trie->node_list[trie->nr_states-1] = next_state;
            /* se_debug( "new node: (%c, %d)\n", next_state->expect, next_state->state ); */
            
            trie_node_add_transfer( cur_state, next_state );
            cur_state = next_state;
            
        } else {
            /* se_debug( "node exists: (%c, %d)\n", next_state->expect, next_state->state ); */
            cur_state = next_state;
        }
    }

    return trie;
}

trie_node* trie_node_get_transfer_node( trie_node* node, int c )
{
    assert( node );
    if ( node->trans_size == 0 )
        return 0;
    
    for ( int i = 0; i < node->trans_size; ++i ) {
        trie_node* n = node->trans[i];
        if ( n->expect == c ) {
            /* se_debug("get transfer: [%d, %c] -> %d\n", node->state, c, n->state ); */
            return n;
        }            
    }

    return 0;
}

static void _trie_calc_children_failure_func( trie_t* trie, GList* node_stack )
{
    GList* next_layer = 0;

    int len = g_list_length( node_stack );
    for ( int i = 0; i < len; ++i ) {
        trie_node* next = g_list_nth( node_stack, i )->data;
        for ( int j = 0; j < next->trans_size; ++j ) {
            next_layer = g_list_append( next_layer, next->trans[j] );
        }

        trie_node* par_node = next->parent;
        if ( par_node == trie->root ) {
            trie->node_failure_func[next->state] = 0;
            continue;
        }
        
        int prev_matched = trie->node_failure_func[par_node->state];
        /* se_debug( "prev_matched:%d\n", prev_matched ); */
        
        trie_node* succ = trie->node_list[prev_matched];
        trie_node* succ_next = trie_node_get_transfer_node(succ, next->expect);

        while( prev_matched && !succ_next ) {
            prev_matched = trie->node_failure_func[prev_matched];
            /* se_debug( "prev_matched:%d\n", prev_matched ); */
            succ = trie->node_list[prev_matched];
            succ_next = trie_node_get_transfer_node(succ, next->expect);
        }

        if ( succ_next )
            trie->node_failure_func[next->state] = succ_next->state;
        else
            trie->node_failure_func[next->state] = 0;
        /* se_debug( "failure[%d] = %d\n", next->state, */
        /*        trie->node_failure_func[next->state] ); */
    }

    if ( next_layer ) {
        _trie_calc_children_failure_func( trie, next_layer );
        g_list_free( next_layer );
    }
}

trie_t* trie_init_with_keywords(char** kws, int nr_kws )
{
    trie_t *trie = (trie_t*) malloc( sizeof(trie_t) );
    bzero( trie, sizeof(trie_t) );
    trie->root = trie_node_new( trie->nr_states, 0, 0 );

    trie->node_list = (trie_node**) malloc( sizeof(trie_node*) );
    trie->node_list[trie->nr_states++] = trie->root;

    for ( int i = 0; i < nr_kws; ++i ) {
        trie_add_keyword( trie, kws[i] );
    }
    
    // calculate failure function for new state
    trie->node_failure_func = (int*) malloc( sizeof(int) * trie->nr_states );
    trie->node_failure_func[0] = 0;
    
    GList* node_stack = NULL;
    for ( int i = 0; i < trie->root->trans_size; ++i ) {
        node_stack = g_list_append( node_stack, trie->root->trans[i] );
    }
    _trie_calc_children_failure_func( trie, node_stack );
    g_list_free( node_stack );
    
    return trie;
}

void trie_free( trie_t* trie)
{
    assert( trie );
    
    for ( int i = 0; i < trie->nr_keywords; ++i ) {
        free( trie->keywords[i] );
    }
    
    for ( int i = 0; i < trie->nr_states; ++i ) {
        trie_node_free( trie->node_list[i] );
    }
    free( trie->node_list );
    
    free( trie->node_failure_func );

    bzero( trie, sizeof(trie_t) );
}

static char* trie_backtrack_get_matched( trie_node* node )
{
    char buf[128] = "";
    trie_node* par = node;
    while( par->state != 0 ) {
        char part[128];
        snprintf( part, sizeof part, "%c%s", par->expect, buf );
        par = par->parent;
        strcpy( buf, part );
    }
    
    return strdup(buf);
}

static void _trie_dump(trie_t* trie)
{
    char head[1024] = "";
    char val[1024] = "";
    
    se_debug( "failure functions: \n" );
    for ( int i = 0; i < trie->nr_states; ++i ) {
        char part[10] = "";
        snprintf(part, sizeof part, "%d\t", i);
        strcat( head, part );

        bzero( part, 10 );
        snprintf(part, sizeof part, "%d\t", trie->node_failure_func[i]);
        strcat( val, part );
    }
    se_debug( "%s\n", head );
    se_debug( "%s\n", val );
}

int trie_match_ex(trie_t* trie, const char* str,
                  int nmatch, trie_match_result results[])
{
    assert( trie );
    assert( str );
    
    int len = strlen( str );
    trie_node* cur_state = trie->root;
    int nr_match = 0;
    
    for ( int i = 0; i < len; ++i ) {
        if ( nr_match >= nmatch )
            break;
        
        trie_node* next_state = trie_node_get_transfer_node( cur_state, str[i] );
        if ( next_state ) {
            cur_state = next_state;
            if ( cur_state->accept ) {
                char* matched_kw = trie_backtrack_get_matched( cur_state );
                results[nr_match++] = (trie_match_result) { matched_kw, i - strlen(matched_kw) };
                /* msg( "matched: %s at %d\n", matched_kw, i - strlen(matched_kw) ); */
                cur_state = trie->node_list[ trie->node_failure_func[cur_state->state] ];
            }
        } else {
            cur_state = trie->node_list[ trie->node_failure_func[cur_state->state] ];
        }
    }

    return nr_match;
}

int trie_match(trie_t* trie, const char* str)
{
    assert( trie );
    assert( str );
    
    int len = strlen( str );
    trie_node* cur_state = trie->root;
    int nr_match = 0;
    
    for ( int i = 0; i < len; ++i ) {
        trie_node* next_state = trie_node_get_transfer_node( cur_state, str[i] );
        if ( next_state ) {
            cur_state = next_state;
            if ( cur_state->accept ) {
                ++nr_match;
                /* char* matched_kw = trie_backtrack_get_matched( cur_state ); */
                /* msg( "matched: %s at %d\n", matched_kw, i - strlen(matched_kw) ); */
                cur_state = trie->node_list[ trie->node_failure_func[cur_state->state] ];
            }
        } else {
            cur_state = trie->node_list[ trie->node_failure_func[cur_state->state] ];
        }
    }

    return nr_match;
}

void trie_testcase()
{
    char* keywords[] = {
        "he", "she", "his", "hers"
    };
    int nr_keywords = 4;
    
    trie_t* trie = trie_init_with_keywords( keywords, nr_keywords );
    _trie_dump( trie );

    trie_match( trie, "uushersab" );
    trie_free( trie );
}

