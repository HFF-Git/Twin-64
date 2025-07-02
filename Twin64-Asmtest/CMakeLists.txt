# ----------------------------------------------------------------------------------------
#  CMAKE File
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-AsmTest )

add_executable( ${PROJECT_NAME} main.cpp )

target_link_libraries (${PROJECT_NAME}

    PRIVATE Twin64-Common Twin64-InlineAsm
)