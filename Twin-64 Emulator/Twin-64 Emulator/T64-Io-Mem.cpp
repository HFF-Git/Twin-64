//
//  T64-Io-Mem.cpp
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//

#include <stdio.h>

#include "T64-Types.h"
#include "T64-Cpu.h"
#include "T64-Io-Mem.h"

namespace {
    
    static inline bool isAligned( T64Word adr, int align ) {
        
        return (( adr & ( align - 1 )) == 0 );
    }

    static inline bool isInRange( T64Word adr, T64Word low, T64Word high ) {
        
        return(( adr >= low ) && ( adr <= high ));
    }

    static inline T64Word roundup( T64Word arg ) {
        
        return( arg ); // for now ...
    }

    static inline T64Word extractBit( T64Word arg, int bitpos ) {
        
        return ( arg >> bitpos ) & 1;
    }

    static inline T64Word extractField( T64Word arg, int bitpos, int len) {
        
        return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
    }

    static inline T64Word extractSignedField( T64Word arg, int bitpos, int len ) {
        
        T64Word field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
        
        if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
        else             return ( field );
        
    }

    static inline T64Word depositField( T64Word word, int bitpos, int len, T64Word value) {
        
        T64Word mask = (( 1ULL << len ) - 1 ) << bitpos;
        return ( word & ~mask ) | (( value << bitpos ) & mask );
    }

    bool willAddOverflow( T64Word a, T64Word b ) {
        
        if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
        if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
        return false;
    }

    bool willSubOverflow( T64Word a, T64Word b ) {
        
        if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
        if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
        return false;
    }

    bool willShiftLftOverflow( T64Word a, int shift ) {
        
        if (( shift < 0 ) || ( shift >= 64 )) return true;
        if ( a == 0 ) return false;
        
        T64Word max = INT64_MAX >> shift;
        T64Word min = INT64_MIN >> shift;
        
        return (( a > max ) || ( a < min ));
    }
}


//************************************************************************************************************
//************************************************************************************************************
//
// IO Memory
//
//************************************************************************************************************
//************************************************************************************************************
T64IoMem::T64IoMem( T64Word size ) {
    
}

void T64IoMem::reset( ) {
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64Word T64IoMem::readMem( T64Word adr, int len, bool signExtend ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        return ( 0 );
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 3 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else if ( len == 64 ) {

        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64IoMem::writeMem( T64Word adr, T64Word arg, int len ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 3 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
       
    }
    else if ( len == 64 ) {

        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}
