cmake_minimum_required(VERSION 3.15)

INCLUDE (CheckTypeSize)
set(CMAKE_CXX_STANDARD 23)
project(xdrfile C CXX)
if(WIN32)
    # Windows
    message(STATUS "Building for Windows")
elseif(UNIX AND NOT APPLE)
    # Linux
    set(CMAKE_CXX_FLAGS "-stdlib=libc++ -static")
    message(STATUS "Building for UNIX (non-Apple)")
elseif(APPLE)
    # macOS
    set(CMAKE_CXX_FLAGS "-stdlib=libc++")
    message(STATUS "Building for macOS")
else()
    message(STATUS "Building for an unknown system")
endif()

check_type_size("int" INT_SIZE)
if (NOT "${INT_SIZE}" MATCHES "^(2|4|8|16)$") # in Bytes; 16, 32, 64, 128 bit
    # See PR #2 for details
    message(FATAL_ERROR "Int size ${INT_SIZE} is not supported")
endif()

set(SOURCES
    src/xdrfile.c
    src/xdrfile_trr.c
    src/xdrfile_xtc.c
    include/xdrfile.h
    include/xdrfile_trr.h
    include/xdrfile_xtc.h
        include/tqdm.h
)
add_library(xdrfile OBJECT ${SOURCES})
target_include_directories(xdrfile PUBLIC include)
add_executable(main main.cpp $<TARGET_OBJECTS:xdrfile>)
target_include_directories(main PRIVATE include)

