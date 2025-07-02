# ----------------------------------------------------------------------------------------
#  CMAKE File
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Common C CXX ASM )

add_library( ${PROJECT_NAME} INTERFACE ) 
target_include_directories( ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )