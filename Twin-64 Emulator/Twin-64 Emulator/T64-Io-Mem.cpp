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

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
namespace {

static inline T64Word roundup( T64Word arg ) {
    
    return( arg ); // for now ...
}

static inline bool isInRange( T64Word adr, T64Word low, T64Word high ) {
    
    return(( adr >= low ) && ( adr <= high ));
}

static inline bool isAligned( T64Word adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline T64Word extractField( T64Word arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline T64Word extractSignedField( T64Word arg, int bitpos, int len ) {
    
    T64Word field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
}

} // namespace


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
T64Word T64IoMem::readIoMem( T64Word adr, int len, bool signExtend ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( ! isAligned( adr, len )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    return ( 0 );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64IoMem::writeIoMem( T64Word adr, T64Word arg, int len ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( ! isAligned( adr, len )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
   
}
