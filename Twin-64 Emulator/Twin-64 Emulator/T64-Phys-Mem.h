//
//  T64-Phys-Mem.h
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//

#ifndef T64_Phys_Mem_h
#define T64_Phys_Mem_h

#include "T64-Types.h"

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64PhysMem {
    
public:
    
    T64PhysMem( int64_t size );
    
    void        reset( );
    uint64_t    readMem( int64_t adr, int len, bool signExtend = false );
    void        writeMem( int64_t adr, uint64_t arg, int len );
   
private:
    
    int64_t    size = 0;
    uint8_t    *mem = nullptr;
    
};


#endif /* T64_Phys_Mem_h */
