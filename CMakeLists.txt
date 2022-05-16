cmake_minimum_required(VERSION 3.21)
project(pix2ascii)

set(CMAKE_CXX_STANDARD 14)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

#Set your directories.  The dot representes the root application folder.
#Thus my the path to my domain folder:
set(APP_SRC ./project/src)
set(APP_HEADS ./project/include)

#Include the directories and now your cpp files will recognize your headers
include_directories(${APP_SRC})
include_directories(${APP_HEADS})

set(
        SOURCE_FILES
        ${APP_HEADS}/utils.h
        ${APP_HEADS}/frame_utils.h
        ${APP_HEADS}/timestamps.h

        ${APP_SRC}/main.c
        ${APP_SRC}/frame_utils.c
        ${APP_SRC}/timestamps.c
)

add_compile_options(-lncurses)
add_executable(main ${SOURCE_FILES})

target_link_libraries(main ${CURSES_LIBRARIES})
