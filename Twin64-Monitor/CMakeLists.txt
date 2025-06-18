# ------------------------------------------------------------------------------------------------------------
#  
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.29)
project(Twin64-Monitor)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross-platform compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

add_executable( ${PROJECT_NAME} main.cpp )

target_include_directories( Twin64-Monitor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # if you have your own headers
        ${CMAKE_SOURCE_DIR}/Twin64-Libraries/Twin64-Common
       # ${CMAKE_SOURCE_DIR}/Twin64-InlineAsm
       # ${CMAKE_SOURCE_DIR}/Twin64-Module
       # ${CMAKE_SOURCE_DIR}/Twin64-Processor
)

target_sources ( ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( ${PROJECT_NAME} Twin64-Common )