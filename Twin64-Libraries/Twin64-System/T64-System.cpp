//----------------------------------------------------------------------------------------
//
// Twin-64 - System
//
//----------------------------------------------------------------------------------------
// "T64System" is the system we simulate. It consist of a set of modules. A module
// represents a processor, a memory unit, and so on. This of the system as a bus 
// where the modules are plugged into.
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

//----------------------------------------------------------------------------------------
// Check whether the module number is equal and whether the HPA or SPA address 
// range of two modules overlap. If we are passed the same modules, we will 
// by definition overlap. Modules with an SPA len of zero do never overlap. 
//
//----------------------------------------------------------------------------------------
bool overlap( T64Module *a, T64Module *b ) {

    const T64Word aModuleNum = a -> getModuleNum( );
    const T64Word bModuleNum = b -> getModuleNum( );
    if ( aModuleNum == bModuleNum ) return true;

    const T64Word aSpaLen = a -> getSpaLen( );
    const T64Word bSpaLen = b -> getSpaLen( );
    if (( aSpaLen == 0 ) || ( bSpaLen == 0 )) return ( false );

    const T64Word aSpaStart = a -> getSpaAdr( );
    if ( aSpaLen > UINT64_MAX - aSpaStart ) return ( true );

    const T64Word aSpaEnd = aSpaStart + aSpaLen - 1;

    const T64Word bSpaStart = b -> getSpaAdr( );
    if ( bSpaLen > UINT64_MAX - bSpaStart ) return ( true );

    const T64Word bSpaEnd = bSpaStart + bSpaLen - 1;

    const bool ovlSpa = ( aSpaStart <= bSpaEnd ) && ( aSpaEnd   >= bSpaStart );

    const T64Word aHpaLen   = a -> getHpaLen( );
    const T64Word bHpaLen   = b -> getHpaLen( );
    const T64Word aHpaStart = a -> getHpaAdr( );

    if ( aHpaLen > UINT64_MAX - aHpaStart ) return ( true );

    const T64Word aHpaEnd = aHpaStart + aHpaLen - 1;

    const T64Word bHpaStart = b -> getHpaAdr( );
    if ( bHpaLen > UINT64_MAX - bHpaStart ) return ( true );

    const T64Word bHpaEnd = bHpaStart + bHpaLen - 1;

    const bool ovlHpa = ( aHpaStart <= bHpaEnd ) && ( aHpaEnd   >= bHpaStart );

    return ( ovlSpa || ovlHpa );
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

        moduleMap[ i ] = nullptr;
    }

    moduleMapHwm = 0;
}

//----------------------------------------------------------------------------------------
// Add to the module map. The entries in the module map are sorted by the SPA 
// address range, which also cannot overlap. We look for the insertion position,
// shift all entries up after this position and insert the new entry.
//
// Returns 0 on success, -2 if address range overlap.
//----------------------------------------------------------------------------------------
int T64System::addToModuleMap( T64Module *module ) {

    if (( module -> getModuleNum( ) < 0 ) || 
        ( module -> getModuleNum( ) > MAX_MOD_MAP_ENTRIES )) return ( -1 );

    for ( int i = 0; i < moduleMapHwm; ++i ) {

        if ( overlap( moduleMap[ i ], module )) return ( -2 ); 
    }

    int pos = 0;
    while (( pos < moduleMapHwm ) && 
           ( moduleMap[ pos ] -> getSpaAdr( ) < module -> getSpaAdr( ))) pos++;

    for ( int i = moduleMapHwm; i > pos; i-- ) moduleMap[ i ] = moduleMap[ i - 1 ];

    moduleMap[ pos ] = module;
    moduleMapHwm ++;

    return ( 0 );
}

//----------------------------------------------------------------------------------------
// Remove a module from the module map. The map remains sorted by SPA address.
// We find the module, shift all entries after it down, and decrement HWM.
//
// Returns 0 on success, -1 if not found.
//----------------------------------------------------------------------------------------
int T64System::removeFromModuleMap( T64Module *module ) {

    int pos = -1;

    for ( int i = 0; i < moduleMapHwm; ++i ) {

        if ( moduleMap[ i ] == module ) {
            pos = i;
            break;
        }
    }

    if ( pos < 0 ) return ( -1 );

    for ( int i = pos; i < moduleMapHwm - 1; ++i ) {

        moduleMap[ i ] = moduleMap[ i + 1 ];
    }

    moduleMapHwm--;

    return ( 0 );
}

//----------------------------------------------------------------------------------------
// Find the module entry by its module number.
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByModNum( int modNum ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] != nullptr ) &&
            ( moduleMap[ i ] -> getModuleNum( ) == modNum )) {
                
            return ( moduleMap[ i ] );
        }
    }
    
    return nullptr;
}

//----------------------------------------------------------------------------------------
// Find the module entry that covers the address.
//
//----------------------------------------------------------------------------------------
T64Module *T64System::lookupByAdr( T64Word adr ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        T64Module *mPtr = moduleMap[ i ];

        if (( adr >= mPtr -> getSpaAdr( )) && 
            ( adr <  mPtr -> getSpaAdr( ) + mPtr -> getSpaLen( ))) 
            return ( mPtr ); 

        if (( adr >= mPtr -> getHpaAdr( )) && 
            ( adr <  mPtr -> getHpaAdr( ) + mPtr -> getHpaLen( ))) 
            return ( mPtr );
    }

    return nullptr;
} 

//----------------------------------------------------------------------------------------
// Get the module type.
//
// ??? where used... rather encode where used ?
//----------------------------------------------------------------------------------------
T64ModuleType T64System::getModuleType( int modNum ) {

    T64Module *mod = lookupByModNum( modNum );
    return (( mod != nullptr ) ? mod -> getModuleType( ) : MT_NIL );
}

//----------------------------------------------------------------------------------------
// Reset the system. We just invoke the module handler for each registered module.
//
//----------------------------------------------------------------------------------------
void T64System::reset( ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if ( moduleMap[ i ] != nullptr ) moduleMap[ i ] -> reset( ); 
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
//----------------------------------------------------------------------------------------
void T64System::step( int steps ) {

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if ( moduleMap[ i ] != nullptr ) moduleMap[ i ] -> step( ); 
    }
}

//----------------------------------------------------------------------------------------
// Bus operations. All defined bus operations follow the same basic logic. We 
// first determine the responsible module for the requested data. Before asking 
// the responsible module to execute the request, we will inform all others about
// the upcoming request so that perhaps a cache coherency operation at other 
// processors can take place before we issue the request to the target module. 
// Note that cache coherency also applies to an uncached request, since a module 
// may have cached and modified the data.
//
//----------------------------------------------------------------------------------------
bool T64System::busOpReadUncached( int     reqModNum,
                                 T64Word    pAdr, 
                                 uint8_t    *data, 
                                 int        len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr != nullptr ) return ( false );
 
    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] -> getModuleNum( ) != reqModNum ) && 
            ( moduleMap[ i ] -> getModuleNum( ) != mPtr -> getModuleNum( ))) {

             moduleMap[ i ] -> busOpReadUncached( reqModNum, pAdr, data, len );
        }
    }
    
    return ( mPtr -> busOpReadUncached( reqModNum, pAdr, data, len ));
}

bool T64System::busOpWriteUncached( int     reqModNum,
                                  T64Word pAdr, 
                                  uint8_t *data, 
                                  int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return ( false );

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] -> getModuleNum( ) != reqModNum ) && 
            ( moduleMap[ i ] -> getModuleNum( ) != mPtr -> getModuleNum( ))) {

             moduleMap[ i ] -> busOpWriteUncached( reqModNum, pAdr, data, len );
        }
    }

    return ( mPtr -> busOpWriteUncached( reqModNum, pAdr, data, len ));
}

bool T64System::busOpReadSharedBlock( int     reqModNum,
                                      T64Word pAdr, 
                                      uint8_t *data, 
                                      int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return( false ); 
        
    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] -> getModuleNum( ) != reqModNum ) && 
            ( moduleMap[ i ] -> getModuleNum( ) != mPtr -> getModuleNum( ))) {

             moduleMap[ i ] -> busOpReadSharedBlock( reqModNum, pAdr, data, len );
        }
    }

    return ( mPtr -> busOpReadSharedBlock( reqModNum, pAdr, data, len ));
}

bool T64System::busOpReadPrivateBlock( int     reqModNum,
                                       T64Word pAdr, 
                                       uint8_t *data, 
                                       int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return ( false );

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] -> getModuleNum( ) != reqModNum ) && 
            ( moduleMap[ i ] -> getModuleNum( ) != mPtr -> getModuleNum( ))) {

             moduleMap[ i ] -> busOpReadPrivateBlock( reqModNum, pAdr, data, len );
        }
    }

    return ( mPtr -> busOpReadPrivateBlock( reqModNum, pAdr, data, len ));
}

bool T64System::busOpWriteBlock( int     reqModNum,
                               T64Word pAdr, 
                               uint8_t *data, 
                               int     len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return ( false );

    for ( int i = 0; i < moduleMapHwm; i++ ) {

        if (( moduleMap[ i ] -> getModuleNum( ) != reqModNum ) && 
            ( moduleMap[ i ] -> getModuleNum( ) != mPtr -> getModuleNum( ))) {

             moduleMap[ i ] -> busOpWriteBlock( reqModNum, pAdr, data, len );
        }
    }

    return ( mPtr -> busOpWriteBlock( reqModNum, pAdr, data, len ));
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
    if ( mPtr == nullptr ) return ( false );

    return ( mPtr -> busOpReadUncached( -1, pAdr, data, len ));
}

bool T64System::writeMem( T64Word pAdr, uint8_t *data, int len ) {

    T64Module *mPtr = lookupByAdr( pAdr );
    if ( mPtr == nullptr ) return ( false );

    return ( mPtr -> busOpWriteUncached( -1, pAdr, data, len ));
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
                      T64Word          spaAdr,
                      int              spaLen ) {

    this -> moduleTyp   = modType;
    this -> moduleNum   = modNum;
    this -> hpaAdr      =  T64_IO_HPA_MEM_START + ( modNum * T64_PAGE_SIZE_BYTES );
    this -> hpaLen      = T64_PAGE_SIZE_BYTES;
    this -> spaAdr      = spaAdr;
    this -> spaLen      = spaLen;
    this -> spaLimit    = spaAdr + spaLen - 1;
}

int T64Module::getModuleNum( ) {

    return ( moduleNum );
}

T64ModuleType T64Module::getModuleType( ) {

    return ( moduleTyp );
}

const char *T64Module::getModuleTypeName( ) {

    switch ( moduleTyp ) {

        case MT_PROC:       return ((char *) "PROC" );
        case MT_CPU_CORE:   return ((char *) "CPU" );
        case MT_CPU_TLB:    return ((char *) "TLB"  );
        case MT_CPU_CACHE:  return ((char *) "CACHE" );
        case MT_IO:         return ((char *) "IO" );
        case MT_MEM:        return ((char *) "MEM" );

        case MT_NIL:
        default:            return ((char *) "NIL" );
    }
}

T64Word T64Module::getHpaAdr( ) {

    return ( hpaAdr );
}

int T64Module::getHpaLen( ) {

    return ( hpaLen );
}

T64Word T64Module::getSpaAdr( )  {

    return ( spaAdr );
}

int T64Module::getSpaLen( )  {

    return ( spaLen );
}
