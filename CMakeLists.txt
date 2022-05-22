cmake_minimum_required(VERSION 3.6)
project(pix2ascii)

set(CMAKE_C_STANDARD 11)

#Set your directories.  The dot representes the root application folder.
#Thus my the path to my domain folder:
set(PROJECT_FOLDER ./project)
set(INCLUDE_DIR ${PROJECT_FOLDER}/include)
set(SOURCE_DIR ${PROJECT_FOLDER}/src)

find_package(Curses REQUIRED)
#Include the directories and now your cpp files will recognize your headers
include_directories(${INCLUDE_DIR})
include_directories(${CURSES_INCLUDE_DIR})

set(
        SOURCE_FILES
        ${SOURCE_DIR}/main.c
        ${INCLUDE_DIR}/utils.h
        ${INCLUDE_DIR}/argparsing.h
        ${SOURCE_DIR}/argparsing.c
        ${INCLUDE_DIR}/videostream.h
        ${SOURCE_DIR}/videostream.c
        ${INCLUDE_DIR}/frame_processing.h
        ${SOURCE_DIR}/frame_processing.c
        ${INCLUDE_DIR}/termstream.h
        ${SOURCE_DIR}/termstream.c
        ${INCLUDE_DIR}/timestamps.h
        ${SOURCE_DIR}/timestamps.c
)

add_compile_options(-lncurses)
add_executable(program ${SOURCE_FILES})
target_link_libraries(program ${CURSES_LIBRARIES})
