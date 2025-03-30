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
// opCode [ .<opt> ] Rr, <ofs>
//
// -> very few different formats

// ( <instr1> : <instr2> )      -> parallel
// ( <instr1> :: <instr2> )     -> serialized

#include <iostream>
 
typedef struct {
    
    uint32_t opGrp  : 2;
    uint32_t opcode : 6;
    uint32_t regA   : 5;
    uint32_t regB   : 5;
    uint32_t regC   : 5;
    uint32_t immediate : 9;
    
} InstrFormatA;

typedef struct {
    
    uint32_t opGrp  : 2;
    uint32_t opcode : 6;
    uint32_t regA   : 5;
    uint32_t regB   : 5;
    uint32_t offset : 14;
    
} InstrFormatB;


typedef union {
    
    uint32_t raw;
    
    InstrFormatA fmtA;
    InstrFormatB fmtB;
    
} Instr;


typedef union {

    struct {
        
        uint64_t        iTemplate   : 3;
        uint64_t        iSerialize  : 1;
        uint64_t        instr1      : 30;
        uint64_t        instr2      : 30;
        
    } ib;
    
    uint64_t raw;
    
} InstrBundle;

// ??? larger than a 64 bit word... Lower portion should be the one that is set via
// ITLB instruction.
//
typedef union {
    
    struct {
        
        uint64_t valid          : 1;
        uint64_t dirty          : 1;
        uint64_t refTrap        : 1;
        uint64_t dRefTrap       : 1;
        uint64_t protectCheck   : 1;
        uint64_t accessRights   : 4;
        uint64_t reserved       : 9;
        uint64_t ppn            : 18;
        uint64_t vpn            : 38;
    };
    
    uint64_t raw;
    
} TlbEntry;

// argument used in ITLB. Just barely fits.

typedef union {
    
    struct {
        
        uint64_t refTrap        : 1;
        uint64_t dRefTrap       : 1;
        uint64_t protectCheck   : 1;
        uint64_t accessRights   : 4;
        uint64_t reserved       : 1;
        uint64_t ppn            : 18;
        uint64_t vpn            : 38;
    };
    
    uint64_t raw;
    
} TlbInfo;

// a two word structure, lots of speace left, still.
typedef union {
    
    struct {
        
        // word 1
        uint64_t valid          : 1;
        uint64_t dirty          : 1;
        uint64_t refTrap        : 1;
        uint64_t dRefTrap       : 1;
        uint64_t protectCheck   : 1;
        uint64_t accessRights   : 4;
        uint64_t vpn            : 38;
        uint64_t reserved1      : 17;
        
        // word2
        uint64_t resrved2       : 26;
        uint64_t ppn            : 18;
        uint64_t nextEntry      : 20;
        
    };
    
    uint64_t raw;
    
} PageTableEntry;


InstrBundle testInstrBundle;
Instr       testInstr;

int main(int argc, const char * argv[]) {
    
    // insert code here...
    std::cout << "INstruction field test\n";
    
    testInstr.raw = 25;
    
    // Set the template and serialize fields
    testInstrBundle.ib.iTemplate        = 5;
    testInstrBundle.ib.iSerialize       = 1;
    
    testInstrBundle.ib.instr1           = (uint32_t) testInstr.raw;
    testInstrBundle.ib.instr2           = (uint32_t) testInstr.raw;

    printf( "Sizeof Instruction Bundle: %d\n", (int)sizeof( InstrBundle ));
    printf( "Sizeof Instruction word: %d\n", (int)sizeof( Instr ));
    printf( "Sizeof FormatA: %d\n", (int)sizeof( InstrFormatA ));
    
    printf("Template: %u\n", (unsigned)testInstrBundle.ib.iTemplate);
    printf("Serialize: %u\n", (unsigned)testInstrBundle.ib.iSerialize);
    printf("Serialize: %u\n", (unsigned)testInstrBundle.ib.instr1);
    printf("Serialize: %u\n", (unsigned)testInstrBundle.ib.instr2);
    
    
    return 0;
}

