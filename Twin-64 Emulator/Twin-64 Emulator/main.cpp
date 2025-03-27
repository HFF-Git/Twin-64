//
//  main.cpp
//  Twin-64 Emulator
//
//  Created by Helmut Fieres on 26.03.25.
//

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

