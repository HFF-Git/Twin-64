//
//  main.cpp
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 26.03.25.
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
// opCode [ .<opt> ] <ofs> [, Rr ]
//
// -> very few different formats
//
// ( <instr1> : <instr2> )      -> parallel
// ( <instr1> :: <instr2> )     -> serialized

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// ------- Constants

const int MAX_GREGS     = 16;
const int MAX_CREGS     = 16;
const int PAGE_SIZE     = 16 * 1024;


// ------- Basics

static inline uint64_t extractBit( uint64_t arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

static inline uint64_t extractField( uint64_t arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
}

static inline int64_t extractSignedField( uint64_t arg, int bitpos, int len ) {
    
    uint64_t field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return (int64_t) ( field << ( 64 - len )) >> ( 64 - len );
    else                return (int64_t) field;
    
}

static inline uint64_t depositField(uint64_t word, int bitpos, int len, uint64_t value) {
    
    uint64_t mask = (( 1ULL << len ) - 1 ) << bitpos;
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


// ----- Memory

struct T64PhysMem {
    
public:
    
    T64PhysMem( uint64_t size );
    
    uint8_t     getMem8( uint64_t adr );
    void        setMem8( uint64_t adr, uint8_t arg );
    
    uint16_t    getMem16( uint64_t adr );
    void        setMem16( uint64_t adr, uint16_t arg );
    
    uint32_t    getMem32( uint64_t adr );
    void        setMem32( uint64_t adr, uint32_t arg );
    
    uint64_t    getMem64( uint64_t adr );
    void        setMem64( uint64_t adr, uint64_t arg );
    
private:
    
    uint64_t    size = 0;
    uint64_t    *mem = nullptr;
    
};

// ----- IO memory

struct T64IoMem {
    
public:
    
    T64IoMem( uint64_t size );
    
    uint8_t     getMem8( uint64_t adr );
    void        setMem8( uint64_t adr, uint8_t arg );
    
    uint16_t    getMem16( uint64_t adr );
    void        setMem16( uint64_t adr, uint16_t arg );
    
    uint32_t    getMem32( uint64_t adr );
    void        setMem32( uint64_t adr, uint32_t arg );
    
    uint64_t    getMem64( uint64_t adr );
    void        setMem64( uint64_t adr, uint64_t arg );
    
private:
    
    uint64_t    size = 0;
    
    // how to structure this space...
    
};

// ---- registers
// ??? should we implement a latch pattern ?

struct T64Reg {
    
public:
    
    T64Reg( );
    
    uint64_t    get( )              { return( valOut ); }
    void        put( uint64_t arg ) { valIn = arg ; }
    void        reset( )            { valIn = 0; valOut = 0; }
    void        tick( )             { valOut = valIn; }
    
private:
    
    int64_t     valIn  = 0;
    int64_t     valOut = 0;
};

// ??? do we need a cache ?

struct T64_CacheEntry {
    
    
};

struct T64Cache {
    
public:
    
    T64Cache( T64PhysMem *physMem );
    
    void        reset( );
    
    void        purgeCacheLine( uint64_t adr );
    void        flushCacheLine( uint64_t adr );
    
    uint8_t     getMem8( uint64_t adr );
    void        setMem8( uint64_t adr, uint8_t arg );
    
    uint16_t    getMem16( uint64_t adr );
    void        setMem16( uint64_t adr, uint16_t arg );
    
    uint32_t    getMem32( uint64_t adr );
    void        setMem32( uint64_t adr, uint32_t arg );
    
    uint64_t    getMem64( uint64_t adr );
    void        setMem64( uint64_t adr, uint64_t arg );
    
    void        getCacheLine( int index );
    
private:
    
    T64PhysMem *mem;
    
};

// ------ Tlb

struct T64TlbEntry {
    
    
};

struct T64Tlb {
    
public:
    
    T64Tlb( );
    
    void            reset( );
    
    void            lookupTlb( uint64_t adr, T64TlbEntry *entry );
    void            purgeTlb( uint64_t adr, T64TlbEntry *entry );
    
    T64TlbEntry     *getTlbEntry( int index );
    void            setTlbEntry( int index, T64TlbEntry *entry );
    
private:
    
    T64TlbEntry *map; // allocate ...
    
};

// ------- CPU

struct T64Cpu {
    
public:
    
    T64Cpu( T64PhysMem *mem, T64IoMem *io, T64Tlb *tlb );
    
    void            reset( );
    void            step( );
    
    void            step( int count );
    
    uint64_t        getGeneralReg( int index );
    void            setGeneralReg( int index, uint64_t val );
    
    uint64_t        getControlReg( int index );
    void            setControlReg( int index, uint64_t val );
    
    uint64_t        getPswReg( );
    void            setPswReg( uint64_t val );
    
    T64TlbEntry     *getTlbENtry( int index );
    void            setTlbEntry( int index );
    
private:
    
    void            fetchInstr( );
    void            executeInstr( );
    bool            accessCheck( );
    bool            protectionCheck( );
    uint64_t        virtToPhys( uint64_t vAdr );
    void            updateState( );
    
private:
    
    T64Reg          cRegs[ MAX_CREGS ];
    T64Reg          gRegs[ MAX_GREGS ];
    T64Reg          psw;
    T64Reg          instr;
    
    T64PhysMem      *mem    = nullptr;
    T64IoMem        *io     = nullptr;
    T64Tlb          *tlb    = nullptr;
    
    // ??? trap info ?
    
};


// ------- Methods

T64Reg::T64Reg ( ) {
    
    valIn   = 0;
    valOut  = 0;
}


T64PhysMem::T64PhysMem( uint64_t size ) {
    
    
}

uint8_t T64PhysMem::getMem8( uint64_t adr ) {
    
    if ( adr >= size ) ;
    return mem[ adr ];
}

void T64PhysMem::setMem8( uint64_t adr, uint8_t arg ) {
    
    if (adr >= size) ;
    mem[ adr ] = arg;
}

uint16_t T64PhysMem::getMem16( uint64_t adr ) {
    
    if ( adr + 1 >= size ) ;
    
    uint16_t val = 0;
    val |= (uint16_t) mem[ adr ] << 8;
    val |= (uint16_t) mem[ adr + 1 ];
    return val;
}

void T64PhysMem::setMem16( uint64_t adr, uint16_t arg ) {
    
    if ( adr + 1 >= size ) ;
    mem[ adr ]      = ( arg >> 8  ) & 0xFF;
    mem[ adr + 1 ]  = ( arg       ) & 0xFF;
}

uint32_t T64PhysMem::getMem32( uint64_t adr ) {
    
    if ( adr + 3 >= size ) ;
    
    uint32_t val = 0;
    val |= (uint32_t) mem[ adr]     << 24;
    val |= (uint32_t) mem[ adr + 1 ] << 16;
    val |= (uint32_t) mem[ adr + 2 ] << 8;
    val |= (uint32_t) mem[ adr + 3 ];
    return val;
}

void T64PhysMem::setMem32( uint64_t adr, uint32_t arg ) {
    
    if ( adr + 3 >= size ) ;
    mem[ adr ]     = ( arg >> 24 ) & 0xFF;
    mem[ adr + 1 ] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 2 ] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 3 ] = ( arg       ) & 0xFF;
}

uint64_t T64PhysMem::getMem64( uint64_t adr ) {
    
    if ( adr + 7 >= size ) ;
    uint64_t val = 0;
    val |= (uint64_t) mem[ adr ]     << 56;
    val |= (uint64_t) mem[ adr + 1 ] << 48;
    val |= (uint64_t) mem[ adr + 2 ] << 40;
    val |= (uint64_t) mem[ adr + 3 ] << 32;
    val |= (uint64_t) mem[ adr + 4 ] << 24;
    val |= (uint64_t) mem[ adr + 5 ] << 16;
    val |= (uint64_t) mem[ adr + 6 ] << 8;
    val |= (uint64_t) mem[ adr + 7 ];
    return val;
}

void T64PhysMem::setMem64( uint64_t adr, uint64_t arg ) {
    
    if ( adr + 7 >= size ) ;
    mem[ adr]     = ( arg >> 56 ) & 0xFF;
    mem[ adr + 1] = ( arg >> 48 ) & 0xFF;
    mem[ adr + 2] = ( arg >> 40 ) & 0xFF;
    mem[ adr + 3] = ( arg >> 32 ) & 0xFF;
    mem[ adr + 4] = ( arg >> 24 ) & 0xFF;
    mem[ adr + 5] = ( arg >> 16 ) & 0xFF;
    mem[ adr + 6] = ( arg >> 8  ) & 0xFF;
    mem[ adr + 7] = ( arg >> 8  ) & 0xFF;
}




T64IoMem::T64IoMem( uint64_t size ) {
    
}


T64Tlb::T64Tlb( ) {
    
    
}

void T64Tlb::reset( ) {
    
}

T64Cpu::T64Cpu( T64PhysMem *mem, T64IoMem *io, T64Tlb *tlb ) {
    
    this -> mem = mem;
    this -> io  = io;
    this -> tlb = tlb;
    
    reset( );
}

void T64Cpu::reset( ) {
    
    for ( int i = 0; i < MAX_CREGS; i++ ) cRegs[ i ].reset( );
    for ( int i = 0; i < MAX_GREGS; i++ ) gRegs[ i ].reset( );
    psw.reset( );
    instr.reset( );
    
    tlb -> reset( );
}

void T64Cpu::fetchInstr ( ) {
    
    
}

void T64Cpu::executeInstr( ) {
    
    
}

void T64Cpu::step( ) {
    
    try {
        
        fetchInstr( );
        executeInstr( );
    }
    
    catch ( int ) {
        
    }
    
}

// ------- Main

int main(int argc, const char * argv[]) {
    
    T64PhysMem  *mem = new T64PhysMem( 2040 );
    T64IoMem    *io  = new T64IoMem( 2048 );
    T64Tlb      *tlb = new T64Tlb( );
    T64Cpu      *cpu = new T64Cpu( mem, io, tlb );
    
    
    return 0;
}

