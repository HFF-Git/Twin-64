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



#if 0
//------------------------------------------------------------------------------------------------------------
// Instruction Opcodes are composed of group, family and a couple of bits in the respective instruction field.
// We logically OR them togther to form the unique instruction OpCode.
//
// ??? this is the central table ....
//----------------------------------------------------------------------------------------------------------
enum OpCodeMasks : uint32_t {
    
    OP_CODE_GRP_ALU     = ( 0U << 30 ),
    OP_CODE_GRP_MEM     = ( 1U << 30 ),
    OP_CODE_GRP_BR      = ( 2U << 30 ),
    OP_CODE_GRP_SYS     = ( 3U << 30 ),
    
    OP_CODE_FAM_NOP     = ( 0U  << 26 ),
    OP_CODE_FAM_AND     = ( 1U  << 26 ),
    OP_CODE_FAM_OR      = ( 2U  << 26 ),
    OP_CODE_FAM_XOR     = ( 3U  << 26 ),
    OP_CODE_FAM_ADD     = ( 4U  << 26 ),
    OP_CODE_FAM_SUB     = ( 5U  << 26 ),
    OP_CODE_FAM_CMP     = ( 6U  << 26 ),
    OP_CODE_FAM_EXTR    = ( 7U  << 26 ),
    OP_CODE_FAM_DEP     = ( 8U  << 26 ),
    OP_CODE_FAM_DSR     = ( 9U  << 26 ),
    OP_CODE_FAM_CHK     = ( 10U << 26 ),
    
    OP_CODE_FAM_LD      = ( 7U  << 26 ),
    OP_CODE_FAM_ST      = ( 8U  << 26 ),
    OP_CODE_FAM_LDR     = ( 9U  << 26 ),
    OP_CODE_FAM_STC     = ( 10U << 26 ),
    
    OP_CODE_FAM_LDI     = ( 0U  << 26 ),
    OP_CODE_FAM_ADDIL   = ( 1U  << 26 ),
    OP_CODE_FAM_LDO     = ( 2U  << 26 ),
    OP_CODE_FAM_B       = ( 3U  << 26 ),
    OP_CODE_FAM_BR      = ( 4U  << 26 ),
    OP_CODE_FAM_BV      = ( 5U  << 26 ),
    OP_CODE_FAM_CBR     = ( 6U  << 26 ),
    OP_CODE_FAM_MBR     = ( 7U  << 26 ),
    
    OP_CODE_FAM_MR      = ( 0U  << 26 ),
    OP_CODE_FAM_MST     = ( 1U  << 26 ),
    OP_CODE_FAM_RFI     = ( 2U  << 26 ),
    OP_CODE_FAM_LPA     = ( 3U  << 26 ),
    OP_CODE_FAM_PRB     = ( 4U  << 26 ),
    OP_CODE_FAM_TLB_OP  = ( 5U  << 26 ),
    OP_CODE_FAM_CA_OP   = ( 6U  << 26 ),
    OP_CODE_FAM_BRK     = ( 7U  << 26 ),
    OP_CODE_FAM_DIAG    = ( 8U  << 26 ),
    
    OP_CODE_BIT13       = ( 1U  << 13 ),
    OP_CODE_BIT14       = ( 1U  << 14 ),
    OP_CODE_BIT19       = ( 1U  << 19 ),
    OP_CODE_BIT20       = ( 1U  << 20 ),
    OP_CODE_BIT21       = ( 1U  << 21 ),
    
    // ??? have fields, easier to use...
    
    OP_CODE_FLD_DW2_B   = ( 0U << 13 ),
    OP_CODE_FLD_DW2_H   = ( 1U << 13 ),
    OP_CODE_FLD_DW2_W   = ( 2U << 13 ),
    OP_CODE_FLD_DW2_D   = ( 3U << 13 ),
    
    OP_CODE_FLD_EQ      = ( 0U << 19 ),
    OP_CODE_FLD_LT      = ( 1U << 19 ),
    OP_CODE_FLD_NE      = ( 2U << 19 ),
    OP_CODE_FLD_LE      = ( 3U << 19 ),
    
    
    OP_CODE_NOP         = OP_CODE_GRP_ALU | OP_CODE_FAM_NOP,
    
    OP_CODE_ADD         = OP_CODE_GRP_ALU | OP_CODE_FAM_ADD,
    
    
    
    OP_CODE_ADDB        = OP_CODE_GRP_MEM | OP_CODE_FAM_ADD | OP_CODE_FLD_DW2_B,
    OP_CODE_ADDH        = OP_CODE_GRP_MEM | OP_CODE_FAM_ADD | OP_CODE_FLD_DW2_H,
    OP_CODE_ADDW        = OP_CODE_GRP_MEM | OP_CODE_FAM_ADD | OP_CODE_FLD_DW2_W,
    OP_CODE_ADDD        = OP_CODE_GRP_MEM | OP_CODE_FAM_ADD | OP_CODE_FLD_DW2_D,
    
    OP_CODE_SUB         = OP_CODE_GRP_ALU | OP_CODE_FAM_SUB,
    OP_CODE_SUBB        = OP_CODE_GRP_MEM | OP_CODE_FAM_SUB | OP_CODE_FLD_DW2_B,
    OP_CODE_SUBH        = OP_CODE_GRP_MEM | OP_CODE_FAM_SUB | OP_CODE_FLD_DW2_H,
    OP_CODE_SUBW        = OP_CODE_GRP_MEM | OP_CODE_FAM_SUB | OP_CODE_FLD_DW2_W,
    OP_CODE_SUBD        = OP_CODE_GRP_MEM | OP_CODE_FAM_SUB | OP_CODE_FLD_DW2_D,
    
    
    // ??? all we need to set initial instruction ?
    OP_CODE_AND_I       = OP_CODE_GRP_ALU | OP_CODE_FAM_AND | OP_CODE_BIT20,
    OP_CODE_AND_ALU     = OP_CODE_GRP_ALU | OP_CODE_FAM_AND,
    OP_CODE_AND_MEM     = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_FLD_DW2_D,
    
    
    OP_CODE_ANDB        = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_FLD_DW2_B,
    OP_CODE_ANDH        = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_FLD_DW2_H,
    OP_CODE_ANDW        = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_FLD_DW2_W,
    OP_CODE_ANDD        = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_FLD_DW2_D,
    
    OP_CODE_ANDC        = OP_CODE_GRP_ALU | OP_CODE_FAM_AND | OP_CODE_BIT20,
    OP_CODE_ANDBC       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT20 | OP_CODE_FLD_DW2_B,
    OP_CODE_ANDHC       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT20 | OP_CODE_FLD_DW2_H,
    OP_CODE_ANDWC       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT20 | OP_CODE_FLD_DW2_W,
    OP_CODE_ANDDC       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT20 | OP_CODE_FLD_DW2_D,
    
    OP_CODE_NAND        = OP_CODE_GRP_ALU | OP_CODE_FAM_AND | OP_CODE_BIT21,
    OP_CODE_NANDB       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_FLD_DW2_B,
    OP_CODE_NANDH       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_FLD_DW2_H,
    OP_CODE_NANDW       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_FLD_DW2_W,
    OP_CODE_NANDD       = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_FLD_DW2_D,
    
    OP_CODE_NANDC       = OP_CODE_GRP_ALU | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_BIT20,
    OP_CODE_NANDBC      = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_BIT20 | OP_CODE_FLD_DW2_B,
    OP_CODE_NANDHC      = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_BIT20 | OP_CODE_FLD_DW2_H,
    OP_CODE_NANDWC      = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_BIT20 | OP_CODE_FLD_DW2_W,
    OP_CODE_NANDDC      = OP_CODE_GRP_MEM | OP_CODE_FAM_AND | OP_CODE_BIT21 | OP_CODE_BIT20 | OP_CODE_FLD_DW2_D,
    
    OP_CODE_OR          = OP_CODE_GRP_ALU | OP_CODE_FAM_OR,
    OP_CODE_ORB         = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_FLD_DW2_B,
    OP_CODE_ORH         = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_FLD_DW2_H,
    OP_CODE_ORW         = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_FLD_DW2_W,
    OP_CODE_ORD         = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_FLD_DW2_D,
    
    OP_CODE_NOR         = OP_CODE_GRP_ALU | OP_CODE_FAM_OR,
    OP_CODE_NORB        = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_B,
    OP_CODE_NORH        = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_H,
    OP_CODE_NORW        = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_W,
    OP_CODE_NORD        = OP_CODE_GRP_MEM | OP_CODE_FAM_OR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_D,
    
    OP_CODE_XOR         = OP_CODE_GRP_ALU | OP_CODE_FAM_XOR,
    OP_CODE_XORB        = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_FLD_DW2_B,
    OP_CODE_XORH        = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_FLD_DW2_H,
    OP_CODE_XORW        = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_FLD_DW2_W,
    OP_CODE_XORD        = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_FLD_DW2_D,
    
    OP_CODE_XNOR        = OP_CODE_GRP_ALU | OP_CODE_FAM_XOR | OP_CODE_BIT21,
    OP_CODE_XNORB       = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_B,
    OP_CODE_XNORH       = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_H,
    OP_CODE_XNORW       = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_W,
    OP_CODE_XNORD       = OP_CODE_GRP_MEM | OP_CODE_FAM_XOR | OP_CODE_BIT21 | OP_CODE_FLD_DW2_D,

    OP_CODE_CMPEQ       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EQ,
    OP_CODE_CMPLT       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LT,
    OP_CODE_CMPNE       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_NE,
    OP_CODE_CMPLE       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LE,
    OP_CODE_CMPEV       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EV,
    OP_CODE_CMPOD       = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_OD,
    
    OP_CODE_CMPBEQ      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EQ | OP_CODE_FLD_DW2_B,
    OP_CODE_CMPBLT      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LT | OP_CODE_FLD_DW2_B,
    OP_CODE_CMPBNE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_NE | OP_CODE_FLD_DW2_B,
    OP_CODE_CMPBLE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LE | OP_CODE_FLD_DW2_B,
    OP_CODE_CMPBEV      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EV | OP_CODE_FLD_DW2_B,
    OP_CODE_CMPBOD      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_OD | OP_CODE_FLD_DW2_B,
    
    OP_CODE_CMPHEQ      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EQ | OP_CODE_FLD_DW2_H,
    OP_CODE_CMPHLT      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LT | OP_CODE_FLD_DW2_H,
    OP_CODE_CMPHNE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_NE | OP_CODE_FLD_DW2_H,
    OP_CODE_CMPHLE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LE | OP_CODE_FLD_DW2_H,
    OP_CODE_CMPHEV      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EV | OP_CODE_FLD_DW2_H,
    OP_CODE_CMPHOD      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_OD | OP_CODE_FLD_DW2_H,
    
    OP_CODE_CMPWEQ      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EQ | OP_CODE_FLD_DW2_W,
    OP_CODE_CMPWLT      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LT | OP_CODE_FLD_DW2_W,
    OP_CODE_CMPWNE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_NE | OP_CODE_FLD_DW2_W,
    OP_CODE_CMPWLE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LE | OP_CODE_FLD_DW2_W,
    OP_CODE_CMPWEV      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EV | OP_CODE_FLD_DW2_W,
    OP_CODE_CMPWOD      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_OD | OP_CODE_FLD_DW2_W,
    
    OP_CODE_CMPDEQ      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EQ | OP_CODE_FLD_DW2_D,
    OP_CODE_CMPDLT      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LT | OP_CODE_FLD_DW2_D,
    OP_CODE_CMPDNE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_NE | OP_CODE_FLD_DW2_D,
    OP_CODE_CMPDLE      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_LE | OP_CODE_FLD_DW2_D,
    OP_CODE_CMPDEV      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_EV | OP_CODE_FLD_DW2_D,
    OP_CODE_CMPDOD      = OP_CODE_GRP_ALU | OP_CODE_FAM_CMP | OP_CODE_FLD_OD | OP_CODE_FLD_DW2_D,
    
    OP_CODE_EXTR        = OP_CODE_GRP_ALU | OP_CODE_FAM_EXTR,
    OP_CODE_EXTRS       = OP_CODE_GRP_ALU | OP_CODE_FAM_EXTR | OP_CODE_BIT21,
    
    OP_CODE_VEXTR        = OP_CODE_GRP_ALU | OP_CODE_FAM_EXTR | OP_CODE_BIT20,
    OP_CODE_VEXTRS       = OP_CODE_GRP_ALU | OP_CODE_FAM_EXTR | OP_CODE_BIT21 | OP_CODE_BIT21,
   
    
};
#endif

#endif

