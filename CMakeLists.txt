# ------------------------------------------------------------------------------------------------------------
#  
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)
project( Twin64 C CXX ASM )

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Cross-platform compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

add_subdirectory( Twin64-Asmtest )
add_subdirectory( Twin64-Monitor )

add_subdirectory( Twin64-Libraries/Twin64-Common )
add_subdirectory( Twin64-Libraries/Twin64-Module )
add_subdirectory( Twin64-Libraries/Twin64-Processor )
add_subdirectory( Twin64-Libraries/Twin64-InlineAsm )

