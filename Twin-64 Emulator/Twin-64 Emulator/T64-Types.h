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
enum OpCodes : uint8_t {
    
    OP_GRP_ALU      = 0x00,
    OP_GRP_MEM      = 0x10,
    OP_GRP_BR       = 0x20,
    OP_GRP_SYS      = 0x30,
    
    OP_ALU_NOP      = OP_GRP_ALU | 0x00,
    OP_ALU_AND      = OP_GRP_ALU | 0x01,
    OP_ALU_OR       = OP_GRP_ALU | 0x02,
    OP_ALU_XOR      = OP_GRP_ALU | 0x03,
    OP_ALU_ADD      = OP_GRP_ALU | 0x04,
    OP_ALU_SUB      = OP_GRP_ALU | 0x05,
    OP_ALU_CMP      = OP_GRP_ALU | 0x06,
    OP_ALU_EXTR     = OP_GRP_ALU | 0x07,
    OP_ALU_DEP      = OP_GRP_ALU | 0x08,
    OP_ALU_DSR      = OP_GRP_ALU | 0x09,
    OP_ALU_CHK      = OP_GRP_ALU | 0x0A,
    
    OP_MEM_LD       = OP_GRP_MEM | 0x00,
    OP_MEM_ST       = OP_GRP_MEM | 0x01,
    OP_MEM_LDR      = OP_GRP_MEM | 0x02,
    OP_MEM_STC      = OP_GRP_MEM | 0x03,
    
    OP_MEM_AND      = OP_GRP_MEM | 0x04,
    OP_MEM_OR       = OP_GRP_MEM | 0x05,
    OP_MEM_XOR      = OP_GRP_MEM | 0x06,
    OP_MEM_ADD      = OP_GRP_MEM | 0x07,
    OP_MEM_SUB      = OP_GRP_MEM | 0x08,
    OP_MEM_CMP      = OP_GRP_MEM | 0x09,
    
    OP_BR_LDI       = OP_GRP_BR  | 0x00,
    OP_BR_ADDIL     = OP_GRP_BR  | 0x01,
    OP_BR_LDO       = OP_GRP_BR  | 0x02,
    OP_BR_B         = OP_GRP_BR  | 0x03,
    OP_BR_BR        = OP_GRP_BR  | 0x04,
    OP_BR_BV        = OP_GRP_BR  | 0x05,
    OP_BR_CBR       = OP_GRP_BR  | 0x06,
    OP_BR_TBR       = OP_GRP_BR  | 0x07,
    OP_BR_MBR       = OP_GRP_BR  | 0x08,
    
    OP_SYS_MR       = OP_GRP_SYS | 0x00,
    OP_SYS_MST      = OP_GRP_SYS | 0x01,
    OP_SYS_LPA      = OP_GRP_SYS | 0x02,
    OP_SYS_PRB      = OP_GRP_SYS | 0x03,
    OP_SYS_ITLB     = OP_GRP_SYS | 0x04,
    OP_SYS_DTLB     = OP_GRP_SYS | 0x05,
    OP_SYS_PCA      = OP_GRP_SYS | 0x06,
    OP_SYS_DIAG     = OP_GRP_SYS | 0x07,
    OP_SYS_BRK      = OP_GRP_SYS | 0x08,
    OP_SYS_RFI      = OP_GRP_SYS | 0x09,
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const struct {
    
    uint8_t op;
    char    name[ 6 ];
    
} opCodeTab[ ] = {
    
    { .op = OP_ALU_NOP,     .name = "NOP"   },
    { .op = OP_ALU_AND,     .name = "AND"   },
    { .op = OP_ALU_OR,      .name = "OR"    },
    { .op = OP_ALU_XOR,     .name = "XOR"   },
    { .op = OP_ALU_ADD,     .name = "ADD"   },
    { .op = OP_ALU_SUB,     .name = "SUB"   },
    { .op = OP_ALU_CMP,     .name = "CMP"   },
    { .op = OP_ALU_EXTR,    .name = "EXTR"  },
    { .op = OP_ALU_DEP,     .name = "DEP"   },
    { .op = OP_ALU_DSR,     .name = "DSR"   },
    { .op = OP_ALU_CHK,     .name = "CHK"   },
    
    { .op = OP_MEM_LD,      .name = "LD"    },
    { .op = OP_MEM_ST,      .name = "ST"    },
    { .op = OP_MEM_LDR,     .name = "LDR"   },
    { .op = OP_MEM_STC,     .name = "STC"   },
    { .op = OP_MEM_AND,     .name = "AND"   },
    { .op = OP_MEM_OR,      .name = "OR"    },
    { .op = OP_MEM_XOR,     .name = "XOR"   },
    { .op = OP_MEM_ADD,     .name = "ADD"   },
    { .op = OP_MEM_SUB,     .name = "SUB"   },
    { .op = OP_MEM_CMP,     .name = "CMP"   },
    
    { .op = OP_BR_LDI,      .name = "LDI"   },
    { .op = OP_BR_ADDIL,    .name = "ADDIL" },
    { .op = OP_BR_LDO,      .name = "LDO"   },
    { .op = OP_BR_B,        .name = "B"     },
    { .op = OP_BR_BR,       .name = "BR"    },
    { .op = OP_BR_BV,       .name = "BV"    },
    { .op = OP_BR_CBR,      .name = "CBR"   },
    { .op = OP_BR_TBR,      .name = "TBR"   },
    { .op = OP_BR_MBR,      .name = "MBR"   },
    
    { .op = OP_SYS_MR,      .name = "MR"    },
    { .op = OP_SYS_MST,     .name = "MST"   },
    { .op = OP_SYS_LPA,     .name = "LPA"   },
    { .op = OP_SYS_PRB,     .name = "PRB"   },
    { .op = OP_SYS_ITLB,    .name = "ITLB"  },
    { .op = OP_SYS_DTLB,    .name = "DTLB"  },
    { .op = OP_SYS_PCA,     .name = "PCA"   },
    { .op = OP_SYS_DIAG,    .name = "DIAG"  },
    { .op = OP_SYS_BRK,     .name = "BRK"   },
    { .op = OP_SYS_RFI,     .name = "RFI"   }
};


#endif

