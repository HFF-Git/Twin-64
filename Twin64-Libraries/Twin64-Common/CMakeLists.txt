# ----------------------------------------------------------------------------------------
#  CMAKE File
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Common C CXX ASM )

# add_library( ${PROJECT_NAME} INTERFACE ) 
add_library( ${PROJECT_NAME} STATIC

    T64-Common.h
    T64-Util.h
    T64-Util.cpp 
) 

target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )