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

static inline T64Word shiftRight128( T64Word hi, T64Word lo, int shift ) {
    
    if      ( shift == 0 ) return ( lo );
    else if (( shift > 0 ) && ( shift < 64 )) {
        
        return(((uint64_t) hi << (64 - shift)) | ((uint64_t) lo >> shift));
    }
    else return( lo );
}

static inline bool willAddOverflow( T64Word a, T64Word b ) {
    
    if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
    if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
    return false;
}

bool willSubOverflow( T64Word a, T64Word b ) {
    
    if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
    if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
    return false;
}

bool willShiftLftOverflow( T64Word val, int shift ) {
    
    if (( shift < 0 ) || ( shift >= 63 ))   return ( true );
    if ( shift == 0 )                       return ( false );
    
    T64Word shifted     = val << shift;
    T64Word recovered   = shifted >> shift;
    
    return ( recovered != val );
}

T64Word addAdrOfs( T64Word adr, T64Word ofs ) {
    
    uint32_t newOfs = (uint32_t)( adr >> 32U ) + (uint32_t)ofs;
    return(( adr & 0xFFFFFFFF00000000 ) | newOfs );
}


// ??? not sure I need it in this file ...
//------------------------------------------------------------------------------------------------------------
//
// Format hex with '_' every 4, 8, 12, or 16 digits, no "0x", left-padded with zeros if desired
//------------------------------------------------------------------------------------------------------------
void formatHexVal( T64Word value, char *buf, int digits = 16 ) {
    
    if ( digits < 1 ) digits    = 1;
    if ( digits > 16 ) digits   = 16;
    
    int     shiftAmount     = 16 - digits;
    int     tmpBufIndex     = 0;
    char    tmpBuf[ 20 ];
    
    for ( int i = shiftAmount; i < 16; i++ ) {
        
        int digit = ( value >> ( i * 4 )) & 0xF;
        tmpBuf[ tmpBufIndex++ ] = "0123456789abcdef"[ digit ];
    }
    
    int out = 0;
    
    for ( int i = 0; i < tmpBufIndex; ++i) {
        
        if (( i > 0 ) && ( i % 4 == 0 )) buf[ out++ ] = '_';
        buf[ out++ ] = tmpBuf[ i ];
    }
    
    buf[ out ] = '\0';
}

// ??? not sure I need it in this file ...
//------------------------------------------------------------------------------------------------------------
//
// Format decimal with '_' every 3 digits (from right to left)
//------------------------------------------------------------------------------------------------------------
void formatDecVal( T64Word value, char *buf ) {
    
    char temp[32]; // enough to hold 20-digit uint64 + separators
    int len = 0;
    
    // Build reversed string with separators
    do {
        if (len > 0 && len % 3 == 0) {
            temp[len++] = '_';
        }
        temp[len++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    // Reverse to get final string
    for (int i = 0; i < len; ++i) {
        buf[i] = temp[len - 1 - i];
    }
    buf[len] = '\0';
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

int T64Tlb::insertTlb( T64Word vAdr, T64Word info ) {
    
    // ??? to do ...
    
    return( 0 );
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
T64Cpu::T64Cpu( T64PhysMem *physMem, T64IoMem *ioMem ) {
    
    this -> physMem = physMem;
    this -> ioMem   = ioMem;
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
    
    pswReg      = 0;
    instrReg    = 0;
    resvReg     = 0;
    
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
// Get the general register values based on the instruction field.
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::getRegR( uint32_t instr ) {
    
    return( getGeneralReg((int) extractField( instr, 22, 4 )));
}

T64Word T64Cpu::getRegB( uint32_t instr ) {
    
    return( getGeneralReg((int) extractField( instr, 15, 4 )));
}

T64Word T64Cpu::getRegA( uint32_t instr ) {
    
    return( getGeneralReg((int) extractField( instr, 9, 4 )));
}

void T64Cpu::setRegR( uint32_t instr, T64Word val ) {
    
    setGeneralReg((int) extractField( instr, 22, 4 ), val );
}

//------------------------------------------------------------------------------------------------------------
// Get IMM-x values based on the instruction field.
//
//------------------------------------------------------------------------------------------------------------

T64Word T64Cpu::getImm13( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 13 ));
}

T64Word T64Cpu::getImm15( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 15 ));
}

T64Word T64Cpu::getImm19( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 19 ));
}

T64Word T64Cpu::getImm20U( uint32_t instr ) {
    
    return( extractField( instr, 0, 20 ));
}

//------------------------------------------------------------------------------------------------------------
// Translate the virtual address to a physical address.
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::translateAdr( T64Word vAdr ) {
    
    if ( extractField( vAdr, 32, 20 ) == 0 ) {  // physical address range ?
        
        if ( ! extractBit( pswReg, 0 )) throw ( T64Trap( PRIV_VIOLATION_TRAP ));
        
        return( vAdr );
    }
    else {
        
        T64TlbEntry *tlbPtr = tlb -> lookupTlb( vAdr );
        
        if ( tlbPtr == nullptr ) throw ( T64Trap( TLB_ACCESS_TRAP ));
        
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
                
                throw ( T64Trap( PROTECTION_TRAP ));
            }
        }
        
        // ??? what else to check ?
        
        return( tlbPtr -> pAdr );
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::dataRead( T64Word vAdr, int len ) {
    
    try {
        
        T64Word pAdr = translateAdr( vAdr );
        
        // ??? fix to get real size ...
        
        if      ( isInRange( pAdr, 0, 1024 ))   return ( physMem -> readMem( pAdr, len ));
        else if ( isInRange( pAdr, 0, 1024 ))   return ( ioMem -> readIoMem( pAdr, len ));
        else                                    return( 0 );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
// Read memory data based using RegB and the IMM-13 offset to form the address.
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::dataReadRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = (int) extractField( instr, 13, 2 );
    T64Word     ofs     = getImm13( instr ) << dw;
    int         len     = 1U << dw;
    
    return( dataRead( addAdrOfs( adr, ofs ), len ));
}

//------------------------------------------------------------------------------------------------------------
// Read memory data based using RegB and the RegX offset to form the address.
//
//------------------------------------------------------------------------------------------------------------
T64Word T64Cpu::dataReadRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = (int) extractField( instr, 13, 2 );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1U << dw;
    
    adr = addAdrOfs( adr, ofs );
    
    if (( dw > 1 ) && ( !isAligned( adr, len ))) throw ( 0 );
    
    return( dataRead( adr, len ));
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::dataWrite( T64Word vAdr, T64Word val, int len ) {
    
    // ??? How to distinguish between meem and io ?
    
    try {
        
        T64Word pAdr = translateAdr( vAdr );
        
        // ??? fix to get real size ...
        
        if      ( isInRange( pAdr, 0, 1024 ))   physMem -> writeMem( pAdr, val, len );
        else if ( isInRange( pAdr, 0, 1024 ))   ioMem -> writeIoMem( pAdr, val, len );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
// Write data to memory based using RegB and the IMM-13 offset to form the address.
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::dataWriteRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = (int) extractField( instr, 13, 2 );
    T64Word     ofs     = getImm13( instr ) << dw;
    int         len     = 1U << dw;
    T64Word     val     = getRegR( instr );
    
    dataWrite( addAdrOfs( adr, ofs ), val, len );
}

//------------------------------------------------------------------------------------------------------------
// Write data to memory based using RegB and the RegX offset to form the address.
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu:: dataWriteRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = (int) extractField( instr, 13, 2 );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1U << dw;
    T64Word     val     = getRegR( instr );
    
    adr = addAdrOfs( adr, ofs );
    
    if (( dw > 1 ) && ( !isAligned( adr, len ))) throw ( 0 );
    
    dataWrite( addAdrOfs( adr, ofs ), val, len );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::fetchInstr( ) {
    
    try {
        
        T64Word pAdr = translateAdr( extractField( pswReg, 63, 52 ));
        
        // ??? fix to get real size ... what does fetch from IO space mean ?
        
        if      ( isInRange( pAdr, 0, 1024 ))   instrReg = (uint32_t ) physMem -> readMem( pAdr, 4 );
        else if ( isInRange( pAdr, 0, 1024 ))   instrReg = (uint32_t ) ioMem   -> readIoMem( pAdr, 4 );
        else                                    throw ( 0 );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
// Execute an instruction. This is the key rooutine of the emulator. Essentially a big case statement. Each
// instruction is encoded based on the instruction group and the opcode family. Inside each such cases, the
// option 1 field ( bits 19 .. 22 ) further qualifies an instruction.
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::executeInstr( ) {
    
    try {
        
        int opCode = ((int) extractField((uint32_t) instrReg, 26, 6 ));
        
        switch ( opCode ) {
                
            case ( OPC_GRP_ALU * 16 + OPC_ADD ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getRegA( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getImm13( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ADD ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsImm13( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsRegX( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_SUB ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getRegA( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getImm13( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_SUB ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsImm13( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsRegX( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                        else setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_AND ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 & val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 & val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_AND ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 & val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 & val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_OR ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 | val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 | val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_OR ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res  = val1 | val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 | val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_XOR ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_XOR ): {
                
                if ( extractBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
                
                T64Word val1 = getRegB( instrReg );
                T64Word val2 = 0;
                T64Word res  = 0;
                
                if ( extractBit( instrReg, 19 )) val2 = getImm13( instrReg );
                else                             val2 = getRegA( instrReg );
                
                switch ( extractField( instrReg, 20, 2 )) {
                        
                    case 0: res = ( val1 == val2 ) ? 1 : 0; break;
                    case 1: res = ( val1 <  val2 ) ? 1 : 0; break;
                    case 2: res = ( val1 != val2 ) ? 1 : 0; break;
                    case 3: res = ( val1 <= val2 ) ? 1 : 0; break;
                }
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_CMP ): {
                
                T64Word val1 = getRegB( instrReg );
                T64Word val2 = 0;
                T64Word res  = 0;
                
                if ( extractBit( instrReg, 19 )) val2 = dataReadRegBOfsRegX( instrReg );
                else                             val2 = dataReadRegBOfsImm13( instrReg );
                
                switch ( extractField( instrReg, 20, 2 )) {
                        
                    case 0: res = ( val1 == val2 ) ? 1 : 0; break;
                    case 1: res = ( val1 <  val2 ) ? 1 : 0; break;
                    case 2: res = ( val1 != val2 ) ? 1 : 0; break;
                    case 3: res = ( val1 <= val2 ) ? 1 : 0; break;
                }
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0: { // EXTR
                        
                        T64Word val = getRegB( instrReg );
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = (int) extractField( instrReg, 0, 6 );
                        
                        if ( extractBit( instrReg, 13 ))   pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                               pos = (int) extractField( instrReg, 6, 6 );
                        
                        if ( extractBit( instrReg, 12 ))   res = extractSignedField( val, (int) pos, len );
                        else                               res = extractField( val, (int) pos, len );
                        
                        setRegR( instrReg, res );
                        
                    } break;
                        
                    case 1: { // DEP
                        
                        T64Word val1 = 0;
                        T64Word val2 = 0;
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = (int) extractField( instrReg, 0, 6 );
                        
                        if ( extractBit( instrReg, 13 ))   pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                            pos = (int) extractField( instrReg, 6, 6 );
                        
                        if ( extractBit( instrReg, 12 ))   val1 = 0;
                        else                            val1 = getRegR( instrReg );
                        
                        if ( extractBit( instrReg, 14 ))   val2 = extractField( instrReg, 15, 4 );
                        else                            val2 = getRegB( instrReg );
                        
                        res = depositField( val1, pos, len , val2 );
                        setRegR( instrReg, res );
                        pswReg = addAdrOfs( pswReg, 4 );
                        
                    } break;
                        
                    case 3: { // DSR
                        
                        T64Word val1    = getRegB( instrReg );
                        T64Word val2    = getRegA( instrReg );
                        int     shamt   = 0;
                        T64Word res     = 0;
                        
                        if ( extractBit( instrReg, 13 ))   shamt = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                            shamt = (int) extractField( instrReg, 6, 6 );
                        
                        res = shiftRight128( val1, val2, shamt );
                        setRegR( instrReg, res );
                        pswReg = addAdrOfs( pswReg, 4 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
                
                T64Word val1  = getRegR( instrReg );
                T64Word val2  = 0;
                T64Word res   = 0;
                int     shamt = (int) extractField( instrReg, 20, 2 );
                
                if ( extractBit( instrReg, 14 ))   val2 = getImm13( instrReg );
                else                            val2 = getRegB( instrReg );
                
                if ( extractBit( instrReg, 19 )) { // SHRxA
                    
                    res = val1 >> shamt;
                }
                else { // SHLxA
                    
                    if ( willShiftLftOverflow( val1, shamt )) throw ( T64Trap( OVERFLOW_TRAP ));
                    res = val1 << shamt;
                }
                
                if ( willAddOverflow( res, val2 )) throw ( T64Trap( OVERFLOW_TRAP ));
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
                
                T64Word val = getImm20U( instrReg );
                T64Word res = 0;
                
                switch ( extractField( instrReg, 20, 2 )) {
                        
                    case 0: res = val;          break;
                    case 1: res = val << 12;    break;
                    case 2: res = val << 32;    break;
                    case 3: res = val << 52;    break;
                }
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_LDO ): {
                
                T64Word base    = getRegB( instrReg );
                T64Word ofs     = getImm15( instrReg );
                T64Word res     = addAdrOfs( base, ofs );
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LD ): {
                
                T64Word res = 0;
                
                if      ( extractField( instrReg, 19, 3 ) == 0 )   res = dataReadRegBOfsImm13( instrReg );
                else if ( extractField( instrReg, 19, 3 ) == 1 )   res = dataReadRegBOfsRegX( instrReg );
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LDR ): {
                
                T64Word res = 0;
                
                if ( extractField( instrReg, 19, 3 ) != 0 ) throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                res = dataReadRegBOfsImm13( instrReg );
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ST ): {
                
                if      ( extractField( instrReg, 19, 3 ) == 0 )   dataWriteRegBOfsImm13( instrReg );
                else if ( extractField( instrReg, 19, 3 ) == 1 )   dataWriteRegBOfsRegX( instrReg );
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_STC ): {
                
                if ( extractField( instrReg, 19, 3 ) == 1 ) dataWriteRegBOfsImm13( instrReg );
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_B ): {
                
                T64Word ofs     = getImm19( instrReg ) << 2;
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( pswReg, ofs );
                
                if ( extractBit( instrReg, 19 )) {  // gateway
                    
                    pswReg = newIA;
                    setRegR( instrReg, rl );
                }
                else { // regular branch
                    
                    pswReg = newIA;
                    setRegR( instrReg, rl );
                }
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BR ): {
                
                T64Word ofs     = getRegB( instrReg ) << 2;
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( pswReg, ofs );
                
                if ( extractField( instrReg, 19, 3 ) != 0 ) throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                if ( ! isAligned( newIA, 4 )) throw( T64Trap( ALIGNMENT_TRAP ));
                
                pswReg = newIA;
                setRegR( instrReg, rl );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BV ): {
                
                T64Word base    = getRegB( instrReg );
                T64Word ofs     = getRegA( instrReg );
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( base, ofs );
                
                if ( extractField( instrReg, 19, 3 ) != 0 ) throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                if ( ! isAligned( newIA, 4 )) throw( T64Trap( ALIGNMENT_TRAP ));
                
                pswReg = newIA;
                setRegR( instrReg, rl );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BB ): {
                
                T64Word newIA   = addAdrOfs( pswReg, getImm13( instrReg ));
                bool    testVal = extractBit( instrReg, 19 );
                bool    testBit = 0;
                int     pos     = 0;
                
                if ( extractBit( instrReg, 20 ))  pos = 0; // use SAR
                else                                    pos = (int) extractField( instrReg, 13, 6 );
                
                testBit = extractBit( instrReg, pos );
                
                if ( testVal ^ testBit )    pswReg = newIA;
                else                        pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_CBR ): {
                
                T64Word newIA   = addAdrOfs( pswReg, getImm15( instrReg ));
                T64Word val1    = getRegR( instrReg );
                T64Word val2    = getRegB( instrReg );
                int     cond    = (int) extractField( instrReg, 20, 2 );
                bool    res     = false;
                
                switch ( cond ) {
                        
                    case 0: res = ( val1 == val2 ); break;
                    case 1: res = ( val1 <  val2 ); break;
                    case 2: res = ( val1 != val2 ); break;
                    case 3: res = ( val1 <= val2 ); break;
                }
                
                if ( res )  pswReg = newIA;
                else        pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_MBR ): {
                
                T64Word newIA   = addAdrOfs( pswReg, getImm15( instrReg ));
                T64Word val1    = getRegR(instrReg );
                T64Word val2    = getRegB(instrReg );
                int     cond    = (int) extractField( instrReg, 20, 2 );
                bool    res     = false;
                
                switch ( cond ) {
                        
                    case 0: res = ( val1 == val2 ); break;
                    case 1: res = ( val1 <  val2 ); break;
                    case 2: res = ( val1 != val2 ); break;
                    case 3: res = ( val1 <= val2 ); break;
                }
                
                if ( res )  pswReg = newIA;
                else        pswReg = addAdrOfs( pswReg, 4 );
                
                setRegR( instrReg, val2 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_MR ): {
                
                switch ( extractField( instrReg, 19, 3 )) {
                        
                    case 0:     setRegR( instrReg, 0 ); break; // ??? fix ...
                    case 1:     break;
                    default:    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_LPA ): {
                
                // ?? privileged op ?
                
                T64Word res = 0;
                T64TlbEntry *e = tlb -> lookupTlb( getRegB(instrReg ));
                
                if ( extractField( instrReg, 19, 3 ) == 0 )   {
                    
                    
                }
                else if ( extractField(instrReg, 19, 3 ) == 1 )   {
                    
                    
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
                
                T64Word adr         = getRegB( instrReg );
                bool    privLevel   = false;
                
                if ( extractBit( instrReg, 14 )) privLevel = extractBit( instrReg, 13 );
                else                                   privLevel = extractBit( getRegA( instrReg ), 0 );
                
                T64TlbEntry *e      = tlb -> lookupTlb( getRegB(instrReg ));
                
                if ( extractField( instrReg, 19, 3 ) == 0 )   {
                    
                    // PRBR
                    
                }
                else if ( extractField( instrReg, 19, 3 ) == 1 )   {
                    
                    // PRBW
                    
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
                
                if ( extractField( instrReg, 19, 3 ) == 0 ) {
                    
                    setRegR(instrReg, tlb -> insertTlb( getRegB(instrReg ), getRegB(instrReg )));
                }
                else if ( extractField( instrReg, 19, 3 ) == 1 ) {
                    
                    tlb -> purgeTlb( getRegB( instrReg ));
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_CA ): {
                
                if ( extractField( instrReg, 19, 3 ) == 0 ) {
                    
                    // PCA, ignored for now.
                }
                else if ( extractField( instrReg, 19, 3 ) == 1 ) {
                    
                    // FCA, ignored for now.
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_MST ): {
                
                if ( extractField( instrReg, 19, 3 ) == 0 )   {
                    
                    // RSM
                }
                else if ( extractField( instrReg, 19, 3 ) == 1 )   {
                    
                    // SSM
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_RFI ): {
                
                // copy data from the CR places...
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_DIAG ): {
                
                T64Word val1 = getRegB( instrReg );
                T64Word val2 = getRegA( instrReg );
                
                // do DIAG word...
                
                setRegR( instrReg, 0 );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_TRAP ): {
                
                // under construction...
                
            } break;
                
            default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
        }
    }
    catch ( const T64Trap t ) {
        
        // ??? we are here because we trapped some level deep...
        // the emulator will fill in the trap info and the next instruction to execute will be
        // that of a trap handler.
        
    }
}

//------------------------------------------------------------------------------------------------------------
// The steo routine is the entry point to the CPU for executing one or more instructions.
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::step( int steps ) {
    
    try {
        
        while ( steps > 0 ) {
            
            fetchInstr( );
            executeInstr( );
            steps --;
        }
    }
    
    catch ( const T64Trap t ) {
        
    }
}

//------------------------------------------------------------------------------------------------------------
// The run routine will just let the CPU run.
//
// ??? should have a catcher for a looping CPU ?
//------------------------------------------------------------------------------------------------------------
void T64Cpu::run( ) {
    
    int steps = 9999; // ??? some high number... configurable ?
    
    step( steps );
}
