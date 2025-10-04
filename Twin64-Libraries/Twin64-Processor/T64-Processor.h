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

//----------------------------------------------------------------------------------------
// Forwards.
//
//----------------------------------------------------------------------------------------
struct T64System;
struct T64Processor;

//----------------------------------------------------------------------------------------
// Processor Options. None defined yet. A place holder. 
//
//
//----------------------------------------------------------------------------------------
enum T64Options : uint32_t {

    T64_PO_NIL = 0
};

//----------------------------------------------------------------------------------------
// TLBs. A translation lookaside buffer is essential. We support fully associative 
// TLBs. TLB type is encoded as follows:
//
//  T64_TT_<sets>S
//
//----------------------------------------------------------------------------------------
enum T64TlbType : int {

    T64_TT_NIL = 0,
    T64_TT_64S = 1
};

//----------------------------------------------------------------------------------------
// Caches. Caches are sub modules to the processor. We support set associative caches,
// 2, 4, and 8-way. There is a cache line info with flags and the tag and an array of
// bytes which holds the cache data. Cache type is encoded as follows:
//
//  T64_CT_<ways>W_<sets>S_<words>L
//
//----------------------------------------------------------------------------------------
enum T64CacheType : int {

    T64_CT_NIL         = 0,

    T64_CT_2W_128S_4L  = 1,
    T64_CT_4W_128S_4L  = 2,
    T64_CT_8W_128S_4L  = 3,

    T64_CT_2W_64S_8L   = 4,
    T64_CT_4W_64S_8L   = 5,
    T64_CT_8W_64S_8L   = 6,
};

//----------------------------------------------------------------------------------------
// CPU. The execution unit. We could support several types. So far, we do not.
//
//----------------------------------------------------------------------------------------
enum T64CpuType : int {

    T64_CPU_T_NIL = 0
};

//----------------------------------------------------------------------------------------
// Cache line info consisting of valid, modified and the cache tag.
//
//----------------------------------------------------------------------------------------
struct T64CacheLineInfo {

    bool        valid;
    bool        modified;
    uint32_t    tag;
};

//----------------------------------------------------------------------------------------
// The cache submodule. The CPU can have one or two caches. All access will go through
// the cache submodule, even when the request is a non-cached request. The CPU
// uses the read, write, flush and purge methods for access. The getting a cache line
// method is used by the simulator for displaying cache data. In addition, the cache
// maintains a set of statistics.
//
//----------------------------------------------------------------------------------------
struct T64Cache {

    public:

    T64Cache( T64Processor *proc, T64CacheType cacheType );

    void                reset( );
    void                step( );

    void                read( T64Word pAdr, T64Word *data, int len, bool cached );
    void                write( T64Word pAdr, T64Word data, int len, bool cached );
    void                flush( T64Word pAdr );
    void                purge( T64Word pAdr );

    bool                getCacheLineByIndex( uint32_t          way,
                                             uint32_t          set, 
                                             T64CacheLineInfo  **info,
                                             uint8_t           **data );

    bool                purgeCacheLineByIndex( uint32_t way, uint32_t set );
    bool                flushCacheLineByIndex( uint32_t way, uint32_t set );

    int                 getRequestCount( );
    int                 getHitCount( );
    int                 getMissCount( );
    int                 getWays( );
    int                 getSetSize( );
    int                 getCacheLineSize( );

    private: 

    bool                lookupCache( T64Word          pAdr, 
                                     T64CacheLineInfo **info, 
                                     uint8_t          **data );

    void                readCacheData( T64Word pAdr, T64Word *data, int len );
    void                writeCacheData( T64Word pAdr, T64Word data, int len );
    void                flushCacheLine( T64Word pAdr );
    void                purgeCacheLine( T64Word pAdr );

    T64Word             getCacheLineData( uint8_t *line, 
                                          int     lineOfs,
                                          int     len );

    void                setCacheLineData( uint8_t *line,
                                          int     lineOfs,
                                          int     len,
                                          T64Word data ); 
                                          
    uint32_t            getTag( T64Word pAdr );                           
    uint32_t            getSetIndex( T64Word  paAdr );
    uint32_t            getLineOfs( T64Word  paAdr );
    T64Word             pAdrFromTag( uint32_t tag, uint32_t index );
    int                 plruVictim( );
    void                plruUpdate( );

    private: 

    T64CacheType        cacheType       = T64_CT_NIL;
    T64CacheLineInfo    *cacheInfo      = nullptr;
    uint8_t             *cacheData      = nullptr;
    T64Processor        *proc           = nullptr;

    int                 ways            = 0;
    int                 sets            = 0;
    int                 lineSize        = 0;
    int                 offsetBits      = 0;
    int                 indexBits       = 0;
    T64Word             offsetBitmask   = 0;
    T64Word             indexBitmask    = 0;
    int                 indexShift      = 0;
    int                 tagShift        = 0;
    int                 cacheHits       = 0;
    int                 cacheMiss       = 0;
    uint8_t             plruState       = 0;
};

//----------------------------------------------------------------------------------------
// TLB Entry. The TLB entry stores one translation along with several flags. Each 
// entry has a last used count for the LRU replacement scheme.
//
//----------------------------------------------------------------------------------------
struct T64TlbEntry {

    public:

    bool            valid           = false;
    bool            uncached        = false;
    bool            locked          = false;
    bool            modified        = false;
    bool            trapOnBranch    = false;
    T64Word         vAdr            = 0;
    T64Word         pAdr            = 0;
    int             pSize           = 0;
    uint8_t         accessRights    = 0;
    T64Word         lastUsed        = 0;
};

//----------------------------------------------------------------------------------------
// The TLB submodule. A CPU can have one or two TLBs. Our TLBs are simple arrays of
// entries, i.e. modeling a full associative array with a LRU replacement policy.
// The CPU uses the lookup, insert and purge methods. The simulator uses the methods
// for display and directly inserting or removing an entry.
//
//----------------------------------------------------------------------------------------
struct T64Tlb {
    
    public:

    T64Tlb( T64Processor *proc, T64TlbType tlbType );
    
    void            reset( );
    T64TlbEntry     *lookup( T64Word vAdr );
    
    void            insert( T64Word vAdr, T64Word info );
    void            purge( T64Word vAdr );

    bool            insertTlbByIndex( uint32_t index, T64Word vAdr, T64Word info );
    bool            purgeTlbByIndex( uint32_t index );
    
    int             getTlbSize( );
    T64TlbEntry     *getTlbEntry( int index );

    private:
    
    T64TlbType      tlbType         = T64_TT_NIL;
    T64TlbEntry     *map            = nullptr; 
    int             tlbEntries      = 0;
    T64Word         timeCounter     = 0;
    T64Processor    *proc           = nullptr;
};

//----------------------------------------------------------------------------------------
// The CPU is a execution unit of the processor.
//
//----------------------------------------------------------------------------------------
struct T64Cpu {

    public: 

    T64Cpu( T64Processor *proc, T64CpuType cpuType );

    void            reset( );
    void            step( );

    T64Word         getGeneralReg( int index );
    void            setGeneralReg( int index, T64Word val );

    T64Word         getControlReg( int index );
    void            setControlReg( int index, T64Word val );

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
    
    void            privModeCheck( );
    void            protectionCheck( uint32_t pId, bool wMode );
    void            alignMentCheck( T64Word vAdr, int align );
    
    T64Word         instrRead( T64Word vAdr );

    T64Word         dataRead( T64Word vAdr, int len  );
    T64Word         dataReadRegBOfsImm13( uint32_t instr );
    T64Word         dataReadRegBOfsRegX( uint32_t instr );

    void            dataWrite( T64Word vAdr, T64Word val, int len );
    void            dataWriteRegBOfsImm13( uint32_t instr );
    void            dataWriteRegBOfsRegX( uint32_t instr );

    void            instrExecute( uint32_t instr );

    T64Word         ctlRegFile[ T64_MAX_CREGS ];
    T64Word         genRegFile[ T64_MAX_GREGS ];
    T64Word         pswReg;
    uint32_t        instrReg;
    T64Word         resvReg;

    T64CpuType      cpuType = T64_CPU_T_NIL;
    T64Processor    *proc   = nullptr;
    T64Tlb          *iTlb   = nullptr;
    T64Tlb          *dTlb   = nullptr;
    T64Cache        *iCache = nullptr;
    T64Cache        *dCache = nullptr;

    T64Word         lowerPhysMemAdr     = 0;
    T64Word         upperPhysMemAdr     = 0;
};


//----------------------------------------------------------------------------------------
// The CPU core executes the instructions. The processor has the interfaces called by
// the simulator. This includes interfaces to TLBs and caches. 
//
// ??? a call to "step" must run to completion:
// 
// STEP
//      EXEC
//          SYS.read
//              ... cache / mem work
//          end
//
//          EXEC work
//
//      end
//
// end
//----------------------------------------------------------------------------------------
struct T64Processor : T64Module {
    
    public:
    
    T64Processor( T64System     *sys,
                  int           modNum,
                  T64Options    options,  
                  T64CpuType    cpuType,
                  T64TlbType    iTlbType,
                  T64TlbType    dTlbType,
                  T64CacheType  iCacheType,
                  T64CacheType  dCacheType
                 );
    
    void            reset( );
    void            step( );
    
    T64Word         getGeneralReg( int index );
    void            setGeneralReg( int index, T64Word val );

    T64Word         getControlReg( int index );
    void            setControlReg( int index, T64Word val );

    T64Word         getPswReg( );
    void            setPswReg( T64Word val );

    void            insertInstrTlb( T64Word vAdr, T64Word info );
    void            purgeInstrTlb( T64Word vAdr );
    bool            getInstrTlbEntryByIndex( int index, T64Word *vAdr, T64Word *info );
    int             getInstrTlbEntries( );

    void            insertDataTlb( T64Word vAdr, T64Word info );
    void            purgeDataTlb( T64Word vAdr );
    bool            getDataTlbEntryByIndex( int index, T64Word *vAdr, T64Word *info );
    int             getDataTlbEntries( );

    void            purgeInstrCache( T64Word vAdr );
    void            flushDataCache( T64Word vAdr );
    void            purgeDataCache( T64Word vAdr );

    bool            getICacheLineByIndex( uint32_t          way,
                                          uint32_t          set, 
                                          T64CacheLineInfo  **info,
                                          uint8_t           **data );
   
    bool            getDCacheLineByIndex( uint32_t          way,
                                          uint32_t          set, 
                                          T64CacheLineInfo  **info,
                                          uint8_t           **data );

    bool            readMem( T64Word adr, T64Word *val, int len );
    bool            writeMem( T64Word adr, T64Word val, int len );

    bool            readBlockShared( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            readBlockPrivate( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            writeBlock( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            readWord( int proc, T64Word pAdr, T64Word *word );
    bool            writeWord( int proc, T64Word pAdr, T64Word *word );
        
private:

    friend struct   T64Cpu;

    T64System       *sys                = nullptr;
    T64Cpu          *cpu                = nullptr;
    T64Tlb          *iTlb               = nullptr;
    T64Tlb          *dTlb               = nullptr;
    T64Cache        *iCache             = nullptr;
    T64Cache        *dCache             = nullptr;

    int             modNum              = 0;
    T64Word         instructionCount    = 0;
    T64Word         cycleCount          = 0;

    bool            isPhysicalAdrRange( T64Word vAdr );
    T64Word         lowerPhysMemAdr     = 0;
    T64Word         upperPhysMemAdr     = 0;
};

#endif // T64_Processor_h
