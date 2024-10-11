#include "gui.h"

gui::gui(
    std::function<bool()> connectCallback,
    std::function<bool()> isConnectedCallback,
    fftw_complex* fftBuffer
) : ringBuffer(256) {
    this->connectCallback = connectCallback;
    this->isConnectedCallback = isConnectedCallback;
    this->fftBuffer = fftBuffer;

    initWaterfall();
}

void gui::render() {
    // Get the size of the main window
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    renderRX(windowSize.x*0.2, windowSize.y, 0.0f);
    renderWaterfall(windowSize.x*0.6, windowSize.y*1.0, windowSize.x*0.2);
    renderTX(windowSize.x*0.2, windowSize.y, windowSize.x*0.8);
}

void gui::renderRX(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::Begin("RX", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if(isConnectedCallback()) {
        if (ImGui::Button("Disconnect")) {
            // TODO: implement disconnect
        }
    } else {
        if (ImGui::Button("Connect")) {
            if(connectCallback()){
                std::cout << "Connected to Pluto" << std::endl;
            } else {
                std::cout << "ERROR: Cannot connect to Pluto" << std::endl;
                // TODO: show error window with reason
            }
        }
    }

    ImGui::End();
}

void gui::renderWaterfall(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Waterfall", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    window = ImGui::GetCurrentWindow();
    auto widgetPos = ImGui::GetWindowContentRegionMin();
    auto widgetEndPos = ImGui::GetWindowContentRegionMax();
    widgetPos.x += window->Pos.x;
    widgetPos.y += window->Pos.y;
    widgetEndPos.x += window->Pos.x; // Padding
    widgetEndPos.y += window->Pos.y;
    auto widgetSize = ImVec2(widgetEndPos.x - widgetPos.x, widgetEndPos.y - widgetPos.y);

    std::array<u_int8_t, 4096> data;
    // Test data:
    //std::generate(data.begin(), data.end(), []() { return static_cast<u_int8_t>(rand() % 256); });
    //for(int i = 0; i < 4096; i++) {
    //    data[i] = static_cast<u_int8_t>(i % 256);
    //}

    double ofs = 20.0f * ::log10f(1.0f / 4096.0f);
    double mult = (10.0f / ::log2f(10.0f));
    for(int i = 0; i < 4096; i++) {
        double f = 1.0 * (
            mult * log2f(
                fftBuffer[i][0]*fftBuffer[i][0] + fftBuffer[i][1]*fftBuffer[i][1]
            ) + ofs + 150 
        );
        data[i] = static_cast<u_int8_t>(f);
        std::cout << f << " : " << static_cast<int>(data[i]) << std::endl;
    }

    // Define a gradient lookup table
    std::array<ImVec4, 256> gradient;
    for (int i = 0; i < 256; ++i) {
        gradient[i] = ImVec4(
            i / 255.0f, // Red
            0.0f, // Green
            (255 - i) / 255.0f, // Blue
            1.0f
        ); // Example gradient from red to blue
    }

    // Add new data to the ring buffer
    ringBuffer[waterfallIndex] = data;
    waterfallIndex = (waterfallIndex + 1) % ringBuffer.size();

    // Create a texture from the ring buffer
    std::vector<u_int8_t> textureData(4096 * 256 * 3);
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 4096; ++j) {
            int index = ((waterfallIndex + i) % 256) * 4096 + j;
            u_int8_t value = ringBuffer[index / 4096][index % 4096];
            ImVec4 color = gradient[value];
            textureData[(i * 4096 + j) * 3 + 0] = static_cast<u_int8_t>(color.x * 255); // Red
            textureData[(i * 4096 + j) * 3 + 1] = static_cast<u_int8_t>(color.y * 255); // Green
            textureData[(i * 4096 + j) * 3 + 2] = static_cast<u_int8_t>(color.z * 255); // Blue
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4096, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Use the shader program
    glUseProgram(shaderProgram);

    ImGui::Image(waterfallTexture, ImVec2(widgetSize.x, widgetSize.y), ImVec2(0, 1), ImVec2(1, 0));

    window->DrawList->AddLine(ImVec2(widgetPos.x, widgetPos.y), ImVec2(widgetEndPos.x, widgetEndPos.y), IM_COL32(50, 50, 50, 255), 1.0);

    ImGui::End();
}

void gui::renderTX(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("TX", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("This is some useful text.");
    ImGui::End();
}

void gui::initWaterfall() {
    glGenTextures(1, &waterfallTexture);

    // Shader source code
    const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
    )";

    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D texture1;
    void main() {
        FragColor = texture(texture1, TexCoord);
    }
    )";

    // Compile and link shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate texture
    glBindTexture(GL_TEXTURE_2D, waterfallTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    waterfallIndex = 0;
}
