//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator command window
//
//----------------------------------------------------------------------------------------
// The command window is the last screen area below all enabled windows displayed. It
// is actually not a window like the others in that it represents lines written to the
// window as well as the command input line. It still has a window header and a line
// drawing area. To enable scrolling of this window, an output buffer needs to be 
// implemented that stores all output in a circular buffer to use for text output. 
// Just like a "real" terminal. The cursor up and down keys will perform the scrolling.
// The command line is also a bit special. It is actually the one line locked scroll
// area. Input can be edited on this line, a carriage return will append the line to
// the output buffer area.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU -Simulator command window
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
#include <elfio/elfio.hpp>

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file. None so far.
//
//----------------------------------------------------------------------------------------
namespace {


}; // namespace

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::loadElfFile( char *fileName ) {
  
    // ??? create the ELFIO reader

    // ??? read in the header and check

    // ??? if successful:

        // ??? needs access to the CPU 
        // ??? clear memory
        

        // ??? for each segment:
        //      ??? locate the section header.
        //      ??? move the data to the physical memory

    // ??? right now, we do not have any access rights, priv levels, etc.
    // ??? need to enhance the assembler to pass this kind of data ?

}
