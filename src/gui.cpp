#include "gui.h"

void gui::render() {
    // Get the size of the main window
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    renderRX(windowSize.x*0.2, windowSize.y, 0.0f);
    renderWaterfall(windowSize.x*0.6, windowSize.y*1.0, windowSize.x*0.2);
    renderTX(windowSize.x*0.2, windowSize.y, windowSize.x*0.8);
}

void gui::renderRX(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("RX Pane", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("This is some useful text.");
    ImGui::End();
}

void gui::renderWaterfall(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Waterfall", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::End();
}

void gui::renderTX(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Third Pane", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("This is some useful text.");
    ImGui::End();
}


