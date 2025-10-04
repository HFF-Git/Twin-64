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
// Processor. 
//
//----------------------------------------------------------------------------------------
// A processor is a module with one CPU, TLBs and Caches. We create the component
// objects right here and pass them our instance, such that they have access to 
// these components. Typically, they keep local copies of the references they need.
// 
//----------------------------------------------------------------------------------------
T64Processor::T64Processor( T64System       *sys,
                            int             modNum,
                            T64Options      options,  
                            T64CpuType      cpuType,
                            T64TlbType      iTlbType,
                            T64TlbType      dTlbType,
                            T64CacheType    iCacheType,
                            T64CacheType    dCacheType ) : 
                T64Module( MT_PROC, modNum ) {
    
    this -> modNum  = modNum;
    this -> sys     = sys;

    cpu     = new T64Cpu( this, cpuType );

    iTlb    = new T64Tlb( this, iTlbType );
    dTlb    = new T64Tlb( this, dTlbType );

    iCache  = new T64Cache( this, iCacheType );
    dCache  = new T64Cache( this, dCacheType );

    this -> reset( );
}

//----------------------------------------------------------------------------------------
// Reset the processor and its submodules.
//
//----------------------------------------------------------------------------------------
void T64Processor::reset( ) {

    this -> cpu -> reset( );
    this -> iTlb -> reset( );
    this -> dTlb -> reset( );
    this -> iCache -> reset( );
    this -> dCache -> reset( );

    instructionCount    = 0;
    cycleCount          = 0;
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
// TLB routines. Called by the CPU and externally by monitors and debuggers. 
//
//----------------------------------------------------------------------------------------
void T64Processor::insertInstrTlb( T64Word vAdr, T64Word info ) {

    if ( iTlb != nullptr ) iTlb -> insert( vAdr, info );
    else throw ( 99 );
}

void T64Processor::purgeInstrTlb( T64Word vAdr ) {

    if ( iTlb != nullptr ) iTlb -> purge( vAdr );
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
// Cache routines. Called by the CPU and externally by monitors and debuggers. 
//
//----------------------------------------------------------------------------------------
void T64Processor::purgeInstrCache( T64Word vAdr ) {


    if ( iCache != nullptr ) iCache -> purge( vAdr );
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

bool T64Processor::getICacheLineByIndex( uint32_t          way,
                                   uint32_t          set, 
                                   T64CacheLineInfo  **info,
                                   uint8_t           **data ) {

    return( iCache -> getCacheLineByIndex( way, set, info, data ));
}

bool T64Processor::purgeICacheLineByIndex( uint32_t way, uint32_t set ) {

    return( iCache -> purgeCacheLineByIndex( way, set ));
}
   
bool T64Processor::getDCacheLineByIndex( uint32_t          way,
                                         uint32_t          set, 
                                         T64CacheLineInfo  **info,
                                         uint8_t           **data ) {

    return( dCache -> getCacheLineByIndex( way, set, info, data ));
}

bool T64Processor::purgeDCacheLineByIndex( uint32_t way, uint32_t set ) {

    return( dCache -> purgeCacheLineByIndex( way, set ));
}

bool T64Processor::fushDCacheLineByIndex( uint32_t way, uint32_t set ) {

    return( dCache -> flushCacheLineByIndex( way, set ));
}

//----------------------------------------------------------------------------------------
// relay methods for accessing the system bus.
//
//----------------------------------------------------------------------------------------
bool T64Processor::readMem( T64Word adr, T64Word *val, int len ) {

    return( true );
}

bool T64Processor::writeMem( T64Word adr, T64Word val, int len ) {

    return( true );
}

bool T64Processor::readBlockShared( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64Processor::readBlockPrivate( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64Processor::writeBlock( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64Processor::readWord( int proc, T64Word pAdr, T64Word *word ) {

    return( true );
}

bool T64Processor::writeWord( int proc, T64Word pAdr, T64Word *word ) {

    return( true );
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
