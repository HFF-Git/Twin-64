//----------------------------------------------------------------------------------------
//
//  Twin64 - A 64-bit CPU Simulator - Configuration 
//
//----------------------------------------------------------------------------------------
// 
//
//----------------------------------------------------------------------------------------
//
// Twin64 - A 64-bit CPU Simulator - Configuration 
// Copyright (C) 2025 - 2025 Helmut Fieres
//
// This program is free software: you can redistribute it and/or modify it under 
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY 
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License for more details. You 
// should have received a copy of the GNU General Public License along with this 
// program. If not, see <http://www.gnu.org/licenses/>.
//
//----------------------------------------------------------------------------------------
#include "T64-SimVersion.h"
#include "T64-SimDeclarations.h"
#include "T64-SimTables.h"

//----------------------------------------------------------------------------------------
// Local data for command line option parsing.
//
//----------------------------------------------------------------------------------------
namespace {

    int  optIndex = 1;  
}

//----------------------------------------------------------------------------------------
// "parseCmdLineOptions" function to parse long command line options. This routine 
// is called repeatedly to parse all command line options. It returns the value field
// of the matched option or -1 if no more options are found. The optArg variable is set
// to the argument string if an argument is expected.   
//
//----------------------------------------------------------------------------------------
int parseCmdLineOptions( int    argc, 
                         char   *argv[ ],
                         struct SimCmdLineOptions *optionTable,
                         char   **optArg ) {

    if ( optIndex >= argc ) return ( -1 );

    const char *arg = argv[ optIndex ];

    if ( strncmp( arg, "--", 2 ) != 0 ) return -1;

    const char *name    = arg + 2;
    const char *eq      = strchr(name, '=');
    size_t name_len     = (( eq ) ? ((size_t)( eq - name )) : ( strlen( name )));

    for ( int i = 0; ( optionTable && optionTable[i].name ); i++ ) {

        if (( strncmp( name, optionTable[i].name, name_len ) == 0 ) &&
             ( strlen(optionTable[i].name) == name_len )) {

            if ( optionTable[i].argOpt == OPT_REQUIRED_ARGUMENT) {
                
                if ( eq ) {

                    *optArg = (char *)( eq + 1 );
                }
                else if ( optIndex + 1 < argc ) {

                    *optArg = argv[ ++optIndex ];
                }
                else {
                    
                    fprintf( stderr, "Option '--%s' requires an argument\n", 
                            optionTable[ i ].name);
                    return '?';
                }
            } 
            else if ( optionTable[ i ].argOpt == OPT_OPTIONAL_ARGUMENT ) {
                
                *optArg = (( eq ) ? ((char *)( eq + 1 )) : ( NULL ));
            
            } else *optArg = NULL;
    
            optIndex++;
            return optionTable[ i ].val;
        }
    }

    fprintf( stderr, "Unknown option '%s'\n", arg) ;
    optIndex++;
    return '?';
}

//----------------------------------------------------------------------------------------
// "processCmdLineOptions" function to process all command line options. We call the
// parser in a loop to get all options one by one. The help option is special in 
// that it will list the program call help and then exit.
//
//----------------------------------------------------------------------------------------
void processCmdLineOptions( SimGlobals *glb, int argc, char *argv[ ] ) {

    char *optArg = NULL;
    int   optVal = 0;

    while (( optVal = parseCmdLineOptions( argc, 
                                           argv, 
                                           optionTable, 
                                           &optArg )) != -1 ) {

        switch ( optVal ) {

            case 'h':  {

                printf( "Twin64 Simulator Version %s, PatchLevel %d\n\n", 
                        SIM_VERSION, SIM_PATCH_LEVEL );

                printf( "Usage: Twin64-Simulator [options]\n\n" );

                printf( "Options:\n" );
                printf( "  --help                 : display this help message\n" );
                printf( "  --version              : display program version\n" );
                printf( "  --verbose              : enable verbose output\n" );
                printf( "  --configfile=<file>    : specify configuration file\n" );
                printf( "  --logfile=<file>       : specify log file\n" );
                exit( 0 );
            
            } break;

            case 'v': {

                printf( "Twin64 Simulator Version %s, PatchLevel %d\n\n", 
                        SIM_VERSION, SIM_PATCH_LEVEL );
                exit( 0 );

            } break;

            case 'd': {

                glb -> verboseFlag = true;
        
            } break;

            case 'f': {

                if ( optArg ) {

                    strncpy( glb -> configFile, optArg, MAX_FILE_PATH_SIZE - 1 );
                    glb -> configFile[ MAX_FILE_PATH_SIZE - 1 ] = '\0';

                    if ( glb->verboseFlag ) {

                        printf( "Using config file: %s\n", glb->configFile );
                    }   
                } 
                else {
                
                    printf( "Error: --configfile requires a filename\n" );
                    exit( 1 );
                }

            } break; 

            case 'l': {

                if ( optArg ) {
        
                    strncpy( glb -> logFile, optArg, MAX_FILE_PATH_SIZE - 1 );
                    glb -> logFile[ MAX_FILE_PATH_SIZE - 1 ] = '\0';

                    #if 0
                    glb -> logFp = fopen(glb->logFile, "w");
                    if ( ! glb-> logFp ) {
                        
                        perror("Failed to open log file");
                        exit(1);
                    }
                    #endif

                    if ( glb->verboseFlag ) {

                        printf("Logging to file: %s\n", glb -> logFile);
                    }
                }
                else {
        
                    printf("Error: --logfile requires a filename\n");
                    exit(1);
                }

            } break; 

            default: {

                printf( "Invalid command parameter option, use help\n" );
                exit( 1 );
            };
        }
    }
}

//----------------------------------------------------------------------------------------
// 
// 
//----------------------------------------------------------------------------------------
//
// the idea is to have two more commands than add and remove a module. 
//
// NM <mod-type> <mod-params>  - add a new module of type mod-type with parameters
// RM <mod-num>                - remove module with module number mod-num
//
// Example:
//
// MN proc, itlb, FA_64S, dtlb, FA_64S, icache, SA_2W_128S_4L, dcache, SA_8W_128S_4L
//
// MN proc, ( cpu: NIL ), ( itlb:FA_64S ), ( dtlb:FA_64S ), ( icache:SA_2W_128S_4L ), ...
// seems easer to read...
// 
// The NM command could then be used in an XF file as the initial setup.
// Best of all, we around to come up with config file syntax and a parser for it.
//
// The module number is assigned by the system, not the config!
//
//
//----------------------------------------------------------------------------------------