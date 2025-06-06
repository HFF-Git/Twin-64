//------------------------------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - DisAssembler
//
//------------------------------------------------------------------------------------------------------------
// The instruction disassemble routines will format an instruction word in human readable form. An instruction
// has the general format
//
//      OpCode [ Opcode Options ] [ target ] [ source ]
//
// The disassemble routine will analyze an instruction word and present the instruction portion in the above
// order. The result is a string with the disassembled instruction.
//
//------------------------------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - DisAssembler
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "T64-Types.h"
#include "T64-InlineAsm.h"

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
namespace {

const int OPCODE_FIELD_LEN  = 16;
const int OPERAND_FIELD_LEN = 32;

//------------------------------------------------------------------------------------------------------------
// Helper routines.
//
//------------------------------------------------------------------------------------------------------------
static inline uint32_t extractBit( uint32_t arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline uint32_t extractField( uint32_t arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline int extractSignedField( uint32_t arg, int bitpos, int len ) {
    
    int field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 32 )  return ( field << ( 32 - len ) >> ( 32 - len ));
    else             return ( field );
}

static inline int extractOpGroup( uint32_t arg ) {
    
    return( extractField( arg, 30, 2 ));
}

static inline int extractOpCode( uint32_t arg ) {
    
    return( extractField( arg, 26, 4 ));
}

static inline int extractRegR( uint32_t instr ) {
    
    return ( extractField( instr, 22, 4 ));
}

static inline int extractRegB( uint32_t instr ) {
    
    return ( extractField( instr, 15, 4 ));
}

static inline int extractRegA( uint32_t instr ) {
    
    return ( extractField( instr, 9, 4 ));
}

static inline int extractDw( uint32_t instr ) {
    
    return ( extractField( instr, 13, 2 ));
}

static inline int extractImm13( uint32_t instr ) {
    
    return ( extractSignedField( instr, 0, 13 ));
}

static inline int extractImm15( uint32_t instr ) {
    
    return ( extractSignedField( instr, 0, 15 ));
}

static inline int extractImm19( uint32_t instr ) {
    
    return ( extractSignedField( instr, 0, 19 ));
}

static inline int extractImm20( uint32_t instr ) {
    
    return ( extractField( instr, 0, 20 ));
}

//------------------------------------------------------------------------------------------------------------
// "printImmVal" display an immediate value in the selected radix. Hex numbers are printed in unsigned
// quantities, decimal numbers are interpreted as signed integers. Most often decimal notation is used to
// specify offsets on indexed addressing modes. The function returns the characters written. The maximum
// size of what is added is set to 16.
//
//------------------------------------------------------------------------------------------------------------
int printImmVal( char *buf, uint32_t val, int rdx = 16 ) {
    
    if ( val == 0 ) return ( snprintf( buf, sizeof( buf ), "0" ));
    
    else {
        
        if      ( rdx == 10 )  return ( snprintf( buf, 16, "%d", ((int) val )));
        else if ( rdx == 16 )  return ( snprintf( buf, 16, "%#0x", val ));
        else                   return ( snprintf( buf, 16, "**num***" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// A little helper function to display the comparison condition in human readable form. We only decode the
// two bits which map to EQ, NE, LT and LE. A possible GT and GE cases cannot be deduced from just looking
// at the instruction. The function returns the characters written. The maximum size of what is added is set
// to 2.
//
//------------------------------------------------------------------------------------------------------------
int printCondField( char *buf, uint32_t cmpCode ) {
    
    switch( cmpCode ) {
            
        case 0:  return ( snprintf( buf, 4, "EQ" ));
        case 1:  return ( snprintf( buf, 4, "LT" ));
        case 2:  return ( snprintf( buf, 4, "NE" ));
        case 3:  return ( snprintf( buf, 4, "LE" ));
        default: return ( snprintf( buf, 4, "**" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// A little helper function to display the DW field.
//
//------------------------------------------------------------------------------------------------------------
int printDwField( char *buf, uint32_t dw ) {
    
    switch( dw ) {
            
        case 0:  return ( snprintf( buf, 4, "B" ));
        case 1:  return ( snprintf( buf, 4, "H" ));
        case 2:  return ( snprintf( buf, 4, "W" ));
        case 3:  return ( snprintf( buf, 4, "D" ));
        default: return ( snprintf( buf, 4, "*" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the opcode and opcode option portion. An opcode consist of the instruction group and the opcode
// fanmily. We construct the final opcode for the case statement.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStr( char *buf, uint32_t instr ) {
    
    uint32_t opCode = extractOpGroup( instr ) * 16 + extractOpCode( instr );
    
    switch( opCode ) {
            
        case ( OPC_GRP_ALU * 16 + OPC_ADD ):
        case ( OPC_GRP_MEM * 16 + OPC_ADD ): return ( snprintf( buf, OPCODE_FIELD_LEN, "ADD" ));
            
        case ( OPC_GRP_ALU * 16 + OPC_SUB ):
        case ( OPC_GRP_MEM * 16 + OPC_SUB ): return ( snprintf( buf, OPCODE_FIELD_LEN, "SUB" ));
            
        case ( OPC_GRP_ALU * 16 + OPC_AND ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "AND" );
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_AND ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "AND" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_OR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "OR" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_OR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "OR" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_XOR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "XOR" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_XOR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "XOR" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "CMP" );
            printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_CMP ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "CMP" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
            
            switch ( extractField( instr, 19, 3 )) {
                    
                case 0: { // EXTR
                    
                    int cursor = snprintf( buf, OPCODE_FIELD_LEN, "EXTR" );
                    if ( extractBit( instr, 12 )) cursor += snprintf( buf + cursor, 4, ".S" );
                    return ( cursor );
                }
                    
                case 1: { // DEP
                    
                    int cursor = snprintf( buf, OPCODE_FIELD_LEN, "DEP" );
                    if ( extractBit( instr, 12 )) cursor += snprintf( buf + cursor, 4, ".Z" );
                    if ( extractBit( instr, 14 )) cursor += snprintf( buf + cursor, 4, ".I" );
                    return ( cursor );
                }
                    
                case 2: { // DSR
                    
                    return ( snprintf( buf, OPCODE_FIELD_LEN, "DSR" ));
                }
                    
                    return ( snprintf( buf, OPCODE_FIELD_LEN, "**BITOP**" ));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
            
            int cursor = 0;
            
            switch ( extractField( instr, 19, 3 )) {
                    
                case 2: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHL1A" ); break;
                case 4: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHL2A" ); break;
                case 6: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHL3A" ); break;
                    
                case 3: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHR1A" ); break;
                case 5: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHR2A" ); break;
                case 7: cursor += snprintf( buf + cursor, OPCODE_FIELD_LEN, "SHR3A" ); break;
                return ( snprintf( buf, OPCODE_FIELD_LEN, "**SHAOP**" ));
            }
            
            if ( extractBit( instr, 14 )) cursor += snprintf( buf + cursor, 4, ".I" );
            
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
            
            switch ( extractField( instr, 20, 2 )) {
                    
                case 1:  return ( snprintf( buf, OPCODE_FIELD_LEN, "LDI.L" ));
                case 2:  return ( snprintf( buf, OPCODE_FIELD_LEN, "LDI.S" ));
                case 3:  return ( snprintf( buf, OPCODE_FIELD_LEN, "LDI.U" ));
                default: return ( 0 );
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_LDO ): return ( snprintf( buf, OPCODE_FIELD_LEN, "LDO" ));
            
        case ( OPC_GRP_MEM * 16 + OPC_LD ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "LD" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_ST ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "ST" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_LDR ): return ( snprintf( buf, OPCODE_FIELD_LEN, "LDR" ));
        case ( OPC_GRP_MEM * 16 + OPC_STC ): return ( snprintf( buf, OPCODE_FIELD_LEN, "STC" ));
            
        case ( OPC_GRP_BR * 16 + OPC_B ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "B" );
            if ( extractBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".G" );
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BR ): return ( snprintf( buf, OPCODE_FIELD_LEN, "BR" ));
            
        case ( OPC_GRP_BR * 16 + OPC_BV ): return ( snprintf( buf, OPCODE_FIELD_LEN, "BV" ));
            
        case ( OPC_GRP_BR * 16 + OPC_BB ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "BB" );
            if ( extractBit( instr, 19 ))   cursor += snprintf( buf + cursor, 4, ".T" );
            else                            cursor += snprintf( buf + cursor, 4, ".F" );
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_CBR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "CBR" );
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_MBR ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "MBR" );
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MR ): {
            
            if ( extractField( instr, 19, 3 ) == 0 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "MFCR" ));
            else                                     return ( snprintf( buf, OPCODE_FIELD_LEN, "MTCR" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_LDPA ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "LDPA" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
            
            int cursor = snprintf( buf, OPCODE_FIELD_LEN, "PRB" );
            if ( extractBit( instr, 19 ))   cursor += snprintf( buf + cursor, 4, ".P" );
            else                            cursor += snprintf( buf + cursor, 4, ".U" );
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "ITLB" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "PTLB" ));
            else return ( snprintf( buf, 8, "**TLB**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_CA ): {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "PCA" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "FCA" ));
            else return ( snprintf( buf, 8, "**CA**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MST ): {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "RSM" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return ( snprintf( buf, OPCODE_FIELD_LEN, "SSM" ));
            else return ( snprintf( buf, 8, "**MST**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_RFI ):   return ( snprintf( buf, OPCODE_FIELD_LEN, "RFI" ));
            
        case ( OPC_GRP_SYS * 16 + OPC_TRAP ):  return ( snprintf( buf, OPCODE_FIELD_LEN, "TRAP" ));
            
        case ( OPC_GRP_SYS * 16 + OPC_DIAG ):  return ( snprintf( buf, OPCODE_FIELD_LEN, "DIAG" ));
            
        default: return ( snprintf( buf, OPCODE_FIELD_LEN, "**OPC:%d**", opCode ));
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the instruction opererands. An opcode consist of the instruction group and the opcode family. We
// construct the final opcode for the case statement.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStr( char *buf, uint32_t instr, int rdx ) {
    
    uint32_t opCode = extractField( instr, 30, 2 ) * 16 + extractField( instr, 26, 4 );
    
    switch ( opCode ) {
            
        case ( OPC_GRP_ALU * 16 + OPC_ADD ):
        case ( OPC_GRP_ALU * 16 + OPC_SUB ):
        case ( OPC_GRP_ALU * 16 + OPC_AND ):
        case ( OPC_GRP_ALU * 16 + OPC_OR  ):
        case ( OPC_GRP_ALU * 16 + OPC_XOR ):
        case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
            
            if ( extractBit( instr, 19 ))
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, %d",
                                 extractRegR( instr ),
                                 extractRegB( instr ),
                                 extractImm15( instr )));
            else
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegA( instr ),
                                  extractRegB( instr )));
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
            
            switch ( extractField( instr, 19, 3 )) {
                    
                case 0: { // EXTR
                    
                    if ( extractBit( instr, 13 ))
                        
                        return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, SAR, %d",
                                          extractRegR( instr ),
                                          extractRegB( instr ),
                                          extractField( instr, 0, 6 )));
                    else
                        
                        return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, %d, %d",
                                          extractRegR( instr ),
                                          extractRegB( instr ),
                                          extractField( instr, 6, 6 ),
                                          extractField( instr, 0, 6 )));
                }
                    
                case 1: { // DEP
                    
                    if ( extractBit( instr, 14 )) {
                        
                        if ( extractBit( instr, 13 ))
                            
                            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, %d, SAR, %d",
                                             extractRegR( instr ),
                                             extractField( instr, 15, 4 ),
                                             extractField( instr, 0, 6 )));
                        else
                            
                            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, %d, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 6, 6 ),
                                             extractField( instr, 0, 6 )));
                    }
                    else {
                        
                        if ( extractBit( instr, 13 ))
                            
                            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, SAR, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 0, 6 )));
                        else
                            
                            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, %d, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 6, 6 ),
                                             extractField( instr, 0, 6 )));
                    }
                }
                    
                case 2: { // DSR
                    
                    if ( extractBit( instr, 13 ))
                        
                        return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                                          extractRegR( instr ),
                                          extractRegB( instr ),
                                          extractRegA( instr )));
                    else
                        return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d, %d",
                                         extractRegR( instr ),
                                         extractRegB( instr ),
                                         extractRegA( instr ),
                                         extractField( instr, 0, 6 )));
                }
                    
                default: return ( snprintf( buf, OPERAND_FIELD_LEN, "**BITOP**" ));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
            
            if ( extractBit( instr, 19 ))
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, %d",
                                 extractRegR( instr ),
                                 extractRegB( instr ),
                                 extractImm15( instr )));
            else
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr ),
                                  extractRegA( instr )));
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, %d",
                             extractRegR( instr ),
                             extractImm20( instr )));
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_ADD ):
        case ( OPC_GRP_MEM * 16 + OPC_SUB ):
        case ( OPC_GRP_MEM * 16 + OPC_AND ):
        case ( OPC_GRP_MEM * 16 + OPC_OR  ):
        case ( OPC_GRP_MEM * 16 + OPC_XOR ):
        case ( OPC_GRP_MEM * 16 + OPC_CMP ):
        case ( OPC_GRP_MEM * 16 + OPC_LD  ):
        case ( OPC_GRP_MEM * 16 + OPC_ST  ):
        case ( OPC_GRP_MEM * 16 + OPC_LDR ):
        case ( OPC_GRP_MEM * 16 + OPC_STC ): {
            
            if ( extractField( instr, 19, 3 ) == 0 )
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, %d(R%d)",
                                 extractRegR( instr ),
                                 extractImm13( instr ),
                                 extractRegB( instr )));
            
            else
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d(R%d)",
                                 extractRegR( instr ),
                                 extractRegA( instr ),
                                 extractRegB( instr )));
        }
            
        case ( OPC_GRP_BR * 16 + OPC_B ): {
            
            int cursor = snprintf( buf, OPERAND_FIELD_LEN, ", %d", extractImm19( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", R%d", extractRegR( instr ));
            
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BR ): {
            
            int cursor = snprintf( buf, OPERAND_FIELD_LEN, "R%d", extractRegB( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", R%d", extractRegR( instr ));
            
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BV ): {
            
            int cursor = snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d",
                                  extractRegB( instr ),
                                  extractRegA( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", R%d", extractRegR( instr ));
            
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BB ): {
            
            int cursor = snprintf( buf, OPERAND_FIELD_LEN, "R%d", extractRegR( instr ));
            
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", SAR" );
            else  cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", %d", extractField( instr, 13, 6 ));
            
            cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", %d", extractImm13( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_CBR ):
        case ( OPC_GRP_BR * 16 + OPC_MBR ):  {
            
            int cursor = snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr ));
            
            cursor += snprintf( buf + cursor, OPERAND_FIELD_LEN, ", %d", extractImm15( instr ));
            
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MR ): {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, C%d",
                              extractRegR( instr ),
                              extractRegB( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_LDPA ): {
            
            if ( extractField( instr, 19, 3 ) == 0 )
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, %d(R%d)",
                                 extractRegR( instr ),
                                 extractImm13( instr ),
                                 extractRegB( instr )));
            
            else
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d(R%d)",
                                 extractRegR( instr ),
                                 extractRegA( instr ),
                                 extractRegB( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
            
            if ( extractBit( instr, 14 ))
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr )));
            
            else
                
                return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr ),
                                  extractRegA( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                              extractRegR( instr ),
                              extractRegB( instr ),
                              extractRegA( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_CA ): {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d",
                              extractRegR( instr ),
                              extractRegB( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MST ): {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d", extractRegR( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_RFI ): {
            
            return ( 0 );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TRAP ):  {
            
            return ( 0 );  // ??? to do ...
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_DIAG ):  {
            
            return ( snprintf( buf, OPERAND_FIELD_LEN, "R%d, R%d, R%d",
                              extractRegR( instr ),
                              extractRegB( instr ),
                              extractRegA( instr )));
        }
            
        default: return ( snprintf( buf, OPERAND_FIELD_LEN, "**OPC:%d**", opCode ));
    }
}

} // namespace


//------------------------------------------------------------------------------------------------------------
// Format an instruction. An instruction has generally three parts. The opCode, the opCode options and the
// operands. An instruction can be formatted as a whole string, or as two groups with opcode and operands
// separated. We need this split for the code window to show the date in two aligned fields.
//
//------------------------------------------------------------------------------------------------------------
T64DisAssemble::T64DisAssemble( ) { }

int T64DisAssemble::getOpCodeFieldWidth( ) {
    
    return ( 16 );
}

int T64DisAssemble::getOperandsFieldWidth( ) {
    
    return ( 32 );
}

int T64DisAssemble::formatOpCode( char *buf, int bufLen, uint32_t instr ) {
    
    if ( bufLen < getOpCodeFieldWidth( )) return ( buildOpCodeStr( buf, instr ));
    else                                  return ( -1 );
}

int T64DisAssemble::formatOperands( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen < getOperandsFieldWidth( )) return ( buildOperandStr( buf, instr, rdx ));
    else                                    return ( -1 );
}

int T64DisAssemble::formatInstr( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen >= ( getOpCodeFieldWidth( ) + 1 + getOperandsFieldWidth( ))) {
        
        int cursor = 0;
        cursor += buildOpCodeStr( buf + cursor, instr );
        cursor += snprintf( buf + cursor, 4, " " );
        cursor += buildOperandStr( buf + cursor, instr, rdx );
        return ( cursor );
    }
    else return ( -1 );
}
