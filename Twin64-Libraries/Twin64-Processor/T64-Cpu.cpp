//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - CPU Core
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - CPU Core
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
#include "T64-Processor.h"
#include "T64-Util.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {

};

//****************************************************************************************
//****************************************************************************************
//
// CPU
//
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Processor::T64Processor( ) {
    
    this -> tlb     = new T64Tlb( 64 );
    this -> cache   = new T64Cache( );
    
    this -> reset( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Processor::reset( ) {

    for ( int i = 0; i < MAX_CREGS; i++ ) ctlRegFile[ i ] = 0;
    for ( int i = 0; i < MAX_GREGS; i++ ) genRegFile[ i ] = 0;
    
    pswReg              = 0; // ??? PDC space address ?
    instrReg            = 0;
    resvReg             = 0;

    instructionCount    = 0;
    cycleCount          = 0;
    
    tlb     -> reset( );
    cache   -> reset( );

}

//----------------------------------------------------------------------------------------
// The register routines. Called externally by monitors and debuggers.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::getGeneralReg( int index ) {
    
    if ( index == 0 )   return( 0 );
    else                return( genRegFile[ index % MAX_GREGS ] );
}

void T64Processor::setGeneralReg( int index, T64Word val ) {
    
    if ( index != 0 ) genRegFile[ index % MAX_GREGS ] = val;
}

T64Word T64Processor::getControlReg( int index ) {
    
    return( ctlRegFile[ index % MAX_CREGS ] );
}

void T64Processor::setControlReg( int index, T64Word val ) {
    
    ctlRegFile[ index % MAX_CREGS ] = val;
}

T64Word T64Processor::getPswReg( ) {
    
    return( pswReg );
}

void T64Processor::setPswReg( T64Word val ) {
    
    pswReg = val;
}

//----------------------------------------------------------------------------------------
// Get the general register values based on the instruction field. These routines are
// used by the instruction execution code.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::getRegR( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegR( instr )));
}

T64Word T64Processor::getRegB( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegB( instr )));
}

T64Word T64Processor::getRegA( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegA( instr )));
}

void T64Processor::setRegR( uint32_t instr, T64Word val ) {
    
    setGeneralReg( extractInstrRegR( instr ), val );
}

//----------------------------------------------------------------------------------------
// Translate the virtual address to a physical address. There are two basic address
// ranges. If the address range is the physical address range or I/O space, the virtual
// address is directly mapped to the physical address. The caller must run in privileged
// mode. If the address range is the virtual address range, the TLB is consulted and we
// are subject to access right and protection checking.   
//
// ??? need type of access in the parameter list ?
//----------------------------------------------------------------------------------------
T64Word T64Processor::translateAdr( T64Word vAdr ) {
    
    if ( extractField64( vAdr, 32, 20 ) == 0 ) {  // physical address range ?
        
        if ( ! extractBit64( pswReg, 0 )) throw ( T64Trap( PRIV_VIOLATION_TRAP ));
        return( vAdr );
    }
    else {
        
        T64TlbEntry *tlbPtr = tlb -> lookupTlb( vAdr );
        if ( tlbPtr == nullptr ) throw ( T64Trap( TLB_ACCESS_TRAP ));
        
        // ??? access check .....
        
        // protection check ... check pid, but not R/W bit...
        
        uint32_t pId   = tlbPtr -> protectId;
        uint32_t pMode = 0;
            
        if ( ! (( extractField64( ctlRegFile[ 0 ],  1, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ], 33, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ],  1, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ], 33, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ],  1, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ], 33, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ],  1, 31 ) == pId ) ||
                ( extractField64( ctlRegFile[ 0 ], 33, 31 ) == pId ))) {
                
            throw ( T64Trap( PROTECTION_TRAP ));
        }

        if ( ! (( extractField64( ctlRegFile[ 0 ],  0, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ], 32, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ],  0, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ], 32, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ],  0, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ], 32, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ],  0, 1 ) == pMode ) ||
                ( extractField64( ctlRegFile[ 0 ], 32, 1 ) == pMode ))) {
                
            throw ( T64Trap( PROTECTION_TRAP ));
        }
       
        return( tlbPtr -> pAdr );
    }
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Processor::instrRead( ) {
    
    try {
        
        T64Word pAdr = translateAdr( extractField64( pswReg, 63, 52 ));

        // ??? readCache ...
        
    }
    catch ( const T64Trap t ) {
        
        // can do something before re-raising ....
        throw;
    }
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::dataRead( T64Word vAdr, int len ) {
    
    try {
        
        T64Word pAdr = translateAdr( vAdr );

        // ??? readCache ...
        
        // ??? fix to get real size ...
        
        return( 0 );
    }
    catch ( const T64Trap t ) {
        
        // can do something before re-raising ....
        throw;
    }
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Processor::dataWrite( T64Word vAdr, T64Word val, int len ) {
    
    // ??? How to distinguish between mem and I/O?
    
    try {
        
        T64Word pAdr = translateAdr( vAdr );

        // ??? writeCache  ...
        
    }
    catch ( const T64Trap t ) {
        
        // can do something before re-raising ....
        throw;
    }
}

//----------------------------------------------------------------------------------------
// Read memory data based using RegB and the IMM-13 offset to form the address.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::dataReadRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr ); 
    T64Word     ofs     = extractImm13( instr ) << dw;
    int         len     = 1U << dw;
    
    return( dataRead( addAdrOfs( adr, ofs ), len ));
}

//----------------------------------------------------------------------------------------
// Read memory data based using RegB and the RegX offset to form the address.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::dataReadRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1U << dw;
    
    adr = addAdrOfs( adr, ofs );
    
    if (( dw > 1 ) && ( !isAligned( adr, len ))) throw ( 0 );
    
    return( dataRead( adr, len ));
}

//----------------------------------------------------------------------------------------
// Write data to memory based using RegB and the IMM-13 offset to form the 
// address.
//
//----------------------------------------------------------------------------------------
void T64Processor::dataWriteRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = extractImm13( instr ) << dw;
    int         len     = 1U << dw;
    T64Word     val     = getRegR( instr );
    
    dataWrite( addAdrOfs( adr, ofs ), val, len );
}

//----------------------------------------------------------------------------------------
// Write data to memory based using RegB and the RegX offset to form the
// address.
//
//----------------------------------------------------------------------------------------
void T64Processor:: dataWriteRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1U << dw;
    T64Word     val     = getRegR( instr );
    
    adr = addAdrOfs( adr, ofs );
    
    if (( dw > 1 ) && ( !isAligned( adr, len ))) throw ( 0 );
    
    dataWrite( addAdrOfs( adr, ofs ), val, len );
}

//----------------------------------------------------------------------------------------
// Execute an instruction. This is the key routine of the emulator. Essentially a big
// case statement. Each instruction is encoded based on the instruction group and the
// opcode family. Inside each such cases, the option 1 field ( bits 19 .. 22 ) further
// qualifies an instruction.
//
//----------------------------------------------------------------------------------------
void T64Processor::instrExecute( ) {
    
    try {
        
        int opCode = extractInstrOpCode( instrReg );
        
        switch ( opCode ) {
                
            case ( OPC_GRP_ALU * 16 + OPC_ADD ): {
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getRegA( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = extractImm13( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ADD ): {
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsImm13( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsRegX( instrReg );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_SUB ): {
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = getRegA( instrReg );
                        
                        if ( willSubOverflow( val1, val2 ))
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instrReg );
                        T64Word val2 = extractImm13( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_SUB ): {
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsImm13( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instrReg );
                        T64Word val2 = dataReadRegBOfsRegX( instrReg );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instrReg, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_AND ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = extractInstrImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 & val2;

                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_AND ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;

                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_OR ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = extractImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_OR ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_XOR ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = getRegA( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instrReg );
                    T64Word val2 = extractImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_XOR ): {
                
                if ( extractInstrBit( instrReg, 19 )) {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsImm13( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instrReg );
                    T64Word val2 = dataReadRegBOfsRegX( instrReg );
                    
                    if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instrReg, 21 )) res = ~ res;
                    setRegR( instrReg, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
                
                T64Word val1 = getRegB( instrReg );
                T64Word val2 = 0;
                T64Word res  = 0;
                
                if ( extractInstrBit( instrReg, 19 )) val2 = extractImm13( instrReg );
                else                                  val2 = getRegA( instrReg );
                
                switch ( extractInstrField( instrReg, 20, 2 )) {
                        
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
                
                if ( extractInstrBit( instrReg, 19 )) 
                    val2 = dataReadRegBOfsRegX( instrReg );
                else                             
                    val2 = dataReadRegBOfsImm13( instrReg );
                
                switch ( extractInstrField( instrReg, 20, 2 )) {
                        
                    case 0: res = ( val1 == val2 ) ? 1 : 0; break;
                    case 1: res = ( val1 <  val2 ) ? 1 : 0; break;
                    case 2: res = ( val1 != val2 ) ? 1 : 0; break;
                    case 3: res = ( val1 <= val2 ) ? 1 : 0; break;
                }
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
                    case 0: { // EXTR
                        
                        T64Word val = getRegB( instrReg );
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = extractInstrField( instrReg, 0, 6 );
                        
                        if ( extractInstrBit( instrReg, 13 ))   
                            pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                               
                            pos = extractInstrField( instrReg, 6, 6 );
                        
                        if ( extractInstrBit( instrReg, 12 ))  
                            res = extractSignedField64( val, (int) pos, len );
                        else                               
                            res = extractField64( val, (int) pos, len );
                        
                        setRegR( instrReg, res );
                        
                    } break;
                        
                    case 1: { // DEP
                        
                        T64Word val1 = 0;
                        T64Word val2 = 0;
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = (int) extractInstrField( instrReg, 0, 6 );
                        
                        if ( extractInstrBit( instrReg, 13 ))    
                            pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                                
                            pos = (int) extractInstrField( instrReg, 6, 6 );
                        
                        if ( extractInstrBit( instrReg, 12 ))    
                            val1 = 0;
                        else                                
                            val1 = getRegR( instrReg );
                        
                        if ( extractInstrBit( instrReg, 14 ))    
                            val2 = extractInstrField( instrReg, 15, 4 );
                        else                                
                            val2 = getRegB( instrReg );
                        
                        res = depositField( val1, pos, len , val2 );
                        setRegR( instrReg, res );
                        pswReg = addAdrOfs( pswReg, 4 );
                        
                    } break;
                        
                    case 3: { // DSR
                        
                        T64Word val1    = getRegB( instrReg );
                        T64Word val2    = getRegA( instrReg );
                        int     shamt   = 0;
                        T64Word res     = 0;
                        
                        if ( extractInstrBit( instrReg, 13 ))    
                            shamt = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                                
                            shamt = (int) extractInstrField( instrReg, 6, 6 );
                        
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
                int     shamt = (int) extractInstrField( instrReg, 20, 2 );
                
                if ( extractInstrBit( instrReg, 14 )) val2 = extractImm13( instrReg );
                else                             val2 = getRegB( instrReg );
                
                if ( extractInstrBit( instrReg, 19 )) { // SHRxA
                    
                    res = val1 >> shamt;
                }
                else { // SHLxA
                    
                    if ( willShiftLftOverflow( val1, shamt )) 
                        throw ( T64Trap( OVERFLOW_TRAP ));

                    res = val1 << shamt;
                }
                
                if ( willAddOverflow( res, val2 )) 
                    throw ( T64Trap( OVERFLOW_TRAP ));

                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
                
                T64Word val = extractImm20U( instrReg );
                T64Word res = 0;
                
                switch ( extractInstrField( instrReg, 20, 2 )) {
                        
                    case 0: res = val;          break;
                    case 1: res = val << 12;    break;
                    case 2: res = val << 32;    break;
                    case 3: res = val << 52;    break;
                }
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_LDO ): {
                
                T64Word base = getRegB( instrReg );
                T64Word ofs  = extractImm15( instrReg );
                T64Word res  = addAdrOfs( base, ofs );
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LD ): {
                
                T64Word res = 0;
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 )   
                    res = dataReadRegBOfsImm13( instrReg );
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   
                    res = dataReadRegBOfsRegX( instrReg );
                else
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LDR ): {
                
                T64Word res = 0;
                
                if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                res = dataReadRegBOfsImm13( instrReg );
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ST ): {
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 )   
                    dataWriteRegBOfsImm13( instrReg );
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   
                    dataWriteRegBOfsRegX( instrReg );
                else    
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_STC ): {
                
                if ( extractInstrField( instrReg, 19, 3 ) == 1 ) 
                    dataWriteRegBOfsImm13( instrReg );
                else 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_B ): {
                
                T64Word ofs     = extractImm19( instrReg ) << 2;
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( pswReg, ofs );
                
                if ( extractInstrBit( instrReg, 19 )) {  // gateway
                    
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
                
                if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                    
                if ( ! isAligned( newIA, 4 )) throw( T64Trap( ALIGNMENT_TRAP ));
                
                pswReg = newIA;
                setRegR( instrReg, rl );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BV ): {
                
                T64Word base    = getRegB( instrReg );
                T64Word ofs     = getRegA( instrReg );
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( base, ofs );
                
                if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                if ( ! isAligned( newIA, 4 )) throw( T64Trap( ALIGNMENT_TRAP ));
                
                pswReg = newIA;
                setRegR( instrReg, rl );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BB ): {
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm13( instrReg ));
                bool    testVal = extractInstrBit( instrReg, 19 );
                bool    testBit = 0;
                int     pos     = 0;
                
                if ( extractInstrBit( instrReg, 20 ))  
                    pos = 0; // use SAR
                else                                    
                    pos = (int) extractInstrField( instrReg, 13, 6 );
                
                testBit = extractInstrBit( instrReg, pos );
                
                if ( testVal ^ testBit )    pswReg = newIA;
                else                        pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_CBR ): {
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm15( instrReg ));
                T64Word val1    = getRegR( instrReg );
                T64Word val2    = getRegB( instrReg );
                int     cond    = (int) extractInstrField( instrReg, 20, 2 );
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
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm15( instrReg ));
                T64Word val1    = getRegR(instrReg );
                T64Word val2    = getRegB(instrReg );
                int     cond    = (int) extractInstrField( instrReg, 20, 2 );
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
                
                switch ( extractInstrField( instrReg, 19, 3 )) {
                        
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
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
                    
                    
                }
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
                    
                    
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instrReg, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
                
                T64Word adr         = getRegB( instrReg );
                bool    privLevel   = false;
                
                if ( extractInstrBit( instrReg, 14 )) 
                    privLevel = extractInstrBit( instrReg, 13 );
                else                                   
                    privLevel = extractBit64( getRegA( instrReg ), 0 );
                
                T64TlbEntry *e      = tlb -> lookupTlb( getRegB(instrReg ));
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
                    
                    // PRBR
                    
                }
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
                    
                    // PRBW
                    
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 ) {
                    
                    tlb -> insertTlb( getRegB(instrReg ), getRegB(instrReg ));
                    
                    setRegR( instrReg, 1 );
                }
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 ) {
                    
                    tlb -> purgeTlb( getRegB( instrReg ));
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_CA ): {
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 ) {
                    
                    // PCA, ignored for now.
                }
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 ) {
                    
                    // FCA, ignored for now.
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_MST ): {
                
                if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
                    
                    // RSM
                }
                else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
                    
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
        // the emulator will fill in the trap info and the next instruction 
        // to execute will be that of a trap handler.
        
    }
}

//----------------------------------------------------------------------------------------
// The step routine is the entry point to the CPU for executing one or more 
// instructions.
//
//----------------------------------------------------------------------------------------
void T64Processor::step( ) {
    
    try {
        
            instrRead( );
            instrExecute( );
    }
    
    catch ( const T64Trap t ) {
        
    }
}

void T64Processor::event( T64ModuleEvent evt ) {


}
