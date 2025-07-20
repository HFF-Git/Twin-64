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
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details. You should have received a copy of the GNU General Public
// License along with this program. If not, see <http://www.gnu.org/licenses/>.
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

#define CACHE_SETS     256
#define CACHE_WAYS     4
#define CACHE_LINE_SIZE 64

typedef struct {
    bool valid;
    uint64_t tag; // Physical address >> 14
} CacheLine;

typedef struct {
    CacheLine ways[CACHE_WAYS][CACHE_SETS]; // skewed sets per way
    uint32_t replacement_ptr[CACHE_SETS];   // simple round-robin
    uint64_t hits, misses;
} SkewedCache;



static inline uint8_t hash0(uint64_t addr) {
    return (addr >> 6) & 0xFF; // bits 13:6
}

static inline uint8_t hash1(uint64_t addr) {
    uint8_t x = (addr >> 6) & 0xFF;
    return (x ^ (x >> 3) ^ 0x5A);
}

static inline uint8_t hash2(uint64_t addr) {
    uint8_t x = (addr >> 6) & 0xFF;
    return ((x * 17) + 0x33) & 0xFF;
}

static inline uint8_t hash3(uint64_t addr) {
    uint8_t x = (addr >> 6) & 0xFF;
    return ((x ^ (x >> 2)) + 0xC7) & 0xFF;
}

static inline uint8_t (*const hash_funcs[CACHE_WAYS])(uint64_t) = {
    hash0, hash1, hash2, hash3
};

bool cache_lookup(SkewedCache *cache, uint64_t phys_addr) {
    uint64_t tag = phys_addr >> 14;

    for (int way = 0; way < CACHE_WAYS; way++) {
        uint8_t index = hash_funcs[way](phys_addr);
        CacheLine *line = &cache->ways[way][index];

        if (line->valid && line->tag == tag) {
            cache->hits++;
            return true;
        }
    }

    cache->misses++;
    return false;
}


void cache_insert(SkewedCache *cache, uint64_t phys_addr) {
    uint64_t tag = phys_addr >> 14;

    // Select a victim way using round-robin over skewed sets
    int victim_way = -1;
    int index = -1;

    // Try to find an invalid line first
    for (int way = 0; way < CACHE_WAYS; way++) {
        index = hash_funcs[way](phys_addr);
        if (!cache->ways[way][index].valid) {
            victim_way = way;
            break;
        }
    }

    // If all lines are valid, use round-robin
    if (victim_way == -1) {
        // Pick the first way’s index to rotate on (arbitrary)
        index = hash_funcs[0](phys_addr);
        victim_way = cache->replacement_ptr[index] % CACHE_WAYS;
        cache->replacement_ptr[index]++;
    }

    cache->ways[victim_way][hash_funcs[victim_way](phys_addr)] = (CacheLine){
        .valid = true,
        .tag = tag
    };
}

bool cache_find_line(
    SkewedCache *cache,
    uint64_t phys_addr,
    CacheLine **line_out,
    int *way_out,
    int *index_out
) {
    uint64_t tag = phys_addr >> 14;

    for (int way = 0; way < CACHE_WAYS; ++way) {
        int index = hash_funcs[way](phys_addr);
        CacheLine *line = &cache->ways[way][index];

        if (line->valid && line->tag == tag) {
            if (line_out)  *line_out  = line;
            if (way_out)   *way_out   = way;
            if (index_out) *index_out = index;
            return true;
        }
    }

    return false;
}





//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {


} // namespace


//****************************************************************************************
//****************************************************************************************
//
// Cache
//
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Cache::T64Cache( ) {

}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void T64Cache::reset( ) {

}