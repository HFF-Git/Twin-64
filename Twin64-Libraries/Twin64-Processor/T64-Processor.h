//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - Processor
//
//------------------------------------------------------------------------------------------------------------
// This ...
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - Processor
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
#ifndef T64_Processor_h
#define T64_Processor_h

#include "T64-Common.h"

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64Trap {
    
public:
    
    T64Trap( int    trapCode,
             int    trapInfo1 = 0,
             int    trapInfo2 = 0,
             int    trapInfo3 = 0 ) {
        
        this -> trapCode  = trapCode;
        this -> trapInfo1 = trapInfo1;
        this -> trapInfo2 = trapInfo1;
        this -> trapInfo3 = trapInfo1;
    }
    
private:
    
    int trapCode;
    int trapInfo1;
    int trapInfo2;
    int trapInfo3;
};

//------------------------------------------------------------------------------------------------------------
// Cache
//
//
// ??? not sure we even need a cache for the Emulator. It is perhaps too slow. However when we have more than
// one cpu thread, the cache protocols are interesting... also for LDR and STC. Firs version, just pass
// through...
//
//------------------------------------------------------------------------------------------------------------
struct T64CacheLine {
    
    bool            valid;
    
    // access rights, status bits
    
    T64Word         vAdr;
    T64Word         pAdr;
    T64Word         line[ 4 ]; // a constant ?
};

struct T64Cache {
    
    T64Cache( );
    void            reset( );
    
    void            readCacheWord( T64Word vAdr, T64Word *data );
    void            writeCacheWord( T64Word vAdr, T64Word *data );
    void            fetchCachLine( T64Word vAdr,  T64Word *data );
    void            purgeCacheLine( T64Word vAdr );
    void            flushCacheLine( T64Word vAdr );
    
    T64CacheLine    *getCacheLine( int index );
};

//------------------------------------------------------------------------------------------------------------
// A CPU needs a TLB. It is vital for address translation. In teh Emulator, we only need one TLB for both
// instruction and data. In the real world, we need to mke sure that we can access from both pipeline stages.
// Our TLB is a simple array of entries, i.e. modelling a full associative array with a LRU replacement
// policy.
//
//------------------------------------------------------------------------------------------------------------
struct T64TlbEntry {

    bool            valid;
    
    // some more flags ...
    
    uint8_t         accessId;
    uint32_t        protectId;
    T64Word         vAdr;
    T64Word         pAdr;
};

struct T64Tlb {
    
public:
    
    T64Tlb( int size );
    
    void            reset( );
    T64TlbEntry     *lookupTlb( T64Word vAdr );
    
    void            insertTlb( T64Word vAdr, T64Word info );
    void            purgeTlb( T64Word vAdr );
    
    T64TlbEntry     *getTlbEntry( int index );
   
private:
    
    int             size = 0;
    T64TlbEntry     *map = nullptr;
};

//------------------------------------------------------------------------------------------------------------
// The CPU core executs the instructions.
//
//------------------------------------------------------------------------------------------------------------
struct T64Processor {
    
public:
    
    T64Processor(  );
    
    void            reset( );
    void            step( int steps = 1 );
    void            run( );
    
    T64Word         getGeneralReg( int index );
    void            setGeneralReg( int index, T64Word val );
    
    T64Word         getControlReg( int index );
    void            setControlReg( int index, T64Word val );
    
    T64Word         getPswReg( );
    void            setPswReg( T64Word val );
    
    void            purgeTlb( T64Word vAdr );
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index );

    void            flushCache( T64Word vAdr );
    void            purgeCache( T64Word vAdr );
    T64CacheLine    *getCacheLine( int index );
            
private:

    T64Word         getRegR( uint32_t instr );
    T64Word         getRegB( uint32_t instr );
    T64Word         getRegA( uint32_t instr );
    void            setRegR( uint32_t instr, T64Word val );
    
    T64Word         getImm13( uint32_t instr );
    T64Word         getImm15( uint32_t instr );
    T64Word         getImm19( uint32_t instr );
    T64Word         getImm20U( uint32_t instr );
    
    T64Word         translateAdr( T64Word vAdr );
    
    void            instrRead( );
    void            instrExecute( );

    T64Word         dataReadRegBOfsImm13( uint32_t instr );
    T64Word         dataReadRegBOfsRegX( uint32_t instr );
    T64Word         dataRead( T64Word vAdr, int len  );
    
    void            dataWriteRegBOfsImm13( uint32_t instr );
    void            dataWriteRegBOfsRegX( uint32_t instr );
    void            dataWrite( T64Word vAdr, T64Word val, int len );
   
private:
    
    T64Word         ctlRegFile[ MAX_CREGS ];
    T64Word         genRegFile[ MAX_GREGS ];
    T64Word         pswReg;
    uint32_t        instrReg;
    T64Word         resvReg;
    
    T64Tlb          *tlb        = nullptr;
};


#endif /* T64_Cpu_h */
