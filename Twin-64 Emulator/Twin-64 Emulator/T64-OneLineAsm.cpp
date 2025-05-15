

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
const int   MAX_INPUT_LINE_SIZE = 256;
const int   MAX_TOKEN_NAME_SIZE = 32;
const int   TOK_STR_SIZE        = 256;
const char  EOS_CHAR            = 0;

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
enum ErrId : uint16_t {
    
    NO_ERR                          = 0,
    ERR_EXTRA_TOKEN_IN_STR          = 4,
    ERR_INVALID_CHAR_IN_IDENT       = 25,
    ERR_INVALID_EXPR                = 20,
    ERR_INVALID_NUM                 = 24,
    ERR_EXPECTED_CLOSING_QUOTE      = 323,
    ERR_EXPECTED_NUMERIC            = 103,
    ERR_EXPECTED_COMMA              = 100,
    ERR_EXPECTED_LPAREN             = 101,
    ERR_EXPECTED_RPAREN             = 102,
    ERR_EXPECTED_STR                = 324,
    ERR_EXPECTED_EXPR               = 325,
    ERR_EXPR_TYPE_MATCH             = 406,
    ERR_EXPR_FACTOR                 = 407,
    ERR_EXPECTED_INSTR_OPT          = 409,
    ERR_INVALID_INSTR_OPT           = 410,
    ERR_INVALID_OP_CODE             = 411,
    ERR_EXPECTED_GENERAL_REG        = 412,
    ERR_IMM_VAL_RANGE               = 413,
    ERR_EXPECTED_LOGICAL_ADR        = 414,
    ERR_INVALID_INSTR_MODE          = 415,
};

//------------------------------------------------------------------------------------------------------------
// Command line tokens and expression have a type.
//
//------------------------------------------------------------------------------------------------------------
enum TokTypeId : uint16_t {
    
    TYP_NIL                 = 0,
    TYP_SYM                 = 1,        TYP_IDENT               = 2,        TYP_PREDEFINED_FUNC     = 3,
    TYP_NUM                 = 4,        TYP_STR                 = 5,        TYP_BOOL                = 6,
    TYP_ADR                 = 7,        TYP_OP_CODE             = 8,        TYP_GREG                = 9,
    TYP_CREG                = 10,       TYP_PSW_PREG            = 11,
};

//------------------------------------------------------------------------------------------------------------
// Tokens are the labels for reserved words and symbols recognized by the tokenizer objects. Tokens have a
// name, a token id, a token type and an optional value with further data.
//
//------------------------------------------------------------------------------------------------------------
enum TokId : uint16_t {
    
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
    TOK_IDENT               = 24,       TOK_NUM                 = 25,       TOK_STR                 = 26,
    
    //--------------------------------------------------------------------------------------------------------
    // General, Segment and Control Registers Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    REG_SET                 = 100,
    
    GR_0                    = 101,      GR_1                    = 102,      GR_2                    = 103,
    GR_3                    = 104,      GR_4                    = 105,      GR_5                    = 106,
    GR_6                    = 107,      GR_7                    = 108,      GR_8                    = 109,
    GR_9                    = 110,      GR_10                   = 111,      GR_11                   = 112,
    GR_12                   = 113,      GR_13                   = 114,      GR_14                   = 115,
    GR_15                   = 116,
    
    CR_0                    = 121,      CR_1                    = 122,      CR_2                    = 123,
    CR_3                    = 124,      CR_4                    = 125,      CR_5                    = 126,
    CR_6                    = 127,      CR_7                    = 128,      CR_8                    = 129,
    CR_9                    = 130,      CR_10                   = 131,      CR_11                   = 132,
    CR_12                   = 133,      CR_13                   = 134,      CR_14                   = 136,
    CR_15                   = 137,
    
    //--------------------------------------------------------------------------------------------------------
    // OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    OP_NOP                  = 300,
    OP_AND                  = 301,      OP_OR                   = 302,      OP_XOR                  = 303,
    OP_ADD                  = 304,      OP_SUB                  = 305,      OP_CMP                  = 306,
    OP_EXTR                 = 307,      OP_DEP                  = 308,      OP_DSR                  = 309,
    OP_SHLA                 = 310,
    
    OP_LDIL                 = 311,      OP_ADDIL                = 312,      OP_LDO                  = 313,
    OP_LD                   = 314,      OP_LDR                  = 315,
    OP_ST                   = 316,      OP_STC                  = 317,
    
    OP_B                    = 318,      OP_BR                   = 319,      OP_BV                   = 320,
    OP_CBR                  = 321,      OP_TBR                  = 322,      OP_MBR                  = 323,
    
    OP_MR                   = 324,      OP_MST                  = 325,      OP_DS                   = 326,
    OP_LDPA                 = 327,      OP_PRB                  = 328,      OP_ITLB                 = 329,
    OP_PTLB                 = 330,      OP_PCA                  = 331,      OP_DIAG                 = 332,
    OP_RFI                  = 333,      OP_BRK                  = 334,
    
    //--------------------------------------------------------------------------------------------------------
    // Synthetic OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    OP_SHL                  = 401,      OP_SHR                  = 402,
    OP_ASL                  = 403,      OP_ASR                  = 404,
    OP_ROR                  = 405,      OP_ROL                  = 406,
    
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
    TokTypeId    typ                         = TYP_NIL;
    TokId        tid                         = TOK_NIL;
    
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
    { .name = "R0",             .typ = TYP_GREG,            .tid = GR_0,                .val = 0            },
    { .name = "R1",             .typ = TYP_GREG,            .tid = GR_1,                .val = 1            },
    { .name = "R2",             .typ = TYP_GREG,            .tid = GR_2,                .val = 2            },
    { .name = "R3",             .typ = TYP_GREG,            .tid = GR_3,                .val = 3            },
    { .name = "R4",             .typ = TYP_GREG,            .tid = GR_4,                .val = 4            },
    { .name = "R5",             .typ = TYP_GREG,            .tid = GR_5,                .val = 5            },
    { .name = "R6",             .typ = TYP_GREG,            .tid = GR_6,                .val = 6            },
    { .name = "R7",             .typ = TYP_GREG,            .tid = GR_7,                .val = 7            },
    { .name = "R8",             .typ = TYP_GREG,            .tid = GR_8,                .val = 8            },
    { .name = "R9",             .typ = TYP_GREG,            .tid = GR_9,                .val = 9            },
    { .name = "R10",            .typ = TYP_GREG,            .tid = GR_10,               .val = 10           },
    { .name = "R11",            .typ = TYP_GREG,            .tid = GR_11,               .val = 11           },
    { .name = "R12",            .typ = TYP_GREG,            .tid = GR_12,               .val = 12           },
    { .name = "R13",            .typ = TYP_GREG,            .tid = GR_13,               .val = 13           },
    { .name = "R14",            .typ = TYP_GREG,            .tid = GR_14,               .val = 14           },
    { .name = "R15",            .typ = TYP_GREG,            .tid = GR_15,               .val = 15           },
    
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
    
    //--------------------------------------------------------------------------------------------------------
    // Runtime architcture register names for general registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "T0",             .typ = TYP_GREG,            .tid = GR_1,                .val =  1           },
    { .name = "T1",             .typ = TYP_GREG,            .tid = GR_2,                .val =  2           },
    { .name = "T2",             .typ = TYP_GREG,            .tid = GR_3,                .val =  3           },
    { .name = "T3",             .typ = TYP_GREG,            .tid = GR_4,                .val =  4           },
    { .name = "T4",             .typ = TYP_GREG,            .tid = GR_5,                .val =  5           },
    { .name = "T5",             .typ = TYP_GREG,            .tid = GR_6,                .val =  6           },
    { .name = "T6",             .typ = TYP_GREG,            .tid = GR_7,                .val =  7           },
    
    { .name = "ARG3",           .typ = TYP_GREG,            .tid = GR_8,                .val =  8           },
    { .name = "ARG2",           .typ = TYP_GREG,            .tid = GR_9,                .val =  9           },
    { .name = "ARG1",           .typ = TYP_GREG,            .tid = GR_10,               .val =  10          },
    { .name = "ARG0",           .typ = TYP_GREG,            .tid = GR_11,               .val =  11          },
    
    { .name = "RET3",           .typ = TYP_GREG,            .tid = GR_8,                .val =  8           },
    { .name = "RET2",           .typ = TYP_GREG,            .tid = GR_9,                .val =  9           },
    { .name = "RET1",           .typ = TYP_GREG,            .tid = GR_10,               .val =  10          },
    { .name = "RET0",           .typ = TYP_GREG,            .tid = GR_11,               .val =  11          },
    
    { .name = "DP",             .typ = TYP_GREG,            .tid = GR_13,               .val =  13          },
    { .name = "RL",             .typ = TYP_GREG,            .tid = GR_14,               .val =  14          },
    { .name = "SP",             .typ = TYP_GREG,            .tid = GR_15,               .val =  15          },
    
    //--------------------------------------------------------------------------------------------------------
    // Assembler mnemonics.
    //
    // ??? add opcode which makes sense as words instead of options ?
    //--------------------------------------------------------------------------------------------------------
    { .name = "NOP",            .typ = TYP_OP_CODE,         .tid = OP_NOP,              .val = 0x00000000   },
    { .name = "ADD",            .typ = TYP_OP_CODE,         .tid = OP_ADD,              .val = 0x00000000   },
    { .name = "SUB",            .typ = TYP_OP_CODE,         .tid = OP_SUB,              .val = 0x00000000   },
    { .name = "AND",            .typ = TYP_OP_CODE,         .tid = OP_AND,              .val = 0x00000000   },
    { .name = "OR" ,            .typ = TYP_OP_CODE,         .tid = OP_OR,               .val = 0x00000000   },
    { .name = "XOR" ,           .typ = TYP_OP_CODE,         .tid = OP_XOR,              .val = 0x00000000   },
    { .name = "CMP" ,           .typ = TYP_OP_CODE,         .tid = OP_CMP,              .val = 0x00000000   },
    { .name = "EXTR",           .typ = TYP_OP_CODE,         .tid = OP_EXTR,             .val = 0x00000000   },
    { .name = "DEP",            .typ = TYP_OP_CODE,         .tid = OP_DEP,              .val = 0x00000000   },
    { .name = "DSR",            .typ = TYP_OP_CODE,         .tid = OP_DSR,              .val = 0x00000000   },
    { .name = "SHLA",           .typ = TYP_OP_CODE,         .tid = OP_SHLA,             .val = 0x00000000   },
    { .name = "LDIL",           .typ = TYP_OP_CODE,         .tid = OP_LDIL,             .val = 0x00000000   },
    { .name = "ADDIL",          .typ = TYP_OP_CODE,         .tid = OP_ADDIL,            .val = 0x00000000   },
    { .name = "LDO",            .typ = TYP_OP_CODE,         .tid = OP_LDO,              .val = 0x00000000   },
    { .name = "LD",             .typ = TYP_OP_CODE,         .tid = OP_LD,               .val = 0x00000000   },
    { .name = "LDR",            .typ = TYP_OP_CODE,         .tid = OP_LDR,              .val = 0x00000000   },
    { .name = "ST",             .typ = TYP_OP_CODE,         .tid = OP_ST,               .val = 0x00000000   },
    { .name = "STC",            .typ = TYP_OP_CODE,         .tid = OP_STC,              .val = 0x00000000   },
    { .name = "B",              .typ = TYP_OP_CODE,         .tid = OP_B,                .val = 0x00000000   },
    { .name = "BR",             .typ = TYP_OP_CODE,         .tid = OP_BR,               .val = 0x00000000   },
    { .name = "BV",             .typ = TYP_OP_CODE,         .tid = OP_BV,               .val = 0x00000000   },
    { .name = "CBR",            .typ = TYP_OP_CODE,         .tid = OP_CBR,              .val = 0x00000000   },
    { .name = "TBR",            .typ = TYP_OP_CODE,         .tid = OP_TBR,              .val = 0x00000000   },
    { .name = "MBR",            .typ = TYP_OP_CODE,         .tid = OP_MBR,              .val = 0x00000000   },
    { .name = "MR",             .typ = TYP_OP_CODE,         .tid = OP_MR,               .val = 0x00000000   },
    { .name = "MST",            .typ = TYP_OP_CODE,         .tid = OP_MST,              .val = 0x00000000   },
    { .name = "DS",             .typ = TYP_OP_CODE,         .tid = OP_DS,               .val = 0x00000000   },
    { .name = "LDPA",           .typ = TYP_OP_CODE,         .tid = OP_LDPA,             .val = 0x00000000   },
    { .name = "PRB",            .typ = TYP_OP_CODE,         .tid = OP_PRB,              .val = 0x00000000   },
    { .name = "ITLB",           .typ = TYP_OP_CODE,         .tid = OP_ITLB,             .val = 0x00000000   },
    { .name = "PTLB",           .typ = TYP_OP_CODE,         .tid = OP_PTLB,             .val = 0x00000000   },
    { .name = "PCA",            .typ = TYP_OP_CODE,         .tid = OP_PCA,              .val = 0x00000000   },
    { .name = "DIAG",           .typ = TYP_OP_CODE,         .tid = OP_DIAG,             .val = 0x00000000   },
    { .name = "RFI",            .typ = TYP_OP_CODE,         .tid = OP_RFI,              .val = 0x00000000   },
    { .name = "BRK",            .typ = TYP_OP_CODE,         .tid = OP_BRK,              .val = 0x00000000   },
    
    //--------------------------------------------------------------------------------------------------------
    // Synthetic instruction mnemonics.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "SHL",            .typ = TYP_OP_CODE,         .tid = OP_SHL,              .val =  0           },
    { .name = "SHR",            .typ = TYP_OP_CODE,         .tid = OP_SHR,              .val =  0           },
    { .name = "ASL",            .typ = TYP_OP_CODE,         .tid = OP_ASL,              .val =  0           },
    { .name = "ASR",            .typ = TYP_OP_CODE,         .tid = OP_ASR,              .val =  0           },
    { .name = "ROR",            .typ = TYP_OP_CODE,         .tid = OP_ROR,              .val =  0           },
    { .name = "ROL",            .typ = TYP_OP_CODE,         .tid = OP_ROL,              .val =  0           }
    
};

const int MAX_ASM_TOKEN_TAB = sizeof( asmTokTab ) / sizeof( SimToken );

//------------------------------------------------------------------------------------------------------------
// Instruction flags. They are used to keep track of instruction attributes used in assembling the final
// word. Examples are the data width encoded in the opCode and the instruction mask.
//
//------------------------------------------------------------------------------------------------------------
enum InstrFlags : uint32_t {
    
    IF_NIL                  = 0,
    IF_BYTE_INSTR           = ( 1U << 0 ),
    IF_HALF_INSTR           = ( 1U << 1 ),
    IF_WORD_INSTR           = ( 1U << 2 ),
    IF_DOUBLE_INSTR         = ( 1U << 3 ),
    IF_ADR_UPDATE           = ( 1U << 4 ),
    IF_USE_SHAMT_REG        = ( 1U << 5 ),
    IF_USE_IMM_VALUE        = ( 1U << 6 ),
    IF_USE_IMM_LEFT         = ( 1U << 7 ),
    IF_USE_IMM_RIGHT        = ( 1U << 8 ),
    IF_USE_IMM_UPPER        = ( 1U << 9 ),
    IF_REG_COMPLEMENT       = ( 1U << 10 ),
    IF_REG_ZERO_BEFORE      = ( 1U << 11 ),
    IF_RES_SIGN_EXT         = ( 1U << 12 ),
    IF_RES_NEGATE           = ( 1U << 13 ),
    IF_MOV_TO               = ( 1U << 14 ),
    IF_MOV_FROM             = ( 1U << 15 ),
    IF_READ_ACCESS          = ( 1U << 16 ),
    IF_WRITE_ACCESS         = ( 1U << 17 ),
    IF_EXEC_ACCESS          = ( 1U << 18 ),
    IF_CODE_TLB             = ( 1U << 19 ),
    IF_DATA_TLB             = ( 1U << 20 ),
    IF_INSERT_OP            = ( 1U << 21 ),
    IF_PURGE_OP             = ( 1U << 22 ),
    IF_FLUSH_OP             = ( 1U << 23 ),
    
    IF_CMP_EQ               = ( 1U << 24 ),
    IF_CMP_NE               = ( 1U << 25 ),
    IF_CMP_LT               = ( 1U << 26 ),
    IF_CMP_LE               = ( 1U << 27 ),
    IF_CMP_OD               = ( 1U << 28 ),
    IF_CMP_EV               = ( 1U << 29 ),
};

//------------------------------------------------------------------------------------------------------------
// Expression value. The analysis of an expression results in a value. Depending on the expression type, the
// values are simple scalar values or a structured value, such as a register pair or virtual address.
//
//------------------------------------------------------------------------------------------------------------
struct SimExpr {
    
    TokTypeId typ;
    
    union {
        
        struct {    TokId       tokId;                      };
        struct {    bool        bVal;                       };
        struct {    int64_t     numVal;                     };
        struct {    int64_t     adr;                        };
        struct {    char        strVal[ TOK_STR_SIZE ];     };
    };
};

//------------------------------------------------------------------------------------------------------------
// Global variables, Assembler.
//
//------------------------------------------------------------------------------------------------------------
char        tokenLine[ MAX_INPUT_LINE_SIZE ]    = { 0 };
int         currentLineLen                      = 0;
int         currentCharIndex                    = 0;
int         currentTokCharIndex                 = 0;
char        currentChar                         = ' ';
SimToken    currentToken;

//------------------------------------------------------------------------------------------------------------
// Global variables, Assembled instruction.
//
//------------------------------------------------------------------------------------------------------------
uint32_t    instrWord;
uint32_t    instrOpCode;
uint32_t    instrFlags;

//------------------------------------------------------------------------------------------------------------
// "parseExpr" needs to be declared forward.
//
//------------------------------------------------------------------------------------------------------------
void parseExpr( SimExpr *rExpr );

//------------------------------------------------------------------------------------------------------------
// Helper functions.
//
// ??? which ones are really needd...
//------------------------------------------------------------------------------------------------------------
static inline bool isAligned( int64_t adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline bool isInRange2( int64_t adr, int64_t low, int64_t high ) {
    
    return (( adr >= low ) && ( adr <= high ));
}


bool isInRangeForBitField( int64_t val, int bitLen ) {
    
    int min = - ( 1 << (( bitLen - 1 ) % 64 ));
    int max = ( 1 << (( bitLen - 1 ) % 64 )) - 1;
    return (( val <= max ) && ( val >= min ));
}

bool isInRangeForBitFieldU( uint64_t val, int bitLen ) {
    
    int max = (( 1 << ( bitLen % 64 )) - 1 );
    return ( val <= max );
}

static inline int64_t roundup( int64_t arg ) {
    
    return ( arg ); // for now ...
}

static inline void setInstrBit( int32_t *word, int bitpos, bool value ) {
    
    uint32_t mask = 1 << bitpos;
    *word = (( *word & ~mask ) | (( value << bitpos ) & mask ));
}

static inline void setInstrField( uint32_t *word, int bitpos, int len, int64_t value ) {
    
    uint32_t mask = (( 1 << len ) - 1 ) << bitpos;
    *word = (( *word & ~mask ) | (( value << bitpos ) & mask ));
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
// The lookup function. We just do a linear search for now.
//
//------------------------------------------------------------------------------------------------------------
int lookupToken( char *inputStr, const SimToken *tokTab ) {
    
    if (( strlen( inputStr ) == 0 ) || ( strlen ( inputStr ) > MAX_TOKEN_NAME_SIZE )) return( -1 );
    
    for ( int i = 0; i < MAX_ASM_TOKEN_TAB; i++  ) {
        
        if ( strcmp( inputStr, tokTab[ i ].name ) == 0 ) return( i );
    }
    
    return ( -1 );
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
// "parseNum" will parse a number. We accept decimals and hexadecimals. The numeric string can also contain
// "_" characters which helped to make the string more readable. Hex numbers start with a "0x", decimals just
// with the numeric digits.
//
//------------------------------------------------------------------------------------------------------------
void parseNum( ) {
    
    currentToken.tid    = TOK_NUM;
    currentToken.typ    = TYP_NUM;
    currentToken.val    = 0;
    
    int     base        = 10;
    int     maxDigits   = 22;
    int     digits      = 0;
    
    if ( currentChar == '0' ) {
        
        nextChar( );
        if (( currentChar == 'x' ) || ( currentChar == 'X' )) {
            
            base        = 16;
            maxDigits   = 16;
            nextChar( );
        }
    }
    
    do {
        
        if ( currentChar == '_' ) {
            
            nextChar( );
            continue;
        }
        else {
            
            if ( isdigit( currentChar ))
                currentToken.val = ( currentToken.val * base ) + currentChar - '0';
            else if (( base == 16 ) && ( currentChar >= 'a' ) && ( currentChar <= 'f' ))
                currentToken.val = ( currentToken.val * base ) + currentChar - 'a' + 10;
            else if (( base == 16 ) && ( currentChar >= 'A' ) && ( currentChar <= 'F' ))
                currentToken.val = ( currentToken.val * base ) + currentChar - 'A' + 10;
            else throw ( ERR_INVALID_NUM );
            
            nextChar( );
            digits ++;
            
            if ( digits > maxDigits ) throw ( ERR_INVALID_NUM );
        }
    }
    while ( isxdigit( currentChar ) || ( currentChar == '_' ));
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
    
    char identBuf[ MAX_INPUT_LINE_SIZE ] = "";
    
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
            else throw ( ERR_INVALID_CHAR_IN_IDENT );
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
            else throw ( ERR_INVALID_CHAR_IN_IDENT );
        }
    }
    
    // ??? other constant options ?
    
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
// "nextToken" is the entry point to the lexer.
//
//------------------------------------------------------------------------------------------------------------
void nextToken( ) {
    
    currentToken.typ       = TYP_NIL;
    currentToken.tid       = TOK_NIL;
    
    while (( currentChar == ' ' ) || ( currentChar == '\n' ) || ( currentChar == '\r' )) nextChar( );
    
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
        throw ( ERR_INVALID_CHAR_IN_IDENT );
    }
}

//------------------------------------------------------------------------------------------------------------
// Parser helper functions.
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

bool isToken( TokId tid ) {
    
    return( currentToken.tid == tid );
}

bool isTokenTyp( TokTypeId typ ) {
    
    return( currentToken.typ = typ );
}

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
        
#if 0   // more consistent, removes ambiguity in expression analysis.
        parseExpr( rExpr );
        if ( rExp -> typ == TYP_GREG ) {
            
            rExpr -> typ    = TYP_ADR;
            rExpr -> numVal = currentToken.val;
        }
#else
        if ( isTokenTyp( TYP_GREG )) {
            
            rExpr -> typ    = TYP_ADR;
            rExpr -> numVal = currentToken.val;
            nextToken( );
        }
        else parseExpr( rExpr );
#endif
        
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

//------------------------------------------------------------------------------------------------------------
// "parseInstrOptions" will analyze the opCode option string. An opCode string is a sequence of characters.
// We will look at each character in the "name" and set the options for the particular instruction. There are
// also cases where the only option is a multi-character sequence. We detect invalid options but not when
// the same option is repeated. E.g. a "LOL" will result in "L" and "O" set.
//

// ??? maybe a different approach. Just accumulate the flags and then syntax check...
//
// ??? we should first just parse all options we can find. Then we check using the instruction at hand.
//
//------------------------------------------------------------------------------------------------------------
void parseInstrOptions( ) {
    
    if ( ! isToken( TOK_IDENT )) throw ( ERR_EXPECTED_INSTR_OPT );
    
    char    *optBuf     = currentToken.str;
    int     dwCount     = 0;
    int     cmpCount    = 0;
    
    if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrFlags |= IF_CMP_EQ;
    else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrFlags |= IF_CMP_LT;
    else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrFlags |= IF_CMP_NE;
    else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrFlags |= IF_CMP_LE;
    else {
        
        // ??? some characters may have more than one meaning ?
        // ??? if so, just report the character and interpretation is done in the check ?
        
        for ( int i = 0; i < strlen( optBuf ); i ++ ) {
            
            if      ( optBuf[ i ] == 'M' ) instrFlags |= IF_ADR_UPDATE;
            if      ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
            else if ( optBuf[ i ] == 'C' ) instrFlags |= IF_REG_COMPLEMENT;
            else if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
            else if ( optBuf[ i ] == 'S' ) instrFlags |= IF_RES_SIGN_EXT;
            else if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
            else if ( optBuf[ i ] == 'Z' ) instrFlags |= IF_REG_ZERO_BEFORE;
            else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
            else if ( optBuf[ i ] == 'R' ) instrFlags |= IF_READ_ACCESS;
            else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_WRITE_ACCESS;
            
            
            else throw ( ERR_INVALID_INSTR_OPT );
        }
    }
    
    // ??? part two check that thej flags are defined for the instruction ...
    
    switch( instrOpCode ) {
            
        case OP_LD:
        case OP_ST: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if ( optBuf[ i ] == 'M' ) instrFlags |= IF_ADR_UPDATE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_ADD:
        case OP_SUB: {
            
            throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_AND:
        case OP_OR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                else if ( optBuf[ i ] == 'C' ) instrFlags |= IF_REG_COMPLEMENT;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_XOR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_CMP: {
            
            /*
             // ??? this becomes ugly, as we have also the data width options...
             
             if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) setBitField( instr, 11, 2, 0 );
             else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) setBitField( instr, 11, 2, 1 );
             else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) setBitField( instr, 11, 2, 2 );
             else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) setBitField( instr, 11, 2, 3 );
             else throw ( ERR_INVALID_INSTR_OPT );
             */
            
        } break;
            
        case OP_CBR: {
            
            /*
             // ??? this becomes ugly, as we have also the data width options...
             
             if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) setBitField( instr, 11, 2, 0 );
             else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) setBitField( instr, 11, 2, 1 );
             else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) setBitField( instr, 11, 2, 2 );
             else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) setBitField( instr, 11, 2, 3 );
             else throw ( ERR_INVALID_INSTR_OPT );
             */
            
        } break;
            
        case OP_EXTR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'S' ) instrFlags |= IF_RES_SIGN_EXT;
                else if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_DEP: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'Z' ) instrFlags |= IF_REG_ZERO_BEFORE;
                else if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_DSR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_SHLA: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_MR: {
            
            throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_MST: {
            
            /*
             for ( int i = 0; i < strlen( optBuf ); i ++ ) {
             
             if      ( optBuf[ i ] == 'S' ) setBitField( instr, 11, 2, 1 );
             else if ( optBuf[ i ] == 'C' ) setBitField( instr, 11, 2, 2 );
             else throw ( ERR_INVALID_INSTR_OPT );
             }
             */
            
        } break;
            
        case OP_PRB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'R' ) instrFlags |= IF_READ_ACCESS; // ???
                else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_ITLB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if ( optBuf[ 0 ] == 'T' ) instrFlags |= IF_CODE_TLB; // ???
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_PTLB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if ( optBuf[ 0 ] == 'T' ) instrFlags |= IF_CODE_TLB; // ???
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_PCA: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ 0 ] == 'T' ) instrFlags |= IF_CODE_TLB; // ???
                else if ( optBuf[ i ] == 'F' ) instrFlags |= IF_FLUSH_OP;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        default: throw ( ERR_INVALID_INSTR_OPT );
    }
    
    // ??? test for data width and cmpCode setings...
    
    nextToken( );
}


#if 0



//------------------------------------------------------------------------------------------------------------
// "parseLogicalAdr" analyzes a logical address, which is used by several instruction with a "seg" field.
//
//      "(" <ofsReg> ")"
//
//------------------------------------------------------------------------------------------------------------
void parseLogicalAdr( uint32_t *instr, uint32_t flags ) {
    
    SimExpr rExpr;
    
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_ADR ) {
        
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
    else throw ( ERR_EXPECTED_LOGICAL_ADR );
}

#endif

void instrDepositCmpCode( uint32_t *instr ) {
    
    
}

void instrDepositDataWidth( uint32_t *instr ) {
    
    
}

//------------------------------------------------------------------------------------------------------------
// "parseModeTypeInstr" parses all instructions that have several types of "operand" encoding. The syntax is
// as follows:
//
//      opCode [ "." <opt> ] <targetReg> "," <num>                              -> Instruction group ALU
//      opCode [ "." <opt> ] <targetReg> "," <num> "(" <baseReg> ")"            -> Instruction group MEM
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg>                        -> Instruction group ALU
//      opCode [ "." <opt> ] <targetReg> "," <sourceRegA> "," "<sourceRegB>     -> Instruction group ALU
//      opCode [ "." <opt> ] <targetReg> "," <indexReg> "(" <baseReg> ")"       -> Instruction group MEM
//
// The instruction options have already been parsed and are available in the instrFlags variable.
//
//------------------------------------------------------------------------------------------------------------
void parseModeTypeInstr( uint32_t *instr ) {
    
    uint8_t     targetRegId = 0;
    SimExpr     rExpr;
    
    setInstrField( instr, 26, 4, instrOpCode );
    
    if ( isTokenTyp( TYP_GREG )) {
        
        targetRegId = currentToken.val;
        setInstrField( instr, 22, 4, targetRegId );
        nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isToken( TOK_EOS )) {
            
            setInstrField( instr, 30, 2, OP_GRP_ALU );
            
            if ( isInRangeForBitField( rExpr.numVal, 18 )) setInstrField( instr, 31, 18, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
        }
        else {
            
            if ( isInRangeForBitField( rExpr.numVal, 12 )) setInstrField( instr, 27, 12, rExpr.numVal );
            else throw ( ERR_IMM_VAL_RANGE );
            
            parseExpr( &rExpr );
            
            if ( rExpr.typ == TYP_ADR ) {
                
                setInstrField( instr, 13, 2, 3 );
                setInstrField( instr, 31, 4, rExpr.numVal );
            }
            else throw ( ERR_EXPECTED_LOGICAL_ADR );
            
            if      ( instrFlags & IF_BYTE_INSTR   ) setInstrField( instr, 2, 0, 0 );
            else if ( instrFlags & IF_HALF_INSTR   ) setInstrField( instr, 15, 2, 1 );
            else if ( instrFlags & IF_WORD_INSTR   ) setInstrField( instr, 15, 2, 2 );
            else if ( instrFlags & IF_DOUBLE_INSTR ) setInstrField( instr, 15, 2, 2 );
        }
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        if ( isToken( TOK_EOS )) {
            
            setInstrField( instr, 30, 2, OP_GRP_ALU );
            
            setInstrField( instr, 13, 2, 1 );
            setInstrField( instr, 27, 4, targetRegId );
            setInstrField( instr, 31, 4, rExpr.numVal );
        }
        else if ( isToken( TOK_COMMA )) {
            
            setInstrField( instr, 13, 2, 1 );
            setInstrField( instr, 27, 4, rExpr.numVal );
            
            nextToken( );
            if ( isTokenTyp( TYP_GREG )) {
               
                setInstrField( instr, 13, 2, 1 );
                setInstrField( instr, 27, 4, rExpr.numVal );
                setInstrField( instr, 31, 4, currentToken.val );
                nextToken( );
            }
            else throw ( ERR_EXPECTED_GENERAL_REG );
            
            setInstrField( instr, 30, 2, OP_GRP_ALU );
            
            if (( instrFlags & IF_BYTE_INSTR ) || ( instrFlags & IF_HALF_INSTR ) || ( instrFlags & IF_WORD_INSTR )) {
                
                // ??? invalid option for not MEM type...
            }
        }
        else if ( isToken( TOK_LPAREN )) {
            
            setInstrField( instr, 27, 4, (uint32_t) rExpr.numVal );
            
            parseExpr( &rExpr );
            if ( rExpr.typ == TYP_ADR ) {
                
                setInstrField( instr, 13, 2, 2 );
                setInstrField( instr, 31, 4, (uint32_t) rExpr.numVal );
            }
            else throw ( ERR_EXPECTED_LOGICAL_ADR );
            
            setInstrField( instr, 30, 2, OP_GRP_MEM );
            
            if      ( instrFlags & IF_BYTE_INSTR    ) setInstrField( instr, 15, 2, 0 );
            else if ( instrFlags & IF_HALF_INSTR    ) setInstrField( instr, 15, 2, 1 );
            else if ( instrFlags & IF_WORD_INSTR    ) setInstrField( instr, 15, 2, 2 );
            else if ( instrFlags & IF_DOUBLE_INSTR  ) setInstrField( instr, 15, 2, 2 );
        }
    }
    else throw ( ERR_INVALID_INSTR_MODE );
    
    checkEOS( );
}

#if 0

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
// The "CBR" and "CBRU" compare register "a" and "b" based on the condition and branch if the comparison
// result is true. The condition code is encoded in the instruction option string parsed before.
//
//      CBR  .<cond> <a>, <b>, <ofs>
//      CBRU .<cond> <a>, <b>, <ofs>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrCBR( uint32_t *instr, uint32_t flags ) {
    
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

#endif

//------------------------------------------------------------------------------------------------------------
// The "NOP" synthetic instruction emits the "BRK 0,0" instruction. Easy case.
//
//      NOP
//
//------------------------------------------------------------------------------------------------------------
void parseInstrNop( uint32_t *instr ) {
    
    // ??? complete current instruction... what to do ?
    
    *instr = 0;
    
    nextToken( );
    checkEOS( );
}

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

// ??? put in a try catch clause ?

//------------------------------------------------------------------------------------------------------------
void parseLine( char *inputStr, uint32_t *instr ) {
    
    strcpy( tokenLine, inputStr );
    upshiftStr( tokenLine );
    
    currentLineLen          = 0;
    currentCharIndex        = 0;
    currentTokCharIndex     = 0;
    currentChar             = ' ';
    instrOpCode             = OP_NOP;
    instrFlags              = IF_NIL;
   
    nextToken( );
    
    if ( isTokenTyp( TYP_OP_CODE )) {
        
        instrOpCode = currentToken.tid;
        
        nextToken( );
        while ( isToken( TOK_PERIOD )) {
            
            nextToken( );
            parseInstrOptions( );
        }
        
        switch( instrOpCode ) {
                
            case OP_NOP: parseInstrNop( instr ); break;
             
            case OP_ADD:
            case OP_SUB:
            case OP_AND:
            case OP_OR:
            case OP_XOR:
            case OP_CMP: parseModeTypeInstr( instr ); break;
                
                
                
                /*
                 
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
                 
                 */
                
                // add pseudo ops too...
                
            default: throw ( ERR_INVALID_OP_CODE );
        }
    }
}

} // namespace


//------------------------------------------------------------------------------------------------------------
// A simple one line assembler. This object is the counterpart to the disassembler. We will parse a one line
// input string for a valid instruction, using the syntax of the real assembler. There will be no labels and
// comments, only the opcode and the operands.
//
//------------------------------------------------------------------------------------------------------------
ErrId parseAsmLine( char *inputStr, uint32_t *instr ) {
    
    try {
        
        char tmpBuf[ MAX_INPUT_LINE_SIZE ];
        strcpy( tmpBuf, inputStr );
        upshiftStr( tmpBuf );
        parseLine( tmpBuf, instr );
        return( NO_ERR );
    }
    catch ( ErrId errNum ) {
        
        *instr = 0;
        return( errNum );
    }
}
