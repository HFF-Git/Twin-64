//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Sketch
//
//------------------------------------------------------------------------------------------------------------
// This module contains all of the mothods for the different windows that the simlulator supports. The
// exception is the command window, which is in a separate file. A window generally consist of a banner line,
// shown in inverse video and a nuber of body lines.
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Sketch
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation, either version 3 of the License,
// or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details. You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "T64-Types.h"
#include "T64-Cpu.h"
#include "T64-Phys-Mem.h"

namespace {
    
    
    static inline bool isAligned( int64_t adr, int align ) {
        
        return (( adr & ( align - 1 )) == 0 );
    }

    static inline bool isInRange( int64_t adr, int64_t low, int64_t high ) {
        
        return(( adr >= low ) && ( adr <= high ));
    }

    static inline int64_t roundup( uint64_t arg ) {
        
        return( arg ); // for now ...
    }

    static inline int64_t extractBit( int64_t arg, int bitpos ) {
        
        return ( arg >> bitpos ) & 1;
    }

    static inline int64_t extractField( int64_t arg, int bitpos, int len) {
        
        return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
    }

    static inline int64_t extractSignedField( int64_t arg, int bitpos, int len ) {
        
        int64_t field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
        
        if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
        else             return ( field );
        
    }

    static inline int64_t depositField( int64_t word, int bitpos, int len, int64_t value) {
        
        int64_t mask = (( 1ULL << len ) - 1 ) << bitpos;
        return ( word & ~mask ) | (( value << bitpos ) & mask );
    }

    bool willAddOverflow( int64_t a, int64_t b ) {
        
        if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
        if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
        return false;
    }

    bool willSubOverflow( int64_t a, int64_t b ) {
        
        if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
        if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
        return false;
    }

    bool willShiftLftOverflow( int64_t a, int shift ) {
        
        if (( shift < 0 ) || ( shift >= 64 )) return true;
        if ( a == 0 ) return false;
        
        int64_t max = INT64_MAX >> shift;
        int64_t min = INT64_MIN >> shift;
        
        return (( a > max ) || ( a < min ));
    }
}


//************************************************************************************************************
//************************************************************************************************************
//
// Physical memory
//
//************************************************************************************************************
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64PhysMem::T64PhysMem( int64_t size ) {
 
    this -> size = roundup( size );
    this -> mem  = (uint8_t *) calloc( this -> size, sizeof( uint8_t ));
}

void T64PhysMem::reset( ) {
    
    if ( mem != nullptr ) free( mem );
    this -> mem  = (uint8_t *) calloc( size, sizeof( uint8_t ));
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64PhysMem::readMem( int64_t adr, int len, bool signExtend ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        uint64_t val= mem[ adr ];
        if ( signExtend ) val = extractSignedField( val, 63, 8 );
        return( val );
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2))  throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        uint64_t val = 0;
        val |= (int16_t) mem[ adr ] << 8;
        val |= (int16_t) mem[ adr + 1 ];
        if ( signExtend ) val = extractSignedField( val, 63, 16 );
        return( val );
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        uint64_t val = 0;
        val |= (int32_t) mem[ adr]     << 24;
        val |= (int32_t) mem[ adr + 1 ] << 16;
        val |= (int32_t) mem[ adr + 2 ] << 8;
        val |= (int32_t) mem[ adr + 3 ];
        if ( signExtend ) val = extractSignedField( val, 63, 32 );
        return( val );
        
    }
    else if ( len == 64 ) {
        
        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        int64_t val = 0;
        val |= (int64_t) mem[ adr ]     << 56;
        val |= (int64_t) mem[ adr + 1 ] << 48;
        val |= (int64_t) mem[ adr + 2 ] << 40;
        val |= (int64_t) mem[ adr + 3 ] << 32;
        val |= (int64_t) mem[ adr + 4 ] << 24;
        val |= (int64_t) mem[ adr + 5 ] << 16;
        val |= (int64_t) mem[ adr + 6 ] << 8;
        val |= (int64_t) mem[ adr + 7 ];
        return ( val );
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64PhysMem::writeMem( int64_t adr, uint64_t arg, int len ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        mem[ adr ] = arg & 0xFF;
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr ]      = ( arg >> 8  ) & 0xFF;
        mem[ adr + 1 ]  = ( arg       ) & 0xFF;
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr ]     = ( arg >> 24 ) & 0xFF;
        mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 3 ] = ( arg       ) & 0xFF;
        
    }
    else if ( len == 64 ) {
        
        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr]     = ( arg >> 56 ) & 0xFF;
        mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
        mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
        mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
        mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
        mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

#if 0
//------------------------------------------------------------------------------------------------------------
//
// ??? may go away...
//------------------------------------------------------------------------------------------------------------
int8_t T64PhysMem::getMem8( int64_t adr ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    return mem[ adr ];
}

void T64PhysMem::setMem8( int64_t adr, int8_t arg ) {
    
    if (adr >= size) throw T64Trap( PHYS_MEM_ADR_TRAP );
    mem[ adr ] = arg;
}

int16_t T64PhysMem::getMem16( int64_t adr ) {
    
    if ( adr + 1 >= size )      throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 2))  throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int16_t val = 0;
    val |= (int16_t) mem[ adr ] << 8;
    val |= (int16_t) mem[ adr + 1 ];
    return val;
}

void T64PhysMem::setMem16( int64_t adr, int16_t arg ) {
    
    if ( adr + 1 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 2)) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr ]      = ( arg >> 8  ) & 0xFF;
    mem[ adr + 1 ]  = ( arg       ) & 0xFF;
}

int32_t T64PhysMem::getMem32( int64_t adr ) {
    
    if ( adr + 3 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int32_t val = 0;
    val |= (int32_t) mem[ adr]     << 24;
    val |= (int32_t) mem[ adr + 1 ] << 16;
    val |= (int32_t) mem[ adr + 2 ] << 8;
    val |= (int32_t) mem[ adr + 3 ];
    return val;
}

void T64PhysMem::setMem32( int64_t adr, int32_t arg ) {
    
    if ( adr + 3 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr ]     = ( arg >> 24 ) & 0xFF;
    mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 3 ] = ( arg       ) & 0xFF;
}

int64_t T64PhysMem::getMem64( int64_t adr ) {
    
    if ( adr + 7 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int64_t val = 0;
    val |= (int64_t) mem[ adr ]     << 56;
    val |= (int64_t) mem[ adr + 1 ] << 48;
    val |= (int64_t) mem[ adr + 2 ] << 40;
    val |= (int64_t) mem[ adr + 3 ] << 32;
    val |= (int64_t) mem[ adr + 4 ] << 24;
    val |= (int64_t) mem[ adr + 5 ] << 16;
    val |= (int64_t) mem[ adr + 6 ] << 8;
    val |= (int64_t) mem[ adr + 7 ];
    return val;
}

void T64PhysMem::setMem64( int64_t adr, int64_t arg ) {
    
    if ( adr + 7 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr]     = ( arg >> 56 ) & 0xFF;
    mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
    mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
    mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
    mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
    mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
}
#endif
