//----------------------------------------------------------------------------------------
//
//  Twin64 - A 64-bit CPU Simulator - Common utility functions
//
//----------------------------------------------------------------------------------------
// Helper functions that are not declared inline ....
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Simulator - Common Declarations
// Copyright (C) 2022 - 2025 Helmut Fieres
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
#include "T64-Util.h"

// ??? the routines may be better placed in the simulator...
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
const char *pageTypeStr( uint8_t pTyp ) {

    switch ( pTyp ) {

        case 0: return( "R" );
        case 1: return( "W" );
        case 2: return( "X" );
        case 3: return( "G" );
        default: return( "*" );
    }
}

//----------------------------------------------------------------------------------------
// "insertChar" will insert a character in the input buffer at the cursor 
// position and adjust cursor and overall string size accordingly. There are 
// two basic cases. The first is simply appending to the buffer when both the 
// current string size and cursor position are equal. The second is when the 
// cursor is somewhere in the input buffer. In this case we need to shift the 
// characters to the right to make room first.
//
//----------------------------------------------------------------------------------------
void insertChar( char *buf, int ch, int *strSize, int *pos ) {
    
    if ( *pos == *strSize ) {
        
        buf[ *strSize ] = ch;
        *strSize        = *strSize + 1;
        *pos            = *pos + 1;
    }
    else if ( *pos < *strSize ) {
        
        for ( int i = *strSize; i > *pos; i-- ) buf[ i ] = buf[ i -1 ];
        
        buf[ *pos ] = ch;
        *strSize    = *strSize + 1;
        *pos        = *pos + 1;
    }
}

//----------------------------------------------------------------------------------------
// "removeChar" will remove a character from the input buffer at the cursor 
// position and adjust the string size accordingly. If the cursor is at the end
// of the string, both string size and cursor position are decremented by one, 
// otherwise the cursor stays where it is and just the string size is 
// decremented.
//
//----------------------------------------------------------------------------------------
void removeChar( char *buf, int *strSize, int *pos ) {
    
    if (( *strSize > 0 ) && ( *strSize == *pos )) {
        
        *strSize    = *strSize - 1;
        *pos        = *pos - 1;
    }
    else if (( *strSize > 0 ) && ( *pos >= 0 )) {
   
        for ( int i = *pos; i < *strSize; i++ ) buf[ i ] = buf[ i + 1 ];
        *strSize    = *strSize - 1;
    }
}

//----------------------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------------------
void addChar( char *buf, int size, char ch ) {
    
    int len = (int) strlen( buf );
    
    if ( len + 1 < size ) {
        
        buf[ len ]     = ch;
        buf[ len + 1 ] = 0;
    }
}

//----------------------------------------------------------------------------------------
// "appendChar" will add a character to the end of the buffer and adjust the 
// overall size.
//
//----------------------------------------------------------------------------------------
void appendChar( char *buf, int ch, int *strSize ) {
    
    buf[ *strSize ] = ch;
    *strSize        = *strSize + 1;
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void upshiftStr( char *str ) {
    
    size_t len = strlen( str );
    
    if ( len > 0 ) {
        
        for ( size_t i = 0; i < len; i++ ) {
            
            str[ i ] = (char) toupper((int) str[ i ] );
        }
    }
}

