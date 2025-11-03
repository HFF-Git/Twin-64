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

#include "T64-Util.h"
#include "T64-Common.h"
#include "T64-System.h"

//----------------------------------------------------------------------------------------
// T64 Memory module. A physical memory module is an array of pages. Each module 
// covers a range of physical memory and reacts to read and write bus operations.
// Although the memory module does not participate in cache coherency operations,
// it uses the same read / write interfaces.
//
//----------------------------------------------------------------------------------------
struct T64Memory : T64Module {
    
public:
    
    T64Memory( T64System    *sys, 
               int          modNum, 
               T64Word      hpaAdr, 
               int          hpaLen,
               T64Word      spaAdr,
               int          spaLen);
    
    void        reset( );
    void        step( );

    bool        busEvtReadUncached( int srcModNum,
                                 T64Word pAdr, 
                                 uint8_t *data, 
                                 int len );

    bool        busEvtWriteUncached( int srcModNum,
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int len );

    bool        busEvtReadSharedBlock( int srcModNum,
                                    T64Word pAdr,
                                    uint8_t *data, 
                                    int len );

    bool        busEvtReadPrivateBlock( int srcModNum, 
                                     T64Word pAdr, 
                                     uint8_t *data, 
                                     int len );

    bool        busEvtWriteBlock(  int srcModNum,
                                T64Word pAdr, 
                                uint8_t *data, 
                                int len );

    void        setSpaReadOnly( bool arg );

    // ??? routines to load/save memory ?

private:

    bool        read( T64Word adr, uint8_t *data, int len );
    bool        write( T64Word adr, uint8_t *data, int len );
    
    T64System   *sys        = nullptr;
    uint8_t     *memData    = nullptr;
    bool        spaReadOnly = false;
    
};

#endif // T64-Memory.h