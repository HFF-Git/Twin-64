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
#include "T64-Util.h"

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
//
//
//----------------------------------------------------------------------------------------
T64Memory::T64Memory( T64Word size ) {
    
    this -> size = roundup( size, 16 ); // ??? what is the roundup level ?
    this -> mem  = (uint8_t *) calloc( this -> size, sizeof( uint8_t ));
}

void T64Memory::reset( ) {

    T64Module::reset( );
    
    if ( mem != nullptr ) free( mem );
    this -> mem  = (uint8_t *) calloc( size, sizeof( uint8_t ));
}

//----------------------------------------------------------------------------------------
//
// 
//----------------------------------------------------------------------------------------
T64Word T64Memory::read( T64Word adr, int len, bool signExtend ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP, adr );
    
    if ( len == 1 ) {
        
        T64Word val= mem[ adr ];
        if ( signExtend ) val = extractSignedField64( val, 63, 8 );
        return( val );
    }
    else if ( len == 2 ) {
        
      //  if ( ! isAligned( adr, 2))  throw T64Trap( ALIGNMENT_TRAP );
        
        T64Word val = 0;
        val |= (int16_t) mem[ adr ] << 8;
        val |= (int16_t) mem[ adr + 1 ];
        if ( signExtend ) val = extractSignedField64( val, 63, 16 );
        return( val );
    }
    else if ( len == 4 ) {
        
       // if ( ! isAligned( adr, 4 )) throw T64Trap( ALIGNMENT_TRAP );
        
        T64Word val = 0;
        val |= (int32_t) mem[ adr]     << 24;
        val |= (int32_t) mem[ adr + 1 ] << 16;
        val |= (int32_t) mem[ adr + 2 ] << 8;
        val |= (int32_t) mem[ adr + 3 ];
        if ( signExtend ) val = extractSignedField64( val, 63, 32 );
        return( val );
        
    }
    else if ( len == 8 ) {
        
      //  if ( ! isAligned( adr, 8 )) throw T64Trap( ALIGNMENT_TRAP );
        
        T64Word val = 0;
        val |= (T64Word) mem[ adr ]     << 56;
        val |= (T64Word) mem[ adr + 1 ] << 48;
        val |= (T64Word) mem[ adr + 2 ] << 40;
        val |= (T64Word) mem[ adr + 3 ] << 32;
        val |= (T64Word) mem[ adr + 4 ] << 24;
        val |= (T64Word) mem[ adr + 5 ] << 16;
        val |= (T64Word) mem[ adr + 6 ] << 8;
        val |= (T64Word) mem[ adr + 7 ];
        return ( val );
    }
  //  else throw T64Trap( ALIGNMENT_TRAP );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Memory::write( T64Word adr, T64Word arg, int len ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP, adr );
    
    if ( len == 1 ) {
        
        mem[ adr ] = arg & 0xFF;
    }
    else if ( len == 2 ) {
        
      //  if ( ! isAligned( adr, 2 )) throw T64Trap( ALIGNMENT_TRAP );
        
        mem[ adr ]      = ( arg >> 8  ) & 0xFF;
        mem[ adr + 1 ]  = ( arg       ) & 0xFF;
    }
    else if ( len == 4 ) {
        
      //  if ( ! isAligned( adr, 4 )) throw T64Trap( ALIGNMENT_TRAP );
        
        mem[ adr ]     = ( arg >> 24 ) & 0xFF;
        mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 3 ] = ( arg       ) & 0xFF;
        
    }
    else if ( len == 8 ) {
        
      //  if ( ! isAligned( adr, 8 )) throw T64Trap( ALIGNMENT_TRAP );
        
        mem[ adr]     = ( arg >> 56 ) & 0xFF;
        mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
        mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
        mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
        mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
        mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
    }
  //   else throw T64Trap( ALIGNMENT_TRAP );
}

// ??? separate routines for monitor display ?
// int getWord( T64Word adr, uint32_t *data );
// int putWord( T64Word adr, uint32_t data );



void T64Memory::event( T64ModuleEvent evt ) {

    
}


