
/**
 * Environment - 
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

#include "env.h"

//TODO: send a ClientMessage For Shutdown or what
void se_env_quit(se_env* env)
{
    env->exitLoop = 1;
    se_env_release( env );
    exit(0);
}

se_env* se_env_init()
{
    se_env* env = (se_env*)g_malloc0( sizeof(se_env) );
    if ( !env )
        se_error( "can not allocate se_env" );

    env->display = XOpenDisplay( NULL );
    if ( env->display == NULL )
        se_error( "cannot connect to X server" );

    env->numScreens = ScreenCount( env->display );
    env->screen = DefaultScreen( env->display );
    env->root = RootWindow( env->display, env->screen );
    env->visual = DefaultVisual(env->display, env->screen);
    env->colormap = DefaultColormap(env->display, env->screen);

    env->world = se_world_create();

    SE_UNUSED char *mono_font_name = "Monaco-10:weight=normal:antilias=true";
    /* char *mono_font_name = "Monaco:pixelsize=15:foundry=unknown:weight=normal" */
    /*     ":width=normal:spacing=mono:scalable=true"; */
    SE_UNUSED char *sans_font_name = "Droid Sans Fallback-11:width=normal"
        ":scalable=true";

    if ( !se_env_change_font( env, mono_font_name ) ) {
        env->xftFont = NULL;
    }

    env->xim = XOpenIM( env->display, NULL, NULL, NULL );
    if ( env->xim == NULL ) {
        se_error(" open im failed" );
        exit(1);
    }
    
    return env;
}

void se_env_release(se_env* env)
{
    if ( env ) {
        XCloseIM( env->xim );
        XCloseDisplay( env->display );
        g_free( env );
    }
}

gboolean se_env_change_font( se_env* env, const char* name )
{
    XftPattern* favourate_pat = XftNameParse( name );
    XftResult result;
    XftPattern* matched_pat = XftFontMatch(
        env->display, env->screen, favourate_pat, &result );
    XftFont *font = XftFontOpenPattern( env->display, matched_pat );
    if ( !font ) {
        se_warn( "can not open font" );
        XftPatternDestroy( favourate_pat );
        XftPatternDestroy( matched_pat );
        return FALSE;
    }
    env->xftFont = font;

    XGlyphInfo metric;
    XftTextExtentsUtf8( env->display, env->xftFont, (const XftChar8*)"W", 1, &metric);
    env->glyphMaxWidth = metric.width;
    env->glyphAscent = metric.y;

    // just use hints from xftFont directly now, not by calculation like above
    env->glyphMaxHeight = font->height;    
//    env->glyphMaxWidth = font->max_advance_width;
//    env->glyphAscent = font->ascent;
    
    se_debug( "ascent:%d, height:%d, max width:%d",
              env->glyphAscent, env->glyphMaxHeight, env->glyphMaxWidth );

    XftPatternDestroy( favourate_pat );
    XftPatternDestroy( matched_pat );

    return TRUE;
}

