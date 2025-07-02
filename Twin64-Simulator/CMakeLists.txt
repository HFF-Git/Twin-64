# ----------------------------------------------------------------------------------------
#  #  CMAKE File
#  Copyright (C) 2025 - 2025  Helmut Fieres
# ----------------------------------------------------------------------------------------
project( Twin64-Simulator )

add_executable( ${PROJECT_NAME} 

    main.cpp
    T64-SimVersion.h
    T64-SimDeclarations.h 
    T64-SimTables.h
    T64-SimTokenizer.cpp
    T64-SimExprEvaluator.cpp
    T64-SimEnvVars.cpp
)

target_link_libraries( ${PROJECT_NAME} PRIVATE 

    Twin64-Common 
    Twin64-ConsoleIO 
    Twin64-InlineAsm 
    Twin64-Processor
)