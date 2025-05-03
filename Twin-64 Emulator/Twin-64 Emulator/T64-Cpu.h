//
//  T64-Core.h
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//

#ifndef T64_Cpu_h
#define T64_Cpu_h

#include "T64-Types.h"
#include "T64-Phys-Mem.h"
#include "T64-Io-Mem.h"

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
        this -> trapInfo1 = trapInfo1;
        this -> trapInfo1 = trapInfo1;
    }
    
private:
    
    int trapCode;
    int trapInfo1;
    int trapInfo2;
    int trapInfo3;
};

//------------------------------------------------------------------------------------------------------------
//
// ??? how to best store the field data ?
//------------------------------------------------------------------------------------------------------------
struct T64TlbEntry {

    bool            valid;
    uint8_t         accessId;
    uint32_t        protectId;
    int64_t         vAdr;
    int64_t         pAdr;
};

//------------------------------------------------------------------------------------------------------------
//
// ??? just a simple translation buffer...
//------------------------------------------------------------------------------------------------------------
struct T64Tlb {
    
public:
    
    T64Tlb( int size );
    
    void            reset( );
    T64TlbEntry     *lookupTlb( int64_t vAdr );
    void            purgeTlb( int64_t vAdr );
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index, T64TlbEntry *entry );
    
private:
    
    int             size = 0;
    T64TlbEntry     *map = nullptr;
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64Cpu {
    
public:
    
    T64Cpu( T64PhysMem *mem, T64IoMem *io );
    
    void            reset( );
    void            step( );
    
    void            step( int count );
    
    uint64_t        getGeneralReg( int index );
    void            setGeneralReg( int index, int64_t val );
    
    uint64_t        getControlReg( int index );
    void            setControlReg( int index, int64_t val );
    
    uint64_t        getPswReg( );
    void            setPswReg( int64_t val );
    
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index );
    
private:
    
    void            fetchInstr( );
    void            executeInstr( );
    void            translateAdr( int64_t vAdr, int64_t *pAdr );
    int64_t         dataRead( int64_t vAdr, int len  );
    void            dataWrite( int64_t vAdr, int64_t val, int len );
   
private:
    
    int64_t         ctlRegFile[ MAX_CREGS ];
    int64_t         genRegFile[ MAX_GREGS ];
    int64_t         pswReg;
    int64_t         instrReg;
    int64_t         resvReg;
    
    T64PhysMem      *mem    = nullptr;
    T64IoMem        *io     = nullptr;
    T64Tlb          *tlb    = nullptr;
};


#endif /* T64_Cpu_h */
