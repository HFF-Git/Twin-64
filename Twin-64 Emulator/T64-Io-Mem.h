//
//  T64-Io-Mem.h
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//

#ifndef T64_Io_Mem_h
#define T64_Io_Mem_h

#include "T64-Types.h"

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

#endif /* T64_Io_Mem_h */
