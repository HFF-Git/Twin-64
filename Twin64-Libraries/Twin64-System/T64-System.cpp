//----------------------------------------------------------------------------------------
//
// Twin-64 - System
//
//----------------------------------------------------------------------------------------
// This ...
//
//----------------------------------------------------------------------------------------
//
// Twin-64 - System
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
#include "T64-System.h"

Twin64System::Twin64System( ) {

    
}


// ??? quick sketch for the module address resolution....


#define MAX_RANGES 64

typedef struct {

    // ??? add type 

    uint64_t start;
    uint64_t len;
    void *ptr;
} RangeEntry;

typedef struct {
    RangeEntry entries[MAX_RANGES];
    size_t hwm;
} RangeTable;

void range_table_init(RangeTable *table) {
    table->hwm = 0;
}

bool insert_range(RangeTable *table, uint64_t start, uint64_t len, void *ptr) {

    if (table->hwm >= MAX_RANGES || len == 0) return false;

    uint64_t end = start + len;

    size_t i;
    for (i = 0; i < table->hwm; ++i) {
        uint64_t s = table->entries[i].start;
        uint64_t e = s + table->entries[i].len;

        if (end <= s) {
            break;  // No overlap, insert before
        } else if (start >= e) {
            continue; // No overlap, check next
        } else {
            return false; // Overlap found
        }
    }

    // Shift to make space
    for (size_t j = table->hwm; j > i; --j) {
        table->entries[j] = table->entries[j - 1];
    }

    table->entries[i].start = start;
    table->entries[i].len = len;
    table->entries[i].ptr = ptr;
    table->hwm++;
    return true;
}

void *find_range(RangeTable *table, uint64_t value) {
    if (table->hwm == 0)
        return NULL;

    RangeEntry *first = &table->entries[0];
    if (value >= first->start && value < first->start + first->len)
        return first->ptr;

    for (size_t i = 1; i < table->hwm; ++i) {
        uint64_t s = table->entries[i].start;
        uint64_t e = s + table->entries[i].len;
        if (value >= s && value < e)
            return table->entries[i].ptr;
    }
    return NULL;
}

// Remove range with exact matching 'start'. Returns true on success.
bool remove_range(RangeTable *table, uint64_t start) {
    for (size_t i = 0; i < table->hwm; ++i) {
        if (table->entries[i].start == start) {
            // Shift down
            for (size_t j = i + 1; j < table->hwm; ++j) {
                table->entries[j - 1] = table->entries[j];
            }
            table->hwm--;
            return true;
        }
    }
    return false;
}



// Returns index of the entry containing 'value', or -1 if not found
int find_range_index(RangeTable *table, uint64_t value) {
    if (table->hwm == 0)
        return -1;

    RangeEntry *first = &table->entries[0];
    if (value >= first->start && value < first->start + first->len)
        return 0;

    for (size_t i = 1; i < table->hwm; ++i) {
        uint64_t s = table->entries[i].start;
        uint64_t e = s + table->entries[i].len;
        if (value >= s && value < e)
            return (int)i;
    }
    return -1;
}

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





