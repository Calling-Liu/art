cmake_minimum_required(VERSION 3.22.1)


project("art")
file(GLOB ART_SRC ./xdllibs/*.c)
file(GLOB ART_HEADER ./xdllibs/*.h ./xdllibs/include/*.h)

add_subdirectory(xdllibs)

add_library(
        art
        SHARED
        art.cpp
        dlopen.c
        dlopen.h
        ${ART_SRC}
        ${ART_HEADER})



find_library(
        log-lib
        log)

target_link_libraries(
        art
        ${log-lib})