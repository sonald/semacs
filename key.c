
/**
 * Keys  - 
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

#include "key.h"
#include <sys/types.h>
#include <regex.h>
#include "submatch.h"

char* se_key_to_string(se_key key)
{
    /* se_debug( "raw key: 0x%x", se_key_to_int(key) ); */
    GString *gstr = g_string_new("");

    if ( key.modifiers & ControlDown ) g_string_append_printf( gstr, "C-" );
    if ( key.modifiers & MetaDown    ) g_string_append_printf( gstr, "M-" );
    if ( key.modifiers & SuperDown   ) g_string_append_printf( gstr, "H-" );
    if ( key.modifiers & ShiftDown   ) g_string_append_printf( gstr, "S-" );    
    if ( key.modifiers & Button1Down ) g_string_append_printf( gstr, "B1-" );
    if ( key.modifiers & Button2Down ) g_string_append_printf( gstr, "B2-" );
    if ( key.modifiers & Button3Down ) g_string_append_printf( gstr, "B3-" );

    if ( key.modifiers & EscapeDown  )
        g_string_append_printf( gstr, "Esc" );
    else if ( isprint(key.ascii) )
        g_string_append_printf( gstr, "%c", key.ascii );        
    else if ( (key.ascii != 0xffff) && (key.ascii != 0) )
        g_string_append_printf( gstr, "\\x%x", key.ascii );
    else
        g_string_printf( gstr, "(null)" );

    return g_string_free( gstr, FALSE );
}

se_key se_key_from_string( const char* rep )
{
    se_key key = se_key_null_init();

    int rep_len = strlen(rep);
    if ( rep_len <= 0 )
        return key;
    
    char *last_prefix = strrchr( rep, '-');
    if ( !last_prefix || rep_len == 1 ) { // rep is not a combination key
        if ( rep_len == 1 ) {
            key.ascii = rep[0];
            
        } else if ( strcmp(rep, "Esc") == 0 ) {
            key.modifiers = EscapeDown;
            
        } else if ( strncmp(rep, "\\x", 2) == 0 ) {
            key.ascii = strtol( rep+2, NULL, 16 );
            
        } else if ( strcmp(rep, "(null)") == 0 ) {
            return key;
            
        } else {
            se_warn( "invalid rep of key: %s", rep );
        }
        
        /* se_debug( "build key[0x%x]: %s", (gint)se_key_to_int(key), */
        /*           se_key_to_string(key) ); */
        return key;
    }
    
    char* keywords[] = {
        "C-", "S-", "M-", "H-", "B1-", "B2-", "B3-"
    };
    int nr_keywords = 6;
    
    trie_t* trie = trie_init_with_keywords( keywords, nr_keywords );

    trie_match_result results[MAX_KEYWORDS];
    int nr_match = trie_match_ex( trie, rep, ARRAY_LEN(results), results );
    trie_free( trie );

    if ( nr_match <= 0 ) {
        se_warn( "rep (%s) is not a valid key", rep );
        return key;
    }

    struct match_data {
        const char* prefix;
        int modifier;
    } match_datas[] = {
        {"C-", ControlDown},  {"M-", MetaDown},
        {"S-", ShiftDown},    {"B1-", Button1Down},
        {"B2-", Button2Down}, {"B3-", Button3Down},
        {"H-", SuperDown }, 
    };
    
    for (int i = 0; i < nr_match; ++i) {
        for (int j = 0; j < ARRAY_LEN(match_datas); ++j) {
            if ( strcmp(results[i].keyword, match_datas[j].prefix) == 0 ) {
                /* se_debug( "catch %s", match_datas[j].prefix ); */
                key.modifiers |= match_datas[j].modifier;
                break;
            }
        }
    }

    ++last_prefix;
    if ( *last_prefix == 0 ) {
        return se_key_null_init();
    }

    if ( strlen(last_prefix) == 1 ) {
        key.ascii = *last_prefix;
    } else {
        if ( strcmp(last_prefix, "Esc") == 0 ) {
            key.modifiers = EscapeDown;
        }
    }

    /* se_debug( "build key[0x%x]: %s", (gint)se_key_to_int(key), */
    /*           se_key_to_string(key) ); */
    return key;
}

/* se_key se_key_from_string( const char* rep ) */
/* { */
/*     se_key key = se_key_null_init(); */
/*     regex_t regex; */
/*     regmatch_t matches[10]; // maximum 10 */
    
/*     if ( regcomp( &regex, "([CMSB123]-)*([a-zA-Z]+)", REG_EXTENDED ) != 0 ) { */
/*         se_error( "regexp is no valid" ); */
/*         return key; */
/*     } */

/*     struct match_data { */
/*         const char* prefix; */
/*         int modifier; */
/*     } match_datas[] = { */
/*         {"C-", ControlDown},  {"M-", MetaDown}, */
/*         {"S-", ShiftDown},    {"B1-", Button1Down}, */
/*         {"B2-", Button2Down}, {"B3-", Button3Down}, */
/*     }; */
    
/*     if ( regexec( &regex, rep, 10, matches, 0 ) == 0 ) { */
/*         for (int i = 0; i < 10; ++i) { */
/*             if ( matches[i].rm_so < 0 ) */
/*                 continue; */

/*             const char *s = rep + matches[i].rm_so; */
/*             int catch_len = matches[i].rm_eo - matches[i].rm_so; */
/*             se_debug( "match %d %*s", catch_len, catch_len, s ); */
/*             int j = 0; */
/*             for (j = 0; j < ARRAY_LEN(match_datas); ++j) { */
/*                 if ( strncmp(s, match_datas[j].prefix, catch_len) == 0 ) { */
/*                     se_debug( "catch %s", match_datas[j].prefix ); */
/*                     key.modifiers |= match_datas[j].modifier; */
/*                     break; */
/*                 } */
/*             } */

/*             if ( j == ARRAY_LEN(match_datas) ) { */
/*                 if ( catch_len == 1 ) { */
/*                     se_debug( "catch char %*s", catch_len, s ); */
/*                     key.ascii = s[0]; */
/*                 } else { */
/*                     if ( strncmp(s, "Esc", matches[i].rm_eo) == 0 ) */
/*                         key.modifiers |= EscapeDown; */
/*                 } */
/*             } */
/*         } */
/*     } else { */
/*         se_msg( "key rep '%s' is not valid", rep ); */
/*     } */

/*     regfree( &regex ); */
    
/*     se_debug( "create key: %s", se_key_to_string(key) ); */
/*     return key; */
/* } */

int se_key_is_control(se_key key)
{
    return ( (key.ascii == 0 || key.ascii == 0xffff) && key.modifiers > 0 );
}

int se_key_is_only(se_key key, unsigned short modifier)
{
    return ((key.ascii == 0 || key.ascii == 0xffff) && key.modifiers == modifier );
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

////////////////////////////////////////////////////////////////////////////////

se_key_seq se_key_seq_null_init()
{
    se_key_seq keyseq;
    bzero( &keyseq, sizeof keyseq );
    return keyseq;
}

se_key_seq se_key_seq_from_string( const char*rep )
{
    se_key_seq keyseq = se_key_seq_null_init();

    int rep_len = strlen( rep );
    if ( rep_len <= 0 )
        return keyseq;

    if ( rep_len == 1 ) {
        keyseq.len = 1;
        keyseq.keys[0] = se_key_from_string( rep );
        return keyseq;
    }
    
    int n = 0;
    
    gchar** key_reps = g_strsplit_set( rep, " ", -1 );
    if ( *key_reps == NULL ) {
        keyseq.len = 1;
        keyseq.keys[0] = se_key_from_string( rep );
        return keyseq;
    }
    
    gchar** repp = key_reps;
    while ( *repp ) {
        if ( n >= SE_MAX_KEY_SEQ_LEN )
            break;
        
        if ( strlen(*repp) == 0 ) {
            ++repp;
            continue;
        }

        keyseq.keys[n++] = se_key_from_string( *repp );
        ++repp;
    }

    g_strfreev( key_reps );
    keyseq.len = n;
    return keyseq;
}

char* se_key_seq_to_string( se_key_seq keyseq )
{
    if ( keyseq.len > SE_MAX_KEY_SEQ_LEN ) {
        se_warn( "key seq is too large, automatically trimmed" );
    }
    
    GString *gstr = g_string_new( "" );
    for (int i = 0; i < MIN(keyseq.len, SE_MAX_KEY_SEQ_LEN); ++i) {
        g_string_append_printf( gstr, "%s ", se_key_to_string(keyseq.keys[i]) );
    }

    if ( gstr->len > 0 && gstr->str[gstr->len-1] == ' ' )
        g_string_erase( gstr, gstr->len-1, 1 );

    return g_string_free( gstr, FALSE );
}

int se_key_seq_is_equal( se_key_seq ks1, se_key_seq ks2 )
{
    if ( ks1.len != ks2.len )
        return FALSE;

    for (int i = 0; i < ks1.len; ++i) {
        if ( !se_key_is_equal(ks1.keys[i], ks2.keys[i]) )
            return FALSE;
    }

    return TRUE;
}

