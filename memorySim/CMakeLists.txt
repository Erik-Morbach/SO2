cmake_minimum_required(VERSION 3.21)

project(MemorySim 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXPORT_COMPILE_COMMANDS TRUE)

add_executable(MemorySim 
    src/main.cpp 
    src/pageEntry.cpp 
)
