///------------------------------------------------------------------------------------------------------------
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
#include "T64-Types.h"
#include "T64-Cpu.h"

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
namespace {
    
    static inline bool isAligned( T64Word adr, int align ) {
        
        return (( adr & ( align - 1 )) == 0 );
    }

    static inline bool isInRange( T64Word adr, T64Word low, T64Word high ) {
        
        return (( adr >= low ) && ( adr <= high ));
    }

    static inline T64Word roundup( T64Word arg, int round ) {
        
        if ( round == 0 ) return ( arg );
        return ((( arg + round - 1 ) / round ) * round );
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

};


//************************************************************************************************************
//************************************************************************************************************
//
// TLB
//
//************************************************************************************************************
//************************************************************************************************************
//
//
//------------------------------------------------------------------------------------------------------------
T64Tlb::T64Tlb( int size ) {
    
    this -> size = size;
    this -> map  = (T64TlbEntry *) calloc( size, sizeof( T64TlbEntry ));
    reset( );
}

void T64Tlb::reset( ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        map[ i ].valid = false;
    }
}

T64TlbEntry *T64Tlb::lookupTlb( T64Word vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) return( ptr );
    }
    
    return( nullptr );
}

void T64Tlb::purgeTlb( T64Word vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) ptr -> valid = false;
    }
}

T64TlbEntry *T64Tlb::getTlbEntry( int index ) {
    
    if ( isInRange( index, 0, size - 1 ))   return( &map[ index ] );
    else                                    return( nullptr );
}

void T64Tlb::setTlbEntry( int index, T64TlbEntry *entry ) {
    
    if ( isInRange( index, 0, size - 1 )) {
        
        map[ index ] = *entry;
    }
}

//************************************************************************************************************
//************************************************************************************************************
//
// CPU
//
//************************************************************************************************************
//************************************************************************************************************
//
//
//------------------------------------------------------------------------------------------------------------
T64Cpu::T64Cpu( T64PhysMem *mem, T64IoMem *io ) {
    
    this -> mem = mem;
    this -> io  = io;
    this -> tlb = new T64Tlb( 64 );
    reset( );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::reset( ) {
    
    for ( int i = 0; i < MAX_CREGS; i++ ) ctlRegFile[ i ] = 0;
    for ( int i = 0; i < MAX_GREGS; i++ ) genRegFile[ i ] = 0;
    pswReg     = 0;
    instrReg   = 0;
    resvReg    = 0;
    
    tlb -> reset( );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::getGeneralReg( int index ) {
    
    if ( index == 0 )   return( 0 );
    else                return( genRegFile[ index % MAX_GREGS ] );
}

void T64Cpu::setGeneralReg( int index, T64Word val ) {
    
    if ( index != 0 ) genRegFile[ index % MAX_GREGS ] = val;
}

T64Word T64Cpu::getControlReg( int index ) {
    
    return( ctlRegFile[ index % MAX_CREGS ] );
}

void T64Cpu::setControlReg( int index, T64Word val ) {
    
    ctlRegFile[ index % MAX_CREGS ] = val;
}

T64Word T64Cpu::getPswReg( ) {
    
    return( pswReg );
}

void T64Cpu::setPswReg( T64Word val ) {
    
    pswReg = val;
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::translateAdr( T64Word vAdr, T64Word *pAdr ) {
    
    if ( extractField( vAdr, 32, 20 ) == 0 ) {  // physical address range ?
        
        if ( ! extractBit( pswReg, 0 )) { // privileged mode ?
            
            throw T64Trap( 0 ); // priv violation
        }
        
        *pAdr = vAdr;
    }
    else {
        
        T64TlbEntry *tlbPtr = tlb -> lookupTlb( vAdr );
        
        if ( tlbPtr == nullptr ) {
            
            throw T64Trap( 0 ); // dara tlb trap
        }
        
        // ??? access check .....
        
        // protection check ...
        
        if ( extractBit( pswReg, 0 )) {
            
            uint32_t pId = tlbPtr -> protectId;
            
            if ( ! (( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ))) {
                
                throw T64Trap( 0 ); // dara protection trap
            }
        }
        
        // ??? what else to check ?
        
        *pAdr = tlbPtr -> pAdr;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::dataRead( T64Word vAdr, int len ) {
   
    try {
        
        T64Word pAdr = 0;
        
        translateAdr( vAdr , &pAdr );
        
        // ??? fix to get real size ...
        
        if ( isInRange( pAdr, 0, 1024 )) return ( mem -> readMem( pAdr, len ));
        else                             return ( io -> readMem( pAdr, len ));
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::dataWrite( T64Word vAdr, T64Word val, int len ) {
    
    // ??? How to distinguish between meem and io ?
    
    try {
        
        T64Word pAdr = 0;
        
        translateAdr( vAdr , &pAdr );
        
        if ( isInRange( pAdr, 0, 1024 )) mem -> writeMem( pAdr, val, len );
        else                             io -> writeMem( pAdr, val, len );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::fetchInstr( ) {
    
    try {
        
        T64Word pAdr = 0;
        
        translateAdr( extractField( pswReg, 63, 52 ) , &pAdr );
        
        // ??? fix to get real size ... what does fetch from IO space mean ?
        
        if ( isInRange( pAdr, 0, 1024 )) instrReg = mem -> readMem( pAdr, 32 );
        else                             instrReg = io -> readMem( pAdr, 32 );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::executeInstr( ) {
    
    try {
        
        int opCode      = (int) extractField( instrReg, 26, 6 );
        int regRIndx    = (int) extractField( instrReg, 22, 4 );
        int regBIndx    = (int) extractField( instrReg, 15, 4 );
        int regAIndx    = (int) extractField( instrReg, 9, 4 );
        
        switch ( opCode ) {
                
            case OP_ALU_NOP: {
                
            } break;
                
            case OP_ALU_AND: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                               valA = getGeneralReg( regAIndx );
                
                if ( extractBit( instrReg, 20 ))   valA = ~ valA;
                
                T64Word valR = valB | valA;
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_OR: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                T64Word valR = valB | valA;
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_XOR: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                               valA = getGeneralReg( regAIndx );
                
                T64Word valR = valB ^ valA;
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_ADD: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                               valA = getGeneralReg( regAIndx );
                
                if ( ! willAddOverflow( valB, valA )) setGeneralReg( regRIndx, valB + valA );
                else throw T64Trap( OVERFLOW_TRAP );
                
            } break;
                
            case OP_ALU_SUB: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                               valA = getGeneralReg( regAIndx );
                
                if ( ! willSubOverflow( valB, valA )) setGeneralReg( regRIndx, valB - valA );
                else throw T64Trap( OVERFLOW_TRAP );
                
            } break;
                
            case OP_ALU_CMP: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                               valA = getGeneralReg( regAIndx );
                
                // ....
                
            } break;
                
            case OP_ALU_EXTR: {
                
                int     pos  = 0;
                int     len  = 0;
                T64Word valB = getGeneralReg( regBIndx );
                
                // ??? check for amt ...
                // ??? get signe extension bit ...
                
                
            } break;
                
            case OP_ALU_DEP: {
                
                T64Word valA = getGeneralReg( regAIndx );
                T64Word valB = getGeneralReg( regBIndx );
                
            } break;
                
            case OP_ALU_DSR: {
                
                T64Word valA = getGeneralReg( regAIndx );
                T64Word valB = getGeneralReg( regBIndx );
                
            } break;
                
            case OP_ALU_CHK: {
                
                T64Word valB = getGeneralReg( regBIndx );
                
            } break;
                
            case OP_MEM_LD: {
                
            } break;
                
            case OP_MEM_ST: {
                
            } break;
                
            case OP_MEM_LDR: {
                
                // translate address
                // set reserved Reg: bit 63 -> true, rest physical address ( of cache line ? )
                
            } break;
                
            case OP_MEM_STC: {
                
                // translatre address
                // check resvered flag. if set store val, reset reserved reg. return 0. else return 1.
                
            } break;
                
            case OP_MEM_AND: {
                
                T64Word valB = getGeneralReg( regBIndx );
                T64Word valA = 0;
                
                // ... now fetch the memory data...
                
                T64Word valR = 0;
                
            } break;
                
            case OP_MEM_OR: {
                
            } break;
                
            case OP_MEM_XOR: {
                
            } break;
                
            case OP_MEM_ADD: {
                
            } break;
                
            case OP_MEM_SUB: {
                
            } break;
                
            case OP_MEM_CMP: {
                
            } break;
                
            case OP_BR_LDI: {
                
            } break;
                
            case OP_BR_ADDIL: {
                
            } break;
                
            case OP_BR_LDO: {
                
            } break;
                
            case OP_BR_B: {
                
            } break;
                
            case OP_BR_BR: {
                
            } break;
                
            case OP_BR_BV: {
                
            } break;
                
            case OP_BR_CBR: {
                
            } break;
                
            case OP_BR_TBR: {
                
            } break;
                
            case OP_BR_MBR: {
                
            } break;
                
            case OP_SYS_MR: {
                
            } break;
                
            case OP_SYS_MST: {
                
            } break;
                
            case OP_SYS_LPA: {
                
            } break;
                
            case OP_SYS_PRB: {
                
            } break;
                
            case OP_SYS_ITLB: {
                
            } break;
                
            case OP_SYS_DTLB: {
                
            } break;
                
            case OP_SYS_PCA: {
                
            } break;
                
            case OP_SYS_DIAG: {
                
            } break;
                
            case OP_SYS_BRK: {
                
            } break;
                
            case OP_SYS_RFI: {
                
            } break;
                
            default: ;
        }
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::step( ) {
    
    try {
        
        fetchInstr( );
        executeInstr( );
    }
    
    catch ( const T64Trap t ) {
        
    }
}
