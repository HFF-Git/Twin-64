//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - One Line Assembler
//
//----------------------------------------------------------------------------------------
// The one line assembler assembles an instruction without further context. It is 
// intended to for testing instructions in the monitor. There is no symbol table or 
// any concept of assembling multiple instructions. The instruction to generate test
// is completely self sufficient. The parser is a straightforward recursive descendant
// parser, LL1 grammar. It uses the C++ try / catch to escape when an error is detected. 
// Considering that we only have one line to  parse, there is no need to implement a
// better parser error recovery method.
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - One Line Assembler
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
// Local namespace. These routines are not visible outside this source file.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// General constants.
//
//----------------------------------------------------------------------------------------
const int   MAX_INPUT_LINE_SIZE = 256;
const int   MAX_TOKEN_NAME_SIZE = 32;
const char  EOS_CHAR            = 0;

//----------------------------------------------------------------------------------------
// Assembler error codes.
//
//----------------------------------------------------------------------------------------
enum ErrId : int {
    
    NO_ERR                          = 0,
    
    ERR_EXTRA_TOKEN_IN_STR          = 10,
    ERR_INVALID_CHAR_IN_IDENT       = 11,
    ERR_INVALID_EXPR                = 12,
    ERR_INVALID_NUM                 = 13,
    ERR_INVALID_OP_CODE             = 14,
    ERR_INVALID_INSTR_MODE          = 15,
    ERR_INVALID_OFS                 = 16,
    ERR_INVALID_INSTR_OPT           = 17,
    
    ERR_EXPECTED_CLOSING_QUOTE      = 20,
    ERR_EXPECTED_NUMERIC            = 21,
    ERR_EXPECTED_COMMA              = 22,
    ERR_EXPECTED_LPAREN             = 23,
    ERR_EXPECTED_RPAREN             = 24,
    ERR_EXPECTED_STR                = 25,
    ERR_EXPECTED_OPCODE             = 26,
    ERR_EXPECTED_INSTR_OPT          = 27,
    ERR_EXPECTED_DIAG_OP            = 28,
    ERR_EXPECTED_GENERAL_REG        = 29,
    ERR_EXPECTED_POS_ARG            = 30,
    ERR_EXPECTED_LEN_ARG            = 31,
    ERR_EXPECTED_BR_OFS             = 32,
    ERR_EXPECTED_CONTROL_REG        = 33,
    ERR_EXPECTED_PRB_ARG            = 34,
    ERR_UNEXPECTED_EOS              = 35,
   
    ERR_EXPR_TYPE_MATCH             = 40,
    ERR_NUMERIC_OVERFLOW            = 41,
    ERR_IMM_VAL_RANGE               = 42,
    ERR_DUPLICATE_INSTR_OPT         = 43
};

//----------------------------------------------------------------------------------------
// Error message strings.
//
//----------------------------------------------------------------------------------------
struct ErrMsg {
    
    int msgId;
    char *msg;
};

const ErrMsg ErrMsgTable[ ] = {
    
    { NO_ERR,                       (char *) "No error" },
    
    { ERR_EXTRA_TOKEN_IN_STR,       (char *) "Extra tokens in input line" },
    { ERR_INVALID_CHAR_IN_IDENT,    (char *) "Invalid char in input line" },
    { ERR_INVALID_EXPR,             (char *) "Invalid expression" },
    { ERR_NUMERIC_OVERFLOW,         (char *) "Numeric overflow" },
    { ERR_INVALID_NUM,              (char *) "Invalid number" },
    { ERR_INVALID_OP_CODE,          (char *) "Invalid OpCode" },
    { ERR_INVALID_INSTR_MODE,       (char *) "Invalid instruction mode" },
    { ERR_INVALID_OFS,              (char *) "Invalid offset" },
    { ERR_INVALID_INSTR_OPT,        (char *) "Invalid instruction option" },
    
    { ERR_EXPECTED_CLOSING_QUOTE,   (char *) "Expected a closing quote" },
    { ERR_EXPECTED_NUMERIC,         (char *) "Expected a numeric value" },
    { ERR_EXPECTED_COMMA,           (char *) "Expected a comma" },
    { ERR_EXPECTED_LPAREN,          (char *) "Expected a left parenthesis" },
    { ERR_EXPECTED_RPAREN,          (char *) "Expected a right parenthesis" },
    { ERR_EXPECTED_STR,             (char *) "Expected a string" },
    { ERR_EXPECTED_OPCODE,          (char *) "Expected an opCode" },
    { ERR_EXPECTED_INSTR_OPT,       (char *) "Expected an instruction option" },
    { ERR_EXPECTED_DIAG_OP,         (char *) "Expected the DIAG opCode" },
    { ERR_EXPECTED_GENERAL_REG,     (char *) "Expected a general register" },
    { ERR_EXPECTED_POS_ARG,         (char *) "Expected a position argument" },
    { ERR_EXPECTED_LEN_ARG,         (char *) "Expected a length argument" },
    { ERR_EXPECTED_BR_OFS,          (char *) "Expected a branch offset" },
    { ERR_EXPECTED_CONTROL_REG,     (char *) "Expected a control register" },
    { ERR_EXPECTED_PRB_ARG,         (char *) "Expected the PRB argument" },
    { ERR_UNEXPECTED_EOS,           (char *) "Unexpected end of string" },   
   
    { ERR_EXPR_TYPE_MATCH ,         (char *) "Expression type mismatch" },
    { ERR_IMM_VAL_RANGE,            (char *) "Value range error " },
    { ERR_DUPLICATE_INSTR_OPT,      (char *) "Duplicate Instruction option " }
};

const int MAX_ERR_MSG_TAB = sizeof( ErrMsgTable ) / sizeof( ErrMsg );

//----------------------------------------------------------------------------------------
// Command line tokens and expression have a type.
//
//----------------------------------------------------------------------------------------
enum TokTypeId : int {
    
    TYP_NIL                 = 0,
    TYP_SYM                 = 1,        
    TYP_IDENT               = 2,        
    TYP_PREDEFINED_FUNC     = 3,
    TYP_NUM                 = 4,        
    TYP_STR                 = 5,       
    TYP_OP_CODE             = 6,
    TYP_GREG                = 7,        
    TYP_CREG                = 8,
};

//----------------------------------------------------------------------------------------
// Tokens are the labels for reserved words and symbols recognized by the 
// tokenizer objects. Tokens have a name, a token id, a token type and an 
// optional value with further data.
//
//----------------------------------------------------------------------------------------
enum TokId : int {
    
    //------------------------------------------------------------------------------------
    // General tokens and symbols.
    //------------------------------------------------------------------------------------
    TOK_NIL         = 0,    TOK_ERR         = 1,    TOK_EOS         = 2,        
    TOK_COMMA       = 3,    TOK_PERIOD      = 4,    TOK_LPAREN      = 5,
    TOK_RPAREN      = 6,    TOK_PLUS        = 8,    TOK_MINUS       = 9,        
    TOK_MULT        = 10,   TOK_DIV         = 11,   TOK_MOD         = 12,       
    TOK_REM         = 13,   TOK_NEG         = 14,   TOK_AND         = 15,       
    TOK_OR          = 16,   TOK_XOR         = 17,   TOK_IDENT       = 24,       
    TOK_NUM         = 25,   TOK_STR         = 26,
    
    //------------------------------------------------------------------------------------
    // General, Segment and Control Registers Tokens.
    //------------------------------------------------------------------------------------
    REG_SET         = 100,
    
    TOK_GR_0        = 101,  TOK_GR_1        = 102,  TOK_GR_2        = 103,
    TOK_GR_3        = 104,  TOK_GR_4        = 105,  TOK_GR_5        = 106,
    TOK_GR_6        = 107,  TOK_GR_7        = 108,  TOK_GR_8        = 109,
    TOK_GR_9        = 110,  TOK_GR_10       = 111,  TOK_GR_11       = 112,
    TOK_GR_12       = 113,  TOK_GR_13       = 114,  TOK_GR_14       = 115,
    TOK_GR_15       = 116,
    
    TOK_CR_0        = 121,  TOK_CR_1        = 122,  TOK_CR_2        = 123,
    TOK_CR_3        = 124,  TOK_CR_4        = 125,  TOK_CR_5        = 126,
    TOK_CR_6        = 127,  TOK_CR_7        = 128,  TOK_CR_8        = 129,
    TOK_CR_9        = 130,  TOK_CR_10       = 131,  TOK_CR_11       = 132,
    TOK_CR_12       = 133,  TOK_CR_13       = 134,  TOK_CR_14       = 136,
    TOK_CR_15       = 137,
    
    //------------------------------------------------------------------------------------
    // OP Code Tokens.
    //------------------------------------------------------------------------------------
    TOK_OP_NOP      = 300,
    
    TOK_OP_AND      = 301,  TOK_OP_OR       = 302,  TOK_OP_XOR      = 303,
    TOK_OP_ADD      = 304,  TOK_OP_SUB      = 305,  TOK_OP_CMP      = 306,
    
    TOK_OP_EXTR     = 311,  TOK_OP_DEP      = 312,  TOK_OP_DSR      = 313,
    TOK_OP_SHL1A    = 314,  TOK_OP_SHL2A    = 315,  TOK_OP_SHL3A    = 316,
    TOK_OP_SHR1A    = 317,  TOK_OP_SHR2A    = 318,  TOK_OP_SHR3A    = 319,
    
    TOK_OP_LDI      = 331,  TOK_OP_ADDIL    = 332,  TOK_OP_LDO      = 333,
    TOK_OP_LD       = 334,  TOK_OP_LDR      = 335,
    TOK_OP_ST       = 337,  TOK_OP_STC      = 338,
    
    TOK_OP_B        = 341,  TOK_OP_BR       = 342,  TOK_OP_BV       = 343,
    TOK_OP_BE       = 344,

    TOK_OP_BB       = 345,  TOK_OP_CBR      = 346,  TOK_OP_MBR      = 347,
    TOK_OP_ABR      = 348,
    
    TOK_OP_MFCR     = 351,  TOK_OP_MTCR     = 352,  TOK_OP_MFIA     = 353,
    TOK_OP_RSM      = 354,  TOK_OP_SSM      = 355,
    TOK_OP_LPA      = 356,  TOK_OP_PRB      = 357,
    
    TOK_OP_ITLB     = 361,  TOK_OP_PTLB     = 362,
    TOK_OP_PCA      = 363,  TOK_OP_FCA      = 364,
    
    TOK_OP_RFI      = 371,  TOK_OP_DIAG     = 372,  TOK_OP_TRAP     = 373,
    
    //------------------------------------------------------------------------------------
    // Synthetic OP Code Tokens.
    //
    // ??? tbd
    //------------------------------------------------------------------------------------
   
};

//----------------------------------------------------------------------------------------
// The one line assembler works the assembly line string processed as a list of 
// tokens. 
//
//----------------------------------------------------------------------------------------
struct Token {
    
    char        name[ MAX_TOKEN_NAME_SIZE ] = { };
    TokTypeId   typ                         = TYP_NIL;
    TokId       tid                         = TOK_NIL;
     T64Word    val                         = 0;  
};

//----------------------------------------------------------------------------------------
// An instruction template consists of the instruction group bits ( 31,30 ), the op 
// code family bits ( 29, 28, 27, 26 ) and the option or mode bits ( 21, 20, 19 ). The
// mode bits are for some instruction the default and could be changed during the 
// parsing process. From the defined constants we will build the instruction template 
// which is stored for the opcode mnemonic in the token value field. The values for the
// opcode group and the opcode families are in the "T64-Types" include file.
//
//----------------------------------------------------------------------------------------
enum InstrTemplate : uint32_t {
    
    OPG_ALU      = ( OPC_GRP_ALU  << 30 ),
    OPG_MEM      = ( OPC_GRP_MEM  << 30 ),
    OPG_BR       = ( OPC_GRP_BR   << 30 ),
    OPG_SYS      = ( OPC_GRP_SYS  << 30 ),
  
    OPF_ADD      = ( OPC_ADD    << 26 ),
    OPF_SUB      = ( OPC_SUB    << 26 ),
    OPF_AND      = ( OPC_AND    << 26 ),
    OPF_OR       = ( OPC_OR     << 26 ),
    OPF_XOR      = ( OPC_XOR    << 26 ),
    OPF_CMP      = ( OPC_CMP    << 26 ),
    OPF_BITOP    = ( OPC_BITOP  << 26 ),
    OPF_SHAOP    = ( OPC_SHAOP  << 26 ),
    OPF_IMMOP    = ( OPC_IMMOP  << 26 ),
    OPF_LDO      = ( OPC_LDO    << 26 ),
    
    OPF_LD       = ( OPC_LD     << 26 ),
    OPF_ST       = ( OPC_ST     << 26 ),
    OPF_LDR      = ( OPC_LDR    << 26 ),
    OPF_STC      = ( OPC_STC    << 26 ),
    
    OPF_B        = ( OPC_B      << 26 ),
    OPF_BR       = ( OPC_BR     << 26 ),
    OPF_BV       = ( OPC_BV     << 26 ),
    OPF_BE       = ( OPC_BE     << 26 ),
    OPF_BB       = ( OPC_BB     << 26 ),
    OPF_CBR      = ( OPC_CBR    << 26 ),
    OPF_MBR      = ( OPC_MBR    << 26 ),
    OPF_ABR      = ( OPC_ABR    << 26 ),
    
    OPF_MR       = ( OPC_MR     << 26 ),
    OPF_LPA      = ( OPC_LPA    << 26 ),
    OPF_PRB      = ( OPC_PRB    << 26 ),
    OPF_TLB      = ( OPC_TLB    << 26 ),
    OPF_CA       = ( OPC_CA     << 26 ),
    OPF_MST      = ( OPC_MST    << 26 ),
    OPF_RFI      = ( OPC_RFI    << 26 ),
    OPF_TRAP     = ( OPC_TRAP   << 26 ),
    OPF_DIAG     = ( OPC_DIAG   << 26 ),
    
    OPM_FLD_0    = ( 0U  << 19 ),
    OPM_FLD_1    = ( 1U  << 19 ),
    OPM_FLD_2    = ( 2U  << 19 ),
    OPM_FLD_3    = ( 3U  << 19 ),
    OPM_FLD_4    = ( 4U  << 19 ),
    OPM_FLD_5    = ( 5U  << 19 ),
    OPM_FLD_6    = ( 6U  << 19 ),
    OPM_FLD_7    = ( 7U  << 19 )
};

//----------------------------------------------------------------------------------------
// Instruction flags. They are used to keep track of instruction attributes used in 
// assembling the final instruction word. Examples are the data width encoded in the 
// opCode and the instruction mask. We define all options and masks for the respective
// instruction where they are valid.
//
//----------------------------------------------------------------------------------------
enum InstrFlags : uint32_t {
   
    IF_NIL      = 0,
    IF_B        = ( 1U << 1  ),
    IF_C        = ( 1U << 2  ),
    IF_D        = ( 1U << 3  ),
    IF_F        = ( 1U << 4  ),
    IF_G        = ( 1U << 5  ),
    IF_H        = ( 1U << 6  ),
    IF_I        = ( 1U << 7  ),
    IF_L        = ( 1U << 8  ),
    IF_M        = ( 1U << 9  ),
    IF_N        = ( 1U << 11 ),
    IF_S        = ( 1U << 13 ),
    IF_T        = ( 1U << 14 ),
    IF_U        = ( 1U << 15 ),
    IF_W        = ( 1U << 16 ),
    IF_Z        = ( 1U << 17 ),
    
    IF_EQ       = ( 1U << 24 ),
    IF_LT       = ( 1U << 25 ),
    IF_NE       = ( 1U << 26 ),
    IF_LE       = ( 1U << 27 ),
    IF_EV       = ( 1U << 28 ),
    IF_OD       = ( 1U << 29 ),
    
    IM_NIL      = 0,
    IM_ADD_OP   = ( IF_B | IF_H | IF_W | IF_D ),
    IM_SUB_OP   = ( IF_B | IF_H | IF_W | IF_D ),
    IM_AND_OP   = ( IF_B | IF_H | IF_W | IF_D | IF_N | IF_C ),
    IM_OR_OP    = ( IF_B | IF_H | IF_W | IF_D | IF_N ),
    IM_XOR_OP   = ( IF_B | IF_H | IF_W | IF_D | IF_N ),
    IM_CMP_OP   = ( IF_B | IF_H | IF_W | IF_D | IF_EQ | IF_LT | IF_NE | IF_LE ),
    IM_EXTR_OP  = ( IF_S ),
    IM_DEP_OP   = ( IF_Z | IF_I ),
    IM_SHLxA_OP = ( IF_I ),
    IM_SHRxA_OP = ( IF_I ),
    IM_LDI_OP   = ( IF_L | IF_S | IF_U ),
    IM_LD_OP    = ( IF_B | IF_H | IF_W | IF_D ),
    IM_ST_OP    = ( IF_B | IF_H | IF_W | IF_D ),
    IM_B_OP     = ( IF_G ),
    IM_BB_OP    = ( IF_T | IF_F ),
    IM_CBR_OP   = ( IF_EQ | IF_LT | IF_NE | IF_LE | IF_EV | IF_OD ),
    IM_MBR_OP   = ( IF_EQ | IF_LT | IF_NE | IF_LE | IF_EV | IF_OD ),
    IM_ABR_OP   = ( IF_EQ | IF_LT | IF_NE | IF_LE | IF_EV | IF_OD ),
    IM_CHK_OP   = ( IF_B | IF_H | IF_W | IF_D )
    
    // ??? synthetic instructions also need a mask constant.
};

//----------------------------------------------------------------------------------------
// The global token table or the one line assembler. All reserved words are allocated 
// in this table. Each entry has the token name, the token id, the token type id, i.e.
// its type, and a value associated with the token. The value allows for a constant 
// token. The parser can directly use the value in an expression.
//
//----------------------------------------------------------------------------------------
const Token AsmTokTab[ ] = {
    
    //------------------------------------------------------------------------------------
    // General registers.
    //
    //------------------------------------------------------------------------------------
    {   .name = "R0",   .typ = TYP_GREG,    .tid = TOK_GR_0,    .val = 0    },
    {   .name = "R1",   .typ = TYP_GREG,    .tid = TOK_GR_1,    .val = 1    },
    {   .name = "R2",   .typ = TYP_GREG,    .tid = TOK_GR_2,    .val = 2    },
    {   .name = "R3",   .typ = TYP_GREG,    .tid = TOK_GR_3,    .val = 3    },
    {   .name = "R4",   .typ = TYP_GREG,    .tid = TOK_GR_4,    .val = 4    },
    {   .name = "R5",   .typ = TYP_GREG,    .tid = TOK_GR_5,    .val = 5    },
    {   .name = "R6",   .typ = TYP_GREG,    .tid = TOK_GR_6,    .val = 6    },
    {   .name = "R7",   .typ = TYP_GREG,    .tid = TOK_GR_7,    .val = 7    },
    {   .name = "R8",   .typ = TYP_GREG,    .tid = TOK_GR_8,    .val = 8    },
    {   .name = "R9",   .typ = TYP_GREG,    .tid = TOK_GR_9,    .val = 9    },
    {   .name = "R10",  .typ = TYP_GREG,    .tid = TOK_GR_10,   .val = 10   },
    {   .name = "R11",  .typ = TYP_GREG,    .tid = TOK_GR_11,   .val = 11   },
    {   .name = "R12",  .typ = TYP_GREG,    .tid = TOK_GR_12,   .val = 12   },
    {   .name = "R13",  .typ = TYP_GREG,    .tid = TOK_GR_13,   .val = 13   },
    {   .name = "R14",  .typ = TYP_GREG,    .tid = TOK_GR_14,   .val = 14   },
    {   .name = "R15",  .typ = TYP_GREG,    .tid = TOK_GR_15,   .val = 15   },
    
    //------------------------------------------------------------------------------------
    // Control registers.
    //
    //------------------------------------------------------------------------------------
    { .name = "C0",     .typ = TYP_CREG,    .tid = TOK_CR_0,    .val = 0    },
    { .name = "C1",     .typ = TYP_CREG,    .tid = TOK_CR_1,    .val = 1    },
    { .name = "C2",     .typ = TYP_CREG,    .tid = TOK_CR_2,    .val = 2    },
    { .name = "C3",     .typ = TYP_CREG,    .tid = TOK_CR_3,    .val = 3    },
    { .name = "C4",     .typ = TYP_CREG,    .tid = TOK_CR_4,    .val = 4    },
    { .name = "C5",     .typ = TYP_CREG,    .tid = TOK_CR_5,    .val = 5    },
    { .name = "C6",     .typ = TYP_CREG,    .tid = TOK_CR_6,    .val = 6    },
    { .name = "C7",     .typ = TYP_CREG,    .tid = TOK_CR_7,    .val = 7    },
    { .name = "C8",     .typ = TYP_CREG,    .tid = TOK_CR_8,    .val = 8    },
    { .name = "C9",     .typ = TYP_CREG,    .tid = TOK_CR_9,    .val = 9    },
    { .name = "C10",    .typ = TYP_CREG,    .tid = TOK_CR_10,   .val = 10   },
    { .name = "C11",    .typ = TYP_CREG,    .tid = TOK_CR_11,   .val = 11   },
    { .name = "C12",    .typ = TYP_CREG,    .tid = TOK_CR_12,   .val = 12   },
    { .name = "C13",    .typ = TYP_CREG,    .tid = TOK_CR_13,   .val = 13   },
    { .name = "C14",    .typ = TYP_CREG,    .tid = TOK_CR_14,   .val = 14   },
    { .name = "C15",    .typ = TYP_CREG,    .tid = TOK_CR_15,   .val = 15   },

    //------------------------------------------------------------------------------------
    // Runtime architecture register names for general registers.
    //
    //------------------------------------------------------------------------------------
    { .name = "T0",     .typ = TYP_GREG,    .tid = TOK_GR_1,    .val =  1   },
    { .name = "T1",     .typ = TYP_GREG,    .tid = TOK_GR_2,    .val =  2   },
    { .name = "T2",     .typ = TYP_GREG,    .tid = TOK_GR_3,    .val =  3   },
    { .name = "T3",     .typ = TYP_GREG,    .tid = TOK_GR_4,    .val =  4   },
    { .name = "T4",     .typ = TYP_GREG,    .tid = TOK_GR_5,    .val =  5   },
    { .name = "T5",     .typ = TYP_GREG,    .tid = TOK_GR_6,    .val =  6   },
    { .name = "T6",     .typ = TYP_GREG,    .tid = TOK_GR_7,    .val =  7   },
    
    { .name = "ARG3",   .typ = TYP_GREG,    .tid = TOK_GR_8,    .val =  8   },
    { .name = "ARG2",   .typ = TYP_GREG,    .tid = TOK_GR_9,    .val =  9   },
    { .name = "ARG1",   .typ = TYP_GREG,    .tid = TOK_GR_10,   .val =  10  },
    { .name = "ARG0",   .typ = TYP_GREG,    .tid = TOK_GR_11,   .val =  11  },
    
    { .name = "RET3",   .typ = TYP_GREG,    .tid = TOK_GR_8,    .val =  8   },
    { .name = "RET2",   .typ = TYP_GREG,    .tid = TOK_GR_9,    .val =  9   },
    { .name = "RET1",   .typ = TYP_GREG,    .tid = TOK_GR_10,   .val =  10  },
    { .name = "RET0",   .typ = TYP_GREG,    .tid = TOK_GR_11,   .val =  11  },
    
    { .name = "DP",     .typ = TYP_GREG,    .tid = TOK_GR_13,   .val =  13  },
    { .name = "RL",     .typ = TYP_GREG,    .tid = TOK_GR_14,   .val =  14  },
    { .name = "SP",     .typ = TYP_GREG,    .tid = TOK_GR_15,   .val =  15  },
    
    { .name = "SAR",    .typ = TYP_GREG,    .tid = TOK_GR_1,    .val =  1   },
    
    //------------------------------------------------------------------------------------
    // Assembler mnemonics. Like all other tokens, we have the name, the type and the 
    // token Id. In addition, the ".val" field contains the initial instruction mask
    // with opCode group, opCode family and the bits set in the first option field to
    // further qualify the instruction.
    //
    //------------------------------------------------------------------------------------
    {   .name   = "ADD",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_ADD,   .val = ( OPG_ALU | OPF_ADD    | OPM_FLD_0 ) },

    {   .name   = "SUB",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SUB,   .val = ( OPG_ALU | OPF_SUB    | OPM_FLD_0 ) },

    {   .name   = "AND",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_AND,   .val = ( OPG_ALU | OPF_AND    | OPM_FLD_0 ) },

    {   .name   = "OR",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_OR,    .val = ( OPG_ALU | OPF_OR     | OPM_FLD_0 ) },
    
    {   .name   = "XOR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_XOR,   .val = ( OPG_ALU | OPF_XOR    | OPM_FLD_0 ) },
    
    {   .name   = "CMP",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_CMP,   .val = ( OPG_ALU | OPF_CMP    | OPM_FLD_0 ) },
    
    {   .name   = "EXTR",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_EXTR,  .val = ( OPG_ALU | OPF_BITOP  | OPM_FLD_0 ) },

    {   .name   = "DEP",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_DEP,   .val = ( OPG_ALU | OPF_BITOP  | OPM_FLD_1 ) },

    {   .name   = "DSR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_DSR,   .val = ( OPG_ALU | OPF_BITOP  | OPM_FLD_2 ) },
    
    {   .name   = "SHL1A",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SHL1A, .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_2 ) },

    {   .name   = "SHL2A",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SHL2A, .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_4 ) },

    {   .name   = "SHL3A",      .typ = TYP_OP_CODE, 
        .tid = TOK_OP_SHL3A,    .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_6 ) },

    {   .name   = "SHR1A",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SHR1A, .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_3 ) },

    {   .name   = "SHR2A",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SHR2A, .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_5 ) },

    {   .name   = "SHR3A",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_SHR3A, .val = ( OPG_ALU | OPF_SHAOP  | OPM_FLD_7 ) },
    
    {   .name   = "LDI",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_LDI,   .val = ( OPG_ALU | OPF_IMMOP  | OPM_FLD_0 ) },

    {   .name   = "ADDIL",      .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_ADDIL, .val = ( OPG_ALU | OPF_IMMOP  | OPM_FLD_0 ) },

    {   .name   = "LDO",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_LDO,   .val = ( OPG_ALU | OPF_LDO    | OPM_FLD_0 ) },
    
    {   .name   = "LD",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_LD,    .val = ( OPG_MEM | OPF_LD     | OPM_FLD_0 ) },

    {   .name   = "LDR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_LDR,   .val = ( OPG_MEM | OPF_LDR    | OPM_FLD_0 ) },

    {   .name   = "ST",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_ST,    .val = ( OPG_MEM | OPF_ST     | OPM_FLD_1 ) },

    {   .name   = "STC",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_STC,   .val = ( OPG_MEM | OPF_STC    | OPM_FLD_1 ) },
    
    {   .name   = "B",          .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_B,     .val = ( OPG_BR  | OPF_B      | OPM_FLD_0 ) },

    {   .name   = "BR",         .typ = TYP_OP_CODE, 
        .tid    =   TOK_OP_BR,  .val = ( OPG_BR  | OPF_BR     | OPM_FLD_0 ) },

    {   .name   = "BV",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_BV,    .val = ( OPG_BR  | OPF_BV     | OPM_FLD_0 ) },

    {   .name   = "BE",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_BE,    .val = ( OPG_BR  | OPF_BE     | OPM_FLD_0 ) },
    
    {   .name   = "BB",         .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_BB,    .val = ( OPG_BR  | OPF_BB     | OPM_FLD_0 ) },
    
    {   .name   = "CBR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_CBR,   .val = ( OPG_BR  | OPF_CBR    | OPM_FLD_0 ) },

    {   .name   = "MBR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_MBR,   .val = ( OPG_BR  | OPF_MBR    | OPM_FLD_0 ) },

    {   .name   = "ABR",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_ABR,   .val = ( OPG_BR  | OPF_ABR    | OPM_FLD_0 ) },
    
    {   .name   = "MFCR",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_MFCR,  .val = ( OPG_SYS | OPF_MR     | OPM_FLD_0 ) },

    {   .name   = "MTCR",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_MTCR,  .val = ( OPG_SYS | OPF_MR     | OPM_FLD_1 ) },

    {   .name   = "MFIA",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_MFIA,  .val = ( OPG_SYS | OPF_MR     | OPM_FLD_2 ) },
    
    {   .name   = "LPA",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_LPA,   .val = ( OPG_SYS | OPF_LPA    | OPM_FLD_0 ) },
    
    {   .name   = "PRB",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_PRB,   .val = ( OPG_SYS | OPF_PRB    | OPM_FLD_0 ) },

    {   .name   = "ITLB",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_ITLB,  .val = ( OPG_SYS | OPF_TLB    | OPM_FLD_0 ) },

    {   .name   = "PTLB",       .typ = TYP_OP_CODE,
        .tid    = TOK_OP_PTLB,  .val = ( OPG_SYS | OPF_TLB    | OPM_FLD_1 ) },
    
    {   .name   = "PCA",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_PCA,   .val = ( OPG_SYS | OPF_CA     | OPM_FLD_0 ) },

    {   .name   = "FCA",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_FCA,   .val = ( OPG_SYS | OPF_CA     | OPM_FLD_1 ) },
    
    {   .name   = "RSM",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_RSM,   .val = ( OPG_SYS | OPF_MST    | OPM_FLD_0 ) },

    {   .name   = "SSM",        .typ = TYP_OP_CODE,
        .tid    = TOK_OP_SSM,   .val = ( OPG_SYS | OPF_MST    | OPM_FLD_1 ) },
    
    {   .name   = "TRAP",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_TRAP,  .val = ( OPG_SYS | OPF_TRAP   | OPM_FLD_1 ) },
   
    {   .name   = "RFI",        .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_RFI,   .val = ( OPG_SYS | OPF_RFI    | OPM_FLD_0 ) },

    {   .name   = "DIAG",       .typ = TYP_OP_CODE, 
        .tid    = TOK_OP_DIAG,  .val = ( OPG_SYS | OPF_DIAG   | OPM_FLD_0 ) },
    
    //------------------------------------------------------------------------------------
    // Assembler synthetic mnemonics. They ar like the assembler mnemonics, except 
    // that they pre decode some bits settings in the option fields and reduce the 
    // ".<opt>" notation settings through meaningful instruction mnemonics.
    //
    // ??? tbd...
    //------------------------------------------------------------------------------------
   
};

const int MAX_ASM_TOKEN_TAB = sizeof( AsmTokTab ) / sizeof( Token );

//----------------------------------------------------------------------------------------
// Expression value. The analysis of an expression results in a value. Depending on 
// the expression type, the values are simple scalar values or a structured value, such
// as a register pair or virtual address.
//
//----------------------------------------------------------------------------------------
struct Expr {
    
    TokTypeId  typ;
    T64Word    val;  
};

const Expr INIT_EXPR = { .typ = TYP_NIL, .val = 0 }; 

//----------------------------------------------------------------------------------------
// Global variables for the tokenizer.
//
//----------------------------------------------------------------------------------------
int     lastErr                             = NO_ERR;
char    tokenLine[ MAX_INPUT_LINE_SIZE ]    = { 0 };
int     currentLineLen                      = 0;
int     currentCharIndex                    = 0;
int     currentTokCharIndex                 = 0;
char    currentChar                         = ' ';
Token   currentToken;

//----------------------------------------------------------------------------------------
// Forward declarations.
//
//----------------------------------------------------------------------------------------
void parseExpr( Expr *rExpr );

//----------------------------------------------------------------------------------------
// The token lookup function. We just do a linear search.
//
//----------------------------------------------------------------------------------------
int lookupToken( char *inputStr, const Token *tokTab ) {
    
    if (( strlen( inputStr ) == 0 ) || 
        ( strlen ( inputStr ) > MAX_TOKEN_NAME_SIZE )) return ( -1 );
    
    for ( int i = 0; i < MAX_ASM_TOKEN_TAB; i++  ) {
        
        if ( strcmp( inputStr, tokTab[ i ].name ) == 0 ) return ( i );
    }
    
    return ( -1 );
}

//----------------------------------------------------------------------------------------
// "nextChar" returns the next character from the token line string.
//
//----------------------------------------------------------------------------------------
void nextChar( ) {
    
    if ( currentCharIndex < currentLineLen ) {
        
        currentChar = tokenLine[ currentCharIndex ];
        currentCharIndex ++;
    }
    else currentChar = EOS_CHAR;
}

//----------------------------------------------------------------------------------------
// Signed 64-bit numeric operations with checking.
//
//----------------------------------------------------------------------------------------
T64Word addOp( Expr *a, Expr *b ) {

    if ( a -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( b -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( willAddOverflow( a -> val, b -> val )) throw( ERR_NUMERIC_OVERFLOW );
    return ( a -> val + b -> val );
}

T64Word subOp( Expr *a, Expr *b ) {

    if ( a -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( b -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( willSubOverflow( a -> val, b -> val )) throw( ERR_NUMERIC_OVERFLOW );
    return ( a -> val - b -> val );
}

T64Word multOp( Expr *a, Expr *b ) {

    if ( a -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( b -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( willMultOverflow( a -> val, b -> val )) throw( ERR_NUMERIC_OVERFLOW );
    return ( a -> val * b -> val );
}

T64Word divOp( Expr *a, Expr *b ) {

    if ( a -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( b -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( willDivOverflow( a -> val, b -> val )) throw( ERR_NUMERIC_OVERFLOW );
    return ( a -> val / b -> val );
}

T64Word modOp( Expr *a, Expr *b ) {

    if ( a -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( b -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    if ( willDivOverflow( a -> val, b -> val )) throw( ERR_NUMERIC_OVERFLOW );
    return ( a -> val % b -> val );
}

//----------------------------------------------------------------------------------------
// "parseNum" will parse a number. We accept decimals and hexadecimals. The numeric 
// string can also contain "_" characters for a better readable string. Hex numbers 
// start with a "0x", decimals just with the numeric digits.
//
//----------------------------------------------------------------------------------------
void parseNum( ) {
    
    currentToken.tid    = TOK_NUM;
    currentToken.typ    = TYP_NUM;
    currentToken.val  = 0;
    
    int     base        = 10;
    int     maxDigits   = 22;
    int     digits      = 0;
    T64Word tmpVal      = 0;
    
    if ( currentChar == '0' ) {
        
        nextChar( );
        if (( currentChar == 'x' ) || ( currentChar == 'X' )) {
            
            base        = 16;
            maxDigits   = 16;
            nextChar( );
        }
        else if ( !isdigit( currentChar )) {

            return;
        }
    }
    
    do {
        
        if ( currentChar == '_' ) {
            
            nextChar( );
            continue;
        }
        else {

            if ( isdigit( currentChar )) {

                tmpVal = ( tmpVal * base ) + currentChar - '0';
            }
            else if (( base == 16         ) && 
                     ( currentChar >= 'a' ) && 
                     ( currentChar <= 'f' )) {

                tmpVal = ( tmpVal * base ) + currentChar - 'a' + 10;            
            }
            else if (( base == 16         ) && 
                     ( currentChar >= 'A' ) && 
                     ( currentChar <= 'F' )) {

                tmpVal = ( tmpVal * base ) + currentChar - 'A' + 10;
            }      
            else throw ( ERR_INVALID_NUM );
            
            nextChar( );
            digits ++;
            
            if ( digits > maxDigits ) throw ( ERR_INVALID_NUM );
        }
    }
    while ( isxdigit( currentChar ) || ( currentChar == '_' ));

    currentToken.val = tmpVal;
}

//----------------------------------------------------------------------------------------
// "parseIdent" parses an identifier. It is a sequence of characters starting with an
// alpha character. An identifier found in the token table will assume the type and 
// value of the token found. Any other identifier is just an identifier symbol. There
// is one more thing. There are qualified constants that begin with a character followed 
// by a percent character, followed by a numeric value. During the character analysis,
// We first check for these kind of qualifiers and if found hand over to parse a number.
//
//----------------------------------------------------------------------------------------
void parseIdent( ) {
    
    currentToken.tid    = TOK_IDENT;
    currentToken.typ    = TYP_IDENT;
    currentToken.val    = 0;
    
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
                currentToken.val >>= 10;
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
                currentToken.val &= 0x000FFFFF00000000;
                currentToken.val >>= 32;
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
                currentToken.val >>= 52;
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
    
    int index = lookupToken( identBuf, AsmTokTab );
    
    if ( index == - 1 ) {
        
        strcpy( currentToken.name, identBuf );
        currentToken.typ = TYP_IDENT;
        currentToken.tid = TOK_IDENT;
        currentToken.val = 0;
    }
    else currentToken = AsmTokTab[ index ];
}

//----------------------------------------------------------------------------------------
// "nextToken" is the entry point to the lexer.
//
//----------------------------------------------------------------------------------------
void nextToken( ) {
    
    currentToken.name[ 0 ]  = 0;
    currentToken.typ        = TYP_NIL;
    currentToken.tid        = TOK_NIL;
    currentToken.val        = 0;
    
    while (( currentChar == ' ' ) || 
            ( currentChar == '\n' ) || 
            ( currentChar == '\r' )) nextChar( );
    
    currentTokCharIndex = currentCharIndex - 1;
    
    if ( isalpha( currentChar )) {
        
        parseIdent( );
    }
    else if ( isdigit( currentChar )) {
        
        parseNum( );
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
    else if ( currentChar == ';' ) {
        
        currentCharIndex    = currentLineLen;
        currentToken.typ    = TYP_NIL;
        currentToken.tid    = TOK_EOS;
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

//----------------------------------------------------------------------------------------
// Initialize the tokenizer and get the first token.
//
//----------------------------------------------------------------------------------------
void setupTokenizer( char *inputStr ) {
    
    strcpy( tokenLine, inputStr );
    upshiftStr( tokenLine );
    
    currentLineLen          = (int) strlen( tokenLine);
    currentCharIndex        = 0;
    currentTokCharIndex     = 0;
    currentChar             = ' ';
    
    nextToken( );
}

//----------------------------------------------------------------------------------------
// Parser helper functions.
//
//----------------------------------------------------------------------------------------
static inline bool isToken( TokId tid ) {
    
    return ( currentToken.tid == tid );
}

static inline bool isTokenTyp( TokTypeId typ ) {
    
    return ( currentToken.typ == typ );
}

static inline void acceptEOS( ) {
    
    if ( ! isToken( TOK_EOS )) throw ( ERR_EXTRA_TOKEN_IN_STR );
}

static inline void acceptComma( ) {
    
    if ( isToken( TOK_COMMA )) nextToken( );
    else throw ( ERR_EXPECTED_COMMA );
}

static inline void acceptLparen( ) {
    
    if ( isToken( TOK_LPAREN )) nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
}

static inline void acceptRparen( ) {
    
    if ( isToken( TOK_RPAREN )) nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//----------------------------------------------------------------------------------------
// "parseFactor" parses the factor syntax part of an expression.
//
//      <factor> -> <number>            |
//                  <gregId>            |
//                  <cregId>            |
//                  "~" <factor>        |
//                  "(" <expr> ")"
//
//----------------------------------------------------------------------------------------
void parseFactor( Expr *rExpr ) {
    
    rExpr -> typ  = TYP_NIL;
    rExpr -> val = 0;
    
    if ( isToken( TOK_NUM )) {
        
        rExpr -> typ    = TYP_NUM;
        rExpr -> val = currentToken.val;
        nextToken( );
    }
    else if ( isTokenTyp( TYP_GREG )) {
        
        rExpr -> typ    = TYP_GREG;
        rExpr -> val = currentToken.val;
        nextToken( );
    }
    else if ( isTokenTyp( TYP_CREG )) {
        
        rExpr -> typ    = TYP_CREG;
        rExpr -> val = currentToken.val;
        nextToken( );
    }
    else if ( isToken( TOK_NEG )) {
        
        parseFactor( rExpr );
        rExpr -> val = ~ rExpr -> val;
    }
    else if ( isToken( TOK_LPAREN )) {
        
        nextToken( );
        parseExpr( rExpr );
        acceptRparen( );
    }
    else throw ( ERR_INVALID_EXPR );
}

//----------------------------------------------------------------------------------------
// "parseTerm" parses the term syntax.
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
//----------------------------------------------------------------------------------------
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

            case TOK_MULT: rExpr -> val = multOp( rExpr, &lExpr );  break;
            case TOK_DIV:  rExpr -> val = divOp( rExpr, &lExpr );   break;
            case TOK_MOD:  rExpr -> val = modOp( rExpr, &lExpr );   break;
            case TOK_AND:  rExpr -> val = rExpr -> val & lExpr.val; break;
        }
    }
}

//----------------------------------------------------------------------------------------
// "parseExpr" parses the expression syntax.
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
//----------------------------------------------------------------------------------------
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

        if ( rExpr -> typ == TYP_NUM ) rExpr -> val = - rExpr -> val;
        else throw ( ERR_EXPECTED_NUMERIC );
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
   
            case TOK_PLUS:  rExpr -> val = addOp( rExpr, &lExpr );      break;
            case TOK_MINUS: rExpr -> val = subOp( rExpr, &lExpr );      break;
            case TOK_OR:    rExpr -> val = rExpr -> val | lExpr.val;    break;
            case TOK_XOR:   rExpr -> val = rExpr -> val ^ lExpr.val;    break;
        }
    }
}

//----------------------------------------------------------------------------------------
// Helper functions for instruction fields.
//
//----------------------------------------------------------------------------------------
inline void depositInstrFieldS( T64Instr *instr, int bitpos, int len, T64Word value ) {
    
    if ( isInRangeForInstrBitField( value, len )) 
        depositInstrField( instr, bitpos, len, value );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrFieldU( T64Instr *instr, int bitpos, int len, uint32_t value ) {
    
    if ( isInRangeForInstrBitFieldU( value, len )) 
        depositInstrField( instr, bitpos, len, value );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrImm13( T64Instr *instr, int val ) {
    
    if ( isInRangeForInstrBitField( val, 13 )) depositInstrField( instr, 0, 13, val );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrScaledImm13( T64Instr *instr, int val ) {
   
    val = val >> extractInstrField( *instr, 13, 2 );
    
    if ( isInRangeForInstrBitField( val, 13 )) depositInstrField( instr, 0, 13, val );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrImm15( T64Instr *instr, int val ) {
    
    if ( isInRangeForInstrBitField( val, 15 )) depositInstrField( instr, 0, 15, val );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrImm19( T64Instr *instr, int val ) {
    
    if ( isInRangeForInstrBitField( val, 19 )) depositInstrField( instr, 0, 19, val );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline void depositInstrImm20U( T64Instr *instr, uint32_t val ) {
    
    if ( isInRangeForInstrBitFieldU( val, 20 )) depositInstrField( instr, 0, 20, val );
    else throw ( ERR_IMM_VAL_RANGE );
}

inline bool hasDataWidthFlags( uint32_t instrFlags ) {
    
    return (( instrFlags & IF_B ) || ( instrFlags & IF_H ) ||
            ( instrFlags & IF_W ) || ( instrFlags & IF_D ));
}

inline void replaceInstrGroupField( T64Instr *instr, uint32_t instrMask ) {
    
    *instr &= 0x3FFFFFFF;
    *instr |= instrMask & 0xC0000000;
}

//----------------------------------------------------------------------------------------
// Set the condition field for compare type instructions based on the instruction flags.
//
//----------------------------------------------------------------------------------------
void setInstrCondField( uint32_t *instr, uint32_t instrFlags ) {
   
    if      ( instrFlags & IF_EQ )  depositInstrFieldU( instr, 20, 2, 0 );
    else if ( instrFlags & IF_LT )  depositInstrFieldU( instr, 20, 2, 1 );
    else if ( instrFlags & IF_NE )  depositInstrFieldU( instr, 20, 2, 2 );
    else if ( instrFlags & IF_LE )  depositInstrFieldU( instr, 20, 2, 3 );
}

//----------------------------------------------------------------------------------------
// Set the data width field for memory access type instructions based on the instruction
// flags. If no date width flags is set, we set the default, which is "D".
//
//----------------------------------------------------------------------------------------
void setInstrDwField( uint32_t *instr, uint32_t instrFlags ) {
    
    if (! hasDataWidthFlags( instrFlags )) instrFlags |= IF_D;
   
    if      ( instrFlags & IF_B )   depositInstrFieldU( instr, 13, 2, 0 );
    else if ( instrFlags & IF_H )   depositInstrFieldU( instr, 13, 2, 1 );
    else if ( instrFlags & IF_W )   depositInstrFieldU( instr, 13, 2, 2 );
    else if ( instrFlags & IF_D )   depositInstrFieldU( instr, 13, 2, 3 );
}

//----------------------------------------------------------------------------------------
// "parseInstrOptions" will analyze the opCode option string. An opCode option string
// is a sequence of characters after the ".". We will look at each char in the "name"
// and set the options for the particular instruction. There are also options where the
// option is a multi-character sequence. They cannot be in the same group with a sequence
// of individual individual characters. Currently only the CMP, CBR and MBR instructions
// are such cases.
//
// The assembler can handle multiple ".xxx" sequences. One could for example put each
// individual character in a separate ".x" location. Once we have all options seen, we
// check that there are no conflicting options where only one option out of an option
// group can be set.
//
//----------------------------------------------------------------------------------------
void parseInstrOptions( uint32_t *instrFlags, uint32_t instrOpToken ) {
    
    uint32_t instrMask = IM_NIL;
    
    while ( isToken( TOK_PERIOD )) {
        
        nextToken( );
        
        if ( isToken( TOK_OP_B )) {
            
            currentToken.typ = TYP_IDENT;
            currentToken.tid = TOK_IDENT;
            strncpy( currentToken.name, "B", 4 );
        }
        
        if ( ! isToken( TOK_IDENT )) throw ( ERR_EXPECTED_INSTR_OPT );
          
        char        *optBuf     = currentToken.name;
        int         optStrLen   = (int) strlen( optBuf );
        
        if      ( strcmp( optBuf, ((char *) "EQ" )) == 0 ) instrMask |= IF_EQ;
        else if ( strcmp( optBuf, ((char *) "LT" )) == 0 ) instrMask |= IF_LT;
        else if ( strcmp( optBuf, ((char *) "NE" )) == 0 ) instrMask |= IF_NE;
        else if ( strcmp( optBuf, ((char *) "LE" )) == 0 ) instrMask |= IF_LE;
            
        else {
            
            for ( int i = 0; i < optStrLen; i ++ ) {
                
                switch ( optBuf[ i ] ) {
                 
                    case 'B': instrMask = instrMask |= IF_B; break;
                    case 'C': instrMask = instrMask |= IF_C; break;
                    case 'D': instrMask = instrMask |= IF_D; break;
                    case 'F': instrMask = instrMask |= IF_F; break;
                    case 'G': instrMask = instrMask |= IF_G; break;
                    case 'H': instrMask = instrMask |= IF_H; break;
                    case 'I': instrMask = instrMask |= IF_I; break;
                    case 'L': instrMask = instrMask |= IF_L; break;
                    case 'M': instrMask = instrMask |= IF_M; break;
                    case 'N': instrMask = instrMask |= IF_N; break;
                    case 'S': instrMask = instrMask |= IF_S; break;
                    case 'T': instrMask = instrMask |= IF_T; break;
                    case 'U': instrMask = instrMask |= IF_U; break;
                    case 'W': instrMask = instrMask |= IF_W; break;
                    case 'Z': instrMask = instrMask |= IF_Z; break;
                    default: throw ( ERR_INVALID_INSTR_OPT );
                }
            }
        }
        
        int cnt = 0;
        if ( instrMask & IF_B   ) cnt ++;
        if ( instrMask & IF_H   ) cnt ++;
        if ( instrMask & IF_W   ) cnt ++;
        if ( instrMask & IF_D   ) cnt ++;
        if ( cnt >  1 ) throw ( ERR_DUPLICATE_INSTR_OPT );
        
        cnt = 0;
        if ( instrMask & IF_EQ ) cnt ++;
        if ( instrMask & IF_LT ) cnt ++;
        if ( instrMask & IF_NE ) cnt ++;
        if ( instrMask & IF_LE ) cnt ++;
        if ( cnt > 1 ) throw ( ERR_DUPLICATE_INSTR_OPT );
        
        cnt = 0;
        if ( instrMask & IF_T ) cnt ++;
        if ( instrMask & IF_F ) cnt ++;
        if ( cnt > 1 ) throw ( ERR_DUPLICATE_INSTR_OPT );
        
        cnt = 0;
        if ( instrMask & IF_L ) cnt ++;
        if ( instrMask & IF_S ) cnt ++;
        if ( instrMask & IF_U ) cnt ++;
        if ( cnt > 1 ) throw ( ERR_DUPLICATE_INSTR_OPT );
        
        nextToken( );
    }
    
    *instrFlags = instrMask;
}

//----------------------------------------------------------------------------------------
// The following routines parse a general register and store the register ID in the 
// respective instruction field.
//
//----------------------------------------------------------------------------------------
void acceptRegR( uint32_t *instr ) {
    
    Expr rExpr = INIT_EXPR;
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) depositInstrRegR( instr, (uint32_t) rExpr.val );
}

void acceptRegA( uint32_t *instr ) {

    Expr rExpr = INIT_EXPR;

    parseExpr( &rExpr );
    if (  rExpr.typ == TYP_GREG ) depositInstrRegA( instr, (uint32_t) rExpr.val );
    else throw ( ERR_EXPECTED_GENERAL_REG );
}

void acceptRegB( uint32_t *instr ) {

    Expr rExpr = INIT_EXPR;

    parseExpr( &rExpr );
    if (  rExpr.typ == TYP_GREG ) depositInstrRegB( instr, (uint32_t) rExpr.val );
    else throw ( ERR_EXPECTED_GENERAL_REG );
}

void acceptCregB( uint32_t *instr ) {

    Expr rExpr = INIT_EXPR;

    parseExpr( &rExpr );
    if (  rExpr.typ == TYP_CREG ) depositInstrRegB( instr, (uint32_t) rExpr.val );
    else throw ( ERR_EXPECTED_CONTROL_REG );
}

//----------------------------------------------------------------------------------------
// The "NOP" instruction. Easy case.
//
//      NOP
//
//----------------------------------------------------------------------------------------
void parseNopInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    nextToken( );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseModeTypeInstr" parses all instructions of type "mode" based on the syntax, 
// which will determine the exact instruction layout and option setting. 
//
// Instruction group ALU syntax:
//
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg> "," <num>          
//      opCode [ "." <opt> ] <targetReg> "," <sourceReg> "," <sourceRegB> 
//
// Instruction group MEM syntax:
//
//      opCode [ "." <opt> ] <targetReg> "," [ <num> ]  "(" <baseReg> ")"  
//      opCode [ "." <opt> ] <targetReg> "," <indexReg> "(" <baseReg> ")"  
//
//----------------------------------------------------------------------------------------
void parseModeTypeInstr( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    
    if ((( instrOpToken == TOK_OP_ADD ) && ( instrFlags & ~IM_ADD_OP )) ||
        (( instrOpToken == TOK_OP_SUB ) && ( instrFlags & ~IM_SUB_OP )) ||
        (( instrOpToken == TOK_OP_AND ) && ( instrFlags & ~IM_AND_OP )) ||
        (( instrOpToken == TOK_OP_OR  ) && ( instrFlags & ~IM_OR_OP  )) ||
        (( instrOpToken == TOK_OP_XOR ) && ( instrFlags & ~IM_XOR_OP )) ||
        (( instrOpToken == TOK_OP_CMP ) && ( instrFlags & ~IM_CMP_OP ))) {
            
        throw ( ERR_INVALID_INSTR_OPT );
    }
    
    acceptRegR( instr );
    acceptComma( );

    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
      
        replaceInstrGroupField( instr, OPG_MEM );
        setInstrDwField( instr, instrFlags );
        depositInstrScaledImm13( instr, (uint32_t) rExpr.val );
        
        acceptLparen( );
        acceptRegB( instr );        
        acceptRparen( );
        acceptEOS( );
    }
    else if ( rExpr.typ == TYP_GREG ) {
        
        if ( isToken( TOK_COMMA )) {
            
            if ( hasDataWidthFlags( instrFlags )) throw ( ERR_INVALID_INSTR_MODE );
            
            int tmpRegId = (int) rExpr.val;
            
            nextToken( );
            parseExpr( &rExpr );
            if ( rExpr.typ == TYP_NUM ) {
                
                depositInstrBit( instr, 19, true );
                depositInstrRegB( instr, tmpRegId );
                depositInstrImm15( instr, (uint32_t) rExpr.val );
            }
            else if ( rExpr.typ == TYP_GREG ) {
                
                depositInstrRegB( instr, tmpRegId );
                depositInstrRegA( instr, (uint32_t) rExpr.val );
            }
            else throw ( ERR_EXPECTED_GENERAL_REG );
        
            acceptEOS( );
        }
        else if ( isToken( TOK_LPAREN )) {

            replaceInstrGroupField( instr, OPG_MEM );
            setInstrDwField( instr, instrFlags );
            depositInstrBit( instr, 19, true );
            depositInstrRegA( instr, (uint32_t) rExpr.val );
            
            nextToken( );
            acceptRegB( instr );
            acceptRparen( );
            acceptEOS( );
        }
        else throw ( ERR_EXPECTED_COMMA );
    }
    
    if (( instrOpToken == TOK_OP_AND ) || ( instrOpToken == TOK_OP_OR )){
        
        if ( instrFlags & IF_C ) depositInstrBit( instr, 20, true );
        if ( instrFlags & IF_N ) depositInstrBit( instr, 21, true );
    }
    else if ( instrOpToken == TOK_OP_XOR ) {
        
        if ( instrFlags & IF_N ) depositInstrBit( instr, 21, true );
    }
    else if ( instrOpToken == TOK_OP_CMP ) {
        
        setInstrCondField( instr, instrFlags );
    }
}

//----------------------------------------------------------------------------------------
// "parseInstrEXTR" parses the extract instruction. The instruction has two basic 
// formats. When the "A" bit is set, the position will be obtained from the shift 
// amount control register. Otherwise, the position it is encoded in the instruction.
//
//      EXTR [ ".S“ ]  <targetReg> "," <sourceReg> "," <pos> "," <len"
//      EXTR [ ".S" ]  <targetReg> "," <sourceReg> ", "SAR", <len"
//
//----------------------------------------------------------------------------------------
void parseInstrEXTR( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_EXTR ) && ( instrFlags & ~IM_EXTR_OP )) {
        
        throw ( ERR_INVALID_INSTR_OPT );
    }
    
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr ); 
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
       depositInstrFieldS( instr, 6, 6, (uint32_t) rExpr.val );
    }
    else if (( rExpr.typ == TYP_GREG ) && ( rExpr.val == 1 )) {
        
        depositInstrBit( instr, 13, true );
    }
    else throw ( ERR_EXPECTED_POS_ARG );
    
    acceptComma( );

    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) { 
        
        depositInstrFieldS( instr, 0, 6, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_LEN_ARG );
    
    if ( instrFlags & IF_S ) depositInstrBit( instr, 12, true );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrDEP" parses the deposit instruction. The instruction has two basic 
// formats. When the "I" option is set, the value to deposit is an immediate value, 
// else the data comes from a general register. When "SAR" is specified instead of a 
// bit position, the "A" bit is encoded in the instruction. When the value to deposit 
// is a numeric value, the "I" bit is set.
//
//      DEP [ ".“ Z ] <targetReg> "," <sourceReg> "," <pos> "," <len>"
//      DEP [ ".“ Z ] <targetReg> "," <sourceReg> "," "SAR" "," <len>"
//      DEP [ ".“ Z ] <targetReg> "," <val>,      "," <pos> "," <len>
//      DEP [ ".“ Z ] <targetReg> "," <val>       "," "SAR" "," <len>
//
//----------------------------------------------------------------------------------------
void parseInstrDEP( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr      = INIT_EXPR;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_DEP ) && ( instrFlags & ~IM_DEP_OP )) {
        
        throw ( ERR_INVALID_INSTR_OPT );
    }
    
    if ( instrFlags & IF_Z ) depositInstrBit( instr, 12, true );
    
    acceptRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) {
        
        depositInstrRegB( instr, (uint32_t) rExpr.val );
    }
    else if ( rExpr.typ == TYP_NUM )    {
        
        depositInstrFieldS( instr, 15, 4, (uint32_t) rExpr.val );
        depositInstrBit( instr, 11, true );
    }
    else throw ( ERR_EXPECTED_POS_ARG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if (( rExpr.typ == TYP_GREG ) && ( rExpr.val == 1 )) {
        
        depositInstrBit( instr, 13, true );
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrFieldS( instr, 6, 6, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_LEN_ARG );
    
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) { 
        
        depositInstrFieldS( instr, 0, 6, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrDSR" parses the double shift instruction. There are two flavors. If the
// "length operand is the "SAR" register, the "A" bit is encoded in the instruction, 
// other wise the instruction "len" field.
//
//      DSR <targetReg> "," <sourceRegA> "," <sourceRegB> "," <len>
//      DSR <targetReg> "," <sourceRegA> "," <sourceRegB> "," SAR
//
//----------------------------------------------------------------------------------------
void parseInstrDSR( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
    
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    acceptRegA( instr ); 
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrFieldS( instr, 0, 6, (uint32_t) rExpr.val );
    }
    else if (( rExpr.typ == TYP_GREG ) && ( rExpr.val == 1 )) {
        
        depositInstrBit( instr, 13, true );
    }
    else throw ( ERR_EXPECTED_LEN_ARG );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// The SHLA instruction performs a shift left of "B" by the instruction encoded shift 
// amount and adds the "A" register to it. If the ".I" option is set, the RegA field is
// interpreted as a number.
//
//      SHLxA       <targetReg> "," <sourceRegB> "," <sourceRegA>
//      SHLxA ".I"  <targetReg> "," <sourceRegA> "," <val>
//
//----------------------------------------------------------------------------------------
void parseInstrSHLxA( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if ((( instrOpToken == TOK_OP_SHL1A ) && ( instrFlags & ~IM_SHLxA_OP )) ||
        (( instrOpToken == TOK_OP_SHL2A ) && ( instrFlags & ~IM_SHLxA_OP )) ||
        (( instrOpToken == TOK_OP_SHL3A ) && ( instrFlags & ~IM_SHLxA_OP ))) {

        throw ( ERR_INVALID_INSTR_OPT );
    } 
    
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) {
        
        depositInstrBit( instr, 13, true );
        depositInstrRegA( instr, (uint32_t) rExpr.val );
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrBit( instr,14, true );
        depositInstrImm13( instr, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// The SHRA instruction performs a shift right of "B" by the instruction encoded shift
// amount and adds the "A" register to it. for a numeric value instead of RegA, the 
// register field is a numeric value and the "I" option is set.
//
//      SHRxA       <targetReg> "," <sourceRegB> "," <sourceRegA>
//      SHRxA ".I"  <targetReg> "," <sourceRegA> "," <val>
//
//----------------------------------------------------------------------------------------
void parseInstrSHRxA( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr      = INIT_EXPR;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if ((( instrOpToken == TOK_OP_SHR1A ) && ( instrFlags & ~IM_SHLxA_OP )) ||
        (( instrOpToken == TOK_OP_SHR2A ) && ( instrFlags & ~IM_SHLxA_OP )) ||
        (( instrOpToken == TOK_OP_SHR3A ) && ( instrFlags & ~IM_SHLxA_OP ))) {

        throw ( ERR_INVALID_INSTR_OPT );
    } 
    
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG ) {
        
        depositInstrBit( instr, 13, true );
        depositInstrRegA( instr, (uint32_t) rExpr.val );
    }
    else if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrBit( instr,14, true );
        depositInstrImm13( instr, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_GENERAL_REG );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// The IMM-OP instruction group deals with the loading of immediate subfield and the
// addition of the ADDIL instruction, which will add the encoded value left shifted to
// <sourceReg>. The result is in R1.
//
//      LDI [ .L/S/U ] <targetReg> "," <val>
//      ADDIL <sourceReg> "," <val>
//
//----------------------------------------------------------------------------------------
void parseInstrImmOp( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr      = INIT_EXPR;
    uint32_t    instrFlags = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_LDI    ) && ( instrFlags & ~IM_LDI_OP  )) {

        throw ( ERR_INVALID_INSTR_OPT );
    } 

    if (( instrOpToken == TOK_OP_ADDIL  ) && ( instrFlags & ~IM_NIL )) { 
        
        throw ( ERR_INVALID_INSTR_OPT );
    }
    
    acceptRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) depositInstrImm20U( instr, (uint32_t) rExpr.val );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// The "LDO" instruction computes the address of an operand, and stores the result
// in "R".
//
//      LDO <targetReg> "," [ <ofs> "," ] "(" <baseReg> ")"
//
//----------------------------------------------------------------------------------------
void parseInstrLDO( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
    
    nextToken( );
    acceptRegR( instr );
    acceptComma( );

    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) depositInstrImm15( instr, (uint32_t) rExpr.val );
    else throw( ERR_EXPECTED_NUMERIC );
    
    acceptLparen( );
    acceptRegB( instr );
    acceptRparen( );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseMemOp" parses the load and store instruction family. The LD and ST family can
// have an option for specifying the data width. The LDR and STC instruction do not have
// an option. However we set the dataWidth field for them to "D".
//
//       LD  [.B/H/W/D/M ] <targetReg> ","  [ <ofs> ] "(" <baseReg> ")"
//       LD  [.B/H/W/D/M ] <targetReg> ","  [ <indexReg> ] "(" <baseReg> ")"
//
//       ST  [.B/H/W/D/M ] <sourceReg> "," [ <ofs> ] "(" <baseReg> ")"
//       ST  [.B/H/W/D/M ] <sourceReg> ","  [ <indexReg> ] "(" <baseReg> ")"
//
//       LDR               <targetReg> ","  [ <ofs> ] "(" <baseReg> ")"
//       STC               <sourceReg> "," [ <ofs> ] "(" <baseReg> ")"
//
//----------------------------------------------------------------------------------------
void parseMemOp( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if ((( instrOpToken == TOK_OP_LD  ) && ( instrFlags & ~IM_LD_OP ))  ||
        (( instrOpToken == TOK_OP_ST  ) && ( instrFlags & ~IM_ST_OP ))  ||
        (( instrOpToken == TOK_OP_LDR ) && ( instrFlags & ~IM_NIL   ))  ||
        (( instrOpToken == TOK_OP_STC ) && ( instrFlags & ~IM_NIL   ))) { 
            
        throw ( ERR_INVALID_INSTR_OPT );
    }
    
    if (( instrOpToken == TOK_OP_LDR ) || ( instrOpToken == TOK_OP_STC )) { 
        
        instrFlags |= IF_D;
    }
    
    setInstrDwField( instr, instrFlags );
    if ( instrFlags & IF_M ) depositInstrBit( instr, 20, true );

    acceptRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        setInstrDwField( instr, instrFlags );
        depositInstrScaledImm13( instr, (uint32_t) rExpr.val );
        
        acceptLparen( );
        acceptRegB( instr );
        acceptRparen( );
    }
    else if ( rExpr.typ == TYP_GREG) {
        
        if (( instrOpToken == TOK_OP_LDR ) || ( instrOpToken == TOK_OP_STC )) {
         
            throw ( ERR_INVALID_INSTR_MODE );
        }
        
        depositInstrBit( instr, 19, true );
        setInstrDwField( instr, instrFlags );
        depositInstrRegA( instr, (uint32_t) rExpr.val );
        
        acceptLparen( );
        acceptRegB( instr );
        acceptRparen( );
    }
    else throw ( ERR_EXPECTED_NUMERIC );
  
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseOpB" parses the branch instruction. The branch instruction may have the "gate"
// option.
//
//      B [ .G ] <ofs> [ "," <Reg R> ]
//
//----------------------------------------------------------------------------------------
void parseInstrB( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_B  ) && ( instrFlags & ~IM_B_OP )) {

        throw ( ERR_INVALID_INSTR_OPT );
    } 
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
     
        rExpr.val = rExpr.val >> 2;
        depositInstrImm19( instr, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_BR_OFS );
    
    if ( isToken( TOK_COMMA )) {
        
        nextToken( );
        acceptRegR( instr );
    }
    
    if ( instrFlags & IF_G ) depositInstrBit( instr, 19, true );
    acceptEOS( ); 
}

//----------------------------------------------------------------------------------------
// "parseOpBE" is the external branch. We add and offset to RegB which forms the target 
// offset. Optionally, we can specify a return link register.
//
//      BE <regB> [ "," ofs [ "," <regR> ]]
//
//----------------------------------------------------------------------------------------
void parseInstrBE( uint32_t *instr, uint32_t instrOpToken ) {

    Expr rExpr = INIT_EXPR;
    
    nextToken( );
    acceptRegB( instr );
    acceptComma( );

    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {

         depositInstrImm15( instr, (uint32_t) rExpr.val );
    }
    else ;

    if ( isToken( TOK_COMMA )) {

        nextToken( );
        acceptRegR( instr );
    }
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseOpBR" is the IA-relative branch adding RegB to IA. Optionally, we can specify
// a return link register.
//
//      BR <regB> [ "," <regR> ]
//
//----------------------------------------------------------------------------------------
void parseInstrBR( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
  
    nextToken( );
    parseExpr( &rExpr );
    acceptRegB( instr );
    
    if ( isToken( TOK_COMMA )) {
        
        nextToken( );
        acceptRegR( instr );
    }
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseOpBV" is the vectored branch. We add RegB and RegX, which form the target 
// offset. Optionally, we can specify a return link register.
//
//      BV <regB>, <RegX> [ "," <regR> ]
//
//----------------------------------------------------------------------------------------
void parseInstrBV( uint32_t *instr, uint32_t instrOpToken ) {
    
    nextToken( );
    acceptRegB( instr );
    acceptComma( );
    acceptRegA( instr );
    
    if ( isToken( TOK_COMMA )) {
        
        nextToken( );
        acceptRegR( instr );
    }
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseOpBB" is the branch on bit instruction.
//
//      BB ".T/F" <regB> "," <pos>
//      BB ".T/F" <regB> "," "SAR"
//
//----------------------------------------------------------------------------------------
void parseInstrBB( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_BB  ) && ( instrFlags & ~IM_BB_OP )) {

        throw ( ERR_INVALID_INSTR_OPT );        
    } 
    
    if ( instrFlags & IF_T ) depositInstrBit( instr, 19, true );
    
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrFieldS( instr, 0, 6, (uint32_t) rExpr.val );
    }
    else if (( rExpr.typ == TYP_GREG ) && ( rExpr.val == 1 )) {
        
        depositInstrBit( instr, 20, true );
    }
    else throw ( ERR_EXPECTED_POS_ARG );
   
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseOpCBR" performa a compare and a branch based on the condition.
//
//      CBR ".EQ/LT/NE/LE" RegR "," RegB "," <ofs>
//
//----------------------------------------------------------------------------------------
void parseInstrCBR( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_CBR  ) && ( instrFlags & ~IM_CBR_OP )) {
        
        throw ( ERR_INVALID_INSTR_OPT );
    }
  
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
     
        rExpr.val = rExpr.val >> 2;
        depositInstrImm19( instr, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_BR_OFS );
    
    setInstrCondField( instr, instrFlags );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrMBR" move the source reg to the target reg and branches on the condition
// specified.
//
//      MBR ".EQ/LT/NE/LE" RegR "," RegB "," <ofs>
//
// ??? same conditions that CMP ?
//----------------------------------------------------------------------------------------
void parseInstrMBR( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr        rExpr       = INIT_EXPR;
    uint32_t    instrFlags  = IF_NIL;
    
    nextToken( );
    parseInstrOptions( &instrFlags, instrOpToken );
    if (( instrOpToken == TOK_OP_CBR  ) && ( instrFlags & ~IM_CBR_OP )) {
        
        throw ( ERR_INVALID_INSTR_OPT );
    }
  
    acceptRegR( instr ); 
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        rExpr.val = rExpr.val >> 2;
        depositInstrImm19( instr, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_BR_OFS );
    
    setInstrCondField( instr, instrFlags );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrMxCR" copies a control register to a general register and vice versa.
//
//      MFCR <RegR> "," <RegB>
//      MTCR <RegR> "," <RegB>
//
//----------------------------------------------------------------------------------------
void parseInstrMxCR( uint32_t *instr, uint32_t instrOpToken ) {
   
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    acceptCregB( instr );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrMFIA" copies the instruction offset to a general register.
//
//      MFIA <RegR>
//
//----------------------------------------------------------------------------------------
void parseInstrMFIA( uint32_t *instr, uint32_t instrOpToken ) {
   
    nextToken( );
    acceptRegR( instr );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrLDPA" returns the physical address of a virtual address. It is very 
// similar to the memory type instruction load and store, except it does just do 
// address translation. If the page is not in main memory, a zero is returned.
//
//       LPA <targetReg> ","  [ <indexReg> ] "(" <baseReg> ")"
//
//----------------------------------------------------------------------------------------
void parseInstrLPA( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
    
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
   
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_GREG) {
        
        depositInstrRegA( instr, (uint32_t) rExpr.val );
    }
    
    acceptLparen( );
    acceptRegB( instr );
    acceptRparen( );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrPRB" probes a virtual address for access. The "P/U" indicate privileged
//  and user mode access.
//
//      PRB <RegR> "," <RegB> "," <RegA>
//      PRB <RegR> "," <RegB> "," <val>
//
//----------------------------------------------------------------------------------------
void parseInstrPRB( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;

    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if      ( rExpr.typ == TYP_GREG ) depositInstrRegA( instr, (uint32_t) rExpr.val );
    else if ( rExpr.typ == TYP_NUM  ) depositInstrField( instr, 9, 2, rExpr.val );
    else throw ( ERR_EXPECTED_PRB_ARG );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrTlbOp" insert or removes a translation in the TLB. RegB contains the 
// virtual address. For TLB inserts RegA contains the info on access rights and 
// physical address. The result is in RegR.
//
//      ITLB <targetReg> "," <RegB> "," <RegA>
// ^    PTLB <targetReg> "," <RegB>
//
//----------------------------------------------------------------------------------------
void parseInstrTlbOp( uint32_t *instr, uint32_t instrOpToken ) {
   
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    
    if ( instrOpToken == TOK_OP_ITLB ) {
        
        acceptComma( );
        acceptRegA( instr );
    }
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrCacheOp" assemble the cache flush and purge operation.
//
//      PCA <targetReg> "," <RegB>
//      FCA <targetReg> "," <RegB>
//
//----------------------------------------------------------------------------------------
void parseInstrCacheOp( uint32_t *instr, uint32_t instrOpToken ) {
    
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    acceptRegB( instr );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrSregOp" sets or clears status register bits. 
// Under construction ...
//
//      RSM <RegR> "," <val>
//      SSM <RegR> "," <val>
//
//----------------------------------------------------------------------------------------
void parseInstrSregOp( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
   
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) depositInstrFieldU( instr, 0, 8, (uint32_t) rExpr.val );
    else throw ( ERR_EXPECTED_NUMERIC );
    
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// The "RFI" instruction is the return from interrupt method. So far it is only
// the instruction with no further options and arguments.
//
//      RFI
//
//----------------------------------------------------------------------------------------
void parseInstrRFI( uint32_t *instr, uint32_t instrOpToken ) {
    
    nextToken( );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrDIAG" is the general purpose diagnostic instruction. It accepts two 
// registers and returns a result in the target register.
//
//      DIAG <RegR> "," <val> "," <RegB> "," <RegA"
//
//----------------------------------------------------------------------------------------
void parseInstrDIAG( uint32_t *instr, uint32_t instrOpToken ) {
    
    Expr rExpr = INIT_EXPR;
   
    nextToken( );
    acceptRegR( instr );
    acceptComma( );
    
    parseExpr( &rExpr );
    if ( rExpr.typ == TYP_NUM ) {
        
        depositInstrField( instr, 19, 3, (uint32_t) rExpr.val >> 2 );
        depositInstrField( instr, 20, 2, (uint32_t) rExpr.val );
    }
    else throw ( ERR_EXPECTED_DIAG_OP );
    
    acceptComma( );
    acceptRegB( instr );
    acceptComma( );
    acceptRegA( instr );
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseInstrTrapOp" assembles the trap operations.
//
//      Generic. TRAP <info1> "," RegB "," RegA "," <info2> "," <val>
//
// We have p to 8 trap group IDs. Group zero should be the BRK group ID..
//
// ??? to be designed ...
//----------------------------------------------------------------------------------------
void parseInstrTrapOp( uint32_t *instr, uint32_t instrOpToken ) {
    
    switch ( extractInstrOptField( *instr )) {
            
        case 0: ;
        case 1: ;
        case 2: ;
        case 3: ;
        case 4: ;
        default: ;
    }
   
    acceptEOS( );
}

//----------------------------------------------------------------------------------------
// "parseLine" will take the input string and parse the line for an instruction. In 
// the one-line case, there is only the opCode mnemonic and the argument list. No 
// labels, comments are ignored.
//
//----------------------------------------------------------------------------------------
void parseLine( char *inputStr, uint32_t *instr ) {
    
    setupTokenizer( inputStr );
 
    if ( isTokenTyp( TYP_OP_CODE )) {
        
        uint32_t instrOpToken   = currentToken.tid;
        *instr                  = (uint32_t) currentToken.val;
        
        switch( instrOpToken ) {
                
            case TOK_OP_NOP:    parseNopInstr( instr, instrOpToken );       break;
                
            case TOK_OP_ADD:
            case TOK_OP_SUB:
            case TOK_OP_AND:
            case TOK_OP_OR:
            case TOK_OP_XOR:
            case TOK_OP_CMP:    parseModeTypeInstr( instr, instrOpToken );  break;
                
            case TOK_OP_EXTR:   parseInstrEXTR( instr, instrOpToken );      break;
            case TOK_OP_DEP:    parseInstrDEP( instr, instrOpToken );       break;
            case TOK_OP_DSR:    parseInstrDSR( instr, instrOpToken );       break;
                
            case TOK_OP_SHL1A:
            case TOK_OP_SHL2A:
            case TOK_OP_SHL3A:  parseInstrSHLxA( instr, instrOpToken );     break;
                
            case TOK_OP_SHR1A:
            case TOK_OP_SHR2A:
            case TOK_OP_SHR3A:  parseInstrSHRxA( instr, instrOpToken );     break;
                
            case TOK_OP_LDI:
            case TOK_OP_ADDIL:  parseInstrImmOp( instr, instrOpToken );     break;
                
            case TOK_OP_LDO:    parseInstrLDO( instr, instrOpToken );       break;
                
            case TOK_OP_LD:
            case TOK_OP_LDR:
            case TOK_OP_ST:
            case TOK_OP_STC:    parseMemOp( instr, instrOpToken );          break;
                
            case TOK_OP_B:      parseInstrB( instr, instrOpToken );         break;
            case TOK_OP_BE:     parseInstrBE( instr, instrOpToken );        break;
            case TOK_OP_BR:     parseInstrBR( instr, instrOpToken );        break;
            case TOK_OP_BV:     parseInstrBV( instr, instrOpToken );        break;
            case TOK_OP_BB:     parseInstrBB( instr, instrOpToken );        break;
                
            case TOK_OP_CBR:    parseInstrCBR( instr, instrOpToken );       break;
            case TOK_OP_MBR:    parseInstrMBR( instr, instrOpToken );       break;
                
            case TOK_OP_MFCR:
            case TOK_OP_MTCR:   parseInstrMxCR( instr, instrOpToken );      break;

            case TOK_OP_MFIA:   parseInstrMFIA( instr, instrOpToken );      break;
                
            case TOK_OP_LPA:    parseInstrLPA( instr, instrOpToken );       break;
                
            case TOK_OP_PRB:    parseInstrPRB( instr, instrOpToken );       break;
                
            case TOK_OP_ITLB:
            case TOK_OP_PTLB:   parseInstrTlbOp( instr, instrOpToken );     break;
                
            case TOK_OP_PCA:
            case TOK_OP_FCA:    parseInstrCacheOp( instr, instrOpToken );   break;
                
            case TOK_OP_SSM:
            case TOK_OP_RSM:    parseInstrSregOp( instr, instrOpToken );    break;
                
            case TOK_OP_RFI:    parseInstrRFI( instr, instrOpToken );       break;
                
            case TOK_OP_DIAG:   parseInstrDIAG( instr, instrOpToken );      break;
                
            case TOK_OP_TRAP:   parseInstrTrapOp( instr, instrOpToken );    break;
                
            default: throw ( ERR_INVALID_OP_CODE );
        }
    }
    else throw ( ERR_EXPECTED_OPCODE );
}

} // namespace

//----------------------------------------------------------------------------------------
// A simple one line assembler. We will parse a one line input string for a 
// valid instruction, using the syntax of the real assembler. There will be no 
// labels and comments, only the opcode and the operands.
//
//----------------------------------------------------------------------------------------
T64Assemble::T64Assemble( ) { }

int T64Assemble::assembleInstr( char *inputStr, uint32_t *instr ) {
    
    try {
        
        parseLine( inputStr, instr );
        return ( NO_ERR );
    }
    catch ( ErrId errNum ) {
        
        *instr  = 0;
        lastErr = errNum;
        return ( errNum );
    }
}

int T64Assemble::getErrId( ) {
    
    return ( lastErr );
}

int T64Assemble::getErrPos( ) {
    
    return ( currentTokCharIndex );
}

const char *T64Assemble::getErrStr( int errId ) {
    
    for ( int i = 0; i < MAX_ERR_MSG_TAB; i++ ) {
        
        if ( ErrMsgTable[ i ].msgId == errId ) return ( ErrMsgTable[ i ].msg );
    }
    
    return ( "Unknown Error Id" );
}
