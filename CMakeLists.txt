cmake_minimum_required(VERSION 3.10)
project(CPPCaller VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add executable
add_executable(cppcaller main.cpp)

# Include directory for rustlib header
target_include_directories(cppcaller PRIVATE 
    ${CMAKE_SOURCE_DIR}/../rustlib
)

# Link the Rust library
target_link_libraries(cppcaller PRIVATE 
    ${CMAKE_SOURCE_DIR}/../rustlib/target/release/rustlib.dll.lib
)
