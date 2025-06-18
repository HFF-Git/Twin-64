//
//  T64-Core.h
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//

#ifndef T64_Cpu_h
#define T64_Cpu_h

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
    void reset( );
    
    T64CacheLine *lookupUpCache( T64Word vAdr );
    void purgeCache( T64Word vAdr );
    void flushCache( T64Word vAdr );
    
    // void insertInCache( ... );
     
    T64CacheLine *getCacheLine( int index );
};


//------------------------------------------------------------------------------------------------------------
//
// ??? how to best store the field data ?
//------------------------------------------------------------------------------------------------------------
struct T64TlbEntry {

    bool            valid;
    
    // some more flags ...
    
    uint8_t         accessId;
    uint32_t        protectId;
    T64Word         vAdr;
    T64Word         pAdr;
};

//------------------------------------------------------------------------------------------------------------
// A CPU needs a TLB. It is vital for address translation. In teh Emulator, we only need one TLB for both
// instruction and data. In the real world, we need to mke sure that we can access from both pipeline stages.
// Our TLB is a simple array of entries, i.e. modelling a full associative array with a LRU replacement
// policy.
//
//------------------------------------------------------------------------------------------------------------
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
//
//
//------------------------------------------------------------------------------------------------------------
struct T64PhysMem {
    
public:
    
    T64PhysMem( T64Word size );
    
    void        reset( );
    T64Word     readMem( T64Word adr, int len, bool signExtend = false );
    void        writeMem( T64Word adr, T64Word arg, int len );
   
private:
    
    T64Word    size = 0;
    uint8_t    *mem = nullptr;
    
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64IoMem {
    
public:
    
    T64IoMem( T64Word size );
    
    void        reset( );
    T64Word     readIoMem( T64Word adr, int len, bool signExtend = false );
    void        writeIoMem( T64Word adr, T64Word arg, int len );
    
private:
    
    T64Word    size = 0;
    
    // how to structure this space...
    
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64Cpu {
    
public:
    
    T64Cpu( T64PhysMem *physMem, T64IoMem *ioMem );
    
    void            reset( );
    void            step( int steps = 1 );
    void            run( );
    
    T64Word         getGeneralReg( int index );
    void            setGeneralReg( int index, T64Word val );
    
    T64Word         getControlReg( int index );
    void            setControlReg( int index, T64Word val );
    
    T64Word         getPswReg( );
    void            setPswReg( T64Word val );
    
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index );
            
private:
    
    void            fetchInstr( );
    void            executeInstr( );

    T64Word         getRegR( uint32_t instr );
    T64Word         getRegB( uint32_t instr );
    T64Word         getRegA( uint32_t instr );
    void            setRegR( uint32_t instr, T64Word val );
    
    T64Word         getImm13( uint32_t instr );
    T64Word         getImm15( uint32_t instr );
    T64Word         getImm19( uint32_t instr );
    T64Word         getImm20U( uint32_t instr );
    
    T64Word         translateAdr( T64Word vAdr );
    
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
    
    T64PhysMem      *physMem    = nullptr;
    T64IoMem        *ioMem      = nullptr;
    T64Tlb          *tlb        = nullptr;
};


#endif /* T64_Cpu_h */
