//----------------------------------------------------------------------------------------
//
// Twin-64 - System
//
//----------------------------------------------------------------------------------------
// "T64System" is the system we simulate. It consist of a set of modules. A module
// represents a processor, a memory unit, and so on. This of the system as a bus where
// the modules are plugged into.
//
//----------------------------------------------------------------------------------------
//
// Twin-64 - System
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
#include "T64-System.h"


//----------------------------------------------------------------------------------------
// The T64System object.
//
//----------------------------------------------------------------------------------------
T64System::T64System( ) {

   initSystemMap( );
   initModuleMap( );
}

void T64System::initSystemMap( ) {

    for ( int i = 0; i < MAX_SYS_MAP_ENTRIES; i++ ) {

        T64SystemMapEntry e;
        systemMap[ i ] = e;
    }
}

void T64System::initModuleMap( ) {

     for ( int i = 0; i < MAX_MOD_MAP_ENTRIES; i++ ) {

        T64SystemMapEntry e;
        systemMap[ i ] = e;
    }
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64System::addToSystemMap( T64Module  *module,
                               T64Word    start,
                               T64Word    len ) {

    if ( systemMapHwm >= MAX_SYS_MAP_ENTRIES ) return -1; // full
    
    T64Word end = start + len - 1;

    // find insertion point
    int pos = 0;
    while (( pos < systemMapHwm ) && ( systemMap[ pos ].start < start )) {

        pos++;
    }

    // check overlap with previous
    if ( pos > 0 ) {

        T64Word prev_end = systemMap[ pos-1 ].start + systemMap[ pos-1 ].len - 1;
        if ( prev_end >= start ) return -2; // overlap
    }

    // check overlap with next
    if ( pos < systemMapHwm ) {

        T64Word next_start = systemMap[ pos ].start;
        if ( end >= next_start ) return -2; // overlap
    }

    // shift up to insert
    for (int i = systemMapHwm; i > pos; i-- ) {

        systemMap[ i ] = systemMap[ i-1 ];
    }

    // fill in
    systemMap[ pos ].start  = start;
    systemMap[ pos ].len    = len;
    systemMap[ pos ].module = module;
    systemMapHwm++;
    
    return 0;
}

//----------------------------------------------------------------------------------------
// Add to the module map. The slot to use is the module number index. We just set the
// entry from the module data. The high water mark is adjusted of necessary.
//
//----------------------------------------------------------------------------------------
int T64System:: addToModuleMap( T64Module *module ) {

    int             modNum  = module -> getModuleNum( );
    T64ModuleType   modType = module -> getModuleType( );

    if ( modNum < MAX_MOD_MAP_ENTRIES ) {

        moduleMap[ modNum ].modNum  = modNum;
        moduleMap[ modNum ].modType = modType;
        moduleMap[ modNum ].module  = module;
    
        if ( modNum > moduleMapHwm ) moduleMapHwm = modNum + 1;
        return( 0 );
    }
    else return( -1 );
}
    
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByModNum( int modNum ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        T64ModuleMapEntry *ptr = &moduleMap[ i ];
        if ( ptr -> modNum == modNum ) return ptr -> module;
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64ModuleType T64System::getModuleType( int modNum ) {

    T64Module *mod = lookupByModNum( modNum );
    return(( mod != nullptr ) ? mod -> getModuleType( ) : MT_NIL );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByAdr( T64Word adr ) {

    int lo = 0;
    int hi = systemMapHwm - 1;

    while ( lo <= hi ) {

        int mid = ( lo + hi ) / 2;
        auto &entry = systemMap[ mid ];

        int64_t start = entry.start;
        int64_t end   = entry.start + entry.len - 1;

        if      ( adr < start ) hi = mid - 1;
        else if ( adr > end   ) lo = mid + 1;
        else                    return entry.module;
    }

    return nullptr;
}                 

//----------------------------------------------------------------------------------------
// Reset the system. We just invoke the module handler for each registered module.
//
//----------------------------------------------------------------------------------------
void T64System::reset( ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if ( moduleMap -> module != nullptr ) moduleMap[ i ].module -> reset( ); 
    }
}

//----------------------------------------------------------------------------------------
// RUN. The simulator can just run the system. We just enter an endless loop which 
// single steps.
//
//----------------------------------------------------------------------------------------
void T64System::run( ) {

    while ( true ) step( 1 );
}

//----------------------------------------------------------------------------------------
// Single step. 
//
// ??? crucial what to do here.... explain.
//----------------------------------------------------------------------------------------
void T64System::step( int steps ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if ( moduleMap -> module != nullptr ) moduleMap[ i ].module -> step( ); 
    }
}


// ??? this is perhaps too complex. Include file mess...
#if 0
//----------------------------------------------------------------------------------------
//
// ??? used by simulator to access TLB data in a processor.
//----------------------------------------------------------------------------------------
int T64System::readTlbEntry( int proc, int tlb, int index, 
                                T64Word *info1, T64Word *info2 )  {

    *info1 = 0;                                
    *info2 = 0;                                     
    return( 0 );
}

int T64System::insertTlbEntry( int proc, int tlb, 
                                T64Word info1, T64Word info2 ) {

    return( 0 );
}

int T64System::purgeTlbEntry( int proc, int tlb, int index ) {

    return( 0 );
}
#endif

#if 0
//----------------------------------------------------------------------------------------
//
// ??? used by simulator to access cache data in a processor.
//----------------------------------------------------------------------------------------
int T64System::readCacheLine( int proc, int cache, int set, int index, T64Word *line ) {

    line[ 0 ] = 0;
    line[ 1 ] = 0;
    line[ 2 ] = 0;
    line[ 3 ] = 0;
    return( 0 );
}

int T64System::flushCacheLine( int proc, int cache, int set, int index ) {

    return( 0 );
}

int T64System::purgeCacheLine( int proc, int cache, int set, int index ) {

    return( 0 );
}
#endif

//----------------------------------------------------------------------------------------
//
// Used by the simulator to access physical memory. Address must be aligned to len =
//----------------------------------------------------------------------------------------
bool T64System::readMem( T64Word adr, T64Word *val, int len ) {

    T64Module *m = lookupByAdr( adr );

    if ( m != nullptr ) {

        // ??? need a method to invoke ...
    }

    return( 0 );
}

bool T64System::writeMem( T64Word adr, T64Word val, int len ) {

    
    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
bool T64System::readBlockShared( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64System::readBlockPrivate( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64System::writeBlock( int proc, T64Word pAdr, uint8_t *data, int len ) {

    return( true );
}

bool T64System::readWord( int proc, T64Word pAdr, T64Word *word ){

    return( true );
}

bool T64System::writeWord( int proc, T64Word pAdr, T64Word *word ) {

    return( true );
}



//****************************************************************************************
//****************************************************************************************
//
// Module
//----------------------------------------------------------------------------------------
// A module is an object plugged into the imaginary system bus. It has a type and 
// a module number, which is the slot in that bus. Each module has a dedicated memory
// page page in the IO HPA space. The address is easily computed from the slot 
// number. In addition, a module can have several SPA regions. This is however module
// specific. 
//
//----------------------------------------------------------------------------------------
T64Module::T64Module( T64ModuleType modType, int modNum ) {

    this -> moduleTyp       = modType;
    this -> moduleNum       = modNum;
}

T64ModuleType T64Module::getModuleType( ) {

    return( moduleTyp );
}

int T64Module::getModuleNum( ) {

    return( moduleNum );
}

T64Word T64Module::getHpaStartAdr( ) {

    return( T64_IO_HPA_MEM_START + ( moduleNum * T64_PAGE_SIZE_BYTES ));
}

int T64Module::getHpaSize( ) {

    return( T64_PAGE_SIZE_BYTES );
}