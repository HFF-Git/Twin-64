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
//
//
//------------------------------------------------------------------------------------------------------------
int testAsm ( char * asmStr ) {
    
    uint32_t instr;
    char     buf[ 128 ];
    
    doAsm -> assembleInstr( asmStr, &instr );
    printf( "ASM: \"%s\" -> Instr: 0x%08x\n", asmStr, instr );
    
    disAsm -> formatInstr( buf, sizeof( buf ), instr, 16 );
    printf( "Instr: 0x%08x -> ASM: \"%s\"\n", instr, buf );

    return ( 0 );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
int main( int argc, const char * argv[] ) {
    
    parseParameters( argc, argv );
    
    createCpu( );
    createAsm( );
    
    testAsm((char *) "ADD r1, r2, r3" );
    testAsm((char *) "ADD r1, -100(r2)" );
    testAsm((char *) "ADD r1, 0x3_9(r2)" );
    testAsm((char *) "AND.N r1, 0x3_9(r2)" );
   
    return 0;
}
