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

#define SE_UNUSED __attribute__ ((unused))

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "semacs"
#endif

#ifdef __cplusplus

#define se_error(fmt, ...) g_error( "[%s]: "#fmt, __FILE__, ##__VA_ARGS__ )
#define se_msg(fmt, ...)   g_message( "[%s]: "#fmt, __FILE__, ##__VA_ARGS__ )
#define se_warn(fmt, ...)  g_warning( "[%s]: "#fmt, __FILE__, ##__VA_ARGS__ )
#define se_debug(fmt, ...) g_debug( "[%s]: "#fmt, __FILE__, ##__VA_ARGS__ )

#else

#define se_error(fmt, ...) g_error( "[%s]: "#fmt, __func__, ##__VA_ARGS__ )
#define se_msg(fmt, ...)   g_message( fmt, ##__VA_ARGS__ )
#define se_warn(fmt, ...)  g_warning( "[%s]: "#fmt, __func__, ##__VA_ARGS__ )
#define se_debug(fmt, ...) g_debug( "[%s]: "#fmt, __func__, ##__VA_ARGS__ )

#endif

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

#define ARRAY_LEN(arr)  (sizeof(arr) / sizeof(arr[0]))

#define IN_SET(elem, set_range) ({              \
            gboolean is_in = FALSE;             \
            switch ( elem ) {                   \
            case  set_range:                    \
                is_in = TRUE;                   \
                break;                          \
            default:                            \
                is_in = FALSE;                  \
            }                                   \
            is_in;                              \
        })

#define SAFE_CALL(cls, memfunc, ...)  ({                    \
            g_assert( cls );                                \
            g_assert( cls->memfunc );                       \
            int _ret = cls->memfunc( cls, ##__VA_ARGS__ );  \
            _ret;                                           \
        })

#define BETWEEN(mem, min, max)  ((mem >= (min)) && (mem <= (max)))
#define BETWEEN_EX(mem, min, max)  ((mem > (min)) && (mem < (max)))

#define SE_DYNAMIC __attribute__((weak))
#define SE_ATTR_RELOC  __attribute__(( __section__(".editorcmds") ))
#endif // ~end of file

