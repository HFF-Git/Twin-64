# ----------------------------------------------------------------------------------------
#  CMAKE File
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Memory C CXX ASM )

add_library( ${PROJECT_NAME} STATIC 
    
    T64-Memory.h
    T64-Memory.cpp
) 

target_link_libraries( ${PROJECT_NAME} PUBLIC Twin64-Common Twin64-Module )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
