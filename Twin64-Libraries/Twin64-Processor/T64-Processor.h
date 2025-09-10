//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Processor
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Processor
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
#ifndef T64_Processor_h
#define T64_Processor_h

#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-System.h"
#include "T64-Module.h"




//----------------------------------------------------------------------------------------
// Cache
//
//
//----------------------------------------------------------------------------------------
struct T64CacheLine {
    
    bool            valid;
    bool            modified;
    T64Word         tag;
    T64Word         line[ T64_WORDS_PER_CACHE_LINE ];
};


//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
struct T64Cache {

    public:

    T64Cache(  T64System *sys );
    virtual void    reset( );

    int             read( T64Word pAdr, T64Word *data, int len, bool cached );
    int             write( T64Word pAdr, T64Word data, int len, bool cached );
    int             flush( T64Word pAdr );
    int             purge( T64Word pAdr );

    virtual int     readCacheData( T64Word pAdr, T64Word *data, int len ) = 0;
    virtual int     writeCacheData( T64Word pAdr, T64Word data, int len ) = 0;
    virtual int     flushCacheData( T64Word pAdr ) = 0;
    virtual int     purgeCacheData( T64Word pAdr ) = 0;

    int             getRequestCount( );
    int             getMissCount( );

    protected: 

    int             cacheRequests   = 0;
    int             cacheMiss       = 0;

    // routines shared by subclasses

    private: 

    T64System       *sys = nullptr;

};


//----------------------------------------------------------------------------------------
struct T64Cache2W : T64Cache {

public:

    static constexpr int T64_MAX_WAYS = 2;

    T64Cache2W(  T64System *sys );
    void            reset( );

    int             readCacheData( T64Word pAdr, T64Word *data, int len );
    int             writeCacheData( T64Word pAdr, T64Word data, int len );
    int             flushCacheData( T64Word pAdr );
    int             purgeCacheData( T64Word pAdr );


private:

    uint8_t         plruState = 0;
    T64CacheLine    cacheArray [ T64_MAX_WAYS ] [ T64_MAX_CACHE_SETS ];

};

struct T64Cache4W : T64Cache {

public:

    static constexpr int T64_MAX_WAYS = 8;      

    T64Cache4W(  T64System *sys );
    void            reset( );

    int             readCacheData( T64Word pAdr, T64Word *data, int len );
    int             writeCacheData( T64Word pAdr, T64Word data, int len );
    int             flushCacheData( T64Word pAdr );
    int             purgeCacheData( T64Word pAdr );

private:

    uint8_t         plruState = 0;
    T64CacheLine    cacheArray [ T64_MAX_WAYS ] [ T64_MAX_CACHE_SETS ];

};

struct T64Cache8W : T64Cache {

public:

    static constexpr int T64_MAX_WAYS = 8;      

    T64Cache8W(  T64System *sys );
    void            reset( );

    int             readCacheData( T64Word pAdr, T64Word *data, int len );
    int             writeCacheData( T64Word pAdr, T64Word data, int len );
    int             flushCacheData( T64Word pAdr );
    int             purgeCacheData( T64Word pAdr );

private:

    uint8_t         plruState = 0;
    T64CacheLine    cacheArray [ T64_MAX_WAYS ] [ T64_MAX_CACHE_SETS ];
    
};


// ??? cache line state: INVALID, SHARED, EXCLUSIVE, MODIFIED

// ??? createCache( int cacheNum, int cacheSets, int cacheSize, in cacheLineSize );

// ??? for simulator windows
// ??? readCacheLine( int cacheNum, int cacheSet, int cacheIndex, *cacheLine );

// ??? for CPU
// ??? readFromCache( int cacheNum, T64Word vAdr, T64Word pAdr, *data, int len );
// ??? writeToCache( int cacheNum, T64Word vAdr, T64Word pAdr, *data, int len );

// ??? purgeFromCache( int cacheNum, T64Word vAdr, T64Word pAdr );
// ??? flushFromCache(  int cacheNum, T64Word vAdr, T64Word pAdr );


//----------------------------------------------------------------------------------------
// A CPU needs a TLB. It is vital for address translation. In the Emulator, we only 
// need one TLB for both instruction and data. In the real world, we need to make sure
// that we can access from both pipeline stages. Our TLB is a simple array of entries,
// i.e. modeling a full associative array with a LRU replacement policy.
//
//----------------------------------------------------------------------------------------
struct T64TlbEntry {

public:

    bool            valid;
    bool            uncached;
    bool            locked;
    bool            modified;
    bool            trapOnBranch;
   
    T64Word         vAdr;
    T64Word         pAdr;
    int             vSize;

    uint8_t         accessRights; // decode the four bits ?

    T64Word         lastUsed;
};

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------


struct T64Tlb {
    
public:
    
    T64Tlb( );
    
    void            reset( );
    T64TlbEntry     *lookup( T64Word vAdr );
    
    void            insert( T64Word vAdr, T64Word info2 );
    void            purge( T64Word vAdr );
    
    int             getTlbSize( );
    T64TlbEntry     *getTlbEntry( int index );

    // routines for simulator

    int insertTlbEntry( int proc, int tlbNum, T64Word info1, T64Word info2 );
    int purgeTlbEntry( int proc, int tlbNum, int index );

private:
    
    T64TlbEntry     map [ T64_MAX_TLB_SIZE ];
    T64Word         timeCounter = 0;
};




//----------------------------------------------------------------------------------------
// The CPU core executes the instructions. The processor has the interfaces called by
// the simulator. This includes interfaces to TLBs and caches. 
//
//----------------------------------------------------------------------------------------
struct T64Processor : T64Module {
    
public:
    
    T64Processor( T64System *sys );
    
    void            reset( );
    void            step( );
    void            event( T64ModuleEvent evt );

    T64Word         getGeneralReg( int index );
    void            setGeneralReg( int index, T64Word val );

    T64Word         getControlReg( int index );
    void            setControlReg( int index, T64Word val );

    void            insertInstrTlb( T64Word vAdr, T64Word info2 );
    void            purgeInstrTlb( T64Word vAdr );
    T64TlbEntry     *getInstrTlbEntry( int index );

    void            insertDataTlb( T64Word vAdr, T64Word info2 );
    void            purgeDataTlb( T64Word vAdr );
    T64TlbEntry     *getDataTlbEntry( int index );

    void            purgeInstrCache( T64Word vAdr );
    void            flushDataCache( T64Word vAdr );
    void            purgeDataCache( T64Word vAdr );

    T64CacheLine    *getInstrCacheEntry( int set, int index );
    T64CacheLine    *getDataCacheEntry( int set, int index );
    
    T64Word         getPswReg( );
    void            setPswReg( T64Word val );
            
private:

    T64Word         getRegR( uint32_t instr );
    T64Word         getRegB( uint32_t instr );
    T64Word         getRegA( uint32_t instr );
    void            setRegR( uint32_t instr, T64Word val );
    
    T64Word         extractImm13( uint32_t instr );
    T64Word         extractImm15( uint32_t instr );
    T64Word         extractImm19( uint32_t instr );
    T64Word         extractImm20U( uint32_t instr );
    T64Word         extractDwField( uint32_t instr );
    
    bool            isPhysicalAdrRange( T64Word vAdr );
    bool            privModeCheck( );
    bool            protectionCheck( uint32_t pId, bool wMode );
    bool            accessRightCheck( T64Word vAdr, AccRights mode );
    
    T64Word         instrRead( T64Word vAdr );

    T64Word         dataRead( T64Word vAdr, int len  );
    T64Word         dataReadRegBOfsImm13( uint32_t instr );
    T64Word         dataReadRegBOfsRegX( uint32_t instr );

    void            dataWrite( T64Word vAdr, T64Word val, int len );
    void            dataWriteRegBOfsImm13( uint32_t instr );
    void            dataWriteRegBOfsRegX( uint32_t instr );

    void            instrExecute( uint32_t instr );
   
private:
    
    T64Word         ctlRegFile[ MAX_CREGS ];
    T64Word         genRegFile[ MAX_GREGS ];
    T64Word         pswReg;
    uint32_t        instrReg;
    T64Word         resvReg;
    
    T64System       *sys                = nullptr;
    T64Tlb          *tlb                = nullptr;
    T64Cache        *cache              = nullptr;
    T64Word         instructionCount    = 0;
    T64Word         cycleCount          = 0;

    T64Word         lowerPhysMemAdr     = 0;
    T64Word         upperPhysMemAdr     = 0;
};


// ??? a call to "step" must run to completion:
// 
// STEP
//      EXEC
//          SYS.read
//              ... cache / mem work
//          end
//
//          ... other EXEC work
//
//      end
//
// end

#endif // T64_Processor_h
