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

include_directories( Twon64-Module Twin64-processor Twin64-InlineAsm )

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries ( Twin64-Monitor PRIVATE Twin64-Libraries )