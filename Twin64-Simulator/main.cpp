//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Simulator
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Simulator
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under the 
// terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should
//  have received a copy of the GNU General Public License along with this program.  
// If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-System.h"
#include "T64-SimDeclarations.h"
#include <elfio/elfio.hpp>

//----------------------------------------------------------------------------------------
// Here we go.
//
//----------------------------------------------------------------------------------------
int main( int argc, const char * argv[] ) {
   
    SimGlobals *glb     = new SimGlobals( );

    glb -> console      = new SimConsoleIO( );
    glb -> env          = new SimEnv( glb, 100 );
    glb -> winDisplay   = new SimWinDisplay( glb );
    glb -> system       = new T64System( );  

    glb -> console      -> initConsoleIO( );
    glb -> env          -> setupPredefined( );
    glb -> winDisplay   -> setupWinDisplay( argc, argv );
    glb -> winDisplay   -> startWinDisplay( );

    return 0;
}
