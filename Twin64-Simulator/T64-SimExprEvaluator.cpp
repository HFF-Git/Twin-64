//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line expression parser
//
//----------------------------------------------------------------------------------------
// The command interpreter features expression evaluation for command arguments. It is
// a straightforward recursive top down interpreter.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line expression parser
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
// The command line features an expression evaluator for the arguments. The overall 
// syntax is as follows:
//
//      <command>   ->  <cmdId> [ <argList> ]
//      <function>  ->  <funcId> “(“ [ <argList> ] ")"
//      <argList>   ->  <expr> { <expr> }
//
// Expression have a type, which are NUM, ADR, STR, SREG, GREG and CREG.
//
//      <factor> -> <number>                        |
//                  <string>                        |
//                  <envId>                         |
//                  <pswId>  [ ":" <proc> ]         |     
//                  <gregId> [ ":" <proc> ]         |
//                  <cregId> [ ":" <proc> ]         |
//                  "~" <factor>                    |
//                  "(" <expr> ")"
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
// If a command is called, there is no output other than what the command was issuing.
// If a function is called in the command place, the function result will be printed. 
// If an argument represents a function, its return value will be the argument in the
//  command.
//
// The token table becomes a kind of dictionary with name, type and values. The 
// environment table needs to enhanced to allow for user defined variables.
//
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//----------------------------------------------------------------------------------------
namespace {

enum logicalOpId : int {
    
    AND_OP  = 0,
    OR_OP   = 1,
    XOR_OP  = 2
};

//----------------------------------------------------------------------------------------
// Add operation.
//
//----------------------------------------------------------------------------------------
void addOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: {
                    
                    if ( willAddOverflow( rExpr -> u.val, lExpr -> u.val ))
                        throw ( ERR_NUMERIC_OVERFLOW );

                    rExpr -> u.val += lExpr -> u.val; 
                    
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
                
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//----------------------------------------------------------------------------------------
// Sub operation.
//
//----------------------------------------------------------------------------------------
void subOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: {
                
                    if ( willSubOverflow( rExpr -> u.val, lExpr -> u.val ))
                        throw ( ERR_NUMERIC_OVERFLOW );

                    rExpr -> u.val -= lExpr -> u.val; 
                
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//----------------------------------------------------------------------------------------
// Multiply operation.
//
//----------------------------------------------------------------------------------------
void multOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: { 
                    
                    if ( willMultOverflow( rExpr -> u.val, lExpr -> u.val ))
                        throw ( ERR_NUMERIC_OVERFLOW );

                    rExpr -> u.val *= lExpr -> u.val;
                    
                } break;
                    
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//----------------------------------------------------------------------------------------
// Divide Operation.
//
//----------------------------------------------------------------------------------------
void divOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: { 

                    if ( willDivOverflow( rExpr -> u.val, lExpr -> u.val ))
                        throw ( ERR_NUMERIC_OVERFLOW );
                    
                    rExpr -> u.val /= lExpr -> u.val; 
                    
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//----------------------------------------------------------------------------------------
// Modulo operation.
//
//----------------------------------------------------------------------------------------
void modOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: { 
                    
                    if ( willDivOverflow( rExpr -> u.val, lExpr -> u.val ))
                        throw ( ERR_NUMERIC_OVERFLOW );

                    rExpr -> u.val %= lExpr -> u.val; 
                    
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
                
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//----------------------------------------------------------------------------------------
// Logical operation.
//
//----------------------------------------------------------------------------------------
void logicalOp( SimExpr *rExpr, SimExpr *lExpr, logicalOpId op ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_BOOL: {
            
            if ( lExpr -> typ == TYP_BOOL ) {
                
                switch ( op ) {
                        
                    case AND_OP:    rExpr -> u.bVal &= lExpr -> u.bVal; break;
                    case OR_OP:     rExpr -> u.bVal |= lExpr -> u.bVal; break;
                    case XOR_OP:    rExpr -> u.bVal ^= lExpr -> u.bVal; break;
                }
            }
            else throw ( ERR_EXPR_TYPE_MATCH );
            
        } break;
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: {
                    
                    switch ( op ) {
                            
                        case AND_OP:    rExpr -> u.val &= lExpr -> u.val; break;
                        case OR_OP:     rExpr -> u.val |= lExpr -> u.val; break;
                        case XOR_OP:    rExpr -> u.val ^= lExpr -> u.val; break;
                    }
                    
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

}; // namespace


//----------------------------------------------------------------------------------------
// Evaluation Expression Object constructor.
//
//----------------------------------------------------------------------------------------
SimExprEvaluator::SimExprEvaluator( SimGlobals *glb, SimTokenizer *tok ) {
    
    this -> glb         = glb;
    this -> tok         = tok;
    this -> inlineAsm   = new T64Assemble( );
    this -> disAsm      = new T64DisAssemble( );
}

//----------------------------------------------------------------------------------------
// "parseFactor" parses the factor syntax part of an expression. The expression directly
// ties into the value providers, i.e. a register of a processor or an environment 
// variable.
//
//      <factor> -> <number>                        |
//                  <pswRegId>  [ ":" <proc> ]      |
//                  <gRegId>    [ ":" <proc> ]      |
//                  <cRegId>    [ ":" <proc> ]      |
//                  "~" <factor>                    |
//                  "(" <expr> ")"
//
//----------------------------------------------------------------------------------------
void SimExprEvaluator::parseFactor( SimExpr *rExpr ) {
    
    rExpr -> typ       = TYP_NIL;
    rExpr -> u.val    = 0;
    
    if ( tok -> isTokenTyp( TYP_NUM ))  {
        
        rExpr -> typ     = TYP_NUM;
        rExpr -> u.val  = tok -> tokVal( );
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_STR ))  {
        
        rExpr -> typ   = TYP_STR;
        rExpr -> u.str = tok -> tokStr( );
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_GREG ))  {
        
        int regId   = tok -> tokVal( );
        int procId  = 0;

        tok -> nextToken( );
        if ( tok -> isToken( TOK_COLON )) {

            tok -> nextToken( );
            if ( tok -> isTokenTyp( TYP_NUM )) {
                
               // ??? get the module pointer to the CPU ( if it is one )
            }
            else throw ( ERR_EXPECTED_NUMERIC );

            tok -> nextToken( );
        }
        else {

            procId = 0; // fix ??? get actual current proc...
        } 

        // ??? read the register from system into rExpr -> u.val;

        rExpr -> typ = TYP_NUM;
    }
    else if ( tok -> isTokenTyp( TYP_CREG ))  {
        
        int regId   = tok -> tokVal( );
        int procId  = 0;
        
        tok -> nextToken( );
        if ( tok -> isToken( TOK_COLON )) {

            tok -> nextToken( );
            if ( tok -> isTokenTyp( TYP_NUM )) procId = tok -> tokVal( );
            else throw ( 999 );

            tok -> nextToken( );
        } 
        else {

            procId = 0; // fix ??? get actual current proc...
        } 

        // ??? read the register from system into rExpr -> u.val;

        rExpr -> typ = TYP_NUM;
    }
    else if ( tok -> isToken( TOK_NEG )) {
        
        tok -> nextToken( );
        parseFactor( rExpr );
        rExpr -> u.val = ~ rExpr -> u.val;
    }
    else if ( tok -> isToken( TOK_LPAREN )) {
        
        tok -> nextToken( );
        parseExpr( rExpr );
            
        if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
        else throw ( ERR_EXPECTED_RPAREN );
    }
     else if ( tok -> isTokenTyp( TYP_P_FUNC )) {

        // ??? would be nicer if the functions are just identifier names...
        // ??? how would we handle overwrite through ENVs ?
        
        parsePredefinedFunction( tok -> token( ), rExpr );
    }
    else if ( tok -> isToken( TOK_IDENT )) {
    
        SimEnvTabEntry *entry = glb -> env -> getEnvEntry( tok -> tokName( ));
        
        if ( entry != nullptr ) {
            
            rExpr -> typ = entry -> typ;
            
            switch( rExpr -> typ ) {
                    
                case TYP_BOOL:  rExpr -> u.bVal     =  entry -> u.bVal;         break;
                case TYP_NUM:   rExpr -> u.val      =  entry -> u.iVal;         break;
                case TYP_STR:   strcpy( rExpr -> u.str, entry -> u.strVal );    break;
                default: throw( 9999 );
            }
        }
        else throw( ERR_ENV_VAR_NOT_FOUND );
       
        tok -> nextToken( );
    }
    else if ( tok -> isToken( TOK_EOS )) {
        
        rExpr -> typ = TYP_NIL;
    }
    else throw ( ERR_EXPR_FACTOR );
}

//----------------------------------------------------------------------------------------
// "parseTerm" parses the term syntax.
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
// ??? type mix options ?
//----------------------------------------------------------------------------------------
void SimExprEvaluator::parseTerm( SimExpr *rExpr ) {
    
    SimExpr lExpr;
   
    parseFactor( rExpr );
    
    while (( tok -> tokId( ) == TOK_MULT )   ||
           ( tok -> tokId( ) == TOK_DIV  )   ||
           ( tok -> tokId( ) == TOK_MOD  )   ||
           ( tok -> tokId( ) == TOK_AND  ))  {
        
        uint8_t op = tok -> tokId( );
        
        tok -> nextToken( );
        parseFactor( &lExpr );
        
        if ( lExpr.typ == TYP_NIL ) throw ( ERR_UNEXPECTED_EOS );
        
        switch( op ) {
                
            case TOK_MULT:   multOp( rExpr, &lExpr );               break;
            case TOK_DIV:    divOp( rExpr, &lExpr );                break;
            case TOK_MOD:    modOp( rExpr, &lExpr );                break;
            case TOK_AND:    logicalOp( rExpr, &lExpr, AND_OP );    break;
        }
    }
}

//----------------------------------------------------------------------------------------
// "parseExpr" parses the expression syntax. The one line assembler parser routines use
// this call in many places where a numeric expression or an address is needed.
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
// ??? type mix options ?
//----------------------------------------------------------------------------------------
void SimExprEvaluator::parseExpr( SimExpr *rExpr ) {
    
    SimExpr lExpr;
    
    if ( tok -> isToken( TOK_PLUS )) {
        
        tok -> nextToken( );
        parseTerm( rExpr );
        
        if ( rExpr -> typ != TYP_NUM ) throw ( ERR_EXPECTED_NUMERIC );
    }
    else if ( tok -> isToken( TOK_MINUS )) {
        
        tok -> nextToken( );
        parseTerm( rExpr );
        
        if ( rExpr -> typ == TYP_NUM ) rExpr -> u.val = - (int32_t) rExpr -> u.val;
        else throw ( ERR_EXPECTED_NUMERIC );
    }
    else parseTerm( rExpr );
    
    while (( tok -> isToken( TOK_PLUS   )) ||
           ( tok -> isToken( TOK_MINUS  )) ||
           ( tok -> isToken( TOK_OR     )) ||
           ( tok -> isToken( TOK_XOR    ))) {
        
        uint8_t op = tok -> tokId( );
        
        tok -> nextToken( );
        parseTerm( &lExpr );
        
        if ( lExpr.typ == TYP_NIL ) throw ( ERR_UNEXPECTED_EOS );
        
        switch ( op ) {
                
            case TOK_PLUS:   addOp( rExpr, &lExpr );                break;
            case TOK_MINUS:  subOp( rExpr, &lExpr );                break;
            case TOK_OR:     logicalOp( rExpr, &lExpr, OR_OP );     break;
            case TOK_XOR:    logicalOp( rExpr, &lExpr, XOR_OP );    break;
        }
    }
}

//----------------------------------------------------------------------------------------
// We often expect a numeric value. A little helper function.
//
//----------------------------------------------------------------------------------------
T64Word SimExprEvaluator::acceptNumExpr( SimErrMsgId errCode, T64Word low, T64Word high ) {

     SimExpr rExpr;
     parseExpr( &rExpr );
     
     if ( rExpr.typ == TYP_NUM ) {
        
        if ( !isInRange( rExpr.u.val, low, high )) throw( errCode );
        return ( rExpr.u.val );
     }
     else throw ( errCode );
}
