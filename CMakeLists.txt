cmake_minimum_required(VERSION 3.30)
project(pluto17)
include(FetchContent)
include(ExternalProject)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

message(STATUS "Fetching libiio as external project")
ExternalProject_Add(
    libiio
    GIT_REPOSITORY https://github.com/analogdevicesinc/libiio
    GIT_TAG v0.26
    INSTALL_COMMAND   ""
)

message(STATUS "Fetching libad9361-ii0 as external project")
ExternalProject_Add(
    libad9361-iio
    GIT_REPOSITORY https://github.com/analogdevicesinc/libad9361-iio
    GIT_TAG v0.3
    #DEPENDS libiio
    INSTALL_COMMAND   ""
    CMAKE_ARGS
        -DLIBIIO_INCLUDEDIR=${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio/
        -DLIBIIO_LIBRARIES=${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio-build/iio.framework/
)

find_package(OpenGL REQUIRED)

file(GLOB EXTERNAL_SOURCE
    ${imgui_SOURCE_DIR}/*.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(pluto17
    src/main.cpp
    src/gui.cpp
    src/pluto.cpp
    ${EXTERNAL_SOURCE}
)

add_dependencies(pluto17 libiio libad9361-iio)

target_include_directories(pluto17
    PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/examples
        ${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio/
        ${CMAKE_BINARY_DIR}/libad9361-iio-prefix/src/libad9361-iio
)

target_link_libraries(pluto17
    PRIVATE
    SDL2
    ${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio-build/iio.framework/iio
    ${CMAKE_BINARY_DIR}/libad9361-iio-prefix/src/libad9361-iio-build/ad9361.framework/ad9361
    ${CMAKE_DL_LIBS}
    ${OPENGL_gl_LIBRARY}
)