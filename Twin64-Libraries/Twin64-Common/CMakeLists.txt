# ------------------------------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required( VERSION 3.29 )

project( Twin64-Common C CXX ASM )

set( CMAKE_C_STANDARD 11 )
set( CMAKE_CXX_STANDARD 17 ) 

add_library( ${PROJECT_NAME} INTERFACE ) 
target_include_directories( ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )