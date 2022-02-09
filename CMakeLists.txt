cmake_minimum_required(VERSION 2.8)
project(roverdisplay C)

add_compile_options(-Wall)

find_package(Curses REQUIRED)
find_library(LIBRT rt)
find_library(LIBCOMM14CUX_LIBRARY comm14cux PATHS ${CMAKE_SOURCE_DIR}/../libcomm14cux/ NO_CMAKE_FIND_ROOT_PATH)

include_directories(${CMAKE_SOURCE_DIR}/../libcomm14cux/src)

add_library(cuxinterface cuxinterface.c)
target_link_libraries(cuxinterface ${LIBCOMM14CUX_LIBRARY})



add_executable(roverdisplay rover.c)
target_link_libraries(roverdisplay ${LIBRT})
target_link_libraries(roverdisplay ${CURSES_LIBRARIES})
target_link_libraries(roverdisplay cuxinterface)


