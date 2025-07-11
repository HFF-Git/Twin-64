//------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Monitor
//
//------------------------------------------------------------------------------
// 
//
//------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Monitor
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
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "T64-Common.h"
#include "T64-Processor.h"

T64Processor *proc;

//------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------
void setupProcssor( ) {

    proc = new T64Processor( );
}

//------------------------------------------------------------------------------
// Here we go.
//
//------------------------------------------------------------------------------
int main( int argc, const char * argv[] ) {

    setupProcssor( );

    proc -> reset( );
   
    printf( "Twin64 - Simulator MAIN - test\n");

    return 0;
}
