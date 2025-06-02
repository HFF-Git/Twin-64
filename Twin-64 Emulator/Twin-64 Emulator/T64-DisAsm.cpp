//------------------------------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - DisAssembler
//
//------------------------------------------------------------------------------------------------------------
// The instruction disassemble routine will format an instruction word in human readable form. An instruction
// has the general format
//
//      OpCode [ Opcode Options ] [ target ] [ source ] 
//
// The disassemble routine will analyze an instruction word and present the instruction portion in the above
// order.The result is a string with the disassembled instruction.
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

// how to best structure this code ?
// ??? seems like big case statements with sprintf ... ?


namespace {

//------------------------------------------------------------------------------------------------------------
//
// ??? where to assemble the pieces...
//------------------------------------------------------------------------------------------------------------
char opCodeBuf[ 128 ];

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
static inline uint32_t extractBit( uint32_t arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline uint32_t extractField( uint32_t arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline int extractSignedField( uint32_t arg, int bitpos, int len ) {
    
    uint32_t field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 32 )  return ( field << ( 32 - len )) >> ( 32- len );
    else             return ( field );
}

static inline int extractRegR( uint32_t instr ) {
    
    return( extractField( instr, 22, 4 ));
}

static inline int extractRegB( uint32_t instr ) {
    
    return( extractField( instr, 15, 4 ));
}

static inline int extractRegA( uint32_t instr ) {
    
    return( extractField( instr, 9, 4 ));
}

static inline int extractImm13( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 13 ));
}

static inline int extractImm15( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 15 ));
}

static inline int extractImm19( uint32_t instr ) {
    
    return( extractSignedField( instr, 0, 19 ));
}

static inline int extractImm20( uint32_t instr ) {
    
    return( extractField( instr, 0, 20 ));
}

//------------------------------------------------------------------------------------------------------------
//
// Format hex with '_' every 4, 8, 12, or 16 digits, no "0x", left-padded with zeros if desired
// Do we need it ?
//------------------------------------------------------------------------------------------------------------
void format_hex64( T64Word value, char *buf, int digits = 16 ) {
    
    if ( digits < 1 ) digits    = 1;
    if ( digits > 16 ) digits   = 16;
    
    int     shiftAmount     = 16 - digits;
    int     tmpBufIndex     = 0;
    char    tmpBuf[ 20 ];
    
    for ( int i = shiftAmount; i < 16; i++ ) {
        
        int digit = ( value >> ( i * 4 )) & 0xF;
        tmpBuf[ tmpBufIndex++ ] = "0123456789abcdef"[ digit ];
    }
    
    int out = 0;
    
    for ( int i = 0; i < tmpBufIndex; ++i) {
        
        if (( i > 0 ) && ( i % 4 == 0 )) buf[ out++ ] = '_';
        buf[ out++ ] = tmpBuf[ i ];
    }
    
    buf[ out ] = '\0';
}

//------------------------------------------------------------------------------------------------------------
//
// Format decimal with '_' every 3 digits (from right to left)
// Do we need it ?
//------------------------------------------------------------------------------------------------------------
void format_dec64( T64Word value, char *buf ) {
    
    char temp[32]; // enough to hold 20-digit uint64 + separators
    int len = 0;
    
    // Build reversed string with separators
    do {
        if (len > 0 && len % 3 == 0) {
            temp[len++] = '_';
        }
        temp[len++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    // Reverse to get final string
    for (int i = 0; i < len; ++i) {
        buf[i] = temp[len - 1 - i];
    }
    buf[len] = '\0';
}

//------------------------------------------------------------------------------------------------------------
// "printImmVal" display an immediate value in the selected radix. Hex numbers are printed in unsigned
// quantities, decimal numbers are interpreted as signed integers. Most often decimal notation is used to
// specify offsets on indexed addressing modes. The function returns the characters written. The maximum
// size of what is added is set to 16.
//
//------------------------------------------------------------------------------------------------------------
int printImmVal( char *buf, uint32_t val, int rdx = 16 ) {
    
    if ( val == 0 ) return( snprintf( buf, sizeof( buf ), "0" ));
    
    else {
       
        if      ( rdx == 10 )  return( snprintf( buf, 16, "%d", ((int) val )));
        else if ( rdx == 16 )  return( snprintf( buf, 16, "%#0x", val ));
        else                   return( snprintf( buf, 16, "**num***" ));
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
            
        case 0:  return( snprintf( buf, 4, "EQ" ));
        case 1:  return( snprintf( buf, 4, "LT" ));
        case 2:  return( snprintf( buf, 4, "NE" ));
        case 3:  return( snprintf( buf, 4, "LE" ));
        default: return( snprintf( buf, 4, "**" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// A little helper function to display the DW field.
//
//------------------------------------------------------------------------------------------------------------
int printDwField( char *buf, uint32_t dw ) {
    
    switch( dw ) {
            
        case 0:  return( snprintf( buf, 4, "B" ));
        case 1:  return( snprintf( buf, 4, "H" ));
        case 2:  return( snprintf( buf, 4, "W" ));
        case 3:  return( snprintf( buf, 4, "D" ));
        default: return( snprintf( buf, 4, "*" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the ALU group opcodes and options.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStrALU( char *buf, uint32_t instr ) {
    
    uint32_t opCode = extractField( instr, 26, 4 );
    
    switch( opCode ) {
            
        case OPC_ADD: return( snprintf( buf, 16, "ADD" ));
        case OPC_SUB: return( snprintf( buf, 16, "ADD" ));
            
        case OPC_AND: {
            
            int cursor = snprintf( buf, 16, "AND" );
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_OR: {
            
            int cursor = snprintf( buf, 16, "OR" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_XOR: {
            
            int cursor = snprintf( buf, 16, "XOR" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_CMP: {
            
            int cursor = snprintf( buf, 16, "CMP" );
            printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return( cursor );
        }
            
        case OPC_BITOP: {
           
            switch ( extractField( instr, 19, 3 )) {
                    
                case 0: {
                    
                    int cursor = snprintf( buf, 16, "EXTR" );
                    if ( extractBit( instr, 12 )) cursor += snprintf( buf + cursor, 4, ".S" );
                    return( cursor );
                }
                    
                case 1: {
                    
                    int cursor = snprintf( buf, 16, "DEP" );
                    if ( extractBit( instr, 12 )) cursor += snprintf( buf + cursor, 4, ".Z" );
                    if ( extractBit( instr, 14 )) cursor += snprintf( buf + cursor, 4, ".I" );
                    return( cursor );
                }
                    
                case 2: return( snprintf( buf, 16, "DSR" ));
                    
                default: return ( 0 );
            }
        }
        
        case OPC_SHAOP: {
            
            int cursor = 0;
            
            switch ( extractField( instr, 19, 3 )) {
                    
                case 2: cursor += snprintf( buf + cursor, 8, "SHL1A" ); break;
                case 4: cursor += snprintf( buf + cursor, 8, "SHL2A" ); break;
                case 6: cursor += snprintf( buf + cursor, 8, "SHL3A" ); break;
               
                case 3: cursor += snprintf( buf + cursor, 8, "SHR1A" ); break;
                case 5: cursor += snprintf( buf + cursor, 8, "SHR2A" ); break;
                case 7: cursor += snprintf( buf + cursor, 8, "SHR3A" ); break;
                default: ;
            }
            
            if ( extractBit( instr, 14 )) cursor += snprintf( buf + cursor, 4, ".I" );
            
            return( 0 );
        }
        
        case OPC_IMMOP: {
            
            switch ( extractField( instr, 20, 2 )) {
                    
                case 1:  return ( snprintf( buf, 8, "LDI.L" ));
                case 2:  return ( snprintf( buf, 8, "LDI.S" ));
                case 3:  return ( snprintf( buf, 8, "LDI.U" ));
                default: return ( 0 );
            }
            
            return( 0 );
        }
            
        case OPC_LDO: return( snprintf( buf, 8, "LDO" ));
        
        default: return ( 0 );
    }
    
    return ( 0 );
}

//------------------------------------------------------------------------------------------------------------
// Decode the ALU group instruction opererands.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStrALU( char *buf, uint32_t instr, int rdx ) {
    
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_ADD:
        case OPC_SUB:
        case OPC_AND:
        case OPC_OR:
        case OPC_XOR:
        case OPC_CMP: {
            
            if ( extractBit( instr, 19 ))
                return( snprintf( buf, 16, "R%d, R%d, %d",
                                 extractRegR( instr ),
                                 extractRegB( instr ),
                                 extractImm15( instr )));
            else
                return( snprintf( buf, 16, "R%d,R%d, R%d",
                                 extractRegR( instr ),
                                 extractRegA( instr ),
                                 extractRegB( instr )));
        }
            
        case OPC_BITOP: {
            
            switch ( extractField( instr, 19, 3 )) {
                    
                case 0: { // EXTR
                    
                    if ( extractBit( instr, 13 ))
                        return( snprintf( buf, 16, "R%d, R%d, SAR, %d",
                                         extractRegR( instr ),
                                         extractRegB( instr ),
                                         extractField( instr, 0, 6 )));
                    else
                        return( snprintf( buf, 16, "R%d, R%d, %d, %d",
                                         extractRegR( instr ),
                                         extractRegB( instr ),
                                         extractField( instr, 6, 6 ), extractField( instr, 0, 6 )));
                }
                    
                case 1: { // DEP
                    
                    if ( extractBit( instr, 14 )) {
                        
                        if ( extractBit( instr, 13 ))
                            return( snprintf( buf, 16, "R%d, %d, SAR, %d",
                                             extractRegR( instr ),
                                             extractField( instr, 15, 4 ),
                                             extractField( instr, 0, 6 )));
                        else
                            return( snprintf( buf, 16, "R%d, R%d, %d, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 6, 6 ),
                                             extractField( instr, 0, 6 )));
                    }
                    else {
                        
                        if ( extractBit( instr, 13 ))
                            return( snprintf( buf, 16, "R%d, R%d, SAR, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 0, 6 )));
                        else
                            return( snprintf( buf, 16, "R%d, R%d, %d, %d",
                                             extractRegR( instr ),
                                             extractRegB( instr ),
                                             extractField( instr, 6, 6 ),
                                             extractField( instr, 0, 6 )));
                    }
                }
                    
                case 2: { // DSR
                    
                    if ( extractBit( instr, 13 ))
                        return( snprintf( buf, 16, "R%d, R%d, R%d, SAR",
                                         extractRegR( instr ),
                                         extractRegB( instr ),
                                         extractRegA( instr )));
                    else
                        return( snprintf( buf, 16, "R%d, R%d, R%d, %d",
                                         extractRegR( instr ),
                                         extractRegB( instr ),
                                         extractRegA( instr ),
                                         extractField( instr, 0, 6 )));
                }
        
                default: return ( 0 );;
            }
        }
            
        case OPC_SHAOP: {
            
            if ( extractBit( instr, 19 ))
                return( snprintf( buf, 16, "R%d, R%d, %d",
                                 extractRegR( instr ),
                                 extractRegB( instr ),
                                 extractImm15( instr )));
            else
                return( snprintf( buf, 16, "R%d,R%d, R%d",
                                 extractRegR( instr ),
                                 extractRegA( instr ),
                                 extractRegB( instr )));
        }
            
        case OPC_IMMOP: {
            
            return( snprintf( buf, 16, "R%d, %d",
                             extractRegR( instr ),
                             extractImm20( instr )));
        }
            
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the MEM group opcodes and options.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStrMEM( char *buf,uint32_t instr ) {
   
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_ADD: return( snprintf( buf, 16, "ADD" ));
        case OPC_SUB: return( snprintf( buf, 16, "ADD" ));
            
        case OPC_AND: {
            
            int cursor = snprintf( buf, 16, "AND" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_OR: {
            
            int cursor = snprintf( buf, 16, "OR" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_XOR: {
            
            int cursor = snprintf( buf, 16, "XOR" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return( cursor );
        }
            
        case OPC_CMP: {
            
            int cursor = snprintf( buf, 16, "CMP" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return( cursor );
        }
            
        case OPC_LD: {
            
            int cursor = snprintf( buf, 4, "LD" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return( cursor );
        }
            
        case OPC_ST: {
            
            int cursor = snprintf( buf, 4, "ST" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return( cursor );
        }
            
        case OPC_LDR: return( snprintf( buf, 8, "LDR" ));
        case OPC_STC: return( snprintf( buf, 8, "STC" ));
            
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the MEM group instruction opererands.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStrMEM( char *buf,uint32_t instr, int rdx ) {
 
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_ADD:
        case OPC_SUB:
        case OPC_AND:
        case OPC_OR:
        case OPC_XOR:
        case OPC_CMP:
        case OPC_LD:
        case OPC_ST:
        case OPC_LDR:
        case OPC_STC: {
            
            if ( extractField( instr, 19, 3 ) == 0 )
                return( snprintf( buf, 16, "R%d, %d(R%d)",
                                 extractRegR( instr ), extractImm13( instr ), extractRegB( instr )));
            else
                return( snprintf( buf, 16, "R%d, R%d(R%d)",
                                 extractRegR( instr ), extractRegA( instr ), extractRegB( instr )));
        }
            
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
//  Decode the BR group opcodes and options.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStrBR( char *buf, uint32_t instr ) {
 
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_B: {
            
            int cursor = snprintf( buf, 4, "B" );
            if ( extractBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".G" );
            return( cursor );
        }
            
        case OPC_BR: return( snprintf( buf, 8, "BR" ));
            
        case OPC_BV: return( snprintf( buf, 8, "BV" ));
            
        case OPC_BB: {
            
            int cursor = snprintf( buf, 4, "BB" );
            if ( extractBit( instr, 19 ))   cursor += snprintf( buf + cursor, 4, ".T" );
            else                            cursor += snprintf( buf + cursor, 4, ".F" );
            return( cursor );
        }
    
        case OPC_CBR: {
            
            int cursor = snprintf( buf, 16, "CBR" );
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return( cursor );
        }
            
        case OPC_MBR: {
            
            int cursor = snprintf( buf, 16, "MBR" );
            cursor += printCondField( buf + cursor, extractField( instr, 20, 2 ));
            return( cursor );
        }
            
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the BR group instruction opererands.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStrBR( char *buf, uint32_t instr, int rdx ) {
 
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_B: {
            
            int cursor = snprintf( buf, 16, ",%d", extractImm19( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, 8, ", R%d", extractRegR( instr ));
            return( cursor );
        }
            
        case OPC_BR: {
            
            int cursor = snprintf( buf, 16, "R%d", extractRegB( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, 8, ", R%d", extractRegR( instr ));
            return( cursor );
        }
            
        case OPC_BV: {
            
            int cursor = snprintf( buf, 16, "R%d, R%d",
                                  extractRegB( instr ),
                                  extractRegA( instr ));
            
            if ( extractField( instr, 26, 4 ) != 0 )
                cursor += snprintf( buf + cursor, 8, ", R%d", extractRegR( instr ));
            return( cursor );
        }
            
        case OPC_BB: {
            
            int cursor = snprintf( buf, 16, "R%d", extractRegR( instr ));
            
            if ( extractBit( instr, 20 )) cursor += snprintf( buf + cursor, 8, ", SAR" );
            else  cursor += snprintf( buf + cursor, 8, ", %d", extractField( instr, 13, 6 ));
            
            cursor += snprintf( buf + cursor, 16, ", %d", extractImm13( instr ));
            return( cursor );
        }
            
        case OPC_CBR:
        case OPC_MBR:  {
            
            int cursor = snprintf( buf, 16, "R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr ));
            cursor += snprintf( buf + cursor, 16, ", %d", extractImm15( instr ));
            return( cursor );
        }
        
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
//  Decode the SYS group opcodes and options.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStrSYS( char *buf, uint32_t instr ) {
 
    switch ( extractField( instr, 26, 4 )) {
         
        case OPC_MR: {
            
            if ( extractField( instr, 19, 3 ) == 0 ) return( snprintf( buf, 8, "MFCR" ));
            else                                     return( snprintf( buf, 8, "MTCR" ));
        }
            
        case OPC_LDPA: {
            
            int cursor = snprintf( buf, 4, "LDPA" );
            cursor += printDwField( buf + cursor, extractField( instr, 13, 2 ));
            return( cursor );
        }
            
        case OPC_PRB: {
            
            int cursor = snprintf( buf, 4, "PRB" );
            if ( extractBit( instr, 19 ))   cursor += snprintf( buf + cursor, 4, ".P" );
            else                            cursor += snprintf( buf + cursor, 4, ".U" );
            return( cursor );
        }
            
        case OPC_TLB: {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return( snprintf( buf, 8, "ITLB" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return( snprintf( buf, 8, "PTLB" ));
            else return( snprintf( buf, 8, "****" ));
        }
            
        case OPC_CA: {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return( snprintf( buf, 8, "PCA" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return( snprintf( buf, 8, "FCA" ));
            else return( snprintf( buf, 8, "***" ));
        }
            
        case OPC_MST: {
            
            if      ( extractField( instr, 19, 3 ) == 0 ) return( snprintf( buf, 8, "RSM" ));
            else if ( extractField( instr, 19, 3 ) == 1 ) return( snprintf( buf, 8, "SSM" ));
            else return( snprintf( buf, 8, "***" ));
        }
            
        case OPC_RFI:   return( snprintf( buf, 8, "RFI" ));
            
        case OPC_TRAP:  return( 0 );  // ??? to do ...
            
        case OPC_DIAG:  return( snprintf( buf, 8, "DIAG" ));
            
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Decode the SYS group instruction opererands.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStrSYS( char *buf, uint32_t instr, int rdx ) {
 
    switch ( extractField( instr, 26, 4 )) {
            
        case OPC_MR: return( snprintf( buf, 16, "R%d, C%d", extractRegR( instr ), extractRegB( instr )));
            
        case OPC_LDPA: {
            
            if ( extractField( instr, 19, 3 ) == 0 )
                return( snprintf( buf, 16, "R%d, %d(R%d)",
                                 extractRegR( instr ), extractImm13( instr ), extractRegB( instr )));
            else
                return( snprintf( buf, 16, "R%d, R%d(R%d)",
                                 extractRegR( instr ), extractRegA( instr ), extractRegB( instr )));
        }
            
        case OPC_PRB: {
            
            if ( extractBit( instr, 14 ))
                return ( snprintf( buf, 16, "R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr )));
            else
                return ( snprintf( buf, 16, "R%d, R%d, R%d",
                                  extractRegR( instr ),
                                  extractRegB( instr ),
                                  extractRegA( instr )));
            
        }
            
        case OPC_TLB: {
            
            return ( snprintf( buf, 16, "R%d, R%d, R%d",
                                extractRegR( instr ),
                                extractRegB( instr ),
                                extractRegA( instr )));
        }
            
        case OPC_CA: {
            
            return ( snprintf( buf, 16, "R%d, R%d",
                                extractRegR( instr ),
                                extractRegB( instr )));
        }
            
        case OPC_MST: {
            
            return ( snprintf( buf, 16, "R%d",
                                extractRegR( instr )
                                ));
        }
            
        case OPC_RFI:   return( 0 );
            
        case OPC_TRAP:  return( 0 );  // ??? to do ...
            
        case OPC_DIAG:  {
            
            return ( snprintf( buf, 16, "R%d, R%d, R%d",
                                extractRegR( instr ),
                                extractRegB( instr ),
                                extractRegA( instr )));
        }
        
        default: return ( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Build the opcode and options string. This routine selects the instruction group handler.
//
//------------------------------------------------------------------------------------------------------------
int buildOpCodeStr( char *buf, uint32_t instr ) {
   
    switch ( extractField( instr, 30, 2 )) {
            
        case OPC_GRP_ALU: return( buildOpCodeStrALU( buf, instr ));
        case OPC_GRP_MEM: return( buildOpCodeStrMEM( buf, instr ));
        case OPC_GRP_BR:  return( buildOpCodeStrBR( buf, instr ));
        case OPC_GRP_SYS: return( buildOpCodeStrSYS( buf, instr ));
        default: return( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Build the operand string. This routine selects the instruction group handler.
//
//------------------------------------------------------------------------------------------------------------
int buildOperandStr( char *buf, uint32_t instr, int rdx ) {
    
    switch ( extractField( instr, 30, 2 )) {
            
        case OPC_GRP_ALU: return( buildOperandStrALU( buf, instr, rdx ));
        case OPC_GRP_MEM: return( buildOperandStrMEM( buf, instr, rdx ));
        case OPC_GRP_BR:  return( buildOperandStrBR( buf, instr, rdx ));
        case OPC_GRP_SYS: return( buildOperandStrSYS( buf, instr, rdx ));
        default: return( 0 );
    }
}

} // namespace


//------------------------------------------------------------------------------------------------------------
// Format an instruction. An instruction has generally three parts. The opCode, the opCode options and the
// operands. An instruction cna be formatted as a whole string, or as two groups with opcode and operands
// separated. We need this split for the code window to show the date in two aligned fields.
//
//------------------------------------------------------------------------------------------------------------
int getOpCodeFieldWidth( ) {
    
    return( 12 );
}

int getOperandsFieldWidth( ) {
    
    return( 16 );
}

int formatOpCode( char *buf, int bufLen, uint32_t instr ) {
    
    if ( bufLen < getOpCodeFieldWidth( )) return ( buildOpCodeStr( buf, instr ));
    else                                  return( -1 );
}

int formatOperands( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen < getOperandsFieldWidth( )) return( buildOperandStr( buf, instr, rdx ));
    else                                    return( -1 );
}

int formatInstr( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen < ( getOpCodeFieldWidth( ) + 1 + getOperandsFieldWidth( ))) {
      
        int cursor = 0;
        cursor += buildOpCodeStr( buf + cursor, instr );
        cursor += snprintf( buf +cursor, 4, " " );
        cursor += buildOperandStr( buf + cursor, instr, rdx );
        return( cursor );
    }
    else return( -1 );
}
