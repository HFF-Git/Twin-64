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
// This program is free software: you can redistribute it and/or modify it under the 
// terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should
//  have received a copy of the GNU General Public License along with this program.  
// If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#include "T64-Common.h"
#include "T64-Util.h"
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//----------------------------------------------------------------------------------------
namespace {

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
                        
                    } 
                    else continue;
                    
                } 
                else break;
                
            } 
            else *dst++ = *src++;
            
        } 
        else *dst++ = *src++;
    }
    
    *dst = '\0';
}

}; // namespace


//****************************************************************************************
//****************************************************************************************
//
// Methods for the Program State Window class. This is the main window for a processor.
//
//----------------------------------------------------------------------------------------
// Object creator.
//
//----------------------------------------------------------------------------------------
SimWinProgState::SimWinProgState( SimGlobals *glb, int procModuleNum ) : SimWin( glb ) { 

    this -> procModuleNum = procModuleNum;
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimWinProgState::setDefaults( ) {
    
    setWinType( WT_CPU_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefRows( 5 );
    setDefColumns( 110 );
    setRows( getDefRows( ));
    setWinToggleLimit( 4 );
    setWinToggleVal( 0 );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// Each window consist of a headline and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. The program state banner
// lists the instruction address and the status word.
//
// Format:
//
//  <winId> Proc: n IA: 0x00_0000_0000 ST: [xxxxxxx] <rdx>
//
//----------------------------------------------------------------------------------------
void SimWinProgState::drawBanner( ) {
    
    uint32_t fmtDesc    = FMT_BOLD | FMT_INVERSE;
   
    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );

    printTextField((char *) " Proc: ", fmtDesc );
    printNumericField( procModuleNum, fmtDesc | FMT_DEC );

    printTextField((char *) " IA: ", fmtDesc );
    printNumericField( 0, fmtDesc | FMT_HEX_2_4_4_4 );

    printTextField((char *) " ST: [", fmtDesc );
    printBitField( 0, 63, 'A', fmtDesc );
    printBitField( 0, 62, 'B', fmtDesc );
    printBitField( 0, 61, 'C', fmtDesc );
    printBitField( 0, 60, 'D', fmtDesc );
    printBitField( 0, 59, 'E', fmtDesc );
    printBitField( 0, 58, 'F', fmtDesc );
    printTextField((char *) "]", fmtDesc );

    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The body lines are displayed after the
// banner line. The program state window body lists the general registers.
//
// Format Ideas:
//
//  R0:  0x0000_0000_0000_0000 0x... 0x... 0x...
//  R4:  0x0000_0000_0000_0000 0x... 0x... 0x...
//  R8:  0x0000_0000_0000_0000 0x... 0x... 0x...
//  R12: 0x0000_0000_0000_0000 0x... 0x... 0x...
//
//  PID: 0x0000_0000_0000_0000 0x... 0x... 0x...
//
// The window supports the toggle concept. We should as default only the GRs.
// Toggle to CRs. Perhaps toggle to more formatted screens...
// 
//----------------------------------------------------------------------------------------
void SimWinProgState::drawBody( ) {
    
    uint32_t fmtDesc = FMT_DEF_ATTR | FMT_ALIGN_LFT;
    
    if ( getWinToggleVal( ) == 0 ) {

        int      numFlen        = glb -> console -> numberFmtLen( FMT_HEX_4_4_4_4 ) + 3;
        int      labelFlen      = 8;
        uint32_t numFmtField    = fmtDesc | FMT_HEX_4_4_4_4;
        uint32_t labelFmtField  = fmtDesc | FMT_BOLD;
        
        setWinCursor( 2, 1 );
        printTextField((char *) "GR0=", labelFmtField, labelFlen );
    
        for ( int i = 0; i < 4; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 3, 1 );
        printTextField((char *) "GR4=", labelFmtField, labelFlen );
    
        for ( int i = 4; i < 8; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 4, 1 );
        printTextField((char *) "GR8=", labelFmtField, labelFlen );
    
        for ( int i = 8; i < 12; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 5, 1 );
        printTextField((char *) "GR12=", labelFmtField, labelFlen );
    
        for ( int i = 12; i < 16; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
    } 
    else if ( getWinToggleVal( ) == 1 ) {

        int      numFlen        = glb -> console -> numberFmtLen( FMT_HEX_4_4_4_4 ) + 3;
        int      labelFlen      = 8;
        uint32_t numFmtField    = fmtDesc | FMT_HEX_4_4_4_4;
        uint32_t labelFmtField  = fmtDesc | FMT_BOLD;
        
        setWinCursor( 2, 1 );
        printTextField((char *) "CR0=", labelFmtField, labelFlen );
    
        for ( int i = 0; i < 4; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 3, 1 );
        printTextField((char *) "CR4=", labelFmtField, labelFlen );
    
        for ( int i = 4; i < 8; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 4, 1 );
        printTextField((char *) "CR8=", labelFmtField, labelFlen );
    
        for ( int i = 8; i < 12; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );
        setWinCursor( 5, 1 );
        printTextField((char *) "CR12=", labelFmtField, labelFlen );
    
        for ( int i = 12; i < 16; i++ ) {

            printNumericField( 0, numFmtField, numFlen );
        }

        padLine( fmtDesc );

        // ??? the control regs ?

        // ??? other toggle windows to come ...

    }
    else if ( getWinToggleVal( ) == 2 ) {

        setWinCursor( 2, 1 );
        printTextField((char *) "Toggle val 2", fmtDesc );
    }
     else if ( getWinToggleVal( ) == 3 ) {

        setWinCursor( 2, 1 );
        printTextField((char *) "Toggle val 3", fmtDesc );
    }
}

int  SimWinProgState::getProcModuleNum( ) {

    return( procModuleNum );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the physical memory window class.
//
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimWinAbsMem::SimWinAbsMem( SimGlobals *glb, T64Word adr ) : SimWinScrollable( glb ) {

    this -> adr = adr;
 }

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first time,
// or for the WDEF command. The memory window is a window where the number of lines to 
// display can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::setDefaults( ) {
    
    setWinType( WT_MEM_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefRows( 5 );
    setDefColumns( 116 );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));
    setEnable( false );
    setWinToggleLimit( 3 );
    setWinToggleVal( 0 );
    setHomeItemAdr( adr );
    setCurrentItemAdr( adr );
    setLineIncrement( 8 * 4 );
    setLimitItemAdr( UINT_MAX );
}

//----------------------------------------------------------------------------------------
// The banner line shows the item address, which is the current absolute physical memory
// address where the window body will start to display. We also need to set the item 
// address limit. 
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
    
    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );
    printTextField((char *) "Current: " );
    printNumericField( getCurrentItemAdr( ), fmtDesc | FMT_HEX_2_4_4 );
    printTextField((char *) "  Home: " );
    printNumericField( getHomeItemAdr( ), fmtDesc | FMT_HEX_2_4_4 );
    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );

    setLimitItemAdr( UINT_MAX );  // ??? what is the actual value ?
}

//----------------------------------------------------------------------------------------
// A scrollable window needs to implement a routine for displaying a row. We are passed
// the item address and need to map this to the actual meaning of the particular window.
// The "itemAdr" value is the byte offset into physical memory, the line increment is
// 8 * 4 = 32 bytes. We show eight 32-bit words or 4 64-bit words. The toggle value
// will decide on the format:
//
// Toggle 0: (0x00_0000_0000) 0x0000_0000             ... 8 times HEX.
// Toggle 1: (0x00_0000_0000) 0x0000_0000_0000_0000   ... 4 times HEX.
// Toggle 2: (0x00_0000_0000)           0             ... 8 times DEC.
// Toggle 3: ??? 
//
//----------------------------------------------------------------------------------------
void SimWinAbsMem::drawLine( T64Word itemAdr ) {

    uint32_t    fmtDesc     = FMT_DEF_ATTR;
    uint32_t    limit       = getLineIncrement( ) - 1;

    printTextField((char *) "(", fmtDesc );
    printNumericField( itemAdr, fmtDesc | FMT_HEX_2_4_4 );
    printTextField((char *) "): ", fmtDesc );

    if ( getWinToggleVal( ) == 0 ) {

        for ( int i = 0; i < limit; i = i + 4 ) {
        
            T64Word ofs = itemAdr + i;
            printNumericField( 0, fmtDesc | FMT_HEX_4_4 );
            printTextField((char *) " " );
        }
    }
    else if ( getWinToggleVal( ) == 1 ) {

        for ( int i = 0; i < limit; i = i + 8 ) {
        
            T64Word ofs = itemAdr + i;
            printNumericField( 0, fmtDesc | FMT_HEX_4_4_4_4 );
            printTextField((char *) "   " );
        }
    }
    else if ( getWinToggleVal( ) == 2 ) {

        for ( int i = 0; i < limit; i = i + 4 ) {
        
            T64Word ofs = itemAdr + i;
            printNumericField( 0, fmtDesc | FMT_DEC_32 );
            printTextField((char *) " " );
        }
    }
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the code memory window class.
//
//----------------------------------------------------------------------------------------
// Object constructor. We create a disassembler object for displaying the decoded 
// instructions and also need to remove it when the window is killed. 
//
//----------------------------------------------------------------------------------------
SimWinCode::SimWinCode( SimGlobals *glb ) : SimWinScrollable( glb ) {

    disAsm = new T64DisAssemble( );
}

SimWinCode::  ~  SimWinCode( ) {

    if ( disAsm != nullptr ) delete ( disAsm );
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first 
// time, or for the WDEF command. The code memory window is a window where the number
// of lines to display can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinCode::setDefaults( ) {
     
    setWinType( WT_CODE_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));

    setDefRows( 9 );
    setDefColumns( 116 );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));

    setHomeItemAdr( 0 );
    setCurrentItemAdr( 0 );
    setLineIncrement( 4 );
    setLimitItemAdr( UINT_MAX );
    setWinToggleLimit( 0 );
    setWinToggleVal( 0 );
    setEnable( true );
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
    T64Word     currentIaOfs    = 0;
    
    SimTokId    currentCmd      = glb -> winDisplay -> getCurrentCmd( );
    bool        hasIaOfsAdr     = (( currentIaOfs >= currentIa ) && 
                                    ( currentIaOfs <= currentIaLimit ));
    
    if (( currentCmd == CMD_STEP ) && ( hasIaOfsAdr )) {
        
        if      ( currentIaOfs >= currentIaLimit ) winJump( currentIaOfs );
        else if ( currentIaOfs < currentIa )       winJump( currentIaOfs );
    }
    
    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );
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
    bool        plWinEnabled    = glb -> winDisplay -> isWinEnabled( 0 );

    uint32_t    instr                       = 0xFFFFFFFF;
    char        buf[ MAX_TEXT_LINE_SIZE ]   = { 0 };
    
    printNumericField( itemAdr, fmtDesc | FMT_ALIGN_LFT, 12 );
  
    if ( itemAdr ==  0 ) {
        
        printTextField((char *) "    >", fmtDesc, 5 );
    }
    else printTextField((char *) "     ", fmtDesc, 5 );
   
    printNumericField( instr, fmtDesc | FMT_ALIGN_LFT | FMT_HEX_8, 12 );
    
    int pos          = getWinCursorCol( );
    int opCodeField  = disAsm -> getOpCodeFieldWidth( );
    int operandField = disAsm -> getOperandsFieldWidth( );
    
    clearField( opCodeField );
    disAsm -> formatOpCode( buf, sizeof( buf ), instr );
    printTextField( buf, fmtDesc, (int) strlen( buf ));
    setWinCursor( 0, pos + opCodeField );
    
    clearField( operandField );
    disAsm -> formatOperands( buf, sizeof( buf ), instr, 16 );
    printTextField( buf, fmtDesc, (int) strlen( buf ));
    setWinCursor( 0, pos + opCodeField + operandField );

    padLine( fmtDesc );
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
SimWinTlb::SimWinTlb( SimGlobals *glb ) : SimWinScrollable( glb ) { 

    setWinType( WT_TLB_WIN );
    setDefaults( );
}

//----------------------------------------------------------------------------------------
// We have a function to set reasonable default values for the window. The default 
// values are the initial settings when windows is brought up the first time, or for 
// the WDEF command. The TLB window is a window where the number of lines to display 
// can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinTlb::setDefaults( ) {
    
    setWinType( WT_TLB_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));

    setDefRows( 5 );
    setDefColumns( 116 );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));
    setCurrentItemAdr( 0 );
    setLineIncrement( 1 );
    setLimitItemAdr( UINT32_MAX );
    setWinToggleLimit( 0 );
    setWinToggleVal( 0 );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. We also need to set the
// item address limit. As this can change with some commands outside the windows system,
// better set it every time.
//
// Format:
//
//  <windId> Proc: n Tlb: n Set: n Current: 0x0000.  <rdx>
// 
// ??? set item limit adr...
//----------------------------------------------------------------------------------------
void SimWinTlb::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
   
    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );

    printTextField((char *) "Proc: " );
    printNumericField( 0, ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Tlb: " );
    printNumericField( 0, ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Set: " );
    printNumericField( getWinToggleVal( ), ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Current: " );
    printNumericField( getCurrentItemAdr( ), ( fmtDesc | FMT_HEX_4 ));

    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// Each window consist of a banner and a body. The body lines are displayed after the 
// banner line. The number of lines can vary. A line represents an entry in the respective
// TLB.
//
// Format:
//
//  (0x0000) [xxxx][xx] : va: 0x00_0000_0000_0000 pa: 0x00_0000_0000 Pid: 0x0000_0000
//----------------------------------------------------------------------------------------
void SimWinTlb::drawLine( T64Word index ) {

    uint32_t  fmtDesc = FMT_DEF_ATTR;

    // ??? get the entry .... 

    printTextField((char *) "(", fmtDesc );
    printNumericField( index, fmtDesc | FMT_HEX_4 );
    printTextField((char *) "): [", fmtDesc );
    printBitField( 0, 63, 'A', fmtDesc );
    printBitField( 0, 62, 'B', fmtDesc );
    printBitField( 0, 61, 'C', fmtDesc );
    printBitField( 0, 60, 'D', fmtDesc );
    printBitField( 0, 59, 'E', fmtDesc );
    printTextField((char *) "] [", fmtDesc );
    printTextField((char *) "xx", fmtDesc ); // ??? page size.... decoded ?
    printTextField((char *) "]", fmtDesc );
    printTextField((char *) "  vAdr: ", fmtDesc );
    printNumericField( 0, fmtDesc | FMT_HEX_2_4_4_4 );
    printTextField((char *) "  pAdr: ", fmtDesc );
    printNumericField( 0, fmtDesc | FMT_HEX_2_4_4 );
}

//****************************************************************************************
//****************************************************************************************
//
// Methods for the Cache class.
//
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimWinCache::SimWinCache( SimGlobals *glb ) : SimWinScrollable( glb ) { 

    // ??? get the cache....

    setWinType( WT_CACHE_WIN );
    setDefaults( );
}

//----------------------------------------------------------------------------------------
// We have a function to set reasonable default values for the window. The default 
// values are the initial settings when windows is brought up the first time, or for
// the WDEF command. The TLB window is a window  where the number of lines to display 
// can be set. However, the minimum is the default number of lines.
//
//----------------------------------------------------------------------------------------
void SimWinCache::setDefaults( ) {

    setWinType( WT_CACHE_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));

    // ??? how do we map a cache line size of 8 and 16 32-bit words ?
   
    if ( cache -> getCacheLineSize( ) == 32 ) {

        setDefRows( 6 );
        setDefColumns( 128 );
    }
    else if ( cache -> getCacheLineSize( ) == 64 ) { 

    } 
    else {

        // ??? would a two line approach work ?
        // ??? what if someone sets rows to an odd number ?

        setDefRows( 6 );
        setDefColumns( 128 * 2 );
    }

    setRows( getDefRows( ));
    setColumns( getDefColumns( ));

    setCurrentItemAdr( 0 );
    setLineIncrement( 1 );
    setLimitItemAdr( cache -> getSetSize( ));
    setWinToggleLimit( cache -> getWays( ));
    setWinToggleVal( 0 );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// Each window consist of a headline and a body. The banner line is always shown in 
// inverse and contains summary or head data for the window. We also need to set the 
// item address limit. As this can change with some commands outside the windows system,
// better set it every time.
//
// Format:
//
//  <windId> Proc: n Cache: n Set: n Current: 0x0000.  <rdx>
//
//----------------------------------------------------------------------------------------
void SimWinCache::drawBanner( ) {
    
    uint32_t    fmtDesc     = FMT_BOLD | FMT_INVERSE;

    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );

    printTextField((char *) "Proc: " );
    printNumericField( procMuduleNum, ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Cache: " );
    printNumericField( cacheNum, ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Set: " );
    printNumericField( getWinToggleVal( ), ( fmtDesc | FMT_DEC ));

    printTextField((char *) "  Current: " );
    printNumericField( getCurrentItemAdr( ), fmtDesc | FMT_HEX_4 );

    padLine( fmtDesc );
    printRadixField( fmtDesc | FMT_LAST_FIELD );
}

//----------------------------------------------------------------------------------------
// The draw line methods for the cache lists a cache entry. There are various cache
// line sizes. And there are up to two sets of cache data.
//
// Format:
//
//  (0x0000): [xxx] [0x00_00000_0000] 0x0000_0000_0000_0000 0x... 0x... 0x... 
//----------------------------------------------------------------------------------------
void SimWinCache::drawLine( T64Word index ) {

    uint32_t fmtDesc = FMT_DEF_ATTR;

    T64CacheLineInfo *cInfo;
    uint8_t          *cData; // ?? this is byte...

    if ( ! cache -> getCacheLine( getWinToggleVal( ),
                                  index,
                                  &cInfo, 
                                  &cData )) {


    }

    printTextField((char *) "(", fmtDesc );
    printNumericField( index, fmtDesc | FMT_HEX_4 );
    printTextField((char *) "): ", fmtDesc );

    if ( index > getLimitItemAdr( )) {
  
        printTextField((char *) "[", fmtDesc );
        printTextField((char *) "Invalid Cache index", fmtDesc );
        printTextField((char *) "]", fmtDesc );
        padLine( );
    }
    else {

        printTextField((char *) "[", fmtDesc );

        if ( cInfo -> valid ) printTextField((char *) "V", fmtDesc );
        else printTextField((char *) "v", fmtDesc );

         if ( cInfo -> modified ) printTextField((char *) "M", fmtDesc );
        else printTextField((char *) "m", fmtDesc );

        printTextField((char *) "] [", fmtDesc );
        printNumericField( cInfo -> tag, fmtDesc | FMT_HEX_2_4 );
        printTextField((char *) "] ", fmtDesc );

        for ( int i = 0; i < 4; i++ ) { // ??? fix ... cache line size
            
            printNumericField( 0, fmtDesc | FMT_HEX_4_4_4_4 );
            printTextField((char *) "  " );
        }
    }
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

    int txWidth = glb -> env -> getEnvVarInt((char *) ENV_WIN_TEXT_LINE_WIDTH );
    
    setWinType( WT_TEXT_WIN );
    setEnable( true );

    setDefRows( 11 );
    setDefColumns( txWidth );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));
    
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
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
    
    setWinCursor( 1, 1 );
    printWindowIdField( fmtDesc );

    printTextField((char *) "Text: ", ( fmtDesc | FMT_ALIGN_LFT ));
    printTextField((char *) fileName, ( fmtDesc | FMT_ALIGN_LFT | FMT_TRUNC_LFT ), 48 );
    printTextField((char *) "  Line: " );
    printNumericField( getCurrentItemAdr( ) + 1, ( fmtDesc | FMT_DEC ));
    printTextField((char *) "  Home: " );
    printNumericField(  getHomeItemAdr( ) + 1, ( fmtDesc | FMT_DEC ));
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
            
            printNumericField( index + 1, ( fmtDesc | FMT_DEC ));
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
// opened yet, it will be opened now and while we are at it, we will also count the
// source lines for setting the limit in the scrollable window.
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
    
    setDefRows( 10 );
    setDefColumns( 110 );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));
    
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
