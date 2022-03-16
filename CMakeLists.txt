cmake_minimum_required(VERSION 2.8)
project(roverdisplay C)

set(SOURCE_SUBDIR "${CMAKE_SOURCE_DIR}/src")
set(CMAKE_BUILD_RPATH ".")

add_compile_options(-Wall)

find_package(Curses REQUIRED)
find_library(LIBRT rt)
find_library(CURSES_PANEL_LIBRARY panel)
find_library(LIBCOMM14CUX_LIBRARY comm14cux PATHS 
${CMAKE_SOURCE_DIR}/../libcomm14cux/, ${CMAKE_SOURCE_DIR}/../libcomm14cux/build NO_CMAKE_FIND_ROOT_PATH)

include_directories(${CMAKE_SOURCE_DIR}/../libcomm14cux/src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(cuxinterface ${SOURCE_SUBDIR}/cuxinterface.c)
target_link_libraries(cuxinterface ${LIBCOMM14CUX_LIBRARY})



add_executable(roverdisplay ${SOURCE_SUBDIR}/rover.c)
target_link_libraries(roverdisplay ${LIBRT})
target_link_libraries(roverdisplay ${CURSES_LIBRARIES})
target_link_libraries(roverdisplay ${CURSES_PANEL_LIBRARY})
target_link_libraries(roverdisplay cuxinterface)


add_custom_command(TARGET roverdisplay POST_BUILD COMMAND cp -L ${LIBCOMM14CUX_LIBRARY} ${CMAKE_BINARY_DIR}/bin)
