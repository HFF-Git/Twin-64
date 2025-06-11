//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU
//
//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation, either version 3 of the License,
// or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details. You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------------------------------------------------
#include "T64-Types.h"
#include "T64-InlineAsm.h"
#include "T64-Cpu.h"
#include "T64-Phys-Mem.h"
#include "T64-Io-Mem.h"

T64Assemble     *doAsm  = nullptr;
T64DisAssemble  *disAsm = nullptr;

T64PhysMem      *mem    = nullptr;
T64IoMem        *io     = nullptr;
T64Cpu          *cpu    = nullptr;

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void parseParameters( int argc, const char * argv[] ) {
    
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void createCpu( ) {
    
    mem = new T64PhysMem( 2040 );
    io  = new T64IoMem( 2048 );
    cpu = new T64Cpu( mem, io );
    
    cpu -> reset( );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void createAsm( ) {
    
    doAsm  = new T64Assemble( );
    disAsm = new T64DisAssemble( );
}

//------------------------------------------------------------------------------------------------------------
// Command handlers.
//
//------------------------------------------------------------------------------------------------------------
void assemble( char *asmStr ) {
    
    uint32_t instr;
    doAsm -> assembleInstr( asmStr, &instr );
    printf( "0x%08x\n", instr );
}


void disassemble( uint32_t instr ) {
    
    char buf[ 128 ];
    disAsm -> formatInstr( buf, sizeof( buf ), instr, 16 );
    printf( "\"%s\"\n", buf );
}

void printHelp( ) {
    
    printf( "A <argStr> -> assemble input argument\n" );
    printf( "D <val>    -> disassemble instruction value\n" );
    printf( "E          -> exit\n" );
}

//------------------------------------------------------------------------------------------------------------
// Read the command input, strip newline char and convert to uppercase.
//
//------------------------------------------------------------------------------------------------------------
int getInput( char *buf ) {
    
    if ( fgets( buf, 128, stdin ) == nullptr ) return( -1 );
    
    buf[ strcspn( buf, "\n") ] = '\0';

    for ( char *s = buf; *s; s++ ) {
        
        *s = toupper((unsigned char) *s );
    }
    
    return( 1 );
}

//------------------------------------------------------------------------------------------------------------
// Here we go.
//
//------------------------------------------------------------------------------------------------------------
int main( int argc, const char * argv[] ) {
    
    parseParameters( argc, argv );
    
    createCpu( );
    createAsm( );

    while (1) {
        
        printf( "-> ");
        
        char input[ 128 ];
        
        if ( getInput( input ) < 0 ) {
            
            printf("Error reading input\n");
            break;
        }
       
        char *cmd = strtok( input, " \t");
        char *arg = strtok( NULL, "" );
        if ( cmd == nullptr ) continue;
             
        if ( strcmp( cmd, "A" ) == 0 ) {
            
            if ( arg != nullptr ) assemble( arg );
            else printf( "Expexted assembler input string\n" );
        }
        else if ( strcmp( cmd, "D" ) == 0 ) {
            
            uint32_t val;
            if (( arg != nullptr ) && ( sscanf( arg, "%i", &val ) == 1 )) disassemble( val );
            else printf( "Invalid number for disassembler\n" );
        }
        else if ( strcmp( cmd, "E" ) == 0 ) {
        
            break;
        }
        else if ( strcmp( cmd, "?" ) == 0 ) {
            
            printHelp( );
        }
        else printf("Unknown command.\n");
    }

    return 0;
}
