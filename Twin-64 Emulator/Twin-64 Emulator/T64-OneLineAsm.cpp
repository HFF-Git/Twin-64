

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
    ERR_EXPECTED_ADR                = 414,
    ERR_INVALID_INSTR_MODE          = 415,
    ERR_REG_VAL_RANGE               = 416
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
    
    OP_LDI                  = 311,      OP_ADDIL                = 312,      OP_LDO                  = 313,
    OP_LD                   = 314,      OP_LDR                  = 315,
    OP_ST                   = 316,      OP_STC                  = 317,
    
    OP_B                    = 318,      OP_BR                   = 319,      OP_BV                   = 320,
    OP_CBR                  = 321,      OP_MBR                  = 322,
    
    OP_MFCR                 = 324,      OP_MTCR                 = 325,
    OP_RSM                  = 326,      OP_SSM                  = 327,
    OP_LDPA                 = 328,      OP_PRB                  = 329,
    OP_ITLB                 = 330,      OP_PTLB                 = 331,
    OP_PCA                  = 332,      OP_FCA                  = 333,
    
    OP_RFI                  = 335,      OP_BRK                  = 336,      OP_CHK                  = 337,
    OP_DIAG                 = 338,
    
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
struct Token {
    
    char        name[ MAX_TOKEN_NAME_SIZE ] = { };
    TokTypeId   typ                         = TYP_NIL;
    TokId       tid                         = TOK_NIL;
    
    union {
        
        struct { T64Word val;               };
        struct { char str[ TOK_STR_SIZE ];  };
    };
};

//------------------------------------------------------------------------------------------------------------
// The global token table or the one line assembler. All reserved words are allocated in this table. Each
// entry has the token name, the token id, the token type id, i.e. its type, and a value associated with the
// token. The value allows for a constant token. The parser can directly use the value in expressions.
//
//------------------------------------------------------------------------------------------------------------
const Token asmTokTab[ ] = {
    
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
    // ??? what do we do about Opcodes sharing the same id ?
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
    { .name = "LDI",            .typ = TYP_OP_CODE,         .tid = OP_LDI,             .val = 0x00000000   },
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
    { .name = "MBR",            .typ = TYP_OP_CODE,         .tid = OP_MBR,              .val = 0x00000000   },
    { .name = "MFCR",           .typ = TYP_OP_CODE,         .tid = OP_MFCR,             .val = 0x00000000   },
    { .name = "MTCR",           .typ = TYP_OP_CODE,         .tid = OP_MTCR,             .val = 0x00000000   },
    { .name = "LDPA",           .typ = TYP_OP_CODE,         .tid = OP_LDPA,             .val = 0x00000000   },
    { .name = "PRB",            .typ = TYP_OP_CODE,         .tid = OP_PRB,              .val = 0x00000000   },
    { .name = "ITLB",           .typ = TYP_OP_CODE,         .tid = OP_ITLB,             .val = 0x00000000   },
    { .name = "PTLB",           .typ = TYP_OP_CODE,         .tid = OP_PTLB,             .val = 0x00000000   },
    { .name = "PCA",            .typ = TYP_OP_CODE,         .tid = OP_PCA,              .val = 0x00000000   },
    { .name = "FCA",            .typ = TYP_OP_CODE,         .tid = OP_FCA,              .val = 0x00000000   },
    { .name = "RSM",            .typ = TYP_OP_CODE,         .tid = OP_RSM,              .val = 0x00000000   },
    { .name = "SSM",            .typ = TYP_OP_CODE,         .tid = OP_SSM,              .val = 0x00000000   },
    { .name = "RFI",            .typ = TYP_OP_CODE,         .tid = OP_RFI,              .val = 0x00000000   },
    { .name = "CHK",            .typ = TYP_OP_CODE,         .tid = OP_CHK,              .val = 0x00000000   },
    { .name = "BRK",            .typ = TYP_OP_CODE,         .tid = OP_BRK,              .val = 0x00000000   },
    { .name = "DIAG",           .typ = TYP_OP_CODE,         .tid = OP_DIAG,             .val = 0x00000000   },
    
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

const int MAX_ASM_TOKEN_TAB = sizeof( asmTokTab ) / sizeof( Token );

//------------------------------------------------------------------------------------------------------------
// Instruction flags. They are used to keep track of instruction attributes used in assembling the final
// word. Examples are the data width encoded in the opCode and the instruction mask.
//
//------------------------------------------------------------------------------------------------------------
enum InstrFlags : uint32_t {
    
    IF_NIL                  = 0,
   
    IF_REG_COMPLEMENT       = ( 1U << 1  ),
    IF_RES_NEGATE           = ( 1U << 2  ),
    IF_REG_ZERO_BEFORE      = ( 1U << 3  ),
    IF_RES_SIGN_EXT         = ( 1U << 4  ),
    IF_USE_SHAMT_REG        = ( 1U << 5  ),
    IF_ADR_UPDATE           = ( 1U << 6  ),
    
    IF_READ_ACCESS          = ( 1U << 10 ),
    IF_WRITE_ACCESS         = ( 1U << 11 ),
    IF_EXEC_ACCESS          = ( 1U << 12 ),
    
    IF_USE_IMM_VALUE        = ( 1U << 13 ),
    IF_USE_IMM_VAL_L         = ( 1U << 14 ),
    IF_USE_IMM_VAL_S        = ( 1U << 15 ),
    IF_USE_IMM_VAL_U        = ( 1U << 16 ),
    IF_USE_IMM_VAL_1        = ( 1U << 17 ),
    IF_USE_IMM_VAL_2        = ( 1U << 18 ),
    IF_USE_IMM_VAL_3        = ( 1U << 19 ),
    
    IF_DW_BYTE              = ( 1U << 20 ),
    IF_DW_HALF              = ( 1U << 21 ),
    IF_DW_WORD              = ( 1U << 22 ),
    IF_DW_DOUBLE            = ( 1U << 23 ),
    
    IF_CMP_EQ               = ( 1U << 24 ),
    IF_CMP_NE               = ( 1U << 25 ),
    IF_CMP_LT               = ( 1U << 26 ),
    IF_CMP_LE               = ( 1U << 27 ),
    IF_CMP_OD               = ( 1U << 28 ),
    IF_CMP_EV               = ( 1U << 29 ),
    IF_RV_30                = ( 1U << 30 ),
    IF_RV_31                = ( 1U << 30 )
};

//------------------------------------------------------------------------------------------------------------
// Expression value. The analysis of an expression results in a value. Depending on the expression type, the
// values are simple scalar values or a structured value, such as a register pair or virtual address.
//
//------------------------------------------------------------------------------------------------------------
struct Expr {
    
    TokTypeId typ;
    
    union {
        
        struct {    T64Word     numVal;                 };
        struct {    T64Word     adr;                    };
        struct {    char        strVal[ TOK_STR_SIZE ]; };
    };
};

//------------------------------------------------------------------------------------------------------------
// Global variables for the tokenizer.
//
//------------------------------------------------------------------------------------------------------------
char    tokenLine[ MAX_INPUT_LINE_SIZE ]    = { 0 };
int     currentLineLen                      = 0;
int     currentCharIndex                    = 0;
int     currentTokCharIndex                 = 0;
char    currentChar                         = ' ';
Token   currentToken;

//------------------------------------------------------------------------------------------------------------
// Forward declrations.
//
//------------------------------------------------------------------------------------------------------------
void parseExpr( Expr *rExpr );

//------------------------------------------------------------------------------------------------------------
// Helper functions for the tokenizer.
//
//------------------------------------------------------------------------------------------------------------
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
// The token lookup function. We just do a linear search.
//
//------------------------------------------------------------------------------------------------------------
int lookupToken( char *inputStr, const Token *tokTab ) {
    
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
    
    // ??? U and S ?
  
    while (( isalnum( currentChar )) || ( currentChar == '_' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
    }
    
    upshiftStr( identBuf );
    
    int index = lookupToken( identBuf, asmTokTab );
    
    if ( index == - 1 ) {
        
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
static inline void checkEOS( ) {
    
    if ( currentToken.tid != TOK_EOS ) throw( ERR_EXTRA_TOKEN_IN_STR );
}

static inline void acceptComma( ) {
    
    if ( currentToken.tid == TOK_COMMA ) nextToken( );
    else throw( ERR_EXPECTED_COMMA );
}

static inline void acceptLparen( ) {
    
    if ( currentToken.tid == TOK_LPAREN ) nextToken( );
    else throw( ERR_EXPECTED_LPAREN );
}

static inline void acceptRparen( ) {
    
    if ( currentToken.tid == TOK_RPAREN ) nextToken( );
    else throw( ERR_EXPECTED_RPAREN );
}

static inline bool isToken( TokId tid ) {
    
    return( currentToken.tid == tid );
}

static inline bool isTokenTyp( TokTypeId typ ) {
    
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
void parseFactor( Expr *rExpr ) {
    
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
        parseExpr( rExpr );
        if ( rExpr -> typ == TYP_GREG ) {
            
            rExpr -> typ    = TYP_ADR;
            rExpr -> numVal = currentToken.val;
        }
        
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
void parseTerm( Expr *rExpr ) {
    
    Expr lExpr;
    
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
void parseExpr( Expr *rExpr ) {
    
    Expr lExpr;
    
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
// Helper functions for instruction fields.
//
//------------------------------------------------------------------------------------------------------------
static inline bool isAligned( T64Word adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline bool isInRangeForBitField( T64Word val, int bitLen ) {
    
    int min = - ( 1 << (( bitLen - 1 ) % 64 ));
    int max = ( 1 << (( bitLen - 1 ) % 64 )) - 1;
    return (( val <= max ) && ( val >= min ));
}

static inline bool isInRangeForBitFieldU( T64Word val, int bitLen ) {
    
    int max = (( 1 << ( bitLen % 64 )) - 1 );
    return ( val <= max );
}

static inline void setBitField( uint32_t *word, int bitpos, int len, T64Word value ) {
    
    uint32_t mask = (( 1 << len ) - 1 ) << bitpos;
    *word = (( *word & ~mask ) | (( value << bitpos ) & mask ));
}

static inline void setInstrBit( uint32_t *word, int bitpos, bool value ) {
    
    uint32_t mask = 1 << bitpos;
    *word = (( *word & ~mask ) | (( value << bitpos ) & mask ));
}

static inline void setInstrField( uint32_t *instr, int bitpos, int len, T64Word value ) {
    
    if ( isInRangeForBitField( value, len )) setBitField( instr, bitpos, len, value );
    else throw( ERR_IMM_VAL_RANGE );
}

static inline void setInstrFieldU( uint32_t *instr, int bitpos, int len, T64Word value ) {
    
    if ( isInRangeForBitField( value, len )) setBitField( instr, bitpos, len, value );
    else throw( ERR_IMM_VAL_RANGE );
}

static inline void setInstrOpCode( uint32_t *instr, int opCodeGrp, int opCode ) {
    
    setInstrField( instr, 30, 2, opCodeGrp );
    setInstrField( instr, 26, 4, opCode );
}

static inline void setInstrRegR( uint32_t *instr, T64Word regId ) {
    
    if ( isInRangeForBitField( regId, 4 )) setBitField( instr, 22, 4, regId );
    else throw( ERR_REG_VAL_RANGE );
}

static inline void setInstrRegB( uint32_t *instr, T64Word regId ) {
    
    if ( isInRangeForBitField( regId, 4 )) setBitField( instr, 15, 4, regId );
    else throw( ERR_REG_VAL_RANGE );
}

static inline void setInstrRegA( uint32_t *instr, T64Word regId ) {
    
    if ( isInRangeForBitField( regId, 4 )) setBitField( instr, 9, 4, regId );
    else throw( ERR_REG_VAL_RANGE );
}

static inline void setInstrImm19( uint32_t *instr, T64Word val ) {
    
    if ( isInRangeForBitField( val, 4 )) setBitField( instr, 0, 19, val );
    else throw( ERR_IMM_VAL_RANGE );
}

static inline void setInstrImm15( uint32_t *instr, T64Word val ) {
    
    if ( isInRangeForBitField( val, 4 )) setBitField( instr, 0, 19, val );
    else throw( ERR_IMM_VAL_RANGE );
}

static inline void setInstrImm13( uint32_t *instr, T64Word val ) {
    
    if ( isInRangeForBitField( val, 4 )) setBitField( instr, 0, 19, val );
    else throw( ERR_IMM_VAL_RANGE );
}

bool hasDataWidthFlags( uint32_t instrFlags ) {
    
    return(( instrFlags & IF_DW_BYTE ) || ( instrFlags & IF_DW_HALF ) ||
           ( instrFlags & IF_DW_WORD ) || ( instrFlags & IF_DW_DOUBLE ));
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrOptions" will analyze the opCode option string. An opCode string is a sequence of characters.
// We will look at each character in the "name" and set the options for the particular instruction. There are
// also cases where the only option is a multi-character sequence. They cannot be in the same ".xxx" group.
// Currently only the CMP instruction is such a case. The assembler can handle multile ".xxx" sequences.
// Once we have all options seen, we check that there are no conflicting options where only one out of a
// flag group can be set.
//
//------------------------------------------------------------------------------------------------------------
uint32_t parseInstrOptions( uint32_t instrOpCode ) {
    
    if ( ! isToken( TOK_IDENT )) throw ( ERR_EXPECTED_INSTR_OPT );
    
    uint32_t instrFlags = IF_NIL;
    char    *optBuf     = currentToken.str;
  
    switch( instrOpCode ) {
            
        case OP_AND:
        case OP_OR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                else if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                else if ( optBuf[ i ] == 'C' ) instrFlags |= IF_REG_COMPLEMENT;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_XOR: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                else if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_CMP: {
            
            if ( strlen( optBuf ) == 1 ) {
                
                if      ( optBuf[ 0 ] == 'B' ) instrFlags |= IF_DW_BYTE;
                else if ( optBuf[ 0 ] == 'H' ) instrFlags |= IF_DW_HALF;
                else if ( optBuf[ 0 ] == 'W' ) instrFlags |= IF_DW_WORD;
                else if ( optBuf[ 0 ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            else {
                
                if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrFlags |= IF_CMP_EQ;
                else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrFlags |= IF_CMP_LT;
                else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrFlags |= IF_CMP_NE;
                else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrFlags |= IF_CMP_LE;
                else if ( strcmp( optBuf, ((char *) "OD" )) == 0 ) instrFlags |= IF_CMP_OD;
                else if ( strcmp( optBuf, ((char *) "EV" )) == 0 ) instrFlags |= IF_CMP_EV;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
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
                else if ( optBuf[ i ] == '1' ) instrFlags |= IF_USE_IMM_VAL_1;
                else if ( optBuf[ i ] == '2' ) instrFlags |= IF_USE_IMM_VAL_2;
                else if ( optBuf[ i ] == '3' ) instrFlags |= IF_USE_IMM_VAL_3;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_LDI: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'L' ) instrFlags |= IF_USE_IMM_VAL_L;
                else if ( optBuf[ i ] == 'S' ) instrFlags |= IF_USE_IMM_VAL_S;
                else if ( optBuf[ i ] == 'U' ) instrFlags |= IF_USE_IMM_VAL_U;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_LD:
        case OP_ST: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                else if ( optBuf[ i ] == 'M' ) instrFlags |= IF_ADR_UPDATE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
       
        case OP_CBR:
        case OP_MBR: {
            
            if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrFlags |= IF_CMP_EQ;
            else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrFlags |= IF_CMP_LT;
            else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrFlags |= IF_CMP_NE;
            else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrFlags |= IF_CMP_LE;
            else if ( strcmp( optBuf, ((char *) "OD" )) == 0 ) instrFlags |= IF_CMP_OD;
            else if ( strcmp( optBuf, ((char *) "EV" )) == 0 ) instrFlags |= IF_CMP_EV;
            else throw ( ERR_INVALID_INSTR_OPT );
            
        } break;
            
        case OP_PRB: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'R' ) instrFlags |= IF_READ_ACCESS;
                else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_WRITE_ACCESS;
                else if ( optBuf[ i ] == 'X' ) instrFlags |= IF_EXEC_ACCESS;
                else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        case OP_CHK: {
            
            for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                
                if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                else throw ( ERR_INVALID_INSTR_OPT );
            }
            
        } break;
            
        default: throw ( ERR_INVALID_INSTR_OPT );
    }
    
    int dwCount = 0;
    if ( instrFlags & IF_DW_BYTE     ) dwCount ++;
    if ( instrFlags & IF_DW_HALF     ) dwCount ++;
    if ( instrFlags & IF_DW_WORD     ) dwCount ++;
    if ( instrFlags & IF_DW_DOUBLE   ) dwCount ++;
    if ( dwCount > 1 ) throw ( ERR_INVALID_INSTR_OPT );
    
    int cmpCount    = 0;
    if ( instrFlags & IF_CMP_EQ ) cmpCount ++;
    if ( instrFlags & IF_CMP_LT ) cmpCount ++;
    if ( instrFlags & IF_CMP_NE ) cmpCount ++;
    if ( instrFlags & IF_CMP_LE ) cmpCount ++;
    if ( instrFlags & IF_CMP_OD ) cmpCount ++;
    if ( instrFlags & IF_CMP_EV ) cmpCount ++;
    if ( cmpCount > 1 ) throw ( ERR_INVALID_INSTR_OPT );
    
    nextToken( );
    
    return( instrFlags );
}

//------------------------------------------------------------------------------------------------------------
// Most instruction have the result register as an instruction field. We set the field and also return the
// register number.
//
//------------------------------------------------------------------------------------------------------------
T64Word parseRegR( uint32_t *instr ) {
    
    T64Word targetRegId = 0;
    
    if ( isTokenTyp( TYP_GREG )) {
        
        targetRegId = currentToken.val;
        setInstrField( instr, 22, 4, targetRegId );
        nextToken( );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    return ( targetRegId );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void instrSetCmpCode( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    int fieldPos = 0; // ??? always the same position ?
    
    if      ( instrFlags & IF_CMP_EQ )  setInstrField( instr, fieldPos, 3, IF_CMP_EQ );
    else if ( instrFlags & IF_CMP_LT )  setInstrField( instr, fieldPos, 3, IF_CMP_LT );
    else if ( instrFlags & IF_CMP_NE )  setInstrField( instr, fieldPos, 3, IF_CMP_NE );
    else if ( instrFlags & IF_CMP_LE )  setInstrField( instr, fieldPos, 3, IF_CMP_LE );
    else if ( instrFlags & IF_CMP_OD )  setInstrField( instr, fieldPos, 3, IF_CMP_OD );
    else if ( instrFlags & IF_CMP_EV )  setInstrField( instr, fieldPos, 3, IF_CMP_EV );
    else throw( ERR_EXPECTED_INSTR_OPT );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void setInstrDataWidth( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    int fieldPos = 0;
    
    if (( instrOpCode == OP_ADD ) || ( instrOpCode == OP_SUB ) || ( instrOpCode == OP_CMP ) ||
        ( instrOpCode == OP_AND ) || ( instrOpCode == OP_OR )  || ( instrOpCode == OP_XOR )) {
        
        fieldPos = 13;
    }
    else if (( instrOpCode == OP_LD) || ( instrOpCode == OP_ST ) || ( instrOpCode == OP_CHK )) {
        
        fieldPos = 20;
        
    }
    else ;
   
    if      ( instrFlags & IF_DW_BYTE   )   setInstrField( instr, fieldPos, 2, 0 );
    else if ( instrFlags & IF_DW_HALF   )   setInstrField( instr, fieldPos, 2, 1 );
    else if ( instrFlags & IF_DW_WORD   )   setInstrField( instr, fieldPos, 2, 2 );
    else if ( instrFlags & IF_DW_DOUBLE )   setInstrField( instr, fieldPos, 2, 3 );
    else throw( ERR_EXPECTED_INSTR_OPT );
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
void parseModeTypeInstr( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    uint8_t  targetRegId = 0;
    Expr     rExpr;
  
    targetRegId = parseRegR( instr );
   
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) {
        
        if ( isToken( TOK_EOS )) {
            
            setInstrOpCode( instr, OP_GRP_ALU, instrOpCode );
            setInstrImm19( instr, rExpr.numVal );
            
            if ( hasDataWidthFlags( instrFlags )) throw ( ERR_INVALID_INSTR_OPT );
        }
        else {
            
            setInstrOpCode( instr, OP_GRP_MEM, instrOpCode );
            setInstrDataWidth( instr, instrOpCode, instrFlags );
            setInstrImm13( instr, rExpr.numVal );
         
            parseExpr( &rExpr );
            if ( rExpr.typ == TYP_ADR ) setInstrRegB( instr, rExpr.numVal );
            else throw ( ERR_EXPECTED_ADR );
        }
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        if ( isToken( TOK_EOS )) {
            
            setInstrOpCode( instr, OP_GRP_ALU, instrOpCode );
            setInstrRegA( instr, targetRegId );
            setInstrRegB( instr, rExpr.numVal );
            
            if ( hasDataWidthFlags( instrFlags )) throw ( ERR_INVALID_INSTR_OPT );
        }
        else if ( isToken( TOK_COMMA )) {
            
            setInstrOpCode( instr, OP_GRP_ALU, instrOpCode );
            setInstrRegB( instr, rExpr.numVal );
            
            nextToken( );
            if ( isTokenTyp( TYP_GREG )) {
               
                setInstrRegA( instr, rExpr.numVal );
                nextToken( );
            }
            else throw ( ERR_EXPECTED_GENERAL_REG );
            
            if ( hasDataWidthFlags( instrFlags )) throw ( ERR_INVALID_INSTR_OPT );
        }
        else if ( isToken( TOK_LPAREN )) {
            
            setInstrOpCode( instr, OP_GRP_MEM, instrOpCode );
            setInstrDataWidth( instr, instrOpCode, instrFlags );
            
            parseExpr( &rExpr );
            if ( rExpr.typ == TYP_ADR )  setInstrRegB( instr, rExpr.numVal );
            else throw ( ERR_EXPECTED_ADR );
        }
    }
    else throw ( ERR_INVALID_INSTR_MODE );
    
    if ( instrOpCode == OP_AND ) {
        
        if ( instrFlags & IF_REG_COMPLEMENT ) setInstrBit( instr, 20, true );
        if ( instrFlags & IF_RES_NEGATE     ) setInstrBit( instr, 21, true );
    }
    else if (( instrOpCode == OP_OR ) || ( instrOpCode == OP_XOR )) {
        
        if ( instrFlags & IF_RES_NEGATE ) setInstrBit( instr, 21, true );
    }
    else if ( instrOpCode == OP_CMP ) {
        
        instrSetCmpCode( instr, instrOpCode, instrFlags );
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
// ??? the "pos" is variable !!!!
//------------------------------------------------------------------------------------------------------------
void parseInstrEXTR( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    Expr rExpr;
    *instr = 0;
    
    setInstrOpCode( instr, OP_GRP_ALU, OP_EXTR );
    parseRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) setInstrField( instr, 6, 6, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    if ( ! ( instrFlags & IF_USE_SHAMT_REG )) {
    
        acceptComma( );
        parseExpr( &rExpr );
        
        if ( rExpr.typ == TYP_NUM ) setInstrField( instr, 0, 6, rExpr.numVal );
        else throw ( ERR_EXPECTED_NUMERIC );
    }
    
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
void parseInstrDEP( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    Expr rExpr;
    
    *instr = 0;
    
    setInstrOpCode( instr, OP_GRP_ALU, OP_DEP );
    parseRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) {
        
        setInstrRegB( instr, rExpr.numVal );
        acceptComma( );
        parseExpr( &rExpr );
        
        if ( rExpr.typ == TYP_NUM ) {
            
            setInstrField( instr, 6, 6, rExpr.numVal );
            
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


#if 0


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

#endif

//------------------------------------------------------------------------------------------------------------
// "parseInstrLoadAndStore" will parse the load instructions family.
//
// <loadInstr>  [ "." <opt> ] <targetReg>   "," <sourceOperand>
// <storeInstr> [ "." <opt> ] <sourceReg>   "," <targetOperand>
//
// The syntax for the <operand> portion is either a
//
//      <ofs> "(" GR ")"
//      <GR>  "(" GR ")"
//
//
//------------------------------------------------------------------------------------------------------------
void parseInstrLoadAndStore( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
    Expr rExpr;
    
    parseRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        setInstrField( instr, 0, 15, rExpr.numVal );
        parseExpr( &rExpr );
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        if (( instrOpCode == OP_LDR ) || ( instrOpCode == OP_STC )) throw ( ERR_INVALID_INSTR_MODE );
        
        //  setBit( instr, 10 );
        //   setBitField( instr, 27, 4, rExpr.numVal );
        
        parseExpr( &rExpr );
    }
    
    if ( rExpr.typ == TYP_ADR ) {
        
        // setBitField( instr, 13, 2, 0 );
        // setBitField( instr, 31, 4, rExpr.numVal );
    }
    else throw ( ERR_EXPECTED_ADR );
}

#if 0

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
void parseInstrNop( uint32_t *instr, uint32_t instrOpCode, uint32_t instrFlags ) {
    
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
void setupTokenizer( char *inputStr ) {
    
    strcpy( tokenLine, inputStr );
    upshiftStr( tokenLine );
    
    currentLineLen          = 0;
    currentCharIndex        = 0;
    currentTokCharIndex     = 0;
    currentChar             = ' ';
    
    nextToken( );
}

void parseLine( char *inputStr, uint32_t *instr ) {
    
    uint32_t instrOpCode    = OP_NOP;
    uint32_t instrFlags     = IF_NIL;
    
    setupTokenizer( inputStr );
    *instr = 0;
   
    if ( isTokenTyp( TYP_OP_CODE )) {
        
        instrOpCode = currentToken.tid;
        
        nextToken( );
        while ( isToken( TOK_PERIOD )) {
            
            nextToken( );
            parseInstrOptions( instrOpCode );
        }
    
        switch( instrOpCode ) {
                
            case OP_NOP:    parseInstrNop( instr, instrOpCode, instrFlags );        break;
           
            case OP_ADD:
            case OP_SUB:
            case OP_AND:
            case OP_OR:
            case OP_XOR:
            case OP_CMP:    parseModeTypeInstr( instr, instrOpCode, instrFlags );   break;
                
            case OP_EXTR:   parseInstrEXTR( instr, instrOpCode, instrFlags );       break;
            case OP_DEP:    parseInstrDEP( instr, instrOpCode, instrFlags );        break;
                
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
