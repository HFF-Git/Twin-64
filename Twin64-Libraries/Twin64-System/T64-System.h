//------------------------------------------------------------------------------
//
// Twin-64 - System
//
//------------------------------------------------------------------------------
// This ...
//
//------------------------------------------------------------------------------
//
//  Twin-64 - System
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
// License along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------------------
#ifndef T64_System_h
#define T64_System_h

#include "T64-Common.h"
#include "T64-Module.h"
#include "T64-Processor.h"
#include "T64-Memory.h"


//------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------
struct Twin64System {

    public: 

    Twin64System( );

    void            reset( );

    T64Processor    *getProcessor( );
    T64Memory       *getMemory( );
    
   // ?? an array of modules ? 

};

#endif