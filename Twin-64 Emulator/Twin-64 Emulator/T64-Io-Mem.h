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
    
    T64IoMem( int64_t size );
    
    void        reset( );
    int64_t     readMem( int64_t adr, int len, bool signExtend = false );
    void        writeMem( int64_t adr, int64_t arg, int len );
    
private:
    
    int64_t    size = 0;
    
    // how to structure this space...
    
};

#endif /* T64_Io_Mem_h */
