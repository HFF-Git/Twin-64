

// ??? rework for a T64 one line assembler
//------------------------------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - One Line Assembler
//
//------------------------------------------------------------------------------------------------------------
// The one line assembler assembles an instruction without further context. It is intended to for testing
// instructions in the simulator. There is no symbol table or any concept of assembling multiple instructions.
// The instruction to generate test is completely self sufficient. The parser is a straightforward recursive
// descendant parser, LL1 grammar. It uses the C++ try / catch to escape when an error is detected. Considering
// that we only have one line to parse, there is no need to implement a better parser error recovery method.
//
//------------------------------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - One Line Assembler
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

//
// 52 bit virtual address: 20bit segment, 32 offset. => 4 Exabytes
// 4Gb Segments, 1Mio Segments
//

// Assembler notes:
//
// opCode [ .<opt> ] Rr, <imm>
// opCode [ .<opt> ] Rr, Ra
// opCode [ .<opt> ] Rr, Ra, Rb
// opCode [ .<opt> ] Rr, ( Rb )
// opCode [ .<opt> ] Rr, <ofs> ( Rb )
// opCode [ .<opt> ] Rr, Ra ( Rb )
// opCode [ .<opt> ] <target> [, Rr ]
//
// -> very few different formats
//
// ( <instr1> : <instr2> )      -> parallel
// ( <instr1> :: <instr2> )     -> serialized



//------------------------------------------------------------------------------------------------------------
// Local namespace. These routines are not visible outside this source file.
//
//------------------------------------------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const int   TOK_INPUT_LINE_SIZE = 256;
const int   MAX_TOKEN_NAME_SIZE = 32;
const int   TOK_STR_SIZE        = 256;
const char  EOS_CHAR            = 0;

//------------------------------------------------------------------------------------------------------------
// Command line tokens and expression have a type.
//
//------------------------------------------------------------------------------------------------------------
enum SimTokTypeId : uint16_t {

    TYP_NIL                 = 0,        TYP_SYM                 = 5,
    TYP_IDENT               = 6,        TYP_PREDEFINED_FUNC     = 7,
    
    TYP_NUM                 = 10,       TYP_STR                 = 11,       TYP_BOOL                = 12,
    TYP_ADR                 = 13,       TYP_EXT_ADR             = 14,       TYP_OP_CODE             = 15,
    TYP_OP_CODE_S           = 16,
    
    TYP_REG                 = 20,       TYP_REG_PAIR            = 21,
    
    TYP_GREG                = 30,       TYP_SREG                = 31,       TYP_CREG                = 32,
    TYP_PSTATE_PREG         = 33,
};

//------------------------------------------------------------------------------------------------------------
// Tokens are the labels for reserved words and symbols recognized by the tokenizer objects. Tokens have a
// name, a token id, a token type and an optional value with further data.
//
//------------------------------------------------------------------------------------------------------------
enum SimTokId : uint16_t {
    
    //--------------------------------------------------------------------------------------------------------
    // General tokens and symbols.
    //
    //--------------------------------------------------------------------------------------------------------
    TOK_NIL                 = 0,        TOK_ERR                 = 1,        TOK_EOS                 = 2,
    TOK_COMMA               = 3,        TOK_PERIOD              = 4,        TOK_LPAREN              = 5,
    TOK_RPAREN              = 6,        TOK_QUOTE               = 7,        TOK_PLUS                = 8,
    TOK_MINUS               = 9,        TOK_MULT                = 10,       TOK_DIV                 = 11,
    TOK_MOD                 = 12,       TOK_REM                 = 13,       TOK_NEG                 = 14,
    TOK_AND                 = 15,       TOK_OR                  = 16,       TOK_XOR                 = 17,
    TOK_EQ                  = 18,       TOK_NE                  = 19,       TOK_LT                  = 20,
    TOK_GT                  = 21,       TOK_LE                  = 22,       TOK_GE                  = 23,
    
    //--------------------------------------------------------------------------------------------------------
    // Token symbols. They are just reserved names used in commands and functions. Their type and optional
    // value is defined in the token tables.
    //
    //--------------------------------------------------------------------------------------------------------
    TOK_IDENT               = 100,      TOK_NUM                 = 101,      TOK_STR                 = 102,
   
    //--------------------------------------------------------------------------------------------------------
    // General, Segment and Control Registers Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    REG_SET                 = 4000,
    
    GR_0                    = 4100,     GR_1                    = 4101,     GR_2                    = 4102,
    GR_3                    = 4103,     GR_4                    = 4104,     GR_5                    = 4105,
    GR_6                    = 4106,     GR_7                    = 4107,     GR_8                    = 4108,
    GR_9                    = 4109,     GR_10                   = 4110,     GR_11                   = 4111,
    GR_12                   = 4112,     GR_13                   = 4113,     GR_14                   = 4114,
    GR_15                   = 4115,     GR_SET                  = 4116,
    
    SR_0                    = 4200,     SR_1                    = 4201,     SR_2                    = 4202,
    SR_3                    = 4203,     SR_4                    = 4204,     SR_5                    = 4205,
    SR_6                    = 4206,     SR_7                    = 4207,     SR_SET                  = 4208,
    
    CR_0                    = 4300,     CR_1                    = 4301,     CR_2                    = 4302,
    CR_3                    = 4303,     CR_4                    = 4304,     CR_5                    = 4305,
    CR_6                    = 4306,     CR_7                    = 4307,     CR_8                    = 4308,
    CR_9                    = 4309,     CR_10                   = 4310,     CR_11                   = 4311,
    CR_12                   = 4312,     CR_13                   = 4313,     CR_14                   = 4314,
    CR_15                   = 4315,     CR_16                   = 4316,     CR_17                   = 4317,
    CR_18                   = 4318,     CR_19                   = 4319,     CR_20                   = 4320,
    CR_21                   = 4321,     CR_22                   = 4322,     CR_23                   = 4323,
    CR_24                   = 4324,     CR_25                   = 4325,     CR_26                   = 4326,
    CR_27                   = 4327,     CR_28                   = 4328,     CR_29                   = 4329,
    CR_30                   = 4330,     CR_31                   = 4331,     CR_SET                  = 4332,
    

    //--------------------------------------------------------------------------------------------------------
    // OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    OP_CODE_LD              = 5000,     OP_CODE_LDR             = 5004,
    OP_CODE_ST              = 5010,     OP_CODE_STC             = 5014,

    OP_CODE_ADD             = 5020,     OP_CODE_SUB             = 5030,

    OP_CODE_AND             = 5040,     OP_CODE_OR              = 5045,     OP_CODE_XOR             = 5050,
    
    OP_CODE_CMP             = 5060,

    OP_CODE_EXTR            = 5071,     OP_CODE_DEP             = 5072,     OP_CODE_DSR             = 5073,
    
    OP_CODE_SHLA            = 5074,
    
    OP_CODE_LDIL            = 5076,     OP_CODE_ADDIL           = 5077,     OP_CODE_LDO             = 5078,

    OP_CODE_B               = 5080,     OP_CODE_GATE            = 5081,     OP_CODE_BR              = 5082,
    OP_CODE_BV              = 5083,
    
    OP_CODE_CBR             = 5086,     OP_CODE_TBR             = 5088,     OP_CODE_MBR             = 5089,

    OP_CODE_MR              = 5090,     OP_CODE_MST             = 5091,     OP_CODE_DS              = 5092,
    OP_CODE_LDPA            = 5093,     OP_CODE_PRB             = 5094,     OP_CODE_ITLB            = 5095,
    OP_CODE_PTLB            = 5096,     OP_CODE_PCA             = 5097,     OP_CODE_DIAG            = 5098,
    
    OP_CODE_RFI             = 5100,     OP_CODE_BRK             = 5101,
    
    //--------------------------------------------------------------------------------------------------------
    // Synthetic OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    OP_CODE_S_NOP           = 6000,     OP_CODE_S_SHL           = 6001,     OP_CODE_S_SHR           = 6002,
    OP_CODE_S_ASL           = 6003,     OP_CODE_S_ASR           = 6004,     OP_CODE_S_ROR           = 6005,
    OP_CODE_S_ROL           = 6006,
    
    //--------------------------------------------------------------------------------------------------------
    // The last token ID. This ID is used to terminate a token table list.
    //
    //--------------------------------------------------------------------------------------------------------
    TOK_LAST                = 9999
};

//------------------------------------------------------------------------------------------------------------
// The command line interpreter as well as the one line assembler work the command line or assembly line
// processed as a list of tokens. A token found in a string is recorded using the token structure. The token
// types are numeric, virtual address and string.
//
//------------------------------------------------------------------------------------------------------------
struct SimToken {

    char            name[ MAX_TOKEN_NAME_SIZE ] = { };
    SimTokTypeId    typ                         = TYP_NIL;
    SimTokId        tid                         = TOK_NIL;
    
    union {
        
        struct {    int64_t val;                    };
        struct {    uint32_t seg;   uint32_t ofs;   }; // phase out...
        struct {    char str[ TOK_STR_SIZE ];       };
    };
};

//------------------------------------------------------------------------------------------------------------
// The global token table or the one line assembler. All reserved words are allocated in this table. Each
// entry has the token name, the token id, the token type id, i.e. its type, and a value associated with the
// token. The value allows for a constant token. The parser can directly use the value in expressions.
//
//------------------------------------------------------------------------------------------------------------
const SimToken asmTokTab[ ] = {
    
    //--------------------------------------------------------------------------------------------------------
    // General registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "R0",             .typ = TYP_GREG,            .tid = GR_0,                .val = 0        },
    { .name = "R1",             .typ = TYP_GREG,            .tid = GR_1,                .val = 1        },
    { .name = "R2",             .typ = TYP_GREG,            .tid = GR_2,                .val = 2        },
    { .name = "R3",             .typ = TYP_GREG,            .tid = GR_3,                .val = 3        },
    { .name = "R4",             .typ = TYP_GREG,            .tid = GR_4,                .val = 4        },
    { .name = "R5",             .typ = TYP_GREG,            .tid = GR_5,                .val = 5        },
    { .name = "R6",             .typ = TYP_GREG,            .tid = GR_6,                .val = 6        },
    { .name = "R7",             .typ = TYP_GREG,            .tid = GR_7,                .val = 7        },
    { .name = "R8",             .typ = TYP_GREG,            .tid = GR_8,                .val = 8        },
    { .name = "R9",             .typ = TYP_GREG,            .tid = GR_9,                .val = 9        },
    { .name = "R10",            .typ = TYP_GREG,            .tid = GR_10,               .val = 10       },
    { .name = "R11",            .typ = TYP_GREG,            .tid = GR_11,               .val = 11       },
    { .name = "R12",            .typ = TYP_GREG,            .tid = GR_12,               .val = 12       },
    { .name = "R13",            .typ = TYP_GREG,            .tid = GR_13,               .val = 13       },
    { .name = "R14",            .typ = TYP_GREG,            .tid = GR_14,               .val = 14       },
    { .name = "R15",            .typ = TYP_GREG,            .tid = GR_15,               .val = 15       },
    
    //--------------------------------------------------------------------------------------------------------
    // Control registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "C0",             .typ = TYP_CREG,            .tid = CR_0,                .val = 0            },
    { .name = "C1",             .typ = TYP_CREG,            .tid = CR_1,                .val = 1            },
    { .name = "C2",             .typ = TYP_CREG,            .tid = CR_2,                .val = 2            },
    { .name = "C3",             .typ = TYP_CREG,            .tid = CR_3,                .val = 3            },
    { .name = "C4",             .typ = TYP_CREG,            .tid = CR_4,                .val = 4            },
    { .name = "C5",             .typ = TYP_CREG,            .tid = CR_5,                .val = 5            },
    { .name = "C6",             .typ = TYP_CREG,            .tid = CR_6,                .val = 6            },
    { .name = "C7",             .typ = TYP_CREG,            .tid = CR_7,                .val = 7            },
    { .name = "C8",             .typ = TYP_CREG,            .tid = CR_8,                .val = 8            },
    { .name = "C9",             .typ = TYP_CREG,            .tid = CR_9,                .val = 9            },
    { .name = "C10",            .typ = TYP_CREG,            .tid = CR_10,               .val = 10           },
    { .name = "C11",            .typ = TYP_CREG,            .tid = CR_11,               .val = 11           },
    { .name = "C12",            .typ = TYP_CREG,            .tid = CR_12,               .val = 12           },
    { .name = "C13",            .typ = TYP_CREG,            .tid = CR_13,               .val = 13           },
    { .name = "C14",            .typ = TYP_CREG,            .tid = CR_14,               .val = 14           },
    { .name = "C15",            .typ = TYP_CREG,            .tid = CR_15,               .val = 15           },
    { .name = "C16",            .typ = TYP_CREG,            .tid = CR_16,               .val = 16           },

    //--------------------------------------------------------------------------------------------------------
    // Runtime architcture register names for general registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "T0",             .typ = TYP_GREG,            .tid = GR_1,                .val =  1       },
    { .name = "T1",             .typ = TYP_GREG,            .tid = GR_2,                .val =  2       },
    { .name = "T2",             .typ = TYP_GREG,            .tid = GR_3,                .val =  3       },
    { .name = "T3",             .typ = TYP_GREG,            .tid = GR_4,                .val =  4       },
    { .name = "T4",             .typ = TYP_GREG,            .tid = GR_5,                .val =  5       },
    { .name = "T5",             .typ = TYP_GREG,            .tid = GR_6,                .val =  6       },
    { .name = "T6",             .typ = TYP_GREG,            .tid = GR_7,                .val =  7       },

    { .name = "ARG3",           .typ = TYP_GREG,            .tid = GR_8,                .val =  8       },
    { .name = "ARG2",           .typ = TYP_GREG,            .tid = GR_9,                .val =  9       },
    { .name = "ARG1",           .typ = TYP_GREG,            .tid = GR_10,               .val =  10      },
    { .name = "ARG0",           .typ = TYP_GREG,            .tid = GR_11,               .val =  11      },

    { .name = "RET3",           .typ = TYP_GREG,            .tid = GR_8,                .val =  8       },
    { .name = "RET2",           .typ = TYP_GREG,            .tid = GR_9,                .val =  9       },
    { .name = "RET1",           .typ = TYP_GREG,            .tid = GR_10,               .val =  10      },
    { .name = "RET0",           .typ = TYP_GREG,            .tid = GR_11,               .val =  11      },
    
    { .name = "DP",             .typ = TYP_GREG,            .tid = GR_13,               .val =  13      },
    { .name = "RL",             .typ = TYP_GREG,            .tid = GR_14,               .val =  14      },
    { .name = "SP",             .typ = TYP_GREG,            .tid = GR_15,               .val =  15      },
    
    //--------------------------------------------------------------------------------------------------------
    // Assembler mnemonics.
    //
    // ??? fill in opCode only ?
    //--------------------------------------------------------------------------------------------------------
    { .name = "LD",             .typ = TYP_OP_CODE,         .tid = OP_CODE_LD,          .val = 0x00000000   },
    { .name = "LDR",            .typ = TYP_OP_CODE,         .tid = OP_CODE_LDR,         .val = 0x00000000   },
    
    { .name = "ST",             .typ = TYP_OP_CODE,         .tid = OP_CODE_ST,          .val = 0x00000000   },
    { .name = "STC",            .typ = TYP_OP_CODE,         .tid = OP_CODE_STC,         .val = 0x00000000   },
    { .name = "ADD",            .typ = TYP_OP_CODE,         .tid = OP_CODE_ADD,         .val = 0x00000000   },
    { .name = "SUB",            .typ = TYP_OP_CODE,         .tid = OP_CODE_SUB,         .val = 0x00000000   },
    { .name = "AND",            .typ = TYP_OP_CODE,         .tid = OP_CODE_AND,         .val = 0x00000000   },
    { .name = "OR" ,            .typ = TYP_OP_CODE,         .tid = OP_CODE_OR,          .val = 0x00000000   },
    { .name = "XOR" ,           .typ = TYP_OP_CODE,         .tid = OP_CODE_XOR,         .val = 0x00000000   },
    { .name = "CMP" ,           .typ = TYP_OP_CODE,         .tid = OP_CODE_CMP,         .val = 0x00000000   },
    { .name = "EXTR",           .typ = TYP_OP_CODE,         .tid = OP_CODE_EXTR,        .val = 0x00000000   },
    { .name = "DEP",            .typ = TYP_OP_CODE,         .tid = OP_CODE_DEP,         .val = 0x00000000   },
    { .name = "DSR",            .typ = TYP_OP_CODE,         .tid = OP_CODE_DSR,         .val = 0x00000000   },
    { .name = "SHLA",           .typ = TYP_OP_CODE,         .tid = OP_CODE_SHLA,        .val = 0x00000000   },
    
    { .name = "LDIL",           .typ = TYP_OP_CODE,         .tid = OP_CODE_LDIL,        .val = 0x00000000   },
    { .name = "ADDIL",          .typ = TYP_OP_CODE,         .tid = OP_CODE_ADDIL,       .val = 0x00000000   },
    { .name = "LDO",            .typ = TYP_OP_CODE,         .tid = OP_CODE_LDO,         .val = 0x00000000   },
    
    { .name = "B",              .typ = TYP_OP_CODE,         .tid = OP_CODE_B,           .val = 0x00000000   },
    { .name = "GATE",           .typ = TYP_OP_CODE,         .tid = OP_CODE_GATE,        .val = 0x00000000   },
    { .name = "BR",             .typ = TYP_OP_CODE,         .tid = OP_CODE_BR,          .val = 0x00000000   },
    { .name = "BV",             .typ = TYP_OP_CODE,         .tid = OP_CODE_BV,          .val = 0x00000000   },
   
    { .name = "CBR",            .typ = TYP_OP_CODE,         .tid = OP_CODE_CBR,         .val = 0x00000000   },
    { .name = "TBR",            .typ = TYP_OP_CODE,         .tid = OP_CODE_TBR,         .val = 0x00000000   },
    { .name = "MBR",            .typ = TYP_OP_CODE,         .tid = OP_CODE_MBR,         .val = 0x00000000   },
    
    { .name = "MR",             .typ = TYP_OP_CODE,         .tid = OP_CODE_MR,          .val = 0x00000000   },
    { .name = "MST",            .typ = TYP_OP_CODE,         .tid = OP_CODE_MST,         .val = 0x00000000   },
    { .name = "DS",             .typ = TYP_OP_CODE,         .tid = OP_CODE_DS,          .val = 0x00000000   },
    { .name = "LDPA",           .typ = TYP_OP_CODE,         .tid = OP_CODE_LDPA,        .val = 0x00000000   },
    { .name = "PRB",            .typ = TYP_OP_CODE,         .tid = OP_CODE_PRB,         .val = 0x00000000   },
    { .name = "ITLB",           .typ = TYP_OP_CODE,         .tid = OP_CODE_ITLB,        .val = 0x00000000   },
    { .name = "PTLB",           .typ = TYP_OP_CODE,         .tid = OP_CODE_PTLB,        .val = 0x00000000   },
    { .name = "PCA",            .typ = TYP_OP_CODE,         .tid = OP_CODE_PCA,         .val = 0x00000000   },
    { .name = "DIAG",           .typ = TYP_OP_CODE,         .tid = OP_CODE_DIAG,        .val = 0x00000000   },
    { .name = "RFI",            .typ = TYP_OP_CODE,         .tid = OP_CODE_RFI,         .val = 0x00000000   },
    { .name = "BRK",            .typ = TYP_OP_CODE,         .tid = OP_CODE_BRK,         .val = 0x00000000   },

    //--------------------------------------------------------------------------------------------------------
    // Synthetic instruction mnemonics.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "NOP",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_NOP,       .val =  0           },
    { .name = "SHL",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_SHL,       .val =  0           },
    { .name = "SHR",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_SHR,       .val =  0           },
    { .name = "ASL",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_ASL,       .val =  0           },
    { .name = "ASR",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_ASR,       .val =  0           },
    { .name = "ROR",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_ROR,       .val =  0           },
    { .name = "ROL",            .typ = TYP_OP_CODE_S,       .tid = OP_CODE_S_ROL,       .val =  0           }
     
};

const int MAX_ASM_TOKEN_TAB = sizeof( asmTokTab ) / sizeof( SimToken );


//------------------------------------------------------------------------------------------------------------
// Token flags. They are used to communicate additional information about the the token to the assembly
// process. Examples are the data width encoded in the opCode and the instruction mask.
//
//------------------------------------------------------------------------------------------------------------
enum TokenFlags : uint32_t {
  
    TF_NIL              = 0,
    TF_BYTE_INSTR       = ( 1U << 0 ),
    TF_HALF_INSTR       = ( 1U << 1 ),
    TF_WORD_INSTR       = ( 1U << 2 )
};

//------------------------------------------------------------------------------------------------------------
// Expression value. The analysis of an expression results in a value. Depending on the expression type, the
// values are simple scalar values or a structured value, such as a register pair or virtual address.
//
//------------------------------------------------------------------------------------------------------------
struct SimExpr {
    
    SimTokTypeId typ;
   
    union {
        
        struct {    SimTokId    tokId;                      };
        struct {    bool        bVal;                       };
        struct {    int64_t     numVal;                     };
        struct {    char        strVal[ TOK_STR_SIZE ];     };
        struct {    uint32_t    adr;                        };
        struct {    uint8_t     sReg;  uint8_t gReg;        };
        struct {    uint32_t    seg;   uint32_t ofs;        };
    };
};


//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
int         currentLineLen                      = 0;
int         currentCharIndex                    = 0;
int         currentTokCharIndex                 = 0;
char        currentChar                         = ' ';
SimToken    currentToken;
char        tokenLine[ TOK_INPUT_LINE_SIZE ]    = { 0 };

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
static inline bool isAligned( int64_t adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline bool isInRange( int64_t adr, int64_t low, int64_t high ) {
    
    return(( adr >= low ) && ( adr <= high ));
}


bool isInRangeForBitField( int32_t val, uint8_t bitLen ) {
    
    int min = - ( 1 << (( bitLen - 1 ) % 32 ));
    int max = ( 1 << (( bitLen - 1 ) % 32 )) - 1;
    return(( val <= max ) && ( val >= min ));
}

bool isInRangeForBitFieldU( uint32_t val, uint8_t bitLen ) {
    
    int max = (( 1 << ( bitLen % 32 )) - 1 );
    return( val <= max );
}

static inline int64_t roundup( uint64_t arg ) {
    
    return( arg ); // for now ...
}

static inline int64_t extractBit( int64_t arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline int64_t extractField( int64_t arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline int64_t extractSignedField( int64_t arg, int bitpos, int len ) {
    
    int64_t field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
}

static inline int64_t depositField( int64_t word, int bitpos, int len, int64_t value) {
    
    int64_t mask = (( 1ULL << len ) - 1 ) << bitpos;
    return ( word & ~mask ) | (( value << bitpos ) & mask );
}

bool willAddOverflow( int64_t a, int64_t b ) {
    
    if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
    if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
    return false;
}

bool willSubOverflow( int64_t a, int64_t b ) {
    
    if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
    if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
    return false;
}

bool willShiftLftOverflow( int64_t a, int shift ) {
    
    if (( shift < 0 ) || ( shift >= 64 )) return true;
    if ( a == 0 ) return false;
    
    int64_t max = INT64_MAX >> shift;
    int64_t min = INT64_MIN >> shift;
    
    return (( a > max ) || ( a < min ));
}

void upshiftStr( char *str ) {
    
    size_t len = strlen( str );
    
    if ( len > 0 ) {
        
        for ( size_t i = 0; i < len; i++ ) str[ i ] = (char) toupper((int) str[ i ] );
    }
}

void addChar( char *buf, int size, char ch ) {
    
    size_t len = strlen( buf );
    
    if ( len + 1 < size ) {
        
        buf[ len ]     = ch;
        buf[ len + 1 ] = 0;
    }
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
void format_hex64( int64_t value, char *buf, int digits = 16 ) {
    
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
void format_dec64( int64_t value, char *buf ) {
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
// Parses a number in hex (with 0x prefix) or decimal (no prefix), allowing '_' separators.
// Returns 1 on success, 0 on error.
//
//------------------------------------------------------------------------------------------------------------
int parse_int64( const char *str, int64_t *out) {
    
    *out = 0;
    int base    = 10;
    int digits  = 0;
    
    if ( str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        
        base = 16;
        str += 2;
    }
    
    while ( *str ) {
        
        if ( *str == '_' ) {
            
            str++;
            continue;
        }
        
        int value = -1;
        
        if      ( isdigit((char)*str))                                  value = *str - '0';
        else if (( base == 16 ) && ( *str >= 'a' ) && ( *str <= 'f' ))  value = *str - 'a' + 10;
        else if (( base == 16 ) && ( *str >= 'A' ) && ( *str <= 'F' ))  value = *str - 'A' + 10;
        else return 0; // Invalid character
        
        if ( value >= base )    return 0;
        if ( digits >= 20  )    return 0; // Prevent overflow (conservative)
        
        *out = *out * base + value;
        
        digits++;
        str++;
    }
    
    return ( digits > 0 );
}

//------------------------------------------------------------------------------------------------------------
// The lookup function. We just do a linear search for now. Note that we expect the last entry in the token
// table to be the NIL token, otherwise bad things will happen.
//
//------------------------------------------------------------------------------------------------------------
int lookupToken( char *inputStr, const SimToken *tokTab ) {
    
    if (( strlen( inputStr ) == 0 ) || ( strlen ( inputStr ) > MAX_TOKEN_NAME_SIZE )) return( -1 );
   
    for ( int i = 0; i < MAX_ASM_TOKEN_TAB; i++  ) {
        
        if ( strcmp( inputStr, tokTab[ i ].name ) == 0 ) return( i );
    }
    
    return( -1 );
}

//------------------------------------------------------------------------------------------------------------
// "nextChar" returns the next character from the token line string.
//
//------------------------------------------------------------------------------------------------------------
void nextChar( ) {
    
    if ( currentCharIndex < currentLineLen ) {
        
        currentChar = tokenLine[ currentCharIndex ];
        currentCharIndex ++;
    }
    else currentChar = EOS_CHAR;
}


//------------------------------------------------------------------------------------------------------------
// "parseNum" will parse a number. We leave the heavy lifting of converting the numeric value to the C
// library.
//
//------------------------------------------------------------------------------------------------------------
void parseNum( ) {
    
    char tmpStr[ TOK_INPUT_LINE_SIZE ]  = "";
    
    currentToken.tid = TOK_NUM;
    currentToken.typ = TYP_NUM;
    currentToken.val = 0;
    
#if 0
    
    do {
        
        addChar( tmpStr, sizeof( tmpStr ), currentChar );
        nextChar( );
        
    } while ( isxdigit( currentChar )   || ( currentChar == 'X' ) || ( currentChar == 'O' )
                                        || ( currentChar == 'x' ) || ( currentChar == 'o' ));
    
    if ( sscanf( tmpStr, "%i", &currentToken.val ) != 1 ) throw ( ERR_INVALID_NUM );
      
    if ( currentChar == '.' ) {
        
        nextChar( );
        if ( ! isdigit( currentChar )) throw ( ERR_EXPECTED_EXT_ADR );
           
        currentToken.seg    = currentToken.val;
        currentToken.typ    = TYP_EXT_ADR;
        tmpStr[ 0 ]         = '\0';
        
        do {
            
            addChar( tmpStr, sizeof( tmpStr ), currentChar );
            nextChar( );
            
        } while ( isxdigit( currentChar )   || ( currentChar == 'X' ) || ( currentChar == 'O' )
                                            || ( currentChar == 'x' ) || ( currentChar == 'o' ));
        
        if ( sscanf( tmpStr, "%i", &currentToken.ofs ) != 1 ) throw ( ERR_INVALID_NUM );
    }
#endif
}

//------------------------------------------------------------------------------------------------------------
// "parseString" gets a string. We manage special characters inside the string with the "\" prefix. Right
// now, we do not use strings, so the function is perhaps for the future. We will just parse it, but record
// no result. One day, the entire simulator might use the lexer functions. Then we need it.
//
//------------------------------------------------------------------------------------------------------------
void parseString( ) {
    
    currentToken.tid        = TOK_STR;
    currentToken.typ        = TYP_STR;
    currentToken.str[ 0 ]   = '\0';

    nextChar( );
    while (( currentChar != EOS_CHAR ) && ( currentChar != '"' )) {

        if ( currentChar == '\\' ) {
            
            nextChar( );
            if ( currentChar != EOS_CHAR ) {
                
                if      ( currentChar == 'n' )  strcat( currentToken.str, (char *) "\n" );
                else if ( currentChar == 't' )  strcat( currentToken.str, (char *) "\t" );
                else if ( currentChar == '\\' ) strcat( currentToken.str, (char *) "\\" );
                else addChar( currentToken.str, sizeof( currentToken.str ), currentChar );
            }
            else throw ( ERR_EXPECTED_CLOSING_QUOTE );
        }
        else addChar( currentToken.str, sizeof( currentToken.str ), currentChar );
        
        nextChar( );
    }

    nextChar( );
}

//------------------------------------------------------------------------------------------------------------
// "parseIdent" parses an identifier. It is a sequence of characters starting with an alpha character. An
// identifier found in the token table will assume the type and value of the token found. Any other identifier
// is just an identifier symbol. There is one more thing. There are qualified constants that begin with a
// character followed by a percent character, followed by the value. During the character analysis, We first
// check for these kind of qualifiers and if found hand over to parse a number.
//
//------------------------------------------------------------------------------------------------------------
void parseIdent( ) {
    
    currentToken.tid        = TOK_IDENT;
    currentToken.typ        = TYP_IDENT;
    currentToken.str[ 0 ]   = '\0';
    
    char identBuf[ TOK_INPUT_LINE_SIZE ] = "";
    
    if (( currentChar == 'L' ) || ( currentChar == 'l' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
        
        if ( currentChar == '%' ) {
            
            addChar( identBuf, sizeof( identBuf ), currentChar );
            nextChar( );
            
            if ( isdigit( currentChar )) {
                
                parseNum( );
                currentToken.val &= 0xFFFFFC00;
                return;
            }
            else {
             
                printf( "invalid ch: %d\n", currentChar );
                throw ( ERR_INVALID_CHAR_IN_IDENT );
                
            }
        }
    }
    else if (( currentChar == 'R' ) || ( currentChar == 'r' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
        
        if ( currentChar == '%' ) {
            
            addChar( identBuf, sizeof( identBuf ), currentChar );
            nextChar( );
            
            if ( isdigit( currentChar )) {
                
                parseNum( );
                currentToken.val &= 0x3FF;
                return;
            }
            else {
                
                printf( "invalid ch: %d\n", currentChar );
                throw ( ERR_INVALID_CHAR_IN_IDENT );
            }
        }
    }
    
    while (( isalnum( currentChar )) || ( currentChar == '_' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
    }
    
    upshiftStr( identBuf );
    
    int index = lookupToken( identBuf, asmTokTab );
    
    if ( index == -1 ) {
        
        currentToken.typ = TYP_IDENT;
        currentToken.tid = TOK_IDENT;
        strcpy( currentToken.str, identBuf );
    }
    else currentToken = asmTokTab[ index ];
}

//------------------------------------------------------------------------------------------------------------
// "nextToken" is the entry point to the token business. It returns the next token from the input string.
//
//------------------------------------------------------------------------------------------------------------
void nextToken( ) {

    currentToken.typ       = TYP_NIL;
    currentToken.tid       = TOK_NIL;
    
    while (( currentChar == ' ' ) || ( currentChar == '\n' ) || ( currentChar == '\n' )) nextChar( );
    
    currentTokCharIndex = currentCharIndex - 1;
    
    if ( isalpha( currentChar )) {
        
       parseIdent( );
    }
    else if ( isdigit( currentChar )) {
        
       parseNum( );
    }
    else if ( currentChar == '"' ) {
        
        parseString( );
    }
    else if ( currentChar == '.' ) {
        
        currentToken.typ   = TYP_SYM;
        currentToken.tid   = TOK_PERIOD;
        nextChar( );
    }
    else if ( currentChar == '+' ) {
        
        currentToken.tid = TOK_PLUS;
        nextChar( );
    }
    else if ( currentChar == '-' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_MINUS;
        nextChar( );
    }
    else if ( currentChar == '*' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_MULT;
        nextChar( );
    }
    else if ( currentChar == '/' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_DIV;
        nextChar( );
    }
    else if ( currentChar == '%' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_MOD;
        nextChar( );
    }
    else if ( currentChar == '&' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_AND;
        nextChar( );
    }
    else if ( currentChar == '|' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_OR;
        nextChar( );
    }
    else if ( currentChar == '^' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_XOR;
        nextChar( );
    }
    else if ( currentChar == '~' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_NEG;
        nextChar( );
    }
    else if ( currentChar == '(' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_LPAREN;
        nextChar( );
    }
    else if ( currentChar == ')' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_RPAREN;
        nextChar( );
    }
    else if ( currentChar == ',' ) {
        
        currentToken.typ    = TYP_SYM;
        currentToken.tid    = TOK_COMMA;
        nextChar( );
    }
    else if ( currentChar == EOS_CHAR ) {
        
        currentToken.typ    = TYP_NIL;
        currentToken.tid    = TOK_EOS;
    }
    else {
    
        currentToken.tid = TOK_ERR;
        {
         
            printf( "invalid ch: %d\n", currentChar );
            throw ( ERR_INVALID_CHAR_IN_IDENT );
            
        }
    }
}

//------------------------------------------------------------------------------------------------------------
// Check that the ASM line does not contain any extra tokens when the parser completed the analysis of the
// assembly line.
//
//------------------------------------------------------------------------------------------------------------
void checkEOS( ) {
    
    if ( currentToken.tid != TOK_EOS ) throw( ERR_EXTRA_TOKEN_IN_STR );
}

void acceptComma( ) {
    
    if ( currentToken.tid == TOK_COMMA ) nextToken( );
    else throw( ERR_EXPECTED_COMMA );
}

void acceptLparen( ) {
    
    if ( currentToken.tid == TOK_LPAREN ) nextToken( );
    else throw( ERR_EXPECTED_LPAREN );
}

void acceptRparen( ) {
    
    if ( currentToken.tid == TOK_RPAREN ) nextToken( );
    else throw( ERR_EXPECTED_RPAREN );
}

bool isToken( SimTokId tid ) {
    
    return( currentToken.tid == tid );
}

bool isTokenTyp( SimTokTypeId typ ) {
    
    return( currentToken.typ = typ );
}

//------------------------------------------------------------------------------------------------------------
// "parseExpr" needs to be declared forward.
//
//------------------------------------------------------------------------------------------------------------
void parseExpr( SimExpr *rExpr );

//------------------------------------------------------------------------------------------------------------
// "parseFactor" parses the factor syntax part of an expression.
//
//      <factor> -> <number>            |
//                  <gregId>            |
//                  <cregId>            |
//                  "~" <factor>        |
//                  "(" <greg> ")"      |
//                  "(" <expr> ")"
//
//------------------------------------------------------------------------------------------------------------
void parseFactor( SimExpr *rExpr ) {
    
    rExpr -> typ  = TYP_NIL;
    rExpr -> numVal = 0;

    if ( isToken( TOK_NUM )) {
        
        rExpr -> typ    = TYP_NUM;
        rExpr -> numVal = currentToken.val;
        nextToken( );
    }
    else if ( isTokenTyp( TYP_GREG )) {
        
        rExpr -> typ    = TYP_GREG;
        rExpr -> numVal = currentToken.val;
        nextToken( );
    }
    else if ( isTokenTyp( TYP_CREG )) {
        
        rExpr -> typ    = TYP_CREG;
        rExpr -> numVal = currentToken.val;
        nextToken( );
    }
    else if ( isToken( TOK_NEG )) {
        
        parseFactor( rExpr );
        rExpr -> numVal = ~ rExpr -> numVal;
    }
    else if ( isToken( TOK_LPAREN )) {
        
        nextToken( );
        if ( isTokenTyp( TYP_GREG )) {
            
            rExpr -> typ    = TYP_ADR;
            rExpr -> numVal = currentToken.val;
            nextToken( );
        }
        else parseExpr( rExpr );
        
        acceptRparen( );
    }
    else throw ( ERR_INVALID_EXPR );
}

//------------------------------------------------------------------------------------------------------------
// "parseTerm" parses the term syntax.
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
//------------------------------------------------------------------------------------------------------------
void parseTerm( SimExpr *rExpr ) {
    
    SimExpr lExpr;
    
    parseFactor( rExpr );
    
    while (( isToken( TOK_MULT ))   ||
           ( isToken( TOK_DIV  ))   ||
           ( isToken( TOK_MOD  ))   ||
           ( isToken( TOK_AND  )))  {
        
        uint8_t op = currentToken.tid;
        
        nextToken( );
        parseFactor( &lExpr );
        
        if ( rExpr -> typ != lExpr.typ ) throw ( ERR_EXPR_TYPE_MATCH );
       
        switch( op ) {
                
            case TOK_MULT:   rExpr -> numVal = rExpr -> numVal * lExpr.numVal; break;
            case TOK_DIV:    rExpr -> numVal = rExpr -> numVal / lExpr.numVal; break;
            case TOK_MOD:    rExpr -> numVal = rExpr -> numVal % lExpr.numVal; break;
            case TOK_AND:    rExpr -> numVal = rExpr -> numVal & lExpr.numVal; break;
        }
    }
}

//------------------------------------------------------------------------------------------------------------
// "parseExpr" parses the expression syntax. The one line assembler parser routines use this call in many
// places where a numeric expression or an address is needed.
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
//------------------------------------------------------------------------------------------------------------
void parseExpr( SimExpr *rExpr ) {
    
    SimExpr lExpr;
    
    if ( isToken( TOK_PLUS )) {
        
        nextToken( );
        parseTerm( rExpr );
        
        if ( ! ( rExpr -> typ == TYP_NUM )) throw ( ERR_EXPECTED_NUMERIC );
    }
    else if ( isToken( TOK_MINUS )) {
        
        nextToken( );
        parseTerm( rExpr );
        
        if ( rExpr -> typ == TYP_NUM ) rExpr -> numVal = - rExpr -> numVal;
        else throw( ERR_EXPECTED_NUMERIC );
    }
    else parseTerm( rExpr );
    
    while (( isToken( TOK_PLUS  )) ||
           ( isToken( TOK_MINUS )) ||
           ( isToken( TOK_OR    )) ||
           ( isToken( TOK_XOR   ))) {
        
        uint8_t op = currentToken.tid;
        
        nextToken( );
        parseTerm( &lExpr );
        
        if ( rExpr -> typ != lExpr.typ ) throw ( ERR_EXPR_TYPE_MATCH );
        
        switch ( op ) {
                
            case TOK_PLUS:   rExpr -> numVal = rExpr -> numVal + lExpr.numVal; break;
            case TOK_MINUS:  rExpr -> numVal = rExpr -> numVal - lExpr.numVal; break;
            case TOK_OR:     rExpr -> numVal = rExpr -> numVal | lExpr.numVal; break;
            case TOK_XOR:    rExpr -> numVal = rExpr -> numVal ^ lExpr.numVal; break;
        }
    }
}

#if 0


//------------------------------------------------------------------------------------------------------------
// "parseInstrOptions" will analyze the opCode option string. An opCode string is a sequence of characters.
// We will look at each character in the "name" and set the options for the particular instruction. There are
// also cases where the only option is a multi-character sequence. We detect invalid options but not when
// the same option is repeated. E.g. a "LOL" will result in "L" and "O" set.
//
//------------------------------------------------------------------------------------------------------------
void parseInstrOptions( uint32_t *instr, uint32_t *flags ) {
    
    if ( ! tok -> isToken( TOK_IDENT )) throw ( ERR_EXPECTED_INSTR_OPT );
    
    char *optBuf = tok -> tokStr( );
    
    switch( getBitField( *instr, 5, 6 )) {
            
        case OP_LD:
        case OP_ST:
        case OP_LDA:
        case OP_STA:  {
            
            if ( optBuf[ 0 ] == 'M' ) setBit( instr, 11 );
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_ADD:
        case OP_ADC:
        case OP_SUB:
        case OP_SBC: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'L' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'O' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_AND:
        case OP_OR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'N' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'C' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_XOR: {
            
            if ( optBuf[ 0 ] == 'N' ) setBit( instr, 10 );
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_CMP:
        case OP_CMPU: {
            
            if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) setBitField( instr, 11, 2, 0 );
            else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) setBitField( instr, 11, 2, 1 );
            else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) setBitField( instr, 11, 2, 2 );
            else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) setBitField( instr, 11, 2, 3 );
            else throw ( ERR_INVALID_INSTR_OPT );

        } break;
            
        case OP_CBR:
        case OP_CBRU: {
            
            if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) setBitField( instr, 7, 2, 0 );
            else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) setBitField( instr, 7, 2, 1 );
            else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) setBitField( instr, 7, 2, 2 );
            else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) setBitField( instr, 7, 2, 3 );
            else throw ( ERR_INVALID_INSTR_OPT );

        } break;
            
        case OP_CMR: {
            
            if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) setBitField( instr, 13, 4, 0 );
            else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) setBitField( instr, 13, 4, 1 );
            else if ( strcmp( optBuf, ((char *) "GT" )) == 0 ) setBitField( instr, 13, 4, 2 );
            else if ( strcmp( optBuf, ((char *) "EV" )) == 0 ) setBitField( instr, 13, 4, 3 );
            else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) setBitField( instr, 13, 4, 4 );
            else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) setBitField( instr, 13, 4, 5 );
            else if ( strcmp( optBuf, ((char *) "GE" )) == 0 ) setBitField( instr, 13, 4, 6 );
            else if ( strcmp( optBuf, ((char *) "OD" )) == 0 ) setBitField( instr, 13, 4, 7 );
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_EXTR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'S' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'A' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_DEP: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'Z' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'A' ) setBit( instr, 11 );
                else if ( optBuf[ i ] == 'I' ) setBit( instr, 12 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_DSR: {
            
            if ( optBuf[ 0 ] == 'A' ) setBit( instr, 11 );
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_SHLA: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'I' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'L' ) setBit( instr, 11 );
                else if ( optBuf[ i ] == 'O' ) setBit( instr, 12 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_MR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'D' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'M' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_MST: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'S' ) setBitField( instr, 11, 2, 1 );
                else if ( optBuf[ i ] == 'C' ) setBitField( instr, 11, 2, 2 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
    
        } break;
            
        case OP_PRB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'W' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'I' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_ITLB: {
            
            if ( optBuf[ 0 ] == 'T' ) setBit( instr, 11 );
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_PTLB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'T' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'M' ) setBit( instr, 11 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_PCA: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'T' ) setBit( instr, 10 );
                else if ( optBuf[ i ] == 'M' ) setBit( instr, 11 );
                else if ( optBuf[ i ] == 'F' ) setBit( instr, 14 );
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        default: throw ( ERR_INSTR_HAS_NO_OPT );
    }
    
    tok -> nextToken( );
}

//------------------------------------------------------------------------------------------------------------
// "parseLogicalAdr" analyzes a logical address, which is used by several instruction with a "seg" field.
//
//      "(" [ <segReg> "," ] <ofsReg> ")"
//
//------------------------------------------------------------------------------------------------------------
void parseLogicalAdr( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
   
    parseExpr( &rExpr );
   
    if ( rExpr.typ == TYP_EXT_ADR ) {
        
        setBitField( instr, 31, 4, rExpr.gReg );
        
        if ( isInRange( rExpr.sReg, 1, 3 )) setBitField( instr, 13, 2, rExpr.sReg );
        else throw ( ERR_EXPECTED_SR1_SR3 );
    }
    else if ( rExpr.typ == TYP_ADR ) {
        
        setBitField( instr, 31, 4, rExpr.adr );
    }
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
}

//------------------------------------------------------------------------------------------------------------
// "parseLoadStoreOperand" parses the operand portion of the load and store instruction family. It represents
// the source location for the load type instruction and the target for the store type instruction. The syntax
// for the <operand> portion is either a
//
//      <ofs> "(" SR "," GR ")"
//      <ofs> "(" GR ")"
//      <GR>  "(" SR "," GR ")"
//      <GR>  "(" GR ")"
//
// <loadInstr>  [ "." <opt> ] <targetReg>       "," <sourceOperand>
// <storeInstr> [ "." <opt> ] <targetOperand>   "," <sourceReg>
//
//------------------------------------------------------------------------------------------------------------
void parseLoadStoreOperand( uint32_t *instr, uint32_t flags ) {
    
    SimExpr  rExpr;
    
    if      ( flags & TF_BYTE_INSTR ) setBitField( instr, 15, 2, 0 );
    else if ( flags & TF_HALF_INSTR ) setBitField( instr, 15, 2, 1 );
    else if ( flags & TF_WORD_INSTR ) setBitField( instr, 15, 2, 2 );
  
    parseExpr( &rExpr );
   
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitField( rExpr.numVal, 12 )) setBitField( instr, 27, 12, rExpr.numVal );
        else throw ( ERR_IMM_VAL_RANGE );
            
        parseExpr( &rExpr );
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        if (( getBitField( *instr, 5, 6 ) == OP_LDR ) || ( getBitField( *instr, 5, 6 ) == OP_STC ))
            throw ( ERR_INVALID_INSTR_MODE );
            
        setBit( instr, 10 );
        setBitField( instr, 27, 4, rExpr.numVal );
        
        parseExpr( &rExpr );
    }
    
    if ( rExpr.typ == TYP_ADR ) {
                    
        setBitField( instr, 13, 2, 0 );
        setBitField( instr, 31, 4, rExpr.numVal );
    }
    else if ( rExpr.typ == TYP_EXT_ADR ) {
                    
        if (( getBitField( *instr, 5, 6 ) == OP_LDA ) || ( getBitField( *instr, 5, 6 ) == OP_STA )) {
            
            throw ( ERR_INVALID_INSTR_MODE );
        }
                
        if ( isInRange( rExpr.sReg, 1, 3 )) setBitField( instr, 13, 2, rExpr.sReg );
        else throw( ERR_EXPECTED_SR1_SR3 );
        
        setBitField( instr, 31, 4, rExpr.gReg );
    }
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
}

//------------------------------------------------------------------------------------------------------------
// "parseModeTypeInstr" parses all instructions that have an "operand" encoding. The syntax is as follows:
//
//      opCode [ "." <opt> ] <targetReg> "," <num>                                - mode 0
//      opCode [ "." <opt> ] <targetReg> "," <num> "(" <baseReg> ")"              - mode 3
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg>                          - mode 1
//      opCode [ "." <opt> ] <targetReg> "," <sourceRegA> "," "<sourceRegB>       - mode 1
//      opCode [ "." <opt> ] <targetReg> "," <indexReg> "(" <baseReg> ")"         - mode 2
//
//------------------------------------------------------------------------------------------------------------
void parseModeTypeInstr( uint32_t *instr, uint32_t flags ) {
    
    uint8_t     targetRegId = 0;
    SimExpr     rExpr;
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        targetRegId = tok -> tokVal( );
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
     
        if ( tok -> isToken( TOK_EOS )) {
            
            if ( isInRangeForBitField( rExpr.numVal, 18 )) setBitField( instr, 31, 18, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else {
            
            if ( isInRangeForBitField( rExpr.numVal, 12 )) setBitField( instr, 27, 12, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
            
            parseExpr( &rExpr );
        
            if ( rExpr.typ == TYP_ADR ) {
                
                setBitField( instr, 13, 2, 3 );
                setBitField( instr, 31, 4, rExpr.numVal );
            }
            else throw ( ERR_EXPECTED_LOGICAL_ADR );
               
            if      ( flags & TF_BYTE_INSTR ) setBitField( instr, 15, 2, 0 );
            else if ( flags & TF_HALF_INSTR ) setBitField( instr, 15, 2, 1 );
            else if ( flags & TF_WORD_INSTR ) setBitField( instr, 15, 2, 2 );
        }
    }
    else if ( rExpr.typ == TYP_GREG ) {
    
        if ( tok -> isToken( TOK_EOS )) {
            
            setBitField( instr, 13, 2, 1 );
            setBitField( instr, 27, 4, targetRegId );
            setBitField( instr, 31, 4, rExpr.numVal );
        }
        else if ( tok -> isToken( TOK_COMMA )) {
            
            setBitField( instr, 13, 2, 1 );
            setBitField( instr, 27, 4, rExpr.numVal );
            
            tok -> nextToken( );
            if ( tok -> isTokenTyp( TYP_GREG )) {
                
                setBitField( instr, 13, 2, 1 );
                setBitField( instr, 27, 4, rExpr.numVal );
                setBitField( instr, 31, 4, tok -> tokVal( ));
                tok -> nextToken( );
            }
            else throw ( ERR_EXPECTED_GENERAL_REG );
        }
        else if ( tok -> isToken( TOK_LPAREN )) {
            
            setBitField( instr, 27, 4, rExpr.numVal );
            
            parseExpr( &rExpr );
            if ( rExpr.typ == TYP_ADR ) {
                
                setBitField( instr, 13, 2, 2 );
                setBitField( instr, 31, 4, rExpr.numVal );
            }
            else throw ( ERR_EXPECTED_LOGICAL_ADR );
            
            if      ( flags & TF_BYTE_INSTR ) setBitField( instr, 15, 2, 0 );
            else if ( flags & TF_HALF_INSTR ) setBitField( instr, 15, 2, 1 );
            else if ( flags & TF_WORD_INSTR ) setBitField( instr, 15, 2, 2 );
        }
    }
    else throw ( ERR_INVALID_INSTR_MODE );
    
    if (  getBitField( *instr, 13, 2 ) < 2 ) {
        
        if (( flags & TF_BYTE_INSTR ) || ( flags & TF_HALF_INSTR ))
            throw ( ERR_INSTR_MODE_OPT_COMBO );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrLSID" parses the LSID instruction.
//
//      <opCode> <targetReg> "," <sourceReg>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLSID( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_CLOSING_QUOTE );
    
    if ( tok -> isToken( TOK_COMMA )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_COMMA );
               
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrDEP" parses the deposit instruction. The instruction has three basic formats.
// When the "A" bit is set, the position will be obtained from the shift amount control register. Otherwise
// it is encoded in the instruction.
//
//      DEP [ ".“ <opt> ]       <targetReg> "," <sourceReg> "," <pos> "," <len>"
//      DEP [ "." "A" <opt> ]   <targetReg> "," <sourceReg> ", <len>"
//      DEP [ "." "I" <opt> ]   <targetReg> "," <val>, <pos> "," <len>
//      DEP [ "." "AI" <opt> ]  <targetReg> "," <val> "," <len>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrDEP( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );

    if ( rExpr.typ == TYP_GREG ) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        
        acceptComma( );
        parseExpr( &rExpr );
       
        if ( rExpr.typ == TYP_NUM ) {
            
            if ( isInRangeForBitFieldU( tok -> tokVal( ), 5 )) {
                
                if ( getBit( *instr, 11 ))  setBitField( instr, 21, 5, rExpr.numVal );
                else                        setBitField( instr, 27, 5, rExpr.numVal );
            }
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else throw ( ERR_EXPECTED_NUMERIC );
        
        if ( ! getBit( *instr, 11 )) {
            
            acceptComma( );
            parseExpr( &rExpr );

            
            if ( rExpr.typ == TYP_NUM ) {
                
                if ( isInRangeForBitFieldU( rExpr.numVal, 5 )) setBitField( instr, 21, 5, rExpr.numVal );
                else throw ( ERR_IMM_VAL_RANGE );
            }
            else throw ( ERR_EXPECTED_NUMERIC );
        }
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        if ( getBit( *instr, 12 )) {
            
            if ( isInRangeForBitField( rExpr.numVal, 4 )) setBitField( instr, 31, 4, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
            
            acceptComma( );
            
            if ( ! getBit( *instr, 11 )) {
               
                if ( isInRangeForBitFieldU( tok -> tokVal( ), 5 )) setBitField( instr, 27, 5, tok -> tokVal( ));
                else throw ( ERR_POS_VAL_RANGE );
                
                tok -> nextToken( );
                if ( tok -> isToken( TOK_COMMA )) tok -> nextToken( );
                else throw ( ERR_EXPECTED_COMMA );
            }
            
            parseExpr( &rExpr );
           
            if ( rExpr.typ == TYP_NUM ) {
                
                if ( isInRangeForBitFieldU( rExpr.numVal, 5 )) setBitField( instr, 21, 5, rExpr.numVal );
                else throw ( ERR_LEN_VAL_RANGE );
            }
            else throw ( ERR_EXPECTED_NUMERIC );
        }
        else throw ( ERR_EXPECTED_NUMERIC );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The DS instruction parses the divide step instruction.
//
//      DS <targetReg> "," <sourceRegA> "," <sourceRegB>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrDS( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The DSR instruction parses the double shift instruction. There are two flavors. If the "A" bit is set, the
// shift amount is taken from the shift amount control register, else from the instruction "len" field.
//
//      DSR [ ".“ <opt> ] <targetReg> "," <sourceRegA> "," <sourceRegB> "," <len"
//      DSR [ ".“ "A"   ] <targetReg> "," <sourceRegA> "," <sourceRegB>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrDSR( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    if ( ! getBit( *instr, 11 )) {
        
        acceptComma( );
        parseExpr( &rExpr );
        
        if ( rExpr.typ == TYP_NUM ) {
            
            if ( isInRangeForBitFieldU( rExpr.numVal, 5 )) setBitField( instr, 21, 5, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else throw ( ERR_EXPECTED_NUMERIC );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrEXTR" parses the extract instruction. The instruction has two basic formats. When the "A" bit
// is set, the position will be obtained from the shift amount control register. Otherwise it is encoded in
// the instruction.
//
//      EXTR [ ".“ <opt> ]      <targetReg> "," <sourceReg> "," <pos> "," <len"
//      EXTR "." "A" [ <opt> ]  <targetReg> "," <sourceReg> ", <len"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrEXTR( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitFieldU( tok -> tokVal( ), 5 )) {
            
            if ( getBit( *instr, 11 ))  setBitField( instr, 21, 5, rExpr.numVal );
            else                        setBitField( instr, 27, 5, rExpr.numVal );
        }
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    if ( ! getBit( *instr, 11 )) {
        
        acceptComma( );
        parseExpr( &rExpr );
       
        if ( rExpr.typ == TYP_NUM ) {
            
            if ( isInRangeForBitFieldU( rExpr.numVal, 5 )) {
                
                setBitField( instr, 21, 5, rExpr.numVal );
            }
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else throw ( ERR_EXPECTED_NUMERIC );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The SHLA instruction performs a shift left of "B" by "sa" and adds the "A" register to it.
//
//      SHLA [ "." <opt> ] <targetReg> "," <sourceRegA> "," <sourceRegB> "," <amt>
//      SHLA ".I" <targetReg> "," <sourceRegA> "," <val> "," <amt>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrSHLA( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
   
    if ( rExpr.typ == TYP_GREG ) {
        
        if ( getBit( *instr, 10 )) throw ( ERR_INSTR_MODE_OPT_COMBO );
        else setBitField( instr, 31, 4, tok -> tokVal( ));
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        if ( getBit( *instr, 11 )) {
            
            if ( ! isInRangeForBitFieldU( rExpr.numVal, 4 )) throw ( ERR_IMM_VAL_RANGE );
        }
    
        setBitField( instr, 31, 4, rExpr.numVal );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitFieldU( rExpr.numVal, 2 )) setBitField( instr, 21, 2, rExpr.numVal );
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
       
   checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The CMR instruction tests register "B" for a condition and if true copies the "A" value to "R".
//
//      CMR "." <cond> <targetReg> "," <regA> "," <regB>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrCMR( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "LDIL" instruction loads the immediate value encoded in the instruction left shifted into "R". The
// "ADDIL" instruction will add the value encoded in the instruction left shifted to "R". The result is
// in R1.
//
//      LDIL  <targetReg> "," <val>
//      ADDIL <sourceReg> "," <val>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLDILandADDIL( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {

        if ( isInRangeForBitFieldU( rExpr.numVal, 22 )) setBitField( instr, 31, 22, rExpr.numVal  );
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "LDO" instruction computes the address of an operand, and stores the result in "R".
//
//      LDO <targetReg> "," [ <ofs> "," ] "(" <baseReg> ")"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLDO( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitField( rExpr.numVal, 18 )) setBitField( instr, 27, 18, rExpr.numVal );
        else throw ( ERR_IMM_VAL_RANGE );
        
        parseExpr( &rExpr );
        
        if ( rExpr.typ == TYP_ADR ) setBitField( instr, 31, 4, rExpr.numVal );
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    else if ( rExpr.typ == TYP_ADR ) {
        
        setBitField( instr, 27, 18, 0 );
        setBitField( instr, 31, 4, rExpr.numVal );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
   
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "B" and "GATE" instruction represent an instruction offset relative branch. Optionally, there is an
// optional return register. When omitted, R0 is used in the instruction generation.
//
//      B       <offset> [ "," <returnReg> ]
//      GATE    <offset> [ "," <returnReg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBandGATE( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitField( rExpr.numVal, 22 )) setBitField( instr, 31, 22, rExpr.numVal >> 2 );
        else throw ( ERR_OFFSET_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_AN_OFFSET_VAL );
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBitField( instr, 9, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "BR" instruction is an IA-relative branch with the offset to be added in a general register. There is
// also an optional return register. When omitted, R0 is used in the instruction generation.
//
//      BR "(" <branchReg> ")" [ "," <returnReg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBR( uint32_t *instr, uint32_t flags ) {
    
    acceptLparen( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptRparen( );
   
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBitField( instr, 9, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "BV" is an absolute branch address instruction in the same segment. Optionally, there is an optional
// return register. When omitted, R0 is used in the instruction generation.
//
//      BV "(" <targetAdrReg> ")" [ "," <returnReg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBV( uint32_t *instr, uint32_t flags ) {
    
    acceptLparen( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptRparen( );
   
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBitField( instr, 31, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "BE" instruction is an external branch to a segment and a segment relative offset. When the offset
// part is omitted, a zero is used. There is also an optional return register. When omitted, R0 is used in
// the instruction generation.
//
//      BE [ <ofs> ] "(" <segReg> "," <ofsReg> ")" [ "," <retSeg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBE( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
   
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitField( rExpr.numVal, 22 )) setBitField( instr, 23, 14, rExpr.numVal >> 2 );
        else throw ( ERR_IMM_VAL_RANGE );
           
        parseExpr( &rExpr );
    }
    
    if ( rExpr.typ == TYP_EXT_ADR ) {
        
        setBitField( instr, 27, 4, rExpr.sReg );
        setBitField( instr, 31, 4, rExpr.gReg );
    }
    else throw ( ERR_EXPECTED_EXT_ADR );
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBitField( instr, 9, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "BVE" instruction forms a logical address by adding general register "a" to base register "b". There
// is also an optional return register. When omitted, R0 is used in the instruction generation.
//
//      BVE [ <offsetReg> ] "(" <baseReg> ")" [ "," <returnReg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBVE( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_ADR ) {
        
        setBitField( instr, 31, 4, rExpr.numVal );
    }
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
      
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBitField( instr, 9, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_EXPECTED_GENERAL_REG );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "CBR" and "CBRU" compare register "a" and "b" based on the condition and branch if the comparison
// result is true. The condition code is encoded in the instruction option string parsed before.
//
//      CBR  .<cond> <a>, <b>, <ofs>
//      CBRU .<cond> <a>, <b>, <ofs>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrCBRandCBRU( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
    
        if ( isInRangeForBitField( rExpr.numVal, 16 )) {
            
            setBitField( instr, 23, 16, rExpr.numVal >> 2 );
            tok -> nextToken( );
        }
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_AN_OFFSET_VAL );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrLoad" will parse the load instructions family. The workhorse is the "parseLoadStoreOperand"
// routine, which parses the operand. General form:
//
//      <opCode>.<opt> <targetReg>, <sourceOperand>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLoadAndStore( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseLoadStoreOperand( instr, flags );
}

//------------------------------------------------------------------------------------------------------------
// The "MR" instruction is a move register instruction. We parse valid combination and assemble the
// instruction. Note that the "MR" instruction is primarily used for moving segment and control registers
// to and from a general register. However, the syntax can also be used to move between general registers.
// We will in this case emit an "OR" instruction.
//
//      MR <targetReg> "," <sourceReg>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrMR( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        uint8_t tRegId = tok -> tokVal( );
        
        tok -> nextToken( );
        acceptComma( );
        
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            *instr = 0;
            setBitField( instr, 5, 6, OP_OR );
            setBitField( instr, 9, 4, tRegId );
            setBitField( instr, 13, 2, 1 );
            setBitField( instr, 27, 4, 0 );
            setBitField( instr, 31, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else if ( tok -> isTokenTyp( TYP_SREG )) {
          
            setBitField( instr, 31, 3, tok -> tokVal( ));
            setBitField( instr, 9, 4, tRegId );
            tok -> nextToken( );
        }
        else if ( tok -> isTokenTyp( TYP_CREG )) {
           
            setBit( instr, 11 );
            setBitField( instr, 31, 5, tok -> tokVal( ));
            setBitField( instr, 9, 4, tRegId );
            tok -> nextToken( );
        }
    }
    else if ( tok -> isTokenTyp( TYP_SREG )) {
        
        uint8_t tRegId = tok -> tokVal( );
        
        tok -> nextToken( );
        acceptComma( );
       
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBit( instr, 10 );
            setBitField( instr, 31, 3, tRegId );
            setBitField( instr, 9, 4, tok -> tokVal( ));
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_REG_COMBO );
    }
    else if ( tok -> isTokenTyp( TYP_CREG )) {
        
        uint8_t tRegId = tok -> tokVal( );
        
        tok -> nextToken( );
        acceptComma( );
       
        if ( tok -> isTokenTyp( TYP_GREG )) {
            
            setBit( instr, 10 );
            setBit( instr, 11 );
            setBitField( instr, 31, 5, tRegId );
            setBitField( instr, 9, 4, tok -> tokVal( ) );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_REG_COMBO );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "MST" instruction sets and clears bits in the program state word. There are two basic formats. The
// first format will use a general register for the data bits, the second format will use the value encoded
// in the instruction.
//
//      MST b
//      MST.S <val>
//      MST.C <val>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrMST( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_GREG ) {
        
        if ( getBitField( *instr, 11, 2 ) == 0 ) {
            
            setBitField( instr, 31, 4, rExpr.numVal );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_INSTR_OPT );
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        if (( getBitField( *instr, 11, 2 ) == 1 ) || ( getBitField( *instr, 11, 2 ) == 2 )) {
            
            if ( isInRangeForBitFieldU( rExpr.numVal, 6 )) setBitField( instr, 31, 6, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else throw ( ERR_INVALID_INSTR_OPT );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
  
   checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "LDPA" instruction loads a physical address for the logical address. When the segment is explicitly
// used, it must be in the range of SR1 to SR3.
//
//      LDPA <targetReg> ","  <indexReg> "(" [ <segmentReg>, ] <offsetReg > ")"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLDPA( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    parseLogicalAdr( instr, flags );
}

//------------------------------------------------------------------------------------------------------------
// The "PRB" instruction will test a logical address for the desired read or write access. The "I" bit will
// when cleared use the "A" reg as input, else bit 27 of the instruction.
//
//      PRB [ "." <opt> ] <targetReg> "," "(" [ <segmentReg>, ] <offsetReg > ")" [ "," <argReg> ]
//
//------------------------------------------------------------------------------------------------------------
void parseInstrPRB( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    acceptComma( );
    parseLogicalAdr( instr, flags );
    acceptComma( );
    parseExpr( &rExpr );
   
    if ( getBit( *instr, 11 )) {
        
        if ( rExpr.typ == TYP_NUM ) {
            
            if ( isInRangeForBitFieldU( rExpr.numVal, 1 )) setBit( instr, 27, rExpr.numVal );
        }
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        setBitField( instr, 27, 4, rExpr.numVal );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
        
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "ITLB" instruction will insert a new entry in the instruction or data TLB. We use the segment and
// offset register pair for the virtual address to enter.
//
//      ITLB [.<opt>] <tlbInfoReg> "," "(" <segmentReg> "," <offsetReg> ")"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrITLB( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    acceptComma( );
    acceptLparen( );
   
    if ( tok -> isTokenTyp( TYP_SREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_SEGMENT_REG );
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptRparen( );
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "PTLB" instruction removes an entry from the instruction or data TLB. We use a logical address to
// refer to the TLB entry.
//
//      PTLB [ "." <opt> ] [ <indexReg" ] "(" [ <segmentReg>, ] <offsetReg > ")"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrPTLB( uint32_t *instr, uint32_t flags ) {

    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    if ( tok -> isToken( TOK_LPAREN )) parseLogicalAdr( instr, flags );
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
        
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "PCA" instruction flushes and / or remove an entry from a data or instruction cache.
//
//      PCA [ "." <opt> ] [ <indexReg" ] "(" [ <segmentReg>, ] <offsetReg > ")"
//
//------------------------------------------------------------------------------------------------------------
void parseInstrPCA( uint32_t *instr, uint32_t flags ) {
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    if ( tok -> isToken( TOK_LPAREN )) parseLogicalAdr( instr, flags );
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
        
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "DIAG" instruction is the instruction for invoking special hardware or diagnostic functions.
//
//      DIAG <resultReg> "," <parmRegA> "," <parmRegB> "," <info>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrDIAG( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 9, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    
    acceptComma( );
   
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 27, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    if ( tok -> isTokenTyp( TYP_GREG )) {
        
        setBitField( instr, 31, 4, tok -> tokVal( ) );
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
            
        if ( isInRangeForBitFieldU( rExpr.numVal, 4 )) {
                
            setBitField( instr, 13, 4, rExpr.numVal );
            tok -> nextToken( );
        }
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
       
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "RFI" instruction is the return from interrupt method. So far it is only the instruction with no
// further options and arguments.
//
//      RFI
//
//------------------------------------------------------------------------------------------------------------
void parseInstrRFI( uint32_t *instr, uint32_t flags ) {
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "BRK" instruction will raise a trap passing along two info fields.
//
//      BRK <info1> "," <info2>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrBRK( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isInRangeForBitFieldU( rExpr.numVal, 4 )) setBitField( instr, 9, 4, rExpr.numVal );
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
    
        if ( isInRangeForBitFieldU( rExpr.numVal, 16 )) setBitField( instr, 31, 16, rExpr.numVal );
        else throw ( ERR_IMM_VAL_RANGE );
    }
    else  throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The "NOP" synthetic instruction emits the "BRK 0,0" instruction. Easy case.
//
//      NOP
//
//------------------------------------------------------------------------------------------------------------
void parseSynthInstrNop( uint32_t *instr, uint32_t flags ) {
    
    *instr = 0x0;
    
    tok -> nextToken( );
    checkEOS( );
}

// ??? add shift and rotate synthetic ops....

//------------------------------------------------------------------------------------------------------------
// "parseLine" will take the input string and parse the line for an instruction. In the simplified case, there
// is only the opCode mnemonic and the argument list. No labels, no comments. For each instruction, there is
// is a routine that parses the instruction specific input.
//
// An instruction starts with the opCode and the optional option qualifiers. For each opCode, the token table
// has an instruction template and some further information about the instruction, which is used to do further
// syntax checking.. For example, mapping the "LDx" instruction to "LDW" is already encoded in the template
// and set in the flags field.
//
// The next step for all instructions is to check for options. Finally, a dedicated parsing routine will
// handle the remainder of the assembly line. As the parsing process comes along the instruction template
// from the token name table will be augmented with further data. If all is successful, we will have the
// final instruction bit pattern.
//
//------------------------------------------------------------------------------------------------------------
void parseLine( char *inputStr, uint32_t *instr ) {
    
    uint32_t    flags   = 0;
    SimTokId       opCode  = TOK_NIL;
    
    tok -> setupTokenizer( inputStr, (SimToken *) asmTokTab );
    tok -> nextToken( );
    
    if ( tok -> isTokenTyp( TYP_OP_CODE )) {
        
        flags   = 0;
        opCode  = tok -> tokId( );
        *instr  = tok -> tokVal( );
       
        tok -> nextToken( );
        while ( tok -> isToken( TOK_PERIOD )) {
            
            tok -> nextToken( );
            parseInstrOptions( instr, &flags );
        }
        
        switch( opCode ) {
                
            case OP_CODE_ADD:   case OP_CODE_ADDW:
            case OP_CODE_ADC:   case OP_CODE_ADCW:
            case OP_CODE_SUB:   case OP_CODE_SUBW:
            case OP_CODE_SBC:   case OP_CODE_SBCW:
            case OP_CODE_AND:   case OP_CODE_ANDW:
            case OP_CODE_OR:    case OP_CODE_ORW:
            case OP_CODE_XOR:   case OP_CODE_XORW:
            case OP_CODE_CMP:   case OP_CODE_CMPW:
            case OP_CODE_CMPU:  case OP_CODE_CMPUW: {
                
                return( parseModeTypeInstr( instr, flags | TF_WORD_INSTR ));
            }
                
            case OP_CODE_ADDB:  case OP_CODE_ADCB:  case OP_CODE_SUBB:  case OP_CODE_SBCB:
            case OP_CODE_ANDB:  case OP_CODE_ORB:   case OP_CODE_XORB:  case OP_CODE_CMPB:
            case OP_CODE_CMPUB: {
                
                return( parseModeTypeInstr( instr, flags | TF_BYTE_INSTR ));
            }
                
            case OP_CODE_ADDH:  case OP_CODE_ADCH:  case OP_CODE_SUBH:  case OP_CODE_SBCH:
            case OP_CODE_ANDH:  case OP_CODE_ORH:   case OP_CODE_XORH:  case OP_CODE_CMPH:
            case OP_CODE_CMPUH: {
                
                return( parseModeTypeInstr( instr, flags | TF_HALF_INSTR ));
            }
                
            case OP_CODE_LD:    case OP_CODE_LDW:   case OP_CODE_LDA:   case OP_CODE_LDR:
            case OP_CODE_ST:    case OP_CODE_STW:   case OP_CODE_STA:   case OP_CODE_STC: {
                
                return( parseInstrLoadAndStore( instr, flags | TF_WORD_INSTR ));
            }
            
            case OP_CODE_STB:   case OP_CODE_LDB: {
                
                return( parseInstrLoadAndStore( instr, flags | TF_BYTE_INSTR ));
            }
            
            case OP_CODE_LDH:   case OP_CODE_STH: {
                
                return( parseInstrLoadAndStore( instr, flags | TF_HALF_INSTR ));
            }
            
            case OP_CODE_LSID:      return( parseInstrLSID( instr, flags ));
            case OP_CODE_EXTR:      return( parseInstrEXTR( instr, flags ));
            case OP_CODE_DEP:       return( parseInstrDEP( instr, flags ));
                
            case OP_CODE_DS:        return( parseInstrDS( instr, flags ));
                
            case OP_CODE_DSR:       return( parseInstrDSR( instr, flags ));
            case OP_CODE_SHLA:      return( parseInstrSHLA( instr, flags ));
            case OP_CODE_CMR:       return( parseInstrCMR( instr, flags ));
                
                
            case OP_CODE_LDIL:
            case OP_CODE_ADDIL:     return( parseInstrLDILandADDIL( instr, flags ));
                
            case OP_CODE_LDO:       return( parseInstrLDO( instr, flags ));
                
            case OP_CODE_B:
            case OP_CODE_GATE:      return( parseInstrBandGATE( instr, flags ));
                
            case OP_CODE_BR:        return( parseInstrBR( instr, flags ));
            case OP_CODE_BV:        return( parseInstrBV( instr, flags ));
            case OP_CODE_BE:        return( parseInstrBE( instr, flags ));
            case OP_CODE_BVE:       return( parseInstrBVE( instr, flags ));
                
            case OP_CODE_CBR:
            case OP_CODE_CBRU:      return( parseInstrCBRandCBRU( instr, flags ));
                
            case OP_CODE_MR:        return( parseInstrMR( instr, flags ));
            case OP_CODE_MST:       return( parseInstrMST( instr, flags ));
            case OP_CODE_LDPA:      return( parseInstrLDPA( instr, flags ));
            case OP_CODE_PRB:       return( parseInstrPRB( instr, flags ));
            case OP_CODE_ITLB:      return( parseInstrITLB( instr, flags ));
            case OP_CODE_PTLB:      return( parseInstrPTLB( instr, flags ));
            case OP_CODE_PCA:       return( parseInstrPCA( instr, flags ));
            case OP_CODE_DIAG:      return( parseInstrDIAG( instr, flags ));
            case OP_CODE_RFI:       return( parseInstrRFI( instr, flags ));
            case OP_CODE_BRK:       return( parseInstrBRK( instr, flags ));
              
            default: throw ( ERR_INVALID_OP_CODE );
        }
    }
    else if ( tok -> isTokenTyp( TYP_OP_CODE_S )) {
        
        flags   = 0;
        opCode  = tok -> tokId( );
        *instr  = 0;
       
        switch ( opCode ) {
                
            case OP_CODE_S_NOP: return( parseSynthInstrNop( instr, flags ));
            
            default: throw ( ERR_INVALID_S_OP_CODE );
        }
    }
    else throw ( ERR_INVALID_OP_CODE );
}

} // namespace

//------------------------------------------------------------------------------------------------------------
// A simple one line assembler. This object is the counterpart to the disassembler. We will parse a one line
// input string for a valid instruction, using the syntax of the real assembler. There will be no labels and
// comments, only the opcode and the operands.
//
//------------------------------------------------------------------------------------------------------------
SimErrMsgId SimOneLineAsm::parseAsmLine( char *inputStr, uint32_t *instr ) {
    
    try {
        
        char tmpBuf[ CMD_LINE_BUF_SIZE ];
        strcpy( tmpBuf, inputStr );
        upshiftStr( tmpBuf );
        parseLine( tmpBuf, instr );
        return( NO_ERR );
    }
    catch ( SimErrMsgId errNum ) {
        
        *instr = 0;
        return( errNum );
    }
}
#endif

}
