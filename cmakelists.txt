cmake_minimum_required(VERSION 3.15)
project(compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES src/*.cpp)

add_executable(compiler_main ${SOURCES})
target_include_directories(compiler_main PRIVATE includes/)

file(GLOB TEST_SOURCES test/*.cpp)

add_executable(compiler_tests ${TEST_SOURCES} 
src/utils.cpp 
src/json_validator.cpp 
src/lexer.cpp 
src/parser.cpp 
src/pratt_parser.cpp 
)

target_include_directories(compiler_tests PRIVATE includes/) 