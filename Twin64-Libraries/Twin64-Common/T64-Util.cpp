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

//----------------------------------------------------------------------------------------
//
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


void addChar( char *buf, int size, char ch ) {
    
    int len = (int) strlen( buf );
    
    if ( len + 1 < size ) {
        
        buf[ len ]     = ch;
        buf[ len + 1 ] = 0;
    }
}

const char *pageTypeStr( uint8_t pTyp ) {

    switch ( pTyp ) {

        case 0: return( "R" );
        case 1: return( "W" );
        case 2: return( "X" );
        case 3: return( "G" );
        default: return( "*" );
    }
}
