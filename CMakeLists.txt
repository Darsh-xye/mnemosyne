cmake_minimum_required(VERSION 3.14)
project(memory_allocator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

#Core library sources
set(LIB_SOURCES
    src/slab.cpp
    src/allocator.cpp
)

#Test executable 
add_executable(test_slab
    tests/test_slab.cpp
    ${LIB_SOURCES}
)


# Run with: cmake -DUSE_ASAN=ON 

option(USE_ASAN "Build with AddressSanitizer" OFF)
if(USE_ASAN)
    target_compile_options(test_slab PRIVATE -fsanitize=address -g)
    target_link_options(test_slab PRIVATE -fsanitize=address)
endif()