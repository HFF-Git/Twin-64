//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator window subsystem
//
//----------------------------------------------------------------------------------------
// This module contains the window display routines. The window subsystem uses a ton
// of escape sequences to create a terminal window screen and displays sub windows on
// the screen.
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU - Simulator window subsystem
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
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"

//----------------------------------------------------------------------------------------
// Local name space. We try to keep utility functions local to the file. None so far.
//
//----------------------------------------------------------------------------------------
namespace {

    bool isWinScrollable ( SimWinType typ ) {

        return (( typ == WT_MEM_WIN     ) ||
                ( typ == WT_CODE_WIN    ) ||
                ( typ == WT_TLB_WIN     ) ||
                ( typ == WT_CACHE_WIN   ) ||
                ( typ == WT_TEXT_WIN    ));  
    }

}; // namespace

//----------------------------------------------------------------------------------------
// Object constructor. We initialize the windows list and create all the predefined
// windows. The remainder of the window list is used by the user defined windows.
//
//----------------------------------------------------------------------------------------
SimWinDisplay::SimWinDisplay( SimGlobals *glb ) {
    
    this -> glb = glb;
    
    for ( int i = 0; i < MAX_WINDOWS; i++ ) windowList[ i ] = nullptr;
    cmdWin = new SimCommandsWin( glb );
}

//----------------------------------------------------------------------------------------
// Get the window display system and command interpreter ready. One day we will handle
// command line arguments.... 
//
//  -v           verbose
//  -i <path>    init file
//
// ??? to do ...
//----------------------------------------------------------------------------------------
void SimWinDisplay::setupWinDisplay( int argc, const char *argv[ ] ) {
    
    while ( argc > 0 ) {
        
        argc --;
    }
    
    glb -> winDisplay  -> windowDefaults( );
}

//----------------------------------------------------------------------------------------
// Start the window display. We start in screen mode and print the initial screen. 
// All left to do is to enter the command loop.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::startWinDisplay( ) {
    
    reDraw( true );
    cmdWin -> cmdInterpreterLoop( );
}

// ??? a bit ugly.... a better way to get to this data ?
// how about an ENV with the current command Id ?
SimTokId SimWinDisplay::getCurrentCmd( ) {
    
    return( cmdWin -> getCurrentCmd( ));
}

bool SimWinDisplay::isWinModeOn( ) {
    
    return( winModeOn );
}

void SimWinDisplay::setWinMode( bool winOn ) {
    
    winModeOn = winOn;
}

//----------------------------------------------------------------------------------------
// The current window number defines which user window is marked "current" as the 
// default number to use in commands. Besides getting and setting the current window
// number, there are also routines that return the window type and associated
// module number.
//
//----------------------------------------------------------------------------------------
int  SimWinDisplay::getCurrentWindow( ) {
    
    return( currentWinNum );
}

void SimWinDisplay::setCurrentWindow( int winNum ) {

    if ( validWindowNum( winNum )) currentWinNum = winNum;
    else throw( ERR_INVALID_WIN_ID );
}

SimWinType SimWinDisplay::getCurrentWinType( ) {

    if ( validWindowNum( currentWinNum )) 
        return( windowList[ currentWinNum - 1 ] -> getWinType( ));
    else throw( ERR_INVALID_WIN_ID );
}

int SimWinDisplay::getCurrentWinModNum( ) {

     if ( validWindowNum( currentWinNum )) 
        return( windowList[ currentWinNum - 1 ] -> getWinModNum( ));
     else throw( ERR_INVALID_WIN_ID );
}

//----------------------------------------------------------------------------------------
// Attribute functions on window Id, stack and type. A window number is the index into
// the window list. It is valid when the number is within bounds and the window list 
// entry is actually used. Window numbers start at 1.
//
//----------------------------------------------------------------------------------------
bool SimWinDisplay::validWindowNum( int winNum ) {
    
    return( ( winNum >= 1 ) && 
            ( winNum <= MAX_WINDOWS ) && 
            ( windowList[ winNum - 1 ] != nullptr ));
}

bool SimWinDisplay::validWindowStackNum( int stackNum ) {
    
    return(( stackNum >= 1 ) && ( stackNum < MAX_WIN_STACKS ));
}

bool SimWinDisplay::validWindowType( SimTokId winType ) {
    
    return( ( winType == TOK_PROC   ) ||
            ( winType == TOK_MEM    ) || 
            ( winType == TOK_ITLB   ) ||
            ( winType == TOK_DTLB   ) ||
            ( winType == TOK_ICACHE ) ||
            ( winType == TOK_DCACHE ) ||
            ( winType == TOK_CODE   ) || 
            ( winType == TOK_TEXT   ));
}

bool SimWinDisplay::isCurrentWin( int winNum ) {
    
    return(( validWindowNum( winNum ) && ( currentWinNum == winNum )));
}

bool SimWinDisplay::isWinEnabled( int winNum ) {

    if ( winNum == 0 ) winNum = getCurrentWindow( );

    return(( validWindowNum( winNum )) && ( windowList[ winNum ] -> isEnabled( )));
}

bool SimWinDisplay::isWindowsOn( ) {

    return( winModeOn );
}

bool SimWinDisplay::isWinStackOn( ) {

    return( winStacksOn );
}

//----------------------------------------------------------------------------------------
// Before drawing the screen content after the execution of a command line, we need 
// to check whether the number of columns needed for a stack of windows has changed. 
// This function just runs through the window list for a given stack and determines 
// the widest column needed for that stack. When no window is enabled the column size
// will be set to the command window default size.
//
//----------------------------------------------------------------------------------------
int SimWinDisplay::computeColumnsNeeded( int winStack ) {
    
    int columnSize = 0;
    
    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if (( windowList[ i ] != nullptr ) &&
            ( windowList[ i ] -> isEnabled( )) &&
            ( windowList[ i ] -> getWinStack( ) == winStack )) {
            
            int columns = windowList[ i ] -> getDefColumns( );
            if ( columns > columnSize ) columnSize = columns;
        }
    }
    
    return( columnSize );
}

//----------------------------------------------------------------------------------------
// Once we know the maximum column size needed for the active windows in a stack, we 
// need to set this size in all those windows, so that they print nicely with a common 
// end of line picture.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::setWindowColumns( int winStack, int columnSize ) {
    
    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if (( windowList[ i ] != nullptr ) &&
            ( windowList[ i ] -> isEnabled( )) &&
            ( windowList[ i ] -> getWinStack( ) == winStack )) {
            
            windowList[ i ] -> setColumns( columnSize );
        }
    }
}

//----------------------------------------------------------------------------------------
// Before drawing the screen content after the execution of a command line, we need to
// check whether the number of rows needed for a stack of windows has changed. This 
// function just runs through the window list and sums up the rows needed for a given
// stack.
//
//----------------------------------------------------------------------------------------
int SimWinDisplay::computeRowsNeeded( int winStack ) {
    
    int rowSize = 0;
    
    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if (( windowList[ i ] != nullptr ) &&
            ( windowList[ i ] -> isEnabled( )) &&
            ( windowList[ i ] -> getWinStack( ) == winStack )) {
            
            rowSize += windowList[ i ] -> getRows( );
        }
    }
    
    return( rowSize );
}

//----------------------------------------------------------------------------------------
// Content for each window is addressed in a window relative way. For this scheme to
// work, each window needs to know the absolute position within in the overall screen. 
// This routine will compute for each window of the passed stack the absolute row and 
// column position for the window in the terminal screen.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::setWindowOrigins( int winStack, int rowOffset, int colOffset ) {
    
    int tmpRow = rowOffset;
    
    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if (( windowList[ i ] != nullptr ) &&
            ( windowList[ i ] -> isEnabled( )) &&
            ( windowList[ i ] -> getWinStack( ) == winStack )) {
            
            windowList[ i ] -> setWinOrigin( tmpRow, colOffset );
            tmpRow += windowList[ i ] -> getRows( );
        }
        
        cmdWin -> setWinOrigin( tmpRow, colOffset );
    }
}

//----------------------------------------------------------------------------------------
// Window screen drawing. This routine is perhaps the heart of the window system. Each
// time we read in a command input, the terminal screen must be updated. A terminal 
// screen consist of a list of stacks and in each stack a list of windows. There is 
// always the main stack, stack Id 0. Only if we have user defined windows assigned to
// another stack and window stacks are enabled, will this stack show up in the terminal
// screen. If window stacks are disabled, all windows, regardless what their stack ID 
// says, will show up in the main stack and shown in their stack set when stack 
// displaying is enabled again.
//
// We first compute the number of rows and columns needed for all windows to show in 
// their assigned stack. Only enabled screens will participate in the overall screen
// size computation. The number of columns required is the sum of the columns a stack
// needs plus a margin between the stacks. Within a stack, the window with the largest
// columns needed determines the stack column size. Rows are determined by adding the 
// required rows of all windows in a given stack. The final number is the rows needed 
// by the largest stack plus the rows needed for the command window. The data is used 
// then to set the window columns of a window in the respective stack to the computed 
// columns size and to set the absolute origin coordinates of each window.
//
// The overall screen size is at least the numbers computed. If the number of rows 
// needed for the windows and command window is less than the defined minimum number of
// rows, the command window is enlarged to have a screen of minimum row size. When the
// screen size changed, we just redraw the screen with the command screen going last. 
// The command screen will have a columns size across all visible stacks.
//
// ??? sometimes the gap between the stacks has stale characters...
// ??? the data sees to be coming from about the middle of the first stack column ?
//----------------------------------------------------------------------------------------
void SimWinDisplay::reDraw( bool mustRedraw ) {
    
    int winStackColumns[ MAX_WIN_STACKS ]   = { 0 };
    int winStackRows[ MAX_WIN_STACKS ]      = { 0 };
    int maxRowsNeeded                       = 0;
    int maxColumnsNeeded                    = 0;
    int stackColumnGap                      = 2;
     int minRowSize = glb -> env -> getEnvVarInt((char *) ENV_WIN_MIN_ROWS );
    
    if ( winModeOn ) {
       
        for ( int i = 0; i < MAX_WIN_STACKS; i++ ) {
            
            winStackColumns[ i ] = computeColumnsNeeded( i + 1 );
            winStackRows[ i ]    = computeRowsNeeded( i + 1 );
            
            if ( winStacksOn ) {
                
                if ( winStackColumns[ i ] > 0 ) {

                    maxColumnsNeeded += winStackColumns[ i ];
                    if ( i > 0 ) maxColumnsNeeded += stackColumnGap;
                }
                
                if ( winStackRows[ i ] > maxRowsNeeded ) 
                    maxRowsNeeded = winStackRows[ i ];
            }
            else {
                
                if ( winStackColumns[ i ] > maxColumnsNeeded ) 
                    maxColumnsNeeded = winStackColumns[ i ];

                maxRowsNeeded += winStackRows[ i ];
            }
        }
        
        int curColumn = 1;
        int curRows   = 1;
        
        for ( int i = 0; i < MAX_WIN_STACKS; i++ ) {
            
            setWindowColumns( i + 1, winStackColumns[ i ] );
            setWindowOrigins( i + 1, curRows, curColumn );
            
            if ( winStacksOn ) {
                
                curColumn += winStackColumns[ i ];
                if ( winStackColumns[ i ] > 0 ) curColumn += stackColumnGap;
            }
            else {
                
                setWindowColumns( i, maxColumnsNeeded );
                curRows += winStackRows[ i ];
            }
        }
        
        if (( maxRowsNeeded + cmdWin -> getRows( )) < minRowSize ) {
            
            cmdWin -> setRows( minRowSize - maxRowsNeeded );
            maxRowsNeeded += cmdWin -> getRows();
        }
        else maxRowsNeeded += cmdWin -> getRows( );
        
        if ( maxColumnsNeeded == 0 ) {

            maxColumnsNeeded = cmdWin -> getDefColumns( );
            if ( winStacksOn ) maxColumnsNeeded += stackColumnGap;
        }
       
        cmdWin -> setColumns( maxColumnsNeeded ); 
        cmdWin -> setWinOrigin( maxRowsNeeded - cmdWin -> getRows( ) + 1, 1 );
    }
    else {
        
        maxRowsNeeded       = cmdWin -> getRows( );
        maxColumnsNeeded    = cmdWin -> getDefColumns( );
        
        cmdWin -> setWinOrigin( 1, 1 );
    }
   
    if ( mustRedraw ) {
       
        glb -> console -> setWindowSize( maxRowsNeeded, maxColumnsNeeded );
        glb -> console -> setAbsCursor( 1, 1 );
        glb -> console -> clearScrollArea( );
        glb -> console -> clearScreen( );
        
        if ( winModeOn )
            glb -> console -> setScrollArea( maxRowsNeeded - 1, maxRowsNeeded );
        else
            glb -> console -> setScrollArea( 2, maxRowsNeeded );
    }
    
    if ( winModeOn ) {

        for ( int i = 0; i < MAX_WINDOWS; i++ ) {
            
            if (( windowList[ i ] != nullptr ) && ( windowList[ i ] -> isEnabled( ))) {
                
                windowList[ i ] -> reDraw( );
            }
        }
    }
    
    cmdWin -> reDraw( );
    glb -> console -> setAbsCursor( maxRowsNeeded, 1 );
}

//----------------------------------------------------------------------------------------
// The entry point to showing windows is to draw the screen on the "windows on" command
// and to clean up when we switch back to line mode. The window defaults method will set
// the windows to a preconfigured default value. This is quite useful when we messed up
// our screens. Also, if the screen is displayed garbled after some windows mouse based
// screen window changes, just do WON again to set it straight. There is also a function
// to enable or disable the window stacks feature.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowsOn( ) {

    winModeOn = true;
    reDraw( true );
}

void SimWinDisplay::windowsOff( ) {

    if ( ! winModeOn ) throw ( ERR_NOT_IN_WIN_MODE );
    
    winModeOn = false;
    glb -> console -> clearScrollArea( );
    glb -> console -> clearScreen( );
    
    cmdWin -> setDefaults( );
    reDraw( true );
}

void SimWinDisplay::windowDefaults( ) {

    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if ( windowList[ i ] != nullptr ) windowList[ i ] -> setDefaults( );
    }
    
    cmdWin -> setDefaults( );
}

void SimWinDisplay::winStacksEnable( bool arg ) {

    if ( ! winModeOn ) throw ( ERR_NOT_IN_WIN_MODE );
        
    winStacksOn = arg;
}

//----------------------------------------------------------------------------------------
// A user defined window can be set to be the current user window. Commands that allow
// to specify a window number will use the window set by default then. Note that each
// user defined command that specifies the window number in its command will also set
// the current value. The user window becomes the actual window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowCurrent( int winNum ) {
    
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    currentWinNum = winNum; 
}

//----------------------------------------------------------------------------------------
// The routine sets the stack attribute for a user window. The setting is not allowed
// for the predefined window. They are always in the main window stack, which has the
// stack Id of one. Theoretically we could have many stacks, numbered 1 to MAX_STACKS. 
// Realistically, 3 to 4 stacks will fit on a screen. The last window moved is made the
// current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowSetStack( int winStack, int winNumStart, int winNumEnd ) {

    if ( ! validWindowStackNum( winStack )) throw ( ERR_INVALID_WIN_STACK_ID );
    if ( ! (( validWindowNum( winNumStart )) && ( validWindowNum( winNumEnd )))) 
        throw ( ERR_INVALID_WIN_ID );
   
    if ( winNumStart > winNumEnd ) {
        
        int tmp = winNumStart;
        winNumStart = winNumEnd;
        winNumEnd = tmp;
    }

    for ( int i = winNumStart; i <= winNumEnd; i++ ) {
        
        if ( windowList[ i - 1 ] != nullptr ) {
            
            windowList[ i - 1 ] -> setWinStack( winStack );
            currentWinNum = i;
        }
    }
}

//----------------------------------------------------------------------------------------
// A window can be added to or removed from the window list shown. We are passed an
// optional windows number, which is used when there are user defined windows for
// locating the window object. In case of a user window, the window is made the current
// window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowEnable( int winNum, bool show ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = currentWinNum;
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    
    windowList[ winNum - 1 ] -> setEnable( show );
    currentWinNum = winNum;

    reDraw( true );
}

//----------------------------------------------------------------------------------------
// For the numeric values in a window, we can set the radix. The token ID for the 
// format option is mapped to the actual radix value.We are passed an optional windows
// number, which is used when there are user defined windows for locating the window
// object. Changing the radix potentially means that the window layout needs to change.
// In case of a user window, the window is made the current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowRadix( int rdx, int winNum ) {

    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = currentWinNum;
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
         
    windowList[ winNum - 1 ] -> setRadix( rdx );
    currentWinNum = winNum;
    
    reDraw( true );
}

//----------------------------------------------------------------------------------------
// "setRows" is the method to set the number if lines in a window. The number includes
// the banner. We are passed an optional windows number, which is used when there are
// user defined windows for locating the window object. The window is made the current
// window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowSetRows( int rows, int winNum ) {

    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = currentWinNum;
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
      
    if ( rows == 0 ) rows = windowList[ winNum - 1 ] -> getDefRows( );
                
    windowList[ winNum - 1 ] -> setRows( rows );
    currentWinNum = winNum;

    reDraw( true );
}

void SimWinDisplay::windowSetCmdWinRows( int rows ) {

    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );

    if ( rows == 0 ) rows = cmdWin -> getDefRows( );
    cmdWin -> setRows( rows );

    reDraw( true );
}

//----------------------------------------------------------------------------------------
// "winHome" will set the current position to the home index, i.e. the position with 
// which the window was cleared. If the position passed is non-zero, it will become the
// new home position. The position meaning is window dependent and the actual window
// will sort it out. We are passed an optional windows number, which is used when there
// are user defined windows for locating the window object. The window is made the
// current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowHome( int pos, int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = getCurrentWindow( );
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );

    ((SimWinScrollable *) windowList[ winNum - 1 ] ) -> winHome( pos );
        setCurrentWindow( winNum );
}

//----------------------------------------------------------------------------------------
// A window is scrolled forward with the "windowForward" method. The meaning of the 
// amount is window dependent and the actual window will sort it out. We are passed an
// optional windows number, which is used when there are user defined windows for 
// locating the window object. The window is made the current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowForward( int amt, int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = getCurrentWindow( );
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    if ( !isWinScrollable( windowList[ winNum - 1 ] -> getWinType( ))) 
        throw (ERR_INVALID_WIN_ID );
    
    ((SimWinScrollable *) windowList[ winNum - 1 ] ) -> winForward( amt );
    setCurrentWindow( winNum );
}

//----------------------------------------------------------------------------------------
// A window is scrolled backward with the "windowBackward" methods. The meaning of the
// amount is window dependent and the actual window will sort it out. We are passed an
// optional windows number, which is used when there are user defined windows for 
// locating the window object. The window is made the current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowBackward( int amt, int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = getCurrentWindow( );
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    if ( !isWinScrollable( windowList[ winNum - 1 ] -> getWinType( ))) 
        throw (ERR_INVALID_WIN_ID );
    
    ((SimWinScrollable *) windowList[ winNum - 1 ] ) -> winBackward( amt );
    setCurrentWindow( winNum );
}

//----------------------------------------------------------------------------------------
// The current index can also directly be set to another location. The position meaning
// is window dependent and the actual window will sort it out. The window is made the 
// current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowJump( int pos, int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = getCurrentWindow( );
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
    if ( !isWinScrollable( windowList[ winNum - 1 ] -> getWinType( ))) 
        throw (ERR_INVALID_WIN_ID );
    
    ((SimWinScrollable *) windowList[ winNum - 1 ] ) -> winJump( pos );
    setCurrentWindow( winNum );
}

//----------------------------------------------------------------------------------------
// The current window index can also directly be set to another location. The position
// meaning is window dependent and the actual window will sort it out. The window is made
// the current window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowToggle( int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    if ( winNum == 0 ) winNum = getCurrentWindow( );
    if ( ! validWindowNum( winNum )) throw ( ERR_INVALID_WIN_ID );
   
    windowList[ winNum - 1 ] -> toggleWin( );
    setCurrentWindow( winNum );
}

//----------------------------------------------------------------------------------------
// The display order of the windows is determined by the window index. It would however
// be convenient to modify the display order. The window exchange command will exchange
// the current window with the window specified by the index of another window.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowExchangeOrder( int winNum ) {
    
    if ( ! winModeOn ) throw( ERR_NOT_IN_WIN_MODE );
    int currentWindow = getCurrentWindow( );
    if ( winNum == currentWindow ) return;
    if ( ! validWindowNum( winNum )) return;
    
    std::swap( windowList[ winNum - 1 ], windowList[ currentWindow ]);
}

//----------------------------------------------------------------------------------------
// "Window New" family of routines create a new window for certain window type. The 
// newly created window also becomes the current user window. The window number is 
// stored from 1 to MAX, the initial stack number is zero.
//
//----------------------------------------------------------------------------------------
int SimWinDisplay::getFreeWindowSlot( ) {

    for ( int i = 0; i < MAX_WINDOWS; i++ ) {
        
        if ( windowList[ i ] == nullptr ) return( i );
    }

    throw( ERR_OUT_OF_WINDOWS );
}

void SimWinDisplay::windowNewAbsMem( int modNum, T64Word adr ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinAbsMem( glb, modNum, adr );
    windowList[ slot ] -> setWinName(( char *) "MEM" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewAbsCode( int modNum, T64Word adr ){

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinCode( glb, modNum, adr ); 
    windowList[ slot ] -> setWinName(( char *) "CODE" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewCpuState( int modNum ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinCpuState( glb, modNum  );
    windowList[ slot ] -> setWinName(( char *) "CPU" );
    windowList[ slot ] -> setWinModNum( modNum );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewITlb( int modNum ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinTlb( glb, modNum );
    windowList[ slot ] -> setWinName(( char *) "I-TLB" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewDTlb( int modNum ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinTlb( glb, modNum );
    windowList[ slot ] -> setWinName(( char *) "D-TLB" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewICache( int modNum ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinCache( glb, modNum );
    windowList[ slot ] -> setWinName(( char *) "I-CACHE" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

void SimWinDisplay::windowNewDCache( int modNum ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinCache( glb, modNum );
    windowList[ slot ] -> setWinName(( char *) "D-CACHE" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
} 
   
void SimWinDisplay::windowNewText( char *pathStr ) {

    int slot = getFreeWindowSlot( );

    windowList[ slot ] = (SimWin *) new SimWinText( glb, pathStr );
    windowList[ slot ] -> setWinName(( char *) "TEXT" );
    windowList[ slot ] -> setDefaults( );
    windowList[ slot ] -> setWinIndex( slot + 1 );
    windowList[ slot ] -> setWinStack( 1 );
    windowList[ slot ] -> setEnable( true );
    currentWinNum = slot + 1;
}

//----------------------------------------------------------------------------------------
// "Window Kill" is the counter part to user windows creation. The method supports 
// removing a range of user windows. When the start is greater than the end, the end 
// is also set to the start window Id. When we kill a window that was the current 
// window, we need to set a new one. We just pick the first used entry in the user 
// range.
//
//----------------------------------------------------------------------------------------
void SimWinDisplay::windowKill( int winNumStart, int winNumEnd ) {
    
    if (( winNumStart < 1 ) || ( winNumEnd > MAX_WINDOWS ))  return;
    if ( winNumStart > winNumEnd ) winNumEnd = winNumStart;

    if ( ! (( validWindowNum( winNumStart )) && ( ! validWindowNum( winNumEnd )))) 
        throw ( ERR_INVALID_WIN_ID );
    
    for ( int i = winNumStart - 1; i <= winNumEnd - 1; i++ ) {
         
        delete ( SimWin * ) windowList[ i ];
        windowList[ i ] = nullptr;
                
        if ( currentWinNum == i + 1 ) {
                 
            for ( int i = 1; i < MAX_WINDOWS; i++ ) {
                        
                if ( validWindowNum( i )) {
                            
                    currentWinNum = i;
                    break;
                }
            }
        }
    }
}
