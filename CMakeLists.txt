cmake_minimum_required(VERSION 3.30)
project(pluto17)
include(FetchContent)
include(ExternalProject)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

message(STATUS "Fetching imgui")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)


message(STATUS "Fetching imgui")
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot
    GIT_TAG master
)
FetchContent_MakeAvailable(implot)

message(STATUS "Fetching SDL2")
FetchContent_Declare(
    SDL2
    URL https://www.libsdl.org/release/SDL2-2.26.5.tar.gz
    URL_HASH MD5=47f22c109070431ecccd90abd3c0ab6e
)
FetchContent_MakeAvailable(SDL2)

message(STATUS "Fetching PortAudio")
FetchContent_Declare(
    PortAudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio
    GIT_TAG v19.7.0
)
FetchContent_MakeAvailable(PortAudio)

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

ExternalProject_Add(
    fftw3
    URL http://fftw.org/fftw-3.3.10.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP false
    INSTALL_COMMAND   ""
)

message(STATUS "Fetching liquid-dsp as external project")
ExternalProject_Add(
    liquid-dsp
    GIT_REPOSITORY https://github.com/jgaeddert/liquid-dsp
    GIT_TAG v1.6.0
    CONFIGURE_COMMAND ./bootstrap.sh && ./configure
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND   ""
)

find_package(OpenGL REQUIRED)

file(GLOB EXTERNAL_SOURCE
    ${imgui_SOURCE_DIR}/*.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)

add_executable(pluto17
    src/main.cpp
    src/gui.cpp
    src/dsp.cpp
    src/pluto.cpp
    ${EXTERNAL_SOURCE}
)

add_dependencies(pluto17 libiio libad9361-iio fftw3 liquid-dsp)

target_include_directories(pluto17
    PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/examples
        ${implot_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio/
        ${CMAKE_BINARY_DIR}/libad9361-iio-prefix/src/libad9361-iio
        ${CMAKE_BINARY_DIR}/fftw3-prefix/src/fftw3/api
        ${CMAKE_BINARY_DIR}/liquid-dsp-prefix/src/liquid-dsp/include
)

target_link_libraries(pluto17
    PRIVATE
        SDL2
        portaudio
        ${CMAKE_BINARY_DIR}/libiio-prefix/src/libiio-build/iio.framework/iio
        ${CMAKE_BINARY_DIR}/libad9361-iio-prefix/src/libad9361-iio-build/ad9361.framework/ad9361
        ${CMAKE_BINARY_DIR}/fftw3-prefix/src/fftw3-build/libfftw3.dylib
        ${CMAKE_BINARY_DIR}/liquid-dsp-prefix/src/liquid-dsp/libliquid.ar
        ${CMAKE_DL_LIBS}
        ${OPENGL_gl_LIBRARY}
)