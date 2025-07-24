//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Module
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Processor
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details. You should have received a copy of the GNU General Public
// License along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#ifndef T64_Module_h
#define T64_Module_h

#include "T64-Common.h"

enum T64ModuleType {

    MT_NIL  = 0,
    MT_PROC = 1,
    MT_MEM  = 2,
    MT_IO   = 3
};

//----------------------------------------------------------------------------------------
// The processor can communicate events during their instruction execution. The most 
// important ones are the cache coherency events. They will be handled by any other
// module that has a cache right away. For example, of a processor wants to modify a 
// cache line, it will send a "read exclusive" event, which tells the other modules
// to invalidate their cache line, and perhaps flush it first if they have an exclusive
// copy. This protocol is not very efficient, but will guarantee a consistent memory
// copy.
// 
// Events triggered must also run to completion. A processor broadcasting an event 
// will cause the system to invoke each module. Thus there is also a priority. When 
// two processor for example want to obtain an exclusive copy of a cache line, the 
// latter wins. 
//
//----------------------------------------------------------------------------------------
enum T64ModuleEvent {

    EVT_NIL             = 0,
    EVT_READ_SHARED     = 1,
    EVT_READ_EXCLUSIVE  = 2
};

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
struct T64Module {
    
    public:

    T64Module( );

    virtual void reset( ) = 0;
    virtual void step( ) = 0;
    virtual void event( T64ModuleEvent evt ) = 0;

    private:

};

//----------------------------------------------------------------------------------------
// A module is an entity on the imaginary system bus. It "listens" to three physical 
// memory address area. The hard physical address range, the soft physical address range
// configured and the broadcast address range. 
//
//----------------------------------------------------------------------------------------



// ??? ideas for registers: 

// ??? module HPA has the seg of admin registers. First in HPA.

// 0 - status
// 1 - command
// 2 - HPA address
// 3 - SPA address
// 4 - SPA len
// 5 - number of I/O elements
// 6 - module hardware version 
// 7 - module software version
// 8 - interrupt target ( when sending an interrupt -> processor + mask )

// ?? the HPA also has a the IODC, a piece that describes the IO Module and 
// code to execute module specific functions.

// ??? I/O Element. Allocated in SPA space. Up to 128 bytes in size ->
// ??? 16 Regs. 

// ??? need for a larger I/O element ?

// SPA can be USER mode too and directly mapped to user segments, etc.


#endif // T64_Module_h
