# ------------------------------------------------------------------------------------------------------------
#
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ------------------------------------------------------------------------------------------------------------
cmake_minimum_required( VERSION 3.29 )

project( Twin64-Memory C CXX ASM )

add_library( ${PROJECT_NAME} STATIC 
    
) 

target_include_directories( ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_sources ( ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( ${PROJECT_NAME} 

    
    )

