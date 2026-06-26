cmake_minimum_required(VERSION 3.14)
project(memory_allocator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
find_package(Threads REQUIRED)

set(LIB_SOURCES
    src/slab.cpp
    src/allocator.cpp
    src/thread_cache.cpp
    src/central_pool.cpp
    src/slab_refill.cpp
)
#PART 1
add_executable(test_slab
    tests/test_slab.cpp
    ${LIB_SOURCES}
)
target_link_libraries(test_slab PRIVATE Threads::Threads atomic)

#PART 2
add_executable(test_threads
    tests/test_threads.cpp
    ${LIB_SOURCES}
)
target_link_libraries(test_threads PRIVATE Threads::Threads atomic)

#Benchmark
add_executable(bench
    bench/bench.cpp
    ${LIB_SOURCES}
)
target_link_libraries(bench PRIVATE Threads::Threads atomic)
target_compile_options(bench PRIVATE -O2 -flto)
target_link_options(bench PRIVATE -flto)

option(USE_ASAN "Build with AddressSanitizer" OFF)
if(USE_ASAN)
    target_compile_options(test_slab PRIVATE -fsanitize=address -g)
    target_link_options(test_slab PRIVATE -fsanitize=address)
endif()

option(USE_TSAN "Build with ThreadSanitizer" OFF)
if(USE_TSAN)
    target_compile_options(test_threads PRIVATE -fsanitize=thread -g)
    target_link_options(test_threads PRIVATE -fsanitize=thread)
endif()