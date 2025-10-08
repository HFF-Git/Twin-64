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
                            T64CacheType    dCacheType,
                            T64Word         hpaAdr, 
                            int             hpaLen,
                            T64Word         spaAdr,
                            int             spaLen ) : 

                            T64Module(      MT_PROC, 
                                            modNum,
                                            hpaAdr,
                                            hpaLen,
                                            spaAdr,
                                            spaLen ) {

    this -> modNum  = modNum;
    this -> sys     = sys;

    cpu     = new T64Cpu( this, cpuType );

    iTlb    = new T64Tlb( this, iTlbType );
    dTlb    = new T64Tlb( this, dTlbType );

    iCache  = new T64Cache( this, T64_CT_INSTR_CACHE, iCacheType );
    dCache  = new T64Cache( this, T64_CT_DATA_CACHE, dCacheType );

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
// Get the reference to the processor components.
//
//----------------------------------------------------------------------------------------
T64Cpu *T64Processor::getCpuPtr( ) {

    return ( cpu );
}

T64Tlb *T64Processor::getITlbPtr( ) {

    return( iTlb );
}

T64Tlb *T64Processor::getDTlbPtr( ) {

    return( dTlb );
}

T64Cache *T64Processor::getICachePtr( ) {

    return( iCache );
}

T64Cache *T64Processor::getDCachePtr( ) {

    return( dCache );
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

bool T64Processor::getInstrTlbEntryByIndex( int index, T64Word *vAdr, T64Word *info ) {

    T64TlbEntry *e = iTlb -> getTlbEntry( index );
    if ( e != nullptr ) {
        

        return( true );
    }
    else {

        *vAdr = 0;
        *info = 0;
        return( false );
    }     
}

int T64Processor::getInstrTlbEntries( ) {

    return( iTlb -> getTlbSize( ));
}

bool T64Processor::getDataTlbEntryByIndex( int index, T64Word *vAdr, T64Word *info ) {

    T64TlbEntry *e = dTlb -> getTlbEntry( index );
    if ( e != nullptr ) {
        

        return( true );
    }
    else {

        *vAdr = 0;
        *info = 0;
        return( false );
    }     
}

int T64Processor::getDataTlbEntries( ) {

    return( dTlb -> getTlbSize( ));
}

//----------------------------------------------------------------------------------------
// Cache routines. Called by the CPU and externally by monitors and debuggers. 
//
//----------------------------------------------------------------------------------------
int T64Processor::getInstrCacheLineSize( ) {

    return ( iCache -> getCacheLineSize( ));
}

int T64Processor::getDataCacheLineSize( ) {

    return( dCache -> getCacheLineSize( ));
}

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
   
bool T64Processor::getDCacheLineByIndex( uint32_t          way,
                                         uint32_t          set, 
                                         T64CacheLineInfo  **info,
                                         uint8_t           **data ) {

    return( dCache -> getCacheLineByIndex( way, set, info, data ));
}

//----------------------------------------------------------------------------------------
// Cache interface routines for requesting system bus operations. Straightforward. 
// The processor offers this facade to the cache subsystem.
//
//----------------------------------------------------------------------------------------
bool T64Processor::readSharedBlock( T64Word pAdr, uint8_t *data, int len ) {

    sys -> busReadSharedBlock( moduleNum, pAdr, data, len );
}

bool T64Processor::readPrivateBlock( T64Word pAdr, uint8_t *data, int len ) {

    sys -> busReadPrivateBlock( moduleNum, pAdr, data, len );
}

bool T64Processor::writeBlock( T64Word pAdr, uint8_t *data, int len ) {

    sys -> busWriteBlock( moduleNum, pAdr, data, len );
}

bool T64Processor::readUncached( T64Word pAdr, uint8_t *data, int len ) {

    if ( isInRange( pAdr, hpaAdr, hpaAdr + hpaLen )) {

        // ??? this is our own HPA...
    }
    else sys -> busReadUncached( moduleNum, pAdr, data, len );
}

bool T64Processor::writeUncached( T64Word pAdr, uint8_t *data, int len ) {

    if ( isInRange( pAdr, hpaAdr, hpaAdr + hpaLen )) {

        // ??? this is our own HPA...
    }
    sys -> busWriteUncached( moduleNum, pAdr, data, len );
}

//----------------------------------------------------------------------------------------
// System Bus operations interface routines. When a module issues a request, any
// other module will be informed. We can now check whether the bus transactions 
// would concern us. For example, when another module requests an exclusive copy 
// of a memory block and we happen to have that block modified in our caches. We 
// will need to flush the data first. If the request concerns us, we will inform 
// the cache about it, so that it can take action if needed. Since we might be the
// source of the request, we will only react when we are not the originator. 
//
// Another scenario is the processor HPA address range. These are uncached requests
// and we will react to them as well.
//
//----------------------------------------------------------------------------------------
bool T64Processor::busReadSharedBlock( int      reqModNum, 
                                       T64Word  pAdr, 
                                       uint8_t  *data, 
                                       int      len ) {

    if ( reqModNum == moduleNum ) return( false );

    // ??? the cache needs to know ...
    return( true );
}

bool T64Processor::busReadPrivateBlock( int     reqModNum, 
                                        T64Word pAdr, 
                                        uint8_t *data, 
                                        int     len ) {

    if ( reqModNum == moduleNum ) return( false );

    // ??? the cache needs to know ...

    return( true );
}

bool T64Processor::busWriteBlock( int     reqModNum, 
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int     len ) {

    if ( reqModNum == moduleNum ) return( false );

    // ??? the cache needs to know ...
   
    return( true );
}

bool T64Processor::busReadUncached( int     reqModNum, 
                                    T64Word pAdr, 
                                    uint8_t *data, 
                                    int     len ) {
    
    if ( reqModNum == moduleNum ) return( false );

    // an uncached read request. we check if it is for our HPA.

    return( true );
}

bool T64Processor::busWriteUncached( int     reqModNum,
                                     T64Word pAdr, 
                                     uint8_t *val, 
                                     int     len ) {

    // an uncached read request. we check if it is for our HPA.
    
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
