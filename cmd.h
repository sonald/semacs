/**
 * basic commands  - 
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

#ifndef _semacs_cmd_h
#define _semacs_cmd_h

#include "util.h"
#include "key.h"

enum {
    SE_UNIVERSAL_ARG = 0x01,
    SE_EXIT_ARG      = 0x02,
    SE_PREFIX_ARG    = 0x04,
    SE_IM_ARG        = 0x08,  // activate composedStr
};

DEF_CLS(se_command_args);
struct se_command_args
{
    int flags;
    int prefix_arg;
    /**
     * SE_PREFIX_ARG set:
     *
     * when a keybinding comes from a key seq, member keyseq records all keys
     * current has been typed, and it certainly a prefix of a keybinding's seq.
     */
    se_key_seq keyseq;
    GString *composedStr; // this is valid when use a IM to input
    GString *universalArg; 
};

struct se_world;
// return TRUE means exiting command dispatch loop
typedef int (*se_key_command_t)(struct se_world* world, se_command_args* args, se_key key);

#define DECLARE_CMD(cmd_name) int cmd_name(struct se_world*, se_command_args*, se_key)

extern DECLARE_CMD(se_self_insert_command);
extern DECLARE_CMD(se_self_silent_command);

extern DECLARE_CMD(se_newline_command); 
extern DECLARE_CMD(se_newline_and_indent_command);
extern DECLARE_CMD(se_indent_for_tab_command);
extern DECLARE_CMD(se_backspace_command);
extern DECLARE_CMD(se_delete_forward_command);

extern DECLARE_CMD(se_second_dispatch_command);
extern DECLARE_CMD(se_universal_arg_command);
extern DECLARE_CMD(se_editor_quit_command);
extern DECLARE_CMD(se_kbd_quit_command);

// point moving
extern DECLARE_CMD(se_forward_char_command);
extern DECLARE_CMD(se_backward_char_command);
extern DECLARE_CMD(se_forward_line_command);
extern DECLARE_CMD(se_backward_line_command);
extern DECLARE_CMD(se_move_end_of_line_command);
extern DECLARE_CMD(se_move_beginning_of_line_command);

extern DECLARE_CMD(se_previous_buffer_command);
extern DECLARE_CMD(se_next_buffer_command);

#undef DECLARE_CMD
#endif

