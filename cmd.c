/**
 * common commands impl - 
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


#include "cmd.h"
#include "editor.h"

se_command_args* se_command_args_create()
{
    se_command_args *args = g_malloc0( sizeof(se_command_args) );
    args->composedStr = g_string_new("");
    args->universalArg = g_string_new("");
    return args;
}

void se_command_args_destroy(se_command_args* args)
{
    g_assert( args );
    g_string_free( args->composedStr, TRUE );
    g_string_free( args->universalArg, TRUE );
    g_free( args );
}

se_command_args se_command_args_init()
{
    se_command_args args;
    bzero( &args, sizeof args );
    args.composedStr = g_string_new("");
    args.universalArg = g_string_new("");
    return args;
}

void se_command_args_clear(se_command_args* args)
{
    g_string_truncate( args->composedStr, 0 );
    g_string_truncate( args->universalArg, 0 );
    args->flags = 0;
    args->keyseq = se_key_seq_null_init();
    args->prefix_arg = 0;
}

gboolean se_command_args_is_null(se_command_args* args)
{
    return (args->flags == 0) && (args->prefix_arg == 0);
}

DEFINE_CMD(se_self_silent_command)
{
    se_debug( "unprintable char, ignore" );
    return TRUE;
}

DEFINE_CMD(se_self_insert_command)
{
    if ( args->flags & SE_IM_ARG ) {
        return SAFE_CALL( world->current, insertString, args->composedStr->str );
    } else {
        return SAFE_CALL( world->current, insertChar, key.ascii );
    }
}

//TODO: indent
DEFINE_CMD(se_newline_and_indent_command)
{
    return SAFE_CALL( world->current, insertChar, '\n' );
}

DEFINE_CMD(se_newline_command)
{
    return SAFE_CALL( world->current, insertChar, '\n' );
}

DEFINE_CMD(se_indent_for_tab_command)
{
    for (int i = 0; i < 8; ++i) {
        if ( !SAFE_CALL( world->current, insertChar, '\x20' ) )
            return FALSE;
    }
    return TRUE;
}

DEFINE_CMD(se_backspace_command)
{
    //g_assert( key.ascii == XK_BackSpace );    
    return TRUE;
}

DEFINE_CMD(se_delete_forward_command)
{
    //TODO: check universalArg for count or backward delete
    return SAFE_CALL( world->current, deleteChars, 1 );
}

DEFINE_CMD(se_universal_arg_command)
{
    se_debug("universal args");
    args->flags |= SE_UNIVERSAL_ARG;
    return FALSE;
}

DEFINE_CMD(se_second_dispatch_command)
{
    se_debug("");
    args->flags |= SE_PREFIX_ARG;
    args->keyseq.keys[args->keyseq.len++] = key;
    return FALSE;
}

DEFINE_CMD(se_editor_quit_command)
{
    se_debug("");
    world->quit( world );
    return TRUE;
}

DEFINE_CMD(se_kbd_quit_command)
{
    se_debug("");
    se_command_args_clear( args );
    return TRUE;
}

/* DEFINE_CMD(se_forward_char_command) */
/* { */
/*     se_debug(""); */
/*     if ( se_forward_char_hook_command ) { */
/*         return se_forward_char_hook_command( world, args, key ); */
/*     } */
/*     return SAFE_CALL( world->current, forwardChar, 1 ); */
/* } */

CMD_TYPE se_forward_char_command = NULL;

DEFINE_CMD(se_backward_char_command)
{
    se_debug("");
    return SAFE_CALL( world->current, forwardChar, -1 );
}

DEFINE_CMD(se_backward_line_command)
{
    se_debug("");
    return SAFE_CALL( world->current, forwardLine, -1 );
}

DEFINE_CMD(se_forward_line_command)
{
    se_debug("");
    return SAFE_CALL( world->current, forwardLine, 1 );
}

DEFINE_CMD(se_move_beginning_of_line_command)
{
    se_debug("");
    return SAFE_CALL( world->current, beginingOfLine );
}

DEFINE_CMD(se_move_end_of_line_command)
{
    se_debug("");
    return SAFE_CALL( world->current, endOfLine );
}

DEFINE_CMD(se_previous_buffer_command)
{
    se_debug("");
    return world->bufferSetPrevious( world ) != NULL;
}

DEFINE_CMD(se_next_buffer_command)
{
    se_debug("");
    return world->bufferSetNext( world ) != NULL;
}

