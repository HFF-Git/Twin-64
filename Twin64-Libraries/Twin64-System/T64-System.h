//----------------------------------------------------------------------------------------
//
// Twin-64 - System
//
//----------------------------------------------------------------------------------------
// The T64-System represent the system consisting of several modules. Modules are for
// example processor, memory and I/O modules. The simulator is connected to the system
// which handles all module functions. A program start, the individual modules are 
// registered to the system. Think of a kind of bus where you plug in boards.  
//
//----------------------------------------------------------------------------------------
//
//  Twin-64 - System
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
#ifndef T64_System_h
#define T64_System_h

#include "T64-Common.h"
#include "T64-Util.h"

// ??? on a step, all processor modules advance.
// ??? after that each module is give a change to do processing. I.e. check for
// a key pressed, etc.
// ??? we also need to handle interrupts too...

//----------------------------------------------------------------------------------------
// The architecture defines 64 module on the system bus so far. Typically the number
// of imaginary boards is much smaller. However, a board could have several modules on
// one board. To the software this transparent.
//
//----------------------------------------------------------------------------------------
const int MAX_MODULES = 16;

//----------------------------------------------------------------------------------------
// Modules have a type, submodules a subtype.
//
//----------------------------------------------------------------------------------------
enum T64ModuleType {

    MT_NIL          = 0,
    MT_PROC         = 10,
    MT_CPU_CORE     = 11,
    MT_CPU_CACHE    = 12,
    MT_CPU_TLB      = 13, 
    MT_MEM          = 20,
    MT_MEM_BANK     = 21,
    MT_IO           = 30   
};

//----------------------------------------------------------------------------------------
// Modules have registers in their HPA.
//
//----------------------------------------------------------------------------------------
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

// I/O Element. Allocated in SPA space. Up to 128 bytes in size -> 16 Regs. 
// ??? need for a larger I/O element ?

// SPA can be USER mode too and directly mapped to user segments, etc.





//----------------------------------------------------------------------------------------
// The T64Module object represents and an object in the system. It is the base class
// for all concrete modules and submodules. The module may also get a copy of the 
// address range. 
//
// ??? may be not, it can be retrieved by looking at the address map for the matching
// module... ?
//
//----------------------------------------------------------------------------------------
struct T64Module {
    
    public:

    T64Module( T64ModuleType modType, int modNum, int subModNum );

    virtual void        reset( ) = 0;
    virtual void        step( ) = 0;

    T64ModuleType       getModuleType( );
    int                 getModuleNum( );
    int                 getSubModuleNum( );

    void                setHpaAddressRange( T64Word *startAdr, T64Word len );
    void                setSpaAddressRange( T64Word *startAdr, T64Word len );

    int                 getHpaStartAdr( T64Word *val );
    int                 getHpaSize( T64Word *val );

    int                 getSpaStartAdr( T64Word *val );
    int                 getSpaSize( T64Word *val );

    private:

    T64ModuleType       moduleTyp           = MT_NIL;
    int                 moduleNum           = 0;
    int                 subModuleNum        = 0;
    T64Word             moduleHPA           = 0;
    T64Word             moduleHPALen        = 0;
    T64Word             moduleSPA           = 0;
    T64Word             moduleSPALen        = 0;
};

//----------------------------------------------------------------------------------------
// A system map entry in the system map table. The purpose is to locate the module
// responsible for the physical address range. Note that a module can have none, one
// or more than one ranges.
//
//----------------------------------------------------------------------------------------
struct T64SystemMapEntry {

    T64Word         start   = 0;
    T64Word         len     = 0;
    T64Module       *module = nullptr;
};

//----------------------------------------------------------------------------------------
// A module entry in the module table. A module is an entity on the imaginary system
// bus. It "listens" to three physical memory address area. The hard physical address
// range, the soft physical address range configured and the broadcast address range.
// A module may also contain modules that are just modules for structuring the 
// components of a module. These modules do not necessarily listen to address ranges.
// For example, a processor is a module that listens to the its hard physical address
// range. A CPU is a module that is associated with a processor but does not listen
// to addresses. A TLB is a module associated with a CPU and so on. These associated 
// modules are also called submodules. ALl these modules are in one table indexed by
// the module and submodule key. 
//
// Modules start with the number zero. A module has a module number and a zero sub
// module number, a sub module has a module number and subModule number to identify
// it. The table is sorted by modules and within a module number by submodules. 
// 
//----------------------------------------------------------------------------------------
struct T64ModuleMapEntry {

    int             modNum      = 0;
    int             subModNum   = 0;
    T64Module       *module     = nullptr;
};


//----------------------------------------------------------------------------------------
// A T64 system is a bus where you plug in modules. A module represents an entity such
// as a processor, a memory module, an I/O module and so on. At program start we create
// the module objects and add them to the systemMap and moduleMap. 
//
// ??? think about how the modules would recognize a cache operation ...
//----------------------------------------------------------------------------------------
struct T64System {

    public: 

    T64System( );

    // ??? perhaps have one with SPA and HPA as arguments that just call ...
    // ??? also maybe one that adds a module to both maps ...

    int             addToSystemMap( T64Module  *module,
                                    T64Word    start,
                                    T64Word    len
                                  );

    int             addToModuleMap( T64Module  *module );
    
    T64ModuleType   getModuleType( int modNum, int subModNum = 0 );
    T64Module       *lookupByModNum( int modNum, int subModNum = 0 );
    T64Module       *lookupByAdr( T64Word adr );                    

    void            reset( );
    void            run( );
    void            step( int steps = 1 );

    // ??? these routines should rather go into the simulator ?

    int             readGeneralReg( int proc, int cpu, int reg, T64Word *val );
    int             writeGeneralReg( int proc, int cpu,int reg, T64Word val );

    int             readControlReg( int proc, int cpu,int reg, T64Word *val );
    int             writeControlReg( int proc, int cpu,int reg, T64Word val );

    int             readPswReg( int proc, int cpu,int reg, T64Word *val );
    int             writePswReg( int proc, int cpu,int reg, T64Word val );

    int             readTlbEntry( int proc, int tlb, int index, 
                                    T64Word *info1, T64Word *info2 );

    int             insertTlbEntry( int proc, int tlb, 
                                    T64Word info1, T64Word info2 );

    int             purgeTlbEntry( int proc, int tlb, int index );

    int             readCacheLine( int proc, int cache, int set, int index,
                                    T64Word *line );

    int             flushCacheLine( int proc, int cache, int set, int index );

    int             purgeCacheLine( int proc, int cache, int set, int index );

    bool            readMem( T64Word adr, T64Word *val, int len );
    bool            writeMem( T64Word adr, T64Word val, int len );

    bool            readBlockShared( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            readBlockPrivate( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            writeBlock( int proc, T64Word pAdr, uint8_t *data, int len );
    bool            readWord( int proc, T64Word pAdr, T64Word *word );
    bool            writeWord( int proc, T64Word pAdr, T64Word *word );

    bool            getHpaStartAdr( int module, T64Word *val );
    bool            getHpaSize( int module, T64Word *val );

    bool            getSpaStartAdr( int module, T64Word *val );
    bool            getSpaSize( int module, T64Word *val );
 
    private:

    void            initSystemMap( );
    void            initModuleMap( );

    T64SystemMapEntry   systemMap[ MAX_MODULES ];
    int                 systemMapHwm = 0;

    T64ModuleMapEntry   moduleMap[ MAX_MODULES * 8 ];
    int                 moduleMapHwm = 0; 

    const int       MAX_SYS_MAP_ENTRIES = 
                        sizeof( systemMap ) / sizeof( T64SystemMapEntry );

    const int       MAX_MODULE_MAP_ENTRIES = 
                        sizeof( moduleMap ) / sizeof( T64ModuleMapEntry );
};

#endif