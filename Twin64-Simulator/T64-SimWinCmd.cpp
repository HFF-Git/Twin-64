//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator command window
//
//----------------------------------------------------------------------------------------
// The command window is the last screen area below all enabled windows displayed. It
// is actually not a window like the others in that it represents lines written to the
// window as well as the command input line. It still as a window header and a line
// drawing area. To enable scrolling of this window, an output buffer needs to be 
// implemented that stores all output in a circular buffer to use for text output. 
// Just like a "real" terminal. The cursor up and down keys will perform the scrolling.
// The command line is also a bit special. It is actually the one line locked scroll
// area. Input can be edited on this line, a carriage return will append the line to
// the output buffer area.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU -Simulator command window
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
#include "T64-SimTables.h"

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// Little helper functions.
//
//----------------------------------------------------------------------------------------
int setRadix( int rdx ) {
    
    return((( rdx == 10 ) || ( rdx == 16 )) ? rdx : 10 );
}

bool isEscapeChar( int ch ) {
    
    return ( ch == 27 );
}

bool isWInSpecialChar( int ch ) {
    
    return ( ch == 0xe0 );
}

bool isCarriageReturnChar( int ch ) {
    
    return (( ch == '\n' ) || ( ch == '\r' ));
}

bool isBackSpaceChar( int ch ) {
    
    return (( ch == 8 ) || ( ch == 127 ));
}

bool isLeftBracketChar( int ch ) {
    
    return ( ch == '[' );
}

//----------------------------------------------------------------------------------------
// A little helper function to remove the comment part of a command line. We do the
// changes on the buffer passed in by just setting the end of string at the position
// of the "#" comment indicator. A "#" inside a string is ignored.
//
//----------------------------------------------------------------------------------------
int removeComment( char *cmdBuf ) {
    
    if ( strlen ( cmdBuf ) > 0 ) {
        
        char *ptr = cmdBuf;
        
        bool inQuotes = false;
        
        while ( *ptr ) {
            
            if ( *ptr == '"' ) {
                
                inQuotes = ! inQuotes;
            }
            else if ( *ptr == '#' && !inQuotes ) {
                
                *ptr = '\0';
                break;
            }
            
            ptr++;
        }
    }
    
    return((int) strlen( cmdBuf ));
}

//----------------------------------------------------------------------------------------
// "removeChar" will remove a character from the input buffer left of the cursor 
// position and adjust the input buffer string size accordingly. If the cursor is at
// the end of the string, both string size and cursor position are decremented by one.
//
// ??? can we simplify this ?
//----------------------------------------------------------------------------------------
void removeChar( char *buf, int *strSize, int *pos ) {
    
    if (( *strSize > 0 ) && ( *strSize == *pos )) {
        
        *strSize        = *strSize - 1;
        *pos            = *pos - 1;
        buf[ *strSize ] = '\0';
    }
    else if (( *strSize > 0 ) && ( *pos > 0 )) {
        
        for ( int i = *pos; i < *strSize; i++ ) buf[ i ] = buf[ i + 1 ];
        
        *strSize        = *strSize - 1;
        *pos            = *pos - 1;
        buf[ *strSize ] = '\0';
    }
}

//----------------------------------------------------------------------------------------
// "insertChar" will insert a character in the input buffer at the cursor position and
// adjust cursor and overall string size accordingly. There are two basic cases. The
// first is simply appending to the buffer when both current string size and cursor
// position are equal. The second is when the cursor is somewhere in the input buffer. 
// In this case we need to shift the characters to the right to make room first.
//
//----------------------------------------------------------------------------------------
void insertChar( char *buf, int ch, int *strSize, int *pos ) {
    
    if ( *pos == *strSize ) {
        
        buf[ *strSize ] = ch;
    }
    else if ( *pos < *strSize ) {
        
        for ( int i = *strSize; i > *pos; i-- ) buf[ i ] = buf[ i - 1 ];
        buf[ *pos ] = ch;
    }
    
    *strSize        = *strSize + 1;
    *pos            = *pos + 1;
    buf[ *strSize ] = '\0';
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

T64Word acceptNumExpr( SimErrMsgId errCode ) {

     SimExpr     rExpr;
     if ( rExpr.typ == TYP_NUM ) return ( rExpr.u.val );
     else throw ( errCode );
}

}; // namespace


//****************************************************************************************
//****************************************************************************************
//
// Object methods - SimCmdHistory
//
//----------------------------------------------------------------------------------------
// The simulator command interpreter features a simple command history. It is a circular
// buffer that holds the last commands. There are functions to show the command history,
// re-execute a previous command and to retrieve a previous command for editing. The
// command stack can be accessed with relative command numbers, i.e. "current - 3" or
// by absolute command number, when still present in the history stack.
//
//----------------------------------------------------------------------------------------
SimCmdHistory::SimCmdHistory( ) {
    
    this -> head    = 0;
    this -> tail    = 0;
    this -> count   = 0;
}

//----------------------------------------------------------------------------------------
// Add a command line. If the history buffer is full, the oldest entry is re-used. Th
// head index points to the next entry for allocation.
//
//----------------------------------------------------------------------------------------
void SimCmdHistory::addCmdLine( char *cmdStr ) {
    
    SimCmdHistEntry *ptr = &history[ head ];
    
    ptr -> cmdId = nextCmdNum;
    strncpy( ptr -> cmdLine, cmdStr, 256 );
    
    if ( count == MAX_CMD_HIST_BUF_SIZE ) tail = ( tail + 1 ) % MAX_CMD_HIST_BUF_SIZE;
    else count++;
    
    nextCmdNum ++;
    head = ( head + 1 ) % MAX_CMD_HIST_BUF_SIZE;
}

//----------------------------------------------------------------------------------------
// Get a command line from the command history. If the command reference is negative, 
// the entry relative to the top is used. "head - 1" refers to the last entry entered.
// If the command ID is positive, we search for the entry with the matching command id,
// if still in the history buffer. Optionally, we return the absolute command Id.
//
//----------------------------------------------------------------------------------------
char *SimCmdHistory::getCmdLine( int cmdRef, int *cmdId ) {
    
    if (( cmdRef >= 0 ) && (( nextCmdNum - cmdRef ) > MAX_CMD_HIST_BUF_SIZE ))
         return ( nullptr );

    if (( cmdRef < 0  ) && ( - cmdRef > nextCmdNum )) return ( nullptr );
    
    if ( count == 0 ) return ( nullptr );
    
    if ( cmdRef >= 0 ) {
        
        for ( int i = 0; i < count; i++ ) {
            
            int pos = ( tail + i ) % MAX_CMD_HIST_BUF_SIZE;
            if ( history[ pos ].cmdId == cmdRef ) {
                
                if ( cmdId != nullptr ) *cmdId = history[ pos ].cmdId;
                return( history[ pos ].cmdLine );
            }
        }
        
        return( nullptr );
    }
    else {
        
        int pos = ( head + cmdRef + MAX_CMD_HIST_BUF_SIZE) % MAX_CMD_HIST_BUF_SIZE;
        
        if (( pos < head ) && ( pos >= tail )) {
            
            if ( cmdId != nullptr ) *cmdId = history[ pos ].cmdId;
            return history[ pos ].cmdLine;
        }
        else return( nullptr );
    }
}

//----------------------------------------------------------------------------------------
// The command history maintains a command counter and number, which we return here.
//
//----------------------------------------------------------------------------------------
int SimCmdHistory::getCmdNum( ) {
    
    return( nextCmdNum );
}

int  SimCmdHistory::getCmdCount( ) {
    
    return( count );
}


//****************************************************************************************
//****************************************************************************************
//
// Object methods - SimCommandsWin
//
//----------------------------------------------------------------------------------------
// Object constructor.
//
//----------------------------------------------------------------------------------------
SimCommandsWin::SimCommandsWin( SimGlobals *glb ) : SimWin( glb ) {
    
    this -> glb = glb;
    
    tok         = new SimTokenizer( );
    eval        = new SimExprEvaluator( glb, tok );
    hist        = new SimCmdHistory( );
    winOut      = new SimWinOutBuffer( );
    disAsm      = new T64DisAssemble( );
    inlineAsm   = new T64Assemble( );
}

//----------------------------------------------------------------------------------------
// The default values are the initial settings when windows is brought up the first
// time, or for the WDEF command.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::setDefaults( ) {
    
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setRows( 21 );
    setColumns( 128 );
    setDefColumns( 128, 128 );
    setWinType( WT_CMD_WIN );
    setEnable( true );
}

//----------------------------------------------------------------------------------------
// "readCmdLine" is used by the command line interpreter to get the command. Since we
// run in raw mode, the basic handling of backspace, carriage return, relevant escape
// sequences, etc. needs to be processed in this routine directly. Characters other 
// than the special characters are piled up in a local buffer until we read in a 
// carriage return. The core is a state machine that examines a character read to 
// analyze whether this is a special character or sequence. Any "normal" character is
// just added to the line buffer. The states are as follows:
//
//      CT_NORMAL: got a character, analyze it.
//      CT_ESCAPE: check the characters got. If a "[" we handle an escape sequence.
//      CT_ESCAPE_BRACKET: analyze the argument after "esc[" input got so far.
//      CT_WIN_SPECIAL: analyze a MS windows special character.
//
// A carriage return character will append a zero to the command line input got so 
// far. We are done reading the input line. Next, we emit a carriage return to the 
// console. The prompt and the command string along with a carriage return are appended 
// to the command output buffer. Before returning to the caller, the last thing to do
// is to remove any comment from the line.
//
// The left and right arrows move the cursor in the command line. Backspacing and
// inserting will then take place at the current cursor position shifting any content
// to the right of the cursor when inserting and shifting to the left when deleting.
//
// On MS windows a special character indicates the start of a special button pressed. 
// We currently recognize only the cursor keys.
//
// We also have the option of a prefilled command buffer for editing a command line 
// before hitting return. This option is used by the REDO command which lists a 
// previously entered command presented for editing.
//
// Finally, there is the cursor up and down key. These keys are used to scroll the 
// command line window. This is the case where we need to get lines from the output 
// buffer to fill from top or bottom of the command window display. We also need to 
// ensure that when a new command line is read in, we are with our cursor at the input
//  line, right after the prompt string.
//
//----------------------------------------------------------------------------------------
int SimCommandsWin::readCmdLine( char *cmdBuf, int initialCmdBufLen, char *promptBuf ) {
    
    enum CharType : uint16_t { CT_NORMAL, CT_ESCAPE, CT_ESCAPE_BRACKET, CT_WIN_SPECIAL };
    
    int         promptBufLen    = (int) strlen( promptBuf );
    int         cmdBufCursor    = 0;
    int         cmdBufLen       = 0;
    int         ch              = ' ';
    CharType    state           = CT_NORMAL;
    
    if (( promptBufLen > 0 ) && ( glb -> console -> isConsole( ))) {
        
        promptBufLen =  glb -> console -> writeChars( " " );
        promptBufLen += glb -> console -> writeChars( "%s", promptBuf );
    }
    
    if ( initialCmdBufLen > 0 ) {
        
        cmdBuf[ initialCmdBufLen ]  = '\0';
        cmdBufLen                   = initialCmdBufLen;
        cmdBufCursor                = initialCmdBufLen;
    }
    else cmdBuf[ 0 ] = '\0';
    
    while ( true ) {
        
        ch = glb -> console -> readChar( );
        
        switch ( state ) {
                
            case CT_NORMAL: {
                
                if ( isEscapeChar( ch )) {
                    
                    state = CT_ESCAPE;
                }
                else if ( isWInSpecialChar( ch )) {
                    
                    state = CT_WIN_SPECIAL;
                }
                else if ( isCarriageReturnChar( ch )) {
                  
                    cmdBuf[ cmdBufLen ] = '\0';
                    glb -> console -> writeCarriageReturn( );
                    
                    winOut -> addToBuffer( promptBuf );
                    winOut -> addToBuffer( cmdBuf );
                    winOut -> addToBuffer( "\n" );
                    
                    cmdBufLen = removeComment( cmdBuf );
                    return ( cmdBufLen );
                }
                else if ( isBackSpaceChar( ch )) {
                    
                    if ( cmdBufLen > 0 ) {
                        
                        removeChar( cmdBuf, &cmdBufLen, &cmdBufCursor );
                        
                        glb -> console -> eraseChar( );
                        glb -> console -> writeCursorLeft( );
                        glb -> console -> writeChar( cmdBuf[ cmdBufCursor ] );
                    }
                }
                else {
                   
                    if ( cmdBufLen < CMD_LINE_BUF_SIZE - 1 ) {
                       
                        insertChar( cmdBuf, ch, &cmdBufLen, &cmdBufCursor );
                        
                        if ( isprint( ch ))
                            glb -> console -> writeCharAtLinePos( ch, 
                                                cmdBufCursor + promptBufLen );
                    }
                }
                
            } break;
                
            case CT_ESCAPE: {
                
                if ( isLeftBracketChar( ch )) state = CT_ESCAPE_BRACKET;
                else                          state = CT_NORMAL;
                
            } break;
                
            case CT_ESCAPE_BRACKET: {
                
                switch ( ch ) {
                        
                    case 'D': {
                        
                        if ( cmdBufCursor > 0 ) {
                            
                            cmdBufCursor --;
                            glb -> console -> writeCursorLeft( );
                        }
                        
                    } break;
                        
                    case 'C': {
                        
                        if ( cmdBufCursor < cmdBufLen ) {
                            
                            cmdBufCursor ++;
                            glb -> console -> writeCursorRight( );
                        }
                        
                    } break;
                        
                    case 'A': {
                        
                        winOut -> scrollUp( );
                        reDraw( );
                        setWinCursor( 0, promptBufLen );
                        
                    } break;
                        
                    case 'B': {
                        
                        winOut -> scrollDown( );
                        reDraw( );
                        setWinCursor( 0, promptBufLen  );
                        
                    } break;
                        
                    default: ;
                }
                
                state = CT_NORMAL;
                
            } break;
                
            case CT_WIN_SPECIAL: {
                
                switch ( ch ) {
                        
                    case 'K': {
                        
                        if ( cmdBufCursor > 0 ) {
                            
                            cmdBufCursor --;
                            glb -> console -> writeCursorLeft( );
                        }
                        
                    } break;
                        
                    case 'M' : {
                        
                        if ( cmdBufCursor < cmdBufLen ) {
                            
                            cmdBufCursor ++;
                            glb -> console -> writeCursorRight( );
                        }
                        
                    } break;
                        
                    case 'H' : {
                        
                        winOut -> scrollUp( );
                        reDraw( );
                        setWinCursor( 0, promptBufLen );
                        
                    } break;
                        
                    case 'P': {
                        
                        winOut -> scrollDown( );
                        reDraw( );
                        setWinCursor( 0, promptBufLen  );
                        
                    } break;
                        
                    default: ;
                }
                
                state = CT_NORMAL;
                
            } break;
        }
    }
}

//----------------------------------------------------------------------------------------
// The banner line for command window. For now, we just label the banner line and show
// a little indicate whether we are in WIN mode or not.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::drawBanner( ) {
    
    uint32_t fmtDesc = FMT_BOLD | FMT_INVERSE;
    
    setWinCursor( 1, 1 );
    printTextField((char *) "Commands", ( fmtDesc | FMT_ALIGN_LFT ));
    padLine( fmtDesc ); 

    if ( glb -> winDisplay -> isWindowsOn( )) 
    printTextField((char *) "W", ( fmtDesc | FMT_LAST_FIELD ));
}

//----------------------------------------------------------------------------------------
// The body lines of the command window are displayed after the banner line. The window
// is filled from the output buffer. We first set the screen lines as the length of 
// the command window may have changed.
//
// Rows to show is the number of lines between the header line and the last line, which
// is out command input line. We fill from the lowest line upward to the header line.
// Finally, we set the cursor to the last line in the command window.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::drawBody( ) {
    
    char lineOutBuf[ MAX_WIN_OUT_LINE_SIZE ];
    
    glb -> console ->setFmtAttributes( FMT_DEF_ATTR );
  
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

//----------------------------------------------------------------------------------------
// "commandLineError" is a little helper that prints out the error encountered. We
// will print a caret marker where we found the error, and then return a false. Note 
// that the position needs to add the prompt part of the command line to where the 
// error was found in the command input.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::cmdLineError( SimErrMsgId errNum, char *argStr ) {
    
    for ( int i = 0; i < MAX_ERR_MSG_TAB; i++ ) {
        
        if ( errMsgTab[ i ].errNum == errNum ) {
            
            winOut -> writeChars( "%s\n", errMsgTab[ i ].errStr );
            return;
        }
    }
    
    winOut -> writeChars( "Error: %d", errNum );
    if ( argStr != nullptr )  winOut -> writeChars( "%32s", argStr );
    winOut -> writeChars( "/n" );
}

//----------------------------------------------------------------------------------------
// "promptYesNoCancel" is a simple function to print a prompt string with a decision
// question. The answer can be yes/no or cancel. A positive result is a "yes" a 
// negative result a "no", anything else a "cancel".
//
//----------------------------------------------------------------------------------------
int SimCommandsWin::promptYesNoCancel( char *promptStr ) {
    
    char buf[ 256 ] = "";
    int  ret        = 0;
    
    if ( readCmdLine( buf, 0, promptStr ) > 0 ) {
        
        if      (( buf[ 0 ] == 'Y' ) ||  ( buf[ 0 ] == 'y' ))   ret = 1;
        else if (( buf[ 0 ] == 'N' ) ||  ( buf[ 0 ] == 'n' ))   ret = -1;
        else                                                    ret = 0;
    }
    else ret = 0;
    
    winOut -> writeChars( "%s\n", buf );
    return( ret );
}

//----------------------------------------------------------------------------------------
// Token analysis helper functions.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::checkEOS( ) {
    
    if ( ! tok -> isToken( TOK_EOS )) throw ( ERR_TOO_MANY_ARGS_CMD_LINE );
}

void SimCommandsWin::acceptComma( ) {
    
    if ( tok -> isToken( TOK_COMMA )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_COMMA );
}

void SimCommandsWin::acceptLparen( ) {
    
    if ( tok -> isToken( TOK_LPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
}

void SimCommandsWin::acceptRparen( ) {
    
    if ( tok -> isToken( TOK_RPAREN )) tok -> nextToken( );
    else throw ( ERR_EXPECTED_LPAREN );
}

//----------------------------------------------------------------------------------------
// Display absolute memory content. We will show the memory starting with offset. The 
// words per line is an environmental variable setting. The offset is rounded down to 
// the next 8-byte boundary, the limit is rounded up to the next 8-byte boundary. We 
// display the data in words. The absolute memory address range currently consist of
// three memory objects. There is main physical memory, PDC memory and IO memory. This
// routine will make the appropriate call.
//
// ??? drop the words per line concept ?
// ?? what about the damn decimal rdx ?
//----------------------------------------------------------------------------------------
void  SimCommandsWin::displayAbsMemContent( T64Word ofs, T64Word len, int rdx ) {
    
    T64Word index           = ( ofs / sizeof( T64Word )) * sizeof( T64Word );
    T64Word limit           = ((( index + len ) + 7 ) / sizeof( T64Word ) ) * sizeof( T64Word );
    int     wordsPerLine    = glb -> env -> getEnvVarInt((char *) ENV_WORDS_PER_LINE );

    // ??? get the memory object ..

    while ( index < limit ) {

        winOut -> printNumber( index, FMT_HEX_2_4_4 );
        winOut -> writeChars( ": " );
        
        for ( uint32_t i = 0; i < wordsPerLine; i++ ) {
            
            if ( index < limit ) {

                // ??? check if valid address...
                // ??? print words... 
                
            }
            
            winOut -> writeChars( " " );
            index += 4; // ??? words per line ?
        }
        
        winOut -> writeChars( "\n" );
    }
    
    winOut -> writeChars( "\n" );
}

//----------------------------------------------------------------------------------------
// Display absolute memory content as code shown in assembler syntax. There is one
// word per line.
//
//----------------------------------------------------------------------------------------
void  SimCommandsWin::displayAbsMemContentAsCode( T64Word adr, T64Word len, int rdx ) {
    
    T64Word index   = ( adr / 4 ) * 4;
    T64Word limit   = ((( index + len ) + 3 ) / 4 );

    while ( index < limit ) {

        winOut -> printNumber( index, FMT_HEX_4 );
        winOut -> writeChars( ": " );

        // ??? get the data word....
        
        winOut -> writeChars( "\n" );
        
        index += 4;
    }
    
    winOut -> writeChars( "\n" );
}


//----------------------------------------------------------------------------------------
// This routine will print a TLB entry with each field formatted.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayTlbEntry( T64TlbEntry *entry, int rdx ) {
    
    winOut -> writeChars( "[" );

    winOut -> writeChars( "TLB flags " );
    
    winOut -> writeChars( "]" );

    winOut -> writeChars( "[" );

    winOut -> writeChars( "ACC " );
    
    winOut -> writeChars( "]" );
    
    winOut -> writeChars( " Vpn: " );
    
    winOut -> writeChars( " Ppn: " );

     winOut -> writeChars( " Pid: " );
}

//----------------------------------------------------------------------------------------
// "displayTlbEntries" displays a set of TLB entries, line by line.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayTlbEntries( T64Tlb *tlb, int index, int len, int rdx ) {
    
    if ( index + len <= tlb -> getTlbSize( )) {
        
        for ( uint32_t i = index; i < index + len; i++  ) {

            winOut -> writeChars( "0x%4x:", 0 );
            
            T64TlbEntry *ptr = tlb -> getTlbEntry( i );
            if ( ptr != nullptr ) displayTlbEntry( ptr, rdx );
            
            winOut -> writeChars( "\n" );
        }
        
    } else winOut -> writeChars( "index + len out of range\n" );
}

//----------------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayCacheLine( T64CacheLine *line, int rdx ) {

    winOut -> writeChars( "[" );
    winOut -> writeChars( "xxx" );
    winOut -> writeChars( "][" );
    winOut ->printNumber( 0, FMT_HEX_2_4_4 );
    winOut -> writeChars( "] " );

    for ( int i = 0; i < 4; i++ ) { // ??? fix cache line size...

        winOut ->printNumber( 0, FMT_HEX_4_4_4_4 );
        winOut -> writeChars( "  " );
    }
}

//----------------------------------------------------------------------------------------
// "displayCacheEntries" displays a list of cache line entries. Since we have a couple
// of block sizes and  perhaps one or more sets, the display is rather complex.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayCacheEntries( T64Cache *cPtr, int index, int len, int rdx ) {

   // ??? get cache size and number of sets
   // ??? for each index print index and setId 
   // ??? printCacheLine
}

//----------------------------------------------------------------------------------------
// Return the current command entered.
//
//----------------------------------------------------------------------------------------
SimTokId SimCommandsWin::getCurrentCmd( ) {
    
    return( currentCmd );
}

//----------------------------------------------------------------------------------------
// Our friendly welcome message with the actual program version. We also set some of the
// environment variables to an initial value. Especially string variables need to be set 
// as they are not initialized from the environment variable table.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::printWelcome( ) {
    
    glb -> env -> setEnvVar((char *) ENV_EXIT_CODE, (T64Word) 0 );
    
    if ( glb -> console -> isConsole( )) {
        
        winOut -> writeChars( "Twin-64 Simulator, Version: %s, Patch Level: %d\n",
                             glb -> env -> getEnvVarStr((char *) ENV_PROG_VERSION ),
                             glb -> env -> getEnvVarStr((char *) ENV_PATCH_LEVEL ));
        
        winOut -> writeChars( "Git Branch: %s\n",
                             glb -> env -> getEnvVarStr((char *) ENV_GIT_BRANCH ));
        
        winOut -> writeChars( "\n" );
    }
}

//----------------------------------------------------------------------------------------
// "promptCmdLine" lists out the prompt string.
//
//----------------------------------------------------------------------------------------
int SimCommandsWin::buildCmdPrompt( char *promptStr, int promptStrLen ) {
    
    if ( glb -> env -> getEnvVarBool((char *) ENV_SHOW_CMD_CNT )) {
            
        return( snprintf( promptStr, promptStrLen,
                           "(%i) ->",
                           (int) glb -> env -> getEnvVarInt((char *) ENV_CMD_CNT )));
        }
    else return( snprintf( promptStr, promptStrLen, "->" ));
}

//----------------------------------------------------------------------------------------
// "execCmdsFromFile" will open a text file and interpret each line as a command. This
// routine is used by the "XF" command and also as the handler for the program argument
// option to execute a file before entering the command loop.
//
// XF "<file-path>"
//
// ??? which error would we like to report here vs. pass on to outer command loop ?
//----------------------------------------------------------------------------------------
void SimCommandsWin::execCmdsFromFile( char* fileName ) {
    
    char cmdLineBuf[ CMD_LINE_BUF_SIZE ] = "";
    
    try {
        
        if ( strlen( fileName ) > 0 ) {
            
            FILE *f = fopen( fileName, "r" );
            if ( f != nullptr ) {
                
                while ( ! feof( f )) {
                    
                    strcpy( cmdLineBuf, "" );
                    fgets( cmdLineBuf, sizeof( cmdLineBuf ), f );
                    cmdLineBuf[ strcspn( cmdLineBuf, "\r\n" ) ] = 0;
                    
                    if ( glb -> env -> getEnvVarBool((char *) ENV_ECHO_CMD_INPUT )) {
                        
                        winOut -> writeChars( "%s\n", cmdLineBuf );
                    }
                    
                    removeComment( cmdLineBuf );
                    evalInputLine( cmdLineBuf );
                }
            }
            else throw( ERR_OPEN_EXEC_FILE );
        }
        else throw ( ERR_EXPECTED_FILE_NAME  );
    }
    
    catch ( SimErrMsgId errNum ) {
        
        switch ( errNum ) {
                
            case ERR_OPEN_EXEC_FILE: {
                
                winOut -> writeChars( "Error in opening file: \"%s\"", fileName );
                
            } break;
                
            default: throw ( errNum );
        }
    }
}

//----------------------------------------------------------------------------------------
// Help command. With no arguments, a short help overview is printed. There are 
// commands, widow commands and predefined functions.
//
//  help ( cmdId | ‘commands‘ | 'wcommands‘ | ‘wtypes‘ | ‘predefined‘ | 'regset' )
//----------------------------------------------------------------------------------------
void SimCommandsWin::helpCmd( ) {
    
    const char FMT_STR_SUMMARY[ ] = "%-16s%s\n";
    const char FMT_STR_DETAILS[ ] = "%s - %s\n";
    
    if ( tok -> isToken( TOK_EOS )) {
        
        for ( int i = 0; i < MAX_CMD_HELP_TAB; i++ ) {
            
            if ( cmdHelpTab[ i ].helpTypeId == TYP_CMD )
                winOut -> writeChars( FMT_STR_SUMMARY, 
                    cmdHelpTab[ i ].cmdNameStr, cmdHelpTab[ i ].helpStr );
        }
        
        winOut -> writeChars( "\n" );
    }
    else if (( tok -> isTokenTyp( TYP_CMD  )) ||
             ( tok -> isTokenTyp( TYP_WCMD )) ||
             ( tok -> isTokenTyp( TYP_P_FUNC ))) {

        if (( tok -> isToken( CMD_SET )) ||
             ( tok -> isToken( WCMD_SET )) ||
             ( tok -> isToken( REG_SET )) ||
             ( tok -> isToken( WTYPE_SET )) ||
             ( tok -> isToken( PF_SET ))) {

            for ( int i = 0; i < MAX_CMD_HELP_TAB; i++ ) {
                
                    if ( cmdHelpTab[ i ].helpTypeId == tok -> tokTyp( )) {

                        winOut -> writeChars( FMT_STR_SUMMARY, 
                                              cmdHelpTab[ i ].cmdNameStr, 
                                              cmdHelpTab[ i ].helpStr );
                    }
            }
        }
        else {

            for ( int i = 0; i < MAX_CMD_HELP_TAB; i++ ) {
                
                if ( cmdHelpTab[ i ].helpTokId == tok -> tokId( )) {
                    
                    winOut -> writeChars( FMT_STR_DETAILS, 
                        cmdHelpTab[ i ].cmdSyntaxStr, cmdHelpTab[ i ].helpStr );
                }
            }
        }
    }
    else throw ( ERR_INVALID_ARG );
}

//----------------------------------------------------------------------------------------
// Exit command. We will exit with the environment variable value for the exit code
// or the argument value in the command. This will be quite useful for test script
// development.
//
// EXIT <val>
//----------------------------------------------------------------------------------------
void SimCommandsWin::exitCmd( ) {
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        int exitVal = glb -> env -> getEnvVarInt((char *) ENV_EXIT_CODE );
        exit(( exitVal > 255 ) ? 255 : exitVal );
    }
    else {

        exit( acceptNumExpr( ERR_INVALID_EXIT_VAL ));
    }
}

//----------------------------------------------------------------------------------------
// ENV command. The test driver has a few global environment variables for data format, 
// command count and so on. The ENV command list them all, one in particular and also
// modifies one if a value is specified. If the ENV variable dos not exist, it will be
// allocated with the type of the value. A value of the token NIL will remove a user
//  defined variable.
//
//  ENV [ <var> [ <val> ]]",
//----------------------------------------------------------------------------------------
void SimCommandsWin::envCmd( ) {
    
    SimEnv *env = glb -> env;
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        env -> displayEnvTable( );
    }
    else if ( tok -> tokTyp( ) == TYP_IDENT ) {
        
        char envName[ MAX_ENV_NAME_SIZE ];
        
        strcpy( envName, tok -> tokStr( ));
        upshiftStr( envName );
        
        tok -> nextToken( );
        if ( tok -> tokId( ) == TOK_EOS ) {
            
            if ( env -> isValid( envName )) env -> displayEnvTableEntry( envName );
            else throw ( ERR_ENV_VAR_NOT_FOUND );
        }
        else {
            
            SimExpr rExpr;
            eval -> parseExpr( &rExpr );
            
            if ( rExpr.typ == TYP_NUM )        
                env -> setEnvVar( envName, rExpr.u.val );
            else if ( rExpr.typ == TYP_BOOL )       
                env -> setEnvVar( envName, rExpr.u.bVal );
            else if ( rExpr.typ == TYP_STR )        
                env -> setEnvVar( envName, rExpr.u.str );
        }
    }
}

//----------------------------------------------------------------------------------------
// Execute commands from a file command. 
//
// XF "<filename>"
//----------------------------------------------------------------------------------------
void SimCommandsWin::execFileCmd( ) {
    
    if ( tok -> tokTyp( ) == TYP_STR ) {
        
        execCmdsFromFile( tok -> tokStr( ));
    }
    else throw( ERR_EXPECTED_FILE_NAME );
}

//----------------------------------------------------------------------------------------
// Reset command.
//
//  RESET [ ( 'PROC' | 'MEM' | 'STATS' | 'ALL' ) ]
//
// ??? rethink what we want ... reset the SYSTEM ? all CPUs ?
//
// ??? when and what statistics to also reset ?
// ??? what execution mode will put the CPU ? halted ?
//----------------------------------------------------------------------------------------
void SimCommandsWin::resetCmd( ) {
    
    if ( tok -> isToken( TOK_EOS )) {
        
        // glb -> cpu -> reset( );
    }
    else if ( tok -> tokTyp( ) == TYP_SYM ) {
        
        switch( tok -> tokId( )) {
                
            case TOK_PROC: {
                
              //  glb -> cpu -> reset( );
                
            } break;
                
            case TOK_MEM: {
                
              //  glb -> cpu -> physMem -> reset( );
                
            } break;

            case TOK_STATS: {


            } break;
                
            case TOK_ALL: {
                
             //   glb -> cpu -> reset( );
             //   glb -> cpu -> physMem -> reset( );
                
            } break;
                
            default: throw ( ERR_INVALID_ARG );
        }
    }
    else throw ( ERR_INVALID_ARG );
}

//----------------------------------------------------------------------------------------
// Run command. The command will just run the CPU until a "halt" instruction is detected.
//
//  RUN
//
// ??? see STEP command for details on the console handling.
//----------------------------------------------------------------------------------------
void SimCommandsWin::runCmd( ) {
    
    winOut -> writeChars( "RUN command to come ... \n");
}

//----------------------------------------------------------------------------------------
// Step command. The command will execute one instruction. Default is one instruction.
// There is an ENV variable that will set the default to be a single clock step.
//
//  S [ <steps> ]
//
// ??? we need to handle the console window. It should be enabled before we pass control
// to the CPU.
// ??? make it the current window, saving the previous current window.
// ??? put the console mode into non-blocking and hand over to the CPU.
// ??? on return from the CPU steps, enable blocking mode again and restore the current window.
//----------------------------------------------------------------------------------------
void SimCommandsWin::stepCmd( ) {
    
    uint32_t numOfSteps = 1;
    
    if ( tok -> tokTyp( ) == TYP_NUM ) {

        numOfSteps = acceptNumExpr( ERR_EXPECTED_STEPS );
    }
    
    checkEOS( );
    
    // ??? system call ...
}

//----------------------------------------------------------------------------------------
// Write line command. We analyze the expression and ping out the result.
//
//  W <expr> [ , <rdx> ]
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::writeLineCmd( ) {
    
    SimExpr  rExpr;
    int      rdx;
    
    eval -> parseExpr( &rExpr );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        if (( tok -> tokId( ) == TOK_HEX ) || ( tok -> tokId( ) == TOK_DEC )) {
            
            rdx = tok -> tokVal( );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_FMT_OPT );
    }
    else rdx = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    checkEOS( );
    
    switch ( rExpr.typ ) {
            
        case TYP_BOOL: {
            
            if ( rExpr.u.bVal == true ) winOut -> writeChars( "TRUE\n" );
            else                        winOut -> writeChars( "FALSE\n" );
            
        } break;
            
        case TYP_NUM: {

            if      ( rdx == 16 )   winOut -> printNumber( rExpr.u.val, FMT_HEX );
            else if ( rdx == 10 )   winOut -> printNumber( rExpr.u.val, FMT_DEC );
            else                    winOut -> writeChars( "Invalid Radix" );
        
            winOut -> writeChars( "\n" );
            
        } break;
            
        case TYP_STR: {
            
            winOut -> writeChars( "\"%s\"\n", rExpr.u.str );
            
        } break;
            
        default: throw (  ERR_INVALID_EXPR );
    }
}

//----------------------------------------------------------------------------------------
// The HIST command displays the command history. Optional, we can only report to a 
// certain depth from the top.
//
//  HIST [ depth ]
//
// ??? command numbers are shown as absolute numbers. Have an option to show relative
//  numbers ?
// ??? how about a negative depth shows relative numbers ?
//----------------------------------------------------------------------------------------
void SimCommandsWin::histCmd( ) {
    
    int     depth = 0;
    int     cmdCount = hist -> getCmdCount( );
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        depth = acceptNumExpr( ERR_INVALID_NUM );
    }
    
    if (( depth == 0 ) || ( depth > cmdCount )) depth = cmdCount;
    
    for ( int i = -depth; i < 0; i++ ) {
        
        int  cmdRef = 0;
        char *cmdLine = hist -> getCmdLine( i, &cmdRef );
        
        if ( cmdLine != nullptr )
            winOut -> writeChars( "[%d]: %s\n", cmdRef, cmdLine );
    }
}

//----------------------------------------------------------------------------------------
// Execute a previous command again. The command Id can be an absolute command Id or
// a top of the command history buffer relative command Id. The selected command is
// copied to the top of the history buffer and then passed to the command interpreter
// for execution.
//
// DO <cmdNum>
//----------------------------------------------------------------------------------------
void SimCommandsWin::doCmd( ) {

    int cmdId = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        cmdId = acceptNumExpr( ERR_INVALID_NUM );
    }
    
    char *cmdStr = hist -> getCmdLine( cmdId );
    
    if ( cmdStr != nullptr ) evalInputLine( cmdStr );
}

//----------------------------------------------------------------------------------------
// REDO is almost like DO, except that we retrieve the selected command and put it
// already into the input command line string for the readCmdLine routine. We also 
// print it without a carriage return. The idea is that it can now be edited. The 
// edited command is added to the history buffer and then executed. The REDO command
// itself is not added to the command history stack. If the cmdNum is omitted, REDO 
// will take the last command entered.
//
// REDO <cmdNum>
//----------------------------------------------------------------------------------------
void SimCommandsWin::redoCmd( ) {
    
    int cmdId = -1;
    
    if ( tok -> tokId( ) != TOK_EOS ) {
        
        cmdId = acceptNumExpr( ERR_INVALID_NUM );
    }
    
    char *cmdStr = hist -> getCmdLine( cmdId );
    
    if ( cmdStr != nullptr ) {
        
        char tmpCmd[ 256 ];
        strncpy( tmpCmd, cmdStr, sizeof( tmpCmd ));
        
        glb -> console -> writeChars( "%s", tmpCmd );
        if ( readCmdLine( tmpCmd, (int) strlen( tmpCmd ), (char *)"" ))
             evalInputLine( tmpCmd );
    }
}

//----------------------------------------------------------------------------------------
// Modify register command. This command modifies a register within a register set.
//
// MR <reg> <val> [ <proc> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::modifyRegCmd( ) {
    
    SimTokTypeId    regSetId    = TYP_GREG;
    int             regNum      = 0;
    T64Word         val         = 0;
    
    if (( tok -> tokTyp( ) == TYP_GREG )        ||
        ( tok -> tokTyp( ) == TYP_CREG )        ||
        ( tok -> tokTyp( ) == TYP_PSW_PREG )) {
        
        regSetId    = tok -> tokTyp( );
        regNum      = tok -> tokVal( );
        tok -> nextToken( );
    }
    else throw ( ERR_INVALID_REG_ID );
    
    if ( tok -> tokId( ) == TOK_EOS ) throw( ERR_EXPECTED_NUMERIC );

    val = acceptNumExpr( ERR_INVALID_NUM );

    // ??? figure out which Processor...
    
    switch( regSetId ) {

        case TYP_PSW_PREG: {


        } break;
            
        case TYP_GREG: {

            // glb -> cpu -> setReg( RC_GEN_REG_SET, regNum, val );   
        
        } break;

        case TYP_CREG: {

            //glb -> cpu -> setReg( RC_CTRL_REG_SET, regNum, val );
        
        } break;
            
        default: throw( ERR_EXPECTED_REG_SET );
    }
}

//----------------------------------------------------------------------------------------
// Display absolute memory command. The memory address is a byte address. The offset 
// address is a byte address, the length is measured in bytes, rounded up to the a 
// word size. We accept any address and length and only check that the offset plus 
// length does not exceed the address space. The display routines, who will call the 
// actual memory object will take care of gaps in the memory address range. The format
// specifier will allow for HEX, DECIMAL and CODE. In the case of the code option, the
// default number format option is used for showing the offset value.
//
//  DA <ofs> [ , <len> [ , <fmt> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayAbsMemCmd( ) {
    
    T64Word     ofs     = 0;
    T64Word     len     = sizeof( T64Word );
    int         rdx     = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    bool        asCode  = false;

    ofs = acceptNumExpr( ERR_EXPECTED_START_OFS );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        if ( tok -> isToken( TOK_COMMA )) len = sizeof( T64Word );
        else                              len = acceptNumExpr( ERR_EXPECTED_LEN );
    }
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        switch(  tok -> tokId( )) {

            case TOK_HEX: 
            case TOK_DEC:  {
                
                rdx = tok -> tokVal( ); 
                
            } break;
            
            case TOK_CODE: {
                
                asCode = true; 
            
            } break;
            
            case TOK_EOS: 
            default:        throw ( ERR_INVALID_FMT_OPT );
        }

        tok -> nextToken( );
    }
    
    checkEOS( );
    
    if (((uint64_t) ofs + len ) <= UINT32_MAX ) {
        
        if ( asCode ) displayAbsMemContentAsCode( ofs, len, rdx );
        else          displayAbsMemContent( ofs, len, rdx );
    }
    else throw ( ERR_OFS_LEN_LIMIT_EXCEEDED );
}

//----------------------------------------------------------------------------------------
// Modify absolute memory command. This command accepts data values for up to eight
// consecutive locations. We also use this command to populate physical memory from 
// a script file.
//
//  MA <ofs> <val>
//----------------------------------------------------------------------------------------
void SimCommandsWin::modifyAbsMemCmd( ) {
    
    T64Word adr = acceptNumExpr( ERR_EXPECTED_OFS );
    T64Word val = acceptNumExpr( ERR_INVALID_NUM );

    checkEOS( );
    
    // ??? call the system memory ...
}

//----------------------------------------------------------------------------------------
// Display cache entries command.
//
//  DCA <proc> <cache> <set> <index> [ , <len> [ , <fmt> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayCacheCmd( ) {
    
   //  CpuMem      *cPtr           = nullptr;
   
    int rdx = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    int index = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    int len   = 1;
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        
        if ( tok -> tokId( ) == TOK_COMMA ) {
            
            len = 1;
            tok -> nextToken( );
        }
        else {

            len = acceptNumExpr( ERR_EXPECTED_NUMERIC );
        }
    }
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        
        if (( tok -> tokId( ) == TOK_HEX ) || ( tok -> tokId( ) == TOK_DEC )) {
            
            rdx = tok -> tokVal( );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_FMT_OPT );
    }
    
    checkEOS( );

    // ??? do the work .... 
    
}

//----------------------------------------------------------------------------------------
// Purges a cache line from the cache.
//
//  DCA <proc> <cache> <set> <index>
//----------------------------------------------------------------------------------------
void SimCommandsWin::purgeCacheCmd( ) {
    
  //  CpuMem      *cPtr           = nullptr;
    uint32_t    index           = 0;
    uint32_t    set             = 0;

    index = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        set = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    }
    
    checkEOS( );

    // ??? do the work ...

}

//----------------------------------------------------------------------------------------
// Flushes a cache line from the cache.
//
// FCA <proc> <cache> <set> <index>
//----------------------------------------------------------------------------------------
void SimCommandsWin::flushCacheCmd( ) {
    
  //  CpuMem      *cPtr           = nullptr;
    uint32_t    index           = 0;
    uint32_t    set             = 0;

    index = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        set = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    }
    
    checkEOS( );
    
    
    // ??? do the work ....
}

//----------------------------------------------------------------------------------------
// Display TLB entries command.
//
//  DTLB <proc> <tlb> <set> <index> [ , <len> [ , <rdx> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayTLBCmd( ) {
    
    uint32_t    index       = 0;
    uint32_t    len         = 0;
    uint32_t    tlbSize     = 0;
  
    int         rdx         = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    index = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        
        if ( tok -> tokId( ) == TOK_COMMA ) {
            
            len = 1;
            tok -> nextToken( );
        }
        else {

            len = acceptNumExpr( ERR_EXPECTED_NUMERIC );
        }
    }
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        
        if (( tok -> tokId( ) == TOK_HEX ) || ( tok -> tokId( ) == TOK_DEC )) {
            
            rdx = tok -> tokVal( );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_FMT_OPT );
    }
    
    checkEOS( );
    
    if ( len == 0 ) len = tlbSize;
    
    if (( index > tlbSize ) || ( index + len > tlbSize )) throw ( ERR_TLB_SIZE_EXCEEDED );
    
  //   displayTlbEntries( tPtr, index, len, rdx );
    winOut -> writeChars( "\n" );
}

//----------------------------------------------------------------------------------------
// Insert into TLB command.
//
//  ITLB <proc> <tlb> <info1> <info2>
//----------------------------------------------------------------------------------------
void SimCommandsWin::insertTLBCmd( ) {
    
    #if 0
    SimExpr     rExpr;
    eval -> parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) argAcc = rExpr.u.val;
    else throw ( ERR_TLB_ACC_DATA );
    
    eval -> parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_NUM ) argAcc = rExpr.u.val;
    else throw ( ERR_TLB_ADR_DATA );
    
    if ( ! tPtr -> insertTlbEntryData( seg, ofs, argAcc, argAdr )) throw ( ERR_TLB_INSERT_OP );
    #endif
}

//----------------------------------------------------------------------------------------
// Purge from TLB command.
//
//  PTLB <proc> <tlb> <adr>
//  
//----------------------------------------------------------------------------------------
void SimCommandsWin::purgeTLBCmd( ) {
    
    #if 0
    SimExpr     rExpr;
    CpuTlb      *tPtr = nullptr;
    
    if ( tok -> tokId( ) == TOK_I ) {
        
        tPtr = glb -> cpu -> iTlb;
        tok -> nextToken( );
    }
    else if ( tok -> tokId( ) == TOK_D ) {
        
        tPtr = glb -> cpu -> dTlb;
        tok -> nextToken( );
    }
    else throw ( ERR_TLB_TYPE );
    
    eval -> parseExpr( &rExpr );
    
    if ( rExpr.typ == TYP_EXT_ADR ) {
        
        if ( ! tPtr -> purgeTlbEntryData( rExpr.seg, rExpr.ofs )) throw ( ERR_TLB_PURGE_OP );
    }
    else throw ( ERR_EXPECTED_EXT_ADR );
    #endif
}

//----------------------------------------------------------------------------------------
// Global windows commands. There are handlers for turning windows on, off and set 
// them back to their default values. We also support two stacks of windows next to
// each other.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::winOnCmd( ) {
   
    glb -> winDisplay -> windowsOn( );
}

void SimCommandsWin::winOffCmd( ) {
  
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    glb -> winDisplay -> windowsOff( );
}

void SimCommandsWin::winDefCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    glb -> winDisplay -> windowDefaults( );
    glb -> winDisplay -> reDraw( true );
}

void SimCommandsWin::winStacksEnable( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
        
        glb -> winDisplay -> winStacksEnable( true );
        glb -> winDisplay -> reDraw( true );
}

void SimCommandsWin::winStacksDisable( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
        
        glb -> winDisplay -> winStacksEnable( false );
        glb -> winDisplay -> reDraw( true );
}

//----------------------------------------------------------------------------------------
// Windows enable and disable. When enabled, a window does show up on the screen. The
// window number is optional, used for user definable windows.
//
//  <win>E [ <winNum> ]
//  <win>D [ <winNum> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winEnableCmd( ) {
    
    int winNum = 0;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        winNum = acceptNumExpr( ERR_EXPECTED_WIN_ID );
    }
    
    if ( glb -> winDisplay -> validWindowNum( winNum )) {
        
        glb -> winDisplay -> windowEnable( winNum, true );
        glb -> winDisplay -> reDraw( true );
    }
    else throw ( ERR_INVALID_WIN_ID );
}

void SimCommandsWin::winDisableCmd( ) {
    
    int winNum = 0;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        winNum = acceptNumExpr( ERR_EXPECTED_WIN_ID );
    }
    
    if ( glb -> winDisplay -> validWindowNum( winNum )) {
        
        glb -> winDisplay -> windowEnable( winNum, false );
        glb -> winDisplay -> reDraw( true );
    }
    else throw ( ERR_INVALID_WIN_ID );
}

//----------------------------------------------------------------------------------------
// Windows radix. This command sets the radix for a given window. We parse the command
// and the format option and pass the tokens to the screen handler. The window number
// is optional, used for user definable windows.
//
//  <win>R [ <radix> [ "," <winNum>]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetRadixCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    int     winNum  = 0;
    int     rdx     =  glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> winDisplay -> windowRadix( rdx, winNum );
        return;
    }
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        rdx = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
        tok -> nextToken( );
    }
    else {
        
        if      ( tok -> isToken( TOK_DEC )) rdx = 10;
        else if ( tok -> isToken( TOK_HEX )) rdx = 16;
        else    rdx = ::setRadix( acceptNumExpr( ERR_INVALID_RADIX ));
    }
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
    }

    checkEOS( );
    
    if ( ! glb -> winDisplay -> validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    glb -> winDisplay -> windowRadix( rdx, winNum );
}

//----------------------------------------------------------------------------------------
// Window scrolling. This command advances the item address of a scrollable window by
// the number of lines multiplied by the number of items on a line forward or backward.
// The meaning of the item address and line items is window dependent. If the amount 
// is zero, the default value of the window will be used. The window number is optional,
// used for user definable windows. If omitted, we mean the current window.
//
//  <win>F [ <amt> [ , <winNum> ]]
//  <win>B [ <amt> [ , <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winForwardCmd( ) {
  
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        glb -> winDisplay -> windowForward( 0, 0 );
    }
    else {

        int winItems = acceptNumExpr( ERR_INVALID_NUM );
        int winNum   = 0;
        
        if ( tok -> tokId( ) == TOK_COMMA ) {
            
            tok -> nextToken( );
            winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
        }
        else winNum = 0;
        
        checkEOS( );
        
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
        
        glb -> winDisplay -> windowForward( winItems, winNum );
    }
}

void SimCommandsWin::winBackwardCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        glb -> winDisplay -> windowBackward( 0, 0  );
    }
    else {

        int winItems = acceptNumExpr( ERR_INVALID_NUM );
        int winNum   = 0;
    
        if ( tok -> tokId( ) == TOK_COMMA ) {

            tok -> nextToken( );
            winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
        }

        checkEOS( );
    
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
    
        glb -> winDisplay -> windowBackward( winItems, winNum  );
    }
}

//----------------------------------------------------------------------------------------
// Window home. Each window has a home item address, which was set at window creation
// or trough a non-zero value previously passed to this command. The command sets the
// window item address to this value. The meaning of the item address is window 
// dependent. The window number is optional, used for user definable windows.
//
//  <win>H [ <pos> [ "," <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winHomeCmd( ) {
   
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        glb -> winDisplay -> windowHome( 0, 0 );
        return;
    }
    else {

        int winPos = acceptNumExpr( ERR_INVALID_NUM );
        int winNum = 0;
  
        if ( tok -> tokId( ) == TOK_COMMA ) {

            tok -> nextToken( );
            winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
        }
    
        checkEOS( );
    
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
        glb -> winDisplay -> windowHome( winPos, winNum  );
    }
}

//----------------------------------------------------------------------------------------
// Window jump. The window jump command sets the item address to the position argument.
// The meaning of the item address is window dependent. The window number is optional,
// used for user definable windows.
//
//  <win>J [ <pos> [ "," <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winJumpCmd( ) {
   
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        glb -> winDisplay -> windowHome( 0, 0 );
    }
    else {

        int winPos   = acceptNumExpr( ERR_INVALID_NUM );
        int winNum   = 0;
    
        if ( tok -> tokId( ) == TOK_COMMA ) {
        
            tok -> nextToken( );
            winNum = acceptNumExpr( ERR_INVALID_NUM );
        }

        checkEOS( );
    
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
        glb -> winDisplay -> windowJump( winPos, winNum );
    }
}

//----------------------------------------------------------------------------------------
// Set window lines. This command sets the the number of rows for a window. The number
// includes the banner line. If the "lines" argument is omitted, the window default 
// value will be used. The window number is optional, used for user definable windows.
//
//  WL [ <lines> [ "," <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetRowsCmd( ) {
   
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        glb -> winDisplay -> windowHome( 0, 0  );
    }
    else {

        int winLines = acceptNumExpr( ERR_INVALID_NUM );
        int winNum      = 0;
    
        if ( tok -> tokId( ) == TOK_COMMA ) {
        
            tok -> nextToken( );
            winNum = acceptNumExpr( ERR_INVALID_NUM );
        }

        checkEOS( );
    
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
    
        glb -> winDisplay -> windowSetRows( winLines, winNum );
        glb -> winDisplay -> reDraw( true );
    }    
}

//----------------------------------------------------------------------------------------
// Set command window lines. This command sets the the number of rows for the command
// window. The number includes the banner line. If the "lines" argument is omitted, the
// window default value will be used. 
//
//  CWL [ <lines> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetCmdWinRowsCmd( ) {

    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );

    if ( tok -> tokId( ) == TOK_EOS ) {

        glb -> winDisplay -> windowSetCmdWinRows( 10 ); // fix .... default..
    }
    else {

        int winLines = acceptNumExpr( ERR_INVALID_NUM );
        glb -> winDisplay -> windowSetCmdWinRows( winLines );
        checkEOS( );
    }
}

//----------------------------------------------------------------------------------------
// Window current command. User definable windows are controlled by their window number.
// To avoid typing this number all the time for a user window command, a user window 
// can explicitly be set as the current command.
//
//  WC <winNum>
//----------------------------------------------------------------------------------------
void SimCommandsWin::winCurrentCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
   
    if ( tok -> isToken( TOK_EOS )) throw ( ERR_EXPECTED_WIN_ID );

    int winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
    
    checkEOS( );
    
    if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
        throw ( ERR_INVALID_WIN_ID );
    
    glb -> winDisplay -> windowCurrent( winNum );
}

//----------------------------------------------------------------------------------------
// This command toggles through alternate window content, if supported by the window.
// An example is the cache sets in a two-way associative cache. The toggle command will
// just flip through the sets.
//
//  WT [ <winNum> ]
//----------------------------------------------------------------------------------------
void  SimCommandsWin::winToggleCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
   
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> winDisplay -> windowToggle( glb -> winDisplay -> getCurrentWindow( ));
    }
    else {

        int winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
    
        glb -> winDisplay -> windowToggle( tok -> tokVal( ));
    }
}

//----------------------------------------------------------------------------------------
// This command exchanges the current user window with the user window specified. It 
// allows to change the order of the user windows in a stacks.
//
// WX <winNum>
//----------------------------------------------------------------------------------------
void SimCommandsWin::winExchangeCmd( ) {
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> isToken( TOK_EOS )) throw ( ERR_EXPECTED_WIN_ID );

    int winNum = acceptNumExpr( ERR_INVALID_WIN_ID );
    
    checkEOS( );
    
    if ( ! glb -> winDisplay -> validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    glb -> winDisplay -> windowExchangeOrder( winNum );
}

//----------------------------------------------------------------------------------------
// This command creates a new user window. The window is assigned a free index form 
// the windows list. This index is used in all the calls to this window. The window 
// type allows to select from a code window, a physical memory window, a TLB and a
// CACHE window.
//
//  WN <winType> [ , <arg> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winNewWinCmd( ) {
    
    SimTokId  winType = TOK_NIL;
    char      *argStr = nullptr;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokTyp( ) == TYP_SYM ) {
        
        winType = tok -> tokId( );
        
        #if 0
        if ((( winType == TOK_PM  ) && ( glb -> cpu -> physMem   == nullptr ))     ||
            (( winType == TOK_PC  ) && ( glb -> cpu -> physMem   == nullptr ))     ||
            (( winType == TOK_IT  ) && ( glb -> cpu -> iTlb      == nullptr ))     ||
            (( winType == TOK_DT  ) && ( glb -> cpu -> dTlb      == nullptr ))     ||
            (( winType == TOK_IC  ) && ( glb -> cpu -> iCacheL1  == nullptr ))     ||
            (( winType == TOK_DC  ) && ( glb -> cpu -> dCacheL1  == nullptr ))     ||
            (( winType == TOK_UC  ) && ( glb -> cpu -> uCacheL2  == nullptr ))) {
            
            throw ( ERR_WIN_TYPE_NOT_CONFIGURED );
        }
        #endif
        
        if ( ! glb -> winDisplay -> validWindowType( winType ))
            throw ( ERR_INVALID_WIN_TYPE );
        
        tok -> nextToken( );
    }
    else throw ( ERR_EXPECTED_WIN_ID );
    
    if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        
        if ( tok -> tokTyp( ) == TYP_STR ) argStr = tok -> tokStr( );
        else throw ( ERR_INVALID_ARG );
    }
    
    checkEOS( );
    
    glb -> winDisplay -> windowNew( winType, argStr );
    glb -> winDisplay -> reDraw( true );
}

//----------------------------------------------------------------------------------------
// This command removes  a user defined window or window range from the list of windows.
// A number of -1 will kill all user defined windows.
//
//  WK [[ <winNumStart> [ "," <winNumEnd]] || "-1" ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winKillWinCmd( ) {
    
    int         winNumStart     = 0;
    int         winNumEnd       = 0;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        winNumStart = glb -> winDisplay -> getCurrentWindow( );
        winNumEnd   = winNumStart;
    }
    else {

        winNumStart = acceptNumExpr( ERR_EXPECTED_NUMERIC );
    
        if ( winNumStart == -1 ) {
            
            winNumStart = 0;
            winNumEnd   = MAX_WINDOWS;
        }
        else {

            if ( tok -> tokId( ) == TOK_COMMA ) {
            
                tok -> nextToken( );
                winNumEnd = acceptNumExpr( ERR_EXPECTED_NUMERIC );     
            }
        }

        if ( winNumStart > winNumEnd ) winNumEnd = winNumStart;
    }
    
    if (( ! glb -> winDisplay -> validWindowNum( winNumStart )) ||
        ( ! glb -> winDisplay -> validWindowNum( winNumEnd ))) 
        throw ( ERR_INVALID_WIN_ID );
    
    glb -> winDisplay -> windowKill( winNumStart, winNumEnd );
    glb -> winDisplay -> reDraw( true );
}

//----------------------------------------------------------------------------------------
// This command assigns a user window to a stack. User windows can be displayed in a
// separate stack of windows. The first stack is always the main stack, where the 
// predefined and command window can be found.
//
//  WS <stackNum> [ , <winNumStart> [ , <winNumEnd ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetStackCmd( ) {
    
    int     stackNum    = 0;
    int     winNumStart = 0;
    int     winNumEnd   = 0;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );

    stackNum = acceptNumExpr( ERR_EXPECTED_STACK_ID );
    
    if ( ! glb -> winDisplay -> validWindowStackNum( stackNum ))
        throw ( ERR_INVALID_WIN_STACK_ID );
    
    if ( tok -> tokId( ) == TOK_EOS ) {
        
        winNumStart = glb -> winDisplay -> getCurrentWindow( );
        winNumEnd   = winNumStart;
    }
    else if ( tok -> tokId( ) == TOK_COMMA ) {
        
        tok -> nextToken( );
        winNumStart = acceptNumExpr( ERR_EXPECTED_NUMERIC );
        
        if ( tok -> tokId( ) == TOK_COMMA ) {
            
            tok -> nextToken( );
            winNumEnd = acceptNumExpr( ERR_EXPECTED_NUMERIC );
        }
        else winNumEnd = winNumStart;
    }
    else throw ( ERR_EXPECTED_COMMA );
    
    if (( ! glb -> winDisplay -> validWindowNum( winNumStart )) ||
        ( ! glb -> winDisplay -> validWindowNum( winNumEnd ))) 
        throw ( ERR_INVALID_WIN_ID );
    
    glb -> winDisplay -> windowSetStack( stackNum, winNumStart, winNumEnd );
    glb -> winDisplay -> reDraw( true );
}

//----------------------------------------------------------------------------------------
// Evaluate input line. There are commands, functions, expressions and so on. This 
// routine sets up the tokenizer and dispatches based on the first token in the input
// line. The commands are also added to the command history, with the exception of 
// the HITS, DO and REDOP commands.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::evalInputLine( char *cmdBuf ) {
    
    try {
        
        if ( strlen( cmdBuf ) > 0 ) {
            
            tok -> setupTokenizer( cmdBuf, (SimToken *) cmdTokTab );
            tok -> nextToken( );
            
            if (( tok -> isTokenTyp( TYP_CMD )) || ( tok -> isTokenTyp( TYP_WCMD ))) {
                
                currentCmd = tok -> tokId( );
                tok -> nextToken( );
                
                if (( currentCmd != CMD_HIST ) &&
                    ( currentCmd != CMD_DO ) &&
                    ( currentCmd != CMD_REDO )) {
                    
                    hist -> addCmdLine( cmdBuf );
                    glb -> env -> setEnvVar((char *) ENV_CMD_CNT, 
                                            (T64Word) hist -> getCmdNum( ));
                }
                
                switch( currentCmd ) {
                        
                    case TOK_NIL:                                           break;
                    case CMD_EXIT:          exitCmd( );                     break;
                        
                    case CMD_HELP:          helpCmd( );                     break;
                    case CMD_ENV:           envCmd( );                      break;
                    case CMD_XF:            execFileCmd( );                 break;
                        
                    case CMD_WRITE_LINE:    writeLineCmd( );                break;
                        
                    case CMD_HIST:          histCmd( );                     break;
                    case CMD_DO:            doCmd( );                       break;
                    case CMD_REDO:          redoCmd( );                     break;
                        
                    case CMD_RESET:         resetCmd( );                    break;
                    case CMD_RUN:           runCmd( );                      break;
                    case CMD_STEP:          stepCmd( );                     break;
                        
                    case CMD_MR:            modifyRegCmd( );                break;
                        
                    case CMD_DA:            displayAbsMemCmd( );            break;
                    case CMD_MA:            modifyAbsMemCmd( );             break;
                        
                    case CMD_D_TLB:         displayTLBCmd( );               break;
                    case CMD_I_TLB:         insertTLBCmd( );                break;
                    case CMD_P_TLB:         purgeTLBCmd( );                 break;
                        
                    case CMD_D_CACHE:       displayCacheCmd( );             break;
                    case CMD_P_CACHE:       purgeCacheCmd( );               break;
                    case CMD_F_CACHE:       flushCacheCmd( );               break;
                        
                    case CMD_WON:           winOnCmd( );                    break;
                    case CMD_WOFF:          winOffCmd( );                   break;
                    case CMD_WDEF:          winDefCmd( );                   break;
                    case CMD_WSE:           winStacksEnable( );             break;
                    case CMD_WSD:           winStacksDisable( );            break;
                        
                    case CMD_WC:            winCurrentCmd( );               break;
                    case CMD_WN:            winNewWinCmd( );                break;
                    case CMD_WK:            winKillWinCmd( );               break;
                    case CMD_WS:            winSetStackCmd( );              break;
                    case CMD_WT:            winToggleCmd( );                break;
                    case CMD_WX:            winExchangeCmd( );              break;
                    case CMD_WF:            winForwardCmd( );               break;
                    case CMD_WB:            winBackwardCmd( );              break;
                    case CMD_WH:            winHomeCmd( );                  break;
                    case CMD_WJ:            winJumpCmd( );                  break;
                    case CMD_WE:            winEnableCmd( );                break;
                    case CMD_WD:            winDisableCmd( );               break;
                    case CMD_WR:            winSetRadixCmd( );              break;    
                    case CMD_CWL:           winSetCmdWinRowsCmd( );         break;
                    case CMD_WL:            winSetRowsCmd( );               break;
                        
                    default:                throw ( ERR_INVALID_CMD );
                }
            }
            else {
            
                hist -> addCmdLine( cmdBuf );
                glb -> env -> setEnvVar((char *) ENV_CMD_CNT, 
                                        (T64Word) hist -> getCmdNum( ));
                glb -> env -> setEnvVar((char *) ENV_EXIT_CODE, (T64Word) -1 );
                throw ( ERR_INVALID_CMD );
            }
        }
    }
    
    catch ( SimErrMsgId errNum ) {
        
        glb -> env -> setEnvVar((char *) ENV_EXIT_CODE, (T64Word) -1 );
        cmdLineError( errNum );
    }
}

//----------------------------------------------------------------------------------------
// "cmdLoop" is the command line input interpreter. The basic loop is to prompt for
// the next input, read the input and evaluates it. If we are in windows mode, we also
// redraw the screen.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::cmdInterpreterLoop( ) {
    
    char cmdLineBuf[ CMD_LINE_BUF_SIZE ];
    char cmdPrompt[ CMD_LINE_BUF_SIZE ];
   
    printWelcome( );
    glb -> winDisplay -> reDraw( );
    
    while ( true ) {
        
        buildCmdPrompt( cmdPrompt, sizeof( cmdPrompt ));
        int cmdLen = readCmdLine( cmdLineBuf, 0, cmdPrompt );
        
        if ( cmdLen > 0 ) evalInputLine( cmdLineBuf );
        glb -> winDisplay -> reDraw( );
    }
}
