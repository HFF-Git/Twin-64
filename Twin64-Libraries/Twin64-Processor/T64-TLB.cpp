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
#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-System.h"
#include "T64-Processor.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {

    const   int     T64_MAX_TLB_SIZE            = 64;


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
T64Tlb::T64Tlb( T64Processor *proc, T64TlbKind tlbKind, T64TlbType tlbType ) {

    this -> proc    = proc;
    this -> tlbKind = tlbKind;
    this -> tlbType = tlbType;

    switch ( tlbType ) {

        case T64_TT_FA_64S:     tlbEntries = 64; break;
        default:                tlbEntries = 64;
    }

    map = (T64TlbEntry *) malloc( tlbEntries * sizeof( T64TlbEntry ));
    reset( );
}

//----------------------------------------------------------------------------------------
// Reset a TLB.
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
// ??? rather do the address check here ??? then we do not need to distinguish 
// in calls to address translation...
//----------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::lookup( T64Word vAdr ) {

    timeCounter ++;
    
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
       
        if (( ptr -> valid ) && 
            ( isInRange( vAdr, ptr ->vAdr, ptr -> vAdr + ptr -> pSize ))) {
         
            ptr -> lastUsed = timeCounter;
            return( ptr );
        }
    }
    
    return( nullptr );
}

//----------------------------------------------------------------------------------------
// The insert method inserts a new entry. First wr check if the virtual address is 
// in the physical address range. We do not enter such ranges in the TLB. Next, we
// check whether the new entry would overlap an existing virtual address range. If
// there is an overlap, the entry found is invalidated. If none found, find a free 
// entry to use. If none found, we replace the least recently used entry. If all 
// entries are locked, we cannot find a free entry.In this case, we just unlock entry
// zero. Note that this is a rather unlikely case, OS software has to ensure that we
// do not lock all entries.
//
// ??? map info fields to the tlb fields...
//----------------------------------------------------------------------------------------
void T64Tlb::insert( T64Word vAdr, T64Word info ) {

    timeCounter++;

    if ( isInIoAdrRange( vAdr )) return;

    #if 0
    for (int i = 0; i < T64_MAX_TLB_SIZE; i++) {
    
        T64TlbEntry *ptr = &map[i];
        if ( !ptr->valid ) continue;

        T64Word vStart1 = ptr->vAdr;
        T64Word vEnd1   = ptr->vAdr + ptr->pSize - 1;
        T64Word vStart2 = vAdr;
        T64Word vEnd2   = vAdr + 00000 - 1; // fix.... needs to be size...

        if ((vStart1 <= vEnd2) && (vEnd1 >= vStart2)) {
            // overlap detected — replace the old entry
            ptr->valid = false;
        }
    }
    #endif

    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {

        T64TlbEntry *ptr = &map[ i ];
        if ( !ptr->valid ) {

            ptr->valid        = true;
            ptr->uncached     = extractBit64( info, 62 );
            ptr->locked       = extractBit64( info, 61 );
            ptr->modified     = extractBit64( info, 60 );
            ptr->trapOnBranch = extractBit64( info, 59 );
            ptr->vAdr         = vAdr;
            ptr->pAdr         = extractField64( info, 12, 24 );
            ptr->pSize        = 0;
            ptr->lastUsed     = timeCounter;
            return;
        }
    }

    T64TlbEntry *victim = nullptr;
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {

        T64TlbEntry *ptr = &map[i];
        if (( ptr->valid ) && ( !ptr->locked )) {

            if (( victim == nullptr ) || ( ptr->lastUsed < victim->lastUsed )) {

                victim = ptr;
            }
        }
    }

    if ( victim == nullptr ) victim = &map[ 0 ];

    victim->valid        = true;
    victim->uncached     = false;
    victim->locked       = false;
    victim->modified     = false;
    victim->trapOnBranch = false;
    victim->vAdr         = vAdr;
    victim->pAdr         = 0;
    victim->pSize        = 0;
    victim->lastUsed     = timeCounter;
}

//----------------------------------------------------------------------------------------
// Remove the TLB entry that contains the virtual address.
// 
//----------------------------------------------------------------------------------------
void T64Tlb::purge( T64Word vAdr ) {
    
    for ( int i = 0; i < T64_MAX_TLB_SIZE; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && 
            ( isInRange( vAdr, ptr ->vAdr, ptr -> vAdr + ptr -> pSize ))) {
        
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
