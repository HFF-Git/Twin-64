//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator command window
//
//----------------------------------------------------------------------------------------
// The command window is the last screen area below all enabled windows displayed. It
// is actually not a window like the others in that it represents lines written to the
// window as well as the command input line. It still has a window header and a line
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
    
    return ((int) strlen( cmdBuf ));
}

//----------------------------------------------------------------------------------------
// "removeChar" will removes the character from the input buffer left of the cursor 
// position and adjust the input buffer string size accordingly. If the cursor is at
// the end of the string, both string size and cursor position are decremented by one.
//
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
// "insertChar" will insert a character into the input buffer at the cursor position
// and adjust cursor and overall string size accordingly. There are two basic cases. 
// The first is simply appending to the buffer when both current string size and cursor
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
// Line sanitizing. We cannot just print out whatever is in the line buffer, since 
// it may contains dangerous escape sequences, which would garble our terminal screen
// layout. In the command window we just allow "safe" escape sequences, such as 
// changing the font color and so on. When we encounter an escape character followed 
// by a "[" character we scan the escape sequence until the final character, which 
// lies between 0x40 and 0x7E. Based on the last character, we distinguish between 
// "safe" and "unsafe" escape sequences. In the other cases, we just copy input to
// output.
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
    
    if ( count == MAX_CMD_HIST ) tail = ( tail + 1 ) % MAX_CMD_HIST;
    else count++;
    
    nextCmdNum ++;
    head = ( head + 1 ) % MAX_CMD_HIST;
}

//----------------------------------------------------------------------------------------
// Get a command line from the command history. If the command reference is negative, 
// the entry relative to the top is used. "head - 1" refers to the last entry entered.
// If the command ID is positive, we search for the entry with the matching command id,
// if still in the history buffer. Optionally, we return the absolute command Id.
//
//----------------------------------------------------------------------------------------
char *SimCmdHistory::getCmdLine( int cmdRef, int *cmdId ) {
    
    if (( cmdRef >= 0 ) && (( nextCmdNum - cmdRef ) > MAX_CMD_HIST ))
         return ( nullptr );

    if (( cmdRef < 0  ) && ( - cmdRef > nextCmdNum )) return ( nullptr );
    
    if ( count == 0 ) return ( nullptr );
    
    if ( cmdRef >= 0 ) {
        
        for ( int i = 0; i < count; i++ ) {
            
            int pos = ( tail + i ) % MAX_CMD_HIST;
            if ( history[ pos ].cmdId == cmdRef ) {
                
                if ( cmdId != nullptr ) *cmdId = history[ pos ].cmdId;
                return ( history[ pos ].cmdLine );
            }
        }
        
        return ( nullptr );
    }
    else {
        
        int pos = ( head + cmdRef + MAX_CMD_HIST ) % MAX_CMD_HIST;
        
        if (( pos < head ) && ( pos >= tail )) {
            
            if ( cmdId != nullptr ) *cmdId = history[ pos ].cmdId;
            return history[ pos ].cmdLine;
        }
        else return ( nullptr );
    }
}

//----------------------------------------------------------------------------------------
// The command history maintains a command counter and number, which we return here.
//
//----------------------------------------------------------------------------------------
int SimCmdHistory::getCmdNum( ) {
    
    return ( nextCmdNum );
}

int  SimCmdHistory::getCmdCount( ) {
    
    return ( count );
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
    
    setWinType( WT_CMD_WIN );
    setRadix( glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT ));
    setDefRows( 21 );
    setDefColumns( 100 );
    setRows( getDefRows( ));
    setColumns( getDefColumns( ));
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
                        glb -> console -> writeChars( "%c", cmdBuf[ cmdBufCursor ] );
                    }
                }
                else {
                   
                    if ( cmdBufLen < MAX_CMD_LINE_SIZE - 1 ) {
                       
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

    if ( glb -> winDisplay -> isWindowsOn( )) {

        if ( glb -> winDisplay -> isWinStackOn( )) 
            printTextField((char *) "WS", ( fmtDesc | FMT_LAST_FIELD ));
        else 
            printTextField((char *) "W", ( fmtDesc | FMT_LAST_FIELD ));
    }
}

//----------------------------------------------------------------------------------------
// The body lines of the command window are displayed after the banner line. The 
// window is filled from the output buffer. We first set the screen lines as the
// length of the command window may have changed.
//
// Rows to show is the number of lines between the header line and the last line,
// which is out command input line. We fill from the lowest line upward to the header
// line. Finally, we set the cursor to the last line in the command window.
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
// "commandLineError" is a little helper that prints out the error encountered.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::cmdLineError( SimErrMsgId errNum, char *argStr ) {
    
    for ( int i = 0; i < MAX_ERR_MSG_TAB; i++ ) {
        
        if ( errMsgTab[ i ].errNum == errNum ) {
            
            winOut -> writeChars( "%s\n", errMsgTab[ i ].errStr );
            return;
        }
    }
    
    winOut -> writeChars( "CmdLine Error: %d", errNum );
    if ( argStr != nullptr )  winOut -> writeChars( "%32s", argStr );
    winOut -> writeChars( "\n" );
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
    return ( ret );
}

//----------------------------------------------------------------------------------------
// A little helper function to ensure that windows are enabled.
//
//----------------------------------------------------------------------------------------
void SimCommandsWin::ensureWinModeOn( ) {

    if ( ! glb -> winDisplay -> isWindowsOn( )) throw( ERR_NOT_IN_WIN_MODE );
}

//----------------------------------------------------------------------------------------
// Return the current command token entered.
//
//----------------------------------------------------------------------------------------
SimTokId SimCommandsWin::getCurrentCmd( ) {
    
    return ( currentCmd );
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
            
        return ( snprintf( promptStr, promptStrLen,
                           "(%i) ->",
                           (int) glb -> env -> getEnvVarInt((char *) ENV_CMD_CNT )));
        }
    else return ( snprintf( promptStr, promptStrLen, "->" ));
}

//----------------------------------------------------------------------------------------
// Display absolute memory content. We will show the memory starting with offset. 
// The words per line is an environmental variable setting. The offset is rounded 
// down to the next 8-byte boundary, the limit is rounded up to the next 8-byte 
// boundary. We display the data in words.
//
//----------------------------------------------------------------------------------------
void  SimCommandsWin::displayAbsMemContent( T64Word ofs, T64Word len, int rdx ) {
    
    T64Word index        = rounddown( ofs, sizeof( T64Word ));
    T64Word limit        = roundup(( index + len ), sizeof( T64Word ));
    int     wordsPerLine = 4;

    while ( index < limit ) {

        winOut -> printNumber( index, FMT_HEX_2_4_4 );
        winOut -> writeChars( ": " );
        
        for ( uint32_t i = 0; i < wordsPerLine; i++ ) {
            
            if ( index < limit ) {

                T64Word val = 0;
                if ( glb -> system -> readMem( index, 
                                               (uint8_t *) &val, 
                                               sizeof( val ))) {

                    winOut -> printNumber( val, FMT_HEX_4_4_4_4 );
                    winOut -> writeChars( " " );   
                }
                else {

                    winOut -> printNumber( val, FMT_INVALID_NUM | FMT_HEX_4_4_4_4 );
                    winOut -> writeChars( " " );  
                }           
            }
            
            winOut -> writeChars( " " );
            index += sizeof( T64Word );
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
void  SimCommandsWin::displayAbsMemContentAsCode( T64Word adr, T64Word len ) {
    
    T64Word  index  = rounddown( adr, 4 );
    T64Word  limit  = roundup(( index + len ), 4 );
    T64Word  instr  = 0;
    char     buf[ MAX_TEXT_FIELD_LEN ];

    while ( index < limit ) {

        winOut -> printNumber( index << 2, FMT_HEX_2_4_4 );
        winOut -> writeChars( ": " );

        if ( glb -> system -> readMem( index, (uint8_t *) &instr, 4 )) {

            disAsm -> formatInstr( buf, sizeof( buf ), instr, 16 );
            winOut -> writeChars( "%s\n", buf ); 
        }
        else winOut -> writeChars( "******\n" );

        index += sizeof( uint32_t );
    }
    
    winOut -> writeChars( "\n" );
}

//----------------------------------------------------------------------------------------
// "execCmdsFromFile" will open a text file and interpret each line as a command. This
// routine is used by the "XF" command and also as the handler for the program argument
// option to execute a file before entering the command loop.
//
// XF "<file-path>"
//----------------------------------------------------------------------------------------
void SimCommandsWin::execCmdsFromFile( char* fileName ) {
    
    char cmdLineBuf[ MAX_CMD_LINE_SIZE ] = "";
    
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
//  HELP ( cmdId | ‘commands‘ | 'wcommands‘ | ‘wtypes‘ | ‘predefined‘ | 'regset' )
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
    
    if ( tok -> isToken( TOK_EOS )) {
        
        int exitVal = glb -> env -> getEnvVarInt((char *) ENV_EXIT_CODE );
        exit(( exitVal > 255 ) ? 255 : exitVal );
    }
    else {

        exit( eval -> acceptNumExpr( ERR_INVALID_EXIT_VAL, 0, 255 ));
    }
}

//----------------------------------------------------------------------------------------
// ENV command. The test driver has a few global environment variables for data 
// format, command count and so on. The ENV command list them all, one in particular
// and also modifies one if a value is specified. If the ENV variable dos not exist, 
// it will be allocated with the type of the value. A value of the token NIL will 
// remove a user defined variable.
//
//  ENV [ <var> [ <val> ]]
//
// ??? how do we remove a user environment variable ?
//----------------------------------------------------------------------------------------
void SimCommandsWin::envCmd( ) {
    
    SimEnv *env = glb -> env;
    
    if ( tok -> isToken( TOK_EOS )) {

        int hwm = env -> getEnvHwm( );
        if ( hwm > 0 ) {

            for ( int i = 0; i < hwm; i++ ) {

                char buf[ 128 ];
                int len = env -> formatEnvEntry( i, buf, sizeof( buf ));
                if ( len > 0 ) winOut -> writeChars( "%s\n", buf );
            }
        }
    }
    else if ( tok -> tokTyp( ) == TYP_IDENT ) {
        
        char envName[ MAX_ENV_NAME_SIZE ];
        
        strcpy( envName, tok -> tokName( ));
        upshiftStr( envName );
        
        tok -> nextToken( );
        if ( tok -> isToken( TOK_EOS )) {
            
            if ( env -> isValid( envName )) {

                char buf[ 128 ];
                int len = env -> formatEnvEntry( envName, buf, sizeof( buf ));
                if ( len > 0 ) winOut -> writeChars( "%s\n", buf );
            } 
            else throw ( ERR_ENV_VAR_NOT_FOUND );
        }
        else {
            
            SimExpr rExpr;
            eval -> parseExpr( &rExpr );
            
            if ( rExpr.typ == TYP_NUM )       env -> setEnvVar( envName, rExpr.u.val );
            else if ( rExpr.typ == TYP_BOOL ) env -> setEnvVar( envName, rExpr.u.bVal );
            else if ( rExpr.typ == TYP_STR )  env -> setEnvVar( envName, rExpr.u.str );
        }
    }
}

//----------------------------------------------------------------------------------------
// Execute commands from a file command. 
//
// XF "<filename>"
//----------------------------------------------------------------------------------------
void SimCommandsWin::execFileCmd( ) {
    
    if ( tok -> tokTyp( ) == TYP_STR )  execCmdsFromFile( tok -> tokStr( ));
    else                                throw( ERR_EXPECTED_FILE_NAME );
}

//----------------------------------------------------------------------------------------
// Load an ELF file command. 
//
// LF "<filename>"
//----------------------------------------------------------------------------------------
void SimCommandsWin::loadElfFileCmd( ) {
    
    if ( tok -> tokTyp( ) == TYP_STR )  loadElfFile( tok -> tokStr( ));
    else                                throw( ERR_EXPECTED_FILE_NAME );
}

//----------------------------------------------------------------------------------------
// Display Module Table command. The simulator features a system bus to which the 
// modules are plugged in. This command shows all known modules.
//
//  DM
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayModuleCmd( ) {

    winOut -> writeChars( "%-5s%-7s%-16s%-16s%-8s\n", 
                            "Mod", "Type", "HPA", "SPA", "Size" );

    for ( int i = 0; i < MAX_MOD_MAP_ENTRIES; i++ ) {

        T64Module *mPtr = glb -> system -> lookupByModNum( i );
        if ( mPtr != nullptr ) {

            winOut -> writeChars( "%02d   ", i  );

            winOut -> writeChars( "%-7s", mPtr -> getModuleTypeName( ));

            winOut -> printNumber( mPtr -> getHpaAdr( ), 
                                   FMT_PREFIX_0X | FMT_HEX_2_4_4 );
            winOut -> writeChars( "  " );

            if ( mPtr -> getSpaLen( ) > 0 ) {

                winOut -> printNumber( mPtr -> getSpaAdr( ), 
                                       FMT_PREFIX_0X | FMT_HEX_2_4_4 );
                winOut -> writeChars( "  " );

                winOut -> printNumber( mPtr -> getSpaLen( ), FMT_HEX_4_4 );
                winOut -> writeChars( "  " );
            }
            
            winOut -> writeChars( "\n" );
        }
    }

    tok -> checkEOS( );
}

//----------------------------------------------------------------------------------------
// Reset command.
//
//  RESET [ ( 'SYS' | 'STATS' ) ]
//
// ??? rethink what we want ... reset the SYSTEM ? all CPUs ?
// ??? do we want to reset a module ? Then we would not need MEM and CPU...
//----------------------------------------------------------------------------------------
void SimCommandsWin::resetCmd( ) {
    
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> system -> reset( );
    }
    else if ( tok -> isToken( TOK_SYS )) {

        throw( ERR_NOT_SUPPORTED );
    }
    else if ( tok -> isToken( TOK_STATS )) {
     
        throw( ERR_NOT_SUPPORTED );
    }
    else throw ( ERR_INVALID_ARG );
}

//----------------------------------------------------------------------------------------
// Run command. The command will just run the CPU until a "halt" instruction is 
// detected.
//
//  RUN
//----------------------------------------------------------------------------------------
void SimCommandsWin::runCmd( ) {
    
    winOut -> writeChars( "RUN command to come ... \n");
}

//----------------------------------------------------------------------------------------
// Step command. The command will advance all processors by one instruction. Default is
// step number is one instruction.
//
//  S [ <steps> ]
//
// ??? we need to handle the console window. It should be enabled before we pass control
// to the CPU. Make it the current window, saving the previous current window. Put the
// console mode into non-blocking and hand over to the CPU. On return from the CPU 
// steps, enable blocking mode again and restore the current window.
//----------------------------------------------------------------------------------------
void SimCommandsWin::stepCmd( ) {
    
    uint32_t numOfSteps = 1;
    
    if ( tok -> tokTyp( ) == TYP_NUM ) {

        numOfSteps = eval -> acceptNumExpr( ERR_EXPECTED_STEPS, 0, UINT32_MAX );
    }
    
    tok -> checkEOS( );
    glb -> system -> step( numOfSteps );
}

//----------------------------------------------------------------------------------------
// Write line command. We analyze the expression and print out the result.
//
//  W <expr> [ , <rdx> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::writeLineCmd( ) {
    
    SimExpr  rExpr;
    int      rdx;
    
    eval -> parseExpr( &rExpr );
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if (( tok -> isToken( TOK_HEX )) || ( tok -> isToken( TOK_DEC ))) {
            
            rdx = tok -> tokVal( );
            tok -> nextToken( );
        }
        else throw ( ERR_INVALID_FMT_OPT );
    }
    else rdx = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    tok -> checkEOS( );
    
    switch ( rExpr.typ ) {
            
        case TYP_BOOL: {
            
            if ( rExpr.u.bVal == true ) winOut -> writeChars( "TRUE\n" );
            else                        winOut -> writeChars( "FALSE\n" );
            
        } break;
            
        case TYP_NUM: {

            if ( rdx == 16 )   
                winOut -> printNumber( rExpr.u.val, FMT_HEX | FMT_PREFIX_0X );
            else if ( rdx == 10 )   
                winOut -> printNumber( rExpr.u.val, FMT_DEC );
            else                    
                winOut -> writeChars( "Invalid Radix" );
        
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
// ??? how about a negative depth shows relative numbers ?
//----------------------------------------------------------------------------------------
void SimCommandsWin::histCmd( ) {
    
    int     depth = 0;
    int     cmdCount = hist -> getCmdCount( );
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        depth = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, MAX_CMD_HIST );
    }
    
    if (( depth == 0 ) || ( depth > cmdCount )) depth = cmdCount;
    
    for ( int i = - depth; i < 0; i++ ) {
        
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

        cmdId = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, MAX_CMD_HIST );
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
        
        cmdId = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, MAX_CMD_HIST );
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
// Display absolute memory command. The offset address is a byte address, the length
// is measured in bytes, rounded up to the a word size. We accept any address and 
// length and only check that the offset plus length does not exceed the physical 
// address space. The format specifier will allow for HEX, DECIMAL and CODE. In the
// case of the code option, the default number format option is used for showing the
// offset value.
//
//  DA <ofs> [ "," <len> [ "," <fmt> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::displayAbsMemCmd( ) {
    
    int         rdx     = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    T64Word     ofs     = 0;
    T64Word     len     = sizeof( T64Word );
    bool        asCode  = false;

    ofs = eval -> acceptNumExpr( ERR_EXPECTED_START_OFS, 0, T64_MAX_PHYS_MEM_LIMIT );
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        if ( tok -> isToken( TOK_COMMA )) len = sizeof( T64Word );
        else len = eval -> acceptNumExpr( ERR_EXPECTED_LEN );
    }
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        switch(  tok -> tokId( )) {

            case TOK_HEX: 
            case TOK_DEC:   rdx = tok -> tokVal( );  break;
            
            case TOK_CODE:  asCode = true; break;
            
            case TOK_EOS: 
            default:        throw ( ERR_INVALID_FMT_OPT );
        }

        tok -> nextToken( );
    }
    
    tok -> checkEOS( );
    
    if (((T64Word) ofs + len ) <= T64_MAX_PHYS_MEM_LIMIT ) { 
        
        if ( asCode ) displayAbsMemContentAsCode( ofs, len );
        else          displayAbsMemContent( ofs, len, rdx );
    }
    else throw ( ERR_OFS_LEN_LIMIT_EXCEEDED );
}

//----------------------------------------------------------------------------------------
// Modify absolute memory command. 
//
//  MA <ofs> <val>
//----------------------------------------------------------------------------------------
void SimCommandsWin::modifyAbsMemCmd( ) {
    
    T64Word adr = eval -> acceptNumExpr( ERR_EXPECTED_OFS, 0, INT64_MAX );
    T64Word val = eval -> acceptNumExpr( ERR_INVALID_NUM );
    tok -> checkEOS( );

    if ( ! glb -> system -> writeMem( adr, (uint8_t *) &val, sizeof( T64Word ))) {

        throw( 9998 ); // ??? fix ...
    }
}

//----------------------------------------------------------------------------------------
// Modify register command. This command modifies a register within a register set.
// We must be in windows mode and the current window must be a CPU type window.
//
//  MR <reg> <val>
//----------------------------------------------------------------------------------------
void SimCommandsWin::modifyRegCmd( ) {
   
    SimTokTypeId    regSetId    = TYP_GREG;
    int             regNum      = 0;
    T64Word         val         = 0;
   
    ensureWinModeOn( );
    
    if (( tok -> tokTyp( ) == TYP_GREG ) ||
        ( tok -> tokTyp( ) == TYP_CREG ) ||
        ( tok -> tokTyp( ) == TYP_PREG )) {
        
        regSetId    = tok -> tokTyp( );
        regNum      = tok -> tokVal( );
        tok -> nextToken( );
    }
    else throw ( ERR_INVALID_REG_ID );
    
    val = eval -> acceptNumExpr( ERR_INVALID_NUM );

    tok -> checkEOS( );

    if ( glb -> winDisplay -> getCurrentWinType( ) != WT_CPU_WIN ) 
        throw( ERR_INVALID_WIN_TYPE );

    int modNum = glb -> winDisplay -> getCurrentWinModNum( );

    T64ModuleType mType = glb -> system -> getModuleType( modNum );
    if ( mType != MT_PROC ) throw ( ERR_INVALID_MODULE_TYPE );

    T64Processor *proc = (T64Processor *) glb -> system -> lookupByModNum( modNum );
    if ( proc == nullptr ) throw ( ERR_INVALID_MODULE_TYPE );

    switch( regSetId ) {

        case TYP_GREG:  proc -> getCpuPtr( ) -> setGeneralReg( regNum, val ); break;
        case TYP_CREG:  proc -> getCpuPtr( ) -> setControlReg( regNum, val ); break;

        case TYP_PREG:  {

            T64Word tmp = proc -> getCpuPtr( ) -> getPswReg( );
            
            if      ( regNum == 1 ) tmp = depositField( tmp, 0, 52, val );
            else if ( regNum == 2 ) tmp = depositField( tmp, 52, 12, val );

            proc -> getCpuPtr( ) -> setPswReg( tmp );     
            
        } break;

            
        default: throw( ERR_EXPECTED_REG_SET );
    }
}

//----------------------------------------------------------------------------------------
// Purges a cache line from the cache. We must be in windows mode and the current
// window must be a cache window. 
//
//  PICA <vAdr> 
//  PDCA <vAdr> 
//----------------------------------------------------------------------------------------
void SimCommandsWin::purgeCacheCmd( ) {
   
    ensureWinModeOn( );
    T64Word vAdr = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC ); 
    tok -> checkEOS( );

    if ( glb -> winDisplay -> getCurrentWinType( ) != WT_CACHE_WIN ) 
        throw( ERR_INVALID_WIN_TYPE );

    int modNum = glb -> winDisplay -> getCurrentWinModNum( );
   
    T64ModuleType mType = glb -> system -> getModuleType( modNum );
    if ( mType != MT_PROC ) throw ( ERR_INVALID_MODULE_TYPE );

    T64Processor *proc = (T64Processor *) glb -> system -> lookupByModNum( modNum );
    if ( proc == nullptr ) throw ( ERR_INVALID_MODULE_TYPE );
    
    if      ( currentCmd == CMD_PCA_I ) proc -> getICachePtr( ) -> purge( vAdr );
    else if ( currentCmd == CMD_PCA_D ) proc -> getDCachePtr( ) -> purge( vAdr );
    else throw( 9996 ); // ??? fix ...
}

//----------------------------------------------------------------------------------------
// Flushes a cache line from the data cache. We must be in windows mode and the
// current window must be a Cache window. 
//
//  FDCA <vAdr> 
//----------------------------------------------------------------------------------------
void SimCommandsWin::flushCacheCmd( ) {

    ensureWinModeOn( );
   
    T64Word vAdr = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC ); 
    tok -> checkEOS( );

    if ( glb -> winDisplay -> getCurrentWinType( ) != WT_CACHE_WIN ) 
        throw( ERR_INVALID_WIN_TYPE );

    int modNum = glb -> winDisplay -> getCurrentWinModNum( );
   
    T64ModuleType mType = glb -> system -> getModuleType( modNum );
    if ( mType != MT_PROC ) throw ( ERR_INVALID_MODULE_TYPE );

    T64Processor *proc = (T64Processor *) glb -> system -> lookupByModNum( modNum );

    proc -> getDCachePtr( ) -> flush( vAdr );
}

//----------------------------------------------------------------------------------------
// Insert into TLB command. We have two modes. We must be in windows mode and the 
// current window must be a TLB window. 
//
//  IITLB <vAdr> "," <pAdr> "," <size> "," <acc> "," <flags>
//  IDTLB <vAdr> "," <pAdr> "," <size> "," <acc> "," <flags>
//
// We could get the flags as an identifier string and parse the individual characters.
//----------------------------------------------------------------------------------------
void SimCommandsWin::insertTLBCmd( ) {

    ensureWinModeOn( );
    T64Word vAdr = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, T64_MAX_VIRT_MEM_LIMIT ); 
    tok -> acceptComma( );
    T64Word pAdr = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, T64_MAX_PHYS_MEM_LIMIT ); 
    tok -> acceptComma( );
    T64Word size = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, 15 );
    tok -> acceptComma( );
    T64Word acc = eval -> acceptNumExpr( ERR_INVALID_NUM, 0, 15 );
    tok -> acceptComma( );
    T64Word flags = eval -> acceptNumExpr( ERR_INVALID_NUM, 0 );
    tok -> checkEOS( );

    // ??? find a way to make to constants for the bits, which my change ...
    // ??? e.g. "buildTlbInfoWord" ...
    
    T64Word info = 0;    
    info = depositField( info, 56, 8, flags );
    info = depositField( info, 40, 4, acc );
    info = depositField( info, 36, 4, size );
    info = depositField( info, 12, 24, pAdr );

    if ( glb -> winDisplay -> getCurrentWinType( ) != WT_TLB_WIN ) 
        throw( ERR_INVALID_WIN_TYPE );

    int modNum = glb -> winDisplay -> getCurrentWinModNum( );
   
    T64ModuleType mType = glb -> system -> getModuleType( modNum );
    if ( mType != MT_PROC ) throw ( ERR_INVALID_MODULE_TYPE );

    T64Processor *proc = (T64Processor *) glb -> system -> lookupByModNum( modNum );

    if      ( currentCmd == CMD_ITLB_I ) proc -> getITlbPtr( ) -> insert( vAdr, info );
    else if ( currentCmd == CMD_ITLB_D ) proc -> getDTlbPtr( ) -> insert( vAdr, info );
    else throw( 9940 ); // ??? mark as internal error...
}

//----------------------------------------------------------------------------------------
// Purge from TLB command. We have two modes. We must be in windows mode and the 
// current window must be a TLB window. 
//
//  PITLB  <vAdr>
//  PDTLB  <vAdr>
//----------------------------------------------------------------------------------------
void SimCommandsWin::purgeTLBCmd( ) {

    ensureWinModeOn( );
    T64Word vAdr = eval -> acceptNumExpr( ERR_INVALID_NUM, 0 ); 
    tok -> checkEOS( );

    if ( glb -> winDisplay -> getCurrentWinType( ) != WT_TLB_WIN ) 
        throw( ERR_INVALID_WIN_TYPE );

    int modNum = glb -> winDisplay -> getCurrentWinModNum( );
    
    T64ModuleType mType = glb -> system -> getModuleType( modNum );
    if ( mType != MT_PROC ) throw ( ERR_INVALID_MODULE_TYPE );

    T64Processor *proc = (T64Processor *) glb -> system -> lookupByModNum( modNum );

    if      ( currentCmd == CMD_PTLB_I ) proc -> getITlbPtr( ) -> purge( vAdr );
    else if ( currentCmd == CMD_PTLB_D ) proc -> getDTlbPtr( ) -> purge( vAdr );
    else ;
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
  
    glb -> winDisplay -> windowsOff( );
}

void SimCommandsWin::winDefCmd( ) {

    glb -> winDisplay -> windowDefaults( );
    glb -> winDisplay -> setWinReFormat( );
}

void SimCommandsWin::winStacksEnable( ) {
    
    glb -> winDisplay -> winStacksEnable( true );
    glb -> winDisplay -> setWinReFormat( );
}

void SimCommandsWin::winStacksDisable( ) {
   
    glb -> winDisplay -> winStacksEnable( false );
    glb -> winDisplay -> setWinReFormat( );
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
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        winNum = eval -> acceptNumExpr( ERR_EXPECTED_WIN_ID, 1, MAX_WINDOWS );
    }

    tok -> checkEOS( );
    glb -> winDisplay -> windowEnable( winNum, true );
    glb -> winDisplay -> setWinReFormat( );
}

void SimCommandsWin::winDisableCmd( ) {
    
    int winNum = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {

        winNum = eval -> acceptNumExpr( ERR_EXPECTED_WIN_ID, 1, MAX_WINDOWS );
    }

    tok -> checkEOS( );
    glb -> winDisplay -> windowEnable( winNum, false );
    glb -> winDisplay -> setWinReFormat( );
}

//----------------------------------------------------------------------------------------
// Windows radix. This command sets the radix for a given window. We parse the command
// and the format option and pass the tokens to the screen handler. The window number
// is optional, used for user definable windows.
//
//  <win>R [ <radix> [ "," <winNum>]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetRadixCmd( ) {
    
    int     winNum  = 0;
    int     rdx     =  glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
    
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> winDisplay -> windowRadix( rdx, winNum );
        return;
    }
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        rdx = glb -> env -> getEnvVarInt((char *) ENV_RDX_DEFAULT );
        tok -> nextToken( );
    }
    else {
        
        if      ( tok -> isToken( TOK_DEC )) rdx = 10;
        else if ( tok -> isToken( TOK_HEX )) rdx = 16;
        else throw( ERR_INVALID_RADIX );
    }
    
    if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        winNum = eval -> acceptNumExpr( ERR_EXPECTED_WIN_ID, 1, MAX_WINDOWS );
    }

    tok -> checkEOS( );
    glb -> winDisplay -> windowRadix( rdx, winNum );
}

//----------------------------------------------------------------------------------------
// Window scrolling. This command advances the item address of a scrollable window 
// by the number of lines multiplied by the number of items on a line forward or 
// backward. The meaning of the item address and line items is window dependent. If 
// the amount is zero, the default value of the window will be used. The window number
// is optional, used for user definable windows. If omitted, we mean the current 
// window.
//
//  <win>F [ <amt> [ , <winNum> ]]
//  <win>B [ <amt> [ , <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winForwardCmd( ) {
  
    T64Word winItems    = 0;
    int     winNum      = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {
      
        winItems = eval -> acceptNumExpr( ERR_INVALID_NUM, 0 ); // ??? max
      
        if ( tok -> isToken( TOK_COMMA )) {
            
            tok -> nextToken( );
            winNum = eval -> acceptNumExpr( ERR_EXPECTED_WIN_ID, 1, MAX_WINDOWS );
        }
      
        tok -> checkEOS( );
    }

    glb -> winDisplay -> windowForward( winItems, winNum );
}

void SimCommandsWin::winBackwardCmd( ) {
   
    T64Word winItems    = 0;
    int     winNum      = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {
      
        winItems = eval -> acceptNumExpr( ERR_INVALID_NUM, 0 ); // ??? max
      
        if ( tok -> isToken( TOK_COMMA )) {
            
            tok -> nextToken( );
            winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
        }
      
        tok -> checkEOS( );
    }
    
    glb -> winDisplay -> windowBackward( winItems, winNum  );
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
   
    T64Word winPos  = 0;
    int     winNum  = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {
      
        winPos = eval -> acceptNumExpr( ERR_INVALID_NUM );
      
        if ( tok -> isToken( TOK_COMMA )) {
            
            tok -> nextToken( );
            winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
        }
      
        tok -> checkEOS( );
    }

    glb -> winDisplay -> windowHome( winPos, winNum  );
}

//----------------------------------------------------------------------------------------
// Window jump. The window jump command sets the item address to the position argument.
// The meaning of the item address is window dependent. The window number is optional,
// used for user definable windows.
//
//  <win>J [ <pos> [ "," <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winJumpCmd( ) {
   
    T64Word winPos  = 0;
    int     winNum  = 0;
    
    if ( tok -> tokId( ) != TOK_EOS ) {
      
        winPos = eval -> acceptNumExpr( ERR_INVALID_NUM );
      
        if ( tok -> isToken( TOK_COMMA )) {
            
            tok -> nextToken( );
            winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );

            if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
        }
      
        tok -> checkEOS( );
    }
    
    glb -> winDisplay -> windowJump( winPos, winNum );
}

//----------------------------------------------------------------------------------------
// Set window lines. This command sets the the number of rows for a window. The 
// number includes the banner line. If the "lines" argument is omitted, the window
// default value will be used. The window number is optional, used for user definable
// windows.
//
//  WL [ <lines> [ "," <winNum> ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetRowsCmd( ) {
   
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> winDisplay -> windowSetRows( 0, 0 );
    }
    else {

        int winLines = eval -> acceptNumExpr( ERR_INVALID_NUM );
        int winNum   = 0;
    
        if ( tok -> isToken( TOK_COMMA )) {
        
            tok -> nextToken( );
            winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );

            if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
        }

        tok -> checkEOS( );
        glb -> winDisplay -> windowSetRows( winLines, winNum );
        glb -> winDisplay -> setWinReFormat( );
    }    
}

//----------------------------------------------------------------------------------------
// Set command window lines. The command sets the the number of rows for the command
// window. The number includes the banner line. If the "lines" argument is omitted, 
// the window default value will be used. 
//
//  CWL [ <lines> ]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetCmdWinRowsCmd( ) {

    if ( tok -> isToken( TOK_EOS )) {

        glb -> winDisplay -> windowSetCmdWinRows( getDefRows( ));
    }
    else {

        int winLines = eval -> acceptNumExpr( ERR_INVALID_NUM, 0 ); // ??? max
        tok -> checkEOS( );
        glb -> winDisplay -> windowSetCmdWinRows( winLines );
    }
}

//----------------------------------------------------------------------------------------
// Window current command. User definable windows are controlled by their window 
// number. To avoid typing this number all the time for a user window command, a 
// user window can explicitly be set as the current command.
//
//  WC <winNum>
//----------------------------------------------------------------------------------------
void SimCommandsWin::winCurrentCmd( ) {
    
    if ( tok -> isToken( TOK_EOS )) throw ( ERR_EXPECTED_WIN_ID );

    int winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
    
    tok -> checkEOS( );
    glb -> winDisplay -> windowCurrent( winNum );
}

//----------------------------------------------------------------------------------------
// This command toggles through alternate window content, if supported by the window.
// An example is the cache sets in a two-way associative cache. The toggle command 
// will just flip through the sets.
//
//  WT [ <winNum> ]
//----------------------------------------------------------------------------------------
void  SimCommandsWin::winToggleCmd( ) {
    
    if ( tok -> isToken( TOK_EOS )) {
        
        glb -> winDisplay -> windowToggle( glb -> winDisplay -> getCurrentWindow( ));
    }
    else {

        int winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
        if ( ! glb -> winDisplay -> validWindowNum( winNum )) 
            throw ( ERR_INVALID_WIN_ID );
    
        glb -> winDisplay -> windowToggle( tok -> tokVal( ));
    }

    glb -> winDisplay -> setWinReFormat( );
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

    int winNum = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
    
    tok -> checkEOS( );
    
    if ( ! glb -> winDisplay -> validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    glb -> winDisplay -> windowExchangeOrder( winNum );
    glb -> winDisplay -> setWinReFormat( );
}

//----------------------------------------------------------------------------------------
// This command creates a new window. The window is assigned a free index from the
// windows list. This index is used in all the calls to this window. The window type
// is determined by the keyword plus additional info such as module and submodule 
// number. Note that we do not create simulator module objects, we merely attach
// a window to them. So they must exist. The general form of the command is:
//
//  WN <winType> [ "," <arg1> [ "," <arg2> ]]
//
//  WN  CPU     "," <proc>
//  WN  ICACHE  "," <proc>
//  WN  DCACHE  "," <proc>
//  WN  ITLB    "," <proc>
//  WN  DTLB    "," <proc>
//  WN  MEM     "," <adr>
//  WN  CODE    "," <adr>
//  WN  TEXT    "," <str>
// 
//----------------------------------------------------------------------------------------
void SimCommandsWin::winNewWinCmd( ) {
    
    SimTokId  winType = TOK_NIL;

    ensureWinModeOn( );
    winType = tok -> acceptTokSym( ERR_EXPECTED_WIN_ID );
 
    switch ( winType ) {

        case TOK_CPU: {

            tok -> acceptComma( );
            int modNum = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC );
            tok -> checkEOS( );

            glb -> winDisplay -> windowNewCpuState( modNum );  

        } break;

        case TOK_ITLB: {

            tok -> acceptComma( );
            int modNum = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC );
            tok -> checkEOS( );

            glb -> winDisplay -> windowNewTlb( modNum, T64_TK_INSTR_TLB );  

        } break;

        case TOK_DTLB: {

            tok -> acceptComma( );
            int modNum = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC );
            tok -> checkEOS( );

            glb -> winDisplay -> windowNewTlb( modNum, T64_TK_DATA_TLB );  

        } break;

        case TOK_ICACHE: {

            tok -> acceptComma( );
            int modNum = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC );
            tok -> checkEOS( );

            glb -> winDisplay -> windowNewCache( modNum, T64_CK_INSTR_CACHE );  

        } break;

        case TOK_DCACHE: {

            tok -> acceptComma( );
            int modNum = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC );
            tok -> checkEOS( );

            glb -> winDisplay -> windowNewCache( modNum, T64_CK_DATA_CACHE );  

        } break;

        case TOK_MEM: {

            tok -> acceptComma( );
            T64Word adr = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC, 
                                                 0,
                                                 T64_MAX_PHYS_MEM_LIMIT );
            tok -> checkEOS( );

            T64Module *mod = glb -> system -> lookupByAdr( adr );
            if ( mod != nullptr ) {

                glb -> winDisplay -> windowNewAbsMem( mod -> getModuleNum( ), adr );
            }
            else throw( 9997 );

        }  break;

        case TOK_CODE: {  

            tok -> acceptComma( );
            T64Word adr = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC, 
                                                 0,
                                                 T64_MAX_PHYS_MEM_LIMIT );
            tok -> checkEOS( );

             T64Module *mod = glb -> system -> lookupByAdr( adr );
            if ( mod != nullptr ) {

                glb -> winDisplay -> windowNewAbsCode( mod -> getModuleNum( ), adr );
            }
            else throw( 9997 );

        }  break;

        case TOK_TEXT: {

            tok -> acceptComma( );
        
            char *argStr = nullptr;
            if ( tok -> tokTyp( ) == TYP_STR ) argStr = tok -> tokStr( );
            else throw ( ERR_INVALID_ARG );

            tok -> nextToken( );
            tok -> checkEOS( );
            glb -> winDisplay -> windowNewText( argStr );

        } break;

        default: throw( ERR_INVALID_WIN_TYPE );
    }
    
    glb -> winDisplay -> setWinReFormat( );
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
    
    if ( tok -> isToken( TOK_EOS )) {
        
        winNumStart = glb -> winDisplay -> getCurrentWindow( );
        winNumEnd   = winNumStart;
    }
    else {

        winNumStart = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC, 0, MAX_WINDOWS );
    
        if ( winNumStart == -1 ) {
            
            winNumStart = 0;
            winNumEnd   = MAX_WINDOWS;
        }
        else {

            if ( tok -> isToken( TOK_COMMA )) {
            
                tok -> nextToken( );
                winNumEnd = eval -> acceptNumExpr( ERR_EXPECTED_NUMERIC, 0, MAX_WINDOWS );     
            }
        }

        if ( winNumStart > winNumEnd ) winNumEnd = winNumStart;
    }
    
    glb -> winDisplay -> windowKill( winNumStart, winNumEnd );
    glb -> winDisplay -> setWinReFormat( );
}

//----------------------------------------------------------------------------------------
// This command assigns a user window to a stack. User windows can be displayed in a
// separate stack of windows. The first stack is always the main stack, where the 
// predefined and command window can be found. Stacks are numbered from 1 to MAX.
//
//  WS <stackNum> [ , <winNumStart> [ , <winNumEnd ]]
//----------------------------------------------------------------------------------------
void SimCommandsWin::winSetStackCmd( ) {
    
    int     winStack    = 0;
    int     winNumStart = 0;
    int     winNumEnd   = 0;
    
    if ( ! glb -> winDisplay -> isWinModeOn( )) throw ( ERR_NOT_IN_WIN_MODE );

    winStack = eval -> acceptNumExpr( ERR_EXPECTED_STACK_ID, 1, MAX_WIN_STACKS );
    
    if ( tok -> isToken( TOK_EOS )) {
        
        winNumStart = glb -> winDisplay -> getCurrentWindow( );
        winNumEnd   = winNumStart;
    }
    else if ( tok -> isToken( TOK_COMMA )) {
        
        tok -> nextToken( );
        winNumStart = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
        
        if ( tok -> isToken( TOK_COMMA )) {
            
            tok -> nextToken( );
            winNumEnd = eval -> acceptNumExpr( ERR_INVALID_WIN_ID, 1, MAX_WINDOWS );
        }
        else winNumEnd = winNumStart;
    }
    else throw ( ERR_EXPECTED_COMMA );
   
    glb -> winDisplay -> windowSetStack( winStack, winNumStart, winNumEnd );
    glb -> winDisplay -> setWinReFormat( );
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
                    glb -> env -> 
                        setEnvVar((char *) ENV_CMD_CNT, (T64Word) hist -> getCmdNum( ));
                }
                
                switch( currentCmd ) {
                        
                    case TOK_NIL:                                           break;
                    case CMD_EXIT:          exitCmd( );                     break;
                        
                    case CMD_HELP:          helpCmd( );                     break;
                    case CMD_ENV:           envCmd( );                      break;
                    case CMD_XF:            execFileCmd( );                 break;
                    case CMD_LF:            loadElfFileCmd( );              break;
                        
                    case CMD_WRITE_LINE:    writeLineCmd( );                break;
                        
                    case CMD_HIST:          histCmd( );                     break;
                    case CMD_DO:            doCmd( );                       break;
                    case CMD_REDO:          redoCmd( );                     break;
                        
                    case CMD_RESET:         resetCmd( );                    break;
                    case CMD_RUN:           runCmd( );                      break;
                    case CMD_STEP:          stepCmd( );                     break;

                    case CMD_DM:            displayModuleCmd( );               break;
                        
                    case CMD_MR:            modifyRegCmd( );                break;
                        
                    case CMD_DA:            displayAbsMemCmd( );            break;
                    case CMD_MA:            modifyAbsMemCmd( );             break;
                        
                    case CMD_ITLB_I: 
                    case CMD_ITLB_D:        insertTLBCmd( );                break;

                    case CMD_PTLB_I:
                    case CMD_PTLB_D:        purgeTLBCmd( );                 break;
                        
                    case CMD_PCA_I:  
                    case CMD_PCA_D:         purgeCacheCmd( );               break;

                    case CMD_FCA_D:         flushCacheCmd( );               break;
                        
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
    
    char cmdLineBuf[ MAX_CMD_LINE_SIZE ];
    char cmdPrompt[ MAX_CMD_LINE_SIZE ];

    glb -> winDisplay -> setWinReFormat( );
    glb -> winDisplay -> reDraw( );
   
    printWelcome( );
    glb -> winDisplay -> setWinReFormat( );
    glb -> winDisplay -> reDraw( );
    
    while ( true ) {
        
        buildCmdPrompt( cmdPrompt, sizeof( cmdPrompt ));
        int cmdLen = readCmdLine( cmdLineBuf, 0, cmdPrompt );

        if ( cmdLen > 0 ) evalInputLine( cmdLineBuf );
        glb -> winDisplay -> reDraw( );
    }
}