//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - TLB
//
//----------------------------------------------------------------------------------------
// The T64 CPU Simulator has a unified TLB. It is a fully associative TLB with 64
// entries and a LRU mechanism t select replacements.
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - TLB
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
//
//----------------------------------------------------------------------------------------
T64Tlb::T64Tlb( ) {

    reset( );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Tlb::reset( ) {
    
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        map[ i ].valid = false;
    }

    timeCounter = 0;
}

//----------------------------------------------------------------------------------------
// The lookup method checks all valid entries if they cover the virtual address. If
// found we update the last used field and return the entry.
//
//----------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::lookup( T64Word vAdr ) {

    timeCounter ++;
    
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
       
        if (( ptr -> valid ) && 
            ( isInRange( vAdr, ptr ->vAdr, ptr -> vAdr + ptr -> vSize ))) {
         
            ptr -> lastUsed = timeCounter;
            return( ptr );
        }
    }
    
    return( nullptr );
}

//----------------------------------------------------------------------------------------
// The insert method inserts a new entry. If the TLB is full, a victim is selected 
// based on the lastUsed data. 
// 
// ??? a check that we do not insert a physical range ?
//----------------------------------------------------------------------------------------
void T64Tlb::insert( T64Word vAdr, T64Word info ) {

    timeCounter++;

    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];

        if ( ! ptr -> valid ) {

             // ??? fill in ...
            ptr -> valid         = true; 
            ptr -> uncached      = false;
            ptr -> locked        = false;
            ptr -> modified      = false;
            ptr -> trapOnBranch  = false;
            ptr -> vAdr          = vAdr;
            ptr -> pAdr          = 0;
            ptr -> vSize         = 0;

            return;
        }
    }
        
    T64TlbEntry *victim = &map[ 0 ];

    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];

        if (( ! ptr -> valid ) && ( ptr -> lastUsed < timeCounter  )) {

            victim = ptr;
        }
    }
     
    // ??? fill in ...
    victim -> valid         = true; 
    victim -> uncached      = false;
    victim -> locked        = false;
    victim -> modified      = false;
    victim -> trapOnBranch  = false;
    victim -> vAdr          = vAdr;
    victim -> pAdr          = 0;
    victim -> vSize         = 0;
}

//----------------------------------------------------------------------------------------
//
// ??? find entry and remove. Move HWM slot to the freed place, dec HWM.
//----------------------------------------------------------------------------------------
void T64Tlb::purge( T64Word vAdr ) {
    
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && 
            ( isInRange( vAdr, ptr ->vAdr, ptr -> vAdr + ptr -> vSize ))) {
        
            ptr -> valid = false;
        }
    }
}

//----------------------------------------------------------------------------------------
// "getTlbEntry" is used by the simulator for the display routines.
// 
//----------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::getTlbEntry( int index ) {
    
    if ( isInRange( index, 0, T64_MAX_TLB_SIZE - 1 )) return( &map[ index ] );
    else                                              return( nullptr );
}


int T64Tlb::getTlbSize( ) {

    return ( T64_MAX_TLB_SIZE );
}
