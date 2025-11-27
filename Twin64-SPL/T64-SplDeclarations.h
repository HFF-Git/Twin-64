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
// Copyright (C) 2025 - 2025 Helmut Fieres
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
#ifndef TWIN64_SplDeclarations_h
#define TWIN64_SplDeclarations_h

#include "T64-Common.h"
#include "T64-SplVersion.h"
#include "T64-SplDeclarations.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>


//----------------------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------------------


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
//
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
//
//
//----------------------------------------------------------------------------------------
struct SplFileLoc {
    
    const char  *fileName;
    int         lineNo;
    int         colNo;
};

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
enum SplTokenId : int {
    
    TOK_NIL         = 0,    TOK_EOL         = 1,    TOK_EOF         = 2,        
  
    TOK_COMMA       = 3,    TOK_PERIOD      = 4,    TOK_LPAREN      = 5,
    TOK_RPAREN      = 6,    TOK_STRING      = 7,    TOK_NUM         = 8,
    TOK_IDENT       = 9,    TOK_OP_CODE     = 10,   TOK_GENERAL_REG = 11,
    TOK_CONTROL_REG = 12
};

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
enum SplTokenTypeId : int {
    
    TYP_NIL         = 0,
    TYP_STR         = 1,
    TYP_NUM         = 2,
    TYP_IDENT       = 3,
    TYP_OP_CODE     = 4,
    TYP_GREG        = 5,
    TYP_CREG        = 6
};



//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
struct SplToken {
    
    const char          *name;          
    SplTokenId          tid;            
    SplTokenTypeId      typ;           
    SplFileLoc          loc;            

    union {

        T64Word             val;        
        const char          *tokStrVal; 
    } val;
};






#endif // TWIN64_SplDeclarations_h
