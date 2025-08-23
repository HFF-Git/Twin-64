//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - DisAssembler
//
//----------------------------------------------------------------------------------------
// The instruction disassemble routines will format an instruction word in a human 
// readable form. An instruction has the general format
//
//      OpCode [ Opcode Options ] [ target ] [ source ]
//
// The disassemble routine will analyze an instruction word and present the instruction
// portion in the above order. The result is a string with the disassembled instruction.
//
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - DisAssembler
// Copyright (C) 2025 - 2025 Helmut Fieres
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
#include "T64-InlineAsm.h"
#include "T64-Util.h"

//----------------------------------------------------------------------------------------
// Local name space.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// The disassemble string consists of two parts. Normally, the string is just one 
// string with both opCode and operand parts. For an aligned display of opCode and 
// operand parts, the two constants specify how big the field will get.
//
//----------------------------------------------------------------------------------------
const int LEN_16 = 16;
const int LEN_32 = 32;

//----------------------------------------------------------------------------------------
// A little helper function to display the comparison condition codes in human readable
// form. The function returns the characters written.
//
//----------------------------------------------------------------------------------------
int printCondField( char *buf, uint32_t cmpCode ) {
    
    switch( cmpCode ) {
            
        case 0:  return ( snprintf( buf, 4, ".EQ" ));
        case 1:  return ( snprintf( buf, 4, ".LT" ));
        case 2:  return ( snprintf( buf, 4, ".NE" ));
        case 3:  return ( snprintf( buf, 4, ".LE" ));
        case 4:  return ( snprintf( buf, 4, ".EV" ));
        case 5:  return ( snprintf( buf, 4, ".OD" ));

        case 6:
        case 7:
        default: return ( snprintf( buf, 4, ".**" ));
    }
}

//----------------------------------------------------------------------------------------
// A little helper function to display the DW field. Note that we do not display the 
// "D" option. It is the default and thus will not be shown.
//
//----------------------------------------------------------------------------------------
int printDwField( char *buf, uint32_t dw ) {
    
    switch( dw ) {
            
        case 0:  return ( snprintf( buf, 4, ".B" ));
        case 1:  return ( snprintf( buf, 4, ".H" ));
        case 2:  return ( snprintf( buf, 4, ".W" ));
        case 3:  return ( 0 );
        default: return ( snprintf( buf, 4, ".*" ));
    }
}

//----------------------------------------------------------------------------------------
// Decode the opcode and opcode option portion. An opcode consist of the instruction
// group and the opcode family. We construct the final opcode for the case statement.
//
//----------------------------------------------------------------------------------------
int buildOpCodeStr( char *buf, uint32_t instr ) {
    
    uint32_t opCode = extractInstrOpGroup( instr ) * 16 + extractInstrOpCode( instr );
    
    switch( opCode ) {
            
        case ( OPC_GRP_ALU * 16 + OPC_ADD ): {
            
            return ( snprintf( buf, LEN_16, "ADD" ));
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_ADD ): {
            
            int cursor = snprintf( buf, LEN_16, "ADD" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_SUB ): {
            
            return ( snprintf( buf, LEN_16, "SUB" ));
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_SUB ): {
            
            int cursor = snprintf( buf, LEN_16, "SUB" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_AND ): {
            
            int cursor = snprintf( buf, LEN_16, "AND" );
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_AND ): {
            
            int cursor = snprintf( buf, LEN_16, "AND" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_OR ): {
            
            int cursor = snprintf( buf, LEN_16, "OR" );
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_OR ): {
            
            int cursor = snprintf( buf, LEN_16, "OR" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".C" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_XOR ): {
            
            int cursor = snprintf( buf, LEN_16, "XOR" );
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".**" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_XOR ): {
            
            int cursor = snprintf( buf, LEN_16, "XOR" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".**" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".N" );
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
            
            int cursor = snprintf( buf, LEN_16, "CMP" );
            printCondField( buf + cursor, extractInstrField( instr, 20, 2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_CMP ): {
            
            int cursor = snprintf( buf, LEN_16, "CMP" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            cursor += printCondField( buf + cursor, extractInstrField( instr,20,2 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
            
            switch ( extractInstrField( instr, 19, 3 )) {
                    
                case 0: { // EXTR
                    
                    int cursor = snprintf( buf, LEN_16, "EXTR" );
                    if ( extractInstrBit( instr, 12 )) 
                        cursor += snprintf( buf + cursor, 4, ".S" );
                    return ( cursor );
                }
                    
                case 1: { // DEP
                    
                    int cursor = snprintf( buf, LEN_16, "DEP" );
                    if ( extractInstrBit( instr, 12 )) 
                        cursor += snprintf( buf + cursor, 4, ".Z" );
                    if ( extractInstrBit( instr, 14 )) 
                        cursor += snprintf( buf + cursor, 4, ".I" );
                    return ( cursor );
                }
                    
                case 2: { // DSR
                    
                    return ( snprintf( buf, LEN_16, "DSR" ));
                }
                    
                default: {
                    
                    return ( snprintf( buf, LEN_16, "**BITOP**" ));
                }
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
          
            switch ( extractInstrField( instr, 19, 3 )) {
                    
                case 2: return ( snprintf( buf, LEN_16, "SHL1A" )); 
                case 4: return ( snprintf( buf, LEN_16, "SHL2A" ));
                case 6: return ( snprintf( buf, LEN_16, "SHL3A" )); 
                
                case 3: return ( snprintf( buf, LEN_16, "SHR1A" )); 
                case 5: return ( snprintf( buf, LEN_16, "SHR2A" )); 
                case 7: return ( snprintf( buf, LEN_16, "SHR3A" ));
               
                default: return ( snprintf( buf, LEN_16, "**SHAOP**" ));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
            
            switch ( extractInstrField( instr, 20, 2 )) {
                  
                case 0:  return ( snprintf( buf, LEN_16, "ADDIL" ));
                case 1:  return ( snprintf( buf, LEN_16, "LDI.L" ));
                case 2:  return ( snprintf( buf, LEN_16, "LDI.S" ));
                case 3:  return ( snprintf( buf, LEN_16, "LDI.U" ));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_LDO ): {
            
            return ( snprintf( buf, LEN_16, "LDO" ));
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_LD ): {
            
            int cursor = snprintf( buf, LEN_16, "LD" );
            
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".M" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".**" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_ST ): {
            
            int cursor = snprintf( buf, LEN_16, "ST" );
            if ( extractInstrBit( instr, 20 )) cursor += snprintf( buf + cursor, 4, ".M" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".**" );
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_LDR ): {
            
            int cursor = snprintf( buf, LEN_16, "LDR" );
            if ( extractInstrField( instr, 19, 3) != 0 ) {
                
                cursor += snprintf( buf + cursor, 4, ".**" );
            }
            
            return ( cursor );
        }
            
        case ( OPC_GRP_MEM * 16 + OPC_STC ): {
            
            int cursor = snprintf( buf, LEN_16, "STC" );
            if ( extractInstrField( instr, 19, 3) != 0 ) {
                
                cursor += snprintf( buf + cursor, 4, ".**" );
            }

            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_B ): {
            
            int cursor = snprintf( buf, LEN_16, "B" );
            if ( extractInstrField( instr, 20, 2) != 0 ) 
                cursor += snprintf( buf + cursor, 4, ".**" );
            if ( extractInstrBit( instr, 19 ))           
                cursor += snprintf( buf + cursor, 4, ".G" );
            return ( cursor );
        }

        case ( OPC_GRP_BR * 16 + OPC_BE ): {
            
            return ( snprintf( buf, LEN_16, "BE" ));
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BR ): {
            
            return ( snprintf( buf, LEN_16, "BR" ));
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BV ): {
            
            return ( snprintf( buf, LEN_16, "BV" ));
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BB ): {
            
            int cursor = snprintf( buf, LEN_16, "BB" );
            if ( extractInstrBit( instr, 21 )) cursor += snprintf( buf + cursor, 4, ".**" );
            if ( extractInstrBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".T" );
            else  cursor += snprintf( buf + cursor, 4, ".F" );
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_CBR ): {
            
            int cursor = snprintf( buf, LEN_16, "CBR" );
            if ( extractInstrBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".**" );
            cursor += printCondField( buf + cursor, extractInstrField( instr, 19, 3 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_MBR ): {
            
            int cursor = snprintf( buf, LEN_16, "MBR" );
            if ( extractInstrBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".**" );
            cursor += printCondField( buf + cursor, extractInstrField( instr, 19, 3 ));
            return ( cursor );
        }

        case ( OPC_GRP_BR * 16 + OPC_ABR ): {
            
            int cursor = snprintf( buf, LEN_16, "ABR" );
            if ( extractInstrBit( instr, 19 )) cursor += snprintf( buf + cursor, 4, ".**" );
            cursor += printCondField( buf + cursor, extractInstrField( instr, 19, 3 ));
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MR ): {
          
            if ( extractInstrField( instr, 19, 3 ) == 0 )
                return ( snprintf( buf, 8, "MFCR "));
            else if ( extractInstrField( instr, 19, 3 ) == 1 )   
                return ( snprintf( buf, 8, "MTCR "));
            else if ( extractInstrField( instr, 19, 3 ) == 2 )   
                return ( snprintf( buf, 8, "MFIA "));
            else 
                return ( snprintf( buf, LEN_16, "**MROP**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_LPA ): {
            
            int cursor = 0;
        
            if ( extractInstrField( instr, 19, 3 ) == 0 ) 
                cursor = snprintf( buf, LEN_16, "LPA" );
            else                                     
                cursor = snprintf( buf, LEN_16, "**LPAOP**" );
            
            cursor += printDwField( buf + cursor, extractInstrDwField( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
            
            int cursor = 0;
            
            if ( extractInstrField( instr, 19, 3 ) == 0 ) 
                cursor = snprintf( buf, LEN_16, "PRB" );
            else                                    
                cursor = snprintf( buf, LEN_16, "**PRBOP**" );
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
            
            if ( extractInstrField( instr, 19, 3 ) == 0 ) 
                return ( snprintf( buf, LEN_16, "ITLB" ));
            else if ( extractInstrField( instr, 19, 3 ) == 1 ) 
                return ( snprintf( buf, LEN_16, "PTLB" ));
            else return ( snprintf( buf, 8, "**TLB**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_CA ): {
            
            if ( extractInstrField( instr, 19, 3 ) == 0 ) 
                return ( snprintf( buf, LEN_16, "PCA" ));
            else if ( extractInstrField( instr, 19, 3 ) == 1 ) 
                return ( snprintf( buf, LEN_16, "FCA" ));
            else 
                return ( snprintf( buf, 8, "**CA**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MST ): {
            
            if ( extractInstrField( instr, 19, 3 ) == 0 ) 
                return ( snprintf( buf, LEN_16, "RSM" ));
            else if ( extractInstrField( instr, 19, 3 ) == 1 )
                 return ( snprintf( buf, LEN_16, "SSM" ));
            else 
                return ( snprintf( buf, 8, "**MST**" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_RFI ):   {
            
            return ( snprintf( buf, LEN_16, "RFI" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TRAP ):  {
            
            return ( snprintf( buf, LEN_16, "TRAP" ));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_DIAG ):  {
            
            return ( snprintf( buf, LEN_16, "DIAG" ));
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_NOP ): {
            
            return ( snprintf( buf, LEN_16, "NOP" ));
        }
            
        default: {
            
            return ( snprintf( buf, LEN_16, "**OPC:%d**", opCode ));
        }
    }
}

//----------------------------------------------------------------------------------------
// Decode the instruction operands. An opcode consist of the instruction group and 
// the opcode family. We construct the final opcode for the case statement.
//
//----------------------------------------------------------------------------------------
int buildOperandStr( char *buf, uint32_t instr, int rdx ) {
    
    uint32_t opCode = 
        extractInstrField( instr, 30, 2 ) * 16 + extractInstrField( instr, 26, 4 );
    
    switch ( opCode ) {
            
        case ( OPC_GRP_ALU * 16 + OPC_ADD ):
        case ( OPC_GRP_ALU * 16 + OPC_SUB ):
        case ( OPC_GRP_ALU * 16 + OPC_AND ):
        case ( OPC_GRP_ALU * 16 + OPC_OR  ):
        case ( OPC_GRP_ALU * 16 + OPC_XOR ):
        case ( OPC_GRP_ALU * 16 + OPC_CMP ): {
            
            if ( extractInstrBit( instr, 19 )) {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d, %d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ),
                                   extractInstrImm15( instr )));
            }
            else {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ),
                                   extractInstrRegA( instr )));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_BITOP ): {
            
            switch ( extractInstrField( instr, 19, 3 )) {
                    
                case 0: { // EXTR
                    
                    if ( extractInstrBit( instr, 13 )) {
                        
                        return ( snprintf( buf, LEN_32, "R%d, R%d, SAR, %d",
                                           extractInstrRegR( instr ),
                                           extractInstrRegB( instr ),
                                           extractInstrField( instr, 0, 6 )));
                    }
                    else {
                        
                        return ( snprintf( buf, LEN_32, "R%d, R%d, %d, %d",
                                           extractInstrRegR( instr ),
                                           extractInstrRegB( instr ),
                                           extractInstrField( instr, 6, 6 ),
                                           extractInstrField( instr, 0, 6 )));
                    }
                }
                    
                case 1: { // DEP
                    
                    if ( extractInstrBit( instr, 14 )) {
                        
                        if ( extractInstrBit( instr, 13 )) {
                            
                            return ( snprintf( buf, LEN_32, "R%d, %d, SAR, %d",
                                               extractInstrRegR( instr ),
                                               extractInstrField( instr, 15, 4 ),
                                               extractInstrField( instr, 0, 6 )));
                        }
                        else {
                            
                            return ( snprintf( buf, LEN_32, "R%d, R%d, %d, %d",
                                               extractInstrRegR( instr ),
                                               extractInstrRegB( instr ),
                                               extractInstrField( instr, 6, 6 ),
                                               extractInstrField( instr, 0, 6 )));
                        }
                    }
                    else {
                        
                        if ( extractInstrBit( instr, 13 )) {
                            
                            return ( snprintf( buf, LEN_32, "R%d, R%d, SAR, %d",
                                               extractInstrRegR( instr ),
                                               extractInstrRegB( instr ),
                                               extractInstrField( instr, 0, 6 )));
                        }
                        else {
                            
                            return ( snprintf( buf, LEN_32, "R%d, R%d, %d, %d",
                                               extractInstrRegR( instr ),
                                               extractInstrRegB( instr ),
                                               extractInstrField( instr, 6, 6 ),
                                               extractInstrField( instr, 0, 6 )));
                        }
                    }
                }
                    
                case 2: { // DSR
                    
                    if ( extractInstrBit( instr, 13 )) {
                        
                        return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                                           extractInstrRegR( instr ),
                                           extractInstrRegB( instr ),
                                           extractInstrRegA( instr )));
                    }
                    else {
                        
                        return ( snprintf( buf, LEN_32, "R%d, R%d, R%d, %d",
                                           extractInstrRegR( instr ),
                                           extractInstrRegB( instr ),
                                           extractInstrRegA( instr ),
                                           extractInstrField( instr, 0, 6 )));
                    }
                }
                    
                default: return ( snprintf( buf, LEN_32, "**BITOP**" ));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_SHAOP ): {
            
            if ( extractInstrBit( instr, 19 )) {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d, %d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ),
                                   extractInstrImm15( instr )));
            }
            else {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ),
                                   extractInstrRegA( instr )));
            }
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_IMMOP ): {
            
            return ( snprintf( buf, LEN_32, "R%d, %d",
                              extractInstrRegR( instr ),
                              extractInstrImm20( instr )));
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
            
            if ( extractInstrBit( instr, 19 ) == 0 ) {
                
                return ( snprintf( buf, LEN_32, "R%d, %d(R%d)",
                                   extractInstrRegR( instr ),
                                   extractInstrScaledImm13( instr ),
                                   extractInstrRegB( instr )));
            }
            else {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d(R%d)",
                                   extractInstrRegR( instr ),
                                   extractInstrRegA( instr ),
                                   extractInstrRegB( instr )));
            }
        }

        case ( OPC_GRP_ALU * 16 + OPC_LDO ): {

            return ( snprintf( buf, LEN_32, "R%d, %d(R%d)",
                               extractInstrRegR( instr ),
                               extractInstrImm15( instr ),
                               extractInstrRegB( instr )));
        }
            
        case ( OPC_GRP_BR * 16 + OPC_B ): {
            
            int cursor = snprintf( buf, LEN_32, ", %d", extractInstrImm19( instr ));
            
            if ( extractInstrField( instr, 26, 4 ) != 0 ) {

                cursor += snprintf( buf + cursor, LEN_32, ", R%d", 
                                    extractInstrRegR( instr ));
            }
            
            return ( cursor );
        }

        case ( OPC_GRP_BR * 16 + OPC_BE ): {

            // ??? what is a good syntax ?
            
             int cursor = snprintf( buf, LEN_32, "R%d", extractInstrRegR( instr ));
            
            cursor += snprintf( buf + cursor, LEN_32, ", %d", extractInstrImm15( instr ));
            
            if ( extractInstrField( instr, 26, 4 ) != 0 ) {

                cursor += snprintf( buf + cursor, LEN_32, ", R%d", 
                                    extractInstrRegR( instr ));
            }
            
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BR ): {
            
            int cursor = snprintf( buf, LEN_32, "R%d", extractInstrRegB( instr ));
            
            if ( extractInstrField( instr, 26, 4 ) != 0 ) {

                cursor += snprintf( buf + cursor, LEN_32, ", R%d", 
                                    extractInstrRegR( instr ));
            }
            
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BV ): {
            
            int cursor = snprintf( buf, LEN_32, "R%d, R%d",
                                   extractInstrRegB( instr ),
                                   extractInstrRegA( instr ));
            
            if ( extractInstrField( instr, 26, 4 ) != 0 ) {

                cursor += snprintf( buf + cursor, LEN_32, ", R%d", 
                                    extractInstrRegR( instr ));
            }

            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_BB ): {
            
            int cursor = snprintf( buf, LEN_32, "R%d", extractInstrRegR( instr ));
            
            if ( extractInstrBit( instr, 20 )) 
                cursor += snprintf( buf + cursor, LEN_32, ", SAR" );
            else  
                cursor += snprintf( buf + cursor, LEN_32, ", %d", 
                                    extractInstrField( instr, 13, 6 ));
            
            cursor += snprintf( buf + cursor, LEN_32, ", %d", extractInstrImm13( instr ));
            return ( cursor );
        }
            
        case ( OPC_GRP_BR * 16 + OPC_CBR ):
        case ( OPC_GRP_BR * 16 + OPC_MBR ):  {
            
            int cursor = snprintf( buf, LEN_32, "R%d, R%d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ));
            
            cursor += snprintf( buf + cursor, LEN_32, ", %d", extractInstrImm15( instr ));
            
            return ( cursor );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MR ): {

            if (( extractInstrField( instr, 19, 3 ) == 0 ) ||
                ( extractInstrField( instr, 19, 3 ) == 1 )) {
            
                return ( snprintf( buf, LEN_32, "R%d, C%d",
                                    extractInstrRegR( instr ),
                                    extractInstrRegB( instr )));
            }
            else if ( extractInstrField( instr, 19, 3 ) == 2 ) {

                return ( snprintf( buf, LEN_32, "R%d",
                                    extractInstrRegR( instr )));
            }
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_LPA ): {
            
            if ( extractInstrField( instr, 19, 3 ) == 0 ) {
                
                return ( snprintf( buf, LEN_32, "R%d, %d(R%d)",
                                   extractInstrRegR( instr ),
                                   extractInstrImm13( instr ),
                                   extractInstrRegB( instr )));
            }
            else {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d(R%d)",
                                   extractInstrRegR( instr ),
                                   extractInstrRegA( instr ),
                                   extractInstrRegB( instr )));
            }
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_PRB ): {
            
            if ( extractInstrBit( instr, 14 )) {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr )));
            }
            else {
                
                return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                                   extractInstrRegR( instr ),
                                   extractInstrRegB( instr ),
                                   extractInstrRegA( instr )));
            }
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TLB ): {
            
            return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                               extractInstrRegR( instr ),
                               extractInstrRegB( instr ),
                               extractInstrRegA( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_CA ): {
            
            return ( snprintf( buf, LEN_32, "R%d, R%d",
                               extractInstrRegR( instr ),
                               extractInstrRegB( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_MST ): {
            
            return ( snprintf( buf, LEN_32, "R%d", extractInstrRegR( instr )));
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_RFI ): {
            
            return ( 0 );
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_TRAP ):  {
            
            return ( 0 );  // ??? to do ...
        }
            
        case ( OPC_GRP_SYS * 16 + OPC_DIAG ):  {
            
            return ( snprintf( buf, LEN_32, "R%d, R%d, R%d",
                               extractInstrRegR( instr ),
                               extractInstrRegB( instr ),
                               extractInstrRegA( instr )));
        }
            
        case ( OPC_GRP_ALU * 16 + OPC_NOP ): {
            
            return ( 0 );
        }
            
        default: return ( snprintf( buf, LEN_32, "**OPC:%d**", opCode ));
    }
}

} // namespace


//----------------------------------------------------------------------------------------
// Format an instruction. An instruction has generally three parts. The opCode, the 
// opCode options and the operands. An instruction can be formatted as a whole string,
// or as two groups with opcode and operands separated. We need this split for the 
// code window to show the date in two aligned fields.
//
//----------------------------------------------------------------------------------------
T64DisAssemble::T64DisAssemble( ) { }

int T64DisAssemble::getOpCodeFieldWidth( ) {
    
    return ( LEN_16 );
}

int T64DisAssemble::getOperandsFieldWidth( ) {
    
    return ( LEN_32 );
}

int T64DisAssemble::formatOpCode( char *buf, int bufLen, uint32_t instr ) {
    
    if ( bufLen < getOpCodeFieldWidth( )) 
        return ( buildOpCodeStr( buf, instr ));
    else                                 
        return ( -1 );
}

int T64DisAssemble::formatOperands( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen < getOperandsFieldWidth( )) 
        return ( buildOperandStr( buf, instr, rdx ));
    else                                    
        return ( -1 );
}

int T64DisAssemble::formatInstr( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    if ( bufLen >= ( getOpCodeFieldWidth( ) + 1 + getOperandsFieldWidth( ))) {
        
        char operandBuf[ LEN_32 ];

        int cursor = buildOpCodeStr( buf, instr );
        
        buildOperandStr( operandBuf, instr, rdx );
        if ( strlen( operandBuf ) > 0 ) {
            
            cursor += snprintf( buf + cursor, LEN_32, " %s", operandBuf );
        }
        
        return ( cursor );
    }
    else return ( -1 );
}
