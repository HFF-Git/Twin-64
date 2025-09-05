//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Cache
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

// We:                          Them:
//                              INV     SHARED      EXCL            MODIFIED                

// READ:        (shared)        -       OK          flush, shared   -

// READ MISS:   (shared)        -       OK          flush, shared   -

// WRITE:       (excl)          -       Purge       flush, purge    flush, purge

// WRITE MISS:  (excl)          -       purge       flush, purge    flush, purge

// FLUSH:                       -

// PRURGE:                      -


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>




//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
//  2-way PRU: repl_state bit meaning: 0 => way0 was used recently (victim = way1)
//                                   1 => way1 was used recently (victim = way0)
//
//----------------------------------------------------------------------------------------
inline int plru2Victim( uint8_t state ) {
    // state bit: 0 -> left recent -> evict right (1)
    return ( state & 1 ) ? 0 : 1;
}

inline uint8_t plru2_update(uint8_t state, int way) {
    // set state to indicate which way was used recently:
    // way==0 => bit = 0, way==1 => bit = 1
    return (uint8_t)(way & 1);
}

//----------------------------------------------------------------------------------------
// PLRU utilities: 4-way 
//
//  3-bit encoding:
//    bit2 = b0 (root)  : 0 = left used recently, 1 = right used recently
//    bit1 = b1 (left)  : 0 = W0 used recently, 1 = W1 used recently
//    bit0 = b2 (right) : 0 = W2 used recently, 1 = W3 used recently
//
//  Victim table (state -> victim):
//    state: 0 1 2 3 4 5 6 7
//    victim:3 2 3 2 1 1 0 0
//  Access update (way -> new_state):
//    way 0 -> 0
//    way 1 -> 2
//    way 2 -> 4
//    way 3 -> 5
//
//----------------------------------------------------------------------------------------
inline static int plru4Victim( uint8_t s ) {

    // bit0 = root (0→left, 1→right)
    // bit1 = left branch (0→way3, 1→way2)
    // bit2 = right branch (0→way1, 1→way0)

    if ((( s >> 0 ) & 1 ) == 0 ) return ((( s >> 1 ) & 1 ) == 0 ) ? 3 : 2;
    else                         return ((( s >> 2 ) & 1 ) == 0 ) ? 1 : 0;    
}

inline static uint8_t plru4Update(uint8_t state, int way) {
        
    uint8_t s = state & 0x07; // only 3 bits used
        
    switch ( way & 3 ) {
        
        case 0: s |= 1;     s |= 4;     break;
        case 1: s |= 1;     s &= ~4;    break;
        case 2: s &= ~1;    s |= 2;     break;
        case 3: s &= ~1;    s &= ~2;    break;
        }

        return ( s );
};

//----------------------------------------------------------------------------------------
// PLRU utilities: 8-way ------------------------------
//
//  8-way tree PLRU uses 7 bits (nodes):
//    nodes indexed as:
//      node 0: root (splits [0..3] left and [4..7] right)
//      node 1: left child of root (splits [0..1] and [2..3])
//      node 2: right child of root (splits [4..5] and [6..7])
//      node 3: left-left node (splits 0 vs 1)
//      node 4: left-right node (splits 2 vs 3)
//      node 5: right-left node (splits 4 vs 5)
//      node 6: right-right node (splits 6 vs 7)
//
//  Convention: bit == 0  => left subtree/leaf was used recently
//              bit == 1  => right subtree/leaf was used recently
//
//  Victim selection: at each node, follow the NOT-recent direction:
//    if bit==0 (left recent) -> victim is in right subtree
//    if bit==1 (right recent) -> victim is in left subtree
//
//----------------------------------------------------------------------------------------
static inline int plru8Victim( uint8_t s ) {

    // s: low 7 bits used: bit0=node0(root), bit1=node1, bit2=node2, bit3=node3, bit4=node4, bit5=node5, bit6=node6
    // follow rule: if node == 0 -> left recent -> pick right (node 2) etc.

    // root:
    if ((( s >> 0 ) & 1 ) == 0 ) {
        
        // left recent -> victim in right subtree (ways 4..7)
        if ((( s >> 2 ) & 1 ) == 0 ) {
            
            // node2 == 0 => left recent within right subtree -> victim in right child of node2 (ways 6..7)
            if ((( s >> 6 ) & 1 ) == 0 )  return 7; // node6==0 => left recent => victim right => way7
            else                          return 6; // node6==1 => right recent => victim left => way6
        } else {

            // node2 == 1 => right recent within right subtree -> victim in left child (ways 4..5)
            if ((( s >> 5 ) & 1 ) == 0 )  return 5; // node5==0 => left recent => victim right => way5
            else                          return 4; // node5==1 => right recent => victim left => way4
        }
    } else {
        
        // root bit == 1 => right recent -> victim in left subtree (ways 0..3)
        if ((( s >> 1 ) & 1 ) == 0 ) {
            
            // node1==0 => left recent -> victim in right child (ways 2..3)
            if ((( s >> 4 ) & 1 ) == 0 )  return 3;
            else                          return 2;

        } else {
            
            // node1==1 => right recent -> victim in left child (ways 0..1)
            if ((( s >> 3 ) & 1 ) == 0 )  return 1;
            else                          return 0;
        }
    }
}

static inline uint8_t plru8Update( uint8_t state, int way ) {

    // set bits along the path to mark the accessed side as recent.
    // convention: bit == 0 => left recent, bit == 1 => right recent

    uint8_t s = state & 0x7F; // mask 7 bits

    switch ( way & 0x7 ) {

        // root left, node1 left, node3 left
        case 0: s &= ~1;    s &= ~2;    s &= ~8;    break;

        // root left, node1 left, node3 right
        case 1: s &= ~1;    s &= ~2;    s |= 8;     break;

         // root left, node1 right, node4 left
        case 2: s &= ~1;    s |= 2;     s &= ~16;   break;

        // root left, node1 right, node4 right
        case 3: s &= ~1;    s |= 2;     s |= 16;    break;

        // root right, node2 left, node5 left
        case 4: s |= 1;     s &= ~4;    s &= ~32;   break;

        // root right, node2 left, node5 right
        case 5: s |= 1;     s &= ~4;    s |= 32;    break;

        // root right, node2 right, node6 left
        case 6: s |= 1;     s |= 4;     s &= ~64;   break;

        // root right, node2 right, node6 right
        case 7: s |= 1;     s |= 4;     s |= 64;    break;
    }

    return s;
}



} // namespace


//****************************************************************************************
//****************************************************************************************
//
// Cache
//
//----------------------------------------------------------------------------------------
//
//
// ??? maybe this becomes an abstract class, the word is done in the subclasses...
//----------------------------------------------------------------------------------------
T64Cache::T64Cache( ) {

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cache::reset( ) {

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::read( T64Word pAdr, T64Word *data, int len, bool cached ) {

    // check sets, index is upper 6bits in page offset
    // move according to len
    // if uncached, bypass cache and call system interface

    // if not found, read shared / private ?
    // if not found and no room, pick victim to purge first
    // if modified flush first
    // read block and return data

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::write( T64Word pAdr, T64Word data, int len, bool cached ) {

    // check sets, index is upper 6bits in page offset
    // move according to len
    // if uncached, bypass cache and call system interface

    // if found and shared, mark private
    // if not found, read private
    // if not found and no room, pick victim to purge first
    // if modified flush first
    // read private and modify

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::flush( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back, mark as read shared

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::purge( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back
    // invalidate line

    return( 0 );
}



//****************************************************************************************
//****************************************************************************************
//
// Cache - pass through
//
//----------------------------------------------------------------------------------------
// Actually, this is not a cache. It is simply a pass through for system that do not have
// a cache. We directly call the system interfaces.
//
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64CachePt::reset( ) {

    // not a lot to do.

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64CachePt::read( T64Word pAdr, T64Word *data, int len, bool cached ) {

    // call system directly for the data. 
   // we fetch a word in any case from MEM

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64CachePt::write( T64Word pAdr, T64Word data, int len, bool cached ) {

   // call system directly for the data. 
   // we fetch / modify and store the word in any case from MEM

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64CachePt::flush( T64Word pAdr ) {

   // no cache

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64CachePt::purge( T64Word pAdr ) {

    // no cache

    return( 0 );
}

//****************************************************************************************
//****************************************************************************************
//
// Cache - 2-way set associative
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------

// what can we make shared functions ?

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cache2W::reset( ) {

    // clear sets

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache2W::read( T64Word pAdr, T64Word *data, int len, bool cached ) {

    // check sets, index is upper 6bits in page offset
    // move according to len
    // if uncached, bypass cache and call system interface

    // if not found, read shared / private ?
    // if not found and no room, pick victim to purge first
    // if modified flush first
    // read block and return data

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache2W::write( T64Word pAdr, T64Word data, int len, bool cached ) {

    // check sets, index is upper 6bits in page offset
    // move according to len
    // if uncached, bypass cache and call system interface

    // if found and shared, mark private
    // if not found, read private
    // if not found and no room, pick victim to purge first
    // if modified flush first
    // read private and modify

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache2W::flush( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back, mark as read shared

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache2W::purge( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back
    // invalidate line

    return( 0 );
}

