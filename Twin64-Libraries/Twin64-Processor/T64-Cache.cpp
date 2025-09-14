//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Cache
//
//----------------------------------------------------------------------------------------
// The Twin-64 processor has a cache subsystem. Since there can be more than one 
// processor, the caches need to maintain cache coherence. We implement this in a 
// simple protocol where each operation in a processor will trigger the cache coherence
// action immediately. For example, if there is a write operation to a cache line not
// available so far, a read private operation will be communicated to the T64 system.
// The T64 system in turn will tell all modules that they need to flush and or purge 
// their cache line. Then the data is read and the processor is the exclusive owner of
// this cache line. In other words, all actions with respect to the cache line are 
// done right away transparently to the processor.
//
// The caches themselves are set associative caches. There are defined cache types to 
// experiment with ways and set sizes. 
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


//----------------------------------------------------------------------------------------
// Local name space. 
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// PLRU utilities: 2-way 
//
//  1-bit encoding: we just alternate between the two ways.
//   
//----------------------------------------------------------------------------------------
inline int plru2Victim( uint8_t state ) {
    
    return (( state & 1 ) ? 0 : 1 );
}

inline uint8_t plru2Update( uint8_t state, int way ) {
    
    return ((uint8_t)( way & 1 ));
}

//----------------------------------------------------------------------------------------
// PLRU utilities: 4-way 
//
//  3-bit encoding:
//
//  bit0 = root         ( 0 -> left, 1 -> right )
//  bit1 = left branch  ( 0 -> way3, 1 -> way2  )
//  bit2 = right branch ( 0 -> way1, 1 -> way0  )
//
//  Convention: bit == 0  => left subtree/leaf was used recently
//              bit == 1  => right subtree/leaf was used recently
//  
//  Access update ( way -> new_state ):
//
//  way 0 -> 0
//  way 1 -> 2
//  way 2 -> 4
//  way 3 -> 5
//
//----------------------------------------------------------------------------------------
inline static int plru4Victim( uint8_t s ) {


    if ((( s >> 0 ) & 1 ) == 0 ) return ((( s >> 1 ) & 1 ) == 0 ) ? 3 : 2;
    else                         return ((( s >> 2 ) & 1 ) == 0 ) ? 1 : 0;    
}

inline static uint8_t plru4Update(uint8_t state, int way) {
        
    uint8_t s = state & 0x07;
        
    switch ( way & 3 ) {
        
        case 0: s |= 1;     s |= 4;     break;
        case 1: s |= 1;     s &= ~4;    break;
        case 2: s &= ~1;    s |= 2;     break;
        case 3: s &= ~1;    s &= ~2;    break;
        }

        return ( s );
};

//----------------------------------------------------------------------------------------
// PLRU utilities: 8-way 
//
// 7-bit encoding:
//
//      node 0: root                    ( splits [0..3] left and [4..7] right )
//      node 1: left child of root      ( splits [0..1] and [2..3] )
//      node 2: right child of root     ( splits [4..5] and [6..7] )
//      node 3: left-left node          ( splits 0 vs 1 )
//      node 4: left-right node         ( splits 2 vs 3 )
//      node 5: right-left node         ( splits 4 vs 5 )
//      node 6: right-right node        ( splits 6 vs 7 )
//
//  Convention: bit == 0  => left subtree/leaf was used recently
//              bit == 1  => right subtree/leaf was used recently
//
//  Victim selection: at each node, follow the NOT-recent direction:
//
//  if bit == 0 ( left recent )  -> victim is in right subtree
//  if bit == 1 ( right recent ) -> victim is in left subtree
//
// Access update:
//
//  set bits along the path to mark the accessed side as recent.
//  convention: bit == 0 => left recent, bit == 1 => right recent
//
//  Way 0:  root left,  node1 left,  node3 left
//  Way 1:  root left,  node1 left,  node3 right
//  Way 2:  root left,  node1 right, node4 left
//  Way 3:  root left,  node1 right, node4 right
//  Way 4:  root right, node2 left,  node5 left
//  Way 5:  root right, node2 left,  node5 right
//  Way 6:  root right, node2 right, node6 left
//  Way 7:  root right, node2 right, node6 right
//
//----------------------------------------------------------------------------------------
static inline int plru8Victim( uint8_t s ) {

    if ((( s >> 0 ) & 1 ) == 0 ) {
        
        if ((( s >> 2 ) & 1 ) == 0 ) {
            
            if ((( s >> 6 ) & 1 ) == 0 )  return 7;
            else                          return 6;
        } else {

            if ((( s >> 5 ) & 1 ) == 0 )  return 5; 
            else                          return 4; 
        }
    } else {
        
        if ((( s >> 1 ) & 1 ) == 0 ) {
            
            if ((( s >> 4 ) & 1 ) == 0 )  return 3;
            else                          return 2;

        } else {
            
            if ((( s >> 3 ) & 1 ) == 0 )  return 1;
            else                          return 0;
        }
    }
}

static inline uint8_t plru8Update( uint8_t state, int way ) {

    uint8_t s = state & 0x7F;

    switch ( way & 0x7 ) {

        case 0: s &= ~1;    s &= ~2;    s &= ~8;    break;        
        case 1: s &= ~1;    s &= ~2;    s |= 8;     break;
        case 2: s &= ~1;    s |= 2;     s &= ~16;   break;
        case 3: s &= ~1;    s |= 2;     s |= 16;    break;
        case 4: s |= 1;     s &= ~4;    s &= ~32;   break;
        case 5: s |= 1;     s &= ~4;    s |= 32;    break;
        case 6: s |= 1;     s |= 4;     s &= ~64;   break;
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
// "T64Cache" is the object constructor. We decode the valid cache type options and
// precompute bit offsets, masks, and so on.
//
//----------------------------------------------------------------------------------------
T64Cache::T64Cache( T64CacheType cacheType, T64System *sys ) { 

    this -> cacheType = cacheType;

     switch ( cacheType ) {

        case T64_CT_2W_64S: 
        case T64_CT_2W_128S:    ways = 2;   break;

        case T64_CT_4W_64S:
        case T64_CT_4W_128S:    ways = 4;   break;

        case T64_CT_8W_64S:
        case T64_CT_8W_128S:    ways = 8;   break;

        default:                ways = 2; 
    }

    switch ( cacheType ) {

        case T64_CT_2W_128S:    
        case T64_CT_4W_128S:
        case T64_CT_8W_128S: {
            
            sets        = 128; 
            lineSize    = 32;
            offsetBits  = 5;                    
            indexBits   = 7;     
        
        } break;

        case T64_CT_2W_64S:    
        case T64_CT_4W_64S:
        case T64_CT_8W_64S: {
            
            sets        = 64; 
            lineSize    = 64;
            offsetBits  = 6;                    
            indexBits   = 6;     

         } break;

        default: {
            
            sets        = 128; 
            lineSize    = 32;
            offsetBits  = 5;                    
            indexBits   = 7;     

         }
    }

    offsetBitmask   = ( 1ULL << offsetBits ) - 1;
    indexBitmask    = ( 1ULL << indexBits ) - 1;
    indexShift      = offsetBits;
    tagShift        = offsetBits + indexBits;
    cacheHits       = 0;
    cacheMiss       = 0;
    plruState       = 0;

    cacheInfo = (T64CacheLineInfo *) malloc( ways * sets );
    cacheData = (uint8_t *) malloc( ways * sets * lineSize );
}

//----------------------------------------------------------------------------------------
// Reset. For now, clear the statistics.
//
//----------------------------------------------------------------------------------------
void T64Cache::reset( ) {

    cacheHits   = 0;
    cacheMiss   = 0;
    plruState   = 0;

    for ( int i = 0; i < ( ways * sets ); i++ ) {

        cacheInfo[ i ].valid    = false;
        cacheInfo[ i ].modified = false;
        cacheInfo[ i ].tag      = 0;
    }
}

//----------------------------------------------------------------------------------------
// A cache requests will decode the tag, index and line offset from the physical 
// address.
//
//----------------------------------------------------------------------------------------
uint32_t T64Cache::getTag( T64Word pAdr ) {

    return ( pAdr >> tagShift );
}   

uint32_t T64Cache::getSetIndex( T64Word  pAdr ) {

    return ( ( pAdr >> indexShift ) & indexBitmask );
}

uint32_t T64Cache::getLineOfs( T64Word  pAdr ) {

    return ( pAdr & offsetBitmask );    
}

//----------------------------------------------------------------------------------------
// The set associative cache uses a pseudo LRU scheme. There are two local routines,
// select a victim and update the state. Depending on the number of ways, we call the
// respective local routine.
//
//----------------------------------------------------------------------------------------
int T64Cache::plruVictim( ) {

    switch( ways ) {

        case 2: return( plru2Victim( plruState ));
        case 4: return( plru4Victim( plruState ));
        case 8: return( plru8Victim( plruState ));
        default: return( 0 );
    }
}

void T64Cache::plruUpdate( ) {

    switch( ways ) {

        case 2:  plruState = plru2Update( plruState, ways ); break;
        case 4:  plruState = plru4Update( plruState, ways ); break;
        case 8:  plruState = plru8Update( plruState, ways ); break;
        default: plruState = 0;
    }
}

//----------------------------------------------------------------------------------------
// "lookup" searches the cache sets. If we find a valid cache line with the matching
// tag, we return the pointers to the line.
//
//----------------------------------------------------------------------------------------
bool T64Cache::lookupCache( T64Word          pAdr, 
                            T64CacheLineInfo **info, 
                            uint8_t          **data ) {

    uint32_t  tag = getTag( pAdr );
    uint32_t  of  = getLineOfs( pAdr );
    uint32_t  set = getSetIndex( pAdr );

    for ( int w = 0; w < ways; w++ ) {

        T64CacheLineInfo *l = &cacheInfo[ w * set ];
        if (( l -> valid ) && ( l -> tag == tag )) {

            *info = l;
            *data = &cacheData[ (( w * sets ) + set ) * lineSize ];
            return( true );
        }
    }

    return( false );               
}

//----------------------------------------------------------------------------------------
// "selectCacheLine" will return the slot based on way an set index regardless if 
// the slot is valid.
//
//----------------------------------------------------------------------------------------
bool T64Cache::getCacheLine( uint32_t         way,
                             uint32_t         set, 
                             T64CacheLineInfo **info, 
                             uint8_t       **data ) {

    if ( way > ways ) return ( false );
    if ( set > sets ) return ( false );

    *info  = &cacheInfo[ way * set ];
     *data = &cacheData[ (( way * sets ) + set ) * lineSize ];
     return ( true );
}

//----------------------------------------------------------------------------------------
// "getCacheLineData" copies data from the cache line. We expect a valid len argument.
// 
//----------------------------------------------------------------------------------------
T64Word T64Cache::getCacheLineData( uint8_t *line, 
                                    int     lineOfs,
                                    int     len ) {

    int wOfs = sizeof( T64Word ) - len;

    T64Word tmp;
    memcpy(((uint8_t *) &tmp + wOfs ), &line[ lineOfs ], len );
    return( tmp );
}

//----------------------------------------------------------------------------------------
// "setCacheLineData" copies data to the cache line. We expect a valid len argument.
//
//----------------------------------------------------------------------------------------
void T64Cache::setCacheLineData( uint8_t *line,
                                 int     lineOfs,
                                 int     len,
                                 T64Word data ) {

    int wOfs = sizeof( T64Word ) - len;

    memcpy( &line[ lineOfs ], ((uint8_t *) &data + wOfs ), len );
}

//----------------------------------------------------------------------------------------
// "readCacheData" is the routine to get the data from the cache. We first check for
// any alignment errors. Next, lookup the cache. If the line is found, just return
// the data.
//
// If the cache does not have the data, we need to get it. First select a victim line
// to read in the cache line from memory. If we select an unused line, easy, just 
// issue a shared read request to the T64 system. If the cache line is valid, we need
// to check if it was modified. If so, we need to flush the data first. Then we READ 
// SHARED the new cache line into this slot. Finally, we return the requested data.
//
//----------------------------------------------------------------------------------------
int T64Cache::readCacheData( T64Word pAdr, T64Word *data, int len ) {

    if ( ! isAligned( pAdr, len )) return( -1 );

    T64CacheLineInfo *cInfo;
    uint8_t          *cData;

    if ( lookupCache( pAdr, &cInfo, &cData )) {

        cacheHits ++;
        plruUpdate( );

        *data = getCacheLineData( cData, getLineOfs( pAdr ), len );
    }
    else {

        cacheMiss ++;

        int vWay = plruVictim( );
        plruUpdate( );
        
        getCacheLine( vWay, getSetIndex( pAdr ), &cInfo, &cData );

        if ( cInfo -> valid ) {

            if ( cInfo -> modified ) {

                sys -> writeBlock( 0, 
                                    ( cInfo -> tag << indexBits ) + getSetIndex( pAdr ), 
                                    cData, 
                                    lineSize );
                // ??? throw on error ?
            }

            cInfo -> valid = false;
        }

        sys -> readBlockShared( 0, pAdr, cData, lineSize );
        // ??? throw on error ?
    }

    *data = getCacheLineData( cData, getLineOfs( pAdr ), len );

    return( 0 );
}

//----------------------------------------------------------------------------------------
// "writeCacheData" is the routine to write data to the cache. We first check for
// any alignment errors. Next, lookup the cache. If the line is found, just update
// the data in the cache line.
//
// If the cache does not have the cache line, we need to get it first. First select 
// a victim line to read in the cache line from memory. If we select an unused line,
// easy, just issue a shared private request to the T64 system. If the cache line is
// valid, we need to check if it was modified. If so, we need to flush the data first.
// Then we READ PRIVATE the new cache line into this slot. Finally, we update the 
// cache line.
//
//----------------------------------------------------------------------------------------
int T64Cache::writeCacheData( T64Word pAdr, T64Word data, int len ) {

    if ( ! isAligned( pAdr, len )) return ( -1 );

    T64CacheLineInfo *cInfo;
    uint8_t          *cData;

    if ( lookupCache( pAdr, &cInfo, &cData )) {

        cacheHits ++;
        plruUpdate( );
        setCacheLineData( cData, getLineOfs( pAdr ), len, data );
    }
    else {

        int vWay = plruVictim( );

        cacheMiss ++;
        plruUpdate( );

        getCacheLine( vWay, getSetIndex( pAdr ), &cInfo, &cData );

        if ( cInfo -> valid ) {

            if ( cInfo -> modified ) {

                sys -> writeBlock( 0, 
                                    ( cInfo -> tag << indexBits ) + getSetIndex( pAdr ), 
                                    cData, 
                                    lineSize );
                // ??? throw on error 
            }

            cInfo -> valid = false;
        }

        sys -> readBlockPrivate( 0, pAdr, cData, lineSize );
        // ??? throw on error ?
    }

    setCacheLineData( cData, getLineOfs( pAdr ), len, data );

    return( 0 );
}

//----------------------------------------------------------------------------------------
// "flushCacheLine" will write back a cache line to memory if it is modified. If we 
// do not have such a cache line, the request is ignored. 
//
//----------------------------------------------------------------------------------------
int T64Cache::flushCacheLine( T64Word pAdr ) {

    T64CacheLineInfo *cInfo;
    uint8_t          *cData;

    if ( lookupCache( pAdr, &cInfo, &cData )) { 

        if ( cInfo -> modified ) {

            // ??? need to mask pAdr ? 
            sys -> writeBlock( 0, 
                               ( cInfo -> tag << indexBits ) + getSetIndex( pAdr ), 
                               cData, 
                               lineSize );
            // ??? throw on error ?

            cInfo -> modified = false;
        }
    }

    return( 0 );
}

//----------------------------------------------------------------------------------------
// "purgeCacheLine" will remove a cache line. If the line was modified it is flushed
// first.
//
//----------------------------------------------------------------------------------------
int T64Cache::purgeCacheLine( T64Word pAdr ) {

    T64CacheLineInfo *cInfo;
    uint8_t          *cData;

    if ( lookupCache( pAdr, &cInfo, &cData )) {

        if ( cInfo -> modified ) {

            // ??? need to mask pAdr ? 
            sys -> writeBlock( 0, pAdr, cData, lineSize );
            // ??? throw on error ?

            cInfo -> modified = false;
        }

        cInfo -> valid  = false;
        cInfo -> tag    = 0;
    }

    return( 0 );
}

//----------------------------------------------------------------------------------------
// A cache read operation. For non-cached requests, we directly read the data from 
// memory.
//
//----------------------------------------------------------------------------------------
int T64Cache::read( T64Word pAdr, T64Word *data, int len, bool cached ) {
    
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

    if ( ! isInIoAdrRange( pAdr )) return( flushCacheLine( pAdr ));
    else return( 99 );
}

//----------------------------------------------------------------------------------------
// A cache purge operation. Only valid for non-I/O address range.
//
//----------------------------------------------------------------------------------------
int T64Cache::purge( T64Word pAdr ) {

     if ( ! isInIoAdrRange( pAdr )) return( purgeCacheLine( pAdr ));
     else return( 99 );
}