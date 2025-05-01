//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Sketch
//
//------------------------------------------------------------------------------------------------------------
// This module contains all of the mothods for the different windows that the simlulator supports. The
// exception is the command window, which is in a separate file. A window generally consist of a banner line,
// shown in inverse video and a nuber of body lines.
//
//------------------------------------------------------------------------------------------------------------
//
// Twin-64 - A 64-bit CPU - Sketch
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


//
// 52 bit virtual address: 20bit segment, 32 offset. => 4 Exabytes
// 4Gb Segments, 1Mio Segments
//

// Assembler notes:
//
// opCode [ .<opt> ] Rr, <imm>
// opCode [ .<opt> ] Rr, Ra
// opCode [ .<opt> ] Rr, Ra, Rb
// opCode [ .<opt> ] Rr, ( Rb )
// opCode [ .<opt> ] Rr, <ofs> ( Rb )
// opCode [ .<opt> ] Rr, Ra ( Rb )
// opCode [ .<opt> ] <target> [, Rr ]
//
// -> very few different formats
//
// ( <instr1> : <instr2> )      -> parallel
// ( <instr1> :: <instr2> )     -> serialized

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
    OP_BR_GATE      = OP_GRP_BR  | 0x04,
    OP_BR_BR        = OP_GRP_BR  | 0x05,
    OP_BR_BV        = OP_GRP_BR  | 0x06,
    OP_BR_CBR       = OP_GRP_BR  | 0x07,
    OP_BR_TBR       = OP_GRP_BR  | 0x08,
    OP_BR_MBR       = OP_GRP_BR  | 0x09,
    
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
    { .op = OP_BR_GATE ,    .name = "GATE"  },
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

//************************************************************************************************************
//************************************************************************************************************
//
// Basics
//
//************************************************************************************************************
//************************************************************************************************************
static inline bool isAligned( int64_t adr, int align ) {
    
    return (( adr & ( align - 1 )) == 0 );
}

static inline bool isInRange( int64_t adr, int64_t low, int64_t high ) {
    
    return(( adr >= low ) && ( adr <= high ));
}

static inline int64_t roundup( uint64_t arg ) {
    
    return( arg ); // for now ...
}

static inline int64_t extractBit( int64_t arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline int64_t extractField( int64_t arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

static inline int64_t extractSignedField( int64_t arg, int bitpos, int len ) {
    
    int64_t field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
    
}

static inline int64_t depositField( int64_t word, int bitpos, int len, int64_t value) {
    
    int64_t mask = (( 1ULL << len ) - 1 ) << bitpos;
    return ( word & ~mask ) | (( value << bitpos ) & mask );
}

bool willAddOverflow( int64_t a, int64_t b ) {
    
    if (( b > 0 ) && ( a > INT64_MAX - b )) return true;
    if (( b < 0 ) && ( a < INT64_MIN - b )) return true;
    return false;
}

bool willSubOverflow( int64_t a, int64_t b ) {
    
    if (( b < 0 ) && ( a > INT64_MAX + b )) return true;
    if (( b > 0 ) && ( a < INT64_MIN + b )) return true;
    return false;
}

bool willShiftLftOverflow( int64_t a, int shift ) {
    
    if (( shift < 0 ) || ( shift >= 64 )) return true;
    if ( a == 0 ) return false;
    
    int64_t max = INT64_MAX >> shift;
    int64_t min = INT64_MIN >> shift;
    
    return (( a > max ) || ( a < min ));
}


//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
const char *opCodeToStr( uint8_t opCode ) {
    
    int entries = sizeof( opCodeTab ) / sizeof( opCodeTab[0]);
    
    for ( int i = 0; i < entries; i++ ) {
        
        if ( opCodeTab[ i ].op == opCode ) return((char *) &opCodeTab[ i ].name );
    }
                                                  
    return((char*) &"***" );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint8_t strToOpCode( char *opStr ) {
    
    int entries = sizeof( opCodeTab ) / sizeof( opCodeTab[0]);
    
    for ( int i = 0; i < entries; i++ ) {
       
        if ( strcmp( opStr, opCodeTab[ i ].name ) == 0 ) return ( opCodeTab[ i ].op );
    }
                                                  
    return( 0 );
}


//************************************************************************************************************
//************************************************************************************************************
//
// Basics
//
//************************************************************************************************************
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
//
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
        this -> trapInfo1 = trapInfo1;
        this -> trapInfo1 = trapInfo1;
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
struct T64PhysMem {
    
public:
    
    T64PhysMem( int64_t size );
    
    void        reset( );
    uint64_t    readMem( int64_t adr, int len, bool signExtend = false );
    void        writeMem( int64_t adr, uint64_t arg, int len );
   
    // ??? goes away....
    int8_t      getMem8( int64_t adr );
    void        setMem8( int64_t adr, int8_t arg );
    
    int16_t     getMem16( int64_t adr );
    void        setMem16( int64_t adr, int16_t arg );
    
    int32_t     getMem32( int64_t adr );
    void        setMem32( int64_t adr, int32_t arg );
    
    int64_t     getMem64( int64_t adr );
    void        setMem64( int64_t adr, int64_t arg );
    
   
    
private:
    
    int64_t    size = 0;
    uint8_t    *mem = nullptr;
    
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64IoMem {
    
public:
    
    T64IoMem( int64_t size );
    
    void        reset( );
    uint64_t    readMem( int64_t adr, int len, bool signExtend = false );
    void        writeMem( int64_t adr, uint64_t arg, int len );
    
private:
    
    int64_t    size = 0;
    
    // how to structure this space...
    
};

//------------------------------------------------------------------------------------------------------------
//
// ??? how to best store the field data ?
//------------------------------------------------------------------------------------------------------------
struct T64TlbEntry {
    
public:
    
    T64TlbEntry( );
    void            reset( );
 
    bool            valid;
    uint8_t         accessId;
    uint32_t        protectId;
    int64_t         vAdr;
    int64_t         pAdr;
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64Tlb {
    
public:
    
    T64Tlb( );
    
    void            reset( );
    
    T64TlbEntry     *lookupTlb( int64_t vAdr );
    void            purgeTlb( int64_t vAdr );
    
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index, T64TlbEntry *entry );
    
private:
    
    int             size = 0;
    T64TlbEntry     *map = nullptr;
};

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
struct T64Cpu {
    
public:
    
    T64Cpu( T64PhysMem *mem, T64IoMem *io, T64Tlb *tlb );
    
    void            reset( );
    void            step( );
    
    void            step( int count );
    
    uint64_t        getGeneralReg( int index );
    void            setGeneralReg( int index, int64_t val );
    
    uint64_t        getControlReg( int index );
    void            setControlReg( int index, int64_t val );
    
    uint64_t        getPswReg( );
    void            setPswReg( int64_t val );
    
    T64TlbEntry     *getTlbENtry( int index );
    void            setTlbEntry( int index );
    
private:
    
    void            fetchInstr( );
    void            executeInstr( );
    void            translateAdr( int64_t vAdr, int64_t *pAdr );
    int64_t         dataRead( int64_t vAdr, int len  );
    void            dataWrite( int64_t vAdr, int64_t val, int len );
   
private:
    
    int64_t         ctlRegFile[ MAX_CREGS ];
    int64_t         genRegFile[ MAX_GREGS ];
    int64_t         pswReg;
    int64_t         instrReg;
    int64_t         resvReg;
    
    T64PhysMem      *mem    = nullptr;
    T64IoMem        *io     = nullptr;
    T64Tlb          *tlb    = nullptr;
    
    // ??? trap info ?
    
};

//************************************************************************************************************
//************************************************************************************************************
//
// Physical memory
//
//************************************************************************************************************
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64PhysMem::T64PhysMem( int64_t size ) {
 
    this -> size = roundup( size );
    this -> mem  = (uint8_t *) calloc( this -> size, sizeof( uint8_t ));
}

void T64PhysMem::reset( ) {
    
    if ( mem != nullptr ) free( mem );
    this -> mem  = (uint8_t *) calloc( size, sizeof( uint8_t ));
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64PhysMem::readMem( int64_t adr, int len, bool signExtend ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        uint64_t val= mem[ adr ];
        if ( signExtend ) val = extractSignedField( val, 63, 8 );
        return( val );
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2))  throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        uint64_t val = 0;
        val |= (int16_t) mem[ adr ] << 8;
        val |= (int16_t) mem[ adr + 1 ];
        if ( signExtend ) val = extractSignedField( val, 63, 16 );
        return( val );
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        uint64_t val = 0;
        val |= (int32_t) mem[ adr]     << 24;
        val |= (int32_t) mem[ adr + 1 ] << 16;
        val |= (int32_t) mem[ adr + 2 ] << 8;
        val |= (int32_t) mem[ adr + 3 ];
        if ( signExtend ) val = extractSignedField( val, 63, 32 );
        return( val );
        
    }
    else if ( len == 64 ) {
        
        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        int64_t val = 0;
        val |= (int64_t) mem[ adr ]     << 56;
        val |= (int64_t) mem[ adr + 1 ] << 48;
        val |= (int64_t) mem[ adr + 2 ] << 40;
        val |= (int64_t) mem[ adr + 3 ] << 32;
        val |= (int64_t) mem[ adr + 4 ] << 24;
        val |= (int64_t) mem[ adr + 5 ] << 16;
        val |= (int64_t) mem[ adr + 6 ] << 8;
        val |= (int64_t) mem[ adr + 7 ];
        return ( val );
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64PhysMem::writeMem( int64_t adr, uint64_t arg, int len ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        mem[ adr ] = arg & 0xFF;
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr ]      = ( arg >> 8  ) & 0xFF;
        mem[ adr + 1 ]  = ( arg       ) & 0xFF;
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr ]     = ( arg >> 24 ) & 0xFF;
        mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 3 ] = ( arg       ) & 0xFF;
        
    }
    else if ( len == 64 ) {
        
        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
        mem[ adr]     = ( arg >> 56 ) & 0xFF;
        mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
        mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
        mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
        mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
        mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
        mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
        mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}


//------------------------------------------------------------------------------------------------------------
//
// ??? may go away...
//------------------------------------------------------------------------------------------------------------
int8_t T64PhysMem::getMem8( int64_t adr ) {
    
    if ( adr >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    return mem[ adr ];
}

void T64PhysMem::setMem8( int64_t adr, int8_t arg ) {
    
    if (adr >= size) throw T64Trap( PHYS_MEM_ADR_TRAP );
    mem[ adr ] = arg;
}

int16_t T64PhysMem::getMem16( int64_t adr ) {
    
    if ( adr + 1 >= size )      throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 2))  throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int16_t val = 0;
    val |= (int16_t) mem[ adr ] << 8;
    val |= (int16_t) mem[ adr + 1 ];
    return val;
}

void T64PhysMem::setMem16( int64_t adr, int16_t arg ) {
    
    if ( adr + 1 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 2)) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr ]      = ( arg >> 8  ) & 0xFF;
    mem[ adr + 1 ]  = ( arg       ) & 0xFF;
}

int32_t T64PhysMem::getMem32( int64_t adr ) {
    
    if ( adr + 3 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int32_t val = 0;
    val |= (int32_t) mem[ adr]     << 24;
    val |= (int32_t) mem[ adr + 1 ] << 16;
    val |= (int32_t) mem[ adr + 2 ] << 8;
    val |= (int32_t) mem[ adr + 3 ];
    return val;
}

void T64PhysMem::setMem32( int64_t adr, int32_t arg ) {
    
    if ( adr + 3 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 4 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr ]     = ( arg >> 24 ) & 0xFF;
    mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 3 ] = ( arg       ) & 0xFF;
}

int64_t T64PhysMem::getMem64( int64_t adr ) {
    
    if ( adr + 7 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    int64_t val = 0;
    val |= (int64_t) mem[ adr ]     << 56;
    val |= (int64_t) mem[ adr + 1 ] << 48;
    val |= (int64_t) mem[ adr + 2 ] << 40;
    val |= (int64_t) mem[ adr + 3 ] << 32;
    val |= (int64_t) mem[ adr + 4 ] << 24;
    val |= (int64_t) mem[ adr + 5 ] << 16;
    val |= (int64_t) mem[ adr + 6 ] << 8;
    val |= (int64_t) mem[ adr + 7 ];
    return val;
}

void T64PhysMem::setMem64( int64_t adr, int64_t arg ) {
    
    if ( adr + 7 >= size ) throw T64Trap( PHYS_MEM_ADR_TRAP );
    if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
    
    mem[ adr]     = ( arg >> 56 ) & 0xFF;
    mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
    mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
    mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
    mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
    mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
}

//************************************************************************************************************
//************************************************************************************************************
//
// IO Memory
//
//************************************************************************************************************
//************************************************************************************************************
T64IoMem::T64IoMem( int64_t size ) {
    
}

void T64IoMem::reset( ) {
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64IoMem::readMem( int64_t adr, int len, bool signExtend ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
        return ( 0 );
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 3 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else if ( len == 64 ) {

        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        return ( 0 );
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64IoMem::writeMem( int64_t adr, uint64_t arg, int len ) {
    
    if ( ! isInRange( adr, IO_MEM_START, IO_MEM_LIMIT )) throw T64Trap( IO_MEM_ADR_TRAP );
    
    if ( len == 8 ) {
        
    }
    else if ( len == 16 ) {
        
        if ( ! isAligned( adr, 2 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
    }
    else if ( len == 32 ) {
        
        if ( ! isAligned( adr, 3 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
       
    }
    else if ( len == 64 ) {

        if ( ! isAligned( adr, 8 )) throw T64Trap( MEM_ADR_ALIGN_TRAP );
        
    }
    else throw T64Trap( MEM_ADR_ALIGN_TRAP );
}

//************************************************************************************************************
//************************************************************************************************************
//
// TLB Entry
//
//************************************************************************************************************
//************************************************************************************************************
T64TlbEntry::T64TlbEntry( ) {
    
}

void T64TlbEntry::reset( ) {
    
}



//************************************************************************************************************
//************************************************************************************************************
//
// TLB
//
//************************************************************************************************************
//************************************************************************************************************
T64Tlb::T64Tlb( ) {
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Tlb::reset( ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        map[ i ].reset( );
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64TlbEntry *T64Tlb::lookupTlb( int64_t vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) return( ptr );
    }
    
    return( nullptr );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Tlb::purgeTlb( int64_t vAdr ) {
    
    for ( int i = 0; i < size; i++ ) {
        
        T64TlbEntry *ptr = &map[ i ];
        
        if (( ptr -> valid ) && ( ptr -> vAdr == vAdr )) ptr -> valid = false;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------


//************************************************************************************************************
//************************************************************************************************************
//
// CPU
//
//************************************************************************************************************
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
T64Cpu::T64Cpu( T64PhysMem *mem, T64IoMem *io, T64Tlb *tlb ) {
    
    this -> mem = mem;
    this -> io  = io;
    this -> tlb = tlb;
    
    reset( );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::reset( ) {
    
    for ( int i = 0; i < MAX_CREGS; i++ ) ctlRegFile[ i ] = 0;
    for ( int i = 0; i < MAX_GREGS; i++ ) genRegFile[ i ] = 0;
    pswReg     = 0;
    instrReg   = 0;
    resvReg    = 0;
    
    tlb -> reset( );
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64Cpu::getGeneralReg( int index ) {
    
    if ( index == 0 )   return( 0 );
    else                return( genRegFile[ index % MAX_GREGS ] );
}

void T64Cpu::setGeneralReg( int index, int64_t val ) {
    
    if ( index != 0 ) genRegFile[ index % MAX_GREGS ] = val;
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64Cpu::getControlReg( int index ) {
    
    return( ctlRegFile[ index % MAX_CREGS ] );
}

void T64Cpu::setControlReg( int index, int64_t val ) {
    
    ctlRegFile[ index % MAX_CREGS ] = val;
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
uint64_t T64Cpu::getPswReg( ) {
    
    return( pswReg );
}

void T64Cpu::setPswReg( int64_t val ) {
    
    pswReg = val;
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::translateAdr( int64_t vAdr, int64_t *pAdr ) {
    
    if ( extractField( vAdr, 32, 20 ) == 0 ) {  // physical address range ?
        
        if ( ! extractBit( pswReg, 0 )) { // privileged mode ?
            
            throw T64Trap( 0 ); // priv violation
        }
        
        *pAdr = vAdr;
    }
    else {
        
        T64TlbEntry *tlbPtr = tlb -> lookupTlb( vAdr );
        
        if ( tlbPtr == nullptr ) {
            
            throw T64Trap( 0 ); // dara tlb trap
        }
        
        // ??? access check .....
        
        // protection check ...
        
        if ( extractBit( pswReg, 0 )) {
            
            uint32_t pId = tlbPtr -> protectId;
            
            if ( ! (( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ) ||
                    ( extractField( ctlRegFile[ 0 ], 0, 32 ) == pId ))) {
                
                throw T64Trap( 0 ); // dara protection trap
            }
        }
        
        // ??? what else to check ?
        
        *pAdr = tlbPtr -> pAdr;
    }
}


//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
int64_t T64Cpu::dataRead( int64_t vAdr, int len ) {
    
    try {
        
        int64_t pAdr = 0;
        
        translateAdr( vAdr , &pAdr );
        return ( mem -> readMem( pAdr, len ));
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

void T64Cpu::dataWrite( int64_t vAdr, int64_t val, int len ) {
    
    try {
        
        int64_t pAdr = 0;
        
        translateAdr( vAdr , &pAdr );
        mem -> writeMem( pAdr, val, len );
    }
    
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::fetchInstr( ) {
    
    try {
        
        int64_t pAdr = 0;
        
        translateAdr( extractField( pswReg, 63, 52 ) , &pAdr );
        instrReg = mem -> readMem( pAdr, 32 );
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::executeInstr( ) {
    
    try {
        
        int opCode      = (int) extractField( instrReg, 26, 6 );
        int regRIndx    = (int) extractField( instrReg, 22, 4 );
        int regBIndx    = (int) extractField( instrReg, 15, 4 );
        int regAIndx    = (int) extractField( instrReg, 9, 4 );
        
        switch ( opCode ) {
                
            case OP_ALU_NOP: {
                
            } break;
                
            case OP_ALU_AND: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                if ( extractBit( instrReg, 20 ))   valA = ~ valA;
                
                int64_t valR = valB | valA;
                
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_OR: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                int64_t valR = valB | valA;
                
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_XOR: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                int64_t valR = valB ^ valA;
                
                if ( extractBit( instrReg, 21 )) valR = ~ valR;
                setGeneralReg( regRIndx, valR );
                
            } break;
                
            case OP_ALU_ADD: {
                
                uint64_t valB = getGeneralReg( regBIndx );
                uint64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                if ( ! willAddOverflow( valB, valA )) {
                   
                    int64_t valR = valB + valA;
                    setGeneralReg( regRIndx, valR );
                }
                else throw T64Trap( OVERFLOW_TRAP );
                
            } break;
                
            case OP_ALU_SUB: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                if ( ! willSubOverflow( valB, valA )) {
                    
                    int64_t valR = valB - valA;
                    setGeneralReg( regRIndx, valR );
                }
                else throw T64Trap( OVERFLOW_TRAP );
                
            } break;
                
            case OP_ALU_CMP: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                if ( extractBit( instrReg, 19 ))   valA = extractSignedField( instrReg, 0, 19 );
                else                            valA = getGeneralReg( regAIndx );
                
                // ....
                
            } break;
                
            case OP_ALU_EXTR: {
                
            } break;
                
            case OP_ALU_DEP: {
                
            } break;
                
            case OP_ALU_DSR: {
                
            } break;
                
            case OP_ALU_CHK: {
                
            } break;
                
            case OP_MEM_LD: {
                
            } break;
                
            case OP_MEM_ST: {
                
            } break;
                
            case OP_MEM_LDR: {
                
                // translate address
                // set reserved Reg: bit 63 -> true, rest physical address ( of cache line ? )
                
            } break;
                
            case OP_MEM_STC: {
                
                // translatre address
                // check resvered flag. if set store val, reset reserved reg. return 0. else return 1.
                
            } break;
                
            case OP_MEM_AND: {
                
                int64_t valB = getGeneralReg( regBIndx );
                int64_t valA = 0;
                
                // ... now fetch the memory data...
                
                int64_t valR = 0;
                
            } break;
                
            case OP_MEM_OR: {
                
            } break;
                
            case OP_MEM_XOR: {
                
            } break;
                
            case OP_MEM_ADD: {
                
            } break;
                
            case OP_MEM_SUB: {
                
            } break;
                
            case OP_MEM_CMP: {
                
            } break;
                
            case OP_BR_LDI: {
                
            } break;
                
            case OP_BR_ADDIL: {
                
            } break;
                
            case OP_BR_LDO: {
                
            } break;
                
            case OP_BR_B: {
                
            } break;
                
            case OP_BR_GATE: {
                
            } break;
                
            case OP_BR_BR: {
                
            } break;
                
            case OP_BR_BV: {
                
            } break;
                
            case OP_BR_CBR: {
                
            } break;
                
            case OP_BR_TBR: {
                
            } break;
                
            case OP_BR_MBR: {
                
            } break;
                
            case OP_SYS_MR: {
                
            } break;
                
            case OP_SYS_MST: {
                
            } break;
                
            case OP_SYS_LPA: {
                
            } break;
                
            case OP_SYS_PRB: {
                
            } break;
                
            case OP_SYS_ITLB: {
                
            } break;
                
            case OP_SYS_DTLB: {
                
            } break;
                
            case OP_SYS_PCA: {
                
            } break;
                
            case OP_SYS_DIAG: {
                
            } break;
                
            case OP_SYS_BRK: {
                
            } break;
                
            case OP_SYS_RFI: {
                
            } break;
                
            default: ;
        }
    }
    catch ( const T64Trap t ) {
        
        // can do someting before reraising ....
        throw;
    }
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void T64Cpu::step( ) {
    
    try {
        
        fetchInstr( );
        executeInstr( );
    }
    
    catch ( const T64Trap t ) {
        
    }
}

//************************************************************************************************************
//************************************************************************************************************
//
// Main
//
//************************************************************************************************************
//************************************************************************************************************

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
void parseParameters( int argc, const char * argv[] ) {
    
    
}

//------------------------------------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------------------------------------
int main( int argc, const char * argv[] ) {
    
    parseParameters( argc, argv );
    
    T64PhysMem  *mem = new T64PhysMem( 2040 );
    T64IoMem    *io  = new T64IoMem( 2048 );
    T64Tlb      *tlb = new T64Tlb( );
    T64Cpu      *cpu = new T64Cpu( mem, io, tlb );
    
    cpu -> reset( );
    
    printf( "OP: %s\n", opCodeToStr( OP_ALU_CMP ));
    
    printf( "OP: 0x%x\n", strToOpCode(( char*) "OR" ));
    
    return 0;
}

