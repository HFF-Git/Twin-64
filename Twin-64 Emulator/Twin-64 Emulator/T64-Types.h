//
//  T64-Types.h
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 01.05.25.
//
#ifndef T64_Types_h
#define T64_Types_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
    
    NO_TRAP             = 0,
    PHYS_MEM_ADR_TRAP   = 1,
    IO_MEM_ADR_TRAP     = 2,
    MEM_ADR_ALIGN_TRAP  = 3,
    OVERFLOW_TRAP       = 4,
    
};


// ??? define bit fields and bit positions ...

// ??? define names for control registers...


//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
enum ControlRegId : int {
    
    CTL_REG_SHAMT   = 3,
    CTL_REG_PID0    = 4,
    CTL_REG_PID1    = 5,
    CTL_REG_PID2    = 6,
    CTL_REG_PID3    = 7,
    CTL_REG_IVA     = 8,
    
};

enum OpCodeGroup : int {
    
    OP_CODE_GRP_ALU = 0,
    OP_CODE_GRP_MEM = 1,
    OP_CODE_GRP_BR  = 2,
    OP_CODE_GRP_SYS = 3
};

enum OpCodeFam : int {
    
    OP_CODE_OP_NOP     = 0,
    OP_CODE_OP_AND     = 1,
    OP_CODE_OP_OR      = 2,
    OP_CODE_OP_XOR     = 3,
    OP_CODE_OP_ADD     = 4,
    OP_CODE_OP_SUB     = 5,
    OP_CODE_OP_CMP     = 6,
    OP_CODE_OP_BITOP   = 7,

    OP_CODE_FAM_CHK     = 10,
};


#endif
