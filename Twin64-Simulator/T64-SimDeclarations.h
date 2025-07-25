//----------------------------------------------------------------------------------------
//
//  Twin64 - A 64-bit CPU Simulator - Declarations
//
//----------------------------------------------------------------------------------------
// ...
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Simulator - Declarations
// Copyright (C) 2022 - 2025 Helmut Fieres
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
#ifndef Sim_Declarations_h
#define Sim_Declarations_h

#include "T64-Common.h"
#include "T64-ConsoleIO.h"
#include "T64-InlineAsm.h"
#include "T64-System.h"
#include "T64-Processor.h"
#include "T64-Memory.h"

//----------------------------------------------------------------------------------------
// General screen structure:
//
//          |---> column (absolute)
//          |
//          v       :--------------------------------------------------------:
//        rows      :                                                        :
//     (absolute)   :                                                        :
//                  :              Active windows shown space                :
//                  :                                                        :
//                  :--------------------------------------------------------:
//                  :                                                        :
//                  :              Command Window space                      :
//                  :                                                        :
//                  :--------------------------------------------------------:
//
// General window structure:
//
//          |---> column (relative)
//          |
//          v       :--------------------------------------------------------:
//        rows      :       Window Banner Line                               :
//      (relative)  :--------------------------------------------------------:
//                  :                                                        :
//                  :                                                        :
//                  :                                                        :
//                  :       Window Content                                   :
//                  :                                                        :
//                  :                                                        :
//                  :--------------------------------------------------------:
//
// Total size of the screen can vary. It is the sum of all active window lines
// plus the command window lines. Command window is a bit special on that it has
// an input line at the lowest line. Scroll lock after the active windows before
// the command window. Routines to move cursor, print fields with attributes.
//
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
//
//  Global Window commands:
//
//  WON, WOFF       -> on, off
//  WDEF            -> window defaults, show initial screen.
//
//  Stacks:
//
//  WSE, WSD        -> winStackEnable/Disable
//
//  Window:
//
//  enable, disable -> winEnable        -> E, D
//  back, forward   -> winMove          -> B, F
//  home, jump      -> winJump          -> H, J
//  rows            -> setRows          -> L
//  radix           -> setRadix         -> R
//  new             -> newUserWin       -> N
//  kill            -> winUserKill      -> K
//  current         -> currentUserWin   -> C
//  toggle          -> winToggle        -> T
//
//  Windows:
//
//  Processor State -> PROC
//. TLB Window      -> TLB
//  Cache Window    -> CACHE
//  Program Code    -> CODE
//  Text Window     -> TEXT
//  Commands        -> n/a
//
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// General maximum size for commands, etc.
//
//----------------------------------------------------------------------------------------
const int MAX_CMD_HIST_BUF_SIZE     = 32;

const int MAX_WIN_CMD_LINES         = 64;
const int CMD_LINE_BUF_SIZE         = 256;
const int MAX_WIN_OUT_LINES         = 256;
const int MAX_WIN_OUT_LINE_SIZE     = 256;

const int TOK_STR_SIZE              = 256;
const int MAX_TOKEN_NAME_SIZE       = 32;
const int MAX_ENV_NAME_SIZE         = 32;
const int MAX_ENV_VARIABLES         = 256;

//----------------------------------------------------------------------------------------
// Fundamental constants for the window system.
//
//----------------------------------------------------------------------------------------
const int MAX_TEXT_FIELD_LEN    = 132;
const int MAX_TEXT_LINE_SIZE    = 256;

const int MAX_WIN_NAME_SIZE     = 8;
const int MAX_WIN_ROW_SIZE      = 64;
const int MAX_WIN_COL_SIZE      = 256;
const int MAX_WINDOWS           = 32;
const int MAX_WIN_STACKS        = 4;

//----------------------------------------------------------------------------------------
// Windows have a type. The type is primarily used to specify what type of window to
// create.
//
//----------------------------------------------------------------------------------------
enum SimWinType : int {
    
    WT_NIL          = 0,
    WT_CMD_WIN      = 1,
    WT_CONSOLE_WIN  = 2,
    WT_TEXT_WIN     = 3,
    WT_PROC_WIN     = 4,
    WT_TLB_WIN      = 5,
    WT_CACHE_WIN    = 6,
    WT_MEM_WIN      = 7,
    WT_CODE_WIN     = 8  
};

//----------------------------------------------------------------------------------------
// Command line tokens and expression have a type.
//
//----------------------------------------------------------------------------------------
enum SimTokTypeId : uint16_t {

    TYP_NIL                 = 0,    

    TYP_NUM                 = 1,        TYP_STR                 = 2,    
    TYP_BOOL                = 3,        TYP_SYM                 = 4,       
    TYP_IDENT               = 5,        TYP_CMD                 = 6,   
    TYP_WCMD                = 7,        TYP_P_FUNC              = 8,    
    
    TYP_GREG                = 11,       TYP_CREG                = 12,   
    TYP_PSW_PREG            = 13,    
};

//----------------------------------------------------------------------------------------
// Tokens are the labels for reserved words and symbols recognized by the 
// tokenizer objects. Tokens have a name, a token id, a token type and an 
// optional value with further data.
//
//----------------------------------------------------------------------------------------
enum SimTokId : uint16_t {
    
    //------------------------------------------------------------------------------------
    // General tokens and symbols.
    //
    //------------------------------------------------------------------------------------
    TOK_NIL                 = 0,        TOK_ERR                 = 1,        
    TOK_EOS                 = 2,        TOK_COMMA               = 3,        
    TOK_PERIOD              = 4,        TOK_COLON               = 5,
    TOK_LPAREN              = 6,        TOK_RPAREN              = 7,        
    TOK_QUOTE               = 8,        

    TOK_PLUS                = 10,       TOK_MINUS               = 11,        
    TOK_MULT                = 12,       TOK_DIV                 = 13,
    TOK_MOD                 = 14,       TOK_REM                 = 15,       
    TOK_NEG                 = 16,       TOK_AND                 = 17,       
    TOK_OR                  = 18,       TOK_XOR                 = 19,
    TOK_EQ                  = 20,       TOK_NE                  = 21,       
    TOK_LT                  = 22,       TOK_GT                  = 23,       
    TOK_LE                  = 24,       TOK_GE                  = 25,
    
    //------------------------------------------------------------------------------------
    // Token symbols. They are just reserved names used in commands and functions. Their
    // type and optional value is defined in the token tables.
    //
    //------------------------------------------------------------------------------------
    TOK_IDENT               = 100,      TOK_NUM                 = 101,      
    TOK_STR                 = 102, 

    TOK_DEF                 = 400,      TOK_ALL                 = 402,
    TOK_PROC                = 103,      TOK_TLB                 = 104, 
    TOK_CACHE               = 105,      TOK_MEM                 = 107,      
    TOK_STATS               = 108,      TOK_TEXT                = 121, 
    
    TOK_DEC                 = 300,      TOK_HEX                 = 302,
    TOK_CODE                = 303,      

    //------------------------------------------------------------------------------------
    // Line Commands.
    //
    //------------------------------------------------------------------------------------
    CMD_SET                 = 1000,
    
    CMD_EXIT                = 1001,     CMD_HELP                = 1002,
    CMD_DO                  = 1003,     CMD_REDO                = 1004,     
    CMD_HIST                = 1005,     CMD_ENV                 = 1006,     
    CMD_XF                  = 1007,     CMD_WRITE_LINE          = 1008,
    CMD_DM                  = 1009,
    
    CMD_RESET               = 1009,     CMD_RUN                 = 1010,     
    CMD_STEP                = 1011,

    CMD_DR                  = 1012,     CMD_MR                  = 1013,
    CMD_DA                  = 1014,     CMD_MA                  = 1015,
    
    CMD_D_TLB               = 1016,     CMD_I_TLB               = 1017,     
    CMD_P_TLB               = 1018,

    CMD_D_CACHE             = 1019,     CMD_F_CACHE             = 1020,
    CMD_P_CACHE             = 1021,

    //------------------------------------------------------------------------------------
    // Window Commands Tokens.
    //
    //------------------------------------------------------------------------------------
    WCMD_SET                = 2000,     WTYPE_SET               = 2001,
    
    CMD_WON                 = 2002,     CMD_WOFF                = 2003,     
    CMD_WDEF                = 2004,     CMD_CWL                 = 2005,     
    CMD_WSE                 = 2006,     CMD_WSD                 = 2007,
    
    CMD_WE                  = 2050,     CMD_WD                  = 2051,     
    CMD_WR                  = 2052,     CMD_WF                  = 2053,     
    CMD_WB                  = 2054,     CMD_WH                  = 2055,
    CMD_WJ                  = 2056,     CMD_WL                  = 2057,     
    CMD_WN                  = 2058,     CMD_WK                  = 2059,     
    CMD_WS                  = 2060,     CMD_WC                  = 2061,
    CMD_WT                  = 2062,     CMD_WX                  = 2063,
    
    //------------------------------------------------------------------------------------
    // Predefined Function Tokens.
    //
    //------------------------------------------------------------------------------------
    PF_SET                  = 3000,
    PF_ASSEMBLE             = 3001,     PF_DIS_ASM         = 3002,     

    // ??? rethink... what is needed...
    PF_HASH                 = 3003,     PF_S32                  = 3005,   
    
    //------------------------------------------------------------------------------------
    // General, Segment and Control Registers Tokens.
    //
    //------------------------------------------------------------------------------------
    REG_SET                 = 4000,
    
    GR_0                    = 4100,     GR_1                    = 4101,     
    GR_2                    = 4102,     GR_3                    = 4103,     
    GR_4                    = 4104,     GR_5                    = 4105,
    GR_6                    = 4106,     GR_7                    = 4107,    
    GR_8                    = 4108,     GR_9                    = 4109,     
    GR_10                   = 4110,     GR_11                   = 4111,
    GR_12                   = 4112,     GR_13                   = 4113,     
    GR_14                   = 4114,     GR_15                   = 4115,     
    GR_SET                  = 4116,
    
    CR_0                    = 4200,     CR_1                    = 4201,     
    CR_2                    = 4202,     CR_3                    = 4203,     
    CR_4                    = 4204,     CR_5                    = 4205,
    CR_6                    = 4206,     CR_7                    = 4207,     
    CR_8                    = 4208,     CR_9                    = 4209,     
    CR_10                   = 4210,     CR_11                   = 4211,
    CR_12                   = 4212,     CR_13                   = 4213,     
    CR_14                   = 4214,     CR_15                   = 4215,       
    CR_SET                  = 4216,
};

//----------------------------------------------------------------------------------------
// Our error messages IDs. There is a routine that maps the ID to a text string.
//
// ??? clean up, keep the ones we need...
//----------------------------------------------------------------------------------------
enum SimErrMsgId : uint16_t {
    
    NO_ERR                          = 0,
    ERR_NOT_SUPPORTED               = 1,
    ERR_NOT_IN_WIN_MODE             = 2,
    ERR_TOO_MANY_ARGS_CMD_LINE      = 3,
    ERR_EXTRA_TOKEN_IN_STR          = 4,
    ERR_NUMERIC_OVERFLOW            = 5,
    
    ERR_INVALID_CMD                 = 10,
    ERR_INVALID_ARG                 = 11,
    ERR_INVALID_WIN_STACK_ID        = 12,
    ERR_INVALID_WIN_ID              = 13,
    ERR_INVALID_WIN_TYPE            = 14,
    ERR_INVALID_EXIT_VAL            = 15,
    ERR_INVALID_RADIX               = 16,
    ERR_INVALID_REG_ID              = 17,
    ERR_INVALID_STEP_OPTION         = 18,
    ERR_INVALID_CHAR_IN_TOKEN_LINE  = 19,
    ERR_INVALID_EXPR                = 20,
    ERR_INVALID_INSTR_OPT           = 21,
    ERR_INVALID_INSTR_MODE          = 22,
    ERR_INVALID_FMT_OPT             = 23,
    ERR_INVALID_NUM                 = 24,
    ERR_INVALID_CHAR_IN_IDENT       = 25,
    ERR_INVALID_REG_COMBO           = 26,
    ERR_INVALID_OP_CODE             = 27,
    ERR_INVALID_S_OP_CODE           = 28,
    ERR_INVALID_CMD_ID              = 29,
    
    ERR_EXPECTED_COMMA              = 100,
    ERR_EXPECTED_LPAREN             = 101,
    ERR_EXPECTED_RPAREN             = 102,
    ERR_EXPECTED_NUMERIC            = 103,
    ERR_EXPECTED_EXT_ADR            = 104,
    ERR_EXPECTED_FILE_NAME          = 105,
    ERR_EXPECTED_WIN_ID             = 106,
    ERR_EXPECTED_WIN_TYPE           = 107,
    ERR_EXPECTED_STACK_ID           = 108,
    ERR_EXPECTED_REG_OR_SET         = 109,
    ERR_EXPECTED_REG_SET            = 110,
    ERR_EXPECTED_GENERAL_REG        = 111,
    ERR_EXPECTED_SEGMENT_REG        = 312,
    ERR_EXPECTED_OFS                = 213,
    ERR_EXPECTED_START_OFS          = 214,
    ERR_EXPECTED_LEN                = 215,
    ERR_EXPECTED_STEPS              = 116,
    ERR_EXPECTED_INSTR_VAL          = 117,
    ERR_EXPECTED_INSTR_OPT          = 318,
    ERR_EXPECTED_SR1_SR3            = 319,
    ERR_EXPECTED_LOGICAL_ADR        = 320,
    ERR_EXPECTED_AN_OFFSET_VAL      = 321,
    ERR_EXPECTED_FMT_OPT            = 322,
    ERR_EXPECTED_CLOSING_QUOTE      = 323,
    ERR_EXPECTED_STR                = 324,
    ERR_EXPECTED_EXPR               = 325,
    
    ERR_UNEXPECTED_EOS              = 350,
    
    ERR_ENV_VAR_NOT_FOUND           = 400,
    ERR_ENV_VALUE_EXPR              = 401,
    ERR_ENV_PREDEFINED              = 403,
    ERR_ENV_TABLE_FULL              = 404,
    ERR_OPEN_EXEC_FILE              = 405,
    
    ERR_EXPR_TYPE_MATCH             = 406,
    ERR_EXPR_FACTOR                 = 407,

    ERR_OFS_LEN_LIMIT_EXCEEDED      = 408,
    ERR_INSTR_HAS_NO_OPT            = 409,
    ERR_IMM_VAL_RANGE               = 410,
    ERR_INSTR_MODE_OPT_COMBO        = 411,
    ERR_POS_VAL_RANGE               = 412,
    ERR_LEN_VAL_RANGE               = 413,
    ERR_OFFSET_VAL_RANGE            = 414,
    
    ERR_OUT_OF_WINDOWS              = 415,
    ERR_WIN_TYPE_NOT_CONFIGURED     = 416,
    
    ERR_UNDEFINED_PFUNC             = 417,

    ERR_TLB_TYPE                    = 500,
    ERR_TLB_PURGE_OP                = 501,
    ERR_TLB_INSERT_OP               = 502,
    ERR_TLB_ACC_DATA                = 503,
    ERR_TLB_ADR_DATA                = 504,
    ERR_TLB_NOT_CONFIGURED          = 505,
    ERR_TLB_SIZE_EXCEEDED           = 506,
    
    ERR_CACHE_TYPE                  = 600,
    ERR_CACHE_PURGE_OP              = 601,
    ERR_CACHE_SET_NUM               = 602,
    ERR_CACHE_NOT_CONFIGURED        = 603,
    ERR_CACHE_SIZE_EXCEEDED         = 604,
};

//----------------------------------------------------------------------------------------
// Predefined environment variable names. When you create another one, put its name
// here.
//
// ??? what to keep....
//----------------------------------------------------------------------------------------
const char ENV_TRUE[ ]                  = "TRUE";
const char ENV_FALSE[ ]                 = "FALSE";

const char ENV_PROG_VERSION [ ]         = "PROG_VERSION";
const char ENV_PATCH_LEVEL [ ]          = "PATCH_LEVEL";
const char ENV_GIT_BRANCH[ ]            = "GIT_BRANCH";

const char ENV_SHOW_CMD_CNT[ ]          = "SHOW_CMD_CNT" ;
const char ENV_CMD_CNT[ ]               = "CMD_CNT" ;
const char ENV_ECHO_CMD_INPUT[ ]        = "ECHO_CMD_INPUT";
const char ENV_EXIT_CODE [ ]            = "EXIT_CODE";

const char ENV_RDX_DEFAULT [ ]          = "RDX_DEFAULT";
const char ENV_WORDS_PER_LINE [ ]       = "WORDS_PER_LINE";
const char ENV_WIN_MIN_ROWS[ ]          = "WIN_MIN_ROWS";
const char ENV_WIN_TEXT_LINE_WIDTH[ ]   = "WIN_TEXT_WIDTH";

const char ENV_CURRENT_PROC[ ]          = "CURRENT_PROC";

//----------------------------------------------------------------------------------------
// Forward declaration of the globals structure. Every object will have access to the
// globals structure, so we do not have to pass around references to all the individual
// objects.
//
//----------------------------------------------------------------------------------------
struct SimGlobals;

//----------------------------------------------------------------------------------------
// An error is described in the error message table.
//
//----------------------------------------------------------------------------------------
struct SimErrMsgTabEntry {
    
    SimErrMsgId errNum;
    char        *errStr;
};

//----------------------------------------------------------------------------------------
// An help message is described in the help message table.
//
//----------------------------------------------------------------------------------------
struct SimHelpMsgEntry {
    
    SimTokTypeId    helpTypeId;
    SimTokId        helpTokId;
    char            *cmdNameStr;
    char            *cmdSyntaxStr;
    char            *helpStr;
};

//----------------------------------------------------------------------------------------
// The command line interpreter works the command line as a list of tokens. A token 
// found in a string is recorded using the token structure. The token types are 
// numeric and strings. The string is a buffer in the tokenizer. Scanning a new token
// potentially overwrites or invalidates the string. You need to copy it to a safe 
// place before scanning the next token.
//
//----------------------------------------------------------------------------------------
struct SimToken {

    char         name[ MAX_TOKEN_NAME_SIZE ] = { };
    SimTokTypeId typ                         = TYP_NIL;
    SimTokId     tid                         = TOK_NIL;
    
    union {
        
        T64Word val;
        char    *str; 

    } u;
};

//----------------------------------------------------------------------------------------
// Tokenizer object. The command line interface parse their input buffer line. The 
// tokenizer will return the tokens found in the line. The tokenizer raises exceptions.
//
//----------------------------------------------------------------------------------------
struct SimTokenizer {

    public:

    SimTokenizer( );

    void            setupTokenizer( char *lineBuf, SimToken *tokTab );
    void            nextToken( );
    
    bool            isToken( SimTokId tokId );
    bool            isTokenTyp( SimTokTypeId typId );

    SimToken        token( );
    SimTokTypeId    tokTyp( );
    SimTokId        tokId( );
    T64Word         tokVal( );
    char            *tokStr( );
    
    int             tokCharIndex( );
    char            *tokenLineStr( );

    private:
    
    void            nextChar( );
    void            parseNum( );
    void            parseString( );
    void            parseIdent( );

    SimToken        currentToken;
    int             currentLineLen          = 0;
    int             currentCharIndex        = 0;
    int             currentTokCharIndex     = 0;
    char            currentChar             = ' ';
    SimToken        *tokTab                 = nullptr;
    char            tokenLine[ 256 ]        = { 0 };
    char            strTokenBuf[ 256 ]      = { 0 };    
};

//----------------------------------------------------------------------------------------
// Expression value. The analysis of an expression results in a value. Depending on 
// the expression type, the values are simple scalar values or a structured values.
//
//----------------------------------------------------------------------------------------
struct SimExpr {
    
    SimTokTypeId typ;
   
    union {

        T64Word val;
        bool    bVal;  
        char    *str;

    } u;
};

//----------------------------------------------------------------------------------------
// The expression evaluator object. We use the "parseExpr" routine wherever we expect
// an expression in the command line. The evaluator raises exceptions.
//
//----------------------------------------------------------------------------------------
struct SimExprEvaluator {
    
    public:
    
    SimExprEvaluator( SimGlobals *glb, SimTokenizer *tok );
    
    void            setTokenizer( SimTokenizer *tok );
    void            parseExpr( SimExpr *rExpr );
    
    private:
    
    void            parseTerm( SimExpr *rExpr );
    void            parseFactor( SimExpr *rExpr );

    void            parsePredefinedFunction( SimToken funcId, SimExpr *rExpr );    
    void            pFuncAssemble( SimExpr *rExpr );
    void            pFuncDisAssemble( SimExpr *rExpr );
    void            pFuncHash( SimExpr *rExpr );
    void            pFuncS32( SimExpr *rExpr );
    void            pFuncU32( SimExpr *rExpr );
    
    SimGlobals      *glb        = nullptr;
    SimTokenizer    *tok        = nullptr;
    T64Assemble     *inlineAsm  = nullptr;
    T64DisAssemble  *disAsm     = nullptr;
};

//----------------------------------------------------------------------------------------
// Environment table entry, Each environment variable has a name, a couple of flags 
// and the value. There are predefined variables and user defined variables.
//
//----------------------------------------------------------------------------------------
struct SimEnvTabEntry {
    
    char            name[ MAX_ENV_NAME_SIZE ]   = { 0 };
    bool            valid                       = false;
    bool            predefined                  = false;
    bool            readOnly                    = false;
    SimTokTypeId    typ                         = TYP_NIL;
    
    union {
    
        bool        bVal; 
        T64Word     iVal;        
        char        *strVal;      

    } u;
};

//----------------------------------------------------------------------------------------
// Environment variables. The simulator has a global table where all variables are 
// kept. It is a simple array with a high water mark concept. The table will be 
// allocated at simulator start.
//
//----------------------------------------------------------------------------------------
struct SimEnv {

    public:
    
    SimEnv( int size );
    
    void            displayEnvTable( );
    void            displayEnvTableEntry( char *name );
    void            setupPredefined( );
    
    void            setEnvVar( char *name, T64Word val );
    void            setEnvVar( char *name, bool val );
    void            setEnvVar( char *name, char *str );
    void            removeEnvVar( char *name );
    
    bool            getEnvVarBool( char *name,bool def = false );
    T64Word         getEnvVarInt( char *name, T64Word def = 0 );
    char            *getEnvVarStr( char *name, char *def = nullptr );
    SimEnvTabEntry  *getEnvEntry( char *name );
    
    bool            isValid( char *name );
    bool            isReadOnly( char *name );
    bool            isPredefined( char *name );
    
    private:
    
    int             lookupEntry( char *name );
    int             findFreeEntry( );
    
    void            enterVar( char *name, 
                              T64Word val, 
                              bool predefined = false, 
                              bool rOnly = false );

    void            enterVar( char *name, 
                              bool val, 
                              bool predefined = false, 
                              bool rOnly = false );

    void            enterVar( char *name, 
                              char *str, 
                              bool predefined = false, 
                              bool rOnly = false );
    
    void            displayEnvEntry( SimEnvTabEntry *entry );
    
    SimEnvTabEntry  *table  = nullptr;
    SimEnvTabEntry  *hwm    = nullptr;
    SimEnvTabEntry  *limit  = nullptr;
};

//----------------------------------------------------------------------------------------
// Command History. The simulator command interpreter features a simple command history.
// It is a circular buffer that holds the last commands. There are functions to show
// the command history, re-execute a previous command and to retrieve a previous 
// command for editing.
//
//----------------------------------------------------------------------------------------
struct SimCmdHistEntry {
    
    int  cmdId;
    char cmdLine[ CMD_LINE_BUF_SIZE ];
};

struct SimCmdHistory {
    
    public:
    
    SimCmdHistory( );
    
    void addCmdLine( char *cmdStr );
    char *getCmdLine( int cmdRef, int *cmdId = nullptr );
    int  getCmdCount( );
    int  getCmdNum( );
   
    private:
    
    int nextCmdNum          = 0;
    int head                = 0;
    int tail                = 0;
    int count               = 0;
    
    SimCmdHistEntry history[ MAX_CMD_HIST_BUF_SIZE ];
};

//----------------------------------------------------------------------------------------
// Command and Console Window output buffer. The output buffer will store all output
// from the command window to support scrolling. This is the price you pay when normal
// terminal scrolling is restricted to an area of the screen. The buffer offers a
// simple interface. Any character added will be stored in a line, a "\n" will advance
// to the next line to store. The buffer itself is a circular buffer. Each time a 
// command line is entered, the display will show the last N lines entered. A cursor
// is defined which is manipulated by the cursor up or down routines.
//
//----------------------------------------------------------------------------------------
struct SimWinOutBuffer : SimFormatter {
    
    public:
    
    SimWinOutBuffer( );
    
    void        initBuffer( );
    void        addToBuffer( const char *data );
    int         writeChars( const char *format, ... );
    int         writeChar( const char ch );
    void        setScrollWindowSize( int size );
    
    void        resetLineCursor( );
    char        *getLineRelative( int lineBelowTop );
    int         getCursorIndex( );
    int         getTopIndex( );
    
    void        scrollUp( int lines = 1 );
    void        scrollDown( int lines = 1 );
    
    private:
    
    char        buffer[ MAX_WIN_OUT_LINES ] [ MAX_WIN_OUT_LINE_SIZE ];
    int         topIndex    = 0; // Index of the next line to use.
    int         cursorIndex = 0; // Index of the last line currently shown.
    int         screenLines = 0; // Number of lines displayed in the window.
    int         charPos     = 0; // Current character position in the line.
};

//----------------------------------------------------------------------------------------
// The "SimWin" class. The simulator will in screen mode feature a set of stacks each 
// with a list of screen sub windows. The default is one stack, the general register
// set window and the command line window, which also spans all stacks. Each sub window
// is an instance of a specific window class with this class as the base class. There
// are routines common to all windows to enable/ disable, set the lines displayed and
// so on. There are also abstract methods that the inheriting class needs to implement.
// Examples are to initialize a window, redraw and so on.
//
//----------------------------------------------------------------------------------------
struct SimWin {
    
    public:
    
    SimWin( SimGlobals *glb );
    virtual         ~ SimWin( );
    
    void            setWinType( int type );
    int             getWinType( );
    
    void            setWinIndex( int index );
    int             getWinIndex( );
    
    void            setEnable( bool arg );
    bool            isEnabled( );
    
    virtual void    setRadix( int radix );
    virtual int     getRadix( );
    
    void            setRows( int arg );
    int             getRows( );
    
    void            setDefColumns( int rdx10, int rdx16 );
    int             getDefColumns( int rdx = 16 );
    
    void            setColumns( int arg );
    int             getColumns( );
    
    void            setWinOrigin( int row, int col );
    void            setWinCursor( int row, int col );
    
    int             getWinCursorRow( );
    int             getWinCursorCol( );
    
    int             getWinStack( );
    void            setWinStack( int wStack );

    void            setWinToggleLimit( int limit );
    int             getWinToggleLimit( );
    
    int             getWinToggleVal( );
    void            setWinToggleVal( int val );
    
    char            *getWinName( );
    
    void            printNumericField(  T64Word val,
                                        uint32_t fmtDesc = 0,
                                        int len = 0,
                                        int row = 0,
                                        int col = 0 );
    
    void            printTextField( char *text,
                                    uint32_t fmtDesc = 0,
                                    int len = 0,
                                    int row = 0,
                                    int col = 0 );
    
    void            printRadixField( uint32_t fmtDesc = 0,
                                     int len = 0,
                                     int row = 0,
                                     int col = 0 );
    
    void            printWindowIdField( int stack,
                                        int index,
                                        char *name,
                                        bool current = false,
                                        uint32_t fmtDesc = 0,
                                        int row = 0,
                                        int col = 0 );
    
    void            padLine( uint32_t fmtDesc = 0 );
    void            padField( int dLen, int fLen );
    void            clearField( int len, uint32_t fmtDesc = 0 ); 
    
    void            reDraw( );
    
    virtual void    toggleWin( );
    virtual void    setDefaults( )  = 0;
    virtual void    drawBanner( )   = 0;
    virtual void    drawBody( )     = 0;
    
    protected:
    
    SimGlobals      *glb;
   
    private:
    
    int             winType             = WT_NIL;
    int             winIndex            = 0;
    
    bool            winEnabled          = false;
    int             winRadix            = 16;
    int             winStack            = 0;
    int             winRows             = 0;
    int             winColumns          = 0;
    int             winDefColumnsHex    = 0;
    int             winDefColumnsDec    = 0;
    int             winToggleLimit      = 0;
    int             winToggleVal        = 0;
    
    int             winAbsCursorRow     = 0;
    int             winAbsCursorCol     = 0;
    int             lastRowPos          = 0;
    int             lastColPos          = 0;

    char            winName[ MAX_WIN_NAME_SIZE + 1 ] = { 0 };
};

//----------------------------------------------------------------------------------------
// "WinScrollable" is an extension to the basic window. It implements scrollable
// windows with a number of lines. There is a high level concept of a starting index
// of zero and a limit. The meaning i.e. whether the index is a memory address or an
// index into a TLB or Cache array is determined by the inheriting class. The 
// scrollable window will show a number of lines, the "drawLine" method needs to be
//  implemented by the inheriting class. The routine is passed the item address for 
// the line and is responsible for the correct address interpretation. The 
// "lineIncrement" is the increment value for the item address passed.
//
//----------------------------------------------------------------------------------------
struct SimWinScrollable : SimWin {
    
    public:
    
    SimWinScrollable( SimGlobals *glb );
    
    void            setHomeItemAdr( T64Word adr );
    T64Word         getHomeItemAdr( );

    void            setCurrentItemAdr( T64Word adr );
    T64Word         getCurrentItemAdr( );
    
    void            setLimitItemAdr( T64Word adr );
    T64Word         getLimitItemAdr( );
    
    void            setLineIncrement( int arg );
    int             getLineIncrement( );

    void            winHome( T64Word pos = 0 );
    void            winJump( T64Word pos );
    void            winForward( T64Word amt );
    void            winBackward( T64Word amt );
   
    
    virtual void    drawBody( );
    virtual void    drawLine( T64Word index ) = 0;
    
    private:
    
    T64Word         homeItemAdr         = 0;
    T64Word         currentItemAdr      = 0;
    T64Word         limitItemAdr        = 0;
    T64Word         lineIncrement       = 0;
};

//----------------------------------------------------------------------------------------
// Program State Register Window. This window holds the programmer visible state with
// the exception of the program relevant control register values. They are a separate
// window.
//
//----------------------------------------------------------------------------------------
struct SimWinProgState : SimWin {
    
    public:
    
    SimWinProgState( SimGlobals *glb );
    
    void setDefaults( );
    void setRadix( int rdx );
    void drawBanner( );
    void drawBody( );
};

//----------------------------------------------------------------------------------------
// Absolute Memory Window. A memory window will show the absolute memory content
// starting with the current address followed by a number of data words. The number
// of words shown is the number of lines of the window times the number of items, 
// i.e. words, on a line.
//
//----------------------------------------------------------------------------------------
struct SimWinAbsMem : SimWinScrollable {
    
    public:
    
    SimWinAbsMem( SimGlobals *glb );
    
    void setDefaults( );
    void setRadix( int rdx );
    void drawBanner( );
    void drawLine( T64Word index );
};

//----------------------------------------------------------------------------------------
// Code Memory Window. A code memory window will show the instruction memory content
// starting with the current address followed by the instruction and a human readable
// disassembled version.
//
//----------------------------------------------------------------------------------------
struct SimWinCode : SimWinScrollable {
    
    public:
    
    SimWinCode( SimGlobals *glb );
    ~ SimWinCode( );
    
    void setDefaults( );
    void drawBanner( );
    void drawLine( T64Word index );
    
    private:

    T64DisAssemble *disAsm = nullptr;
};

//----------------------------------------------------------------------------------------
// TLB Window. The TLB data window displays the TLB entries.
//
//----------------------------------------------------------------------------------------
struct SimWinTlb : SimWinScrollable {
    
    public:
    
    SimWinTlb( SimGlobals *glb, int winType );
    
    void setDefaults( );
    void setRadix( int rdx );
    void drawBanner( );
    void drawLine( T64Word index );
};

//----------------------------------------------------------------------------------------
// Cache Window. The memory object window display the cache date lines. Since we can
// have caches with more than one set, the toggle function allows to flip through the
// sets, one at a time.
//
//----------------------------------------------------------------------------------------
struct SimWinCache : SimWinScrollable {
    
    public:
    
    SimWinCache( SimGlobals *glb, int winType );
    
    void setDefaults( );
    void setRadix( int rdx );
    void drawBanner( );
    void drawLine( T64Word index );
};

//----------------------------------------------------------------------------------------
// Text Window. It may be handy to also display an ordinary ASCII text file. One day
// this will allow us to display for example the source code to a running program 
// when symbolic debugging is supported.
//
//----------------------------------------------------------------------------------------
struct SimWinText : SimWinScrollable {
    
    public:
    
    SimWinText( SimGlobals *glb, char *fName );
    ~ SimWinText( );
    
    void    setDefaults( );
    void    drawBanner( );
    void    drawLine( T64Word index );
    
    private:

    bool    openTextFile( );
    int     readTextFileLine( int linePos, char *lineBuf, int bufLen );
    
    FILE    *textFile          = nullptr;
    int     fileSizeLines      = 0;
    int     lastLinePos        = 0;
    char    fileName[ 256 ]    = { 0 };
};

//----------------------------------------------------------------------------------------
// Console Window. When the CPU is running, it has access to a "console window". This
// is a rather simple console IO window. Care needs to be taken however what character
// IO directed to this window means. For example, escape sequences cannot be just 
// printed out as it would severely impact the simulator windows. Likewise scrolling 
// and line editing are to be handheld.
//
//----------------------------------------------------------------------------------------
struct SimWinConsole : SimWin {
    
public:
    
    SimWinConsole( SimGlobals *glb );
   
    void    setDefaults( );
    void    drawBanner( );
    void    drawBody( );
    
    void    putChar( char ch );
    
    // ??? methods to read a character ?
    // ??? methods to switch between command and console mode ?
    
private:
    
    SimGlobals      *glb    = nullptr;
    SimWinOutBuffer *winOut = nullptr;
};

//----------------------------------------------------------------------------------------
// Command Line Window. The command window is a special class, which comes always last
// in the windows list and cannot be disabled. It is intended to be a scrollable window,
// where only the banner line is fixed.
//
//----------------------------------------------------------------------------------------
struct SimCommandsWin : SimWin {
    
public:
    
    SimCommandsWin( SimGlobals *glb );
    
    void            setDefaults( );
    void            drawBanner( );
    void            drawBody( );
    SimTokId        getCurrentCmd( );
    void            cmdInterpreterLoop( );
    
private:
    
    void            printWelcome( );
    int             buildCmdPrompt( char *promptStr, int promptStrLen );
    int             readCmdLine( char *cmdBuf, int cmdBufLen, char *promptStr );
    
    void            evalInputLine( char *cmdBuf );
    void            cmdLineError( SimErrMsgId errNum, char *argStr = nullptr );
    int             promptYesNoCancel( char *promptStr );
  
    void            checkEOS( );
    void            acceptComma( );
    void            acceptLparen( );
    void            acceptRparen( );
    
    void            displayAbsMemContent( T64Word ofs, T64Word len, int rdx = 16 );
    void            displayAbsMemContentAsCode( T64Word ofs, T64Word len, int rdx = 16 );
    
    void            displayTlbEntry( T64TlbEntry *entry, int rdx = 16 );
    void            displayTlbEntries( T64Tlb *tlb, int index, int len, int rdx = 16 );
    void            displayCacheLine( T64CacheLine *line, int rdx = 16 );
    void            displayCacheEntries( T64Cache *cache, int index, int len, int rdx = 16 );

    void            exitCmd( );
    void            helpCmd( );
    void            envCmd( );
    void            execFileCmd( );
    void            writeLineCmd( );
    void            execCmdsFromFile( char* fileName );
    
    void            histCmd( );
    void            doCmd( );
    void            redoCmd( );
    
    void            resetCmd( );
    void            runCmd( );
    void            stepCmd( );
   
    void            modifyRegCmd( );
    
    void            displayAbsMemCmd( );
    void            modifyAbsMemCmd( );
    
    void            displayCacheCmd( );
    void            purgeCacheCmd( );
    void            flushCacheCmd( );
    
    void            displayTLBCmd( );
    void            insertTLBCmd( );
    void            purgeTLBCmd( );
    
    void            winOnCmd( );
    void            winOffCmd( );
    void            winDefCmd( );
    void            winStacksEnable( );
    void            winStacksDisable( );

    void            winCurrentCmd( );
    void            winEnableCmd( );
    void            winDisableCmd( );
    void            winSetRadixCmd( );
    
    void            winForwardCmd( );
    void            winBackwardCmd( );
    void            winHomeCmd( );
    void            winJumpCmd( );
    void            winSetRowsCmd( );
    void            winSetCmdWinRowsCmd( );
    void            winNewWinCmd( );
    void            winKillWinCmd( );
    void            winSetStackCmd( );
    void            winToggleCmd( );
    void            winExchangeCmd( );

    private:
    
    SimGlobals          *glb            = nullptr;
    SimCmdHistory       *hist           = nullptr;
    SimTokenizer        *tok            = nullptr;
    SimExprEvaluator    *eval           = nullptr;
    SimWinOutBuffer     *winOut         = nullptr;
    T64Assemble         *inlineAsm      = nullptr;
    T64DisAssemble      *disAsm         = nullptr;   
    SimTokId            currentCmd      = TOK_NIL;
   
};

//----------------------------------------------------------------------------------------
// The window display screen object is the central object of the simulator. Commands
// send from the command input will eventually end up as calls to this object. A 
// simulator screen is an ordered list of windows. Although you can disable a window
// such that it disappears on the screen, when enabled, it will show up in the place
// intended for it. For example, the program state register window will always be on
// top, followed by the special regs. The command input scroll area is always last and
// is the only window that cannot be disabled. In addition, windows can be grouped in
// stacks that are displayed next to each other. The exception is the command window
// area which is always displayed across the entire terminal window width.
//
//----------------------------------------------------------------------------------------
struct SimWinDisplay {
    
public:
    
    SimWinDisplay( SimGlobals *glb );
    
    void            setupWinDisplay( int argc, const char *argv[ ] );
    void            startWinDisplay( );
    SimTokId        getCurrentCmd( );
    bool            isWinModeOn( );
    
    void            reDraw( bool mustRedraw = false );
    void            setWinMode( bool winOn );
    
    void            windowsOn( );
    void            windowsOff( );
    void            windowDefaults( );
    void            windowCurrent( int winNum = 0 );
    void            windowEnable( int winNum = 0, bool show = true );
    void            winStacksEnable( bool arg );
    void            windowRadix( int rdx, int winNum = 0 );
    void            windowSetRows( int rows, int winNum = 0 );
    void            windowSetCmdWinRows( int rows );
    
    void            windowHome( int amt, int winNum = 0 );
    void            windowForward( int amt, int winNum = 0 );
    void            windowBackward( int amt, int winNum = 0 );
    void            windowJump( int amt, int winNum = 0 );
    void            windowToggle( int winNum = 0 );
    void            windowExchangeOrder( int winNum );
    void            windowNew( SimTokId winType = TOK_NIL, char *argStr = nullptr );
    void            windowKill( int winNumStart, int winNumEnd = 0  );
    void            windowSetStack( int winStack, int winNumStart, int winNumEnd = 0 );
    
    int             getCurrentWindow( );
    void            setCurrentWindow( int winNum );
    bool            isCurrentWin( int winNum );
    bool            isWinEnabled( int winNum );
    bool            isWindowsOn( );

    bool            validWindowType( SimTokId winType );
    bool            validWindowNum( int winNum );
    bool            validWindowStackNum( int winNum );

    private:
    
    int             computeColumnsNeeded( int winStack );
    int             computeRowsNeeded( int winStack );
    void            setWindowColumns( int winStack, int columns );
    void            setWindowOrigins( int winStack, int rowOfs = 1, int colOfs = 1 );
   
    int             currentWinNum               = -1;
    bool            winStacksOn                 = true;
    bool            winModeOn                   = true;

    SimGlobals      *glb                        = nullptr;
   
    SimWin          *windowList[ MAX_WINDOWS ]  = { nullptr };

    public: 
     
    SimCommandsWin  *cmdWin                     = nullptr;
    
};

//----------------------------------------------------------------------------------------
// The globals, accessible to all objects. To ease the passing around there is the
// idea a global structure with a reference to all the individual objects.
//
//----------------------------------------------------------------------------------------
struct SimGlobals {
    
    SimConsoleIO        *console        = nullptr;
    SimEnv              *env            = nullptr;
    SimWinDisplay       *winDisplay     = nullptr;
    
    T64System           *system         = nullptr;
};

#endif  // Sim_Declarations_h
