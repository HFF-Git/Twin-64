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

    T64Processor *proc = 
        new T64Processor(   glb -> system,
                            3,
                            T64_PO_NIL,
                            T64_CPU_T_NIL,
                            T64_TT_FA_64S,
                            T64_TT_FA_64S,
                            T64_CT_2W_128S_4L,
                            T64_CT_4W_64S_8L,
                            T64_IO_HPA_MEM_START + ( 3 * T64_PAGE_SIZE_BYTES ),
                            T64_PAGE_SIZE_BYTES,
                            0,
                            0 );

    T64Memory *mem1 = 
        new T64Memory(  glb -> system,
                        1, 
                        T64_IO_HPA_MEM_START + ( 1 * T64_PAGE_SIZE_BYTES ),
                        T64_PAGE_SIZE_BYTES,
                        0,
                        64 * T64_PAGE_SIZE_BYTES );

    
    T64Memory *mem2 = 
        new T64Memory(  glb -> system,
                        2, 
                        T64_IO_HPA_MEM_START + ( 2 * T64_PAGE_SIZE_BYTES ),
                        T64_PAGE_SIZE_BYTES,
                        64 * T64_PAGE_SIZE_BYTES,
                        64 * T64_PAGE_SIZE_BYTES );

    
    if ( glb -> system -> addToModuleMap( mem1 ) != 0 ) {

        glb -> console -> writeChars( "Config Error: Module MEM 1\n" );
        return( -1 );
    }
    
    if ( glb -> system -> addToModuleMap( mem2 ) != 0 ) {

        glb -> console -> writeChars( "Config Error: Module MEM 2\n" );
        return( -1 );
    }

    if ( glb -> system -> addToModuleMap( proc ) != 0 ) {

        glb -> console -> writeChars( "Config Error: Module PROC\n" );
        return( -1 );
    }

    glb -> winDisplay   -> startWinDisplay( );
    
    return 0;
}
