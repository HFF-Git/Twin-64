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
// ??? when we have unified caches or TLBs, both submodules refer to the same 
// object.
//----------------------------------------------------------------------------------------
T64Processor::T64Processor( int modNum, T64System *sys ) : 
                T64Module( MT_PROC, modNum, 0 ) {
    
    this -> sys     = sys;

    // ??? should we rather register with the fixed numbers ? Too complex ?

    #if 0
    this -> subModTab[ PSM_CPU ]    = new T64Cpu( this );
    this -> subModTab[ PSM_ITLB ]   = new T64Tlb( );
    this -> subModTab[ PSM_DTLB ]   = new T64Tlb( );
    this -> subModTab[ PSM_ICACHE ] = new T64Cache( T64_CT_2W_128S_4L, sys );
    this -> subModTab[ PSM_DCACHE ] = new T64Cache( T64_CT_4W_128S_4L, sys );
    #endif
    
    this -> reset( );
}

//----------------------------------------------------------------------------------------
// Reset the processor and its submodules.
//
//----------------------------------------------------------------------------------------
void T64Processor::reset( ) {

    T64Module::reset( );

    instructionCount    = 0;
    cycleCount          = 0;

}

//----------------------------------------------------------------------------------------
// The register routines. Called externally by monitors and debuggers.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::getGeneralReg( int index ) {

   return((( T64Cpu *) subModTab[ PSM_CPU ] ) -> getGeneralReg( index ));
}

void T64Processor::setGeneralReg( int index, T64Word val ) {

    (( T64Cpu *) subModTab[ PSM_CPU ] ) -> setGeneralReg( index, val );
}

T64Word T64Processor::getControlReg( int index ) {

    return( (( T64Cpu *) subModTab[ PSM_CPU ] ) -> getControlReg( index ));
}

void T64Processor::setControlReg( int index, T64Word val ) {
    
    (( T64Cpu *) subModTab[ PSM_CPU ] ) -> setControlReg( index, val );
}

T64Word T64Processor::getPswReg( ) {
    
    return( (( T64Cpu *) subModTab[ PSM_CPU ] ) -> getPswReg( ));
}

void T64Processor::setPswReg( T64Word val ) {
    
    (( T64Cpu *) subModTab[ PSM_CPU ] ) -> setPswReg( val );
}

//----------------------------------------------------------------------------------------
// TLB routines. Called by the CPU and externally by monitors and debuggers. 
//
//----------------------------------------------------------------------------------------
void T64Processor::insertInstrTlb( T64Word vAdr, T64Word info ) {

    if ( subModTab[ PSM_ITLB ] != nullptr ) 
        (( T64Tlb *) subModTab[ PSM_ITLB ] ) -> insert( vAdr, info );
    else throw ( 99 );
}

void T64Processor::purgeInstrTlb( T64Word vAdr ) {

    if ( subModTab[ PSM_ITLB ] != nullptr ) 
        (( T64Tlb *) subModTab[ PSM_ITLB ] ) -> purge( vAdr );
    else throw ( 99 );
}

void  T64Processor::insertDataTlb( T64Word vAdr, T64Word info ) {

    if ( subModTab[ PSM_DTLB ] != nullptr ) 
        (( T64Tlb *) subModTab[ PSM_DTLB ] ) -> insert( vAdr, info );
    else throw ( 99 );
}

void T64Processor::purgeDataTlb( T64Word vAdr ) {

   if ( subModTab[ PSM_DTLB ] != nullptr ) 
        (( T64Tlb *) subModTab[ PSM_DTLB ] ) -> purge( vAdr );
    else throw ( 99 );
}

//----------------------------------------------------------------------------------------
// Cache routines. Called by the CPU and externally by monitors and debuggers. 
//
//----------------------------------------------------------------------------------------
void T64Processor::purgeInstrCache( T64Word vAdr ) {

    if ( subModTab[ PSM_ICACHE ] != nullptr ) 
        (( T64Cache *) subModTab[ PSM_ICACHE ] ) -> purge( vAdr );
    else throw ( 99 );
}

void T64Processor::flushDataCache( T64Word vAdr ) {

    if ( subModTab[ PSM_DCACHE ] != nullptr ) 
        (( T64Cache *) subModTab[ PSM_DCACHE ] )-> purge( vAdr );
    else throw ( 99 );
}

void  T64Processor::purgeDataCache( T64Word vAdr ) {

     if ( subModTab[ PSM_DCACHE ] != nullptr ) 
        (( T64Cache *) subModTab[ PSM_DCACHE ] )-> flush( vAdr );
    else throw ( 99 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
bool T64Processor::isPhysicalAdrRange( T64Word vAdr ) {

    return( isInRange( vAdr, lowerPhysMemAdr, upperPhysMemAdr ));
}


//----------------------------------------------------------------------------------------
// The step routine is the entry point to the processor for executing one or more 
// instructions.
//
//----------------------------------------------------------------------------------------
void T64Processor::step( ) {

    try {
        
        (( T64Cpu *) subModTab[ PSM_CPU ] ) -> step( );
    }
    
    catch ( const T64Trap t ) {
        
    }
}

void T64Processor::event( T64ModuleEvent evt ) {


}
