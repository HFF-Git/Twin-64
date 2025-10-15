//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line tokenizer
//
//----------------------------------------------------------------------------------------
// The tokenizer will accept an input line and return one token at a time. 
// Upon an error, the tokenizer will raise an exception.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line tokenizer
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
#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"
#include "T64-SimTables.h"

//----------------------------------------------------------------------------------------
// Local namespace. These routines are not visible outside this source file.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
const int   TOK_INPUT_LINE_SIZE = 256;
const int   TOK_NAME_SIZE       = 32;
const char  EOS_CHAR            = 0;

//----------------------------------------------------------------------------------------
// The lookup function. We just do a linear search for now.
//
//----------------------------------------------------------------------------------------
int lookupToken( char *inputStr, SimToken *tokTab ) {
    
    if (( strlen( inputStr ) == 0 ) || 
        ( strlen ( inputStr ) > TOK_NAME_SIZE )) return( -1 );
    
    for ( int i = 0; i < MAX_CMD_TOKEN_TAB; i++  ) {
        
        if ( strcmp( inputStr, tokTab[ i ].name ) == 0 ) return( i );
    }
    
    return( -1 );
}

}; // namespace

//----------------------------------------------------------------------------------------
// The object constructor, nothing to do for now.
//
//----------------------------------------------------------------------------------------
SimTokenizer::SimTokenizer( ) { }

//----------------------------------------------------------------------------------------
// We initialize a couple of globals that represent the current state of the 
// parsing process. This call is the first before any other method can be 
// called.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::setupTokenizer( char *lineBuf, SimToken *tokTab ) {
    
    strncpy( tokenLine, lineBuf, strlen( lineBuf ) + 1 );
    
    this -> tokTab                  = tokTab;
    this -> currentLineLen          = (int) strlen( tokenLine );
    this -> currentCharIndex        = 0;
    this -> currentTokCharIndex     = 0;
    this -> currentChar             = ' ';

}

//----------------------------------------------------------------------------------------
// helper functions for the current token.
//
//----------------------------------------------------------------------------------------
bool SimTokenizer::isToken( SimTokId tokId ) { 
    
    return( currentToken.tid == tokId ); 
}

bool SimTokenizer::isTokenTyp( SimTokTypeId typId )  { 
    
    return( currentToken.typ == typId ); 
}

SimToken SimTokenizer::token( ) { 
    
    return( currentToken );    
}

SimTokTypeId SimTokenizer::tokTyp( ) { 
    
    return( currentToken.typ ); 
}

SimTokId SimTokenizer::tokId( ) { 
    
    return( currentToken.tid ); 
}

T64Word SimTokenizer::tokVal( ) { 
    
    return( currentToken.u.val ); 
}

char *SimTokenizer::tokName( ) {

    return( currentToken.name );
}

char *SimTokenizer::tokStr( ) { 
    
    return( currentToken.u.str );
}

int SimTokenizer::tokCharIndex( ) { 
    
    return( currentCharIndex ); 
}

char *SimTokenizer::tokenLineStr( ) { 
    
    return( tokenLine ); 
}

//----------------------------------------------------------------------------------------
// "nextChar" returns the next character from the token line string.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::nextChar( ) {
    
    if ( currentCharIndex < currentLineLen ) {
        
        currentChar = tokenLine[ currentCharIndex ];
        currentCharIndex ++;
    }
    else currentChar = EOS_CHAR;
}

//----------------------------------------------------------------------------------------
// "parseNum" will parse a number. We accept decimals and hexadecimals. The 
// numeric string can also contain "_" characters for a better readable string. 
// Hex numbers start with a "0x", decimals just with the numeric digits.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::parseNum( ) {
    
    currentToken.tid    = TOK_NUM;
    currentToken.typ    = TYP_NUM;
    currentToken.u.val  = 0;
    
    int     base        = 10;
    int     maxDigits   = 22;
    int     digits      = 0;
    T64Word tmpVal      = 0;
    
    if ( currentChar == '0' ) {

        while ( currentChar == '0' ) nextChar( );
        
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

    currentToken.u.val = tmpVal;
}

//----------------------------------------------------------------------------------------
// "parseString" gets a string. We manage special characters inside the string 
// with the "\" prefix. Right now, we do not use strings, so the function is 
// perhaps for the future. We will just parse it, but record no result. One day,
// the entire simulator might use the lexer functions. Then we need it.
//
// ??? changes...
//----------------------------------------------------------------------------------------
void SimTokenizer::parseString( ) {
    
    currentToken.tid        = TOK_STR;
    currentToken.typ        = TYP_STR;
    currentToken.u.str      = nullptr;

    strTokenBuf[ 0 ] = '\0';             

    nextChar( );
    while (( currentChar != EOS_CHAR ) && ( currentChar != '"' )) {

        if ( currentChar == '\\' ) {
            
            nextChar( );
            if ( currentChar != EOS_CHAR ) {
                
                if ( currentChar == 'n' ) {

                    strcat( strTokenBuf, (char *) "\n" );
                } 
                else if ( currentChar == 't' ) {  
                    
                    strcat( strTokenBuf, (char *) "\t" );
                }
                else if ( currentChar == '\\' ) { 
                    
                    strcat( strTokenBuf, (char *) "\\" );
                }
                else {
                    
                    addChar( strTokenBuf, 
                             sizeof( strTokenBuf ), 
                             currentChar );
                }
            }
            else throw ( ERR_EXPECTED_CLOSING_QUOTE );
        }
        else addChar( strTokenBuf, sizeof( strTokenBuf ), currentChar );
        
        nextChar( );
    }

    currentToken.u.str = strTokenBuf;

    nextChar( );
}

//----------------------------------------------------------------------------------------
// "parseIdent" parses an identifier. It is a sequence of characters starting 
// with an alpha character. An identifier found in the token table will assume 
// the type and value of the token found. Any other identifier is just an 
// identifier symbol. There is one more thing. There are qualified constants 
// that begin with a character followed by a percent character, followed by a 
// numeric value. During the character analysis, We first check for these kind
//  of qualifiers and if found hand over to parse a number.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::parseIdent( ) {
    
    currentToken.tid    = TOK_IDENT;
    currentToken.typ    = TYP_IDENT;

    char identBuf[ MAX_TOK_NAME_SIZE ] = "";
    
    if (( currentChar == 'L' ) || ( currentChar == 'l' )) {
        
        addChar( identBuf, sizeof( identBuf ), currentChar );
        nextChar( );
        
        if ( currentChar == '%' ) {
            
            addChar( identBuf, sizeof( identBuf ), currentChar );
            nextChar( );
            
            if ( isdigit( currentChar )) {
                
                parseNum( );
                currentToken.u.val &= 0x00000000FFFFFC00;
                currentToken.u.val >>= 10;
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
                currentToken.u.val &= 0x00000000000003FF;
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
                currentToken.u.val &= 0x000FFFFF00000000;
                currentToken.u.val >>= 32;
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
                currentToken.u.val &= 0xFFF0000000000000;
                currentToken.u.val >>= 52;
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
    
    int index = lookupToken( identBuf, tokTab );
    
    if ( index == - 1 ) {
        
        strncpy( currentToken.name, identBuf, sizeof( MAX_TOK_NAME_SIZE ));
        currentToken.typ    = TYP_IDENT;
        currentToken.tid    = TOK_IDENT;
    }
    else currentToken = tokTab[ index ];
}

//----------------------------------------------------------------------------------------
// "nextToken" is the entry point to the token business. It returns the next token from
// the input string.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::nextToken( ) {

    currentToken.typ    = TYP_NIL;
    currentToken.tid    = TOK_NIL;
    currentToken.u.val  = 0;
    
    while (( currentChar == ' '  ) || ( currentChar == '\n' )) nextChar( );

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
    else if ( currentChar == ':' ) {
        
        currentToken.typ   = TYP_SYM;
        currentToken.tid   = TOK_COLON;
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

//----------------------------------------------------------------------------------------
// Helper functions.
//
//----------------------------------------------------------------------------------------
void SimTokenizer::checkEOS( ) {
    
    if ( ! isToken( TOK_EOS )) throw ( ERR_TOO_MANY_ARGS_CMD_LINE );
}

void SimTokenizer::acceptComma( ) {
    
    if ( isToken( TOK_COMMA )) nextToken( );
    else throw ( ERR_EXPECTED_COMMA );
}

void SimTokenizer::acceptLparen( ) {
    
    if ( isToken( TOK_LPAREN )) nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
}

void SimTokenizer::acceptRparen( ) {
    
    if ( isToken( TOK_RPAREN )) nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
}

SimTokId SimTokenizer::acceptTokSym( SimErrMsgId errId ) {

    if ( isTokenTyp( TYP_SYM )) {
        
        SimTokId tmp = tokId( );
        nextToken( );
        return( tmp );
    }
    else throw ( errId );
}
