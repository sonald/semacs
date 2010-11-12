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

#include <locale.h>

#include "util.h"
#include "editor.h"
#include "xview.h"


static const char* XEventTypeString(int type) 
{
#define prologue(msg) return msg
    
    switch (type) {
    case KeyPress:
        prologue ("KeyPress");
        break;
    case KeyRelease:
        prologue ("KeyRelease");
        break;
    case ButtonPress:
        prologue ("ButtonPress");
        break;
    case ButtonRelease:
        prologue ("ButtonRelease");
        break;
    case MotionNotify:
        prologue ("MotionNotify");
        break;
    case EnterNotify:
        prologue ("EnterNotify");
        break;
    case LeaveNotify:
        prologue ("LeaveNotify");
        break;
    case FocusIn:
        prologue ("FocusIn");
        break;
    case FocusOut:
        prologue ("FocusOut");
        break;
    case KeymapNotify:
        prologue ("KeymapNotify");
        break;
    case Expose:
        prologue ("Expose");
        break;
    case GraphicsExpose:
        prologue ("GraphicsExpose");
        break;
    case NoExpose:
        prologue ("NoExpose");
        break;
    case VisibilityNotify:
        prologue ("VisibilityNotify");
        break;
    case CreateNotify:
        prologue ("CreateNotify");
        break;
    case DestroyNotify:
        prologue ("DestroyNotify");
        break;
    case UnmapNotify:
        prologue ("UnmapNotify");
        break;
    case MapNotify:
        prologue ("MapNotify");
        break;
    case MapRequest:
        prologue ("MapRequest");
        break;
    case ReparentNotify:
        prologue ("ReparentNotify");
        break;
    case ConfigureNotify:
        prologue ("ConfigureNotify");
        break;
    case ConfigureRequest:
        prologue ("ConfigureRequest");
        break;
    case GravityNotify:
        prologue ("GravityNotify");
        break;
    case ResizeRequest:
        prologue ("ResizeRequest");
        break;
    case CirculateNotify:
        prologue ("CirculateNotify");
        break;
    case CirculateRequest:
        prologue ("CirculateRequest");
        break;
    case PropertyNotify:
        prologue ("PropertyNotify");
        break;
    case SelectionClear:
        prologue ("SelectionClear");
        break;
    case SelectionRequest:
        prologue ("SelectionRequest");
        break;
    case SelectionNotify:
        prologue ("SelectionNotify");
        break;
    case ColormapNotify:
        prologue ("ColormapNotify");
        break;
    case ClientMessage:
        prologue ("ClientMessage");
        break;
    case MappingNotify:
        prologue ("MappingNotify");
    }

    return "UNKNOWN";
#undef prologue
}

int main_loop(int argc, char *argv[])
{
    se_env* env = se_env_init();
    se_text_viewer* viewer = se_text_viewer_create( env );
    
    long im_event_mask;
    XGetICValues( viewer->xic, XNFilterEvents, &im_event_mask, NULL );

    int mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
        | KeyPressMask | KeyReleaseMask | SubstructureNotifyMask
        | FocusChangeMask
        | im_event_mask;
    XSelectInput( env->display, viewer->view, mask );
    XSetICFocus( viewer->xic );
    
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
        if ( XFilterEvent( &ev, None ) == True ) {
            se_debug("IM filtered event: %s", XEventTypeString(ev.type) );
            continue;
        }
        
        if ( ev.xany.window != viewer->view ) {
            se_msg( "skip event does not forward to view" );
            continue;
        }
        
        switch( ev.type ) {
        case Expose:
            se_debug( "Expose" );
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

    XUnsetICFocus( viewer->xic );
    XDestroyIC( viewer->xic );
//    XCloseIM( env->xim );
    se_env_release( env );
    return 0;
}

void setup_language()
{
    char* locale = setlocale( LC_ALL, "" );
    se_debug( "set locale: %s", locale );
    if ( XSupportsLocale() == False ) {
        se_error( "locale is not supported" );
    }
    char* xmodifiers = XSetLocaleModifiers(NULL);
    se_debug("xmodifiers: %s", xmodifiers );
}

int main(int argc, char *argv[])
{
    setup_language();
   return main_loop(argc, argv);
}




