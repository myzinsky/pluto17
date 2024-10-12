#include "gui.h"

gui::gui(
    std::function<bool()> connectCallback,
    std::function<bool()> isConnectedCallback,
    fftw_complex* fftBuffer,
    uint64_t N
) : waterfallRingBuffer(256),
    zoomRingBuffer(128),
    N(N)
{
    // Calculate dynamic range (14 bit ADC of Pluto and log2(N) bits for FFT, for each bit we have 6 dB gain)
    dynamicRange = static_cast<uint64_t>((14.0f+log2f(N))*6);

    // Initialize waterfallRingBuffer with -1 * dynamicRange
    for (auto& buffer : waterfallRingBuffer) {
        buffer.fill(-1 * dynamicRange);
    }

    // Initialize zoomRingBuffer with 0
    for (auto& buffer : zoomRingBuffer) {
        buffer.fill(0);
    }

    // Prepare the Gradient:
    max = -65;
    min = -80;
    prepareGradient();

    // Initialize the OpenGL Shaders
    initWaterfall();
    initZoom();

    // Setup Callbacks
    this->connectCallback = connectCallback;
    this->isConnectedCallback = isConnectedCallback;
    this->fftBuffer = fftBuffer;

    connected = false;

}

void gui::render() {
    // Get the size of the main window
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    renderRX(windowSize.x*0.2, windowSize.y, 0.0f);
    renderMain(windowSize.x*0.6, windowSize.y*1.0, windowSize.x*0.2);
    renderTX(windowSize.x*0.2, windowSize.y, windowSize.x*0.8);
}

void gui::renderRX(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::Begin("RX", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);


    if (ImGui::CollapsingHeader("Adalm Pluto Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if(isConnectedCallback()) {
            if (ImGui::Button("Disconnect")) {
                // TODO: implement disconnect
            }
        } else {
            if (ImGui::Button("Connect")) {
                if(connectCallback()){
                    std::cout << "Connected to Pluto" << std::endl;
                    connected = true;
                } else {
                    std::cout << "ERROR: Cannot connect to Pluto" << std::endl;
                    // TODO: show error window with reason
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Waterfall Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if(ImGui::SliderInt("Min", &min, -1*dynamicRange,0)){
            prepareGradient();
        }
        if(ImGui::SliderInt("Max", &max, -1*dynamicRange,0)) {
            prepareGradient();
        }
    }

    if (ImGui::CollapsingHeader("Zoom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int M = 128;
        if(connected==true) {
            window = ImGui::GetCurrentWindow();
            auto widgetPos = ImGui::GetWindowContentRegionMin();
            auto widgetEndPos = ImGui::GetWindowContentRegionMax();
            widgetPos.x += window->Pos.x;
            widgetPos.y += window->Pos.y;
            widgetEndPos.x += window->Pos.x; 
            widgetEndPos.y += window->Pos.y;
            auto widgetSize = ImVec2(widgetEndPos.x - widgetPos.x, 128);

            std::array<int, 128> data;

            double ofs = 20.0f * ::log10f(1.0f / N);
            double mult = (10.0f / ::log2f(10.0f));
            for (int i = zoomOffset; i < zoomOffset+M; i++)
            {
                double f = (
                    mult * log2f(
                        fftBuffer[i][0]*fftBuffer[i][0] + fftBuffer[i][1]*fftBuffer[i][1]
                    ) + ofs 
                );

                data[i-zoomOffset] = static_cast<int>(f);
            }

            // Add new data to the ring buffer
            zoomRingBuffer[zoomIndex] = data;
            zoomIndex = (zoomIndex + 1) % zoomRingBuffer.size();

            // Create a texture from the ring buffer
            std::vector<u_int8_t> zoomTextureData(M * 128 * 3);
            for (int i = 0; i < 128; ++i) {
                for (int j = 0; j < M; ++j) {
                    int index = ((zoomIndex + i) % 128) * M + j;
                    int value = zoomRingBuffer[index / M][index % M];
                    ImVec4 color = gradient[value];
                    zoomTextureData[(i * M + j) * 3 + 0] = static_cast<u_int8_t>(color.x * 255); // Red
                    zoomTextureData[(i * M + j) * 3 + 1] = static_cast<u_int8_t>(color.y * 255); // Green
                    zoomTextureData[(i * M + j) * 3 + 2] = static_cast<u_int8_t>(color.z * 255); // Blue
                }
            }

            // Update texture data
            glBindTexture(GL_TEXTURE_2D, zoomTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, M, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, zoomTextureData.data());
            glGenerateMipmap(GL_TEXTURE_2D);

            // Use the shader program
            glUseProgram(zoomShaderProgram);

            // Bind the texture to the shader
            glActiveTexture(GL_TEXTURE0); // Use texture unit 0
            glBindTexture(GL_TEXTURE_2D, zoomTexture);
            glUniform1i(glGetUniformLocation(zoomShaderProgram, "texture1"), 0);

            ImGui::SliderInt("Offset", &zoomOffset, 0, N-M);
            ImGui::Image(zoomTexture, ImVec2(widgetSize.x, widgetSize.y), ImVec2(0, 1), ImVec2(1, 0));
        }
    }

    ImGui::End();
}

void gui::renderMain(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Waterfall", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if(connected == true) {
        window = ImGui::GetCurrentWindow();
        auto widgetPos = ImGui::GetWindowContentRegionMin();
        auto widgetEndPos = ImGui::GetWindowContentRegionMax();
        widgetPos.x += window->Pos.x;
        widgetPos.y += window->Pos.y;
        widgetEndPos.x += window->Pos.x; 
        widgetEndPos.y += window->Pos.y;
        auto widgetSize = ImVec2(widgetEndPos.x - widgetPos.x, widgetEndPos.y - widgetPos.y);

        std::array<int, 4096> data;

        double ofs = 20.0f * ::log10f(1.0f / N);
        double mult = (10.0f / ::log2f(10.0f));
        for(int i = 0; i < N; i++) {

            double f = (
                mult * log2f(
                    fftBuffer[i][0]*fftBuffer[i][0] + fftBuffer[i][1]*fftBuffer[i][1]
                ) + ofs 
            );
            
            data[i] = static_cast<int>(f);
            //std::cout << f << " : " << static_cast<int>(data[i]) << std::endl;
        }

        // Add new data to the ring buffer
        waterfallRingBuffer[waterfallIndex] = data;
        waterfallIndex = (waterfallIndex + 1) % waterfallRingBuffer.size();

        // Create a texture from the ring buffer
        std::vector<u_int8_t> waterfallTextureData(N * 256 * 3);
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < N; ++j) {
                int index = ((waterfallIndex + i) % 256) * N + j;
                int value = waterfallRingBuffer[index / N][index % N];
                ImVec4 color = gradient[value];
                waterfallTextureData[(i * N + j) * 3 + 0] = static_cast<u_int8_t>(color.x * 255); // Red
                waterfallTextureData[(i * N + j) * 3 + 1] = static_cast<u_int8_t>(color.y * 255); // Green
                waterfallTextureData[(i * N + j) * 3 + 2] = static_cast<u_int8_t>(color.z * 255); // Blue
            }
        }

        // Update texture data
        glBindTexture(GL_TEXTURE_2D, waterfallTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, waterfallTextureData.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        // Use the shader program
        glUseProgram(waterfallShaderProgram);

        // Bind the texture to the shader
        glActiveTexture(GL_TEXTURE1); // Use texture unit 1
        glBindTexture(GL_TEXTURE_2D, waterfallTexture);
        glUniform1i(glGetUniformLocation(waterfallShaderProgram, "texture2"), 1);

        ImGui::Image(waterfallTexture, ImVec2(widgetSize.x, widgetSize.y), ImVec2(0, 1), ImVec2(1, 0));

        // Handle mouse click events on the waterfall image
        if (ImGui::IsItemClicked()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float relativeX = mousePos.x - widgetPos.x;
            int newOffset = static_cast<int>((relativeX / widgetSize.x) * N) - 64; // Center the zoom window around the click
            zoomOffset = std::max(0, std::min(newOffset, static_cast<int>(N) - 128)); // Ensure the offset is within valid range
        }

        int offset1 = zoomOffset;
        int offset2 = zoomOffset+(128); // TODO: 128=M

        offset1 = widgetPos.x + offset1 * (widgetSize.x / N);
        offset2 = widgetPos.x + offset2 * (widgetSize.x / N);

        window->DrawList->AddRectFilled(ImVec2(offset1, widgetPos.y), ImVec2(offset2, widgetEndPos.y), IM_COL32(0, 200, 0, 50));
        window->DrawList->AddRect(ImVec2(offset1, widgetPos.y), ImVec2(offset2, widgetEndPos.y), IM_COL32(0, 200, 0, 255));
    }

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

    waterfallShaderProgram = glCreateProgram();
    glAttachShader(waterfallShaderProgram, vertexShader);
    glAttachShader(waterfallShaderProgram, fragmentShader);
    glLinkProgram(waterfallShaderProgram);

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

void gui::initZoom() {
    glGenTextures(1, &zoomTexture);

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
    uniform sampler2D texture2;
    void main() {
        FragColor = texture(texture2, TexCoord);
    }
    )";

    // Compile and link shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    zoomShaderProgram = glCreateProgram();
    glAttachShader(zoomShaderProgram, vertexShader);
    glAttachShader(zoomShaderProgram, fragmentShader);
    glLinkProgram(zoomShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate texture
    glBindTexture(GL_TEXTURE_2D, zoomTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    zoomIndex = 0;
    zoomOffset = 0;
}
            
void gui::prepareGradient()
{
    //      yellow
    //   max ---- e.g. -80 dB
    //        y2w                aSize
    //     a ---- e.g. -85 dB
    //       white 
    //        
    //        to                 cSize 
    //        
    //       blue
    //     b ---- e.g. -100 dB
    //        b2b                bSize
    //   min ---- e.g. -105 dB
    //       black

    int dist = max - min;
    int aSize = dist/10;
    int bSize = dist/10;
    int cSize = dist-aSize-bSize;
    int a = max - aSize;
    int b = min + bSize;
    
    gradient.clear();

    std::cout<< "Prepare Gradient" << std::endl;
    for (int i = 0; i >= -dynamicRange; i--) {
        if(i > max) { // Yellow
            gradient[i] = ImVec4(1.0f, 1.0f, 0.f, 1.0f); 
        } else if(i > a) { // Yellow to White
            float j = (max - static_cast<float>(i))/aSize;
            gradient[i] = ImVec4(1.0f, 1.0f, 1.0f*j, 1.0f); 
        } else if(i > b) { // White to Blue
            float j = (a - static_cast<float>(i))/cSize;
            gradient[i] = ImVec4(1.0f-1.0f*j, 1.0f-1.0f*j, 1.0f, 1.0f); 
        } else if(i > min) { // Blue to Black
            float j = (b - static_cast<float>(i))/bSize;
            gradient[i] = ImVec4(0.0f, 0.0f, 1.0f-1.0f*j, 1.0f); 
        } else { // Black
            gradient[i] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); 
        }
    }
}
