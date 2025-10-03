//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - Module
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// T64 - A 64-bit CPU - CPU Core
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
#include "T64-System.h"

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
namespace {

};


//****************************************************************************************
//****************************************************************************************
//
// Module
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64Module::T64Module( T64ModuleType modType, int modNum ) {

    this -> moduleTyp       = modType;
    this -> moduleNum       = modNum;
}

T64ModuleType T64Module::getModuleType( ) {

    return( moduleTyp );
}

int T64Module::getModuleNum( ) {

    return( moduleNum );
}

#if 0
int T64Module::getHpaStartAdr( T64Word *val ) {

    return( moduleHPA );
}

int T64Module::getHpaSize( T64Word *val ) {

    return( moduleHPALen );
}

int T64Module:: getSpaStartAdr( T64Word *val ) {

    return( moduleSPA );
}

int T64Module::getSpaSize( T64Word *val ) {

    return( moduleSPALen );
}
#endif
