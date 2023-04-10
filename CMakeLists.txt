cmake_minimum_required(VERSION 2.8.12)
project(search-server)

add_definitions("-std=c++17")

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(DCMAKE_PREFIX_PATH "C:\\msys64\\ucrt64\\include\\tbb")

find_package(TBB REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3") #g++ -ltbb  -lpthread

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()


add_executable(search-server search-server/main.cpp
    search-server/document.cpp
    search-server/log_duration.cpp
    search-server/paginator_impl.h
    search-server/read_input_functions.cpp
    search-server/request_queue.cpp
    search-server/search_server.cpp
    search-server/string_processing.cpp
    search-server/test_example_functions.cpp
    search-server/remove_duplicates.cpp
    search-server/process_queries.cpp
)

target_link_libraries(search-server PRIVATE TBB::tbb)
