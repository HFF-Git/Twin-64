//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command Expression Parser Predefined Functions
//
//----------------------------------------------------------------------------------------
// The command interpreter features expression evaluation for command arguments. It is
// a straightforward recursive top down interpreter.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command Expression Parser Predefined Functions
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
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"
#include "T64-SimTables.h"

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//----------------------------------------------------------------------------------------
namespace {



}; // namespace


// ??? we will have all kinds of functions that will need access to modules, etc.


//----------------------------------------------------------------------------------------
// Coercing functions. Not a lot there yet. The idea is to coerce an expression into a
// 32-bit value where possible. There are signed and unsigned versions, which at the
// moment identical. We only have 32-bit values. If we have one day 16-bit and 64-bit 
// various in addition, there is more to do. What we also coerced is the first characters
// of a string, right justified if shorter than 4 bytes.
//
//----------------------------------------------------------------------------------------
void SimExprEvaluator::pFuncS32( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    res = 0;
     
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );
    if ( lExpr.typ == TYP_NUM ) {
        
        res = lExpr.u.val;
    }
    else if ( lExpr.typ == TYP_STR ) {
       
        if ( strlen( lExpr.u.str ) > 0 ) res = ( lExpr.u.str[ 0 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 1 ) res = ( res << 8 ) | ( lExpr.u.str[ 1 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 2 ) res = ( res << 8 ) | ( lExpr.u.str[ 2 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 3 ) res = ( res << 8 ) | ( lExpr.u.str[ 3 ] & 0xFF );
    }
    else throw ( ERR_EXPECTED_EXPR );
    
    rExpr -> typ    = TYP_NUM;
    rExpr -> u.val = res;

    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

void SimExprEvaluator::pFuncU32( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    res = 0;
     
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );
    if ( lExpr.typ == TYP_NUM ) {
        
        res = lExpr.u.val;
    }
    else if ( lExpr.typ == TYP_STR ) {
        
        if ( strlen( lExpr.u.str ) > 0 ) res = ( lExpr.u.str[ 0 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 1 ) res = ( res << 8 ) | ( lExpr.u.str[ 1 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 2 ) res = ( res << 8 ) | ( lExpr.u.str[ 2 ] & 0xFF );
        if ( strlen( lExpr.u.str ) > 3 ) res = ( res << 8 ) | ( lExpr.u.str[ 3 ] & 0xFF );
    }
    else throw ( ERR_EXPECTED_EXPR );
    
    rExpr -> typ    = TYP_NUM;
    rExpr -> u.val = res;

    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//----------------------------------------------------------------------------------------
// Assemble function.
//
// ASSEMBLE "(" <str> ")"
//----------------------------------------------------------------------------------------
void SimExprEvaluator::pFuncAssemble( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    T64Instr    instr   = 0;
    int         ret     = 0;
    
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );
    if ( lExpr.typ == TYP_STR ) {
        
        ret = inlineAsm -> assembleInstr( lExpr.u.str, &instr );
        
        if ( ret == 0 ) {
            
            rExpr -> typ    = TYP_NUM;
            rExpr -> u.val = instr;
        }
        else throw ( ret );
    }
    else throw ( ERR_EXPECTED_STR );

    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//----------------------------------------------------------------------------------------
// Dis-assemble function. We take the value and produce a string. 
//
// WARNING: the expression evaluator returns a pointer to a string. This is typically
// a token and the string is value until the next token is read. This is bit ugly for
// our disassembler which returns an assembled string. When we just set the expression
// pointer and return, the string is lost. Therefore the disassemble string is a static
// variable.
//
// DISASSEMBLE "(" <str> [ "," <rdx> ] ")"
//----------------------------------------------------------------------------------------
void SimExprEvaluator::pFuncDisAssemble( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    instr = 0;
    int         rdx   = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    static char        asmStr[ MAX_CMD_LINE_SIZE ];
    
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
    
    parseExpr( &lExpr );
    
    if ( lExpr.typ == TYP_NUM ) {
        
        instr = lExpr.u.val;
        
        if ( tok -> tokId( ) == TOK_COMMA ) {
            
            tok -> nextToken( );
            
            if (( tok -> tokId( ) == TOK_HEX ) ||
                ( tok -> tokId( ) == TOK_DEC )) {
                
                rdx = tok -> tokVal( );
                tok -> nextToken( );
            }
            else if ( tok -> tokId( ) == TOK_EOS ) {
                
                throw ( ERR_UNEXPECTED_EOS );
            }
            else throw ( ERR_INVALID_FMT_OPT );
        }
        
        if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
        else throw ( ERR_EXPECTED_RPAREN );
        
        disAsm -> formatInstr( asmStr, sizeof( asmStr ), instr, rdx );
        
        rExpr -> typ   = TYP_STR;
        rExpr -> u.str = asmStr; 
    }
    else throw ( ERR_EXPECTED_INSTR_VAL );
}

//----------------------------------------------------------------------------------------
// Virtual address hash function.
//
// HASH "(" <extAdr> ")"
//----------------------------------------------------------------------------------------
void SimExprEvaluator::pFuncHash( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
   
    tok -> nextToken( );
    
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );

    // ??? call the function ...
   
    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//----------------------------------------------------------------------------------------
// Entry point to the predefined functions. We dispatch based on the predefined function
// token Id.
//
//----------------------------------------------------------------------------------------
void SimExprEvaluator::parsePredefinedFunction( SimToken funcId, SimExpr *rExpr ) {
    
    switch( funcId.tid ) {
            
        case PF_ASSEMBLE:       pFuncAssemble( rExpr );     break;
        case PF_DIS_ASM:        pFuncDisAssemble( rExpr );  break;
        case PF_HASH:           pFuncHash( rExpr );         break;
        case PF_S32:            pFuncS32( rExpr );          break;
            
        default: throw ( ERR_UNDEFINED_PFUNC );
    }
}
