# ----------------------------------------------------------------------------------------
#  CMAKE File
#  Copyright (C) 2020 - 2026  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-SPL )

add_executable( ${PROJECT_NAME} 

    main.cpp
    T64-SplVersion.h
    T64-SplDeclarations.h 
    T64-SplConfig.cpp
    T64-SplTokenizer.cpp
    T64-SplSymtab.cpp
    T64-SplParser.cpp
    T64-SplAST.cpp
    T64-SplCodeGen.cpp
)

target_link_libraries( ${PROJECT_NAME} PRIVATE 

    Twin64-Common 
    ELFIO
)