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

struct SplCmdLineOptions {

    const char              *name;
    SplCmdLineArgOptions    argOpt;   
    int                     val;
};

//----------------------------------------------------------------------------------------
// The simulator offers a set of command line options. Each option has a name, an 
// argument type and a value returned when the option is found.
//
// ??? under construction...
//----------------------------------------------------------------------------------------
static struct SplCmdLineOptions optionTable[ ] = {
        { "help",       OPT_NO_ARGUMENT,       'h' },
        { "verbose",    OPT_NO_ARGUMENT,       'v' },
        { "configfile", OPT_REQUIRED_ARGUMENT, 'f' },
        { "logfile",    OPT_REQUIRED_ARGUMENT, 'l' },
        {0, OPT_NO_ARGUMENT, 0}
    };



#endif // TWIN64_SplDeclarations_h
