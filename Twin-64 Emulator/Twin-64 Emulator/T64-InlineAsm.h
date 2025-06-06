///------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Inline Assebler / Disassembler
//
//------------------------------------------------------------------------------------------------------------
// This ...
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Sketch
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
#ifndef T64_InlineAsm_h
#define T64_InlineAsm_h

#include "T64-Types.h"

//------------------------------------------------------------------------------------------------------------
// "T64Assemble" is a one line assembler. It just parses the instrcution string and produces an instruction.
// Utiity routines for converting an error code to an error message and an index into the input source line
// to where the error occured is provided too.
//
//------------------------------------------------------------------------------------------------------------
struct T64Assemble {
    
public:
    
    T64Assemble( );
    
    int         assembleInstr( char *inputStr, uint32_t *instr );

    int         getErrId( );
    int         getErrPos( );
    const char   *getErrStr( int err );
};

//------------------------------------------------------------------------------------------------------------
// "T64DisAssemble" will disassemble an instruction and return a human readable form. The disassmbly string
// can also congain just the opcode part, the operand part or both. The split allows for displaying the
// disassembled instruction in an aligned fashion, when printing several lines.
//
//------------------------------------------------------------------------------------------------------------
struct T64DisAssemble {
    
public:
    
    T64DisAssemble( );
    
    int formatInstr( char *buf, int bufLen, uint32_t instr, int rdx );
    int formatOpCode( char *buf, int bufLen, uint32_t instr );
    int formatOperands( char *buf, int bufLen, uint32_t instr, int rdx );
    int getOpCodeFieldWidth( );
    int getOperandsFieldWidth( );
};

#endif /* T64_InlineAsm_h */
