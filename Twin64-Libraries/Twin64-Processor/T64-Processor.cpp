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
#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-System.h"
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
T64Processor::T64Processor( T64System *sys ) {
    
    this -> sys     = sys;

    this -> cpu     = new T64Cpu( this );

    this -> iTlb    = new T64Tlb( );
    this -> dTlb    = new T64Tlb( );

    this -> iCache   = new T64Cache( T64_CT_2W_128S, sys );
    this -> dCache   = new T64Cache( T64_CT_4W_128S, sys );
    
    this -> reset( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Processor::reset( ) {

    instructionCount    = 0;
    cycleCount          = 0;

    cpu -> reset( );
    
    if ( iTlb != nullptr ) iTlb -> reset( );
    if ( dTlb != nullptr ) dTlb -> reset( );
    
    if ( iCache != nullptr ) iCache -> reset( );
    if ( dCache != nullptr ) dCache -> reset( );
}

//----------------------------------------------------------------------------------------
// The register routines. Called externally by monitors and debuggers.
//
//----------------------------------------------------------------------------------------
T64Word T64Processor::getGeneralReg( int index ) {

   return( cpu -> getGeneralReg( index ));
}

void T64Processor::setGeneralReg( int index, T64Word val ) {

    cpu -> setGeneralReg( index, val );
}

T64Word T64Processor::getControlReg( int index ) {

    return( cpu -> getControlReg( index ));
}

void T64Processor::setControlReg( int index, T64Word val ) {
    
    cpu -> setControlReg( index, val );
}

T64Word T64Processor::getPswReg( ) {
    
    return( cpu -> getPswReg( ));
}

void T64Processor::setPswReg( T64Word val ) {
    
    cpu -> setPswReg( val );
}

//----------------------------------------------------------------------------------------
// TLB routines. Called by the CPU and externally by monitors and debuggers. When we
// configured a unified TLB, the data TLB is used.
//
//----------------------------------------------------------------------------------------
void T64Processor::insertInstrTlb( T64Word vAdr, T64Word info ) {

    if      ( iTlb != nullptr ) iTlb -> insert( vAdr, info );
    else if ( dTlb != nullptr ) dTlb -> insert( vAdr, info );
    else throw ( 99 );
}

void T64Processor::purgeInstrTlb( T64Word vAdr ) {

    if      ( iTlb != nullptr ) iTlb -> purge( vAdr );
    else if ( dTlb != nullptr ) dTlb -> purge( vAdr );
    else throw ( 99 );
}

void  T64Processor::insertDataTlb( T64Word vAdr, T64Word info ) {

    if ( dTlb != nullptr ) dTlb -> insert( vAdr, info );
    else throw ( 99 );
}

void T64Processor::purgeDataTlb( T64Word vAdr ) {

   if ( dTlb != nullptr ) dTlb -> purge( vAdr );
    else throw ( 99 );
}

//----------------------------------------------------------------------------------------
// Cache routines. Called by the CPU and externally by monitors and debuggers. When we
// configured a unified cache, the data cache is used.
//
//----------------------------------------------------------------------------------------
void T64Processor::purgeInstrCache( T64Word vAdr ) {

    if      ( iCache != nullptr ) iCache -> purge( vAdr );
    else if ( dCache != nullptr ) dCache -> purge( vAdr );
    else throw ( 99 );
}

void T64Processor::flushDataCache( T64Word vAdr ) {

    if ( dCache != nullptr ) dCache -> flush( vAdr );
    else throw ( 99 );
}

void  T64Processor::purgeDataCache( T64Word vAdr ) {

    if ( dCache != nullptr ) dCache -> purge( vAdr );
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
        
        cpu -> step( );
    }
    
    catch ( const T64Trap t ) {
        
    }
}

void T64Processor::event( T64ModuleEvent evt ) {


}
