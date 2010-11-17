
/**
 * BaseView  - 
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

#include <glib.h>
#include "view.h"
#include <locale.h>

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



const char* XEventTypeString(int type) 
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


