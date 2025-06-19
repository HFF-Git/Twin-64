# ------------------------------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required( VERSION 3.29 )

project( Twin64-InlineAsm C CXX ASM )

add_library( ${PROJECT_NAME} STATIC 

    T64-InlineAsm.h 
    T64-InlineAsm.cpp 
    T64-InlineDisasm.cpp
) 

target_link_libraries( ${PROJECT_NAME} PUBLIC Twin64-Common )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )