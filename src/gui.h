#pragma once
#include <imgui.h>
#include <imgui_internal.h>
//#include <OpenGL/gl.h>
#include <functional>
#include <iostream>
#include <map>
#include <fftw3.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif

class gui {
public: 
    gui(
        std::function<bool()> connectCallback,
        std::function<bool()> isConnectedCallback,
        fftw_complex* fftBuffer,
        uint64_t N=4096
    );
    void render();
private:

    // Callbacks:
    std::function<bool()> connectCallback;
    std::function<bool()> isConnectedCallback;
    fftw_complex* fftBuffer;

    // State:
    bool connected;
    uint64_t N;
    int min;
    int max;
    int dynamicRange;

    // Draw Subwindows:
    void renderRX(float width, float height, float xoffset);
    void renderWaterfall(float width, float height, float xoffset);
    void renderTX(float width, float height, float xoffset);

    // Waterfall:
    GLuint waterfallTexture;
    GLuint shaderProgram;
    int waterfallIndex;
    //std::vector<std::array<u_int8_t, 4096>> ringBuffer;
    std::vector<std::array<int, 4096>> ringBuffer;
    void initWaterfall();
    //std::array<ImVec4, 10000> gradient;
    std::map<int, ImVec4> gradient;
    ImGuiWindow* window;
    void prepareGradient();
};