cmake_minimum_required(VERSION 3.10)
project(CPPCaller VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add executable with SQLite3 amalgamation
add_executable(cppcaller 
    main.cpp
    sqlite3/sqlite3.c
)

# Include directories
target_include_directories(cppcaller PRIVATE 
    ${CMAKE_SOURCE_DIR}/../rustlib
    ${CMAKE_SOURCE_DIR}/sqlite3
)

# Link the Rust library
target_link_libraries(cppcaller PRIVATE 
    ${CMAKE_SOURCE_DIR}/../rustlib/target/release/rustlib.dll.lib
)
