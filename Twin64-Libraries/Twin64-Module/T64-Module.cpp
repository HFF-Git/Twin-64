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
#include "T64-Module.h"

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
T64Module::T64Module( ) {
    
}

T64ModuleType T64Module::getModuleType( ) {

    return( MT_NIL );
}

int T64Module::getModuleNum( ) {

    return( moduleNum );
}

T64SubModuleType T64Module::getSubModuleType( int subModNum ) {

    if ( subModNum < maxSubModules ) 
        return( subModTab[ subModNum ] -> getSubModType( ));
    else return ( MST_NIL );
}
    
int T64Module::getHpaStartAdr( T64Word *val ) {

    return( 0 );
}

int T64Module::getHpaSize( T64Word *val ) {

    return( 0 );
}


int T64Module:: getSpaStartAdr( T64Word *val ) {

    return( 0 );
}

int T64Module::getSpaSize( T64Word *val ) {

    return( 0 );
}

//****************************************************************************************
//****************************************************************************************
//
// SubModule
//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
T64SubModule::T64SubModule( int mNum, 
                            int subModNum, 
                            T64SubModuleType smType ) {

    this -> moduleNum   = mNum;
    this -> subModNum   = subModNum;
    this -> subModType  = smType;
}

T64SubModuleType T64SubModule::getSubModType( ) {

    return( subModType );
}

int T64SubModule::getSubModNum( ) {

    return( subModNum );
}