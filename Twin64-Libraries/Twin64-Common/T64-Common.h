//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - Common Definitions
//
//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - Common Definitions
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation, either version 3 of the License,
// or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details. You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------------------------------------------------
#ifndef T64_Common_h
#define T64_Common_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
typedef int64_t T64Word;

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const   int     MAX_GREGS       = 16;
const   int     MAX_CREGS       = 16;
const   int     PAGE_SIZE       = 16 * 1024;

const   int64_t IO_MEM_START    = 0xF0000000;
const   int64_t IO_MEM_LIMIT    = 0xFFFFFFFF;

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
enum TrapCode : int {
    
    NO_TRAP                 = 0,
    ILLEGAL_INSTR_TRAP      = 1,
    ALIGNMENT_TRAP          = 2,
    
    PHYS_MEM_ADR_TRAP       = 4,
    IO_MEM_ADR_TRAP         = 5,
    
    OVERFLOW_TRAP           = 6,
    PROTECTION_TRAP         = 7,
    PRIV_VIOLATION_TRAP     = 8,
    TLB_ACCESS_TRAP         = 9,
    
};

//------------------------------------------------------------------------------------------------------------
// Trap definition. A Trap will consist of a trap code and up to three info parameters.
//
//------------------------------------------------------------------------------------------------------------
struct T64Trap {
    
public:
    
    T64Trap( int    trapCode,
             int    trapInfo1 = 0,
             int    trapInfo2 = 0,
             int    trapInfo3 = 0 ) {
        
        this -> trapCode  = trapCode;
        this -> trapInfo1 = trapInfo1;
        this -> trapInfo2 = trapInfo1;
        this -> trapInfo3 = trapInfo1;
    }
    
private:
    
    int trapCode;
    int trapInfo1;
    int trapInfo2;
    int trapInfo3;
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
enum ControlRegId : int {
    
    CTL_REG_CPU_INFO    = 0,
    CTL_REG_SHAMT       = 1,
    
    CTL_REG_PID0        = 4,
    CTL_REG_PID1        = 5,
    CTL_REG_PID2        = 6,
    CTL_REG_PID3        = 7,
    CTL_REG_IVA         = 8,
    
};

//------------------------------------------------------------------------------------------------------------
// Instruction groups and opcode families. Instrcutions are decoded in three fields. The first two bits
// contain the instruction group. Next are the 4 bits for the opcode family. Finally, bits 19..21 are further
// qualifying teh instruction.
//
//------------------------------------------------------------------------------------------------------------
enum OpCodeGroup : uint32_t {
    
    OPC_GRP_ALU = 0U,
    OPC_GRP_MEM = 1U,
    OPC_GRP_BR  = 2U,
    OPC_GRP_SYS = 3U
};

enum OpCodeFam : uint32_t {
    
    OPC_NOP      = 0U,
    
    OPC_ADD      = 1U,
    OPC_SUB      = 2U,
    OPC_AND      = 3U,
    OPC_OR       = 4U,
    OPC_XOR      = 5U,
    OPC_CMP      = 6U,
    OPC_BITOP    = 7U,
    OPC_SHAOP    = 8U,
    OPC_IMMOP    = 9U,
    OPC_LDO      = 10U,
    
    OPC_LD       = 8U,
    OPC_ST       = 9U,
    OPC_LDR      = 10U,
    OPC_STC      = 11U,
    
    OPC_B        = 1U,
    OPC_BR       = 2U,
    OPC_BV       = 3U,
    OPC_BB       = 4U,
    OPC_CBR      = 5U,
    OPC_MBR      = 6U,
    
    OPC_MR       = 1U,
    OPC_LPA      = 2U,
    OPC_PRB      = 3U,
    OPC_TLB      = 4U,
    OPC_CA       = 5U,
    OPC_MST      = 6U,
    OPC_RFI      = 7U,
    OPC_TRAP     = 14U,
    OPC_DIAG     = 15U
};


// ??? a test ..... we could move all extract / deposit / etc. to this file....
static inline void hugo( ) {  }

#endif
