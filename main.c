/**
 * Main Entry - 
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
#include "editor.h"
#include "xview.h"

int main_loop(int argc, char *argv[])
{
    se_env* env = se_env_init();
    se_text_viewer* viewer = se_text_viewer_create( env );

    int mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
        | KeyPressMask | KeyReleaseMask | SubstructureNotifyMask;
    XSelectInput( env->display, viewer->view, mask );

    viewer->repaint( viewer );    
    viewer->show( viewer );
    
    while ( 1 ) { // sync the first expose event
        XEvent ev;
        XNextEvent( env->display, &ev );
        if ( ev.xany.window == viewer->view && ev.type == Expose ) {
            viewer->redisplay( viewer );
            break;
        }            
    }
    
    while( !env->exitLoop ) {
        XEvent ev;
        XNextEvent( env->display, &ev );
        if ( ev.xany.window != viewer->view ) {
            se_msg( "skip event does not forward to view" );
            continue;
        }
        
        switch( ev.type ) {
        case Expose:
            if ( ev.xexpose.count > 0 )
                break;

            viewer->redisplay( viewer );
            break;

        case KeyPress:
        case KeyRelease:
            viewer->key_handler( viewer, &ev );
            break;

        case ConfigureNotify:
        case MapNotify:
            se_debug( "confiugration changed" );
            
            viewer->configure_change_handler( viewer, &ev );
            break;
            
        default:
            break;
        }
    }

    se_debug( "exit loop" );
    se_env_release( env );
    return 0;
}

int main(int argc, char *argv[])
{
    return main_loop(argc, argv);
}

