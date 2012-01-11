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

#include "qview.h"
#include "util.h"
#include "xview.h"

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <libgen.h>

#include <dlfcn.h>

static char *progname = NULL;

void usage()
{
    std::cerr << "USAGE: " << progname << " [options]" << std::endl
              << "OPTIONS: " << std::endl
              << "  -qview, -Q \t use qt based views\n"
              << "  -xview, -X \t use raw x based views\n"
              << "  -im \t\t specify im to use\n";
    exit(1);
}

bool loadMode()
{
    dlerror();
    void *hnd = dlopen( "./libccmode.so", RTLD_NOW | RTLD_GLOBAL );
    if ( !hnd ) {
        se_debug( "load mode failed" );
        return false;
    }
    se_forward_char_command = (CMD_TYPE)dlsym( hnd, "se_forward_char_hook_command" );
    char* err_msg = dlerror();
    if ( err_msg ) {
        se_debug( err_msg );
        dlclose( hnd );
        return false;
    }
    
    //dlclose( hnd );    
    return true;
}

int parse_args(int argc, char *argv[])
{
    progname = strdup( basename(argv[0]) );
    
    struct option long_opts[] = {
        {"qview", no_argument, 0, 0 },
        {"xview", no_argument, 0, 0 },
        {"im", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
    };
    int long_idx = 0;

    gboolean flag_qview = FALSE;
    char *im = NULL;
    
    while ( 1 ) {
        int c = getopt_long( argc, argv, "QXh", long_opts, &long_idx );
        if ( c == -1 )
            break;
        
        switch( c ) {
        case 0:
            se_debug( "long option: %s with arg: %s", long_opts[long_idx].name, optarg );
            switch ( long_idx ) {
            case 0: flag_qview = TRUE; break;
            case 1: flag_qview = FALSE; break;
            case 2:
                if ( !optarg )
                    usage();
                im = strdup( optarg ); break;
                
            default: usage(); break;                
            } 
            break;

        case 'Q':
            se_debug( "QView" );
            flag_qview = TRUE;
            break;

        case 'X':
            se_debug( "XView" );
            break;

        case 'h':
            usage();
            break;
            
        default:
            break;
        }
    }

    if ( optind < argc ) {

    }

    loadMode();
    
    if  ( flag_qview ) {
        QApplication app(argc, argv);
        app.setQuitOnLastWindowClosed( true );
        
        SEView view;
        view.setFocus();
        view.show();
        return app.exec();
        
    } else
        return main_loop( argc, argv );
    return TRUE;
}

int main(int argc, char *argv[])
{
    return parse_args( argc, argv );
}



