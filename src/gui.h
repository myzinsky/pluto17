#pragma once
#include "imgui.h"

class gui {
public: 
    void render();
private:
    void renderRX(float width, float height, float xoffset);
    void renderWaterfall(float width, float height, float xoffset);
    void renderTX(float width, float height, float xoffset);
};