//----------------------------------------------------------------------------------------
//
//  Twin-64 - System Programming Language Compiler
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// Twin-64 - System Programming Language Compiler
// Copyright (C) 2020 - 2026 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under 
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details. You 
// should have received a copy of the GNU General Public License along with this 
// program. If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#pragma once

#include "T64-Common.h"
#include "T64-SplVersion.h"
#include "T64-SplDeclarations.h"

//----------------------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------------------

// ??? rework .... what is needed ?

//----------------------------------------------------------------------------------------
// Tokens are the labels for reserved words and symbols recognized by the tokenizer 
// objects. Tokens have a name, a token id, a token type and an optional value with 
// further data. See also the "SimTables" include file how types and token Id are 
// used to build the command and expression tokens.
//
//----------------------------------------------------------------------------------------
enum SimTokId : uint16_t {

   

   
    //------------------------------------------------------------------------------------
    // Predefined Function Tokens.
    //
    //------------------------------------------------------------------------------------
    PF_SET                  = 500,
    PF_ASSEMBLE             = 501,      PF_DIS_ASM              = 502,
    PF_HASH                 = 503,      PF_S32                  = 504,

    //------------------------------------------------------------------------------------
    // General, Control and PSW Register Tokens.
    //
    //------------------------------------------------------------------------------------
    REG_SET                 = 600,

    GR_0                    = 610,      GR_1                    = 611,
    GR_2                    = 612,      GR_3                    = 613,
    GR_4                    = 614,      GR_5                    = 615,
    GR_6                    = 616,      GR_7                    = 617,
    GR_8                    = 618,      GR_9                    = 619,
    GR_10                   = 620,      GR_11                   = 621,
    GR_12                   = 622,      GR_13                   = 623,
    GR_14                   = 624,      GR_15                   = 625,
    GR_SET                  = 626,

    CR_0                    = 630,      CR_1                    = 631,
    CR_2                    = 632,      CR_3                    = 633,
    CR_4                    = 634,      CR_5                    = 635,
    CR_6                    = 636,      CR_7                    = 637,
    CR_8                    = 638,      CR_9                    = 639,
    CR_10                   = 640,      CR_11                   = 641,
    CR_12                   = 642,      CR_13                   = 643,
    CR_14                   = 644,      CR_15                   = 645,
    CR_SET                  = 646,

    PR_IA                   = 650,      PR_ST                   = 651
};



//----------------------------------------------------------------------------------------
// Command line option argument types and structure. This is the argc, argv parser
// used to parse long options (e.g. --option=value).
//
//----------------------------------------------------------------------------------------
enum SplCmdLineArgOptions : int {

    OPT_NO_ARGUMENT         = 0,
    OPT_REQUIRED_ARGUMENT   = 1, 
    OPT_OPTIONAL_ARGUMENT   = 2    
};

//----------------------------------------------------------------------------------------
// An command line option is described in the command line option table.
//
//----------------------------------------------------------------------------------------
struct SplCmdLineOptions {

    const char              *name;
    SplCmdLineArgOptions    argOpt;   
    int                     val;
};

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
enum SplTokenId : int {
    
    TOK_NIL         = 0,    TOK_EOL         = 1,    TOK_EOF         = 2,     
    TOK_IDENT       = 3,    TOK_NUM         = 4,    TOK_STRING      = 5,
    TOK_COMMA       = 6,    TOK_PERIOD      = 7,    TOK_SEMICOLON   = 8,
    TOK_LPAREN      = 9,    TOK_RPAREN      = 10,   
    
    TOK_PLUS                = 110,      TOK_MINUS               = 111,
    TOK_MULT                = 112,      TOK_DIV                 = 113,
    TOK_MOD                 = 114,      TOK_REM                 = 115,
    TOK_NEG                 = 116,      TOK_AND                 = 117,
    TOK_OR                  = 118,      TOK_XOR                 = 119,
    TOK_EQ                  = 120,      TOK_NE                  = 121,
    TOK_LT                  = 122,      TOK_GT                  = 123,
    TOK_LE                  = 124,      TOK_GE                  = 125,

    TOK_GENERAL_REG = 12,   TOK_CONTROL_REG = 13,

    // ??? a ton of reserved words...
};

//----------------------------------------------------------------------------------------
// A source file is opened for reading characters. We keep track of the current 
// line number and column number for error reporting.
//
//----------------------------------------------------------------------------------------
struct SplSourceFile {

    public: 

    SplSourceFile( const char *fPath );
    ~SplSourceFile( );
    
    char getChar( );

    private:

    const char  *fileName;
    const char  *filePath;
    FILE        *pFile;
    int         lineNo;
    int         colNo;
};

//----------------------------------------------------------------------------------------
// File location in source file.
//
//----------------------------------------------------------------------------------------
struct SplFileLoc {
    
    const char  *fileName;
    int         lineNo;
    int         colNo;
};


//----------------------------------------------------------------------------------------
// The lexer reads tokens from a source file.
//
//----------------------------------------------------------------------------------------
struct SplToken {
        
    SplTokenId          tid;               
    SplFileLoc          loc;            

    union {

        T64Word         val;        
        const char      *str; 

    } u;
};



#if 0
//----------------------------------------------------------------------------------------
// Tokenizer base abstract class. The tokenizer object scans an input character
// stream. It breaks the stream into tokens. The tokenizer raises exceptions. 
// The base class contains the common routines and data structures. The derived 
// classes implement the character input routines.
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
    char            *tokName( );
    T64Word         tokVal( );
    char            *tokStr( );
   
    void            checkEOS( );
    void            acceptComma( );
    void            acceptColon( );
    void            acceptEqual( );
    void            acceptLparen( );
    void            acceptRparen( );
  //  SimTokId        acceptTokSym( SimErrMsgId errId );

    private:
    
    virtual void    nextChar( ) = 0;
    void            parseNum( );
    void            parseString( );
    void            parseIdent( );

    protected:

    char            currentChar     = ' ';
    SimToken        *tokTab         = nullptr;   
    SimToken        currentToken;
     
};

//----------------------------------------------------------------------------------------
// Tokenizer from string. The command line interface parse their input buffer line.
// The tokenizer will return the tokens found in the line. The tokenizer raises 
// exceptions.
//
//----------------------------------------------------------------------------------------
struct SimTokenizerFromString : public SimTokenizer {
    
    public:
    
    SimTokenizerFromString( );
    void setupTokenizer( char *lineBuf, SimToken *tokTab ); 
    
    private:

    void            nextChar( );

    int             currentCharIndex    = 0;
    int             currentLineLen      = 0;
    char            tokenLine[ 256 ]    = { 0 };

};

//----------------------------------------------------------------------------------------
// Tokenizer from file. We may need one day to read commands from a file. This
// tokenizer reads characters from a file stream. The tokenizer raises exceptions.
//
//----------------------------------------------------------------------------------------
struct SimTokenizerFromFile : public SimTokenizer {
    
    public:
    
    SimTokenizerFromFile( );
    virtual ~SimTokenizerFromFile( );

    void    setupTokenizer( char *filePath, SimToken *tokTab );
    int     getCurrentLineIndex( );
    int     getCurrentCharPos( );
    
    private:
    
    void    openFile( char *filePath );
    void    closeFile( );
    void    nextChar( );

    int    currentLineIndex     = 0;
    int    currentCharIndex     = 0;
    FILE   *srcFile             = nullptr;
};

#endif
