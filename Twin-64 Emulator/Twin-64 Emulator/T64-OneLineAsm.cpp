

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
    TYP_NUM                 = 4,        TYP_STR                 = 5,        TYP_ADR                 = 7,
    TYP_OP_CODE             = 8,        TYP_GREG                = 9,        TYP_CREG                = 10,
    TYP_PSW_PREG            = 11,
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
    
    TOK_GR_0                = 101,      TOK_GR_1                = 102,      TOK_GR_2                = 103,
    TOK_GR_3                = 104,      TOK_GR_4                = 105,      TOK_GR_5                = 106,
    TOK_GR_6                = 107,      TOK_GR_7                = 108,      TOK_GR_8                = 109,
    TOK_GR_9                = 110,      TOK_GR_10               = 111,      TOK_GR_11               = 112,
    TOK_GR_12               = 113,      TOK_GR_13               = 114,      TOK_GR_14               = 115,
    TOK_GR_15               = 116,
    
    TOK_CR_0                = 121,      TOK_CR_1                = 122,      TOK_CR_2                = 123,
    TOK_CR_3                = 124,      TOK_CR_4                = 125,      TOK_CR_5                = 126,
    TOK_CR_6                = 127,      TOK_CR_7                = 128,      TOK_CR_8                = 129,
    TOK_CR_9                = 130,      TOK_CR_10               = 131,      TOK_CR_11               = 132,
    TOK_CR_12               = 133,      TOK_CR_13               = 134,      TOK_CR_14               = 136,
    TOK_CR_15               = 137,
    
    //--------------------------------------------------------------------------------------------------------
    // OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    TOK_OP_NOP              = 300,
    
    TOK_OP_AND              = 301,      TOK_OP_AND_I            = 311,      TOK_OP_AND_M            = 321,
    TOK_OP_OR               = 302,      TOK_OP_OR_I             = 312,      TOK_OP_OR_M             = 322,
    TOK_OP_XOR              = 303,      TOK_OP_XOR_I            = 313,      TOK_OP_XOR_M            = 323,
    TOK_OP_ADD              = 304,      TOK_OP_ADD_I            = 314,      TOK_OP_ADD_M            = 324,
    TOK_OP_SUB              = 305,      TOK_OP_SUB_I            = 315,      TOK_OP_SUB_M            = 325,
    TOK_OP_CMP              = 306,      TOK_OP_CMP_I            = 316,      TOK_OP_CMP_M            = 326,
    
    TOK_OP_EXTR             = 331,      TOK_OP_DEP              = 332,      TOK_OP_DSR              = 333,
    TOK_OP_SHL1A            = 334,      TOK_OP_SHL2A            = 335,      TOK_OP_SHL3A            = 336,
    
    TOK_OP_LDI              = 341,      TOK_OP_ADDIL            = 342,      TOK_OP_LDO              = 343,
    TOK_OP_LD               = 345,      TOK_OP_LDR              = 346,
    TOK_OP_ST               = 347,      TOK_OP_STC              = 348,
    
    TOK_OP_B                = 351,      TOK_OP_BR               = 352,      TOK_OP_BV               = 353,
    TOK_OP_BB               = 355,      TOK_OP_CBR              = 356,      TOK_OP_MBR              = 357,
    
    TOK_OP_MFCR             = 361,      TOK_OP_MTCR             = 362,
    TOK_OP_RSM              = 363,      TOK_OP_SSM              = 364,
    TOK_OP_LPA              = 365,      TOK_LPAX                = 366,
    TOK_OP_PRB              = 367,
    TOK_OP_ITLB             = 371,      TOK_OP_PTLB             = 372,
    TOK_OP_PCA              = 373,      TOK_OP_FCA              = 374,
    
    TOK_OP_RFI              = 381,      TOK_OP_DIAG             = 382,
    TOK_OP_BRK              = 383,      TOK_OP_CHK              = 384,
    
    
    //--------------------------------------------------------------------------------------------------------
    // Synthetic OP Code Tokens.
    //
    //--------------------------------------------------------------------------------------------------------
    OP_SHL                  = 401,      OP_SHR                  = 402,
    OP_ASL                  = 403,      OP_ASR                  = 404,
    OP_ROR                  = 405,      OP_ROL                  = 406
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
// An instruction template consists of the instruction group bits ( 31,30 ), the op code family bits ( 29,
// 28, 27, 26 ) and the option or mode bits ( 21, 20, 19 ). The mode bits are for some instruction the
// default and could be changed during instruction parsing. From the defined constants we will build the
// instruction template which is stored for the opcode mnemoic in the token value field.
//
//------------------------------------------------------------------------------------------------------------
enum InstrTemplate : uint32_t {
    
    OPG_ALU         = ( 0U  << 30 ),
    OPG_MEM         = ( 1U  << 30 ),
    OPG_BR          = ( 2U  << 30 ),
    OPG_SYS         = ( 3U  << 30 ),
    
    OPF_ADD_OP      = ( 1U  << 26 ),
    OPF_SUB_OP      = ( 2U  << 26 ),
    OPF_AND_OP      = ( 3U  << 26 ),
    OPF_OR_OP       = ( 4U  << 26 ),
    OPF_XOR_OP      = ( 5U  << 26 ),
    OPF_CMP_OP      = ( 6U  << 26 ),
    OPF_BIT_OP      = ( 7U  << 26 ),
    OPF_SHA_OP      = ( 8U  << 26 ),
    OPF_IMM_OP      = ( 9U  << 26 ),
    OPF_LDO_OP      = ( 10U << 26 ),
    
    OPF_LD_OP       = ( 8U  << 26 ),
    OPF_ST_OP       = ( 9U  << 26 ),
    OPF_LDR_OP      = ( 10U << 26 ),
    OPF_STC_OP      = ( 11U << 26 ),
    
    OPF_B_OP        = ( 1U  << 26 ),
    OPF_BR_OP       = ( 2U  << 26 ),
    OPF_BB_OP       = ( 3U  << 26 ),
    OPF_CBR_OP      = ( 4U  << 26 ),
    OPF_MBR_OP      = ( 5U  << 26 ),
    
    OPF_MR_OP       = ( 1U  << 26 ),
    OPF_LPA_OP      = ( 2U  << 26 ),
    OPF_PRB_OP      = ( 3U  << 26 ),
    OPF_TLB_OP      = ( 4U  << 26 ),
    OPF_CA_OP       = ( 5U  << 26 ),
    OPF_MST_OP      = ( 6U  << 26 ),
    OPF_RFI_OP      = ( 7U  << 26 ),
    OPF_TRAP_OP     = ( 8U  << 26 ),
    OPF_DIAG_OP     = ( 8U  << 26 ),
    
    OPM_FLD_0       = ( 0U  << 19 ),
    OPM_FLD_1       = ( 1U  << 19 ),
    OPM_FLD_2       = ( 2U  << 19 ),
    OPM_FLD_3       = ( 3U  << 19 ),
    OPM_FLD_4       = ( 4U  << 19 ),
    OPM_FLD_5       = ( 5U  << 19 ),
    OPM_FLD_6       = ( 6U  << 19 ),
    OPM_FLD_7       = ( 7U  << 19 )
};

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
    IF_USE_IMM_VAL_L        = ( 1U << 14 ),
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
    IF_RV_30                = ( 1U << 30 ),
    IF_RV_31                = ( 1U << 31 )
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
    { .name = "R0",             .typ = TYP_GREG,            .tid = TOK_GR_0,        .val = 0            },
    { .name = "R1",             .typ = TYP_GREG,            .tid = TOK_GR_1,        .val = 1            },
    { .name = "R2",             .typ = TYP_GREG,            .tid = TOK_GR_2,        .val = 2            },
    { .name = "R3",             .typ = TYP_GREG,            .tid = TOK_GR_3,        .val = 3            },
    { .name = "R4",             .typ = TYP_GREG,            .tid = TOK_GR_4,        .val = 4            },
    { .name = "R5",             .typ = TYP_GREG,            .tid = TOK_GR_5,        .val = 5            },
    { .name = "R6",             .typ = TYP_GREG,            .tid = TOK_GR_6,        .val = 6            },
    { .name = "R7",             .typ = TYP_GREG,            .tid = TOK_GR_7,        .val = 7            },
    { .name = "R8",             .typ = TYP_GREG,            .tid = TOK_GR_8,        .val = 8            },
    { .name = "R9",             .typ = TYP_GREG,            .tid = TOK_GR_9,        .val = 9            },
    { .name = "R10",            .typ = TYP_GREG,            .tid = TOK_GR_10,       .val = 10           },
    { .name = "R11",            .typ = TYP_GREG,            .tid = TOK_GR_11,       .val = 11           },
    { .name = "R12",            .typ = TYP_GREG,            .tid = TOK_GR_12,       .val = 12           },
    { .name = "R13",            .typ = TYP_GREG,            .tid = TOK_GR_13,       .val = 13           },
    { .name = "R14",            .typ = TYP_GREG,            .tid = TOK_GR_14,       .val = 14           },
    { .name = "R15",            .typ = TYP_GREG,            .tid = TOK_GR_15,       .val = 15           },
    
    //--------------------------------------------------------------------------------------------------------
    // Control registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "C0",             .typ = TYP_CREG,            .tid = TOK_CR_0,        .val = 0            },
    { .name = "C1",             .typ = TYP_CREG,            .tid = TOK_CR_1,        .val = 1            },
    { .name = "C2",             .typ = TYP_CREG,            .tid = TOK_CR_2,        .val = 2            },
    { .name = "C3",             .typ = TYP_CREG,            .tid = TOK_CR_3,        .val = 3            },
    { .name = "C4",             .typ = TYP_CREG,            .tid = TOK_CR_4,        .val = 4            },
    { .name = "C5",             .typ = TYP_CREG,            .tid = TOK_CR_5,        .val = 5            },
    { .name = "C6",             .typ = TYP_CREG,            .tid = TOK_CR_6,        .val = 6            },
    { .name = "C7",             .typ = TYP_CREG,            .tid = TOK_CR_7,        .val = 7            },
    { .name = "C8",             .typ = TYP_CREG,            .tid = TOK_CR_8,        .val = 8            },
    { .name = "C9",             .typ = TYP_CREG,            .tid = TOK_CR_9,        .val = 9            },
    { .name = "C10",            .typ = TYP_CREG,            .tid = TOK_CR_10,       .val = 10           },
    { .name = "C11",            .typ = TYP_CREG,            .tid = TOK_CR_11,       .val = 11           },
    { .name = "C12",            .typ = TYP_CREG,            .tid = TOK_CR_12,       .val = 12           },
    { .name = "C13",            .typ = TYP_CREG,            .tid = TOK_CR_13,       .val = 13           },
    { .name = "C14",            .typ = TYP_CREG,            .tid = TOK_CR_14,       .val = 14           },
    { .name = "C15",            .typ = TYP_CREG,            .tid = TOK_CR_15,       .val = 15           },
    
    //--------------------------------------------------------------------------------------------------------
    // Runtime architcture register names for general registers.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "T0",             .typ = TYP_GREG,            .tid = TOK_GR_1,        .val =  1           },
    { .name = "T1",             .typ = TYP_GREG,            .tid = TOK_GR_2,        .val =  2           },
    { .name = "T2",             .typ = TYP_GREG,            .tid = TOK_GR_3,        .val =  3           },
    { .name = "T3",             .typ = TYP_GREG,            .tid = TOK_GR_4,        .val =  4           },
    { .name = "T4",             .typ = TYP_GREG,            .tid = TOK_GR_5,        .val =  5           },
    { .name = "T5",             .typ = TYP_GREG,            .tid = TOK_GR_6,        .val =  6           },
    { .name = "T6",             .typ = TYP_GREG,            .tid = TOK_GR_7,        .val =  7           },
    
    { .name = "ARG3",           .typ = TYP_GREG,            .tid = TOK_GR_8,        .val =  8           },
    { .name = "ARG2",           .typ = TYP_GREG,            .tid = TOK_GR_9,        .val =  9           },
    { .name = "ARG1",           .typ = TYP_GREG,            .tid = TOK_GR_10,       .val =  10          },
    { .name = "ARG0",           .typ = TYP_GREG,            .tid = TOK_GR_11,       .val =  11          },
    
    { .name = "RET3",           .typ = TYP_GREG,            .tid = TOK_GR_8,        .val =  8           },
    { .name = "RET2",           .typ = TYP_GREG,            .tid = TOK_GR_9,        .val =  9           },
    { .name = "RET1",           .typ = TYP_GREG,            .tid = TOK_GR_10,       .val =  10          },
    { .name = "RET0",           .typ = TYP_GREG,            .tid = TOK_GR_11,       .val =  11          },
    
    { .name = "DP",             .typ = TYP_GREG,            .tid = TOK_GR_13,       .val =  13          },
    { .name = "RL",             .typ = TYP_GREG,            .tid = TOK_GR_14,       .val =  14          },
    { .name = "SP",             .typ = TYP_GREG,            .tid = TOK_GR_15,       .val =  15          },
    
    { .name = "SAR",            .typ = TYP_GREG,            .tid = TOK_GR_1,        .val =  1           },
    
    //--------------------------------------------------------------------------------------------------------
    // Assembler mnemonics. Like all other tokens, we habe the name, the type and the token Id. In addition,
    // the ".val" field contains the initial instriuction mask with opCode group, opCode family and the
    // bits set in the first opütion field to further qualify the instruction.
    //
    //--------------------------------------------------------------------------------------------------------
    { .name = "ADD",   .typ = TYP_OP_CODE, .tid = TOK_OP_ADD,   .val = ( OPG_ALU | OPF_ADD_OP  | OPM_FLD_0 ) },
    { .name = "ADDI",  .typ = TYP_OP_CODE, .tid = TOK_OP_ADD_I, .val = ( OPG_ALU | OPF_ADD_OP  | OPM_FLD_1 ) },
    { .name = "ADDM",  .typ = TYP_OP_CODE, .tid = TOK_OP_ADD_M, .val = ( OPG_ALU | OPF_ADD_OP  | OPM_FLD_0 ) },
    
    { .name = "SUB",   .typ = TYP_OP_CODE, .tid = TOK_OP_SUB,   .val = ( OPG_ALU | OPF_SUB_OP  | OPM_FLD_0 ) },
    { .name = "SUBI",  .typ = TYP_OP_CODE, .tid = TOK_OP_SUB_I, .val = ( OPG_ALU | OPF_SUB_OP  | OPM_FLD_1 ) },
    { .name = "SUBM",  .typ = TYP_OP_CODE, .tid = TOK_OP_SUB_M, .val = ( OPG_ALU | OPF_SUB_OP  | OPM_FLD_0 ) },
    
    { .name = "AND",   .typ = TYP_OP_CODE, .tid = TOK_OP_AND,   .val = ( OPG_ALU | OPF_AND_OP  | OPM_FLD_0 ) },
    { .name = "ANDI",  .typ = TYP_OP_CODE, .tid = TOK_OP_AND_I, .val = ( OPG_ALU | OPF_AND_OP  | OPM_FLD_1 ) },
    { .name = "ANDM",  .typ = TYP_OP_CODE, .tid = TOK_OP_AND_M, .val = ( OPG_ALU | OPF_AND_OP  | OPM_FLD_0 ) },
    
    { .name = "OR",    .typ = TYP_OP_CODE, .tid = TOK_OP_OR,    .val = ( OPG_ALU | OPF_OR_OP   | OPM_FLD_0 ) },
    { .name = "ORI",   .typ = TYP_OP_CODE, .tid = TOK_OP_OR_I,  .val = ( OPG_ALU | OPF_OR_OP   | OPM_FLD_1 ) },
    { .name = "ORM",   .typ = TYP_OP_CODE, .tid = TOK_OP_OR_M,  .val = ( OPG_ALU | OPF_OR_OP   | OPM_FLD_0 ) },
    
    { .name = "XOR",   .typ = TYP_OP_CODE, .tid = TOK_OP_XOR,   .val = ( OPG_ALU | OPF_XOR_OP  | OPM_FLD_0 ) },
    { .name = "XORI",  .typ = TYP_OP_CODE, .tid = TOK_OP_XOR_I, .val = ( OPG_ALU | OPF_XOR_OP  | OPM_FLD_1 ) },
    { .name = "XORM",  .typ = TYP_OP_CODE, .tid = TOK_OP_XOR_M, .val = ( OPG_ALU | OPF_XOR_OP  | OPM_FLD_0 ) },
    
    { .name = "CMP",   .typ = TYP_OP_CODE, .tid = TOK_OP_CMP,   .val = ( OPG_ALU | OPF_CMP_OP  | OPM_FLD_0 ) },
    { .name = "CMPI",  .typ = TYP_OP_CODE, .tid = TOK_OP_CMP_I, .val = ( OPG_ALU | OPF_CMP_OP  | OPM_FLD_1 ) },
    { .name = "CMPM",  .typ = TYP_OP_CODE, .tid = TOK_OP_CMP_M, .val = ( OPG_ALU | OPF_CMP_OP  | OPM_FLD_0 ) },
    
    { .name = "EXTR",  .typ = TYP_OP_CODE, .tid = TOK_OP_EXTR,  .val = ( OPG_ALU | OPF_BIT_OP  | OPM_FLD_0 ) },
    { .name = "DEP",   .typ = TYP_OP_CODE, .tid = TOK_OP_DEP,   .val = ( OPG_ALU | OPF_BIT_OP  | OPM_FLD_1 ) },
    { .name = "DSR",   .typ = TYP_OP_CODE, .tid = TOK_OP_DSR,   .val = ( OPG_ALU | OPF_BIT_OP  | OPM_FLD_2 ) },
    
    { .name = "SHL1A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL1A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_0 ) },
    { .name = "SHL2A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL2A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_0 ) },
    { .name = "SHL3A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL3A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_0 ) },
    
    { .name = "SHR1A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL1A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_1 ) },
    { .name = "SHR2A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL2A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_1 ) },
    { .name = "SHR3A", .typ = TYP_OP_CODE, .tid = TOK_OP_SHL3A, .val = ( OPG_ALU | OPF_SHA_OP  | OPM_FLD_1 ) },
    
    { .name = "LDI",   .typ = TYP_OP_CODE, .tid = TOK_OP_LDI,   .val = ( OPG_ALU | OPF_IMM_OP  | OPM_FLD_0 ) },
    { .name = "ADDIL", .typ = TYP_OP_CODE, .tid = TOK_OP_ADDIL, .val = ( OPG_ALU | OPF_IMM_OP  | OPM_FLD_0 ) },
    { .name = "LDO",   .typ = TYP_OP_CODE, .tid = TOK_OP_LDO,   .val = ( OPG_ALU | OPF_LDO_OP  | OPM_FLD_0 ) },
    
    { .name = "LD",    .typ = TYP_OP_CODE, .tid = TOK_OP_LD,    .val = ( OPG_MEM | OPF_LD_OP   | OPM_FLD_0 ) },
    { .name = "LDX",   .typ = TYP_OP_CODE, .tid = TOK_OP_LD,    .val = ( OPG_MEM | OPF_LD_OP   | OPM_FLD_1 ) },
    { .name = "LDR",   .typ = TYP_OP_CODE, .tid = TOK_OP_LDR,   .val = ( OPG_MEM | OPF_LDR_OP  | OPM_FLD_0 ) },
    { .name = "ST",    .typ = TYP_OP_CODE, .tid = TOK_OP_ST,    .val = ( OPG_MEM | OPF_ST_OP   | OPM_FLD_0 ) },
    { .name = "STX",   .typ = TYP_OP_CODE, .tid = TOK_OP_ST,    .val = ( OPG_MEM | OPF_ST_OP   | OPM_FLD_1 ) },
    { .name = "STC",   .typ = TYP_OP_CODE, .tid = TOK_OP_STC,   .val = ( OPG_MEM | OPF_STC_OP  | OPM_FLD_0 ) },
   
    { .name = "B",     .typ = TYP_OP_CODE, .tid = TOK_OP_B,     .val = ( OPG_BR  | OPF_B_OP    | OPM_FLD_0 ) },
    { .name = "BR",    .typ = TYP_OP_CODE, .tid = TOK_OP_BR,    .val = ( OPG_BR  | OPF_BR_OP   | OPM_FLD_0 ) },
    { .name = "BV",    .typ = TYP_OP_CODE, .tid = TOK_OP_BV,    .val = ( OPG_BR  | OPF_BR_OP   | OPM_FLD_1 ) },
    { .name = "BB",    .typ = TYP_OP_CODE, .tid = TOK_OP_BB,    .val = ( OPG_BR  | OPF_BB_OP   | OPM_FLD_0 ) },
    { .name = "CBR",   .typ = TYP_OP_CODE, .tid = TOK_OP_CBR,   .val = ( OPG_BR  | OPF_CBR_OP  | OPM_FLD_0 ) },
    { .name = "MBR",   .typ = TYP_OP_CODE, .tid = TOK_OP_MBR,   .val = ( OPG_BR  | OPF_MBR_OP  | OPM_FLD_0 ) },
    { .name = "MFCR",  .typ = TYP_OP_CODE, .tid = TOK_OP_MFCR,  .val = ( OPG_SYS | OPF_MR_OP   | OPM_FLD_0 ) },
    { .name = "MTCR",  .typ = TYP_OP_CODE, .tid = TOK_OP_MTCR,  .val = ( OPG_SYS | OPF_MR_OP   | OPM_FLD_1 ) },
    
    { .name = "LPA",   .typ = TYP_OP_CODE, .tid = TOK_OP_LPA,   .val = ( OPG_SYS | OPF_LPA_OP  | OPM_FLD_0 ) },
    { .name = "LPAX",  .typ = TYP_OP_CODE, .tid = TOK_OP_LPA,   .val = ( OPG_SYS | OPF_LPA_OP  | OPM_FLD_1 ) },
    
    { .name = "PRBR",  .typ = TYP_OP_CODE, .tid = TOK_OP_PRB,   .val = ( OPG_SYS | OPF_PRB_OP  | OPM_FLD_0 ) },
    { .name = "PRBW",  .typ = TYP_OP_CODE, .tid = TOK_OP_PRB,   .val = ( OPG_SYS | OPF_PRB_OP  | OPM_FLD_1 ) },

    { .name = "ITLB",  .typ = TYP_OP_CODE, .tid = TOK_OP_ITLB,  .val = ( OPG_SYS | OPF_TLB_OP  | OPM_FLD_0 ) },
    { .name = "PTLB",  .typ = TYP_OP_CODE, .tid = TOK_OP_PTLB,  .val = ( OPG_SYS | OPF_TLB_OP  | OPM_FLD_1 ) },

    { .name = "PCA",   .typ = TYP_OP_CODE, .tid = TOK_OP_PCA,   .val = ( OPG_SYS | OPF_CA_OP   | OPM_FLD_0 ) },
    { .name = "FCA",   .typ = TYP_OP_CODE, .tid = TOK_OP_FCA,   .val = ( OPG_SYS | OPF_CA_OP   | OPM_FLD_1 ) },
        
    { .name = "RSM",   .typ = TYP_OP_CODE, .tid = TOK_OP_RSM,   .val = ( OPG_SYS | OPF_MST_OP  | OPM_FLD_0 ) },
    { .name = "SSM",   .typ = TYP_OP_CODE, .tid = TOK_OP_SSM,   .val = ( OPG_SYS | OPF_MST_OP  | OPM_FLD_1 ) },
    
    { .name = "CHK",   .typ = TYP_OP_CODE, .tid = TOK_OP_CHK,   .val = ( OPG_SYS | OPF_TRAP_OP | OPM_FLD_1 ) },
    { .name = "BRK",   .typ = TYP_OP_CODE, .tid = TOK_OP_BRK,   .val = ( OPG_SYS | OPF_TRAP_OP | OPM_FLD_1 ) },

    { .name = "RFI",   .typ = TYP_OP_CODE, .tid = TOK_OP_RFI,   .val = ( OPG_SYS | OPF_RFI_OP  | OPM_FLD_0 ) },
    { .name = "DIAG",  .typ = TYP_OP_CODE, .tid = TOK_OP_DIAG,  .val = ( OPG_SYS | OPF_DIAG_OP | OPM_FLD_0 ) },
    
    //--------------------------------------------------------------------------------------------------------
    // Synthetic instruction mnemonics.
    //
    //--------------------------------------------------------------------------------------------------------
    // ??? tbd
    
};

const int MAX_ASM_TOKEN_TAB = sizeof( asmTokTab ) / sizeof( Token );


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
                currentToken.val &= 0x00000000FFFFFC00;
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
                currentToken.val &= 0x00000000000003FF;
                return;
            }
            else throw ( ERR_INVALID_CHAR_IN_IDENT );
        }
    }
    else if (( currentChar == 'S' ) || ( currentChar == 's' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
        
        if ( currentChar == '%' ) {
            
            addChar( identBuf, sizeof( identBuf ), currentChar );
            nextChar( );
            
            if ( isdigit( currentChar )) {
                
                parseNum( );
                currentToken.val &= 0xFFFFF00000000;
                return;
            }
            else throw ( ERR_INVALID_CHAR_IN_IDENT );
        }
    }
    else if (( currentChar == 'U' ) || ( currentChar == 'u' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
        
        if ( currentChar == '%' ) {
            
            addChar( identBuf, sizeof( identBuf ), currentChar );
            nextChar( );
            
            if ( isdigit( currentChar )) {
                
                parseNum( );
                currentToken.val &= 0xFFF0000000000000;
                return;
            }
            else throw ( ERR_INVALID_CHAR_IN_IDENT );
        }
    }
   
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
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
// "parseFactor" parses the factor syntax part of an expression.
//
//      <factor> -> <number>            |
//                  <gregId>            |
//                  <cregId>            |
//                  "~" <factor>        |
//                  "(" <greg> ")"      |    // ???? not consistent....
//                  "(" <expr> ")"
//
//-----------------------------------------------------------------------------------------------------------
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

static inline T64Word extractField( T64Word arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
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

static inline bool hasDataWidthFlags( uint32_t instrFlags ) {
    
    return (( instrFlags & IF_DW_BYTE ) || ( instrFlags & IF_DW_HALF ) ||
            ( instrFlags & IF_DW_WORD ) || ( instrFlags & IF_DW_DOUBLE ));
}

static inline T64Word getInstrGroup( uint32_t instr ) {
    
    return ( extractField( instr, 30, 2 ));
}

static inline T64Word getInstrOp( uint32_t instr ) {
    
    return ( extractField( instr, 26, 4 ));
}


static inline void setInstrOpCode( uint32_t *instr, int instrOpToken ) {
    
    *instr = 0;
    
    switch ( instrOpToken ) {
            
            
    }
    
    // setInstrField( instr, 30, 2, opCodeGrp );
    // setInstrField( instr, 26, 4, opCode );
}




//------------------------------------------------------------------------------------------------------------
// "parseInstrOptions" will analyze the opCode option string. An opCode option string is a sequence of
// characters after the ".". We will look at each character in the "name" and set the options for the
// particular instruction. There are also options where the only option is a multi-character sequence. They
// cannot be in the same ".xxx" group. Currently only the CMP instruction is such a case. The assembler can
// handle multile ".xxx" sequences. Once we have all options seen, we check that there are no conflicting
// options where only one option out of an option group can be set.
//
//------------------------------------------------------------------------------------------------------------
uint32_t parseInstrOptions( uint32_t instrOpToken ) {
    
    uint32_t instrFlags  = IF_NIL;
    
    while ( isToken( TOK_PERIOD )) {
        
        nextToken( );
        if ( ! isToken( TOK_IDENT )) throw ( ERR_EXPECTED_INSTR_OPT );
        
        char        *optBuf     = currentToken.str;
        int         optStrLen   = (int) strlen( optBuf );
        
        switch( instrOpToken ) {
                
            case TOK_OP_AND:    case TOK_OP_AND_I:  case TOK_OP_AND_M: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                    else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                    else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                    else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                    else if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                    else if ( optBuf[ i ] == 'C' ) instrFlags |= IF_REG_COMPLEMENT;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_OR:     case TOK_OP_OR_I:   case TOK_OP_OR_M:
            case TOK_OP_XOR:    case TOK_OP_XOR_I:  case TOK_OP_XOR_M: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                    else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                    else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                    else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                    else if ( optBuf[ i ] == 'N' ) instrFlags |= IF_RES_NEGATE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_CMP:    case TOK_OP_CMP_I:   case TOK_OP_CMP_M: {
                
                if ( optStrLen == 1 ) {
                    
                    if      ( optBuf[ 0 ] == 'B' ) instrFlags |= IF_DW_BYTE;
                    else if ( optBuf[ 0 ] == 'H' ) instrFlags |= IF_DW_HALF;
                    else if ( optBuf[ 0 ] == 'W' ) instrFlags |= IF_DW_WORD;
                    else if ( optBuf[ 0 ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                else  if ( optStrLen == 2 ) {
                    
                    if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrFlags |= IF_CMP_EQ;
                    else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrFlags |= IF_CMP_LT;
                    else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrFlags |= IF_CMP_NE;
                    else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrFlags |= IF_CMP_LE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                else throw ( ERR_INVALID_INSTR_OPT );
                
            } break;
                
            case TOK_OP_EXTR: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'S' ) instrFlags |= IF_RES_SIGN_EXT;
                    else if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_DEP: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'Z' ) instrFlags |= IF_REG_ZERO_BEFORE;
                    else if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                    else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_DSR: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if ( optBuf[ i ] == 'A' ) instrFlags |= IF_USE_SHAMT_REG;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_SHL1A:  case TOK_OP_SHL2A:  case TOK_OP_SHL3A: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_LDI: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'L' ) instrFlags |= IF_USE_IMM_VAL_L;
                    else if ( optBuf[ i ] == 'S' ) instrFlags |= IF_USE_IMM_VAL_S;
                    else if ( optBuf[ i ] == 'U' ) instrFlags |= IF_USE_IMM_VAL_U;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_LD:
            case TOK_OP_ST: {
                
                for ( int i = 0; i < optStrLen; i ++ ) {
                    
                    if      ( optBuf[ i ] == 'B' ) instrFlags |= IF_DW_BYTE;
                    else if ( optBuf[ i ] == 'H' ) instrFlags |= IF_DW_HALF;
                    else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_DW_WORD;
                    else if ( optBuf[ i ] == 'D' ) instrFlags |= IF_DW_DOUBLE;
                    else if ( optBuf[ i ] == 'M' ) instrFlags |= IF_ADR_UPDATE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_CBR:
            case TOK_OP_MBR: {
                
                if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrFlags |= IF_CMP_EQ;
                else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrFlags |= IF_CMP_LT;
                else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrFlags |= IF_CMP_NE;
                else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrFlags |= IF_CMP_LE;
                else throw ( ERR_INVALID_INSTR_OPT );
                
            } break;
                
            case TOK_OP_PRB: {
                
                for ( int i = 0; i < strlen( optBuf ); i ++ ) {
                    
                    if      ( optBuf[ i ] == 'R' ) instrFlags |= IF_READ_ACCESS;
                    else if ( optBuf[ i ] == 'W' ) instrFlags |= IF_WRITE_ACCESS;
                    else if ( optBuf[ i ] == 'X' ) instrFlags |= IF_EXEC_ACCESS;
                    else if ( optBuf[ i ] == 'I' ) instrFlags |= IF_USE_IMM_VALUE;
                    else throw ( ERR_INVALID_INSTR_OPT );
                }
                
            } break;
                
            case TOK_OP_CHK: {
                
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
        if ( instrFlags & IF_DW_BYTE    ) dwCount ++;
        if ( instrFlags & IF_DW_HALF    ) dwCount ++;
        if ( instrFlags & IF_DW_WORD    ) dwCount ++;
        if ( instrFlags & IF_DW_DOUBLE  ) dwCount ++;
        if ( dwCount > 1 ) throw ( ERR_INVALID_INSTR_OPT );
        
        int cmpCount    = 0;
        if ( instrFlags & IF_CMP_EQ ) cmpCount ++;
        if ( instrFlags & IF_CMP_LT ) cmpCount ++;
        if ( instrFlags & IF_CMP_NE ) cmpCount ++;
        if ( instrFlags & IF_CMP_LE ) cmpCount ++;
        if ( cmpCount > 1 ) throw ( ERR_INVALID_INSTR_OPT );
        
        nextToken( );
    }
    
    return( instrFlags );
}

//------------------------------------------------------------------------------------------------------------
// Set the codition field for compare type instructions.
//
//------------------------------------------------------------------------------------------------------------
void instrSetCmpCode( uint32_t *instr, uint32_t instrFlags ) {
    
    int fieldPos = 20;
    
    if      ( instrFlags & IF_CMP_EQ )  setInstrField( instr, fieldPos, 2, 0 );
    else if ( instrFlags & IF_CMP_LT )  setInstrField( instr, fieldPos, 2, 1 );
    else if ( instrFlags & IF_CMP_NE )  setInstrField( instr, fieldPos, 2, 2 );
    else if ( instrFlags & IF_CMP_LE )  setInstrField( instr, fieldPos, 2, 3 );
    else throw( ERR_EXPECTED_INSTR_OPT );
}

//------------------------------------------------------------------------------------------------------------
// Set the data width field for memory access type instructions.
//
//------------------------------------------------------------------------------------------------------------
void setInstrDataWidth( uint32_t *instr, uint32_t instrFlags ) {
    
    int fieldPos = 13;
   
    if      ( instrFlags & IF_DW_BYTE   )   setInstrField( instr, fieldPos, 2, 0 );
    else if ( instrFlags & IF_DW_HALF   )   setInstrField( instr, fieldPos, 2, 1 );
    else if ( instrFlags & IF_DW_WORD   )   setInstrField( instr, fieldPos, 2, 2 );
    else if ( instrFlags & IF_DW_DOUBLE )   setInstrField( instr, fieldPos, 2, 3 );
    else throw( ERR_EXPECTED_INSTR_OPT );
}

//------------------------------------------------------------------------------------------------------------
// The "NOP" synthetic instruction emits the "BRK 0,0" instruction. Easy case.
//
//      NOP
//
//------------------------------------------------------------------------------------------------------------
void parseNopInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    nextToken( );
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseImmModeInstr" parses all instructions that have the immediate operand encoding. The instruction
// options have already been parsed and are available in the instrFlags variable. TThe syntax is as follows:
//
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg> "," <num>  -> Instruction group ALU
//
//-----------------------------------------------------------------------------------------------------------
void parseImmModeInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    if ( isToken( TOK_PERIOD )) instrFlags = parseInstrOptions( instrOpToken );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) setInstrImm19( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    if ( instrOpToken == TOK_OP_AND_I ) {
        
        if ( instrFlags & IF_REG_COMPLEMENT ) setInstrBit( instr, 20, true );
        if ( instrFlags & IF_RES_NEGATE     ) setInstrBit( instr, 21, true );
    }
    else if (( instrOpToken == TOK_OP_OR_I ) || ( instrOpToken == TOK_OP_XOR_I )) {
        
        if ( instrFlags & IF_RES_NEGATE ) setInstrBit( instr, 21, true );
    }
    else if ( instrOpToken == TOK_OP_CMP_I ) {
        
        instrSetCmpCode( instr, instrFlags );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseRegModeInstr" parses all instructions that have the register operand encoding. The instruction
// options have already been parsed and are available in the instrFlags variable. TThe syntax is as follows:
//
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg> "," <sourceRegB>  -> Instruction group ALU
//
//-----------------------------------------------------------------------------------------------------------
void parseRegModeInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    if ( isToken( TOK_PERIOD )) instrFlags = parseInstrOptions( instrOpToken );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegA( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    if ( instrOpToken == TOK_OP_AND_I ) {
        
        if ( instrFlags & IF_REG_COMPLEMENT ) setInstrBit( instr, 20, true );
        if ( instrFlags & IF_RES_NEGATE     ) setInstrBit( instr, 21, true );
    }
    else if (( instrOpToken == TOK_OP_OR_I ) || ( instrOpToken == TOK_OP_XOR_I )) {
        
        if ( instrFlags & IF_RES_NEGATE ) setInstrBit( instr, 21, true );
    }
    else if ( instrOpToken == TOK_OP_CMP_I ) {
        
        instrSetCmpCode( instr, instrFlags );
    }
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseAdrModeInstr" parses all instructions that have the memory address operand encoding. The instruction
// options have already been parsed and are available in the instrFlags variable. TThe syntax is as follows:
//
//      opCode [ "." <opt> ] <targetReg> "," [ <num> ]  "(" <baseReg> ")"     -> Instruction group MEM
//      opCode [ "." <opt> ] <targetReg> "," <indexReg> "(" <baseReg> ")"    -> Instruction group MEM
//
//-----------------------------------------------------------------------------------------------------------
void parseAdrModeInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr;
    uint32_t    instrFlags = IF_NIL;
  
    setInstrDataWidth( instr, instrFlags );
    
    nextToken( );
    if ( isToken( TOK_PERIOD )) instrFlags = parseInstrOptions( instrOpToken );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
    
        setInstrImm13( instr, rExpr.numVal );
        parseExpr( &rExpr );
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        setInstrRegA( instr, rExpr.numVal );
        parseExpr( &rExpr );
    }
    
    if ( rExpr.typ == TYP_ADR ) {
        
        if ( rExpr.typ == TYP_ADR ) setInstrRegB( instr, rExpr.numVal );
        else throw ( ERR_EXPECTED_ADR );
    }
    else throw ( ERR_EXPECTED_ADR );
 
    if ( instrOpToken == TOK_OP_AND_M ) {
        
        if ( instrFlags & IF_REG_COMPLEMENT ) setInstrBit( instr, 20, true );
        if ( instrFlags & IF_RES_NEGATE     ) setInstrBit( instr, 21, true );
    }
    else if (( instrOpToken == TOK_OP_OR_M ) || ( instrOpToken == TOK_OP_XOR_M )) {
        
        if ( instrFlags & IF_RES_NEGATE ) setInstrBit( instr, 21, true );
    }
    else if ( instrOpToken == TOK_OP_CMP_M ) {
        
        instrSetCmpCode( instr, instrFlags );
    }
             
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrEXTR" parses the extract instruction. The instruction has two basic formats. When the "A" bit
// is set, the position will be obtained from the shift amount control register. Otherwise it is encoded in
// the instruction.
//
//      EXTR [ ".S“ ]  <targetReg> "," <sourceReg> "," <pos> "," <len"
//      EXTR [ ".S" ]  <targetReg> "," <sourceReg> ", "SAR", <len"
//
//------------------------------------------------------------------------------------------------------------
void parseExtrInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr;
    uint32_t    instrFlags = IF_NIL;
   
    nextToken( );
    if ( isToken( TOK_PERIOD )) instrFlags = parseInstrOptions( instrOpToken );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) setInstrField( instr, 6, 6, rExpr.numVal );
    else if (( rExpr.typ == TYP_GREG ) && ( rExpr.numVal == 1 )) setInstrBit( instr, 13, true );
    else throw ( ERR_EXPECTED_NUMERIC );
   
    acceptComma( );
    parseExpr( &rExpr );
        
    if ( rExpr.typ == TYP_NUM ) setInstrField( instr, 0, 6, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    if ( instrFlags & IF_RES_SIGN_EXT ) setInstrBit( instr, 12, true );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// "parseInstrDEP" parses the deposit instruction. The instruction has zwo basic formats. When the "I" option
// is set, the value to deposti is an immediate value, else the data comes from a general register. When the
// "SAR" is specified instead of a bit position, the "A" bit is encoded in the instruction.
//
//      DEP [ ".“ Z/I ] <targetReg> "," <sourceReg> "," <pos> "," <len>"
//      DEP [ ".“ Z/I ] <targetReg> "," <sourceReg> "," "SAR" "," <len>"
//      DEP [ ".“ Z/I ] <targetReg> "," <val>,      "," <pos> "," <len>
//      DEP [ ".“ Z/I ] <targetReg> "," <val>       "," "SAR" "," <len>
//
//------------------------------------------------------------------------------------------------------------
void parseDepInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    if ( isToken( TOK_PERIOD )) instrFlags = parseInstrOptions( instrOpToken );
    
    if ( instrFlags & IF_REG_ZERO_BEFORE ) setInstrBit( instr, 12, true );
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if      ( rExpr.typ == TYP_GREG )   setInstrRegB( instr, rExpr.numVal );
    else if ( rExpr.typ == TYP_NUM )    setInstrField( instr, 15, 4, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if      (( rExpr.typ == TYP_GREG ) && ( rExpr.numVal == 1 )) setInstrBit( instr, 13, true );
    else if ( rExpr.typ == TYP_NUM )                             setInstrField( instr, 6, 6, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) setInstrField( instr, 0, 6, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The DSR instruction parses the double shift instruction. There are two flavors. If the "length operand is
// the "SAR" register, the "A" bit is encoded in teh instruction, other wise the instruction "len" field.
//
//      DSR <targetReg> "," <sourceRegA> "," <sourceRegB> "," <len>
//      DSR <targetReg> "," <sourceRegA> "," <sourceRegB> "," SAR
//
//------------------------------------------------------------------------------------------------------------
void parseDsrInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr  rExpr;
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG )   setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG )   setInstrRegA( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if      (( rExpr.typ == TYP_GREG ) && ( rExpr.numVal == 1 )) setInstrBit( instr, 13, true );
    else if ( rExpr.typ == TYP_NUM )                             setInstrField( instr, 6, 6, rExpr.numVal );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}

//------------------------------------------------------------------------------------------------------------
// The SHLA instruction performs a shift left of "B" by the instruction encided shift amount and adds the
// "A" register to it. If the ".I" optin is set, the RegA field is interpreted as a number.
//
//      SHLxA       <targetReg> "," <sourceRegB> "," <sourceRegA>
//      SHLxA ".I"  <targetReg> "," <sourceRegA> "," <val>
//
//------------------------------------------------------------------------------------------------------------
void parseInstrSHLA( uint32_t *instr, uint32_t instrOpToken, uint32_t instrFlags ) {
    
    Expr rExpr;
   
    nextToken( );
    if ( isToken( TOK_PERIOD )) parseInstrOptions( instrOpToken );
    
    // ??? set shamt...
    
    parseExpr( &rExpr );
    if ( isTokenTyp( TYP_GREG )) setInstrRegR( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG )   setInstrRegB( instr, rExpr.numVal );
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) {
        
    
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    checkEOS( );
}


#if 0



//------------------------------------------------------------------------------------------------------------
// The "LDIL" instruction loads the immediate value encoded in the instruction left shifted into "R". The
// "ADDIL" instruction will add the value encoded in the instruction left shifted to "R". The result is
// in R1.
//
//      LDI   <targetReg> "," <val>
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


#endif


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
    
    currentLineLen          = (int) strlen( tokenLine);
    currentCharIndex        = 0;
    currentTokCharIndex     = 0;
    currentChar             = ' ';
}

void parseLine( char *inputStr, uint32_t *instr ) {
    
    setupTokenizer( inputStr );
    nextToken( );
    
    if ( isTokenTyp( TYP_OP_CODE )) {
        
        uint32_t instrOpToken   = currentToken.tid;
        *instr                  = (uint32_t) currentToken.val;
       
        switch( instrOpToken ) {
                
            case TOK_OP_NOP:    parseNopInstr( instr, instrOpToken );           break;
                
            case TOK_OP_ADD:    parseRegModeInstr( instr, instrOpToken );       break;
            case TOK_OP_SUB:    parseRegModeInstr( instr, instrOpToken );       break;
            case TOK_OP_AND:    parseRegModeInstr( instr, instrOpToken );       break;
            case TOK_OP_OR:     parseRegModeInstr( instr, instrOpToken );       break;
            case TOK_OP_XOR:    parseRegModeInstr( instr, instrOpToken );       break;
            case TOK_OP_CMP:    parseRegModeInstr( instr, instrOpToken );       break;
           
            case TOK_OP_ADD_I:  parseImmModeInstr( instr, instrOpToken );       break;
            case TOK_OP_SUB_I:  parseImmModeInstr( instr, instrOpToken );       break;
            case TOK_OP_CMP_I:  parseImmModeInstr( instr, instrOpToken );       break;
            case TOK_OP_AND_I:  parseImmModeInstr( instr, instrOpToken );       break;
            case TOK_OP_OR_I:   parseImmModeInstr( instr, instrOpToken );       break;
            case TOK_OP_XOR_I:  parseImmModeInstr( instr, instrOpToken );       break;
                
            case TOK_OP_ADD_M:  parseAdrModeInstr( instr, instrOpToken );       break;
            case TOK_OP_SUB_M:  parseAdrModeInstr( instr, instrOpToken );       break;
            case TOK_OP_CMP_M:  parseAdrModeInstr( instr, instrOpToken );       break;
            case TOK_OP_AND_M:  parseAdrModeInstr( instr, instrOpToken );       break;
            case TOK_OP_OR_M:   parseAdrModeInstr( instr, instrOpToken );       break;
            case TOK_OP_XOR_M:  parseAdrModeInstr( instr, instrOpToken );       break;
                
            case TOK_OP_EXTR:   parseExtrInstr( instr, instrOpToken );          break;
            case TOK_OP_DEP:    parseDepInstr( instr, instrOpToken );           break;
            case TOK_OP_DSR:    parseDsrInstr( instr, instrOpToken );           break;
                
                /*
                 
                 case OP_CODE_LD:    case OP_CODE_LDW:   case OP_CODE_LDA:   case OP_CODE_LDR:
                 case OP_CODE_ST:    case OP_CODE_STW:   case OP_CODE_STA:   case OP_CODE_STC: {
                 
                 return( parseInstrLoadAndStore( instr, flags | TF_WORD_INSTR ));
                 }
                 
                
                 case OP_CODE_SHLA:      return( parseInstrSHLA( instr, flags ));
                 case OP_CODE_CMR:       return( parseInstrCMR( instr, flags ));
                 
                 
                 case OP_CODE_LDIL:
                 case OP_CODE_ADDIL:     return( parseInstrLDILandADDIL( instr, flags ));
                 
                 case OP_CODE_LDO:       return( parseInstrLDO( instr, flags ));
                 
                 case OP_CODE_B: return( parseInstrBandGATE( instr, flags ));
                 
                 case OP_CODE_BR:        return( parseInstrBR( instr, flags ));
                 case OP_CODE_BV:        return( parseInstrBV( instr, flags ));
                 case OP_CODE_BE:        return( parseInstrBE( instr, flags ));
                
                 case OP_CODE_CBR:
                    return( parseInstrCBRandCBRU( instr, flags ));
                 
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
        
        parseLine( inputStr, instr );
        return( NO_ERR );
    }
    catch ( ErrId errNum ) {
        
        *instr = 0;
        return( errNum );
    }
}

void testAsm( char *inputStr ) {
    
    uint32_t instr;
    parseAsmLine( inputStr, &instr );
}
