//------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line expression parser
//
//------------------------------------------------------------------------------
// The command interpreter features expression evaluation for command arguments.
// It is a straightforward recursive top down interpeter.
//
//------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator Command line expression parser
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details. You should have received a copy of the GNU General Public
// License along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------------------
#include "T64-Common.h"
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"
#include "T64-SimTables.h"

//------------------------------------------------------------------------------
// The commadn line features an expression evaluator for the arguments. The 
// overall syntaxt is as follows:
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
//                  <gregId>                        |
//                  <cregId>                        |
//                  "~" <factor>                    |
//                  "(" <expr> ")"
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
// If a command is called, there is no output other than what the command was 
// issuing. If a function is called in the command place, the function result 
// will be printed. If an argument represents a function, its return value will
// be the argument in the command.
//
// The token table becomes a kind of dictionary with name, type and values.
// The environment table needs to enhanced to allow for user defined variables.
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//------------------------------------------------------------------------------
namespace {

enum logicalOpId : int {
    
    AND_OP  = 0,
    OR_OP   = 1,
    XOR_OP  = 2
};

//------------------------------------------------------------------------------
// Add operation.
//
//------------------------------------------------------------------------------
void addOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: rExpr -> numVal += lExpr -> numVal; break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
                
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//------------------------------------------------------------------------------
// Sub operation.
//
//------------------------------------------------------------------------------
void subOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: rExpr -> numVal -= lExpr -> numVal; break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//------------------------------------------------------------------------------
// Multiply operation.
//
//------------------------------------------------------------------------------
void multOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: rExpr -> numVal *= lExpr -> numVal; break;
                    
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//------------------------------------------------------------------------------
// Divide Operation.
//
//------------------------------------------------------------------------------
void divOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: rExpr -> numVal /= lExpr -> numVal; break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//------------------------------------------------------------------------------
// Modulo operation.
//
//------------------------------------------------------------------------------
void modOp( SimExpr *rExpr, SimExpr *lExpr ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: rExpr -> numVal %= lExpr -> numVal; break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
                
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

//------------------------------------------------------------------------------
// Logical operation.
//
//------------------------------------------------------------------------------
void logicalOp( SimExpr *rExpr, SimExpr *lExpr, logicalOpId op ) {
    
    switch ( rExpr -> typ ) {
            
        case TYP_BOOL: {
            
            if ( lExpr -> typ == TYP_BOOL ) {
                
                switch ( op ) {
                        
                    case AND_OP:    rExpr -> bVal &= lExpr -> bVal; break;
                    case OR_OP:     rExpr -> bVal |= lExpr -> bVal; break;
                    case XOR_OP:    rExpr -> bVal ^= lExpr -> bVal; break;
                }
            }
            else throw ( ERR_EXPR_TYPE_MATCH );
            
        } break;
            
        case TYP_NUM: {
            
            switch ( lExpr -> typ ) {
                    
                case TYP_NUM: {
                    
                    switch ( op ) {
                            
                        case AND_OP:    rExpr -> numVal &= lExpr -> numVal; break;
                        case OR_OP:     rExpr -> numVal |= lExpr -> numVal; break;
                        case XOR_OP:    rExpr -> numVal ^= lExpr -> numVal; break;
                    }
                    
                } break;
                
                default: throw ( ERR_EXPR_TYPE_MATCH );
            }
            
        } break;
            
        default: throw ( ERR_EXPR_TYPE_MATCH );
    }
}

}; // namespace


//------------------------------------------------------------------------------
// Evaluation Expression Object constructor.
//
//------------------------------------------------------------------------------
SimExprEvaluator::SimExprEvaluator( SimGlobals *glb, SimTokenizer *tok ) {
    
    this -> glb         = glb;
    this -> tok         = tok;
    
}

//------------------------------------------------------------------------------
// Coercing functions. Not a lot there yet. The idea is to coerce an expression 
// into a 32-bit value where possible. There are signed and unisgned versions, 
// which at the moment identical. We only have 32-bit values. If we have one 
// day 16-bit and 64-bit vaous in addition, there is more to do. What we also 
// coerced is the first characters of a string, right justified if shorter than
//  4 bytes.
//
//------------------------------------------------------------------------------
void SimExprEvaluator::pFuncS32( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    res = 0;
     
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );
    if ( lExpr.typ == TYP_NUM ) {
        
        res = lExpr.numVal;
    }
    else if ( lExpr.typ == TYP_STR ) {
       
        if ( strlen( lExpr.strVal ) > 0 ) res = ( lExpr.strVal[ 0 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 1 ) res = ( res << 8 ) | ( lExpr.strVal[ 1 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 2 ) res = ( res << 8 ) | ( lExpr.strVal[ 2 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 3 ) res = ( res << 8 ) | ( lExpr.strVal[ 3 ] & 0xFF );
    }
    else throw ( ERR_EXPECTED_EXPR );
    
    rExpr -> typ    = TYP_NUM;
    rExpr -> numVal = res;

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
        
        res = lExpr.numVal;
    }
    else if ( lExpr.typ == TYP_STR ) {
        
        if ( strlen( lExpr.strVal ) > 0 ) res = ( lExpr.strVal[ 0 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 1 ) res = ( res << 8 ) | ( lExpr.strVal[ 1 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 2 ) res = ( res << 8 ) | ( lExpr.strVal[ 2 ] & 0xFF );
        if ( strlen( lExpr.strVal ) > 3 ) res = ( res << 8 ) | ( lExpr.strVal[ 3 ] & 0xFF );
    }
    else throw ( ERR_EXPECTED_EXPR );
    
    rExpr -> typ    = TYP_NUM;
    rExpr -> numVal = res;

    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//------------------------------------------------------------------------------
// Assemble function.
//
// ASSEMBLE "(" <str> ")"
//------------------------------------------------------------------------------
void SimExprEvaluator::pFuncAssemble( SimExpr *rExpr ) {
    
    SimExpr         lExpr;
    uint32_t        instr;
    SimErrMsgId     ret = NO_ERR;
    
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );
    if ( lExpr.typ == TYP_STR ) {
        
       // ret = oneLineAsm -> parseAsmLine( lExpr.strVal, &instr ); // ???
        
        if ( ret == NO_ERR ) {
            
            rExpr -> typ    = TYP_NUM;
            rExpr -> numVal = instr;
        }
        else throw ( ret );
    }
    else throw ( ERR_EXPECTED_STR );

    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//------------------------------------------------------------------------------
// Dis-assemble function.
//
// DISASSEMBLE "(" <str> [ "," <rdx> ] ")"
//------------------------------------------------------------------------------
void SimExprEvaluator::pFuncDisAssemble( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    instr;
    char        asmStr[ CMD_LINE_BUF_SIZE ];
    int         rdx = 16;  // ??? fix ....
    // glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
    
    parseExpr( &lExpr );
    
    if ( lExpr.typ == TYP_NUM ) {
        
        instr = lExpr.numVal;
        
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
        
        // disAsm -> formatInstr( asmStr, sizeof( asmStr ), instr ); // ???
        
        rExpr -> typ = TYP_STR;
        strcpy( rExpr -> strVal, asmStr );
    }
    else throw ( ERR_EXPECTED_INSTR_VAL );
}

//------------------------------------------------------------------------------
// Virtual address hash function.
//
// HASH "(" <extAdr> ")"
//------------------------------------------------------------------------------
void SimExprEvaluator::pFuncHash( SimExpr *rExpr ) {
    
    SimExpr     lExpr;
    uint32_t    hashVal;
    
    tok -> nextToken( );
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
        
    parseExpr( &lExpr );

    // ??? call the function ...
   
    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_RPAREN );
}

//------------------------------------------------------------------------------
// Entry point to the predefined functions. We dispatch based on the predefined
// function token Id.
//
//------------------------------------------------------------------------------
void SimExprEvaluator::parsePredefinedFunction( SimToken funcId, SimExpr *rExpr ) {
    
    switch( funcId.tid ) {
            
        case PF_ASSEMBLE:       pFuncAssemble( rExpr );     break;
        case PF_DIS_ASSEMBLE:   pFuncDisAssemble( rExpr );  break;
        case PF_HASH:           pFuncHash( rExpr );         break;
        case PF_S32:            pFuncS32( rExpr );          break;
            
        default: throw ( ERR_UNDEFINED_PFUNC );
    }
}

//------------------------------------------------------------------------------
// "parseFactor" parses the factor syntax part of an expression.
//
//      <factor> -> <number>                        |
//                  <gregId>                        |
//                  <sregId>                        |
//                  <cregId>                        |
//                  "~" <factor>                    |
//                  "(" <expr> ")"
//
//------------------------------------------------------------------------------
void SimExprEvaluator::parseFactor( SimExpr *rExpr ) {
    
    rExpr -> typ       = TYP_NIL;
    rExpr -> numVal    = 0;
    
    if ( tok -> isTokenTyp( TYP_NUM ))  {
        
        rExpr -> typ     = TYP_NUM;
        rExpr -> numVal  = tok -> tokVal( );
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_STR ))  {
        
        rExpr -> typ = TYP_STR;
        strcpy( rExpr -> strVal, tok -> tokStr( ));
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_GREG ))  {
        
        rExpr -> typ    = TYP_NUM;
       //  rExpr -> numVal = glb -> cpu -> getReg( RC_GEN_REG_SET, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_CREG ))  {
        
        rExpr -> typ    = TYP_CREG;
      //  rExpr -> numVal = glb -> cpu -> getReg( RC_CTRL_REG_SET, tok -> tokVal( ));
        tok -> nextToken( );
    }
    else if ( tok -> isTokenTyp( TYP_P_FUNC )) {
        
        parsePredefinedFunction( tok -> token( ), rExpr );
    }
    else if ( tok -> isToken( TOK_IDENT )) {
        
        #if 0 // fix .....
        SimEnvTabEntry *entry = glb -> env -> getEnvVarEntry ( tok -> tokStr( ));
        
        if ( entry != nullptr ) {
            
            rExpr -> typ = entry -> typ;
            
            switch( rExpr -> typ ) {
                    
                case TYP_BOOL:  rExpr -> bVal       =  entry -> u.bVal;           break;
                case TYP_NUM:   rExpr -> numVal     =  entry -> u.iVal;           break;
              
                case TYP_STR:   strcpy( rExpr -> strVal, entry -> u.strVal );     break;
                
                default: fprintf( stdout, "**** uncaptured type in factor, fix ... \n" );
            }
        }
        else throw( ERR_ENV_VAR_NOT_FOUND );
        #endif
        
        tok -> nextToken( );
    }
    else if ( tok -> isToken( TOK_NEG )) {
        
        tok -> nextToken( );
        parseFactor( rExpr );
        rExpr -> numVal = ~ rExpr -> numVal;
    }
    else if ( tok -> isToken( TOK_LPAREN )) {
        
        tok -> nextToken( );
        parseExpr( rExpr );
            
        if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
        else throw ( ERR_EXPECTED_RPAREN );
    }
    else if (( tok -> tokTyp( ) == TYP_NIL ) && 
             ( tok -> tokId( ) == TOK_EOS )) {
        
        rExpr -> typ = TYP_NIL;
    }
    else throw ( ERR_EXPR_FACTOR );
}

//------------------------------------------------------------------------------
// "parseTerm" parses the term syntax.
//
//      <term>      ->  <factor> { <termOp> <factor> }
//      <termOp>    ->  "*" | "/" | "%" | "&"
//
// ??? type mix options ?
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// "parseExpr" parses the expression syntax. The one line assembler parser 
// routines use this call in many places where a numeric expression or an 
// address is needed.
//
//      <expr>      ->  [ ( "+" | "-" ) ] <term> { <exprOp> <term> }
//      <exprOp>    ->  "+" | "-" | "|" | "^"
//
// ??? type mix options ?
//------------------------------------------------------------------------------
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
        
        if ( rExpr -> typ == TYP_NUM ) rExpr -> numVal = - (int32_t) rExpr -> numVal;
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
