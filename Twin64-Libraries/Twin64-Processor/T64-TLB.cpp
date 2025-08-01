//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - TLB
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Cache
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
#include "T64-Processor.h"
#include "T64-Util.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {



} // namespace


//****************************************************************************************
//****************************************************************************************
//
// TLB
//
//----------------------------------------------------------------------------------------
//
// ??? add HWM ?
//----------------------------------------------------------------------------------------
T64Tlb::T64Tlb( int size ) {
    
    this -> size = size;
    this -> map  = (T64TlbEntry *) calloc( size, sizeof( T64TlbEntry ));
    reset( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Tlb::reset( ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        map[ i ].valid = false;
    }
}


//----------------------------------------------------------------------------------------
//
// ??? look up to the HWM.
//----------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::lookupTlb( T64Word vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) return( ptr );
    }
    
    return( nullptr );
}

//----------------------------------------------------------------------------------------
//
// ??? find free slot. Is it the one after HWM ?
//----------------------------------------------------------------------------------------
void T64Tlb::insertTlb( T64Word vAdr, T64Word info ) {
    
    // ??? to do ...
    
}

//----------------------------------------------------------------------------------------
//
// ??? find entry and remove. Move HWM slot to the freed place, dec HWM.
//----------------------------------------------------------------------------------------
void T64Tlb::purgeTlb( T64Word vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) ptr -> valid = false;
    }
}

//----------------------------------------------------------------------------------------
//
// ??? return entry, regardless of HWM
//----------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::getTlbEntry( int index ) {
    
    if ( isInRange( index, 0, size - 1 ))   return( &map[ index ] );
    else                                    return( nullptr );
}


int T64Tlb::getTlbSize( ) {

    return ( size );
}
