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
#include "T64-Util.h"

//----------------------------------------------------------------------------------------
// The T64System object.
//
//----------------------------------------------------------------------------------------
T64System::T64System( ) {

    moduleTabHwm = 0;

    for ( int i = 0; i < MAX_MODULES; i++ ) {

        T64SystemMapEntry e;
        moduleTab[ i ] = e;
    }
}

//----------------------------------------------------------------------------------------
// Register a module in the module table. This is typically done at simulator start. The
// modules are sorted by their SPA address range they are responsible for.
//
//----------------------------------------------------------------------------------------
int T64System::registerModule(  int             mId,
                                T64ModuleType   mType,
                                T64Word         hpaAdr,
                                T64Word         hpaLen,
                                T64Word         spaAdr,
                                T64Word         spaLen,
                                T64Module       *handler
                              ) {


    if ( moduleTabHwm >= MAX_MODULES ) return( -1 );
    if ( spaLen == 0 ) return( -1 );

    T64Word spaEnd = spaAdr + spaLen;

    int i;

    for ( i = 0; i < moduleTabHwm; ++i ) {

        T64Word  s = moduleTab[ i ].moduleSPA;
        T64Word  e = s + moduleTab[ i ].moduleSPALen;

        if      ( spaEnd <= s ) break;      // No overlap, insert before    
        else if ( spaAdr >= e ) continue;   // No overlap, check next
        else return ( -1 );                 // Overlap found
    }

    // ??? also check for overlapping HPA ?

    // Shift to make space
    for ( int j = moduleTabHwm; j > i; --j ) {

        moduleTab[ j ] = moduleTab[ j - 1 ];
    }

    moduleTab[ i ].moduleTyp        = mType;
    moduleTab[ i ].moduleNum        = mId;
    moduleTab[ i ].moduleHPA        = hpaAdr;    
    moduleTab[ i ].moduleHPALen     = hpaLen;
    moduleTab[ i ].moduleSPA        = spaAdr;
    moduleTab[ i ].moduleSPALen     = spaLen;
    moduleTab[ i ].moduleHandler    = handler;

    moduleTabHwm++;
    return ( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64System::unregisterModule( int moduleNum ) {



    #if 0
    // Removes entry at index. Returns true on success.
    bool remove_range_by_index(RangeTable *table, size_t index) {
    if (index >= table->hwm)
        return false;

    for (size_t j = index + 1; j < table->hwm; ++j) {
        table->entries[j - 1] = table->entries[j];
    }
    table->hwm--;
    return true;
    }
    #endif

    return ( 0 );
}

 void T64System::broadCastEvent( T64ModuleEvent evt ) {

    if ( moduleTabHwm > 0 ) {

        for ( int i = 1; i < moduleTabHwm; i++ ) 
            moduleTab[ i ].moduleHandler -> event( evt );
    }
 }

//----------------------------------------------------------------------------------------
// Find the module handler based on module Id. 
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByNum( int modNum ) {

    if ( moduleTabHwm == 0 ) return ( nullptr );

    for ( int i = 0; i < moduleTabHwm; i++ ) {

         if ( moduleTab[ i ].moduleNum == modNum ) 
            return ( moduleTab[ i ].moduleHandler );
    }

    return( nullptr );
}
    
//----------------------------------------------------------------------------------------
// Find the module handler based on the physical address. There ate HPA and SPA ranges
// to check.
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByAdr( T64Word adr ) {

    if ( moduleTabHwm == 0 ) return ( nullptr );

    for ( int i = 0; i < moduleTabHwm; i++ ) {

        T64SystemMapEntry *p = &moduleTab[ i ];

        if (( adr >= p -> moduleSPA ) && 
            ( adr <= ( p -> moduleSPA + p -> moduleSPALen ))) {

            return( p -> moduleHandler );
        }

        if (( adr >= p -> moduleHPA ) && 
            ( adr <= ( p -> moduleHPA + p -> moduleHPALen ))) {

            return( p -> moduleHandler );
        }
    }

    return( nullptr );
}

//----------------------------------------------------------------------------------------
// Reset the system. We just invoke the handler for each registered module.
//
//----------------------------------------------------------------------------------------
void T64System::reset( ) {

    if ( moduleTabHwm == 0 ) return;

    for ( int i = 0; i < moduleTabHwm; i++ ) {

        moduleTab[ i ].moduleHandler -> reset( );
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
// ??? crucial what tor do here.... explain.
//----------------------------------------------------------------------------------------
void T64System::step( int steps ) {

    if ( moduleTabHwm == 0 ) return;

    for ( int i = 0; i < moduleTabHwm; i++ ) {

        moduleTab[ i ].moduleHandler -> step( );
    }
}

int T64System::readGeneralReg( int proc, int reg, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::writeGeneralReg( int proc, int reg, T64Word val ) {

    return( 0 );
}

int T64System::readControlReg( int proc, int reg, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::writeControlReg( int proc, int reg, T64Word val ) {

    return( 0 );
}

int T64System::readPswReg( int proc, int reg, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::writePswReg( int proc, int reg, T64Word val ) {

    return( 0 );
}

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

}

    // ??? also need BYTE, HALF, WORD, DOUBLE ?
int T64System::readMemWord( T64Word adr, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::writeMemWord( T64Word adr, T64Word val ) {

    return( 0 );
}

int T64System::readMemBlock( T64Word adr, T64Word *blk ) {

    blk[ 0 ] = 0;
    blk[ 1 ] = 0;
    blk[ 2 ] = 0;
    blk[ 3 ] = 0;
    return( 0 );
}

int T64System::writeMemBlock( T64Word adr, T64Word *blk ) {

    return( 0 );
}

int T64System::getHpaStartAdr( int module, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::getHpaSize( int module, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::getSpaStartAdr( int module, T64Word *val ) {

    *val = 0;
    return( 0 );
}

int T64System::getSpaSize( int module, T64Word *val ) {

    *val = 0;
    return( 0 );
}
 
