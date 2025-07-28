//----------------------------------------------------------------------------------------
//
//  Twin64 - A 64-bit CPU Simulator - Simulator Tables
//
//----------------------------------------------------------------------------------------
// ...
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Simulator - Simulator Tables
// Copyright (C) 2024 - 2025 Helmut Fieres
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
#ifndef SimTables_h
#define SimTables_h

#include "T64-SimDeclarations.h"

//----------------------------------------------------------------------------------------
// The global command interpreter token table. All reserved words are allocated
// in this table. Each entry has the token name, the token id, the token type 
// id, i.e. its type, and a value associated with the token. The value allows 
// for a constant token. The parser can directly use the value in expressions.
//
//----------------------------------------------------------------------------------------
const SimToken cmdTokTab[ ] = {
    
    //------------------------------------------------------------------------------------
    // General tokens.
    //
    //------------------------------------------------------------------------------------
    { .name = "NIL",        .typ = TYP_SYM,     .tid = TOK_NIL,   .u.val = 0  },
    
    { .name = "ALL",        .typ = TYP_SYM,     .tid = TOK_ALL                },
    { .name = "PROC",       .typ = TYP_SYM,     .tid = TOK_PROC               },
    { .name = "CACHE",      .typ = TYP_SYM,     .tid = TOK_CACHE              },
    { .name = "MEM",        .typ = TYP_SYM,     .tid = TOK_MEM                },
    { .name = "TEXT",       .typ = TYP_SYM,     .tid = TOK_TEXT               },

    { .name = "DEC",        .typ = TYP_SYM,     .tid = TOK_DEC,   .u.val = 10 },
    { .name = "HEX",        .typ = TYP_SYM,     .tid = TOK_HEX,   .u.val = 16 },
    { .name = "CODE",       .typ = TYP_SYM,     .tid = TOK_CODE               },
    
    //------------------------------------------------------------------------------------
    //
    //
    //------------------------------------------------------------------------------------
    { .name = "COMMANDS",   .typ = TYP_CMD,     .tid = CMD_SET                },
    { .name = "WCOMMANDS",  .typ = TYP_WCMD,    .tid = WCMD_SET               },
    { .name = "PREDEFINED", .typ = TYP_P_FUNC,  .tid = PF_SET                 },
    
    //------------------------------------------------------------------------------------
    // Command Line tokens.
    //
    //------------------------------------------------------------------------------------
    { .name = "HELP",       .typ = TYP_CMD,     .tid = CMD_HELP               },
    { .name = "?",          .typ = TYP_CMD,     .tid = CMD_HELP               },
    
    { .name = "EXIT",       .typ = TYP_CMD,     .tid = CMD_EXIT               },
    { .name = "E",          .typ = TYP_CMD,     .tid = CMD_EXIT               },
    
    { .name = "HIST",       .typ = TYP_CMD,     .tid = CMD_HIST               },
    { .name = "DO",         .typ = TYP_CMD,     .tid = CMD_DO                 },
    { .name = "REDO",       .typ = TYP_CMD,     .tid = CMD_REDO               },
    { .name = "ENV",        .typ = TYP_CMD,     .tid = CMD_ENV                },
    { .name = "XF",         .typ = TYP_CMD,     .tid = CMD_XF                 },
    { .name = "W",          .typ = TYP_CMD,     .tid = CMD_WRITE_LINE         },
    
    { .name = "RESET",      .typ = TYP_CMD,     .tid = CMD_RESET              },
    { .name = "RUN",        .typ = TYP_CMD,     .tid = CMD_RUN                },
    { .name = "STEP",       .typ = TYP_CMD,     .tid = CMD_STEP               },
    { .name = "S",          .typ = TYP_CMD,     .tid = CMD_STEP               },
    
    { .name = "DR",         .typ = TYP_CMD,     .tid = CMD_DR                 },
    { .name = "MR",         .typ = TYP_CMD,     .tid = CMD_MR                 },
    { .name = "DA",         .typ = TYP_CMD,     .tid = CMD_DA                 },
    { .name = "MA",         .typ = TYP_CMD,     .tid = CMD_MA                 },
    
    { .name = "ITLB",       .typ = TYP_CMD,     .tid = CMD_I_TLB              },
    { .name = "DTLB",       .typ = TYP_CMD,     .tid = CMD_D_TLB              },
    { .name = "PTLB",       .typ = TYP_CMD,     .tid = CMD_P_TLB              },
    
    { .name = "DCA",        .typ = TYP_CMD,     .tid = CMD_D_CACHE            },
    { .name = "PCA",        .typ = TYP_CMD,     .tid = CMD_P_CACHE            },
    { .name = "FCA",        .typ = TYP_CMD,     .tid = CMD_F_CACHE            },
    
    //------------------------------------------------------------------------------------
    // Window command tokens.
    //
    //------------------------------------------------------------------------------------
    { .name = "WON",        .typ = TYP_WCMD,    .tid = CMD_WON                },
    { .name = "WOFF",       .typ = TYP_WCMD,    .tid = CMD_WOFF               },
    { .name = "WDEF",       .typ = TYP_WCMD,    .tid = CMD_WDEF               },
    { .name = "WSE",        .typ = TYP_WCMD,    .tid = CMD_WSE                },
    { .name = "WSD",        .typ = TYP_WCMD,    .tid = CMD_WSD                },
    
    { .name = "CWL",        .typ = TYP_WCMD,    .tid = CMD_CWL                },
    
    { .name = "WE",         .typ = TYP_WCMD,    .tid = CMD_WE                 },
    { .name = "WD",         .typ = TYP_WCMD,    .tid = CMD_WD                 },
    { .name = "WR",         .typ = TYP_WCMD,    .tid = CMD_WR                 },
    { .name = "WF",         .typ = TYP_WCMD,    .tid = CMD_WF                 },
    { .name = "WB",         .typ = TYP_WCMD,    .tid = CMD_WB                 },
    { .name = "WH",         .typ = TYP_WCMD,    .tid = CMD_WH                 },
    { .name = "WJ",         .typ = TYP_WCMD,    .tid = CMD_WJ                 },
    { .name = "WL",         .typ = TYP_WCMD,    .tid = CMD_WL                 },
    { .name = "WN",         .typ = TYP_WCMD,    .tid = CMD_WN                 },
    { .name = "WK",         .typ = TYP_WCMD,    .tid = CMD_WK                 },
    { .name = "WC",         .typ = TYP_WCMD,    .tid = CMD_WC                 },
    { .name = "WS",         .typ = TYP_WCMD,    .tid = CMD_WS                 },
    { .name = "WT",         .typ = TYP_WCMD,    .tid = CMD_WT                 },
    { .name = "WX",         .typ = TYP_WCMD,    .tid = CMD_WX                 },
    
    { .name = "PM",         .typ = TYP_SYM,     .tid = TOK_MEM                },
    { .name = "PC",         .typ = TYP_SYM,     .tid = TOK_CODE               },

    //------------------------------------------------------------------------------------
    // General registers.
    //
    //------------------------------------------------------------------------------------
    { .name = "R0",         .typ = TYP_GREG,    .tid = GR_0,     .u.val =  0  },
    { .name = "R1",         .typ = TYP_GREG,    .tid = GR_1,     .u.val =  1  },
    { .name = "R2",         .typ = TYP_GREG,    .tid = GR_2,     .u.val =  2  },
    { .name = "R3",         .typ = TYP_GREG,    .tid = GR_3,     .u.val =  3  },
    { .name = "R4",         .typ = TYP_GREG,    .tid = GR_4,     .u.val =  4  },
    { .name = "R5",         .typ = TYP_GREG,    .tid = GR_5,     .u.val =  5  },
    { .name = "R6",         .typ = TYP_GREG,    .tid = GR_6,     .u.val =  6  },
    { .name = "R7",         .typ = TYP_GREG,    .tid = GR_7,     .u.val =  7  },
    { .name = "R8",         .typ = TYP_GREG,    .tid = GR_8,     .u.val =  8  },
    { .name = "R9",         .typ = TYP_GREG,    .tid = GR_9,     .u.val =  9  },
    { .name = "R10",        .typ = TYP_GREG,    .tid = GR_10,    .u.val =  10 },
    { .name = "R11",        .typ = TYP_GREG,    .tid = GR_11,    .u.val =  11 },
    { .name = "R12",        .typ = TYP_GREG,    .tid = GR_12,    .u.val =  12 },
    { .name = "R13",        .typ = TYP_GREG,    .tid = GR_13,    .u.val =  13 },
    { .name = "R14",        .typ = TYP_GREG,    .tid = GR_14,    .u.val =  14 },
    { .name = "R15",        .typ = TYP_GREG,    .tid = GR_15,    .u.val =  15 },
    { .name = "GR",         .typ = TYP_GREG,    .tid = GR_SET,   .u.val =  0  },

    //------------------------------------------------------------------------------------
    // Runtime architecture register names for general registers.
    //
    //------------------------------------------------------------------------------------
    { .name = "T0",         .typ = TYP_GREG,    .tid = GR_1,     .u.val =  1  },
    { .name = "T1",         .typ = TYP_GREG,    .tid = GR_2,     .u.val =  2  },
    { .name = "T2",         .typ = TYP_GREG,    .tid = GR_3,     .u.val =  3  },
    { .name = "T3",         .typ = TYP_GREG,    .tid = GR_4,     .u.val =  4  },
    { .name = "T4",         .typ = TYP_GREG,    .tid = GR_5,     .u.val =  5  },
    { .name = "T5",         .typ = TYP_GREG,    .tid = GR_6,     .u.val =  6  },
    { .name = "T6",         .typ = TYP_GREG,    .tid = GR_7,     .u.val =  7  },

    { .name = "ARG3",       .typ = TYP_GREG,    .tid = GR_8,     .u.val =  8  },
    { .name = "ARG2",       .typ = TYP_GREG,    .tid = GR_9,     .u.val =  9  },
    { .name = "ARG1",       .typ = TYP_GREG,    .tid = GR_10,    .u.val =  10 },
    { .name = "ARG0",       .typ = TYP_GREG,    .tid = GR_11,    .u.val =  11 },

    { .name = "RET3",       .typ = TYP_GREG,    .tid = GR_8,     .u.val =  8  },
    { .name = "RET2",       .typ = TYP_GREG,    .tid = GR_9,     .u.val =  9  },
    { .name = "RET1",       .typ = TYP_GREG,    .tid = GR_10,    .u.val =  10 },
    { .name = "RET0",       .typ = TYP_GREG,    .tid = GR_11,    .u.val =  11 },
    
    { .name = "DP",         .typ = TYP_GREG,    .tid = GR_13,    .u.val =  13 },
    { .name = "RL",         .typ = TYP_GREG,    .tid = GR_14,    .u.val =  14 },
    { .name = "SP",         .typ = TYP_GREG,    .tid = GR_15,    .u.val =  15 },
    
    //------------------------------------------------------------------------------------
    // Control registers.
    //
    //------------------------------------------------------------------------------------
    { .name = "C0",         .typ = TYP_CREG,    .tid = CR_0,     .u.val =  0  },
    { .name = "C1",         .typ = TYP_CREG,    .tid = CR_1,     .u.val =  1  },
    { .name = "C2",         .typ = TYP_CREG,    .tid = CR_2,     .u.val =  2  },
    { .name = "C3",         .typ = TYP_CREG,    .tid = CR_3,     .u.val =  3  },
    { .name = "C4",         .typ = TYP_CREG,    .tid = CR_4,     .u.val =  4  },
    { .name = "C5",         .typ = TYP_CREG,    .tid = CR_5,     .u.val =  5  },
    { .name = "C6",         .typ = TYP_CREG,    .tid = CR_6,     .u.val =  6  },
    { .name = "C7",         .typ = TYP_CREG,    .tid = CR_7,     .u.val =  7  },
    { .name = "C8",         .typ = TYP_CREG,    .tid = CR_8,     .u.val =  8  },
    { .name = "C9",         .typ = TYP_CREG,    .tid = CR_9,     .u.val =  9  },
    { .name = "C10",        .typ = TYP_CREG,    .tid = CR_10,    .u.val =  10 },
    { .name = "C11",        .typ = TYP_CREG,    .tid = CR_11,    .u.val =  11 },
    { .name = "C12",        .typ = TYP_CREG,    .tid = CR_12,    .u.val =  12 },
    { .name = "C13",        .typ = TYP_CREG,    .tid = CR_13,    .u.val =  13 },
    { .name = "C14",        .typ = TYP_CREG,    .tid = CR_14,    .u.val =  14 },
    { .name = "C15",        .typ = TYP_CREG,    .tid = CR_15,    .u.val =  15 },
    
    //------------------------------------------------------------------------------------
    // Predefined functions.
    //
    //------------------------------------------------------------------------------------
    { .name = "ASM",        .typ = TYP_P_FUNC, .tid = PF_ASSEMBLE, .u.val = 0 },
    { .name = "DISASM",     .typ = TYP_P_FUNC, .tid = PF_DIS_ASM,  .u.val = 0 },
    { .name = "HASH",       .typ = TYP_P_FUNC, .tid = PF_HASH,     .u.val = 0 },

    // ??? tons of more predefines....

    { .name = "S32",        .typ = TYP_P_FUNC, .tid = PF_S32,      .u.val = 0 },
    
};

const int MAX_CMD_TOKEN_TAB = sizeof( cmdTokTab ) / sizeof( SimToken );

//----------------------------------------------------------------------------------------
// The error message table. Each entry has the error number and the 
// corresponding error message text.
//
// ??? sort the entries... remove what is not needed anymore...
//----------------------------------------------------------------------------------------
const SimErrMsgTabEntry errMsgTab [ ] = {
    
    { .errNum = NO_ERR,                         
      .errStr = (char *) "NO_ERR" },

    { .errNum = ERR_NOT_SUPPORTED,              
      .errStr = (char *) "Command or Function not supported (yet)" },
    
    { .errNum = ERR_INVALID_CMD,                
      .errStr = (char *) "Invalid command, use help" },

    { .errNum = ERR_NUMERIC_OVERFLOW,                
      .errStr = (char *) "Numeric overflow in expression" },
    
    { .errNum = ERR_INVALID_CHAR_IN_TOKEN_LINE, 
      .errStr = (char *) "Invalid char in input line" },
    
    { .errNum = ERR_INVALID_ARG,                
      .errStr = (char *) "Invalid argument for command" },

    { .errNum = ERR_INVALID_WIN_ID,             
      .errStr = (char *) "Invalid window Id" },

    { .errNum = ERR_INVALID_REG_ID,             
      .errStr = (char *) "Invalid register Id" },

    { .errNum = ERR_INVALID_RADIX,              
      .errStr = (char *) "Invalid radix" },

    { .errNum = ERR_INVALID_EXIT_VAL,           
      .errStr = (char *) "Invalid program exit code" },

    { .errNum = ERR_INVALID_WIN_STACK_ID,       
      .errStr = (char *) "Invalid window stack Id" },

    { .errNum = ERR_INVALID_STEP_OPTION,        
      .errStr = (char *) "Invalid steps/instr option" },

    { .errNum = ERR_INVALID_EXPR,               
      .errStr = (char *) "Invalid expression" },

    { .errNum = ERR_INVALID_NUM,
      .errStr = (char *) "Invalid number" },

    { .errNum = ERR_INVALID_INSTR_OPT,          
      .errStr = (char *) "Invalid instruction option" },

    { .errNum = ERR_INVALID_INSTR_MODE,         
      .errStr = (char *) "Invalid adr mode for instruction" },

    { .errNum = ERR_INVALID_REG_COMBO,          
      .errStr = (char *) "Invalid register combo for instruction" },

    { .errNum = ERR_INVALID_OP_CODE,            
      .errStr = (char *) "Invalid instruction opcode" },

    { .errNum = ERR_INVALID_S_OP_CODE,          
      .errStr = (char *) "Invalid synthetic instruction opcode" },

    { .errNum = ERR_INVALID_FMT_OPT,            
      .errStr = (char *) "Invalid format option" },

    { .errNum = ERR_INVALID_WIN_TYPE,           
      .errStr = (char *) "Invalid window type" },

    { .errNum = ERR_INVALID_CMD_ID,             
      .errStr = (char *) "Invalid command Id" },

    { .errNum = ERR_EXPECTED_INSTR_VAL,         
      .errStr = (char *) "Expected the instruction value" },

    { .errNum = ERR_EXPECTED_FILE_NAME,         
      .errStr = (char *) "Expected a file name" },

    { .errNum = ERR_EXPECTED_STACK_ID,          
      .errStr = (char *) "Expected stack Id" },

    { .errNum = ERR_EXPECTED_WIN_ID,            
      .errStr = (char *) "Expected a window Id" },

    { .errNum = ERR_EXPECTED_LPAREN,            
      .errStr = (char *) "Expected a left paren" },

    { .errNum = ERR_EXPECTED_RPAREN,            
      .errStr = (char *) "Expected a right paren" },

    { .errNum = ERR_EXPECTED_COMMA,             
      .errStr = (char *) "Expected a comma" },

    { .errNum = ERR_EXPECTED_STR,               
      .errStr = (char *) "Expected a string value" },

    { .errNum = ERR_EXPECTED_REG_SET,           
      .errStr = (char *) "Expected a register set" },

    { .errNum = ERR_EXPECTED_REG_OR_SET,        
      .errStr = (char *) "Expected a register or register set" },

    { .errNum = ERR_EXPECTED_NUMERIC,           
      .errStr = (char *) "Expected a numeric value" },

    { .errNum = ERR_EXPECTED_EXT_ADR,           
      .errStr = (char *) "Expected a virtual address" },

    { .errNum = ERR_EXPECTED_GENERAL_REG,       
      .errStr = (char *) "Expected a general reg" },

    { .errNum = ERR_EXPECTED_STEPS,             
      .errStr = (char *) "Expected number of steps/instr" },

    { .errNum = ERR_EXPECTED_START_OFS,         
      .errStr = (char *) "Expected start offset" },
        
    { .errNum = ERR_EXPECTED_LEN,               
      .errStr = (char *) "Expected length argument" },

    { .errNum = ERR_EXPECTED_OFS,               
      .errStr = (char *) "Expected an address" },

    { .errNum = ERR_EXPECTED_INSTR_OPT,         
      .errStr = (char *) "Expected the instruction options" },

    { .errNum = ERR_EXPECTED_SR1_SR3,           
      .errStr = (char *) "Expected SR1 .. SR3 as segment register" },

    { .errNum = ERR_EXPECTED_LOGICAL_ADR,       
      .errStr = (char *) "Expected a logical address" },

    { .errNum = ERR_EXPECTED_AN_OFFSET_VAL,     
      .errStr = (char *) "Expected an offset value" },

    { .errNum = ERR_EXPECTED_SEGMENT_REG,       
      .errStr = (char *) "Expected a segment register" },

    { .errNum = ERR_EXPECTED_FMT_OPT,           
      .errStr = (char *) "Expected a format option" },

    { .errNum = ERR_EXPECTED_WIN_TYPE,          
      .errStr = (char *) "Expected a window type" },

    { .errNum = ERR_EXPECTED_EXPR,              
      .errStr = (char *) "Expected an expression" },
   
    { .errNum = ERR_UNEXPECTED_EOS,             
      .errStr = (char *) "Unexpected end of command line" },

    { .errNum = ERR_NOT_IN_WIN_MODE,            
      .errStr = (char *) "Command only valid in Windows mode" },

    { .errNum = ERR_OPEN_EXEC_FILE,             
      .errStr = (char *) "Error while opening file" },

    { .errNum = ERR_EXTRA_TOKEN_IN_STR,         
      .errStr = (char *) "Extra tokens in command line" },

    { .errNum = ERR_ENV_VALUE_EXPR,             
      .errStr = (char *) "Invalid expression for ENV variable" },

    { .errNum = ERR_ENV_VAR_NOT_FOUND,          
      .errStr = (char *) "ENV variable not found" },

    { .errNum = ERR_WIN_TYPE_NOT_CONFIGURED,    
      .errStr = (char *) "Win object type not configured" },

    { .errNum = ERR_EXPR_TYPE_MATCH,            
      .errStr = (char *) "Expression type mismatch" },

    { .errNum = ERR_EXPR_FACTOR,                
      .errStr = (char *) "Expression error: factor" },

    { .errNum = ERR_TOO_MANY_ARGS_CMD_LINE,     
      .errStr = (char *) "Too many args in command line" },

    { .errNum = ERR_OFS_LEN_LIMIT_EXCEEDED,     
      .errStr = (char *) "Offset/Length exceeds limit" },

    { .errNum = ERR_UNDEFINED_PFUNC,            
      .errStr = (char *) "Unknown predefined function" },

    { .errNum = ERR_ENV_PREDEFINED,             
      .errStr = (char *) "ENV variable is predefined" },

    { .errNum = ERR_ENV_TABLE_FULL,             
      .errStr = (char *) "ENV Table is full" },

    { .errNum = ERR_INSTR_HAS_NO_OPT,           
      .errStr = (char *) "Instruction has no option" },

    { .errNum = ERR_IMM_VAL_RANGE,              
      .errStr = (char *) "Immediate value out of range" },

    { .errNum = ERR_POS_VAL_RANGE,              
      .errStr = (char *) "Bit position value out of range" },

    { .errNum = ERR_LEN_VAL_RANGE,              
      .errStr = (char *) "Bit field length value out of range" },

    { .errNum = ERR_OFFSET_VAL_RANGE,           
      .errStr = (char *) "Offset value out of range" },

    { .errNum = ERR_OUT_OF_WINDOWS,             
      .errStr = (char *) "Cannot create more windows" },
        
    { .errNum = ERR_TLB_TYPE,                   
      .errStr = (char *) "Expected a TLB type" },

    { .errNum = ERR_TLB_INSERT_OP,              
      .errStr = (char *) "Insert in TLB operation error" },

    { .errNum = ERR_TLB_PURGE_OP,               
      .errStr = (char *) "Purge from TLB operation error" },

    { .errNum = ERR_TLB_ACC_DATA,               
      .errStr = (char *) "Invalid TLB insert access data" },

    { .errNum = ERR_TLB_ADR_DATA,               
      .errStr = (char *) "Invalid TLB insert address data" },

    { .errNum = ERR_TLB_NOT_CONFIGURED,         
      .errStr = (char *) "TLB type not configured" },

    { .errNum = ERR_TLB_SIZE_EXCEEDED,          
      .errStr = (char *) "TLB size exceeded" },
        
    { .errNum = ERR_CACHE_TYPE,                 
      .errStr = (char *) "Expected a cache type" },

    { .errNum = ERR_CACHE_PURGE_OP,             
      .errStr = (char *) "Purge from cache operation error" },

    { .errNum = ERR_CACHE_NOT_CONFIGURED,       
      .errStr = (char *) "Cache type not configured" },

    { .errNum = ERR_CACHE_SIZE_EXCEEDED,        
      .errStr = (char *) "Cache size exceeded" },

    { .errNum = ERR_CACHE_SET_NUM,              
      .errStr = (char *) "Invalid cache set" },
    
};

const int MAX_ERR_MSG_TAB = sizeof( errMsgTab ) / sizeof( SimErrMsgTabEntry );

//----------------------------------------------------------------------------------------
// Help message text table. Each entry has a type field, a token field, a command syntax 
// field and are explanation field.
//
//----------------------------------------------------------------------------------------
const SimHelpMsgEntry cmdHelpTab[ ] = {
    
    //------------------------------------------------------------------------------------
    // Commands.
    //
    //------------------------------------------------------------------------------------
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_HELP,
        .cmdNameStr     = (char *) "help",
        .cmdSyntaxStr   = (char *) "help ( cmdId | ‘commands‘ | "
                                   "'wcommands‘ | ‘wtypes‘ | ‘predefined‘ )",
        .helpStr        = (char *) "list help info ( type \"help help\" for details )"
    },
  
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_EXIT,
        .cmdNameStr     = (char *) "exit",
        .cmdSyntaxStr   = (char *) "exit (e) [ <val> ]",
        .helpStr        = (char *) "program exit"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_HIST,
        .cmdNameStr     = (char *) "hist",
        .cmdSyntaxStr   = (char *) "hist [ depth ]",
        .helpStr        = (char *) "command history"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_DO,
        .cmdNameStr     = (char *) "do",
        .cmdSyntaxStr   = (char *) "do [ cmdNum ]",
        .helpStr        = (char *) "re-execute command"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_REDO,
        .cmdNameStr     = (char *) "redo",
        .cmdSyntaxStr   = (char *) "redo [ cmdNum ]",
        .helpStr        = (char *) "edit and then re-execute command"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_ENV,
        .cmdNameStr     = (char *) "env",
        .cmdSyntaxStr   = (char *) "env [ <var> [ <val> ]]",
        .helpStr        = (char *) "lists the env tab, a variable, sets a variable"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_XF,
        .cmdNameStr     = (char *) "xf",
        .cmdSyntaxStr   = (char *) "xf \"<filePath>\"",
        .helpStr        = (char *) "execute commands from a file"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_RESET,
        .cmdNameStr     = (char *) "reset",
        .cmdSyntaxStr   = (char *) "reset ( 'tbd' | 'MEM' | 'STATS' | 'ALL' )",
        .helpStr        = (char *) "resets the CPU"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_RUN,
        .cmdNameStr     = (char *) "run",
        .cmdSyntaxStr   = (char *) "run",
        .helpStr        = (char *) "run the system ( all CPUs )"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_STEP,
        .cmdNameStr     = (char *) "step",
        .cmdSyntaxStr   = (char *) "s [ <steps> ]",
        .helpStr        = (char *) "single step the system"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_WRITE_LINE,
        .cmdNameStr     = (char *) "w",
        .cmdSyntaxStr   = (char *) "w <expr> [ , <rdx> ]",
        .helpStr        = (char *) "evaluates and prints an expression"
    },

    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_DM,
        .cmdNameStr     = (char *) "dm",
        .cmdSyntaxStr   = (char *) "dm",
        .helpStr        = (char *) "display registers modules for the system"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_DR,
        .cmdNameStr     = (char *) "dr",
        .cmdSyntaxStr   = (char *) "dr <cpu> [ ( <regSet> | <reg> ) ] [ , <fmt> ]",
        .helpStr        = (char *) "display a CPU register or register set"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_MR,
        .cmdNameStr     = (char *) "mr",
        .cmdSyntaxStr   = (char *) "mr <cpu> <reg> <val>",
        .helpStr        = (char *) "modify a CPU register"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_DA,
        .cmdNameStr     = (char *) "da",
        .cmdSyntaxStr   = (char *) "da <adr> [ , <len> ] [ , <fmt> ]",
        .helpStr        = (char *) "display absolute memory"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_MA,
        .cmdNameStr     = (char *) "ma",
        .cmdSyntaxStr   = (char *) "ma <adr> <val>",
        .helpStr        = (char *) "modify absolute memory"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_D_CACHE,
        .cmdNameStr     = (char *) "dca",
        .cmdSyntaxStr   = (char *) "dca <proc>, <cache>, <set>, "
                                   " <index> [ , <len> [ , <fmt> ]] ",
        .helpStr        = (char *) "display cache content"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_P_CACHE,
        .cmdNameStr     = (char *) "pca",
        .cmdSyntaxStr   = (char *) "pca <proc>, <cache>, <set>, <index>",
        .helpStr        = (char *) "purges cache line data"
    },

    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_F_CACHE,
        .cmdNameStr     = (char *) "fca",
        .cmdSyntaxStr   = (char *) "fca <proc>, <cache>, <set>, <index>",
        .helpStr        = (char *) "flushes cache line data"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_D_TLB,
        .cmdNameStr     = (char *) "dtlb",
        .cmdSyntaxStr   = (char *) "dtlb <proc>, <tlb>, <set>, "
                                   "<index> [ , <len> [ , <rdx> ]]",
        .helpStr        = (char *) "display TLB content"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_I_TLB,
        .cmdNameStr     = (char *) "itlb",
        .cmdSyntaxStr   = (char *) "itlb <proc>, <tlb>, <set>, <info1> <info2>",
        .helpStr        = (char *) "inserts an entry into the TLB"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_P_TLB,
        .cmdNameStr     = (char *) "ptlb",
        .cmdSyntaxStr   = (char *) "ptlb <proc>, <tlb>, <set>, <vAdr>",
        .helpStr        = (char *) "purges an entry from the TLB"
    },
    
    {
        .helpTypeId = TYP_CMD,  .helpTokId  = CMD_WON,
        .cmdNameStr     = (char *) "won",
        .cmdSyntaxStr   = (char *) "won",
        .helpStr        = (char *) "enables windows mode / redraw"
    },

    //------------------------------------------------------------------------------------
    // Window commands.
    //
    //------------------------------------------------------------------------------------
    {
        .helpTypeId = TYP_WCMD,  .helpTokId  = CMD_WON,
        .cmdNameStr     = (char *) "woff",
        .cmdSyntaxStr   = (char *) "woff",
        .helpStr        = (char *) "disables windows mode"
    },
    
    {
        .helpTypeId = TYP_WCMD,  .helpTokId  = CMD_WDEF,
        .cmdNameStr     = (char *) "wdef",
        .cmdSyntaxStr   = (char *) "wdef",
        .helpStr        = (char *) "reset the windows to their default values"
    },
    
    {
        .helpTypeId = TYP_WCMD,  .helpTokId  = CMD_WSE,
        .cmdNameStr     = (char *) "wse",
        .cmdSyntaxStr   = (char *) "wse",
        .helpStr        = (char *) "enable window stacks"
    },
    
    {
        .helpTypeId = TYP_WCMD,  .helpTokId  = CMD_WSD,
        .cmdNameStr     = (char *) "wsd",
        .cmdSyntaxStr   = (char *) "wsd",
        .helpStr        = (char *) "disable window stacks"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WR,
        .cmdNameStr     = (char *)  "we",
        .cmdSyntaxStr   = (char *)  "we <wNum>",
        .helpStr        = (char *)  "enable window"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WR,
        .cmdNameStr     = (char *)  "wd",
        .cmdSyntaxStr   = (char *)  "wd <wNum>",
        .helpStr        = (char *)  "disable window"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WR,
        .cmdNameStr     = (char *)  "wr",
        .cmdSyntaxStr   = (char *)  "wr [ <rdx> [ , <wNum> ]]",
        .helpStr        = (char *)  "set window radix"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WF,
        .cmdNameStr     = (char *)  "wf",
        .cmdSyntaxStr   = (char *)  "wf [ <amt> ] [ , <wNum> ]",
        .helpStr        = (char *)  "move backward by n items"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WB,
        .cmdNameStr     = (char *)  "wb",
        .cmdSyntaxStr   = (char *)  "wb [ <amt> ] [ , <wNum> ]",
        .helpStr        = (char *)  "move backward by n items"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WH,
        .cmdNameStr     = (char *)  "wh",
        .cmdSyntaxStr   = (char *)  "wh [ <pos> ] [ , <wNum> ]",
        .helpStr        = (char *)  "set window home position"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WJ,
        .cmdNameStr     = (char *)  "wj",
        .cmdSyntaxStr   = (char *)  "wj <pos> [ , <wNum> ]",
        .helpStr        = (char *)  "set window start to new position"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WT,
        .cmdNameStr     = (char *)  "wt",
        .cmdSyntaxStr   = (char *)  "wt [ <wNum> ]",
        .helpStr        = (char *)  "toggle through alternate window content"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WL,
        .cmdNameStr     = (char *)  "wl",
        .cmdSyntaxStr   = (char *)  "wl <lines> [ , <wNum> ]",
        .helpStr        = (char *)  "set window lines including banner line"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WC,
        .cmdNameStr     = (char *)  "wc",
        .cmdSyntaxStr   = (char *)  "wc <wNum>",
        .helpStr        = (char *)  "set the window as current window"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WX,
        .cmdNameStr     = (char *)  "wx",
        .cmdSyntaxStr   = (char *)  "wx <wNum>",
        .helpStr        = (char *)  "exchange current window with this window"
    },
    
    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WN,
        .cmdNameStr     = (char *)  "wn",
        .cmdSyntaxStr   = (char *)  "wn <type> [ , <argStr> ]",
        .helpStr        = (char *)  "create a new window " 
                                    "( CPU, TLB, CACHE, MEM, CODE, TEXT )"
    },

     {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WK,
        .cmdNameStr     = (char *)  "wk",
        .cmdSyntaxStr   = (char *)  "wk [ <start> [ , <end> ]] | 'ALL'",
        .helpStr        = (char *)  "removes a range of windows"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_WS,
        .cmdNameStr     = (char *)  "ws",
        .cmdSyntaxStr   = (char *)  "ws <stackNum> [ , <wStart> ] [ , <wEnd>]",
        .helpStr        = (char *)  "moves a range of user windows into stack <stackNum>"
    },

    {
        .helpTypeId     = TYP_WCMD, .helpTokId  = CMD_CWL,
        .cmdNameStr     = (char *)  "cwl",
        .cmdSyntaxStr   = (char *)  "cwl <lines>",
        .helpStr        = (char *)  "set command window lines including banner line"
    },

    //------------------------------------------------------------------------------------
    // Predefined Functions.
    //
    //------------------------------------------------------------------------------------
    {
        .helpTypeId = TYP_P_FUNC,  .helpTokId  = PF_ASSEMBLE,
        .cmdNameStr     = (char *) "asm",
        .cmdSyntaxStr   = (char *) "asm ( <asmStr> )",
        .helpStr        = (char *) "returns the instruction value for an assemble string"
    },
    
    {
        .helpTypeId = TYP_P_FUNC,  .helpTokId  = PF_DIS_ASM,
        .cmdNameStr     = (char *) "disasm",
        .cmdSyntaxStr   = (char *) "disasm ( <instr> )",
        .helpStr        = (char *) "returns the assemble string for an instruction value"
    },

    {
        .helpTypeId = TYP_P_FUNC,  .helpTokId  = PF_HASH,
        .cmdNameStr     = (char *) "hash",
        .cmdSyntaxStr   = (char *) "hash ( <extAdr> )",
        .helpStr        = (char *) "returns the hash value of a virtual address"
    },


    // ??? a ton of functions for processor attributes ....
   
};

const int MAX_CMD_HELP_TAB = sizeof( cmdHelpTab ) / sizeof( SimHelpMsgEntry );

#endif  // SimTables_h
