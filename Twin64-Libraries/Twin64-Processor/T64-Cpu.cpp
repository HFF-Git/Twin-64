//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit Processor - CPU Core
//
//----------------------------------------------------------------------------------------
// The CPU core is a submodule of the processor. It implements the actual CPU with
// all registers and executes the instructions.
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit Processor - CPU Core
// Copyright (C) 2020 - 2026 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under 
// the terms of the GNU General Public License as published by the Free Software 
// Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details. You 
// should have received a copy of the GNU General Public License along with this 
// program.  If not, see <http://www.gnu.org/licenses/>.
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
// CPU object constructor. We keep a reference to the processor object for access
// the processor components and via the processor to the system.
//
// ??? need to set physical memory boundaries... where to get the correct value
// from ? Initially, we could just accept the first 4Gb until we know better...
//----------------------------------------------------------------------------------------
T64Cpu::T64Cpu( T64Processor *proc, T64CpuType cpuType ) {

    this -> proc    = proc;
    this -> cpuType = cpuType;

    this -> lowerPhysMemAdr = 0;
    this -> upperPhysMemAdr = T64_DEF_PHYS_MEM_LIMIT;
    
    switch ( cpuType ) {

        default: ;
    }

    this -> reset( );
}

//----------------------------------------------------------------------------------------
// Destructor.
//
//----------------------------------------------------------------------------------------
T64Cpu:: ~T64Cpu( ) { }   

//----------------------------------------------------------------------------------------
// CPU reset method.
//
// ??? get the actual physical memory size ?
//----------------------------------------------------------------------------------------
void T64Cpu::reset( ) {

    for ( int i = 0; i < T64_MAX_CREGS; i++ ) cRegFile[ i ] = 0;
    for ( int i = 0; i < T64_MAX_GREGS; i++ ) gRegFile[ i ] = 0;
    
    psrReg          = 0;
    instrReg        = 0;
    resvReg         = 0;
    lowerPhysMemAdr = 0;
    upperPhysMemAdr = T64_DEF_PHYS_MEM_LIMIT;
}

//----------------------------------------------------------------------------------------
// The register access routines.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::getGeneralReg( int index ) {
    
    if ( index == 0 )   return( 0 );
    else                return( gRegFile[ index % T64_MAX_GREGS ] );
}

void T64Cpu::setGeneralReg( int index, T64Word val ) {
    
    if ( index != 0 ) gRegFile[ index % T64_MAX_GREGS ] = val;
}

T64Word T64Cpu::getControlReg( int index ) {
    
    return( cRegFile[ index % T64_MAX_CREGS ] );
}

void T64Cpu::setControlReg( int index, T64Word val ) {
    
    cRegFile[ index % T64_MAX_CREGS ] = val;
}

T64Word T64Cpu::getPsrReg( ) {
    
    return( psrReg );
}

void T64Cpu::setPsrReg( T64Word val ) {
    
    psrReg = val;
}

//----------------------------------------------------------------------------------------
// Get/Set the general register values based on the register position in the
// instruction. These routines are used by the instruction execution code.
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
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::dataTlbMissTrap( T64Word adr ) {

    throw( T64Trap( DATA_TLB_MISS_TRAP, psrReg, instrReg, adr ));
}

void T64Cpu::instrTlbMissTrap( T64Word adr ) {

    throw( T64Trap( INSTR_TLB_MISS_TRAP, psrReg, instrReg, adr ));
}

void T64Cpu::instrAlignmentTrap( T64Word adr ) {

    throw( T64Trap( INSTR_ALIGNMENT_TRAP, psrReg, instrReg, adr ));
}

void T64Cpu::instrMemProtectionTrap( T64Word adr ) {

    throw( T64Trap( INSTR_PROTECTION_TRAP, psrReg, 0, adr ));
}

void T64Cpu::dataAlignmentTrap( T64Word adr ) {

    throw( T64Trap( DATA_ALIGNMENT_TRAP, psrReg, instrReg, adr ));
}

void T64Cpu::dataMemProtectionTrap( T64Word adr ) {

    throw( T64Trap( INSTR_PROTECTION_TRAP, psrReg, instrReg, adr ));
}

void T64Cpu::privModeOperationTrap( ) {

    throw( T64Trap( PRIV_OPERATION_TRAP, psrReg, instrReg, 0 ));
}

void T64Cpu::overFlowTrap( ) {

    throw( T64Trap( OVERFLOW_TRAP, psrReg, instrReg, 0 ));
}

void T64Cpu::illegalInstrTrap( ) {

    throw( T64Trap( ILLEGAL_INSTR_TRAP, psrReg, instrReg, 0 ));
}

//----------------------------------------------------------------------------------------
// Checks with traps.
//
//----------------------------------------------------------------------------------------
bool T64Cpu::regionIdCheck( uint32_t rId, bool wMode ) {

    if ( extractBit64( psrReg, 0 ) != 0 ) {

        for ( int i = 4; i < 8; i++ ) {

            if ((( extractField64( cRegFile[ i ],  0, 20 ) == rId   ) &&
                 ( extractField64( cRegFile[ i ], 31,  1 ) == wMode )) ||
                (( extractField64( cRegFile[ i ], 32, 20 ) == rId   ) &&
                 ( extractField64( cRegFile[ i ], 63,  1 ) == wMode ))) {

                return( true );
            }
        }

        return( false );        
    }   
}

void T64Cpu::privModeCheck( ) {

    if ( ! extractPsrXbit( psrReg )) privModeOperationTrap( );
}

void T64Cpu::instrAlignmentCheck( T64Word adr ) {

    if ( ! isAlignedDataAdr( adr, 4 )) instrAlignmentTrap( adr );
}

void T64Cpu::instrProtectionCheck( T64Word adr ) {

    if ( ! regionIdCheck( vAdrRegionId( adr ), false )) instrMemProtectionTrap( adr );
}

void T64Cpu::dataAlignmentCheck( T64Word adr, int len ) {

    if ( ! isAlignedDataAdr( adr, len )) dataAlignmentTrap( adr );
}

void T64Cpu::dataProtectionCheck( T64Word adr, bool wMode ) {

    if ( ! regionIdCheck( vAdrRegionId( adr ), wMode )) dataMemProtectionTrap( adr );  
}

void T64Cpu::addOverFlowCheck( T64Word val1, T64Word val2 ) {

    if ( willAddOverflow( val1, val2 )) overFlowTrap( );
}

void T64Cpu::subUnderFlowCheck( T64Word val1, T64Word val2 ) {

    if ( willSubOverflow( val1, val2 )) overFlowTrap( );
}

void T64Cpu::nextInstr( ) {

    nextInstr( );
}

//----------------------------------------------------------------------------------------
// Check address for being in the configured physical memory address range.
// 
//----------------------------------------------------------------------------------------
bool T64Cpu::isPhysMemAdr( T64Word vAdr ) {

    return( isInRange( vAdr, lowerPhysMemAdr, upperPhysMemAdr ));
}

//----------------------------------------------------------------------------------------
// Compare and conditional branch condition evaluation.
//
//----------------------------------------------------------------------------------------
bool T64Cpu::evalCond( int cond, T64Word val1, T64Word val2 ) {
    
    switch ( cond ) {
                        
        case 0: return ( val1           == val2 ); 
        case 1: return  ( val1          <  val2 ); 
        case 2: return  ( val1          >  val2 ); 
        case 3: return  (( val1 & 0x1 ) == val2 ); 
        case 4: return  ( val1          != val2 );
        case 5: return  ( val1          <= val2 );
        case 6: return  ( val1          >= val2 );
        case 7: return  (( val1 & 0x1 ) != val2 );
        default: return( false );
    }
}

//----------------------------------------------------------------------------------------
// Instruction read. This is the central routine that fetches an instruction word. 
// We first check the address range. For a physical address we must be in priv mode.
// For a virtual address, the TLB is consulted for the translation and access 
// control.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::instrRead( T64Word vAdr ) {

    uint32_t instr = 0;

    instrAlignmentCheck( vAdr );

    if ( isInPhysMemAdrRange( vAdr )) { 

        privModeCheck( );
        proc -> iCache -> read( vAdr, (uint8_t *) &instr, 4, false );     
    }
    else {

        T64TlbEntry *tlbPtr = proc -> iTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) instrTlbMissTrap( vAdr );

        instrProtectionCheck( vAdr );
       
        proc -> iCache -> read( tlbPtr -> pAdr, 
                                (uint8_t *) &instr, 
                                4, 
                                tlbPtr -> uncached );
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
T64Word T64Cpu::dataRead( T64Word vAdr, int len, bool sExt ) {

    T64Word data    = 0;
    int     wordOfs = sizeof( T64Word ) - len;

    dataAlignmentCheck( vAdr, len );
   
    if ( isPhysMemAdr( vAdr )) { 
        
        privModeCheck( );
        proc -> dCache -> read( vAdr, ((uint8_t *) &data ) + wordOfs, len, false );
    }
    else {

        T64TlbEntry *tlbPtr = proc -> dTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) {
                
            throw ( T64Trap( DATA_TLB_MISS_TRAP, 0, 0, 0 )); // fix 
        }

        dataProtectionCheck( vAdr, false );
        proc -> iCache -> read( tlbPtr -> pAdr, 
                                ((uint8_t *) &data ) + wordOfs, 
                                len, 
                                tlbPtr -> uncached );
    }

    if ( sExt ) {

        // ??? sign extend
    }

    return( data );
}

//----------------------------------------------------------------------------------------
// Data write. We write the data item to memory. Valid lengths are 1, 2, 4 and 8. The
// data is stored in memory in the length given. We first check the address range. For
// a physical address we must be in priv mode. For a virtual address, the TLB is 
// consulted for the translation and security checking. 
//
//----------------------------------------------------------------------------------------
void T64Cpu::dataWrite( T64Word vAdr, T64Word data, int len ) {

    int wordOfs = sizeof( T64Word ) - len;

    dataAlignmentCheck( vAdr, len );
  
    if ( isPhysMemAdr( vAdr )) { 
        
        privModeCheck( );
        proc -> dCache -> write( vAdr, ((uint8_t *) &data ) + wordOfs, len, false );           
    }
    else {

        T64TlbEntry *tlbPtr = proc -> dTlb -> lookup( vAdr );
        if ( tlbPtr == nullptr ) {
                
            throw ( T64Trap( DATA_TLB_MISS_TRAP, 0, 0, 0 )); // fix 
        }

        dataProtectionCheck( vAdr, true );
        
        proc -> dCache -> write( tlbPtr -> pAdr, 
                                 ((uint8_t *) &data ) + wordOfs, 
                                 len, 
                                 tlbPtr -> uncached );
    }
}

//----------------------------------------------------------------------------------------
// Read memory data based using RegB and the IMM-13 offset to form the address.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::dataReadRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr ); 
    T64Word     ofs     = extractInstrScaledImm13( instr );
    int         len     = 1 << dw;
    
    return( dataRead( addAdrOfs32( adr, ofs ), len, true ));
}

//----------------------------------------------------------------------------------------
// Read memory data based using RegB and the RegX offset to form the address.
//
//----------------------------------------------------------------------------------------
T64Word T64Cpu::dataReadRegBOfsRegX( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = getRegA( instr ) << dw;
    int         len     = 1 << dw;
   
    return( dataRead( addAdrOfs32( adr, ofs ), len, true ));
}

//----------------------------------------------------------------------------------------
// Write data to memory based using RegB and the IMM-13 offset to form the 
// address.
//
//----------------------------------------------------------------------------------------
void T64Cpu::dataWriteRegBOfsImm13( uint32_t instr ) {
    
    T64Word     adr     = getRegB( instr );
    int         dw      = extractInstrDwField( instr );
    T64Word     ofs     = extractInstrScaledImm13( instr );
    int         len     = 1 << dw;
    T64Word     val     = getRegR( instr );
    
    dataWrite( addAdrOfs32( adr, ofs ), val, len );
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
  
    dataWrite( addAdrOfs32( adr, ofs ), val, len );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluAddOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0: {
                        
            T64Word val1 = getRegB( instrReg );
            T64Word val2 = getRegA( instrReg );
            addOverFlowCheck( val1, val2 );
            setRegR( instrReg, val1 + val2 );
            
        } break;
                        
        case 1: {
            
            T64Word val1 = getRegB( instrReg );
            T64Word val2 = extractInstrScaledImm13( instrReg );
            addOverFlowCheck( val1, val2 );
            setRegR( instrReg, val1 + val2 );
            
        } break;
            
        default: illegalInstrTrap( );
    }
                
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemAddOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0: {
            
            T64Word val1 = getRegR( instrReg );
            T64Word val2 = dataReadRegBOfsImm13( instrReg );
            addOverFlowCheck( val1, val2 );
            setRegR( instrReg, val1 + val2 );
            
        } break;
            
        case 1: {
            
            T64Word val1 = getRegR( instrReg );
            T64Word val2 = dataReadRegBOfsRegX( instrReg );

            addOverFlowCheck( val1, val2 );
            setRegR( instrReg, val1 + val2 );
            
        } break;
            
        default: illegalInstrTrap( );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluSubOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0: {
            
            T64Word val1 = getRegB( instrReg );
            T64Word val2 = getRegA( instrReg );
            subUnderFlowCheck( val1, val2 );
            setRegR( instrReg, val1 - val2 );
            
        } break;
            
        case 1: {
            
            T64Word val1 = getRegB( instrReg );
            T64Word val2 = extractInstrImm15( instrReg );
            subUnderFlowCheck( val1, val2 );
            setRegR( instrReg, val1 - val2 );
            
        } break;
            
        default: illegalInstrTrap( );
    }
                
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemSubOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0: {
            
            T64Word val1 = getRegR( instrReg );
            T64Word val2 = dataReadRegBOfsImm13( instrReg );
            subUnderFlowCheck( val1, val2 );
            setRegR( instrReg, val1 - val2 );
            
        } break;
            
        case 1: {
            
            T64Word val1 = getRegR( instrReg );
            T64Word val2 = dataReadRegBOfsRegX( instrReg );
            subUnderFlowCheck( val1, val2 );
            setRegR( instrReg, val1 - val2 );
            
        } break;
            
        default: illegalInstrTrap( );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluAndOp( ) {

    if ( extractInstrBit( instrReg, 19 )) {
                    
        T64Word val1 = getRegB( instrReg );
        T64Word val2 = getRegA( instrReg );
        if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
        T64Word res = val1 & val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    else {
        
        T64Word val1 = getRegB( instrReg );
        T64Word val2 = extractInstrImm15( instrReg );
        if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
        T64Word res = val1 & val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemAndOp( ) {

    if ( extractInstrBit( instrReg, 19 )) {
                    
        T64Word val1 = getRegR( instrReg );
        T64Word val2 = dataReadRegBOfsImm13( instrReg );
        if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
        T64Word res = val1 & val2; 
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    else {
        
        T64Word val1 = getRegR( instrReg );
        T64Word val2 = dataReadRegBOfsRegX( instrReg );
        if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
        T64Word res = val1 & val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluOrOp( ) {

    if ( extractInstrBit( instrReg, 19 )) {
                    
        T64Word val1 = getRegB( instrReg );
        T64Word val2 = getRegA( instrReg );
        if ( extractInstrBit( instrReg, 20 )) val1 = ~ val1;
        T64Word res = val1 | val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    else {
        
        T64Word val1 = getRegB( instrReg );
        T64Word val2 = extractInstrImm15( instrReg );
        if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
        T64Word res = val1 | val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemOrOp( ) {

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
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluXorOp( ) {

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
        T64Word val2 = extractInstrImm15( instrReg ); 
        if ( extractInstrBit( instrReg, 20 ))   val1 = ~ val1;
        T64Word res = val1 ^ val2;
        if ( extractInstrBit( instrReg, 21 )) res = ~ res;
        setRegR( instrReg, res );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemXorOp( ) {

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
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluCmpOp( ) {

     T64Word val1 = getRegB( instrReg );
    T64Word val2 = 0;
    
    if ( extractInstrBit( instrReg, 19 )) val2 = extractInstrImm15( instrReg );
    else                                  val2 = getRegA( instrReg );

    if ( evalCond( extractInstrField( instrReg, 19, 3 ), val1, val2 ))
        setRegR( instrReg, 1 );
    else 
        setRegR( instrReg, 0 );
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemCmpOp( ) {

    T64Word val1 = getRegB( instrReg );
    T64Word val2 = 0;
    T64Word res  = 0;
    
    if ( extractInstrBit( instrReg, 19 )) val2 = dataReadRegBOfsRegX( instrReg );
    else                                  val2 = dataReadRegBOfsImm13( instrReg );
    
    if ( evalCond( extractInstrField( instrReg, 19, 3 ), val1, val2 ))
        setRegR( instrReg, 1 );
    else 
        setRegR( instrReg, 0 );

    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluBitOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0: { 
            
            T64Word val = getRegB( instrReg );
            T64Word res  = 0;
            int     pos  = 0;
            int     len  = extractInstrField( instrReg, 0, 6 );
            
            if ( extractInstrBit( instrReg, 13 ))   
                pos = (int) cRegFile[ CTL_REG_SHAMT ] & 0x3F;
            else                               
                pos = extractInstrField( instrReg, 6, 6 );
            
            if ( extractInstrBit( instrReg, 12 ))  
                res = extractSignedField64( val, (int) pos, len );
            else                               
                res = extractField64( val, (int) pos, len );
            
            setRegR( instrReg, res );
            
        } break;
            
        case 1: {
            
            T64Word val1 = 0;
            T64Word val2 = 0;
            T64Word res  = 0;
            int     pos  = 0;
            int     len  = (int) extractInstrField( instrReg, 0, 6 );
            
            if ( extractInstrBit( instrReg, 13 ))    
                pos = (int) cRegFile[ CTL_REG_SHAMT ] & 0x3F;
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
            
        } break;
            
        case 3: { 
            
            T64Word val1    = getRegB( instrReg );
            T64Word val2    = getRegA( instrReg );
            int     shamt   = 0;
            T64Word res     = 0;
            
            if ( extractInstrBit( instrReg, 13 ))    
                shamt = (int) cRegFile[ CTL_REG_SHAMT ] & 0x3F;
            else                                
                shamt = (int) extractInstrField( instrReg, 6, 6 );
            
            res = shiftRight128( val1, val2, shamt );
            setRegR( instrReg, res );
            
        } break;
            
        default: illegalInstrTrap( );
    }

    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluShaOP( ) {

    T64Word val1  = getRegR( instrReg );
    T64Word val2  = 0;
    T64Word res   = 0;
    int     shamt = (int) extractInstrField( instrReg, 20, 2 );
    
    if ( extractInstrBit( instrReg, 14 )) val2 = extractInstrImm13( instrReg );
    else                                  val2 = getRegB( instrReg );
    
    if ( extractInstrBit( instrReg, 19 )) { 
        
        res = val1 >> shamt;
    }
    else {
        
        if ( willShiftLeftOverflow( val1, shamt )) overFlowTrap( );
        res = val1 << shamt;
    }

    addOverFlowCheck( res, val2 );
    setRegR( instrReg, res + val2 );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluImmOp( ) {

    T64Word val = extractInstrImm20( instrReg );
    T64Word res = getRegR( instrReg );
    
    switch ( extractInstrField( instrReg, 20, 2 )) {
            
        case 0: res = addAdrOfs32( res, val );      break;
        case 1: res = val << 12;                    break;
        case 2: depositField( res, 32, 20, val );   break;
        case 3: depositField( res, 52, 12, val );   break;
    }
    
    setRegR( instrReg, res );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrAluLdoOp( ) {

    T64Word base = getRegB( instrReg );
    T64Word ofs  = extractInstrScaledImm13( instrReg );
    T64Word res  = addAdrOfs32( base, ofs );
    
    setRegR( instrReg, res );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemLdOp( ) {

    T64Word res = 0;
                
    if ( extractInstrField( instrReg, 19, 3 ) == 0 )   
        res = dataReadRegBOfsImm13( instrReg );
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   
        res = dataReadRegBOfsRegX( instrReg );
    else
        illegalInstrTrap( );
    
    setRegR( instrReg, res );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemLdrOp( ) {

    T64Word res = 0;
                
    if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
        illegalInstrTrap( );
    
    res = dataReadRegBOfsImm13( instrReg );
    setRegR( instrReg, res );
    nextInstr( );

    // ??? set reserved flag ?
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemStOp( ) {

    if ( extractInstrField( instrReg, 19, 3 ) == 0 )   
        dataWriteRegBOfsImm13( instrReg );
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   
        dataWriteRegBOfsRegX( instrReg );
    else    
        illegalInstrTrap( );
    
    nextInstr( );

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrMemStcOp( ) {

    if ( extractInstrField( instrReg, 19, 3 ) == 1 ) 
        dataWriteRegBOfsImm13( instrReg );
    else 
        illegalInstrTrap( );
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrBOp( ) {

    T64Word ofs     = extractInstrImm19( instrReg ) << 2;
    T64Word rl      = addAdrOfs32( psrReg, 4 );
    T64Word newIA   = addAdrOfs32( psrReg, ofs );
    
    if ( extractInstrBit( instrReg, 19 )) { 
        
        // ??? gateway check ?
        // ??? priv transfer check ?

        psrReg = newIA;
    }
    else {

        // ??? priv transfer check ?
        
        psrReg = newIA;
    }

    setRegR( instrReg, rl );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrBrOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {

        case 0: {

            T64Word ofs     = getRegB( instrReg ) << 2;
            T64Word rl      = addAdrOfs32( psrReg, 4 );
            T64Word newIA   = addAdrOfs32( psrReg, ofs );
    
            if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
                illegalInstrTrap( );

            instrAlignmentCheck( newIA );
        
            psrReg = newIA;
            setRegR( instrReg, rl );
        
        } break;

        case 1: {

            T64Word base    = getRegB( instrReg );
            T64Word ofs     = getRegA( instrReg );
            T64Word rl      = addAdrOfs32( psrReg, 4 );
            T64Word newIA   = addAdrOfs32( base, ofs );
    
            if ( extractInstrField( instrReg, 19, 3 ) != 0 ) 
                illegalInstrTrap( );

            instrAlignmentCheck( newIA );
            
            psrReg = newIA;
            setRegR( instrReg, rl );
        
        } break;

        default: illegalInstrTrap( );
    }
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrBbOp( ) {

     bool    testVal = extractInstrBit( instrReg, 19 );
    bool    testBit = 0;
    int     pos     = 0;
    
    if ( extractInstrBit( instrReg, 20 ))  
        pos = cRegFile[ CTL_REG_SHAMT ] & 0x3F;
    else                                    
        pos = (int) extractInstrField( instrReg, 13, 6 );
    
    testBit = extractInstrBit( instrReg, pos );
    
    if ( testVal ^ testBit ) { 
        
        psrReg =addAdrOfs32( psrReg, extractInstrImm13( instrReg ) << 2 );
    }
    else nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrAbrOp( ) {

    T64Word val1    = getRegR( instrReg );
    T64Word val2    = getRegB( instrReg );
    T64Word sum     = 0;

    addOverFlowCheck( val1, val2 );
    sum = val1 + val2;
    setRegR( instrReg, sum );

    if ( evalCond( extractInstrField( instrReg, 19, 3 ), sum, 0 ))
        psrReg = addAdrOfs32( psrReg, extractInstrImm15( instrReg ));
    else 
        nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrCbrOp( ) {

    T64Word val1    = getRegR( instrReg );
    T64Word val2    = getRegB( instrReg );

    if ( evalCond( extractInstrField( instrReg, 19, 3 ), val1, val2 ))
        psrReg = addAdrOfs32( psrReg, extractInstrImm15( instrReg ));
    else 
        nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrBrMbrOp( ) {

    T64Word val = getRegB( instrReg );
        
    setRegR( instrReg, val );
    
    if ( evalCond( extractInstrField( instrReg, 19, 3 ), val, 0 ))
        psrReg = addAdrOfs32( psrReg, extractInstrImm15( instrReg ));
    else 
        nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysMrOp( ) {

    switch ( extractInstrField( instrReg, 19, 3 )) {
                        
        case 0:     setRegR( instrReg, 0 ); break; // ??? fix ...
        case 1:     break;

        default:    illegalInstrTrap( );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysLpaOp( ) {

    privModeCheck( );

    T64Word res = 0;
    T64TlbEntry *e = proc -> dTlb -> lookup( getRegB( instrReg ));
    
    if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
        
        
    }
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
        
        
    }
    else illegalInstrTrap( );
    
    setRegR( instrReg, res );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysPrbOp( ) {

    T64Word adr         = getRegB( instrReg );
    bool    privLevel   = false;
    
    if ( extractInstrBit( instrReg, 14 )) 
        privLevel = extractInstrBit( instrReg, 13 );
    else                                   
        privLevel = extractBit64( getRegA( instrReg ), 0 );
    
    T64TlbEntry *e = proc -> dTlb -> lookup( getRegB( instrReg ));
    if ( e != nullptr ) {

        if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
        
            // PRBR
        
        }
        else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
        
            // PRBW
        
        }
        else illegalInstrTrap( );
    }
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysTlbOp( ) {

    // ??? should distinguish between instr and data in the opt1 field ?
    // ??? need a distinction !!!!
    
    if ( extractInstrField( instrReg, 19, 3 ) == 0 ) {

        proc -> dTlb -> insert( getRegB( instrReg ), getRegA( instrReg ));
        setRegR( instrReg, 1 );
    }
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 ) {
        
        proc -> dTlb -> purge( getRegB( instrReg ));
    }
    else illegalInstrTrap( );
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysCaOp( ) {

    // ??? should distinguish between instr and data in the opt1 field ?
    
    if ( extractInstrField( instrReg, 19, 3 ) == 0 ) {

        proc -> dCache -> purge( getRegB( instrReg ));
    }
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 ) {
        
        proc -> dCache -> flush( getRegB( instrReg ));
    }
    else illegalInstrTrap( );
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysMstOp( ) {

    if ( extractInstrField( instrReg, 19, 3 ) == 0 )   {
                    
        // RSM
    }
    else if ( extractInstrField( instrReg, 19, 3 ) == 1 )   {
        
        // SSM
    }
    else illegalInstrTrap( );
    
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysRfiOp( ) {

    // copy data from the CR places...
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysDiagOp( ) {

    T64Word val1 = getRegB( instrReg );
    T64Word val2 = getRegA( instrReg );
    
    // do DIAG word...

    // ??? perhaps have a diagHandler routine ...
    
    setRegR( instrReg, 0 );
    nextInstr( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrSysTrapOp( ) {

    // under construction...
}

//----------------------------------------------------------------------------------------
// Execute an instruction. This is the key routine of the simulator. Essentially
// a big case statement. Each instruction is encoded based on the instruction 
// group and the opcode family. Inside each such cases, the option 1 field 
// ( bits 19 .. 22 ) further qualifies an instruction.
//
//----------------------------------------------------------------------------------------
void T64Cpu::instrExecute( uint32_t instr ) {
    
    try {
        
        int opCode = extractInstrOpCode( instr );
        
        switch ( opCode ) {
                
            case ( OPC_GRP_ALU * 16 + OPC_ADD ):    instrAluAddOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_ADD ):    instrMemAddOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_SUB ):    instrAluSubOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_SUB ):    instrMemSubOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_AND ):    instrAluAndOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_AND ):    instrMemAndOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_OR ):     instrAluOrOp( );    break;
            case ( OPC_GRP_MEM * 16 + OPC_OR ):     instrMemOrOp( );    break;
            case ( OPC_GRP_ALU * 16 + OPC_XOR ):    instrAluXorOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_XOR ):    instrMemXorOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_CMP_A ):  instrAluCmpOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_CMP_B ):  instrAluCmpOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_CMP_A ):  instrMemCmpOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_CMP_B ):  instrMemCmpOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_BITOP ):  instrAluBitOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_SHAOP ):  instrAluShaOP( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_IMMOP ):  instrAluImmOp( );   break;
            case ( OPC_GRP_ALU * 16 + OPC_LDO ):    instrAluLdoOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_LD ):     instrMemLdOp( );    break;
            case ( OPC_GRP_MEM * 16 + OPC_LDR ):    instrMemLdrOp( );   break;
            case ( OPC_GRP_MEM * 16 + OPC_ST ):     instrMemStOp( );    break;
            case ( OPC_GRP_MEM * 16 + OPC_STC ):    instrMemStcOp( );   break;
            case ( OPC_GRP_BR * 16 + OPC_B ):       instrBrBOp( );      break;
            case ( OPC_GRP_BR * 16 + OPC_BR ):      instrBrBrOp( );     break;
            case ( OPC_GRP_BR * 16 + OPC_BB ):      instrBrBbOp( );     break;
            case ( OPC_GRP_BR * 16 + OPC_ABR ):     instrBrAbrOp( );    break;
            case ( OPC_GRP_BR * 16 + OPC_CBR ):     instrBrCbrOp( );    break;
            case ( OPC_GRP_BR * 16 + OPC_MBR ):     instrBrMbrOp( );    break;
            case ( OPC_GRP_SYS * 16 + OPC_MR ):     instrSysMrOp( );    break;
            case ( OPC_GRP_SYS * 16 + OPC_LPA ):    instrSysLpaOp( );   break;
            case ( OPC_GRP_SYS * 16 + OPC_PRB ):    instrSysPrbOp( );   break;
            case ( OPC_GRP_SYS * 16 + OPC_TLB ):    instrSysTlbOp( );   break;
            case ( OPC_GRP_SYS * 16 + OPC_CA ):     instrSysCaOp( );    break;
            case ( OPC_GRP_SYS * 16 + OPC_MST ):    instrSysMstOp( );   break;
            case ( OPC_GRP_SYS * 16 + OPC_RFI ):    instrSysRfiOp( );   break;
            case ( OPC_GRP_SYS * 16 + OPC_DIAG ):   instrSysDiagOp( );  break;
            case ( OPC_GRP_SYS * 16 + OPC_TRAP ):   instrSysTrapOp( );  break;
           
            default: illegalInstrTrap( );
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
        
            instrReg = instrRead( extractField64( psrReg, 0, 52 ));
            instrExecute( instrReg );
    }
    
    catch ( const T64Trap t ) {
        
    }
}