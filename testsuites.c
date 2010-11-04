#include "util.h"
#include "editor.h"
#include "xview.h"
#include "key.h"
#include "cmd.h"
#include "modemap.h"

void test_keys()
{
    se_key key = se_key_from_string( "C-M-S-H-x" );
    g_assert( se_key_to_int(key) == 0x78000f );

    g_assert( se_key_is_equal(se_key_from_string("C-H-c"), se_key_from_string("H-C-c")) );
        
    key = se_key_from_string( "T" );
    g_assert( strcmp("T", se_key_to_string(key)) == 0 );
    
    se_key_seq keyseq = se_key_seq_from_string( "C-M-a" );
    g_assert( strcmp( "C-M-a", se_key_seq_to_string(keyseq)) == 0
        || strcmp( "M-C-a", se_key_seq_to_string(keyseq)) == 0 );
    
    keyseq = se_key_seq_from_string( "C-c c" );
    g_assert( strcmp( "C-c c", se_key_seq_to_string(keyseq)) == 0 );

    keyseq = se_key_seq_from_string( "C-x M-s g" );
    g_assert( strcmp( "C-x M-s g", se_key_seq_to_string(keyseq)) == 0 );

    {
        
        for (int i = 1; i < 128; ++i) {
            se_key k1 = {0, i};

            char s[2];
            s[0] = i;
            s[1] = 0;
            se_key k2 = se_key_from_string( s );
            g_assert( se_key_is_equal(k1, k2) );
            g_assert( se_key_is_equal(k1, se_key_from_string(se_key_to_string(k1))) );
            
            /* se_debug( "%d. key: %s", i, se_key_to_string(k1) ); */
        }
    }
}

void test_keys2()
{

    {
        se_key_seq keyseq = {
            1, {{0, 1}}
        };

        char s[2];
        s[0] = 1;
        s[1] = 0;
        se_key_seq ks2 = se_key_seq_from_string( s );

        g_assert( ks2.len == 1 );
        se_key k = ks2.keys[0];
        g_test_message( "k: 0x%x", se_key_to_int(k) );

        k = keyseq.keys[0];
        g_test_message( "k: 0x%x", se_key_to_int(k) );
    
        g_assert( se_key_seq_is_equal(keyseq, ks2) );
    }

    {
        
        for (int i = 1; i < 128; ++i) {
            se_key_command_t cmd = se_self_silent_command;
            if ( isprint(i) ) {
                cmd = se_self_insert_command;
            }

            se_key_seq keyseq = {
                1, {{0, i}}
            };

            char s[2];
            s[0] = i;
            s[1] = 0;
            se_key_seq ks2 = se_key_seq_from_string( s );
            g_assert( se_key_seq_is_equal(keyseq, ks2) );
            /* se_debug( "%d. keyseq: %s", i, se_key_seq_to_string(ks2) ); */
        }
    }
}

void test_modemap1()
{
    se_modemap *map = se_modemap_simple_create( "testMap" );
    g_assert( map );

    se_modemap_insert_keybinding_str( map, "t", se_self_insert_command );
    se_key_command_t cmd1 = se_modemap_lookup_command(map, se_key_from_string("t"));
    g_assert( cmd1 );
    g_assert( cmd1 == se_self_insert_command );

    se_modemap_insert_keybinding_str( map, "t", se_self_silent_command );
    cmd1 = se_modemap_lookup_command(map, se_key_from_string("t"));
    g_assert( cmd1 );
    g_assert( cmd1 == se_self_silent_command );

    se_modemap_insert_keybinding_str( map, "s", se_self_silent_command );
    cmd1 = se_modemap_lookup_command(map, se_key_from_string("s"));
    g_assert( cmd1 );
    g_assert( cmd1 == se_self_silent_command );
    
    se_modemap_free( map );
}


void test_modemap2()
{
    se_modemap *map = se_modemap_simple_create( "testMap" );
    g_assert( map );

    se_modemap_insert_keybinding_str( map, "C-c c", se_self_silent_command );
    GNode *np = map->root->children;
    g_assert( np );
    np = np->children;
    g_assert( np );
    
    g_assert( se_modemap_keybinding_exists_str(map, "C-c c") );

    se_modemap_free( map );
}

void test_modemap3()
{
    se_modemap *map = se_modemap_simple_create( "testMap" );
    g_assert( map );

    se_modemap_insert_keybinding_str( map, "C-c", se_self_insert_command );
    se_key_command_t cmd1 = se_modemap_lookup_command(map, se_key_from_string("C-c"));
    g_assert( cmd1 == se_self_insert_command );

    se_modemap_insert_keybinding_str( map, "C-c c", se_self_silent_command );
    g_assert( se_modemap_keybinding_exists_str(map, "C-c c") );

    cmd1 = se_modemap_lookup_command(map, se_key_from_string("C-c"));
    g_assert( cmd1 == se_second_dispatch_command );

    se_modemap_insert_keybinding_str( map, "C-c t", se_self_silent_command );
    cmd1 = se_modemap_lookup_keybinding(map, se_key_seq_from_string("C-c t"));
    g_assert( cmd1 == se_self_silent_command );
    
    se_modemap_free( map );
}

void test_modemap4()
{
    se_modemap *map = se_modemap_full_create( "testMap" );
    g_assert( map );

    for (int i = 1; i < 128; ++i) {
        char ks[2] = "";
        ks[0] = i;
        /* se_debug( "testing for %c(0x%x)", i, i ); */
        g_assert( se_modemap_keybinding_exists_str(map, ks) );
    }
    
    se_key_command_t cmd1 = se_modemap_lookup_command(map, se_key_from_string("C-x"));
    g_assert( cmd1 == se_second_dispatch_command );
    
    cmd1 = se_modemap_lookup_command(map, se_key_from_string("C-g"));
    g_assert( cmd1 == se_kbd_quit_command );

    cmd1 = se_modemap_lookup_command(map, se_key_from_string("C-g"));
    g_assert( cmd1 == se_kbd_quit_command );

    se_modemap_insert_keybinding_str( map, "C-x C-p k", se_self_silent_command );
    g_assert( se_modemap_keybinding_exists_str(map, "C-x C-p k") );
    g_assert( se_modemap_keybinding_exists_str(map, "C-x C-c") );
    
    cmd1 = se_modemap_lookup_command_ex(map, se_key_seq_from_string("C-x C-p"));
    g_assert( cmd1 == se_second_dispatch_command );
    
    se_modemap_insert_keybinding_str( map, "C-x", se_self_silent_command );
    g_assert( !se_modemap_keybinding_exists_str(map, "C-x C-p k") );
    g_assert( se_modemap_keybinding_exists_str(map, "C-x") );
    
    se_modemap_free( map );
}

int main(int argc, char *argv[])
{
    g_test_init( &argc, &argv, NULL );

    g_test_add_func( "/semacs/keys/1",  test_keys );
    g_test_add_func( "/semacs/keys/2",  test_keys2 );    
    g_test_add_func( "/semacs/modemap/simple/1", test_modemap1 );
    g_test_add_func( "/semacs/modemap/simple/2", test_modemap2 );
    g_test_add_func( "/semacs/modemap/simple/rebinding", test_modemap3 );
    g_test_add_func( "/semacs/modemap/compound", test_modemap4 );
    
    g_test_run();
    
    return 0;
}

