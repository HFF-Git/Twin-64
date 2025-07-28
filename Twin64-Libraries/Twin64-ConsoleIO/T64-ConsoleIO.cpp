//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Monitor - Console IO
//
//----------------------------------------------------------------------------------------
// Console IO is the piece of code that provides a single character interface for the
// terminal screen. For the simulator, it is just plain character IO to the terminal 
// screen.For the simulator running in CPU mode, the characters are taken from and 
// place into the virtual console declared on the IO space.
//
// Unfortunately, PCs and Macs differ. The standard system calls typically buffer the
// input up to the carriage return. To avoid this, the terminal needs to be place in
// "raw" mode. And this is different for the two platforms.
//
//----------------------------------------------------------------------------------------
//
// // Twin64 - A 64-bit CPU Monitor - Console IO
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
#include "T64-ConsoleIO.h"

//----------------------------------------------------------------------------------------
// Local name space.
//
//----------------------------------------------------------------------------------------
namespace {

//----------------------------------------------------------------------------------------
// The previous terminal attribute settings. We restore them when the console IO object
// is deleted.
//
//----------------------------------------------------------------------------------------
#if __APPLE__
struct termios saveTermSetting;
#endif

char outputBuffer[ 1024 ];

#if 0
// ??? we move these routines to the simulator if needed ...
//----------------------------------------------------------------------------------------
// "removeChar" will remove a character from the input buffer at the cursor 
// position and adjust the string size accordingly. If the cursor is at the end
// of the string, both string size and cursor position are decremented by one, 
// otherwise the cursor stays where it is and just the string size is 
// decremented.
//
//----------------------------------------------------------------------------------------
void removeChar( char *buf, int *strSize, int *pos ) {
    
    if (( *strSize > 0 ) && ( *strSize == *pos )) {
        
        *strSize    = *strSize - 1;
        *pos        = *pos - 1;
    }
    else if (( *strSize > 0 ) && ( *pos >= 0 )) {
   
        for ( int i = *pos; i < *strSize; i++ ) buf[ i ] = buf[ i + 1 ];
        *strSize    = *strSize - 1;
    }
}

//----------------------------------------------------------------------------------------
// "insertChar" will insert a character in the input buffer at the cursor 
// position and adjust cursor and overall string size accordingly. There are 
// two basic cases. The first is simply appending to the buffer when both the 
// current string size and cursor position are equal. The second is when the 
// cursor is somewhere in the input buffer. In this case we need to shift the 
// characters to the right to make room first.
//
//----------------------------------------------------------------------------------------
void insertChar( char *buf, int ch, int *strSize, int *pos ) {
    
    if ( *pos == *strSize ) {
        
        buf[ *strSize ] = ch;
        *strSize        = *strSize + 1;
        *pos            = *pos + 1;
    }
    else if ( *pos < *strSize ) {
        
        for ( int i = *strSize; i > *pos; i-- ) buf[ i ] = buf[ i -1 ];
        
        buf[ *pos ] = ch;
        *strSize    = *strSize + 1;
        *pos        = *pos + 1;
    }
}

//----------------------------------------------------------------------------------------
// "appendChar" will add a character to the end of the buffer and adjust the 
// overall size.
//
//----------------------------------------------------------------------------------------
void appendChar( char *buf, int ch, int *strSize ) {
    
    buf[ *strSize ] = ch;
    *strSize        = *strSize + 1;
}
#endif

}; // namespace


//----------------------------------------------------------------------------------------
// Object constructor. We will save the current terminal settings.
//
//----------------------------------------------------------------------------------------
SimConsoleIO::SimConsoleIO( ) {
  
#if __APPLE__
    tcgetattr( fileno( stdin ), &saveTermSetting );
#endif
    
}

SimConsoleIO::~SimConsoleIO( ) {
    
#if __APPLE__
    tcsetattr( fileno( stdin ), TCSANOW, &saveTermSetting );
#endif
    
}

//----------------------------------------------------------------------------------------
// The Simulator works in raw character mode. This is to support basic editing features
// and IO to the simulator console window when the simulation is active. There is a 
// price to pay in that there is no nice buffering of input and basic line editing 
// capabilities. On Mac/Linux the terminal needs to be set into raw character mode. On 
// windows, this seems to work without special setups. Hmm, anyway. This routine will
// set the raw mode attributes. For a windows system, these methods are a no operation.
//
// There is also a non-blocking IO mode. When the simulator hands over control to the 
// CPU, the console IO is mapped to the PDC console driver and output is directed to
// the console window. The console IO becomes part of the periodic processing and a key
// pressed will set the flags in the PDC console driver data. We act as"true" hardware.
// Non-blocking mode is enabled on entry to single step and run command and disabled 
// when we are back to the monitor.
//
//
// ??? perhaps a place to save the previous settings and restore them ?
// ??? Or just do all this in the object creator ?
//----------------------------------------------------------------------------------------
void SimConsoleIO::initConsoleIO( ) {
    
#if __APPLE__
    struct termios term;
    tcgetattr( fileno( stdin ), &term );
    term.c_lflag &= ~ ( ICANON | ECHO );
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr( STDIN_FILENO, TCSANOW, &term );
#endif
    
    blockingMode  = true;
}

//----------------------------------------------------------------------------------------
// "isConsole" is used by the command interpreter to figure whether we have a true 
// terminal or just read from a file.
//
//----------------------------------------------------------------------------------------
bool  SimConsoleIO::isConsole( ) {
    
    return( isatty( fileno( stdin )));
}

//----------------------------------------------------------------------------------------
// "setBlockingMode" will put the terminal into blocking or non-blocking mode. For the
// command interpreter we will use the blocking mode, i.e. we wait for character input.
// When the CPU runs, the console IO must be in non-blocking, and we check for input on
// each CPU "tick".
//
//----------------------------------------------------------------------------------------
void SimConsoleIO::setBlockingMode( bool enabled ) {
    
#if __APPLE__
    
    int flags = fcntl( STDIN_FILENO, F_GETFL, 0 );
    if ( flags == -1 ) {
        
        // ??? error ....
    }
    else {
        
        if ( enabled )  flags &= ~O_NONBLOCK;
        else            flags |= O_NONBLOCK;
        
        if ( fcntl( STDIN_FILENO, F_SETFL, flags ) == -1 ) {
               
            // ??? error ....
        }
    }
#endif
    
    blockingMode = enabled;
}

//----------------------------------------------------------------------------------------
// "readConsoleChar" is the single entry point to get a character from the terminal
// input. On Mac/Linux, this is the "read" system call. Whether the mode is blocking or
// non-blocking is set in the terminal settings. The read function is the same. If 
// there is no character available, a zero is returned, otherwise the character.
//
// On Windows there is a similar call, which does just return one character at a time. 
// However, there seems to be no real waiting function. Instead, the "_kbhit" tests for
// a keyboard input. In blocking mode, we will loop for a keyboard input and then get
// the character. In non-blocking mode, we test the keyboard and return either the 
// character typed or a zero.
//
// ??? this also means on Windows a "busy" loop..... :-(
// ??? perhaps a "sleep" eases the busy loop a little...
//----------------------------------------------------------------------------------------
int SimConsoleIO::readChar( ) {
    
#if __APPLE__
    char ch;
    if ( read( STDIN_FILENO, &ch, 1 ) == 1 ) return( ch );
    else return ( 0 );
#else
    if ( blockingMode ) {
        
        while ( ! _kbhit( )) Sleep( 50 );
        return( _getch( ));
    }
    else {
        
        if ( _kbhit( )) {
        
            int ch = _getch();
            return ( ch );
        }
        else return( 0 );
    }
#endif
    
}

//----------------------------------------------------------------------------------------
// "writeChar" is the single entry point to write to the terminal. On Mac/Linux, this
// is the "write" system call. On windows there is a similar call, which does just 
// prints one character at a time.
//
//----------------------------------------------------------------------------------------
int SimConsoleIO::writeChar( char ch  ) {
    
#if __APPLE__
    write( STDOUT_FILENO, &ch, 1 );
    return( 1 );
#else
    _putch( int(ch) );
    return( 1 );
#endif
    
}

int SimConsoleIO::writeChars( const char *format, ... ) {
    
    va_list args;
    
    va_start( args, format );
    int len = vsnprintf( outputBuffer, sizeof( outputBuffer ), format, args );
    va_end(args);
    
    if ( len > 0 ) {
        
        for ( int i = 0; i < len; i++  ) writeChar( outputBuffer[ i ] );
    }
    
    return( len );
}


//****************************************************************************************
//****************************************************************************************
//
// Console IO Formatter routines.
//
//----------------------------------------------------------------------------------------
// The Simulator features two distinct ways of text output. The first is the console
// I/O where a character will make its way directly to the terminal screen of the 
// application. The second output mode is a buffered I/O where all characters are 
// placed in an output buffer consisting of lines of text. When the command or console
// window is drawn, the text is taken form the this output buffer. Common to both
// output methods is the formatter.
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Escape code functions.
//
//----------------------------------------------------------------------------------------
void SimFormatter::eraseChar( ) {
    
    writeChars( "\033[D \033[P" );
}

void SimFormatter::writeCursorLeft( ) {
    
    writeChars( "\033[D" );
}

void SimFormatter::writeCursorRight( ) {
    
    writeChars( "\033[C" );
}

void SimFormatter::writeScrollUp( int n ) {
    
    writeChars( "\033[%dS", n );
}

void SimFormatter::writeScrollDown( int n ) {
    
    writeChars( "\033[%dT", n );
}

void SimFormatter::writeCarriageReturn( ) {

    #if __APPLE__
        writeChars( "\n" );
    #else 
        writeChars( "\r\n" );
    #endif
}

void SimFormatter::writeCharAtLinePos( int ch, int pos ) {
    
    writeChars( "\033[%dG\033[1@%c", pos, ch );
}

void SimFormatter::clearScreen( ) {
    
    writeChars((char *) "\x1b[2J" );
    writeChars((char *) "\x1b[3J" );
}

void SimFormatter::clearLine( ) {
    
    writeChars((char *) "\x1b[2K" );
}

void SimFormatter::setAbsCursor( int row, int col ) {
    
    writeChars((char *) "\x1b[%d;%dH", row, col );
}

void SimFormatter::setWindowSize( int row, int col ) {
    
    writeChars((char *) "\x1b[8;%d;%dt", row, col );
}

void SimFormatter::setScrollArea( int start, int end ) {
    
    writeChars((char *) "\x1b[%d;%dr", start, end );
}

void SimFormatter::clearScrollArea( ) {
    
    writeChars((char *) "\x1b[r" );
}

//----------------------------------------------------------------------------------------
// Console output is also used to print out window forms. A window will consist of lines
// with lines having fields on them. A field has a set of attributes such as foreground
// and background colors, bold characters and so on. This routine sets the attributes 
// based on the format descriptor. If the descriptor is zero, we will just stay where 
// are with their attributes.
//
//----------------------------------------------------------------------------------------
void SimFormatter::setFmtAttributes( uint32_t fmtDesc ) {
    
    if ( fmtDesc != 0 ) {
        
        writeChars((char *) "\x1b[0m" );
        if ( fmtDesc & FMT_INVERSE ) writeChars((char *) "\x1b[7m" );
        if ( fmtDesc & FMT_BLINK )   writeChars((char *) "\x1b[5m" );
        if ( fmtDesc & FMT_BOLD )    writeChars((char *) "\x1b[1m" );
        
        switch ( fmtDesc & 0xF ) { // BG Color
                
            case 1:     writeChars((char *) "\x1b[41m"); break;
            case 2:     writeChars((char *) "\x1b[42m"); break;
            case 3:     writeChars((char *) "\x1b[43m"); break;
            default:    writeChars((char *) "\x1b[49m");
        }
        
        switch (( fmtDesc >> 4 ) & 0xF ) { // FG Color
                
            case 1:     writeChars((char *) "\x1b[31m"); break;
            case 2:     writeChars((char *) "\x1b[32m"); break;
            case 3:     writeChars((char *) "\x1b[33m"); break;
            default:    writeChars((char *) "\x1b[39m");
        }
    }
}

//----------------------------------------------------------------------------------------
// Just emit blanks.
//
//----------------------------------------------------------------------------------------
int SimFormatter::printBlanks( int len ) {

    for ( int i = 0; i < len; i++ ) writeChar( ' ' );
    return( len );
}

//----------------------------------------------------------------------------------------
// Routine for putting out simple text. We make sure that the string length is in the
// range of what the text size could be.
//
//----------------------------------------------------------------------------------------
int SimFormatter::printText( char *text, int maxLen ) {
    
    if ( strlen( text ) <= maxLen ) {
        
        return( writeChars((char *) "%s", text ));
    }
    else {
     
        if ( maxLen > 4 ) {

            for ( int i = 0; i < maxLen - 3; i++  ) writeChar( text[ i ] );
            writeChars((char *) "..." );

            return( maxLen );
        }
        else {

            for ( int i = 0; i < maxLen; i++  ) writeChar( '.' );
            return( maxLen );
        }
    }
}

//----------------------------------------------------------------------------------------
// "printNumber" will print the number in the selected format. There quite a few HEX
// format to ease the printing of large numbers as we have in 64-bit system. If the 
// "invalid number" option is set in addition to the number format, the format is filled 
// with asterisks instead of numbers.
//
//----------------------------------------------------------------------------------------
int SimFormatter::printNumber( T64Word val, uint32_t fmtDesc ) {

    switch (( fmtDesc >> 8 ) & 0xF ) {

        case 1: { // HEX_2

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x""**" ));
            else
                return ( writeChars((char *) "0x%2x", val & 0xFF ));

        } break;

        case 2: { // HEX_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x""****" ));
            else
                return ( writeChars((char *) "0x%4x", val & 0xFFFF ));

        } break;

        case 3: { // HEX_8

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x""****""****" ));
            else
                return ( writeChars((char *) "0x%8x", val & 0xFFFFFFFF ));

        } break;

        case 4: { // HEX_16

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x""****""****""****""****" ));
            else
                return ( writeChars((char *) "0x%16x", val ));

        } break;

        case 5: { // FMT_HEX_2_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x**_****" ));
            else
                return ( writeChars((char *) "0x%02x_%04x", 
                                    (( val >> 4 ) & 0xFF   ),
                                    (( val      ) & 0xFFFF )));

        } break;

        case 6: { // FMT_HEX_4_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x****_****" ));
            else
                return ( writeChars((char *) "0x%04x_%04x", 
                                    (( val >> 4 ) & 0xFFFF ),
                                    (( val      ) & 0xFFFF )));

        } break;

        case 7: { // FMT_HEX_2_4_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x**_****_****" ));
            else
                return ( writeChars((char *) "0x%02x_%04x_%04x", 
                                    (( val >> 8 ) & 0xFF   ),
                                    (( val >> 4 ) & 0xFFFF ),
                                    (( val      ) & 0xFFFF )));

        } break;

        case 8: { // FMT_HEX_4_4_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x****_****_****" ));
            else
                return ( writeChars((char *) "0x%04x_%04x_%04x", 
                                    (( val >> 8 ) & 0xFFFF ),
                                    (( val >> 4 ) & 0xFFFF ),
                                    (( val      ) & 0xFFFF )));

        } break;

        case 9: { // FMT_HEX_2_4_4_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x**_****_****_****" ));
            else
                return ( writeChars((char *) "0x%02X_%04x_%04x_%04x", 
                                    (( val >> 12 ) & 0xFF   ),
                                    (( val >> 8  ) & 0xFFFF ),
                                    (( val >> 4  ) & 0xFFFF ),
                                    (( val       ) & 0xFFFF )));

        } break;

        case 10: { // FMT_HEX_4_4_4_4

            if ( fmtDesc & FMT_INVALID_NUM ) 
                return ( writeChars((char *) "0x****_****_****_****" ));
            else
                return ( writeChars((char *) "0x%04x_%04x_%04x_%04x", 
                                    (( val >> 12 ) & 0xFFFF ),
                                    (( val >> 8  ) & 0xFFFF ),
                                    (( val >> 4  ) & 0xFFFF ),
                                    (( val       ) & 0xFFFF )));

        } break;

        case 11: { // HEX as is

            return ( writeChars((char *) "0x%x", val));

        } break;

        case 12: { // DEC as is

            return ( writeChars((char *) "%d", val));

        } break;

        default: return ( writeChars ((char *) "*NN*" ));
    }
}

//----------------------------------------------------------------------------------------
// The window system sometimes prints numbers in a field with a given length. This
// routine returns based on value and format the necessary field length.
//
//----------------------------------------------------------------------------------------
int SimFormatter::numberFmtLen( T64Word val, uint32_t fmtDesc ) {

    switch (( fmtDesc >> 8 ) & 0xF ) {

        case 1:     return ( 4  );   // HEX_2
        case 2:     return ( 6  );   // HEX_4
        case 3:     return ( 10 );   // HEX_8
        case 4:     return ( 18 );   // HEX_16
        case 5:     return ( 9  );   // FMT_HEX_2_4
        case 6:     return ( 11 );   // FMT_HEX_4_4
        case 7:     return ( 14 );   // FMT_HEX_2_4_4
        case 8:     return ( 16 );   // FMT_HEX_4_4_4
        case 9:     return ( 19 );   // FMT_HEX_2_4_4_4
        case 10:    return ( 21 );   // FMT_HEX_4_4_4_4

        case 11: {

            int len = 3;

            val = abs( val );
            
            while ( val & 0xF ) {

                val >>= 4;
                len ++;
            }
            
            return( len );

        } break;

        case 12: {

            int len = (( val < 0 ) ? 2 : 1 );

            val = abs( val );
            
            while ( val >= 10 ) {

                val /= 10;
                len ++;
            }
            
            return( len );

        } break;

        default: return( 0 );
    }
}
