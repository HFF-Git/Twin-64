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
#include "T64-Memory.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {

} // namespace

//****************************************************************************************
//****************************************************************************************
//
// Physical memory
//
//----------------------------------------------------------------------------------------
// Physical memory. We have a rather simple module for memory. It is a range of bytes.
// The SPA range describes where in physical memory this memory is allocated. It is
// possible to have several memory modules, each mapping a different range of physical 
// memory. The read and write function merely copy data from and to memory. The 
// address must however be aligned to the length of the data to fetch.
//
//----------------------------------------------------------------------------------------
T64Memory::T64Memory( T64System     *sys, 
                      int           modNum, 
                      T64Word       hpaAdr, 
                      int           hpaLen,
                      T64Word       spaAdr,
                      int           spaLen ) : 

                      T64Module(    MT_MEM, 
                                    modNum,
                                    hpaAdr, 
                                    hpaLen,
                                    spaAdr,
                                    spaLen
                                 ) {
    
    this -> sys     = sys;
    this -> hpaLen  = roundup( hpaLen, T64_PAGE_SIZE_BYTES );
    this -> spaLen  = roundup( spaLen, T64_PAGE_SIZE_BYTES );
    this -> memData = (uint8_t *) calloc( spaLen, sizeof( uint8_t ));
}

//----------------------------------------------------------------------------------------
// Reset the memory module. We clear out the physical memory range.
//
//----------------------------------------------------------------------------------------
void T64Memory::reset( ) {

    if ( memData != nullptr ) free( memData );
    this -> memData  = (uint8_t *) calloc( spaLen, sizeof( uint8_t ));
}

//----------------------------------------------------------------------------------------
// Each module has a step function. Ours does nothing.
// 
//----------------------------------------------------------------------------------------
void T64Memory::step( ) { }

//----------------------------------------------------------------------------------------
// Read function. We read a block of data from memory. The address the physical address
// and we compute the offset on our SPA range. The address needs to be aligned with 
// length parameter.
//
//----------------------------------------------------------------------------------------
bool T64Memory::read( T64Word adr, uint8_t *data, int len ) {

    if ( adr + len >= spaLen ) return( false );
    if ( ! isAligned( adr, len )) return( false );

    memcpy( data, &memData[ adr - spaAdr ], len );
    return( true );
}

//----------------------------------------------------------------------------------------
// Write function. We write a block of data to memory. The address the physical address
// and we compute the offset on our SPA range. The address needs to be aligned with 
// length parameter.
//
//----------------------------------------------------------------------------------------
bool T64Memory::write( T64Word adr, uint8_t *data, int len ) {

    if ( adr + len >= spaLen ) return( false );
    if ( ! isAligned( adr, len )) return( false );

    memcpy( &memData[ adr - spaAdr ], data, len );
    return( true );
}

//----------------------------------------------------------------------------------------
// Bus operations. We listen to all of them and if the physical addresses matches our
// address range and we are not the source module, the request is handled. Since we
// do not have a cache, the request handling is very simple.
//
//----------------------------------------------------------------------------------------
bool T64Memory::busReadUncached( int     srcModNum,
                                 T64Word pAdr, 
                                 uint8_t *data, 
                                 int len ) {

    if ( moduleNum != srcModNum ) return( read( pAdr, data, len ));
    else return( true );
}

bool T64Memory::busWriteUncached( int     srcModNum,
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int len ) {

    if ( moduleNum != srcModNum ) return( write( pAdr, data, len ));
    else return( true );
}

bool T64Memory::busReadShared( int     srcModNum,
                               T64Word pAdr,
                               uint8_t *data, 
                               int len ) {

    if ( moduleNum != srcModNum ) return( read( pAdr, data, len ));
    else return( true );
}

bool T64Memory::busReadPrivate( int     srcModNum, 
                                T64Word pAdr, 
                                uint8_t *data, 
                                int len ) {

    if ( moduleNum != srcModNum ) return( read( pAdr, data, len ));
    else return( true );
}

bool T64Memory::busWrite( int     srcModNum,
                          T64Word pAdr, 
                          uint8_t *data, 
                          int len ) {

    if ( moduleNum != srcModNum ) return( write( pAdr, data, len ));
    else return( true );
}