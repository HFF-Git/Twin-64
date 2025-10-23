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

    iTlb    = new T64Tlb( this, T64_TK_INSTR_TLB, iTlbType );
    dTlb    = new T64Tlb( this, T64_TK_DATA_TLB, dTlbType );

    iCache  = new T64Cache( this, T64_CK_INSTR_CACHE, iCacheType );
    dCache  = new T64Cache( this, T64_CK_DATA_CACHE, dCacheType );

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
// Cache interface routines for requesting system bus operations. Straightforward. 
// The processor offers this facade to the cache subsystems. We augment the request
// with our module number and pass on to the system bus.
//
//----------------------------------------------------------------------------------------
bool T64Processor::readSharedBlock( T64Word pAdr, uint8_t *data, int len ) {

    return ( sys -> busReadSharedBlock( moduleNum, pAdr, data, len ));
}

bool T64Processor::readPrivateBlock( T64Word pAdr, uint8_t *data, int len ) {

    return ( sys -> busReadPrivateBlock( moduleNum, pAdr, data, len ));
}

bool T64Processor::writeBlock( T64Word pAdr, uint8_t *data, int len ) {

    return ( sys -> busWriteBlock( moduleNum, pAdr, data, len ));
}

bool T64Processor::readUncached( T64Word pAdr, uint8_t *data, int len ) {

    if ( isInRange( pAdr, hpaAdr, hpaAdr + hpaLen )) {

        // ??? this is our own HPA...

        *data = 0;
        return( true );
    }
    else return ( sys -> busReadUncached( moduleNum, pAdr, data, len ));
}

bool T64Processor::writeUncached( T64Word pAdr, uint8_t *data, int len ) {

    if ( isInRange( pAdr, hpaAdr, hpaAdr + hpaLen )) {

        // ??? this is our own HPA...
        
        return( true );
    }
    else return ( sys -> busWriteUncached( moduleNum, pAdr, data, len ));
}

//----------------------------------------------------------------------------------------
// System Bus operations cache interface routines. When a module issues a request,
// any other module will be informed. We can now check whether the bus transactions 
// would concern us.
//
//      busReadSharedBlock:
//
//      Another module is requesting a shared cache block read. If we are a 
//      an observer, we need to check that we do not have the block exclusive.
//      If so and modified, the block is written back to memory and marked as 
//      shared. Note that a modified cache line only applies to a unified or data
//      cache. Since we read into an instruction cache only shared, no need to 
//      check the iCache.
//
//      busReadPrivateBlock:
//      
//      Another module is requesting a private copy. If we are an observer, we
//      need to check that we do not have that copy exclusive or shared. In the 
//      exclusive case, we flush and purge the block. In the shared case we just 
//      purge the block from our cache. Note that this request applies to a data
//      and an instruction cache as well.
//      
//      busWriteBlock:
//
//      Another module is writing back an exclusive copy if its cache block. By
//      definition, we do not own that block in any case.
//
// For all cases, we first check that we are not the originator of that request. 
// Just a little sanity check. Next, we lookup the responsible module by the physical 
// address of the request. If we are not the address range owner, we perform the 
// operations described above on our caches. If we are the owner, we simply carry 
// the request.
//
// A processor cannot be the target of a cache operation. It does not own a physical
// address range other then its HPA address range. And this range can only be accessed
// uncached.
//
//----------------------------------------------------------------------------------------
bool T64Processor::busReadSharedBlock( int      reqModNum, 
                                       T64Word  pAdr, 
                                       uint8_t  *data, 
                                       int      len ) {

    if ( reqModNum == moduleNum ) return( false );

    T64Processor *proc = (T64Processor *) sys -> lookupByAdr( pAdr );
    if ( proc == nullptr ) {

        getDCachePtr( ) -> flush( pAdr );
        return (true );
    }
}

bool T64Processor::busReadPrivateBlock( int     reqModNum, 
                                        T64Word pAdr, 
                                        uint8_t *data, 
                                        int     len ) {

    if ( reqModNum == moduleNum ) return( false );

    T64Processor *proc = (T64Processor *) sys -> lookupByAdr( pAdr );
    if ( proc == nullptr ) {

        getDCachePtr( ) -> purge( pAdr );
        getICachePtr( ) -> purge( pAdr );
        return (true );
    }
}

bool T64Processor::busWriteBlock( int     reqModNum, 
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int     len ) {

    // do nothing,
    return( false );
}

//----------------------------------------------------------------------------------------
// System Bus operations non-cache interface routines. When a module issues a request,
// any other module will be informed. We can now check whether the bus transactions 
// would concern us.
//
//      busReadUncached:
//
//      Another module issued an uncached read. We check wether this concerns our
//      HPA address range. If so, we return the data from the HPA space.
//
//      busWriteUncached:
//
//      Another module issued an uncached write. We check wether this concerns our
//      HPA address range. If so, we update the data in our HPA space.
//    
//    
//
//----------------------------------------------------------------------------------------
bool T64Processor::busReadUncached( int     reqModNum, 
                                    T64Word pAdr, 
                                    uint8_t *data, 
                                    int     len ) {
    
    if ( reqModNum == moduleNum ) return( false );

    T64Processor *proc = (T64Processor *) sys -> lookupByAdr( pAdr );
    if ( proc != nullptr ) {

        // ??? we are the target.
        // ??? if HPA space return the data ...

        return (true );
    }
    else return( false );
}

bool T64Processor::busWriteUncached( int     reqModNum,
                                     T64Word pAdr, 
                                     uint8_t *val, 
                                     int     len ) {

   if ( reqModNum == moduleNum ) return( false );

    T64Processor *proc = (T64Processor *) sys -> lookupByAdr( pAdr );
    if ( proc != nullptr ) {

        // ??? we are the target.
        // ??? if HPA space return the data ...

        return (true );
    }
    else return( false );
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
