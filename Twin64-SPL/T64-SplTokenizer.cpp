//----------------------------------------------------------------------------------------
//
//  Twin-64 -System Programming Language Compiler - Tokenizer
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// Twin-64 -System Programming Language Compiler - Tokenizer
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

#include "T64-SplDeclarations.h"


//----------------------------------------------------------------------------------------
// Local data for tokenizer operation.
///----------------------------------------------------------------------------------------
namespace {       
    
    


}


//----------------------------------------------------------------------------------------
// SplSourceFile. Objects of this class represent a source file
//
//----------------------------------------------------------------------------------------
SplSourceFile::SplSourceFile( const char *fPath ) {

    this -> filePath = fPath;
    this -> fileName = nullptr;
    this -> pFile    = nullptr;
    this -> lineNo   = 1;
    this -> colNo    = 0;

    if ( fPath == nullptr ) {

        // throw std::runtime_error("Null file path");
    }
        
    pFile = fopen( fPath, "r" );
    if ( pFile == nullptr ) {

        // throw std::runtime_error("Failed to open file");
    }
        
    #ifdef SPL_ON_APPLE
        const char *lastBackslash = strrchr(fPath, '\\');
        if (( lastSlash ==  nullptr ) || 
            (lastBackslash  && lastBackslash > lastSlash ))
            lastSlash = lastBackslash;
    #else
        const char *lastSlash = strrchr(fPath, '/');
        fileName  = (( lastSlash ) ? ( lastSlash + 1 ) : ( fPath ));
    #endif
}

SplSourceFile::~SplSourceFile() {

    if ( pFile != nullptr ) fclose( pFile );
}

//----------------------------------------------------------------------------------------
// getChar. Get next character from source file. We also return EOL on line endings.
//
//----------------------------------------------------------------------------------------
char SplSourceFile::getChar() {

    int c = fgetc( pFile );

    if ( c == EOF ) return (char)EOF;

    if ( c == '\n' ) {
        
        lineNo++;
        colNo = 0;

        // return( EOL );
         return ( c );
    }
    else { 
        
        colNo++;
        return ( c );       
    }
}

//----------------------------------------------------------------------------------------
// Tokenizer functions.
//
//----------------------------------------------------------------------------------------