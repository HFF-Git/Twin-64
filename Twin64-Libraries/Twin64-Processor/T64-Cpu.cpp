//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - CPU Core
//
//----------------------------------------------------------------------------------------
// The CPU core is a submodule of the processor. It implements the actual CPU with
// all registers and executes the instructions.
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

//----------------------------------------------------------------------------------------
// Name space for local routines.
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
T64Cpu::T64Cpu( int modNum,
                int subModNum ) : 
                T64Module( MT_CPU_CORE, modNum, subModNum ) {
    

    #if 0
    this -> iTlb    = ( T64Tlb *) proc -> subModTab[ 1 ]; 
    this -> dTlb    = ( T64Tlb *) proc -> subModTab[ 2 ];
    this -> iCache  = ( T64Cache *) proc -> subModTab[ 3 ];
    this -> dCache  = ( T64Cache *) proc -> subModTab[ 4 ];
    #endif

    this -> reset( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::reset( ) {

    for ( int i = 0; i < MAX_CREGS; i++ ) ctlRegFile[ i ] = 0;
    for ( int i = 0; i < MAX_GREGS; i++ ) genRegFile[ i ] = 0;
    
    pswReg              = 0;
    instrReg            = 0;
    resvReg             = 0;
}

//----------------------------------------------------------------------------------------
// The register routines. Called externally by monitors and debuggers.
//
//----------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------
// Get the general register values based on the instruction field. These routines are
// used by the instruction execution code.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::getRegR( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegR( instr )));
}

T64Word T64Cpu::getRegB( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegB( instr )));
}

T64Word T64Cpu::getRegA( uint32_t instr ) {
    
    return( getGeneralReg( extractInstrRegA( instr )));
}

void T64Cpu::setRegR( uint32_t instr, T64Word val ) {
    
    setGeneralReg( extractInstrRegR( instr ), val );
}

//----------------------------------------------------------------------------------------
// Privilege Mode Check.
//
//----------------------------------------------------------------------------------------
void T64Cpu::privModeCheck( ) {

    if ( extractBit64( pswReg, 0 ) != 0 ) {

        throw( T64Trap( PRIV_VIOLATION_TRAP ), 0, 0, 0 ); // fix ...
    }
}

//----------------------------------------------------------------------------------------
// Protection check. If we are in user mode, we compare the pId with the protection 
// identifiers stored in the control registers CR4 to CR7. We also check the write 
// disable bit in the control register PID word. 
//
//----------------------------------------------------------------------------------------
void T64Cpu::protectionCheck( uint32_t pId, bool wMode ) {

    if ( extractBit64( pswReg, 0 ) != 0 ) {

        bool validPid   = false;
        bool validAcc   = false;
   
        validPid = (( extractField64( ctlRegFile[ 4 ],  1, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 4 ], 33, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 5 ],  1, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 5 ], 33, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 6 ],  1, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 6 ], 33, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 7 ],  1, 31 ) == pId ) ||
                    ( extractField64( ctlRegFile[ 7 ], 33, 31 ) == pId ));
                
        validAcc =  (( extractField64( ctlRegFile[ 4 ],  0, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 4 ], 32, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 5 ],  0, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 5 ], 32, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 6 ],  0, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 6 ], 32, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 7 ],  0, 1 ) && wMode ) ||
                     ( extractField64( ctlRegFile[ 7 ], 32, 1 ) && wMode ));

        if (( ! validPid ) || ( ! validAcc )) {

            throw( T64Trap( PROTECTION_TRAP, 0, 0, 0 )); // fix ...
        }          
    }   
}

//----------------------------------------------------------------------------------------
// Alignment Check.
//
//----------------------------------------------------------------------------------------
void T64Cpu::alignMentCheck( T64Word vAdr, int align ) {

    if ( ! isAligned( vAdr, align )) {

        throw( T64Trap( INSTR_ALIGNMENT_TRAP ), 0, 0, 0 ); // fix ...
    }
}

//----------------------------------------------------------------------------------------
// Instruction read. This is the central routine that fetches an instruction word. We
// first check the address range. For a physical address we must be in priv mode. For
// a virtual address, the TLB is consulted for the translation and security checking.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::instrRead( T64Word vAdr ) {

    T64Word instr = 0;
   
    alignMentCheck( vAdr, 4 );

    if ( proc -> isPhysicalAdrRange( vAdr )) { 

        privModeCheck( );
        iCache -> read( vAdr, &instr, 4, false );     
    }
    else {

        T64TlbEntry *tlbPtr = iTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) {
                
            throw ( T64Trap( TLB_ACCESS_TRAP, 0, 0, 0 )); // fix 
        }

        protectionCheck( vAdrSeg( tlbPtr ->vAdr ), false );
        iCache -> read( tlbPtr -> pAdr, &instr, 4, tlbPtr -> uncached );
    }

    return( instr );
}

//----------------------------------------------------------------------------------------
// Data read. We read a data item from memory. Valid lengths are 1, 2, 4 and 8. The 
// data is read from memory in the length given and stored right justified and sign 
// extended in the return argument. We first check the address range. For a physical 
// address we must be in priv mode. For a virtual address, the TLB is consulted for the
// translation and security checking. 
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::dataRead( T64Word vAdr, int len ) {

    T64Word data = 0;
   
    alignMentCheck( vAdr, len );

    if ( proc -> isPhysicalAdrRange( vAdr )) { 
        
        privModeCheck( );
        dCache -> read( vAdr, &data, 8, false );
    }
    else {

        T64TlbEntry *tlbPtr = dTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) {
                
            throw ( T64Trap( TLB_ACCESS_TRAP, 0, 0, 0 )); // fix 
        }

        protectionCheck( vAdrSeg( tlbPtr ->vAdr ), false );
        iCache -> read( tlbPtr -> pAdr, &data, len, tlbPtr -> uncached );
    }

    // ??? sign extend

    return( data );
}

//----------------------------------------------------------------------------------------
// Data write. We write the data item to memory. Valid lengths are 1, 2, 4 and 8. The
// data is stored in memory in the length given. We first check the address range. For
// a physical address we must be in priv mode. For a virtual address, the TLB is 
// consulted for the translation and security checking. 
//
//----------------------------------------------------------------------------------------
void T64Cpu::dataWrite( T64Word vAdr, T64Word val, int len ) {
  
    alignMentCheck( vAdr, len );
        
    if ( proc -> isPhysicalAdrRange( vAdr )) { 
        
        privModeCheck( );
        dCache -> write( vAdr, val, len, false );           
    }
    else {

        T64TlbEntry *tlbPtr = dTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) {
                
            throw ( T64Trap( TLB_ACCESS_TRAP, 0, 0, 0 )); // fix 
        }

        protectionCheck( vAdrSeg( tlbPtr ->vAdr ), true );
        iCache -> write( tlbPtr -> pAdr, val, len, tlbPtr -> uncached );
    }
}

//----------------------------------------------------------------------------------------
// Read memory data based using RegB and the IMM-13 offset to form the address.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::dataReadRegBOfsImm13( uint32_t instr ) {
    
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
T64Word T64Cpu::dataReadRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1U << dw;
    
    adr = addAdrOfs( adr, ofs );
    
    if (( dw > 1 ) && ( !isAligned( adr, len ))) {
                
        throw ( T64Trap( DATA_ALIGNMENT_TRAP, 0, 0, 0 )); // fix 
    }
    
    return( dataRead( adr, len ));
}

//----------------------------------------------------------------------------------------
// Write data to memory based using RegB and the IMM-13 offset to form the 
// address.
//
//----------------------------------------------------------------------------------------
void T64Cpu::dataWriteRegBOfsImm13( uint32_t instr ) {
    
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
void T64Cpu:: dataWriteRegBOfsRegX( uint32_t instr ) {
    
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
void T64Cpu::instrExecute( uint32_t instr ) {
    
    try {
        
        int opCode = extractInstrOpCode( instr );
        
        switch ( opCode ) {
                
            case ( OPC_GRP_ALU * 16 + OPC_ADD ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instr );
                        T64Word val2 = getRegA( instr );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instr );
                        T64Word val2 = extractImm13( instr );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ADD ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instr );
                        T64Word val2 = dataReadRegBOfsImm13( instr );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 + val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instr );
                        T64Word val2 = dataReadRegBOfsRegX( instr );
                        
                        if ( willAddOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 + val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_SUB ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegB( instr );
                        T64Word val2 = getRegA( instr );
                        
                        if ( willSubOverflow( val1, val2 ))
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegB( instr );
                        T64Word val2 = extractImm13( instr );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_SUB ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0: {
                        
                        T64Word val1 = getRegR( instr );
                        T64Word val2 = dataReadRegBOfsImm13( instr );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 - val2 );
                        
                    } break;
                        
                    case 1: {
                        
                        T64Word val1 = getRegR( instr );
                        T64Word val2 = dataReadRegBOfsRegX( instr );
                        
                        if ( willSubOverflow( val1, val2 )) 
                            throw ( T64Trap( OVERFLOW_TRAP ));
                        else 
                            setRegR( instr, val1 - val2 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_AND ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = getRegA( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = extractInstrImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 & val2;

                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_AND ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsRegX( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 & val2;

                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_OR ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = getRegA( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = extractImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_OR ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsRegX( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 | val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_XOR ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = getRegA( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res  = val1 ^ val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegB( instr );
                    T64Word val2 = extractImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 ))   val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_XOR ): {
                
                if ( extractInstrBit( instr, 19 )) {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsImm13( instr );
                    
                    if ( extractInstrBit( instr, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                else {
                    
                    T64Word val1 = getRegR( instr );
                    T64Word val2 = dataReadRegBOfsRegX( instr );
                    
                    if ( extractInstrBit( instr, 20 )) val1 = ~ val1;
                    
                    T64Word res = val1 ^ val2;
                    
                    if ( extractInstrBit( instr, 21 )) res = ~ res;
                    setRegR( instr, res );
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
                
                T64Word val1 = getRegB( instr );
                T64Word val2 = 0;
                T64Word res  = 0;
                
                if ( extractInstrBit( instr, 19 )) val2 = extractImm13( instr );
                else                                  val2 = getRegA( instr );
                
                switch ( extractInstrField( instr, 20, 2 )) {
                        
                    case 0: res = ( val1 == val2 ) ? 1 : 0; break;
                    case 1: res = ( val1 <  val2 ) ? 1 : 0; break;
                    case 2: res = ( val1 != val2 ) ? 1 : 0; break;
                    case 3: res = ( val1 <= val2 ) ? 1 : 0; break;
                }
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_CMP ): {
                
                T64Word val1 = getRegB( instr );
                T64Word val2 = 0;
                T64Word res  = 0;
                
                if ( extractInstrBit( instr, 19 )) 
                    val2 = dataReadRegBOfsRegX( instr );
                else                             
                    val2 = dataReadRegBOfsImm13( instr );
                
                switch ( extractInstrField( instr, 20, 2 )) {
                        
                    case 0: res = ( val1 == val2 ) ? 1 : 0; break;
                    case 1: res = ( val1 <  val2 ) ? 1 : 0; break;
                    case 2: res = ( val1 != val2 ) ? 1 : 0; break;
                    case 3: res = ( val1 <= val2 ) ? 1 : 0; break;
                }
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0: { // EXTR
                        
                        T64Word val = getRegB( instr );
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = extractInstrField( instr, 0, 6 );
                        
                        if ( extractInstrBit( instr, 13 ))   
                            pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                               
                            pos = extractInstrField( instr, 6, 6 );
                        
                        if ( extractInstrBit( instr, 12 ))  
                            res = extractSignedField64( val, (int) pos, len );
                        else                               
                            res = extractField64( val, (int) pos, len );
                        
                        setRegR( instr, res );
                        
                    } break;
                        
                    case 1: { // DEP
                        
                        T64Word val1 = 0;
                        T64Word val2 = 0;
                        T64Word res  = 0;
                        int     pos  = 0;
                        int     len  = (int) extractInstrField( instr, 0, 6 );
                        
                        if ( extractInstrBit( instr, 13 ))    
                            pos = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                                
                            pos = (int) extractInstrField( instr, 6, 6 );
                        
                        if ( extractInstrBit( instr, 12 ))    
                            val1 = 0;
                        else                                
                            val1 = getRegR( instr );
                        
                        if ( extractInstrBit( instr, 14 ))    
                            val2 = extractInstrField( instr, 15, 4 );
                        else                                
                            val2 = getRegB( instr );
                        
                        res = depositField( val1, pos, len , val2 );
                        setRegR( instr, res );
                        pswReg = addAdrOfs( pswReg, 4 );
                        
                    } break;
                        
                    case 3: { // DSR
                        
                        T64Word val1    = getRegB( instr );
                        T64Word val2    = getRegA( instr );
                        int     shamt   = 0;
                        T64Word res     = 0;
                        
                        if ( extractInstrBit( instr, 13 ))    
                            shamt = (int) ctlRegFile[ 1 ]; // ??? fix ...
                        else                                
                            shamt = (int) extractInstrField( instr, 6, 6 );
                        
                        res = shiftRight128( val1, val2, shamt );
                        setRegR( instr, res );
                        pswReg = addAdrOfs( pswReg, 4 );
                        
                    } break;
                        
                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
                
                T64Word val1  = getRegR( instr );
                T64Word val2  = 0;
                T64Word res   = 0;
                int     shamt = (int) extractInstrField( instr, 20, 2 );
                
                if ( extractInstrBit( instr, 14 )) val2 = extractImm13( instr );
                else                             val2 = getRegB( instr );
                
                if ( extractInstrBit( instr, 19 )) { // SHRxA
                    
                    res = val1 >> shamt;
                }
                else { // SHLxA
                    
                    if ( willShiftLeftOverflow( val1, shamt )) 
                        throw ( T64Trap( OVERFLOW_TRAP ));

                    res = val1 << shamt;
                }
                
                if ( willAddOverflow( res, val2 )) 
                    throw ( T64Trap( OVERFLOW_TRAP ));

                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
                
                T64Word val = extractImm20U( instr );
                T64Word res = 0;
                
                switch ( extractInstrField( instr, 20, 2 )) {
                        
                    case 0: res = val;          break;
                    case 1: res = val << 12;    break;
                    case 2: res = val << 32;    break;
                    case 3: res = val << 52;    break;
                }
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_ALU * 16 + OPC_LDO ): {
                
                T64Word base = getRegB( instr );
                T64Word ofs  = extractImm15( instr );
                T64Word res  = addAdrOfs( base, ofs );
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LD ): {
                
                T64Word res = 0;
                
                if ( extractInstrField( instr, 19, 3 ) == 0 )   
                    res = dataReadRegBOfsImm13( instr );
                else if ( extractInstrField( instr, 19, 3 ) == 1 )   
                    res = dataReadRegBOfsRegX( instr );
                else
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_LDR ): {
                
                T64Word res = 0;
                
                if ( extractInstrField( instr, 19, 3 ) != 0 ) 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                res = dataReadRegBOfsImm13( instr );
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_ST ): {
                
                if ( extractInstrField( instr, 19, 3 ) == 0 )   
                    dataWriteRegBOfsImm13( instr );
                else if ( extractInstrField( instr, 19, 3 ) == 1 )   
                    dataWriteRegBOfsRegX( instr );
                else    
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_MEM * 16 + OPC_STC ): {
                
                if ( extractInstrField( instr, 19, 3 ) == 1 ) 
                    dataWriteRegBOfsImm13( instr );
                else 
                    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_B ): {
                
                T64Word ofs     = extractImm19( instr ) << 2;
                T64Word rl      = addAdrOfs( pswReg, 4 );
                T64Word newIA   = addAdrOfs( pswReg, ofs );
                
                if ( extractInstrBit( instr, 19 )) {  // gateway
                    
                    pswReg = newIA;
                    setRegR( instr, rl );
                }
                else { // regular branch
                    
                    pswReg = newIA;
                    setRegR( instr, rl );
                }
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_BR ): {

                switch ( extractInstrField( instr, 19, 3 )) {

                    case 0: {

                        T64Word ofs     = getRegB( instr ) << 2;
                        T64Word rl      = addAdrOfs( pswReg, 4 );
                        T64Word newIA   = addAdrOfs( pswReg, ofs );
                
                        if ( extractInstrField( instr, 19, 3 ) != 0 ) 
                            throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                    
                        if ( ! isAligned( newIA, 4 )) {
                            
                            throw( T64Trap( INSTR_ALIGNMENT_TRAP ));
                        }
                
                        pswReg = newIA;
                        setRegR( instr, rl );
                    
                    } break;

                    case 1: {

                        T64Word base    = getRegB( instr );
                        T64Word ofs     = getRegA( instr );
                        T64Word rl      = addAdrOfs( pswReg, 4 );
                        T64Word newIA   = addAdrOfs( base, ofs );
                
                        if ( extractInstrField( instr, 19, 3 ) != 0 ) 
                            throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                        if ( ! isAligned( newIA, 4 )) {
                            
                            throw( T64Trap( INSTR_ALIGNMENT_TRAP ));
                        }
                
                        pswReg = newIA;
                        setRegR( instr, rl );
                    
                    } break;

                    default: throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }

            } break;   
               
            case ( OPC_GRP_BR * 16 + OPC_BB ): {
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm13( instr ));
                bool    testVal = extractInstrBit( instr, 19 );
                bool    testBit = 0;
                int     pos     = 0;
                
                if ( extractInstrBit( instr, 20 ))  
                    pos = 0; // use SAR
                else                                    
                    pos = (int) extractInstrField( instr, 13, 6 );
                
                testBit = extractInstrBit( instr, pos );
                
                if ( testVal ^ testBit )    pswReg = newIA;
                else                        pswReg = addAdrOfs( pswReg, 4 );
                
            } break;

            case ( OPC_GRP_BR * 16 + OPC_ABR ): {
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm15( instr ));
                T64Word val1    = getRegR( instr );
                T64Word val2    = getRegB( instr );
                int     cond    = (int) extractInstrField( instr, 19, 3 );
                bool    res     = false;

                if ( willAddOverflow( val1, val2 )) 
                    throw ( T64Trap( OVERFLOW_TRAP ));
                else 
                    setRegR( instr, val1 + val2 );

                switch ( cond ) {
                        
                    case 0: res = ( val1 == val2 ); break;
                    case 1: res = ( val1 <  val2 ); break;
                    case 2: res = ( val1 != val2 ); break;
                    case 3: res = ( val1 <= val2 ); break;

                    // ??? fix
                }
                
                if ( res )  pswReg = newIA;
                else        pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_BR * 16 + OPC_CBR ): {
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm15( instr ));
                T64Word val1    = getRegR( instr );
                T64Word val2    = getRegB( instr );
                int     cond    = (int) extractInstrField( instr, 20, 2 );
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
                
                T64Word newIA   = addAdrOfs( pswReg, extractImm15( instr ));
                T64Word val1    = getRegR(instr );
                T64Word val2    = getRegB(instr );
                int     cond    = (int) extractInstrField( instr, 20, 2 );
                bool    res     = false;
                
                switch ( cond ) {
                        
                    case 0: res = ( val1 == val2 ); break;
                    case 1: res = ( val1 <  val2 ); break;
                    case 2: res = ( val1 != val2 ); break;
                    case 3: res = ( val1 <= val2 ); break;
                }
                
                if ( res )  pswReg = newIA;
                else        pswReg = addAdrOfs( pswReg, 4 );
                
                setRegR( instr, val2 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_MR ): {
                
                switch ( extractInstrField( instr, 19, 3 )) {
                        
                    case 0:     setRegR( instr, 0 ); break; // ??? fix ...
                    case 1:     break;
                    default:    throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_LPA ): {
                
                privModeCheck( );

                T64Word res = 0;
                T64TlbEntry *e = dTlb -> lookup( getRegB( instr ));
                
                if ( extractInstrField( instr, 19, 3 ) == 0 )   {
                    
                    
                }
                else if ( extractInstrField( instr, 19, 3 ) == 1 )   {
                    
                    
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                setRegR( instr, res );
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
                
                T64Word adr         = getRegB( instr );
                bool    privLevel   = false;
                
                if ( extractInstrBit( instr, 14 )) 
                    privLevel = extractInstrBit( instr, 13 );
                else                                   
                    privLevel = extractBit64( getRegA( instr ), 0 );
                
                T64TlbEntry *e = dTlb -> lookup( getRegB( instr ));
                if ( e != nullptr ) {

                    if ( extractInstrField( instr, 19, 3 ) == 0 )   {
                    
                        // PRBR
                    
                    }
                    else if ( extractInstrField( instr, 19, 3 ) == 1 )   {
                    
                        // PRBW
                    
                    }
                    else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                }
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_TLB ): {

                // ??? should distinguish between instr and data in the opt1 field ?
                
                if ( extractInstrField( instr, 19, 3 ) == 0 ) {

                    proc -> insertDataTlb( getRegB( instr ), getRegA( instr ));
                    setRegR( instr, 1 );
                }
                else if ( extractInstrField( instr, 19, 3 ) == 1 ) {
                    
                    proc -> purgeDataTlb( getRegB( instr ));
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_CA ): {

                // ??? should distinguish between instr and data in the opt1 field ?
                
                if ( extractInstrField( instr, 19, 3 ) == 0 ) {

                    proc -> purgeDataCache( getRegB( instr ));
                }
                else if ( extractInstrField( instr, 19, 3 ) == 1 ) {
                    
                    proc -> flushDataCache( getRegB( instr ));
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_MST ): {
                
                if ( extractInstrField( instr, 19, 3 ) == 0 )   {
                    
                    // RSM
                }
                else if ( extractInstrField( instr, 19, 3 ) == 1 )   {
                    
                    // SSM
                }
                else throw ( T64Trap( ILLEGAL_INSTR_TRAP ));
                
                pswReg = addAdrOfs( pswReg, 4 );
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_RFI ): {
                
                // copy data from the CR places...
                
            } break;
                
            case ( OPC_GRP_SYS * 16 + OPC_DIAG ): {
                
                T64Word val1 = getRegB( instr );
                T64Word val2 = getRegA( instr );
                
                // do DIAG word...
                
                setRegR( instr, 0 );
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
        // the emulator will finalize in the trap info and the next instruction 
        // to execute will be that of a trap handler.
        
    }
}

//----------------------------------------------------------------------------------------
// The step routine is the entry point to the CPU for executing one or more 
// instructions.
//
//----------------------------------------------------------------------------------------
void T64Cpu::step( ) {
    
    try {
        
            instrReg = instrRead( extractField64( pswReg, 63, 52 ));
            instrExecute( instrReg );
    }
    
    catch ( const T64Trap t ) {
        
    }
}


