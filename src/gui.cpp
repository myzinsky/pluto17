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
    // Setup Callbacks
    this->connectCallback = connectCallback;
    this->isConnectedCallback = isConnectedCallback;
    this->fftBuffer = fftBuffer;

    connected = false;

    // Calculate dynamic range
    dynamicRange = static_cast<uint64_t>((14.0f+log2f(N))*6);
    initWaterfall();
    initZoom();
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
        if(connected==true) {
            window = ImGui::GetCurrentWindow();
            auto widgetPos = ImGui::GetWindowContentRegionMin();
            auto widgetEndPos = ImGui::GetWindowContentRegionMax();
            widgetPos.x += window->Pos.x;
            widgetPos.y += window->Pos.y;
            widgetEndPos.x += window->Pos.x; 
            widgetEndPos.y += window->Pos.y;
            auto widgetSize = ImVec2(widgetEndPos.x - widgetPos.x, 128);

            std::array<int, 512> data;
            int M = 512;
            //std::generate(data.begin(), data.end(), []() { return static_cast<int>(0); });
            for(int i = 0; i < M; i++) {
                data[i] = static_cast<int>(-i);
            }

            // Add new data to the ring buffer
            zoomRingBuffer[zoomIndex] = data;
            zoomIndex = (zoomIndex + 1) % zoomRingBuffer.size();

            // Create a texture from the ring buffer
            std::vector<u_int8_t> textureData(M * 256 * 3);
            for (int i = 0; i < 256; ++i) {
                for (int j = 0; j < M; ++j) {
                    int index = ((zoomIndex + i) % 256) * M + j;
                    int value = zoomRingBuffer[index / M][index % M];
                    ImVec4 color = gradient[value];
                    textureData[(i * M + j) * 3 + 0] = static_cast<u_int8_t>(color.x * 255); // Red
                    textureData[(i * M + j) * 3 + 1] = static_cast<u_int8_t>(color.y * 255); // Green
                    textureData[(i * M + j) * 3 + 2] = static_cast<u_int8_t>(color.z * 255); // Blue
                }
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
            glGenerateMipmap(GL_TEXTURE_2D);

            // Use the shader program
            glUseProgram(zoomShaderProgram);

            ImGui::Image(zoomTexture, ImVec2(widgetSize.x, widgetSize.y), ImVec2(0, 1), ImVec2(1, 0));
        }
    }

    ImGui::End();
}

void gui::renderWaterfall(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Waterfall", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if(connected == true) {
        window = ImGui::GetCurrentWindow();
        auto widgetPos = ImGui::GetWindowContentRegionMin();
        auto widgetEndPos = ImGui::GetWindowContentRegionMax();
        widgetPos.x += window->Pos.x;
        widgetPos.y += window->Pos.y;
        widgetEndPos.x += window->Pos.x; // Padding
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
        std::vector<u_int8_t> textureData(N * 256 * 3);
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < N; ++j) {
                int index = ((waterfallIndex + i) % 256) * N + j;
                int value = waterfallRingBuffer[index / N][index % N];
                ImVec4 color = gradient[value];
                textureData[(i * N + j) * 3 + 0] = static_cast<u_int8_t>(color.x * 255); // Red
                textureData[(i * N + j) * 3 + 1] = static_cast<u_int8_t>(color.y * 255); // Green
                textureData[(i * N + j) * 3 + 2] = static_cast<u_int8_t>(color.z * 255); // Blue
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        // Use the shader program
        glUseProgram(waterfallShaderProgram);

        ImGui::Image(waterfallTexture, ImVec2(widgetSize.x, widgetSize.y), ImVec2(0, 1), ImVec2(1, 0));

        window->DrawList->AddLine(ImVec2(widgetPos.x, widgetPos.y), ImVec2(widgetEndPos.x, widgetEndPos.y), IM_COL32(255, 255, 255, 255), 1.0);
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
    max = -65;
    min = -80;
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

    prepareGradient();
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
