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
    OPC_LDPA     = 2U,
    OPC_PRB      = 3U,
    OPC_TLB      = 4U,
    OPC_CA       = 5U,
    OPC_MST      = 6U,
    OPC_RFI      = 7U,
    OPC_TRAP     = 8U,
    OPC_DIAG     = 9U
};


#endif
