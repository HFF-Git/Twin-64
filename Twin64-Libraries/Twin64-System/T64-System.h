//------------------------------------------------------------------------------
//
// Twin-64 - System
//
//------------------------------------------------------------------------------
// This ...
//
//------------------------------------------------------------------------------
//
//  Twin-64 - System
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
//------------------------------------------------------------------------------
#ifndef T64_System_h
#define T64_System_h

#include "T64-Common.h"
#include "T64-Module.h"
#include "T64-Processor.h"
#include "T64-Memory.h"


// ??? on a step, all processor modules advance.
// ??? not clear if other modules need a "step" too..

// ??? after that each module is give a change to do processing. I.e. check for
// a key pressed, etc.

// ??? we also need to handle interrupts too...


// ??? sequence: create SYSTEM. Register Modules. RESET.

//------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------
const int MAX_MODULES = 16;

//------------------------------------------------------------------------------
// A module entry in the module table. A module has a number and physical 
// address ranges it handles. The handler is the reference to managing object.
//
//------------------------------------------------------------------------------
struct T64SystemMapEntry {

    T64ModuleType   moduleTyp       = MT_NIL;
    int             moduleNum       = 0;
    T64Word         moduleHPA       = 0;
    T64Word         moduleHPALen    = 0;
    T64Word         moduleSPA       = 0;
    T64Word         moduleSPALen    = 0;
    T64Module       *moduleHandler  = nullptr;

};

//------------------------------------------------------------------------------
// A T64 system is a bus where you plug in modules. A module represents an
// entity such as a processor, a memory module, an I/O module and so on. At
// program start we create the module objects and register them. Two routines
// are used to lookup the module object for a module number of managed address.
//
//------------------------------------------------------------------------------
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

    void        reset( );
    void        run( );
    void        step( int steps = 1 );
                
    private:

    T64SystemMapEntry   moduleTab[ MAX_MODULES ];
    int                 moduleTabHwm = 0;

};

#endif