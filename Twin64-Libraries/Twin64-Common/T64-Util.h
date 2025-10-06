//----------------------------------------------------------------------------------------
//
//  Twin64 - A 64-bit CPU Simulator - Common utility functions
//
//----------------------------------------------------------------------------------------
// ...
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Simulator - Common Declarations
// Copyright (C) 2022 - 2025 Helmut Fieres
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
#ifndef T64_Common_Util_h
#define T64_Common_Util_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "T64-Common.h"

//----------------------------------------------------------------------------------------
// Byte order conversion functions. They are different on Mac and Windows.
//
// ??? do we need to have an option for a big endian machine, where we would do
// nothing ?
//----------------------------------------------------------------------------------------
#if __APPLE__

#include <libkern/OSByteOrder.h>
inline uint16_t toBigEndian16(uint16_t val) { return OSSwapHostToBigInt16(val); }
inline uint32_t toBigEndian32(uint32_t val) { return OSSwapHostToBigInt32(val); }
inline uint64_t toBigEndian64(uint64_t val) { return OSSwapHostToBigInt64(val); }

#else

#include <stdlib.h> // for _byteswap_*
#include <intrin.h>
inline uint16_t toBigEndian16(uint16_t val) { return _byteswap_ushort(val); }
inline uint32_t toBigEndian32(uint32_t val) { return _byteswap_ulong(val); }
inline uint64_t toBigEndian64(uint64_t val) { return _byteswap_uint64(val); }

#endif

//----------------------------------------------------------------------------------------
// Helper functions.
//
//----------------------------------------------------------------------------------------
inline T64Word roundup( T64Word arg, int round ) {
    
    if ( round == 0 ) return ( arg );
    return ((( arg + round - 1 ) / round ) * round );
}

inline bool isAligned( T64Word adr, int align ) {

    if (( align == 1 ) || ( align = 2 ) || ( align == 4 ) || ( align = 8 )) 
        return (( adr & ( align - 1 )) == 0 );
    else 
        return( false );
}

inline bool isInRange( T64Word adr, T64Word low, T64Word high ) {
    
    return (( adr >= low ) && ( adr <= high ));
}

//----------------------------------------------------------------------------------------
// Helper function to check a bit range value in the instruction.
//
//----------------------------------------------------------------------------------------
inline bool isInRangeForInstrBitField( int val, int bitLen ) {
    
    int min = - ( 1 << (( bitLen - 1 ) % 64 ));
    int max = ( 1 << (( bitLen - 1 ) % 64 )) - 1;
    return (( val <= max ) && ( val >= min ));
}

inline bool isInRangeForInstrBitFieldU( uint32_t val, int bitLen ) {
    
    uint32_t max = (( 1 << ( bitLen % 64 )) - 1 );
    return ( val <= max );
}

//----------------------------------------------------------------------------------------
// Instruction field routines.
//
//----------------------------------------------------------------------------------------
inline int extractInstrBit( T64Instr arg, int bitpos ) {
    
    if ( bitpos > 31 ) return ( 0 );
    return ( arg >> bitpos ) & 1;
}

inline int extractInstrField( T64Instr arg, int bitpos, int len ) {
    
    if ( bitpos > 31 ) return ( 0 );
    if ( bitpos + len > 31 ) return ( 0 );
    return ( arg >> bitpos ) & (( 1L << len ) - 1 );
}

inline int extractInstrSignedField( T64Instr arg, int bitpos, int len ) {
    
    T64Word field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 32 )  return ( field << ( 32 - len )) >> ( 32 - len );
    else             return ( field );
}

inline int extractInstrOpGroup( T64Instr instr ) {
    
    return ( extractInstrField( instr, 30, 2 ));
}

inline int extractInstrOpCode( T64Instr instr ) {
    
    return ( extractInstrField( instr, 26, 4 ));
}

inline int extractInstrOptField( T64Instr instr ) {
    
    return ( extractInstrField( instr, 19, 3 ));
}

inline int extractInstrRegR( T64Instr instr ) {
    
    return ( extractInstrField( instr, 22, 4 ));
}

inline int extractInstrRegB( T64Instr instr ) {
    
    return ( extractInstrField( instr, 15, 4 ));
}

inline int extractInstrRegA( T64Instr instr ) {
    
    return ( extractInstrField( instr, 9, 4 ));
}

inline int extractInstrDwField( T64Instr instr) {
    
    return ( extractInstrField( instr, 13, 2 ));
}

inline int extractInstrImm13( T64Instr instr ) {
    
    return ( extractInstrSignedField( instr, 0, 13 ));
}

inline int extractInstrScaledImm13( T64Instr instr ) {
    
    int val = extractInstrImm13( instr ) << extractInstrDwField( instr );
    return ( val );
}

inline int extractInstrImm15( T64Instr instr ) {
    
    return ( extractInstrSignedField( instr, 0, 15 ));
}

inline int extractInstrImm19( T64Instr instr ) {
    
    return ( extractInstrSignedField( instr, 0, 19 ));
}

inline int extractInstrImm20( T64Instr instr ) {

    return ( extractInstrField( instr, 0, 20 ));
}

//----------------------------------------------------------------------------------------
// Helper function for depositing value in the instruction.
//
//----------------------------------------------------------------------------------------
inline void depositInstrField( T64Instr *instr, int bitpos, int len, T64Word value ) {
    
    uint32_t mask = (( 1 << len ) - 1 ) << bitpos;
    *instr = (( *instr & ~mask ) | (( value << bitpos ) & mask ));
}

inline void depositInstrBit( T64Instr *instr, int bitpos, bool value ) {
    
    uint32_t mask = 1 << bitpos;
    *instr = (( *instr & ~mask ) | (( value << bitpos ) & mask ));
}

inline void depositInstrRegR( T64Instr *instr, uint32_t regId ) {
    
    depositInstrField( instr, 22, 4, regId );
}

inline void depositInstrRegB( T64Instr *instr, uint32_t regId ) {
    
   depositInstrField( instr, 15, 4, regId );
}

inline void depositInstrRegA( T64Instr *instr, uint32_t regId ) {
    
    depositInstrField( instr, 9, 4, regId );
}

//----------------------------------------------------------------------------------------
// General extract, deposit and shift functions.
//
//----------------------------------------------------------------------------------------
inline T64Word extractBit64( T64Word arg, int bitpos ) {
    
    if ( bitpos > 63 ) return ( 0 );
    return ( arg >> bitpos ) & 1;
}

inline T64Word extractField64( T64Word arg, int bitpos, int len ) {
    
    if ( bitpos > 63 ) return ( 0 );
    if ( bitpos + len > 64 ) return ( 0 );
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

inline T64Word extractSignedField64( T64Word arg, int bitpos, int len ) {
    
    T64Word field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
}

inline T64Word depositField( T64Word word, int bitpos, int len, T64Word value ) {
    
    T64Word mask = (( 1ULL << len ) - 1 ) << bitpos;
    return ( word & ~mask ) | (( value << bitpos ) & mask );
}

inline T64Word shiftRight128( T64Word hi, T64Word lo, int shift ) {
    
    if      ( shift == 0 ) return ( lo );
    else if (( shift > 0 ) && ( shift < 64 )) {
        
        return (((uint64_t) hi << (64 - shift)) | ((uint64_t) lo >> shift));
    }
    else return ( lo );
}

//----------------------------------------------------------------------------------------
// Signed 64-bit numeric operations and overflow check.
//
//----------------------------------------------------------------------------------------
inline bool willAddOverflow( T64Word a, T64Word b ) {
    
    if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
    if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
    return false;
}

inline bool willSubOverflow( T64Word a, T64Word b ) {
    
    if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
    if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
    return false;
}

inline bool willMultOverflow( T64Word a, T64Word b ) {

    if (( a == 0 ) ||( b == 0 )) return ( false );

    if (( a == INT64_MIN ) && ( b == -1 )) return ( true );
    if (( b == INT64_MIN ) && ( a == -1 )) return ( true );

    if ( a > 0 ) {

        if ( b > 0 ) return ( a > INT64_MAX / b );
        else         return ( b < INT64_MIN / a );
    }
    else {

        if ( b > 0 ) return ( a < INT64_MIN / b );
        else         return ( a < INT64_MAX / b );
    }
}

inline bool willDivOverflow( T64Word a, T64Word b ) {

    if ( b == 0 ) return ( true );

    if (( a == INT64_MIN ) && ( b == -1 )) return ( true );

    return ( false );
}

inline bool willShiftLeftOverflow( T64Word val, int shift ) {
    
    if (( shift < 0 ) || ( shift >= 63 ))   return ( true );
    if ( shift == 0 )                       return ( false );
    
    T64Word shifted     = val << shift;
    T64Word recovered   = shifted >> shift;
    
    return ( recovered != val );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
inline T64Word vAdrSeg( T64Word vAdr ) {

    return( extractField64( vAdr, 32, 20 ));
}

inline T64Word vAdrSegOfs( T64Word vAdr ) {

   return( extractField64( vAdr, 0, 32 ));
}

inline T64Word vAdrPageOfs( T64Word vAdr ) {

   return( extractField64( vAdr, 0, 12 ));
}

//----------------------------------------------------------------------------------------
// Address arithmetic.
//
//----------------------------------------------------------------------------------------
inline T64Word addAdrOfs( T64Word adr, T64Word ofs ) {
    
    uint32_t newOfs = (uint32_t)( adr >> 32U ) + (uint32_t)ofs;
    return (( adr & 0xFFFFFFFF00000000 ) | newOfs );
}


//----------------------------------------------------------------------------------------
// Address range check.
//
//----------------------------------------------------------------------------------------
inline bool isInIoAdrRange( T64Word adr ) {

    return(( adr >= T64_IO_MEM_START ) && ( adr <= T64_IO_MEM_LIMIT ));
}

//----------------------------------------------------------------------------------------
// Little helpers.
//
//----------------------------------------------------------------------------------------
void upshiftStr( char *str );
void addChar( char *buf, int size, char ch );


#endif // T64_Common_Util_h