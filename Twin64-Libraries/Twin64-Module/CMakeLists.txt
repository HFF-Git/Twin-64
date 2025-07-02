# ----------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Module C CXX ASM )

add_library( ${PROJECT_NAME} STATIC  

    T64-Module.h
    T64-Module.cpp
) 

target_link_libraries( ${PROJECT_NAME} PUBLIC Twin64-Common )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )