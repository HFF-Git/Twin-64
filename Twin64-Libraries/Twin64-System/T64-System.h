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
#include "T64-Module.h"

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
// A module entry in the module table. A module has a number and physical 
// address ranges it handles. The handler is the reference to managing object.
//
//----------------------------------------------------------------------------------------
struct T64SystemMapEntry {

    T64ModuleType   moduleTyp       = MT_NIL;
    int             moduleNum       = 0;
    T64Word         moduleHPA       = 0;
    T64Word         moduleHPALen    = 0;
    T64Word         moduleSPA       = 0;
    T64Word         moduleSPALen    = 0;
    T64Module       *moduleHandler  = nullptr;
};

//----------------------------------------------------------------------------------------
// A T64 system is a bus where you plug in modules. A module represents an entity such
// as a processor, a memory module, an I/O module and so on. At program start we create
// the module objects and register them. Two routines are used to lookup the module 
// object for a module number of managed address.
//
// ??? think about how the modules would recognize a cache operation ...
//----------------------------------------------------------------------------------------
struct T64System {

    public: 

    T64System( );

    int         registerModule( int             mId,
                                T64ModuleType   mType,
                                T64Word         hpaAdr,
                                T64Word         hpaLen,
                                T64Word         spaAdr,
                                T64Word         spaLen,
                                T64Module       *handler
                              );

    int         unregisterModule( int moduleNum );

    T64Module   *lookupByNum( int modNum );
    T64Module   *lookupByAdr( T64Word adr ); 

    // ??? this is a bit tricky, event is not enough ...
    void        broadCastEvent( T64ModuleEvent evt );

    void        reset( );
    void        run( );
    void        step( int steps = 1 );

    int         readGeneralReg( int proc, int reg, T64Word *val );
    int         writeGeneralReg( int proc, int reg, T64Word val );

    int         readControlReg( int proc, int reg, T64Word *val );
    int         writeControlReg( int proc, int reg, T64Word val );

    int         readPswReg( int proc, int reg, T64Word *val );
    int         writePswReg( int proc, int reg, T64Word val );

    int         readTlbEntry( int proc, int tlb, int index, 
                                T64Word *info1, T64Word *info2 );

    int         insertTlbEntry( int proc, int tlb, 
                                T64Word info1, T64Word info2 );

    int         purgeTlbEntry( int proc, int tlb, int index );

    int         readCacheLine( int proc, int cache, int set, int index,
                                T64Word *line );

    int         flushCacheLine( int proc, int cache, int set, int index );

    int         purgeCacheLine( int proc, int cache, int set, int index );

    bool        readMem( T64Word adr, T64Word *val, int len );
    bool        writeMem( T64Word adr, T64Word val, int len );

    bool        readBlockShared( int proc, T64Word pAdr, uint8_t *data, int len );
    bool        readBlockPrivate( int proc, T64Word pAdr, uint8_t *data, int len );
    bool        writeBlock( int proc, T64Word pAdr, uint8_t *data, int len );
    bool        readWord( int proc, T64Word pAdr, T64Word *word );
    bool        writeWord( int proc, T64Word pAdr, T64Word *word );

    bool        getHpaStartAdr( int module, T64Word *val );
    bool        getHpaSize( int module, T64Word *val );

    bool        getSpaStartAdr( int module, T64Word *val );
    bool        getSpaSize( int module, T64Word *val );
 
    private:

    T64SystemMapEntry   moduleTab[ MAX_MODULES ];
    int                 moduleTabHwm = 0;

};

#endif