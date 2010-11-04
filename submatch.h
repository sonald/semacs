/**
 * match utilities - comm definitions
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

#ifndef _str_match_comm_h
#define _str_match_comm_h

#define _GNU_SOURCE

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include <glib.h>

/**
 * KMP Algo struct
 */

typedef struct kmp_next_s 
{
    int len;
    int *next;
    char *pat;
} kmp_entry;

extern kmp_entry* kmp_init( const char * pat );
extern void kmp_free( kmp_entry* kmp );
extern int kmp_match( kmp_entry* kmp, const char* str );
extern void kmp_verify( kmp_entry* kmp, const char* origin, int match_idx );


/**
 * Aho Corasick Algo structs
 */
typedef struct trie_transfer_struct trie_transfer;

typedef struct trie_node_struct
{
    int expect;  // char that makes state go here
    int state;
    int accept;  // bool value - true if a keyword is met
    
    struct trie_node_struct* parent;
    struct trie_node_struct** trans;
    int trans_size;
} trie_node;

#define MAX_KEYWORDS 16

/**
 * trie struct
 */
typedef struct trie_struct
{
    int nr_states;
    trie_node** node_list; // this used to locate node by state efficiently
    int* node_failure_func;
    
    char* keywords[MAX_KEYWORDS];
    int nr_keywords;

    /**
     * root node of trie tree
     */
    trie_node *root; // root node
} trie_t;

extern trie_t* trie_init();
extern trie_t* trie_add_keyword(trie_t* trie, const char* keyword);
extern trie_t* trie_init_with_keywords(char** kws, int nr_kws );
extern void trie_free( trie_t* trie);

/**
 * this just tells you that weather one of keywords has been found in str,
 * if you exepcting matched details, use routine below
 */
extern int trie_match(trie_t* trie, const char* str);

DEF_CLS(trie_match_result);
struct trie_match_result
{
    const char* keyword;  // do not free, internal reference of kw
    int position;
};

extern int trie_match_ex(trie_t* trie, const char* str,
                         int nmatch, trie_match_result results[]);

#endif
