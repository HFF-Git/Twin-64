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
// IO Memory
//
//************************************************************************************************************
//************************************************************************************************************
T64IoMem::T64IoMem( int64_t size ) {
    
}

void T64IoMem::reset( ) {
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
int64_t T64IoMem::readMem( int64_t adr, int len, bool signExtend ) {
    
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
void T64IoMem::writeMem( int64_t adr, int64_t arg, int len ) {
    
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
