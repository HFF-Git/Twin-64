//----------------------------------------------------------------------------------------
//
//  Twin-64 - System Programming Language Compiler
//
//----------------------------------------------------------------------------------------
// The whole purpose of  this file is to define the current version String.
// We also set a constant to use for APPle vs. Windows specific coding. We use
// it in the command handler. It is not designed for compiling different code
// sequence, but rather make logical decisions on some output specifics, such
// as carriage return handling.
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
#ifndef TWIN64_SplVersion_h
#define TWIN64_SplVersion_h

const char SPL_VERSION[ ] = "A.00.01";
const char SPL_GIT_BRANCH[ ] = "main";
const int  SPL_PATCH_LEVEL = 1;

#if __APPLE__
const bool SPL_IS_APPLE = true;
#else
const bool SPL_IS_APPLE = false;
#endif

#endif // TWIN64_SplVersion_h
