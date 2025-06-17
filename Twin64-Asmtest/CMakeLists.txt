# ------------------------------------------------------------------------------------------------------------
#  
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.29)
project(Twin64-AsmTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross-platform compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)
endif()

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries ( ${PROJECT_NAME} Twin64-Module Twin64-Processor Twin64-InlineAsm )

# Add a custom command to run a shell script. We would like to move the uf2 file to a dedicated directory.
# The script is executed after the build proccess. The target direvtory is "bin" a child at the overall
# project level.

# add_custom_command(
#   TARGET ${PROJECT_NAME}
#   POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E echo "Move target to bin directory"
#    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../bin
#    COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2
#        ${CMAKE_BINARY_DIR}/../bin/${PROJECT_NAME}.uf2
#    )
