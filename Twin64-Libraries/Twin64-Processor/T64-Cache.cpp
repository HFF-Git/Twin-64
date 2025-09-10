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
//
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
// 7-bit encoding:
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

void clearCacheLine( T64CacheLine *cl ) {

    cl -> valid     = false;
    cl -> modified  = false;
    cl -> tag       = 0;

    for ( int i = 0; i < T64_WORDS_PER_CACHE_LINE; i++ ) cl -> line[ i ] = 0;
}

int getLineIndex( T64Word pAdr ) {

    return(( pAdr >> T64_LINE_OFS_BITS ) & (( 1 << T64_CACHE_INDEX_BITS ) - 1 ));
}




} // namespace


//****************************************************************************************
//****************************************************************************************
//
// Cache
//
//----------------------------------------------------------------------------------------
// "T64Cache" is the abstract class for the T64 caches.
//
//----------------------------------------------------------------------------------------
T64Cache::T64Cache( int ways, T64System *sys ) { 

    this -> sys = sys;
    this -> ways = ways;

    if (( ways != 2 ) && ( ways != 4 ) && ( ways != 8 )) ways = 4;

    cacheArray = (T64CacheLine *) malloc ( ways * sets * sizeof( T64CacheLine ));
}

//----------------------------------------------------------------------------------------
// The set associative cache uses a pseudo LRU scheme.
//
//----------------------------------------------------------------------------------------
int T64Cache::plruVictim( uint8_t state ) {

    switch( ways ) {

        case 2: break;
        case 4: break;
        case 8: break;
    }

    return( 0 );
}

uint8_t T64Cache::plruUpdate( uint8_t state, int way ) {

    switch( ways ) {

        case 2: break;
        case 4: break;
        case 8: break;
    }

    return( 0 );
}

//----------------------------------------------------------------------------------------
// "lookup" searches the cache sets. If we find a valid cache line with the matching
// tag, we return this line.
//
//----------------------------------------------------------------------------------------
T64CacheLine *T64Cache::lookup( T64Word pAdr ) {

    T64Word ofsMask = ( 1 << T64_LINE_OFS_BITS ) - 1;
    T64Word setMask = ( 1 << T64_CACHE_INDEX_BITS ) - 1;
    int     index   = ( pAdr >> T64_LINE_OFS_BITS ) & setMask;
    T64Word tag     = pAdr & ofsMask;

    for ( int w = 0; w < ways; w++ ) {

        T64CacheLine *l = &cacheArray[ w * T64_MAX_CACHE_SETS ] + index;
        if (( l -> valid ) && ( l -> tag == tag )) return ( l );
    }

    return( nullptr );
}

//----------------------------------------------------------------------------------------
// "getCacheLineData" copies data from the cache line. We expect a valid len argument.
// The pAdr parameter contains the full address from which we derive the alignment and
// offset into the cache line.
//
//----------------------------------------------------------------------------------------
T64Word T64Cache::getCacheLineData( T64CacheLine *cLine,
                                    T64Word      pAdr, 
                                    int          len ) {

    T64Word ofsMask = ( 1 << T64_LINE_OFS_BITS ) - 1;
    int     lOfs    = pAdr & ofsMask;
    int     wOfs    = sizeof( T64Word ) - len;

    T64Word tmp;
    memcpy((uint8_t *) &cLine -> line[ lOfs ], 
           ((uint8_t *) &tmp + wOfs ), 
           len );

    return( tmp );
}

//----------------------------------------------------------------------------------------
// "setCacheLineData" copies data to the cache line. We expect a valid len argument.
// The pAdr parameter contains the full address from which we derive the alignment and
// offset into the cache line.
//
//----------------------------------------------------------------------------------------
void T64Cache::setCacheLineData( T64CacheLine *cLine,
                                 T64Word pAdr, 
                                 T64Word data, 
                                 int len ) {

    T64Word ofsMask = ( 1 << T64_LINE_OFS_BITS ) - 1;
    int     lOfs    = pAdr & ofsMask;
    int     wOfs    = sizeof( T64Word ) - len;

    memcpy(((uint8_t *) &data + wOfs ),
           (uint8_t *) &cLine -> line[ lOfs ], 
           len );
}

//----------------------------------------------------------------------------------------
// "readCacheData" is the routine to get the data from the cache. 
//
//----------------------------------------------------------------------------------------
int T64Cache::readCacheData( T64Word pAdr, T64Word *data, int len ) {

    // check len ?
    // check align ?

    T64CacheLine *cLine = lookup( pAdr );

    if ( cLine == nullptr ) {

        cacheMiss ++;

        int vSet = plru2Victim( plruState );

        cLine = &cacheArray[ vSet * T64_MAX_CACHE_SETS + getLineIndex( pAdr ) ];

       // pick victim to flush / purge

        // miss: fetch from memory. 

        // update plru state

    }
    
    // found: check alignment, copy data
    // move according to len

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::writeCacheData( T64Word pAdr, T64Word data, int len ) {

    T64CacheLine *cLine = lookup( pAdr );

     if ( cLine == nullptr ) {

        cacheMiss ++;

        int vSet = plru2Victim( plruState );

        cLine = &cacheArray[ vSet * T64_MAX_CACHE_SETS + getLineIndex( pAdr ) ];

       // pick victim to flush / purge

        // miss: fetch from memory. 

        // update plru state

    }

    // check sets, index is upper 6bits in page offset
    // move according to len

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
int T64Cache::flushCacheData( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back, mark as read shared

    return( 0 );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
int T64Cache::purgeCacheData( T64Word pAdr ) {

    // lookup cache line
    // if found and modified, write back
    // invalidate line

    return( 0 );
}

//----------------------------------------------------------------------------------------
// Reset. For now, clear the statistics.
//
//----------------------------------------------------------------------------------------
void T64Cache::reset( ) {

    cacheRequests   = 0;
    cacheMiss       = 0;
    plruState       = 0;

    for ( int i = 0; i < ( ways * sets ); i++ ) 
        clearCacheLine( &cacheArray[ i ] ); 
}

//----------------------------------------------------------------------------------------
// A cache read operation. For non-cached requests, we directly read the data from 
// memory.
//
//----------------------------------------------------------------------------------------
int T64Cache::read( T64Word pAdr, T64Word *data, int len, bool cached ) {

    cacheRequests++;
    
    if (( isInIoAdrRange( pAdr ) || ( ! cached ))) {

        return( sys -> readMem( pAdr, data, len ));
    }
    else return( readCacheData( pAdr, data, len ));
}

//----------------------------------------------------------------------------------------
// A cache write operation. For non-cached requests, we directly write the data to 
// memory.
//
//----------------------------------------------------------------------------------------
int T64Cache::write( T64Word pAdr, T64Word data, int len, bool cached ) {

    cacheRequests++;

    if (( isInIoAdrRange( pAdr ) || ( ! cached ))) {

        return( sys -> writeMem( pAdr, data, len ));
    }
    else return( writeCacheData( pAdr, data, len ));
}

//----------------------------------------------------------------------------------------
// A cache flush operation. Only valid for non-I/O address range.
//
//----------------------------------------------------------------------------------------
int T64Cache::flush( T64Word pAdr ) {

    if ( ! isInIoAdrRange( pAdr )) return( flushCacheData( pAdr ));
    else return( 99 );
}

//----------------------------------------------------------------------------------------
// A cache purge operation. Only valid for non-I/O address range.
//
//----------------------------------------------------------------------------------------
int T64Cache::purge( T64Word pAdr ) {

     if ( ! isInIoAdrRange( pAdr )) return( purgeCacheData( pAdr ));
     else return( 99 );
}