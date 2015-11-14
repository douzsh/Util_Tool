cmake_minimum_required(VERSION 2.6)
project(Util_Tool)

option(USE_OPENMP "Use openmp threading support." ON)

#include(ICC.cmake)
#include(cross.cmake)

string(FIND ${CMAKE_CXX_COMPILER} "c++" findRES) 
if(NOT ${findRES} EQUAL -1)
	MESSAGE( STATUS "CXX_COMPILER:         " ${CMAKE_CXX_COMPILER} )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -Wall -msse4")
endif()

include(FindOpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/release)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(SRC_LIST
    ./src/Wave.cpp
)

add_library(Util_Tool STATIC ${SRC_LIST})
