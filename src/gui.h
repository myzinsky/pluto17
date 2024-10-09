#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <functional>
#include <iostream>

class gui {
public: 
    gui(
        std::function<bool()> connectCallback,
        std::function<bool()> isConnectedCallback
    );
    void render();
private:

    // Callbacks:
    std::function<bool()> connectCallback;
    std::function<bool()> isConnectedCallback;

    // Draw Subwindows:
    void renderRX(float width, float height, float xoffset);
    void renderWaterfall(float width, float height, float xoffset);
    void renderTX(float width, float height, float xoffset);

    ImGuiWindow* window;

};