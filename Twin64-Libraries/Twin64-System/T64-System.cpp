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
// Name space for local routines.
//
//----------------------------------------------------------------------------------------
namespace {

bool overlap( T64Module *a, T64Module *b ) {

    T64Word a_end = a -> spaAdr + a -> spaLen;
    T64Word b_end = b -> spaAdr + b -> spaLen;
    return (( a -> spaAdr < b_end ) && ( b -> spaAdr < a_end ));
}

};

//----------------------------------------------------------------------------------------
// The T64System object.
//
//----------------------------------------------------------------------------------------
T64System::T64System( ) {

   initModuleMap( );
}

void T64System::initModuleMap( ) {

     for ( int i = 0; i < MAX_MOD_MAP_ENTRIES; i++ ) {

        T64ModuleMapEntry e;
        moduleMap[ i ] = e;
    }
}

//----------------------------------------------------------------------------------------
// Add to the module map. The entries in the module map are sorted by the SPA 
// address range, which also cannot overlap.
//
//----------------------------------------------------------------------------------------
int T64System::addToModuleMap( T64Module *module ) {

    if ( module -> moduleNum >= MAX_MOD_MAP_ENTRIES ) return( -1 );

    // Check overlap with existing entries
    for ( int i = 0; i < moduleMapHwm; ++i ) {

        if ( overlap( moduleMap[ i ].module, module )) return ( -2 ); 
    }

    // Find insertion position to keep sorted by spaAdr
    int pos = 0;
    while (( pos < moduleMapHwm ) && 
           ( moduleMap[ pos ].spaAdr < module -> spaAdr )) pos++;

    // Shift later entries up
    for ( int i = moduleMapHwm; i > pos; i-- ) moduleMap[ i ] = moduleMap[ i - 1 ];

    // Insert new entry
    moduleMap[ pos ].modNum     = module -> moduleNum;
    moduleMap[ pos ].modType    = module -> moduleTyp; 
    moduleMap[ pos ].hpaAdr     = module -> hpaAdr;
    moduleMap[ pos ].hpaLen     = module -> hpaAdr; 
    moduleMap[ pos ].spaAdr     = module -> spaAdr; 
    moduleMap[ pos ].spaLen     = module -> spaLen; 
    moduleMap[ pos ].module     = module;
   
    moduleMapHwm ++;

    return ( 0 );
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

T64ModuleMapEntry *T64System::getModMapEntry( int index ) {

    if ( index < MAX_MOD_MAP_ENTRIES ) return( &moduleMap[ index ] );
    else return ( nullptr );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByAdr( T64Word adr ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        T64ModuleMapEntry *mPtr = & moduleMap[ i ];

        if (( adr >= mPtr -> hpaAdr ) && ( adr < mPtr -> hpaAdr + mPtr -> hpaLen )) 
            return( mPtr -> module );

         if (( adr >= mPtr -> spaAdr ) && ( adr < mPtr -> spaAdr + mPtr -> spaLen )) 
            return( mPtr -> module ); 
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

//----------------------------------------------------------------------------------------
// Uncached bus operations. We locate the responsible module and let it handle the 
// request.
//
//----------------------------------------------------------------------------------------
bool T64System::busReadUncached( int     srcModNum,
                                 T64Word pAdr, 
                                 uint8_t *data, 
                                 int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    // ??? to do ...

    return( true );
}

bool T64System::busWriteUncached( int     srcModNum,
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    // ??? to do ...

    return( true );
}

//----------------------------------------------------------------------------------------
// Cached coherent bus operation. We first determine the responsible module. For each
// module on the bus that is not the requesting module nor the responsible module
// for the physical address, we issue the call to signal that perhaps a cache 
// coherency operation must take place. Next, we just invoke the responsible module
// to carry out the request. 
//
//----------------------------------------------------------------------------------------
bool T64System::busReadShared( int     srcModNum,
                               T64Word pAdr, 
                               uint8_t *data, 
                               int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    // ??? to do ...

    return( true );
}

bool T64System::busReadPrivate( int     srcModNum,
                                T64Word pAdr, 
                                uint8_t *data, 
                                int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    // ??? to do ...

    return( true );
}

bool T64System::busWrite( int     srcModNum,
                          T64Word pAdr, 
                          uint8_t *data, 
                          int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    // ??? to do ...

    return( true );
}

//----------------------------------------------------------------------------------------
// "readMem" and "writeMem" are routines for the simulator commands and windows to
// access physical memory. We will need to find the handling module and the perform
// the operation. Since there is no requesting module, we mark the requesting module
// parameter with a -1. 
//
//----------------------------------------------------------------------------------------
bool T64System::readMem( T64Word pAdr, uint8_t *data, int len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    return( mPtr -> busReadUncached( -1, pAdr, data, len ));
}

bool T64System::writeMem( T64Word pAdr, uint8_t *data, int len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false );

    return( mPtr -> busWriteUncached( -1, pAdr, data, len ));
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
// specific and not stored at the common module level.
//
//----------------------------------------------------------------------------------------
T64Module::T64Module( T64ModuleType    modType, 
                      int              modNum,
                      T64Word          hpaAdr,
                      int              hpaLen,
                      T64Word          spaAdr,
                      int              spaLen ) {

    this -> moduleTyp       = modType;
    this -> moduleNum       = modNum;
    this -> hpaAdr          = hpaAdr;
    this -> hpaLen          = hpaLen;
    this -> spaAdr          = spaAdr;
    this -> spaLen          = spaLen;
}

T64ModuleType T64Module::getModuleType( ) {

    return( moduleTyp );
}

int T64Module::getModuleNum( ) {

    return( moduleNum );
}