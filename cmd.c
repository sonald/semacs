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

#define DEFINE_CMD(cmd_name) int cmd_name(se_world* world, se_command_args* args, se_key key)

DEFINE_CMD(se_self_silent_command)
{
    se_debug( "unprintable char, ignore" );
    return TRUE;
}

DEFINE_CMD(se_self_insert_command)
{
    // g_assert( isprint(key.ascii) ); // can not assert, cause any key can bind to this cmd
    world->current->insertChar( world->current, key.ascii );
    return TRUE;
}

DEFINE_CMD(se_newline_command)
{
    world->current->insertChar( world->current, '\n' );
    return TRUE;
}

DEFINE_CMD(se_indent_for_tab_command)
{
    //g_assert( key.ascii == XK_Tab );    
    return TRUE;
}

DEFINE_CMD(se_backspace_command)
{
    //g_assert( key.ascii == XK_BackSpace );    
    return TRUE;
}

DEFINE_CMD(se_delete_forward_command)
{
    return TRUE;
}

DEFINE_CMD(se_universal_arg_command)
{
    return FALSE;
}

DEFINE_CMD(se_second_dispatch_command)
{
    return FALSE;
}

DEFINE_CMD(se_editor_quit_command)
{
    
    return TRUE;
}

DEFINE_CMD(se_kbd_quit_command)
{
    return TRUE;
}

#undef DEFINE_CMD
