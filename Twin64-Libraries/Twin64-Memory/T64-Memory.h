//----------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Physical memory
//
//----------------------------------------------------------------------------------------
// This module contains ...
//
//----------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Physical memory
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
#ifndef T64_Memory_h
#define T64_Memory_h

#include "T64-Common.h"
#include "T64-Module.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
struct T64Memory : T64Module {
    
public:
    
    T64Memory( T64Word size );
    
    void        reset( ) override;
    T64Word     read( T64Word pAdr, int len, bool signExtend = false );
    void        write( T64Word pAdr, T64Word arg, int len );

    void        event( T64ModuleEvent evt ) override;

    // ??? separate routines for monitor display ?
    // int getWord( T64Word adr, uint32_t *data );
    // int putWord( T64Word adr, uint32_t data );
   
private:
    
    T64Word    size = 0;
    uint8_t    *mem = nullptr;
    
};

#endif // T64-Memory.h