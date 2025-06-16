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
    
    T64PhysMem( T64Word size );
    
    void        reset( );
    T64Word     readMem( T64Word adr, int len, bool signExtend = false );
    void        writeMem( T64Word adr, T64Word arg, int len );
   
private:
    
    T64Word    size = 0;
    uint8_t    *mem = nullptr;
    
};


#endif /* T64_Phys_Mem_h */
