/**
 * KMP implementation - used to find a single keyword in a string
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

kmp_entry* kmp_init( const char * pat )
{
    assert( pat );
    
    kmp_entry *kmp = (kmp_entry*) malloc( sizeof(kmp_entry) );
    
    int len = strlen( pat );
    assert( len > 0 );

    kmp->next = (int*) malloc( len * sizeof(int) );
    kmp->pat = (char*) malloc( len + 1);
    strncpy( kmp->pat, pat, len+1 );
    kmp->len = len;
    
    kmp->next[0] = -1;
    if ( len == 2 ) {
        kmp->next[1] = 0;
        
    } else {  // len > 2
        kmp->next[1] = 0;
        int idx = 2;
        while ( idx < len ) {
            int cnt = kmp->next[ idx-1 ];
            if ( pat[idx-1] == pat[cnt] ) {
                kmp->next[idx] = cnt+1;
                /* se_debug( "pat[%d] == pat[%d]\n", idx-1, kmp->next[idx] ); */
                
            } else {
                /* se_debug( "backtrack for %d\n", idx ); */
                
                while ( pat[idx-1] != pat[cnt] && cnt > 0 ) {
                    cnt = kmp->next[cnt];
                }
                if ( cnt <= 0 ) 
                    kmp->next[idx] = cnt;
                else
                    kmp->next[idx] = cnt+1;
            }
            /* se_debug( "cal next[%d] = %d\n", idx, kmp->next[idx] ); */
            ++idx;
        }
    }
    
#ifdef SE_DEBUG
    char head[512] = "";
    char msg[512] = "";
    for (int i = 0; i < len; ++i) {
        char tmp[10];
        snprintf( tmp, sizeof tmp, "\t%c", kmp->pat[i] );
        strncat( head, tmp, sizeof tmp );
        snprintf( tmp, sizeof tmp, "\t%d", kmp->next[i] );
        strncat( msg, tmp, sizeof tmp );
    }
    se_debug( "%s\n", head );
    se_debug( "%s\n", msg );
    
#endif

    return kmp;
}

void kmp_free( kmp_entry* kmp )
{
    assert( kmp );
    if ( kmp->next ) {
        free( kmp->next );
        free( kmp->pat );
    }

    free( kmp );
}

int kmp_match( kmp_entry* kmp, const char* str )
{
    assert( str );
    
    int pat_idx = 0, str_idx = 0;
    int str_len = strlen( str );
    assert ( str_len );
    
    while ( str_idx < str_len ) {
        
        if ( str[str_idx] == kmp->pat[pat_idx] ) {
            ++str_idx;
            ++pat_idx;
        } else {
            /* se_debug( "mismatch: str_idx:%d, pat_idx: %d\n", str_idx, pat_idx ); */
            
            int cnt = kmp->next[ pat_idx ];
            if ( cnt <= 0 ) {
                ++str_idx;
                pat_idx = 0;
                /* se_debug( "move str_idx: %d, reset pat idx to 0\n", str_idx ); */
                
            } else {
                pat_idx = cnt;
                /* se_debug( "move pat_idx to %d\n", pat_idx ); */
            }
        }

        if ( pat_idx == kmp->len ) {
            /* se_debug( "matched at %d\n", str_idx - kmp->len + 1 ); */
            return str_idx - kmp->len + 1;
        }
        
    }
    return -1;
}

void kmp_verify( kmp_entry* kmp, const char* origin, int match_idx )
{
    if ( match_idx < 0 )
        return;
    
    printf( "%s\n", origin );
    printf( "%*c%s\n", match_idx-1, ' ', kmp->pat );
}


void kmp_testcase2() 
{
    const char *A = "aaababaabb";
    const char *pattern = "ababaa";
    
    kmp_entry* kmp = kmp_init( pattern );
    int idx = kmp_match( kmp, A );
    kmp_verify( kmp, A, idx );
    kmp_free( kmp );
}

void kmp_testcase() 
{
    const char *A = "xyxxyxyxyyxyxyxyyxyxyxx";
    const char *B = "xyxxyxyxyyxyxyxyyxyxyxxyxyxx";
    const char *pattern = "xyxyyxyxyxx";
    
    kmp_entry* kmp = kmp_init( pattern );
    int idx = kmp_match( kmp, A );
    kmp_verify( kmp, A, idx );

    idx = kmp_match( kmp, B );
    kmp_verify( kmp, B, idx );

    kmp_free( kmp );

}

void run_testcases()
{
    kmp_testcase();
    kmp_testcase2();
}
