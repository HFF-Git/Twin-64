# ----------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-System C CXX ASM )

add_library( ${PROJECT_NAME} STATIC  

    T64-System.h 
    T64-System.cpp
 ) 

target_link_libraries( ${PROJECT_NAME} PUBLIC 

    Twin64-Module
    Twin64-Processor 
    Twin64-Memory 
    Twin64-Common 
)

target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )