//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator window classes
//
//----------------------------------------------------------------------------------------
// This module contains all of the methods for the different simulator windows. The 
// exception is the command window, which is in a separate file. A window generally
// consist of a banner line, shown in inverse video and a number of body lines.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator window classes
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details. You should have received a copy of the GNU General Public
// License along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#include "T64-Common.h"
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"

//----------------------------------------------------------------------------------------
//
//  Windows:
//
//  Program Regs    -> PS
//  General Regs    -> GR
//  Special Regs    -> CR
//  Program Code    -> PC
//  TLB             -> IT, DT
//  Cache           -> IC, DC, UC
//  Text Window     -> TX
//  User Defined    -> UW
//
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// Little bit field helpers.
//
//----------------------------------------------------------------------------------------
inline uint32_t extractBit( T64Word arg, int bitpos ) {
    
    return ( arg >> bitpos ) & 1;
}

inline uint32_t extractField( T64Word arg, int bitpos, int len) {
    
    return ( arg >> bitpos ) & (( 1LL << len ) - 1 );
}

inline int extractSignedField( T64Word arg, int bitpos, int len ) {
    
    int field = ( arg >> bitpos ) & (( 1ULL << len ) - 1 );
    
    if ( len < 64 )  return ( field << ( 64 - len )) >> ( 64 - len );
    else             return ( field );
}

//----------------------------------------------------------------------------------------
// Routine for creating the access rights string. It consists of the page access and 
// the two privilege levels.
//
//----------------------------------------------------------------------------------------
int buildAccessRightsStr( char *bufStr, int type, int p1, int p2 ) {
    
    switch( type ) {
            
        case ACC_READ_ONLY:     return( snprintf( bufStr, 10, 
                                                  "[ro:%1d:%1d]", 
                                                  p1 % 2, 
                                                  p2 % 2 )); 
        

        case ACC_READ_WRITE:    return( snprintf( bufStr, 10, 
                                                  "[rW:%1d:%1d]", 
                                                  p1 % 2, 
                                                  p2 % 2 ));

        case ACC_EXECUTE:       return( snprintf( bufStr, 10, 
                                                  "[ex:%1d:%1d]", 
                                                  p1 % 2, 
                                                  p2 % 2 ));

        default:                return( snprintf( bufStr, 10, 
                                                  "[ex:%1d:%1d]", 
                                                  p1 % 2, 
                                                  p2 % 2 ));        
    }
}

//----------------------------------------------------------------------------------------
// Line sanitizing. We cannot just print out whatever is in the line buffer, since it 
// may contains dangerous escape sequences, which would garble our terminal screen 
// layout. In the command window we just allow "safe" escape sequences, such as changing
// the font color and so on. When we encounter an escape character followed by a "[" 
// character we scan the escape sequence until the final character, which lies between
// 0x40 and 0x7E. Based on the last character, we distinguish between "safe" and 
// "unsafe" escape sequences. In the other cases, we just copy input to output.
//
//----------------------------------------------------------------------------------------
bool isSafeFinalByte( char finalByte ) {
    
    //Example:  m = SGR (color/formatting), others can be added
    return finalByte == 'm';
}

bool isDangerousFinalByte( char finalByte ) {
    
    return strchr("ABCDHfJKnsu", finalByte) != NULL;
}

void sanitizeLine( const char *inputStr, char *outputStr ) {
    
    const char  *src = inputStr;
    char        *dst = outputStr;
    
    while ( *src ) {
        
        if ( *src == '\x1B' ) {
            
            if ( *( src + 1 ) == '\0' ) {
                
                *dst++ = *src++;
            }
            else if ( *( src + 1 ) == '[' ) {
                
                const char *escSeqStart = src;
                src += 2;
                
                while (( *src ) && ( ! ( *src >= 0x40 && *src <= 0x7E ))) src++;
                
                if ( *src ) {
                    
                    char finalByte = *src++;
                    
                    if ( isSafeFinalByte( finalByte )) {
                        
                        while ( escSeqStart < src ) *dst++ = *escSeqStart++;
                        
                    } else continue;
                    
                } else break;
                
            } else *dst++ = *src++;
            
        } else *dst++ = *src++;
    }
    
    *dst = '\0';
}

}; // namespace


//****************************************************************************************
//****************************************************************************************
//
// Methods for the Program State Window class.
//
//----------------------------------------------------------------------------------------
// Object creator.
//
//----------------------------------------------------------------------------------------
SimWinProgState::SimWinProgState( SimGlobals *glb ) : SimWin( glb ) { }

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimWinProgState::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefColumns( 12 + ( 4 * 21 ), 16 );
    setDefColumns( 12 + ( 4 * 21 ), 10 );
    setRadix( 16 );
    setRows( 4 );

    setWinType( WT_PS_WIN );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// The window overrides the setRadix method to set the column width according to the
// radix chosen.
//
//----------------------------------------------------------------------------------------
void SimWinProgState::setRadix( int rdx ) {
    
    SimWin::setRadix( rdx );
    setColumns( getDefColumns( rdx ));
}

//----------------------------------------------------------------------------------------
// Each window consist of a headline and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. The program state banner
// lists the instruction address and the status word.
//
//----------------------------------------------------------------------------------------
void SimWinProgState::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE | FMT_ALIGN_LFT ;
    
    setWinCursor( 1, 1 );
    printTextField((char *) "Program State", ( fmtDesc ), 16 );
    
    printTextField((char *) "Seg:", fmtDesc, 5 );

    printTextField((char *) "Ofs:", fmtDesc, 5 );

    printTextField((char *) "ST:", fmtDesc, 4 );

    #if 0
    // IA Seg: 0x0_0000, Ofs: 0x0000_0000_0000_0000, ST: .........
   
    printNumericField( glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_0 ) & 0xFFFF, fmtDesc | FMT_HALF_WORD, 8 );
    
    printNumericField( glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_1 ), fmtDesc, 12 );
    
    uint32_t stat = glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_0 );
    
    printTextField(( getBit( stat, ST_MACHINE_CHECK )) ? (char *) "M" : (char *) "m", fmtDesc );
    printTextField(( getBit( stat, ST_EXECUTION_LEVEL )) ? (char *) "X" : (char *) "x", fmtDesc );
    printTextField(( getBit( stat, ST_CODE_TRANSLATION_ENABLE )) ? (char *) "C" : (char *) "c", fmtDesc );
    printTextField(( getBit( stat, ST_NULLIFY )) ? (char *) "N" : (char *) "n", fmtDesc );
    printTextField(( getBit( stat, ST_DIVIDE_STEP )) ? (char *) "V" : (char *) "v", fmtDesc );
    printTextField(( getBit( stat, ST_CARRY )) ? (char *) "C" : (char *) "c", fmtDesc );
    
    printTextField(( getBit( stat, ST_REC_COUNTER )) ? (char *) "R" : (char *) "r", fmtDesc );
    printTextField(( getBit( stat, ST_SINGLE_STEP )) ? (char *) "Z" : (char *) "z", fmtDesc );
    printTextField(( getBit( stat, ST_DATA_TRANSLATION_ENABLE )) ? (char *) "D" : (char *) "d", fmtDesc );
    printTextField(( getBit( stat, ST_PROTECT_ID_CHECK_ENABLE )) ? (char *) "P" : (char *) "p", fmtDesc );
    printTextField(( getBit( stat, ST_INTERRUPT_ENABLE )) ? (char *) "E" : (char *) "e", fmtDesc );
    #endif
    
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The body lines are displayed after the
// banner line. The program state window body lists the general registers.
//
//----------------------------------------------------------------------------------------
void SimWinProgState::drawBody( ) {
    
    uint32_t fmtDesc = FMT_DEF_ATTR;
    
    setWinCursor( 2, 1 );
    printTextField((char *) "GR0=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 0; i < 4; i++ ) {

        // ??? issue. these routines do not know anything about the cursor pos---
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 3, 1 );
    printTextField((char *) "GR4=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 4; i < 8; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 4, 1 );
    printTextField((char *) "GR8=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 8; i < 12; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 5, 1 );
    printTextField((char *) "GR12=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 12; i < 16; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the special register window class.
//
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimWinSpecialRegs::SimWinSpecialRegs( SimGlobals *glb )  : SimWin( glb ) { }

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimWinSpecialRegs::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefColumns( 12 + ( 4 * 21 ), 16 );
    setDefColumns( 12 + ( 4 * 21 ), 10 );
    setRadix( 16 );
    setRows( 5 );

    setWinType( WT_CR_WIN );
    setEnable( false );
}

//----------------------------------------------------------------------------------------
// The window overrides the setRadix method to set the column width according to the
// radix chosen.
//
//----------------------------------------------------------------------------------------
void SimWinSpecialRegs::setRadix( int rdx ) {
    
    SimWin::setRadix( rdx );
    setColumns( getDefColumns( getRadix( )));
}

//----------------------------------------------------------------------------------------
// The banner line for the special register window.
//
//----------------------------------------------------------------------------------------
void SimWinSpecialRegs::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
    
    setWinCursor( 1, 1 );
    printTextField((char *) "Special Reg", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The body lines are displayed after the
// banner line. We currently just display all control registers. A later version may 
// print the registers a bit more formatted with respect to their content.
//
//----------------------------------------------------------------------------------------
void SimWinSpecialRegs::drawBody( ) {
    
    uint32_t fmtDesc = FMT_DEF_ATTR;
    
    setWinCursor( 2, 1 );
    printTextField((char *) "SR0=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 0; i < 4; i++ ) {

        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 3, 1 );
    printTextField((char *) "SR4=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 4; i < 8; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 4, 1 );
    printTextField((char *) "SR8=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 8; i < 12; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
    setWinCursor( 5, 1 );
    printTextField((char *) "SR12=", ( fmtDesc | FMT_BOLD | FMT_ALIGN_LFT ), 6 );
    
    for ( int i = 12; i < 16; i++ ) {
        
        glb -> console -> printNumber( 0, fmtDesc );
        glb -> console -> printBlanks( 2 );
    }

    padLine( fmtDesc );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the physical memory window class.
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimWinAbsMem::SimWinAbsMem( SimGlobals *glb ) : SimWinScrollable( glb ) { }

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first time, or for the WDEF
// command. The memory window is a window where the number of lines to display can be set. However, the
// minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefColumns( 12 + ( 8 * 11 ), 16 );
    setDefColumns( 12 + ( 8 * 11 ), 10 );
    setColumns( getDefColumns( getRadix( )));
    
    setWinType( WT_PM_WIN );
    setEnable( false );
    setRows( 5 );
    setHomeItemAdr( 0 );
    setCurrentItemAdr( 0 );
    setLineIncrement( 8 * 4 );
    setLimitItemAdr( 0 );
}

//----------------------------------------------------------------------------------------
// The window overrides the setRadix method to set the column width according to the
// radix chosen.
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::setRadix( int rdx ) {
    
    SimWin::setRadix( rdx );
    setColumns( getDefColumns( getRadix( )));
}

//----------------------------------------------------------------------------------------
// The banner line shows the item address, which is the current absolute physical memory
// address where the window body will start to display. We also need to set the item 
// address limit. 
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::drawBanner( ) {
    
    uint32_t    fmtDesc     = FMT_BOLD | FMT_INVERSE;
    uint32_t    currentAdr  = getCurrentItemAdr( );
    bool        isCurrent   = glb -> winDisplay -> isCurrentWin( getWinIndex( ));

    #if 0
    CpuMem      *physMem    = glb -> cpu -> physMem;
    CpuMem      *pdcMem     = glb -> cpu -> pdcMem;
    CpuMem      *ioMem      = glb -> cpu -> ioMem;
    #endif

    setWinCursor( 1, 1 );
    printWindowIdField( getWinStack( ), getWinIndex( ), isCurrent, fmtDesc );
    
    #if 0
    if (( physMem != nullptr ) && ( physMem -> validAdr( currentAdr ))) {
        
        printTextField((char *) "Main Memory ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    }
    else if (( pdcMem != nullptr ) && ( pdcMem -> validAdr( currentAdr ))) {
        
        printTextField((char *) "PDC Memory ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    }
    else if (( ioMem != nullptr ) && ( ioMem -> validAdr( currentAdr ))) {
        
        printTextField((char *) "IO Memory ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    }
    else printTextField((char *) "**** Memory ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    #endif
    
    printTextField((char *) "Current " );
    printNumericField( getCurrentItemAdr( ));
    printTextField((char *) "  Home: " );
    printNumericField(  getHomeItemAdr( ));
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );

    setLimitItemAdr( UINT_MAX );  // ??? what is the value ?
}

//----------------------------------------------------------------------------------------
// A scrollable window needs to implement a routine for displaying a row. We are passed
// the item address and need to map this to the actual meaning of the particular window.
// The "itemAdr" value is the byte offset into physical memory, the line increment is
// 8 * 4 = 32 bytes. We show eight 32-bit words.
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::drawLine( T64Word itemAdr ) {

// Format:
//
// (0x00_0000_0000) 0x0000_0000 0x0000_0000 .... 8 times.

    uint32_t    fmtDesc     = FMT_DEF_ATTR;
    uint32_t    limit       = getLineIncrement( ) - 1;

    #if 0
    CpuMem      *physMem    = glb -> cpu -> physMem;
    CpuMem      *pdcMem     = glb -> cpu -> pdcMem;
    CpuMem      *ioMem      = glb -> cpu -> ioMem;
    #endif 

    printNumericField( itemAdr, fmtDesc | FMT_HEX_2_4_4 );
    printTextField((char *) ": ", fmtDesc );
    
    for ( int i = 0; i < limit; i = i + 4 ) {
        
        uint32_t ofs = itemAdr + i;
        
        #if 0
        if (( physMem != nullptr ) && ( physMem -> validAdr( ofs ))) {
            
            printNumericField( physMem -> getMemDataWord( ofs ), fmtDesc );
        }
        else if (( pdcMem != nullptr ) && ( pdcMem -> validAdr( ofs ))) {
            
            printNumericField( pdcMem -> getMemDataWord( ofs ), fmtDesc );
        }
        else if (( ioMem != nullptr ) && ( ioMem -> validAdr( ofs ))) {
            
            printNumericField( ioMem -> getMemDataWord( ofs ), fmtDesc );
        }
        else printNumericField( 0, fmtDesc | FMT_INVALID_NUM );
        #endif 

        printTextField((char *) " " );
    }
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the code memory window class.
//
//----------------------------------------------------------------------------------------
// Object constructor. We create a disassembler object for displaying the decoded 
// instructions.
//
// ??? check math for window scrolling, debug: 0xf000000 + forward ....
// ??? need to rework for virtual addresses ? We need to work with segment and offset !!!!
//----------------------------------------------------------------------------------------
SimWinCode::SimWinCode( SimGlobals *glb ) : SimWinScrollable( glb ) {

    disAsm = new T64DisAssemble( );
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command. The code memory window is a window where the number
// of lines to display can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinCode::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setColumns( 84 );
    setDefColumns( 84 );
    setRows( 9 );
    setHomeItemAdr( 0 );

    // setCurrentItemAdr( glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_1 ));
    setCurrentItemAdr( 0 );

    setLineIncrement( 4 );
    setLimitItemAdr( UINT_MAX );
    setWinType( WT_PC_WIN );
    setEnable( false );
}

//----------------------------------------------------------------------------------------
// The banner for the code window shows the code address. We automatically scroll the
// window for the single step command. We detect this by examining the current command
// and adjust the current item address to scroll to the next lines to show.
//
//----------------------------------------------------------------------------------------
void SimWinCode::drawBanner( ) {
    
    uint32_t    fmtDesc         = FMT_BOLD | FMT_INVERSE;
    T64Word     currentIa       = getCurrentItemAdr( );
    T64Word     currentIaLimit  = currentIa + (( getRows( ) - 1 ) * getLineIncrement( ));

    // T64Word     currentIaOfs    = (int) glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_1  );
    T64Word     currentIaOfs    = 0;
    
    SimTokId    currentCmd      = glb -> winDisplay -> getCurrentCmd( );
    bool        isCurrent       = glb -> winDisplay -> isCurrentWin( getWinIndex( ));
    bool        hasIaOfsAdr     = (( currentIaOfs >= currentIa ) && 
                                    ( currentIaOfs <= currentIaLimit ));
    
    if (( currentCmd == CMD_STEP ) && ( hasIaOfsAdr )) {
        
        if      ( currentIaOfs >= currentIaLimit ) winJump( currentIaOfs );
        else if ( currentIaOfs < currentIa )       winJump( currentIaOfs );
    }
    
    // ??? do we show seg and offset ?

    setWinCursor( 1, 1 );
    printWindowIdField( getWinStack( ), getWinIndex( ), isCurrent, fmtDesc );
    printTextField((char *) "Code Memory ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    printTextField((char *) "Current: " );
    printNumericField( getCurrentItemAdr( ), fmtDesc | FMT_HEX_4_4 );
    printTextField((char *) "  Home: " );
    printNumericField(  getHomeItemAdr( ));
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// A scrollable window needs to implement a routine for displaying a row. We are passed
// the item address and need to map this to the actual meaning of the particular window.
// The disassembled format is printed in two parts, the first is the instruction and
// options, the second is the target and operand field. We make sure that both parts
// are nicely aligned.
//
//----------------------------------------------------------------------------------------
void SimWinCode::drawLine( T64Word itemAdr ) {
    
    uint32_t    fmtDesc         = FMT_DEF_ATTR;
    bool        plWinEnabled    = glb -> winDisplay -> isWinEnabled( PL_REG_WIN );

    #if 0
    CpuMem      *physMem        = glb -> cpu -> physMem;
    CpuMem      *pdcMem         = glb -> cpu -> pdcMem;
    CpuMem      *ioMem          = glb -> cpu -> ioMem;
    #endif

    uint32_t    instr           = 0xFFFFFFFF;
    char        buf[ 128 ]      = { 0 };
    
    #if 0
    if (( physMem != nullptr ) && ( physMem -> validAdr( itemAdr ))) {
        
        instr = physMem -> getMemDataWord( itemAdr );
    }
    else if (( pdcMem != nullptr ) && ( pdcMem -> validAdr( itemAdr ))) {
        
        instr = pdcMem -> getMemDataWord( itemAdr );
    }
    else if (( ioMem != nullptr ) && ( ioMem -> validAdr( itemAdr ))) {
        
        instr = ioMem -> getMemDataWord( itemAdr );
    }
    #endif

    printNumericField( itemAdr, fmtDesc | FMT_ALIGN_LFT, 12 );
  
    #if 0
    if ( itemAdr == glb -> cpu -> getReg( RC_FD_PSTAGE, PSTAGE_REG_ID_PSW_1 )) {
        
        printTextField((char *) "    >", fmtDesc, 5 );
    }
    else printTextField((char *) "     ", fmtDesc, 5 );
   
    printNumericField( instr, fmtDesc | FMT_ALIGN_LFT, 12 );
    
    int pos          = getWinCursorCol( );
    int opCodeField  = disAsm -> getOpCodeOptionsFieldWidth( );
    int operandField = disAsm -> getOpCodeOptionsFieldWidth( );
    
    clearField( opCodeField );
    disAsm -> formatOpCodeAndOptions( buf, sizeof( buf ), instr );
    printText( buf, (int) strlen( buf ));
    setWinCursor( 0, pos + opCodeField );
    
    clearField( operandField );
    disAsm -> formatTargetAndOperands( buf, sizeof( buf ), instr );
    printText( buf, (int) strlen( buf ));
    setWinCursor( 0, pos + opCodeField + operandField );

    #endif

    padLine( );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the TLB class.
//
//----------------------------------------------------------------------------------------
// Object constructor. All we do is to remember what kind of TLB this is.
//
//----------------------------------------------------------------------------------------
SimWinTlb::SimWinTlb( SimGlobals *glb, int winType ) : SimWinScrollable( glb ) {
    
    this -> winType = winType;
}

//----------------------------------------------------------------------------------------
// We have a function to set reasonable default values for the window. The default 
// values are the initial settings when windows is brought up the first time, or for 
// the WDEF command. The TLB window is a window where the number of lines to display 
// can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinTlb::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefColumns( 84, 16 );
    setDefColumns( 102, 8 );
    setDefColumns( 84, 10 );
    setColumns( getDefColumns( getRadix( )));
    setWinType( winType );
    setEnable( false );
    setRows( 5 );
    setCurrentItemAdr( 0 );
    setLineIncrement( 1 );
    setLimitItemAdr( 0 );
    
    #if 0
    if      ( winType == WT_ITLB_WIN ) tlb = glb -> cpu -> iTlb;
    else if ( winType == WT_DTLB_WIN ) tlb = glb -> cpu -> dTlb;
    #endif

}

//----------------------------------------------------------------------------------------
// The window overrides the setRadix method to set the column width according to the
// radix chosen.
//
//----------------------------------------------------------------------------------------
void SimWinTlb::setRadix( int rdx ) {
    
    SimWin::setRadix( rdx );
    setColumns( getDefColumns( getRadix( )));
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. We also need to set the
// item address limit. As this can change with some commands outside the windows system,
// better set it every time.
//
//----------------------------------------------------------------------------------------
void SimWinTlb::drawBanner( ) {
    
    uint32_t    fmtDesc     = FMT_BOLD | FMT_INVERSE;
    bool        isCurrent   = glb -> winDisplay -> isCurrentWin( getWinIndex( ));
    
    setWinCursor( 1, 1 );
    printWindowIdField( getWinStack( ), getWinIndex( ), isCurrent, fmtDesc );
    
    if      ( winType == WT_ITLB_WIN ) printTextField((char *) "I-TLB ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    else if ( winType == WT_DTLB_WIN ) printTextField((char *) "D-TLB ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    else                               printTextField((char *) "***** ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    
    printTextField((char *) "Current: " );
    printNumericField( getCurrentItemAdr( ));
    printTextField((char *) "  Home: " );
    printNumericField(  getHomeItemAdr( ));
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
    
    // setLimitItemAdr( tlb -> getTlbSize( ));
    setLimitItemAdr( 1000 );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The body lines are displayed after the banner line. The
// number of lines can vary. A line represents an entry in the respective TLB.
//
//----------------------------------------------------------------------------------------
void SimWinTlb::drawLine( T64Word index ) {

    // Format:
//
// (0x0000) [xxxx][xx] ∏: 0x0_0000_0000_0000 pa: 0x00_0000_0000 Pid: 0x0000_0000

    
    uint32_t  fmtDesc = FMT_DEF_ATTR;
    
    printNumericField( index, fmtDesc );
    printTextField((char *) ":[", fmtDesc );
    
    #if 0
    if ( index > tlb -> getTlbSize( )) {
  
        printTextField((char *) "Invalid TLB index", fmtDesc );
        printTextField((char *) "]", fmtDesc );
        padLine( );
    }
    else {
        
        TlbEntry   *tEntry  = tlb -> getTlbEntry( index );
        char            tmpBuf[ 32 ];
        
        printTextField((( tEntry -> tValid( )) ? (char *) "V" : (char *) "v" ), fmtDesc );
        printTextField((( tEntry -> tDirty( )) ? (char *) "D" : (char *) "d" ), fmtDesc );
        printTextField((( tEntry -> tTrapPage( )) ? (char *) "P" : (char *) "p" ), fmtDesc );
        printTextField((( tEntry -> tTrapDataPage( )) ? (char *) "D" : (char *) "d" ), fmtDesc );
        printTextField((char *) "]", fmtDesc );
        
        buildAccessRightsStr( tmpBuf, 32, tEntry ->tPageType( ), tEntry -> tPrivL1( ), tEntry -> tPrivL2( ));
        printTextField((char *) " ACC:", fmtDesc );
        printTextField( tmpBuf, fmtDesc );
        printTextField((char *) " PID:", fmtDesc );
        printNumericField( tEntry -> tSegId( ), fmtDesc| FMT_HALF_WORD );
        printTextField((char *) " VPN:", fmtDesc );
        printNumericField( tEntry -> vpnHigh, fmtDesc );
        printTextField((char *) ".", fmtDesc );
        printNumericField( tEntry -> vpnLow, fmtDesc );
        printTextField((char *) " PPN:", fmtDesc );
        printNumericField( tEntry -> tPhysPage( ), fmtDesc );
    }
    #endif
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the Cache class.
//
//----------------------------------------------------------------------------------------
// Object constructor. All we do is to remember what kind of Cache this is.
//
//----------------------------------------------------------------------------------------
SimWinCache::SimWinCache( SimGlobals *glb, int winType ) : SimWinScrollable( glb ) {
    
    this -> winType = winType;
}

//----------------------------------------------------------------------------------------
// We have a function to set reasonable default values for the window. The default 
// values are the initial settings when windows is brought up the first time, or for
// the WDEF command. The TLB window is a window  where the number of lines to display 
// can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinCache::setDefaults( ) {
    
    #if 0
    if      ( winType == WT_ICACHE_WIN )    cPtr = glb -> cpu -> iCacheL1;
    else if ( winType == WT_DCACHE_WIN )    cPtr = glb -> cpu -> dCacheL1;
    else if ( winType == WT_UCACHE_WIN )    cPtr = glb -> cpu -> uCacheL2;
    #endif

    // uint32_t wordsPerBlock = cPtr -> getBlockSize( ) / 4;
    uint32_t wordsPerBlock = 4;
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefColumns( 36 + ( wordsPerBlock * 11 ), 16 );
    setDefColumns( 36 + ( wordsPerBlock * 13 ), 8 );
    setDefColumns( 36 + ( wordsPerBlock * 11 ), 10 );
    setColumns( getDefColumns( getRadix( )));
    setRows( 6 );
    setWinType( winType );
    setEnable( false );
    setCurrentItemAdr( 0 );
    setLineIncrement( 1 );
    setLimitItemAdr( 0 );
    winToggleVal = 0;
}

//----------------------------------------------------------------------------------------
// The window overrides the setRadix method to set the column width according to the radix chosen.
//
//----------------------------------------------------------------------------------------
void SimWinCache::setRadix( int rdx ) {
    
    SimWin::setRadix( rdx );
    setColumns( getDefColumns( getRadix( )));
}

//----------------------------------------------------------------------------------------
// We allow for toggling through the sets if the cache is an n-way associative cache.
//
//----------------------------------------------------------------------------------------
void SimWinCache::toggleWin( ) {
    
    // uint32_t blockSize   = cPtr -> getBlockSets( );
    int blockSize = 4;

    winToggleVal = ( winToggleVal + 1 ) % blockSize;
}
    
//----------------------------------------------------------------------------------------
// Each window consist of a headline and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. We also need to set the 
// item address limit. As this can change with some commands outside the windows system,
// better set it every time.
//
//----------------------------------------------------------------------------------------
void SimWinCache::drawBanner( ) {
    
    uint32_t    fmtDesc     = FMT_BOLD | FMT_INVERSE;
    bool        isCurrent   = glb -> winDisplay -> isCurrentWin( getWinIndex( ));
    
    setWinCursor( 1, 1 );
    printWindowIdField( getWinStack( ), getWinIndex( ), isCurrent, fmtDesc );

    printTextField((char *) "Set: " );

    printTextField((char *) " Current: " );

    printTextField((char *) "  Home: " );

    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
    
    #if 0
    if      ( winType == WT_ICACHE_WIN ) printTextField((char *) "I-Cache (L1) ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    else if ( winType == WT_DCACHE_WIN ) printTextField((char *) "D-Cache (L1)", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    else if ( winType == WT_UCACHE_WIN ) printTextField((char *) "U-Cache (L2)", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
    else                                 printTextField((char *) "******* ", ( fmtDesc | FMT_ALIGN_LFT ), 16 );
  
    setLimitItemAdr( cPtr -> getBlockEntries( ));
    
    printNumericField( winToggleVal, ( fmtDesc | FMT_HALF_WORD ));
    
    printNumericField( getCurrentItemAdr( ));
    
    printNumericField(  getHomeItemAdr( ));
  
    #endif 
}

//----------------------------------------------------------------------------------------
// The draw line methods for the cache lists a cache entry. There are various cache
// line sizes. And there are up to two sets of cache data.
//
//----------------------------------------------------------------------------------------
void SimWinCache::drawLine( T64Word index ) {
    
    uint32_t  fmtDesc   = FMT_DEF_ATTR;
 
    #if 0
    if ( index > cPtr -> getBlockEntries( )) {
  
        printNumericField( index, fmtDesc );
        printTextField((char *) ":[", fmtDesc );
        printTextField((char *) "Invalid Cache index", fmtDesc );
        printTextField((char *) "]", fmtDesc );
        padLine( );
    }
    else {
        
        MemTagEntry *tagPtr         = cPtr -> getMemTagEntry( index, winToggleVal );
        uint32_t    *dataPtr        = (uint32_t *) cPtr -> getMemBlockEntry( index, winToggleVal );
        uint32_t    wordsPerBlock   = cPtr -> getBlockSize( ) / 4;
      
        printNumericField( index, fmtDesc );
        printTextField((char *) ":[", fmtDesc );
        printTextField((( tagPtr -> valid ) ? (char *) "V" : (char *) "v" ), fmtDesc );
        printTextField((( tagPtr -> dirty ) ? (char *) "D" : (char *) "d" ), fmtDesc );
        printTextField((char *) "] (", fmtDesc );
        printNumericField( tagPtr -> tag, fmtDesc );
        printTextField((char *) ") ", fmtDesc );
        
        for ( uint32_t i = 0; i < wordsPerBlock; i++ ) {
          
            printNumericField( dataPtr[ i ], fmtDesc );
            printTextField((char *) " " );
        }
    }
    #endif
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the text window class.
//
//----------------------------------------------------------------------------------------
// Object constructor. We are passed the globals and the file path. All we do right now
// is to remember the file name. The text window has a destructor method as well. We 
// need to close a potentially opened file.
//
//----------------------------------------------------------------------------------------
SimWinText::SimWinText( SimGlobals *glb, char *fName ) : SimWinScrollable( glb ) {
    
    if ( fName != nullptr ) strcpy( fileName, fName );
    else throw ( ERR_EXPECTED_FILE_NAME );
}

SimWinText:: ~SimWinText( ) {
    
    if ( textFile != nullptr ) fclose( textFile );
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimWinText::setDefaults( ) {
    
    setWinType( WT_TEXT_WIN );
    setEnable( true );
    setRows( 11 );
    
    int txWidth = glb -> env -> getEnvVarInt((char *) ENV_WIN_TEXT_LINE_WIDTH );
    setRadix( txWidth );
    setDefColumns( txWidth );
    
    setRadix( 10 );
    setCurrentItemAdr( 0 );
    setLineIncrement( 1 );
    setLimitItemAdr( 1 );
}

//----------------------------------------------------------------------------------------
// The banner line for the text window. It contains the open file name and the current
// line and home line number. The file path may be a bit long for listing it completely,
// so we will truncate it on the left side. The routine will print the the filename, 
// and the position into the file. The banner method also sets the preliminary line 
// size of the window. This value is used until we know the actual number of lines in
// the file. Lines shown on the display start with one, internally we start at zero.
//
//----------------------------------------------------------------------------------------
void SimWinText::drawBanner( ) {
    
    uint32_t    fmtDesc     = FMT_BOLD | FMT_INVERSE;
    bool        isCurrent   = glb -> winDisplay -> isCurrentWin( getWinIndex( ));
    
    setWinCursor( 1, 1 );
    printWindowIdField( getWinStack( ), getWinIndex( ), isCurrent, fmtDesc );
    printTextField((char *) "Text: ", ( fmtDesc | FMT_ALIGN_LFT ));
    printTextField((char *) fileName, ( fmtDesc | FMT_ALIGN_LFT | FMT_TRUNC_LFT ), 48 );
    printTextField((char *) "  Line: " );
   
    // printNumericField( getCurrentItemAdr( ) + 1, ( fmtDesc | FMT_HALF_WORD ));
    
    printTextField((char *) "  Home: " );
    
    // printNumericField(  getHomeItemAdr( ) + 1, ( fmtDesc | FMT_HALF_WORD ));
    
    padLine( fmtDesc );
}

//----------------------------------------------------------------------------------------
// The draw line method for the text file window. We print the file content line by
// line. A line consists of the line number followed by the text. This routine will
// first check whether the file is already open. If we cannot open the file, we would
// now print an error message into the screen. This is also the time  where we actually
// figure out how many lines are on the file so that we can set the limitItemAdr field
// of the window object.
//
//----------------------------------------------------------------------------------------
void SimWinText::drawLine( T64Word index ) {
    
    uint32_t    fmtDesc = FMT_DEF_ATTR;
    char        lineBuf[ MAX_TEXT_LINE_SIZE ];
    int         lineSize = 0;
  
    if ( openTextFile( )) {
  
        lineSize = readTextFileLine( index + 1, lineBuf, sizeof( lineBuf ));
        if ( lineSize > 0 ) {
            
            //  printNumericField( index + 1, ( fmtDesc | FMT_HALF_WORD ));
            
            printTextField((char *) ": " );
            printTextField( lineBuf, fmtDesc, lineSize );
            padLine( );
        }
        else padLine( );
    }
    else printTextField((char *) "Error opening the text file", fmtDesc );
}

//----------------------------------------------------------------------------------------
// "openTextFile" is called every time we want to print a line. If the file is not
// opened yet, it will be now and while we are at it, we will also count the source
// lines for setting the limit in the scrollable window.
//
//----------------------------------------------------------------------------------------
bool SimWinText::openTextFile( ) {
    
    if ( textFile == nullptr ) {
        
        textFile = fopen( fileName, "r" );
        if ( textFile != nullptr ) {
           
            while( ! feof( textFile )) {
                
                if( fgetc( textFile ) == '\n' ) fileSizeLines++;
            }
            
            lastLinePos = 0;
            rewind( textFile );
            setLimitItemAdr( fileSizeLines );
        }
    }
    
    return( textFile != nullptr );
}

//----------------------------------------------------------------------------------------
// "readTextFileLine" will get a line from the text file. Unfortunately, we do not 
// have a line concept in a text file. In the worst case, we read from the beginning
// of the file, counting the lines read. To speed up a little, we remember the last
// line position read. If the requested line position is larger than the last position,
// we just read ahead. If it is smaller, no luck, we start to read from zero until we
// match. If equal, we just re-read the current line.
//
//----------------------------------------------------------------------------------------
int SimWinText::readTextFileLine( int linePos, char *lineBuf, int bufLen  ) {
 
    if ( textFile != nullptr ) {
        
        if ( linePos > lastLinePos ) {
            
            while ( lastLinePos < linePos ) {
                
                lastLinePos ++;
                fgets( lineBuf, bufLen, textFile );
            }
        }
        else if ( linePos < lastLinePos ) {
            
            lastLinePos = 0;
            rewind( textFile );
            
            while ( lastLinePos < linePos ) {
                
                lastLinePos ++;
                fgets( lineBuf, bufLen, textFile );
            }
        }
        else fgets( lineBuf, bufLen, textFile );
            
        lineBuf[ strcspn( lineBuf, "\r\n") ] = 0;
        return((int) strlen ( lineBuf ));
    }
    else return ( 0 );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the console window class.
//
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimWinConsole::SimWinConsole( SimGlobals *glb ) : SimWin( glb ) {
    
    this -> glb = glb;
    winOut      = new SimWinOutBuffer( );
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimWinConsole::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setRows( 11 );
    setColumns( 80 );
    setDefColumns( 80 );
    setWinType( WT_CONSOLE_WIN );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// The console offers two basic routines to read a character and to write a character.
// The write function will just add the character to the output buffer.
//
//----------------------------------------------------------------------------------------
void SimWinConsole::putChar( char ch ) {
    
    winOut -> writeChar( ch );
}


// ??? what about the read part. Do we just get a character from the terminal input 
// and add it to the output side ? Or is this a function of the console driver code
// written for the emulator ?

// ??? should we add the switch to and from the console in this class ?


//----------------------------------------------------------------------------------------
// The banner line for console window.
//
//----------------------------------------------------------------------------------------
void SimWinConsole::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
    
    setWinCursor( 1, 1 );
    printTextField((char *) "Console ", fmtDesc );
    padLine( fmtDesc );
}

//----------------------------------------------------------------------------------------
// The body lines of the console window are displayed after the banner line. Each line
// is "sanitized" before we print it out. This way, dangerous escape sequences are 
// simply filtered out.
//
//----------------------------------------------------------------------------------------
void SimWinConsole::drawBody( ) {
    
    char lineOutBuf[ MAX_WIN_OUT_LINE_SIZE ];
    
    glb -> console -> setFmtAttributes( FMT_DEF_ATTR );
    
    int rowsToShow = getRows( ) - 2;
    winOut -> setScrollWindowSize( rowsToShow );
    setWinCursor( rowsToShow + 1, 1 );
    
    for ( int i = 0; i < rowsToShow; i++ ) {
        
        char *lineBufPtr = winOut -> getLineRelative( i );
        if ( lineBufPtr != nullptr ) {
            
            sanitizeLine( lineBufPtr, lineOutBuf );
            glb -> console -> clearLine( );
            glb -> console -> writeChars( "%s", lineBufPtr );
        }
        
        setWinCursor( rowsToShow - i, 1 );
    }
    
    setWinCursor( getRows( ), 1 );
}
