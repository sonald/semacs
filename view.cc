
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




