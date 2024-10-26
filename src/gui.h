#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
//#include <OpenGL/gl.h>
#include <functional>
#include <iostream>
#include <map>
#include <fftw3.h>
#include "dsp.h"

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif

class gui {
public: 
    gui(
        std::function<bool()> connectCallback,
        std::function<bool()> fakeConnectCallback,
        std::function<bool()> isConnectedCallback,
        fftw_complex* fftBuffer,
        uint64_t *carrier,
        uint64_t N=4096
    );
    void render();
private:

    // Callbacks:
    std::function<bool()> connectCallback;
    std::function<bool()> fakeConnectCallback;
    std::function<bool()> isConnectedCallback;
    fftw_complex* fftBuffer;

    // State:
    bool connected;
    uint64_t N;
    int min;
    int max;
    double dmin;
    double dmax;
    int dynamicRange;
    int historyIndex;
    uint64_t *carrier;
    std::vector<std::vector<float>> spectrumHistory;
    double filterWidth;
    double filterStart;
    double filterEnd;
    int zoomOffset;
    float qrg;

    // Draw Subwindows:
    void renderRX(float width, float height, float xoffset);
    void renderMain(float width, float height, float xoffset);
    void renderTX(float width, float height, float xoffset);

    // Bandplan:
    // Define bandplan segments
    struct BandSegment {
        double startFreq;
        double endFreq;
        ImVec4 color;
        const char* label;
    };

    std::vector<BandSegment> bandplan = {
        {10'489'500'000.0, 10'489'505'000.0, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "B"},
        {10'489'505'000.0, 10'489'540'000.0, ImVec4(0.0f, 0.5f, 0.0f, 1.0f), "CW"},
        {10'489'540'000.0, 10'489'580'000.0, ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "NB"},
        {10'489'580'000.0, 10'489'650'000.0, ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "D"},
        {10'489'650'000.0, 10'489'745'000.0, ImVec4(0.0f, 0.0f, 0.8f, 1.0f), "SSB"},
        {10'489'745'000.0, 10'489'755'000.0, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "B"},
        // Center: 10'489'750'000
        {10'489'755'000.0, 10'489'850'000.0, ImVec4(0.0f, 0.0f, 0.8f, 1.0f), "SSB"},
        {10'489'850'000.0, 10'489'990'000.0, ImVec4(0.3f, 0.5f, 0.0f, 1.0f), "Mix"},
        {10'489'990'000.0, 10'490'000'000.0, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "B"},
    };

    // Waterfall:
    std::array<float, 4096> frequencyBins;
    GLuint waterfallTexture;
    GLuint waterfallShaderProgram;
    int waterfallIndex;
    std::vector<std::array<int, 4096>> waterfallRingBuffer;
    void initWaterfall();
    std::map<int, ImVec4> gradient;
    ImGuiWindow* window;
    void prepareGradient();
    void renderVFO(float height);
    void dragVFO();
    bool renderVFOtrigger;
};