# ----------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Processor C CXX ASM )

add_library( ${PROJECT_NAME} STATIC  

    T64-Processor.h
    T64-Cpu.cpp
    T64-Tlb.cpp 
    T64-Cache.cpp
) 

target_link_libraries( ${PROJECT_NAME} PUBLIC Twin64-Common Twin64-Module )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )