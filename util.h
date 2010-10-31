/**
 * Utils - 
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

#ifndef _semacs_util_h
#define _semacs_util_h

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <glib.h>

#define DEF_CLS(_name) typedef struct _name _name


#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "semacs"
#endif

#define se_error(fmt, ...) g_error( fmt, ##__VA_ARGS__ )
#define se_msg(fmt, ...)   g_message( fmt, ##__VA_ARGS__ )
#define se_warn(fmt, ...)  g_warning( fmt, ##__VA_ARGS__ )
#define se_debug(fmt, ...) g_debug( fmt, ##__VA_ARGS__ )

#ifndef MIN
#define MIN(a, b) ({                            \
            typeof(a) _tmp1 = a;                \
            typeof(b) _tmp2 = b;                \
            typeof(a) _tmp = _tmp1;             \
            if ( _tmp > _tmp2 )                 \
                _tmp = _tmp2;                   \
            _tmp;                               \
        })
#endif

#ifndef MAX
#define MAX(a, b) ({                            \
            typeof(a) _tmp1 = a;                \
            typeof(b) _tmp2 = b;                \
            typeof(a) _tmp = _tmp1;             \
            if ( _tmp < _tmp2 )                 \
                _tmp = _tmp2;                   \
            _tmp;                               \
        })
#endif

#endif

