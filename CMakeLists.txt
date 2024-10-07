cmake_minimum_required(VERSION 3.30)
project(pluto17)
include(FetchContent)

set (CMAKE_CXX_STANDARD 20)

message(STATUS "Fetching imgui from github")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching SDL2")
FetchContent_Declare(
    SDL2
    URL https://www.libsdl.org/release/SDL2-2.26.5.tar.gz
    URL_HASH MD5=47f22c109070431ecccd90abd3c0ab6e
)
FetchContent_MakeAvailable(SDL2)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

message(STATUS "Fetching libiio from github")
FetchContent_Declare(
    libiio
    GIT_REPOSITORY https://github.com/analogdevicesinc/libiio
    GIT_TAG main
)

#include_directories(build/_deps/libiio-build)

FetchContent_MakeAvailable(libiio)

find_package(OpenGL REQUIRED)

file(GLOB EXTERNAL_SOURCE
    ${imgui_SOURCE_DIR}/*.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(pluto17 src/main.cpp src/gui.cpp ${EXTERNAL_SOURCE})

target_include_directories(pluto17
    PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/examples
)

target_link_libraries(pluto17 PRIVATE SDL2 ${CMAKE_DL_LIBS} ${OPENGL_gl_LIBRARY} ${libiio_LIBRARY})

#cmake_minimum_required(VERSION 3.10)
#include(FetchContent)
#
#
##set(CMAKE_INCLUDE_CURRENT_DIR ON)
#
## GEHT LEIDER NICHT: 
## https://stackoverflow.com/questions/79055537/weird-behavior-of-cmakes-fetchcontent-declare-on-macos
## message(STATUS "Fetching libiio from github")
## FetchContent_Declare(
##     libiio
##     GIT_REPOSITORY https://github.com/analogdevicesinc/libiio
##     GIT_TAG main
## )
#
##set(LIBIIO_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/libiio/build")
##set(LIBIIO_INCLUDEDIR "${CMAKE_CURRENT_LIST_DIR}/libiio")
##
###set(LIBIIO_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/libiio/build")
###set(LIBIIO_INCLUDEDIR "${CMAKE_CURRENT_LIST_DIR}/libiio/")
##
##message(STATUS "Fetching libad9361-iio from github")
##FetchContent_Declare(
##    libad9361-iio
##    GIT_REPOSITORY https://github.com/analogdevicesinc/libad9361-iio
##    GIT_TAG main
##)
#
#
##message(STATUS "Fetching SoapySDR from github")
##FetchContent_Declare(
##    SoapySDR
##    GIT_REPOSITORY https://github.com/pothosware/SoapySDR
##    GIT_TAG master
##)
##
##
#
#message(STATUS "Fetching GuiLite from github")
#FetchContent_Declare(
#    GuiLite
#    GIT_REPOSITORY https://github.com/idea4good/GuiLite
#    GIT_TAG master
#)
#
#FetchContent_MakeAvailable(
#    #libiio
#    #libad9361-iio 
#    #SoapySDR
#    GuiLite 
#)
