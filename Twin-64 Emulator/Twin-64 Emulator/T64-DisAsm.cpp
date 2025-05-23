#if 0

// ??? rework for a T64 one line disassembler
//------------------------------------------------------------------------------------------------------------
//
// VCPU32 - A 32-bit CPU - Disassembler
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
// VCPU32 - A 32-bit CPU - Disassembler
// Copyright (C) 2022 - 2024 Helmut Fieres
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


/*
//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const struct {
    
    uint8_t op;
    char    name[ 6 ];
    
} opCodeTab[ ] = {
    
    { .op = OP_ALU_NOP,     .name = "NOP"   },
    { .op = OP_ALU_AND,     .name = "AND"   },
    { .op = OP_ALU_OR,      .name = "OR"    },
    { .op = OP_ALU_XOR,     .name = "XOR"   },
    { .op = OP_ALU_ADD,     .name = "ADD"   },
    { .op = OP_ALU_SUB,     .name = "SUB"   },
    { .op = OP_ALU_CMP,     .name = "CMP"   },
    { .op = OP_ALU_EXTR,    .name = "EXTR"  },
    { .op = OP_ALU_DEP,     .name = "DEP"   },
    { .op = OP_ALU_DSR,     .name = "DSR"   },
    { .op = OP_ALU_CHK,     .name = "CHK"   },
    
    { .op = OP_MEM_LD,      .name = "LD"    },
    { .op = OP_MEM_ST,      .name = "ST"    },
    { .op = OP_MEM_LDR,     .name = "LDR"   },
    { .op = OP_MEM_STC,     .name = "STC"   },
    { .op = OP_MEM_AND,     .name = "AND"   },
    { .op = OP_MEM_OR,      .name = "OR"    },
    { .op = OP_MEM_XOR,     .name = "XOR"   },
    { .op = OP_MEM_ADD,     .name = "ADD"   },
    { .op = OP_MEM_SUB,     .name = "SUB"   },
    { .op = OP_MEM_CMP,     .name = "CMP"   },
    
    { .op = OP_BR_LDI,      .name = "LDI"   },
    { .op = OP_BR_ADDIL,    .name = "ADDIL" },
    { .op = OP_BR_LDO,      .name = "LDO"   },
    { .op = OP_BR_B,        .name = "B"     },
    { .op = OP_BR_BR,       .name = "BR"    },
    { .op = OP_BR_BV,       .name = "BV"    },
    { .op = OP_BR_CBR,      .name = "CBR"   },
    { .op = OP_BR_TBR,      .name = "TBR"   },
    { .op = OP_BR_MBR,      .name = "MBR"   },
    
    { .op = OP_SYS_MR,      .name = "MR"    },
    { .op = OP_SYS_MST,     .name = "MST"   },
    { .op = OP_SYS_LPA,     .name = "LPA"   },
    { .op = OP_SYS_PRB,     .name = "PRB"   },
    { .op = OP_SYS_ITLB,    .name = "ITLB"  },
    { .op = OP_SYS_DTLB,    .name = "DTLB"  },
    { .op = OP_SYS_PCA,     .name = "PCA"   },
    { .op = OP_SYS_DIAG,    .name = "DIAG"  },
    { .op = OP_SYS_BRK,     .name = "BRK"   },
    { .op = OP_SYS_RFI,     .name = "RFI"   }
};
*/

//------------------------------------------------------------------------------------------------------------
// Local namespace. These routines are not visible outside this source file.
//
//------------------------------------------------------------------------------------------------------------
namespace {



static inline bool isAligned( T64Word adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline bool isInRange( T64Word adr, T64Word low, T64Word high ) {
    
    return(( adr >= low ) && ( adr <= high ));
}

static inline T64Word roundup( uT64Word arg ) {
    
    return( arg ); // for now ...
}

static inline T64Word extractBit( T64Word arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline T64Word extractField( T64Word arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline T64Word extractSignedField( T64Word arg, int bitpos, int len ) {
    
    T64Word field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
    
}

static inline T64Word depositField( T64Word word, int bitpos, int len, T64Word value) {
    
    T64Word mask = (( 1ULL << len ) - 1 ) << bitpos;
    return ( word & ~mask ) | (( value << bitpos ) & mask );
}

bool willAddOverflow( T64Word a, T64Word b ) {
    
    if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
    if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
    return false;
}

bool willSubOverflow( T64Word a, T64Word b ) {
    
    if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
    if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
    return false;
}

bool willShiftLftOverflow( T64Word a, int shift ) {
    
    if (( shift < 0 ) || ( shift >= 64 )) return true;
    if ( a == 0 ) return false;
    
    T64Word max = INT64_MAX >> shift;
    T64Word min = INT64_MIN >> shift;
    
    return (( a > max ) || ( a < min ));
}


//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const char *opCodeToStr( uint8_t opCode ) {
    
    int entries = sizeof( opCodeTab ) / sizeof( opCodeTab[0]);
    
    for ( int i = 0; i < entries; i++ ) {
        
        if ( opCodeTab[ i ].op == opCode ) return((char *) &opCodeTab[ i ].name );
    }
                                                  
    return((char*) &"***" );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint8_t strToOpCode( char *opStr ) {
    
    int entries = sizeof( opCodeTab ) / sizeof( opCodeTab[0]);
    
    for ( int i = 0; i < entries; i++ ) {
       
        if ( strcmp( opStr, opCodeTab[ i ].name ) == 0 ) return ( opCodeTab[ i ].op );
    }
                                                  
    return( 0 );
}




//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
// Format hex with '_' every 4, 8, 12, or 16 digits, no "0x", left-padded with zeros if desired
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
//
//------------------------------------------------------------------------------------------------------------
// Format decimal with '_' every 3 digits (from right to left)
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





// ??? see what we keep...

//------------------------------------------------------------------------------------------------------------
// Instruction decoding means to get to bits and bit fields. Here is a set of helper functions.
//
//------------------------------------------------------------------------------------------------------------
bool getBit( uint32_t arg, int pos ) {
    
    return( arg & ( 1U << ( 31 - ( pos % 32 ))));
}

uint32_t getBitField( uint32_t arg, int pos, int len, bool sign = false ) {
    
    pos = pos % 32;
    len = len % 32;

    uint32_t tmpM = ( 1U << len ) - 1;
    uint32_t tmpA = arg >> ( 31 - pos );
    
    if (( sign ) && ( getBit( arg, pos - len + 1 ))) {
        
        return( tmpA | ( ~ tmpM ));
    }
    else return( tmpA & tmpM );
}

//------------------------------------------------------------------------------------------------------------
// "printImmVal" display an immediate value in the selected radix. Octals and hex numbers are printed unsigned
// quantities, decimal numbers are interpreted as signed integers. Most often decimal notation is used to
// specify offsets on indexed addressing modes. The function returns the characters written. The maximum
// size of what is added is set to 16.
//
//------------------------------------------------------------------------------------------------------------
int printImmVal( char *buf, uint32_t val, int rdx = 16 ) {
    
    if ( val == 0 ) return( snprintf( buf, sizeof( buf ), "0" ));
    
    else {
       
        if      ( rdx == 10 )  return( snprintf( buf, 16, "%d", ((int) val )));
        else if ( rdx == 8  )  return( snprintf( buf, 16, "%#0o", val ));
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
int formatComparisonCodes( char *buf, uint32_t cmpCode ) {
    
    switch( cmpCode ) {
            
        case CC_EQ:  return( snprintf( buf, 4, "EQ" ));
        case CC_LT:  return( snprintf( buf, 4, "LT" ));
        case CC_NE:  return( snprintf( buf, 4, "NE" ));
        case CC_LE:  return( snprintf( buf, 4, "LE" ));
        default:     return( snprintf( buf, 4, "**" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// A little helper function to display the test condition in human readable form. The function returns the
// characters written. The maximum size of what is added is set to 2.
//
//------------------------------------------------------------------------------------------------------------
int formatTestCodes( char *buf, uint32_t tstCode ) {
    
    switch( tstCode ) {
            
        case TC_EQ: return( snprintf( buf, 4, "EQ" ));
        case TC_LT: return( snprintf( buf, 4, "LT" ));
        case TC_GT: return( snprintf( buf, 4, "GT" ));
        case TC_EV: return( snprintf( buf, 4, "EV" ));
        case TC_NE: return( snprintf( buf, 4, "NE" ));
        case TC_LE: return( snprintf( buf, 4, "LE" ));
        case TC_GE: return( snprintf( buf, 4, "GE" ));
        case TC_OD: return( snprintf( buf, 4, "OD" ));
        default:    return( snprintf( buf, 4, "**" ));
    }
}

//------------------------------------------------------------------------------------------------------------
// There are instructions that use the operand argument format. This routine will format such an operand.
// The function returns the characters written. The function returns the characters written.
//
//------------------------------------------------------------------------------------------------------------
int formatOperandModeField( char *buf, uint32_t instr, int rdx = 10 ) {
    
    uint32_t opMode = getBitField( instr, 13, 2 );
    
    switch ( opMode ) {
            
        case OP_MODE_IMM: {
            
            return( printImmVal( buf, getBitField( instr, 31, 18, true ), 10 ));
            
        } break;
          
        case OP_MODE_REG: {
            
            return( snprintf( buf, 16, "r%d, r%d", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 )));
            
        } break;
            
        case OP_MODE_REG_INDX: {
           
            return( snprintf( buf, 16, "r%d(r%d)", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 )));
            
        } break;
            
        case OP_MODE_INDX: {
            
            int len = printImmVal( buf, getBitField( instr, 27, 12, true ), 10 );
            return ( snprintf( buf + len , 16, "(r%d)", getBitField( instr, 31, 4 )));
            
        } break;
            
        default: return( 0 );
    }
}

//------------------------------------------------------------------------------------------------------------
// Each instruction has an opCode. For most of the instructions, the mnemonic is just a simple mapping to the
// name stored in the opCode table. However, for some instructions we need to look at more options in the
// instruction word to come up with the mnemonic. Currently we append to the opCode that allow for a word
// length to append a character to indicate byte, half-word or word access.
//
// There are also instructions have the same opCode but result in a different mnemonic. For example the LD
// instruction will decode to four different mnemonics. The function returns the characters written.
//
//------------------------------------------------------------------------------------------------------------
int formatOpCode( char *buf, uint32_t instr ) {
    
    uint32_t opCode = getBitField( instr, 5, 6 );
    int     cursor  = 0;
    
    if ( opCodeTab[ opCode ].flags & OP_MODE_INSTR ) {
        
        cursor += snprintf( buf + cursor, 16, "%s", opCodeTab[ opCode ].mnemonic );
        
        if (( getBitField( instr, 13, 2 ) == 2 ) || ( getBitField( instr, 13, 2 ) == 3 )) {
            
            switch ( getBitField( instr, 15, 2 )) {
                    
                case 0:  cursor += snprintf( buf + cursor, 16, "B" ); break;
                case 1:  cursor += snprintf( buf + cursor, 16, "H" ); break;
                case 2:  break;
                default: cursor += snprintf( buf + cursor, 16, "**dw**" );
            }
        }
    }
    else {
        
        switch ( opCode ) {
                
            case OP_LD: 
            case OP_ST: {
                
                cursor += snprintf( buf + cursor, 16, "%s", opCodeTab[ opCode ].mnemonic );
                
                switch ( getBitField( instr, 15, 2 )) {
                        
                    case 0:  cursor += snprintf( buf + cursor, 16, "B" ); break;
                    case 1:  cursor += snprintf( buf + cursor, 16, "H" ); break;
                    case 2:  break;
                    default: cursor += snprintf( buf + cursor, 16, "**dw**" );
                }
                
            } break;
            
            default: cursor += snprintf( buf + cursor, 16, "%s", opCodeTab[ opCode ].mnemonic );
        }
    }
    
    return( cursor );
}

//------------------------------------------------------------------------------------------------------------
// Some instructions have a set of further qualifiers. They are listed after a "." and are single characters.
// If there is no option in a given set is set or it is the common case value, nothing is printed. The
// function returns the characters written.
//
//------------------------------------------------------------------------------------------------------------
int formatOpCodeOptions( char *buf, uint32_t instr ) {
    
    uint32_t    opCode = getBitField( instr, 5, 6 );
    int         cursor = 0;
    
    switch ( opCode ) {
            
        case OP_LD:     
        case OP_ST:
        case OP_LDA:    
        case OP_STA: {
            
            if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, ".M" );
            
        } break;
            
        case OP_ADD:    
        case OP_ADC:
        case OP_SUB:    
        case OP_SBC: {
            
            if ( getBitField( instr, 11, 2 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "L" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "O" );
            }
            
        } break;
            
        case OP_AND:
        case OP_OR: {
            
            if ( getBitField( instr, 11, 2 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "N" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "C" );
            }
            
        } break;
        
        case OP_XOR: {
            
            if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, ".N" );
            
        } break;
            
        case OP_CMP: 
        case OP_CMPU: {
            
            cursor += snprintf( buf + cursor, 4, "." );
            cursor += formatComparisonCodes( buf + cursor, getBitField( instr, 11, 2 ));
            
        } break;
            
        case OP_EXTR: {
            
            if ( getBitField( instr, 11, 2 )) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "S" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "A" );
            }
            
        } break;
            
        case OP_DEP: {
            
            if ( getBitField( instr, 12, 3 )) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "Z" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "A" );
                if ( getBit( instr, 12 )) cursor += snprintf( buf + cursor, 4, "I" );
            }
            
        } break;
            
        case OP_DSR: {
            
            if ( getBit( instr, 11 )) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "A" );
            }
            
        } break;
            
        case OP_SHLA: {
            
            if ( getBitField( instr, 12, 3 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 ))   cursor += snprintf( buf + cursor, 4, "L" );
                if ( getBit( instr, 11 ))   cursor += snprintf( buf + cursor, 4, "O" );
            }
            
        } break;
            
        case OP_CMR: {
            
            cursor += snprintf( buf + cursor, 4, "." );
            cursor += formatTestCodes( buf + cursor, getBitField( instr, 13, 4 ));
            
        } break;
            
        case OP_CBR: 
        case OP_CBRU: {
            
            cursor += snprintf( buf + cursor, 4, "." );
            cursor += formatComparisonCodes( buf + cursor, getBitField( instr, 7, 2 ));
            
        } break;
            
        case OP_MST: {
            
            switch ( getBitField( instr, 11, 2 )) {
                    
                case 0:                                                    break;
                case 1:     cursor += snprintf( buf + cursor, 4, ".S" );   break;
                case 2:     cursor += snprintf( buf + cursor, 4, ".C" );   break;
                default:    cursor += snprintf( buf + cursor, 4, ".***" ); break;
            }
            
        } break;
            
        case OP_PRB: {
            
            if (( getBit( instr, 10 ) || getBit( instr, 11 ))) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "W" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "I" );
            }
           
        } break;
            
        case OP_ITLB: {
            
            if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, ".T" );
           
        } break;
            
        case OP_PTLB: {
            
            cursor += snprintf( buf + cursor, 4, "." );
            if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "T" );
            if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "M" );
          
        } break;
            
        case OP_PCA: {
            
            if (( getBit( instr, 10 ) || getBit( instr, 11 ))) {
                
                cursor += snprintf( buf + cursor, 4, "." );
                if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 4, "T" );
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 4, "M" );
                if ( getBit( instr, 14 )) cursor += snprintf( buf + cursor, 4, "F" );
            }
            
        } break;
    }
    
    cursor += snprintf( buf + cursor, 4, " " );
    return ( cursor );
}

//------------------------------------------------------------------------------------------------------------
// This routine display the instruction target. Most of the time it is a general register. For the STORE
// type instructions the target address is decoded and printed. Finally there are the MTR instructions which
// which will use a segment or control register as the target. There is one further exception. The BLE
// instruction will produce a register value, the return link stored in R0. This is however not shown in the
// disassembly printout. The function returns the characters written.
//
//------------------------------------------------------------------------------------------------------------
int formatTarget( char *buf, uint32_t instr, int rdx = 10 ) {
    
    uint32_t    opCode = getBitField( instr, 5, 6 );
    int         cursor = 0;
    
    if (( opCodeTab[ opCode ].flags & REG_R_INSTR ) && ( ! ( opCodeTab[ opCode ].flags & BRANCH_INSTR ))) {
        
        cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 9, 4 ));
    }
    else if ( opCodeTab[ opCode ].flags & STORE_INSTR ) {
        
        cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 9, 4 ));
    }
    else if ( opCode == OP_MR ) {
        
        if ( getBit( instr, 10 )) {
            
            if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 8, "c%d", getBitField( instr, 31, 5 ));
            else cursor += snprintf( buf + cursor, 8, "s%d", getBitField( instr, 31, 4 ));
        }
        else cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 9, 4 ));
    }
    
    return( cursor );
}

//------------------------------------------------------------------------------------------------------------
// Instruction have operands. For most of the instructions this is the operand field with the defined
// addressing modes. For others it is highly instruction specific. The operand routine also has a parameter
// to specify in what radix a value is shown. Address offsets are however always printed in decimal. The
// function returns the characters written.
//
//------------------------------------------------------------------------------------------------------------
int formatOperands( char *buf, uint32_t instr, int rdx = 10 ) {
    
    uint32_t    opCode = getBitField( instr, 5, 6 );
    int         cursor = 0;
    
    switch ( opCode ) {
            
        case OP_ADD:    case OP_ADC:    case OP_SUB:    case OP_SBC:    case OP_CMP:
        case OP_CMPU:   case OP_AND:    case OP_OR:     case OP_XOR: {
            
            cursor += snprintf( buf + cursor, 4, ", " );
            cursor += formatOperandModeField( buf + cursor, instr, rdx );
            
        } break;
            
        case OP_EXTR: {
            
            cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 31, 4 ));
            
            if ( ! getBit( instr, 11 )) {
                
                cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 27, 5 ));
                cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 21, 5 ));
                
            } else cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 21, 5 ));
            
        } break;
            
        case OP_DEP: {
            
            if ( getBit( instr, 12 ))   cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 31, 4 ));
            else                        cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 31, 4 ));
            
            if ( ! getBit( instr, 11 )) {
                
                cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 27, 5 ));
                cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 21, 5 ));
                
            } else cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 21, 5 ));
            
        } break;
            
        case OP_DSR: {
            
            cursor += snprintf( buf + cursor, 16, ", r%d, r%d", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            if ( ! getBit( instr, 11 )) cursor += snprintf( buf + cursor, 8, ", %d", getBitField( instr, 21, 5 ));
            
        } break;
            
        case OP_DS: {
            
            cursor += snprintf( buf + cursor, 16, ", r%d, r%d", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_LSID: {
            
            cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_CMR: {
            
            cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 27, 4 ));
            cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_DIAG: {
            
            cursor += snprintf( buf + cursor, 32, "r%d, r%d, r%d, %d",
                                getBitField( instr, 9, 4  ),
                                getBitField( instr, 27, 4 ),
                                getBitField( instr, 31, 4 ),
                                getBitField( instr, 13, 4 ));
        } break;
            
        case OP_LD: case OP_ST: case OP_LDR: case OP_STC: {
            
            if ( getBit( instr, 10 )) {
                
                if ( getBitField( instr, 13, 2 ) == 0 ) {
                    
                    cursor += snprintf( buf + cursor, 16, ", r%d(r%d)", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
                }
                else {
                    
                    cursor += snprintf( buf + cursor, 16, ", r%d(s%d, r%d)",
                                        getBitField( instr, 27, 4 ),
                                        getBitField( instr, 13, 2 ),
                                        getBitField( instr, 31, 4 ));
                }
            }
            else {
                
                cursor += snprintf( buf + cursor, 4, ", " );
                cursor += printImmVal( buf + cursor, getBitField( instr, 27, 12, true ), 10 );
               
                if ( getBitField( instr, 13, 2 ) == 0 ) {
                    
                    cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
                }
                else {
                    
                    cursor += snprintf( buf + cursor, 16, "(s%d, r%d)", getBitField( instr, 13, 2 ), getBitField( instr, 31, 4 ));
                }
            }
        
        } break;
            
        case OP_LDA: case OP_STA: {
            
            if ( getBit( instr, 10 )) {
                
                cursor += snprintf( buf + cursor, 16, ", r%d(r%d)", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            }
            else {
                
                cursor += snprintf( buf + cursor, 4, ", " );
                cursor += printImmVal( buf + cursor, getBitField( instr, 27, 12, true ), 10 );
                cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
            }
        
        } break;
            
        case OP_SHLA: {
            
            cursor += snprintf( buf + cursor, 16, ", r%d, %d", getBitField( instr, 27, 4 ),  getBitField( instr, 31, 4 ));
            
            if ( getBitField( instr, 21, 2 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", %d",  getBitField( instr, 21, 2 ));
            
        } break;
            
        case OP_LDIL:
        case OP_ADDIL: {
            
            cursor += snprintf( buf + cursor, 4, ", " );
            cursor += printImmVal( buf + cursor, getBitField( instr, 31, 22 ), rdx );
            
        } break;
            
        case OP_LDO: {
            
            cursor += snprintf( buf + cursor, 4, ", " );
            cursor += printImmVal( buf + cursor, getBitField( instr, 27, 18, true ), 10 );
            cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_B: 
        case OP_GATE: {
            
            cursor += printImmVal( buf + cursor, getBitField( instr, 31, 22, true ) << 2 , 10 );
                                  
            if ( getBitField( instr, 9, 4 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
        
        } break;
            
        case OP_BR: {
           
            cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
            if ( getBitField( instr, 9, 4 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
            
        } break;
            
        case OP_BV: {
           
            cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
            if ( getBitField( instr, 9, 4 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
            
        } break;
            
        case OP_BE: {
            
            cursor += printImmVal( buf + cursor, getBitField( instr, 23, 14, true ) << 2, 10 );
            cursor += snprintf( buf + cursor, 16, "(s%d,r%d)", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            if ( getBitField( instr, 9, 4 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
            
        } break;
            
        case OP_BVE: {
            
            if ( getBitField( instr, 27,4 )) cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 27,4 ));
            cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31,4 ));
            if ( getBitField( instr, 9, 4 ) > 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
            
        } break;
            
        case OP_CBR: 
        case OP_CBRU: {
            
            cursor += snprintf( buf + cursor, 16, "r%d, r%d, ", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            
            cursor += printImmVal( buf + cursor, getBitField( instr, 23, 16, true ) << 2, 10 );
            
        } break;
            
        case OP_MR: {
            
            if ( getBit( instr, 10 )) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 9, 4 ));
            else {
                
                if ( getBit( instr, 11 )) cursor += snprintf( buf + cursor, 8, ", c%d", getBitField( instr, 31, 5 ));
                else cursor += snprintf( buf + cursor, 8, ", s%d", getBitField( instr, 31, 3 ));
            }
            
        } break;
            
        case OP_MST: {
            
            cursor += snprintf( buf + cursor, 4, "," );
            switch( getBitField( instr, 11, 2 )) {
                    
                case 0:  cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 31, 4 ));  break;
                case 1:
                case 2:  cursor += snprintf( buf + cursor, 8, "0x%x", getBitField( instr, 31, 6 )); break;
                default: cursor += snprintf( buf + cursor, 4, "***" );
            }
            
        } break;
            
        case OP_PRB: {
            
            if ( getBitField( instr, 13, 2 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 16, ", (s%d, r%d)", getBitField(instr, 13, 2 ), getBitField( instr, 31, 4 ));
            }
            else cursor += snprintf( buf + cursor, 8, ", (r%d)", getBitField( instr, 31, 4 ));
            
            if ( getBit( instr, 11 ))   cursor += snprintf( buf + cursor, 8, ", %d", getBit( instr, 27 ));
            else                        cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 27, 4 ));
            
        } break;
        
        case OP_LDPA: {
            
            if ( getBitField( instr, 27, 4 ) != 0 ) cursor += snprintf( buf + cursor, 8, ", r%d", getBitField( instr, 27, 4 ));
            
            if ( getBitField( instr, 13, 2 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 16, "(s%d, r%d)", getBitField( instr, 13, 2 ), getBitField( instr, 31, 4 ));
            }
            else cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
                                                          
        } break;
            
        case OP_ITLB: {
            
            cursor += snprintf( buf + cursor, 8, "r%d, ", getBitField( instr, 9, 4 ));
            cursor += snprintf( buf + cursor, 16, "(s%d,r%d)", getBitField( instr, 27, 4 ), getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_PTLB:
        case OP_PCA:{
            
            if ( getBitField( instr, 27, 4 ) != 0 ) cursor += snprintf( buf + cursor, 8, "r%d", getBitField( instr, 27, 4 ));
            
            if ( getBitField( instr, 13, 2 ) > 0 ) {
                
                cursor += snprintf( buf + cursor, 16, "(s%d, r%d)", getBitField(instr, 13, 2 ), getBitField( instr, 31, 4 ));
            }
            else cursor += snprintf( buf + cursor, 8, "(r%d)", getBitField( instr, 31, 4 ));
            
        } break;
            
        case OP_BRK: {
            
            cursor += snprintf( buf + cursor, 16, "%d, %d", getBitField( instr, 9, 4 ), getBitField( instr, 31, 16 ));
           
        } break;
    }
    
    return( cursor );
}

}; // namespace


//************************************************************************************************************
//
// Object methods.
//
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
// The object constructor.
//
//------------------------------------------------------------------------------------------------------------
SimDisAsm::SimDisAsm( ) { }

//------------------------------------------------------------------------------------------------------------
// Print an instruction, nicely formatted. An instruction has generally four parts. The opCode, the opCode
// options, the source and the target. The opCode and options are grouped as are the target and operand.
//
//------------------------------------------------------------------------------------------------------------
int SimDisAsm::formatInstr( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    int cursor = 0;
    
    cursor += formatOpCodeAndOptions( buf + cursor, bufLen, instr, rdx );
    cursor += formatTargetAndOperands( buf + cursor, bufLen, instr );
    return( cursor );
}

int SimDisAsm::formatOpCodeAndOptions( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    int cursor = 0;
   
    cursor += formatOpCode( buf + cursor, instr );
    cursor += formatOpCodeOptions( buf + cursor, instr );
    return( cursor );
}

int SimDisAsm::formatTargetAndOperands( char *buf, int bufLen, uint32_t instr, int rdx ) {
    
    int cursor = 0;
   
    cursor += formatTarget( buf + cursor, instr, rdx );
    cursor += formatOperands( buf + cursor, instr, rdx );
    return( cursor );
}

int SimDisAsm::displayInstr( uint32_t instr, int rdx ) {
    
    int  cursor = 0;
    char buf[ 128 ];
    
    cursor += formatOpCodeAndOptions( buf + cursor, sizeof( buf ), instr );
    buf[ cursor ] = ' ';
    cursor += formatTargetAndOperands( buf + cursor, sizeof( buf ), instr, rdx );
    fprintf( stdout, "%s", buf );
    return( cursor );
}

int SimDisAsm::getOpCodeOptionsFieldWidth( ) {
    
    return( 12 );
}

int SimDisAsm::getTargetAndOperandsFieldWidth( ) {
    
    return( 16 );
}
#endif
