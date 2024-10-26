#include "gui.h"

gui::gui(
    std::function<bool()> connectCallback,
    std::function<bool()> fakeConnectCallback,
    std::function<bool()> isConnectedCallback,
    fftw_complex* fftBuffer,
    uint64_t *carrier,
    uint64_t N
) : waterfallRingBuffer(256),
    spectrumHistory(100, std::vector<float>(N, 0.0f)),
    carrier(carrier),
    N(N)
{
    // Calculate dynamic range (14 bit ADC of Pluto and log2(N) bits for FFT, for each bit we have 6 dB gain)
    dynamicRange = static_cast<uint64_t>((14.0f+log2f(N))*6);

    // Initialize waterfallRingBuffer with -1 * dynamicRange
    for (auto& buffer : waterfallRingBuffer) {
        buffer.fill(-1 * dynamicRange);
    }

    // Prepare the Gradient:
    max = -58;
    min = -85;
    dmax = static_cast<double>(max);
    dmin = static_cast<double>(min);
    prepareGradient();
    historyIndex=0;

    // Generate Frequency Caption for Waterfall:
    for(int i = 0; i < 4096; i++) {
        frequencyBins[i] = fft::bucketToFrequency(i, N) / 1'000'000.0;
    }

    filterStart = (fft::bucketToFrequency(4096/2, N) / 1'000'000.0) - (filterWidth / 1'000'000.0)/2.0;
    filterEnd = (fft::bucketToFrequency(4096/2, N) / 1'000'000.0) + (filterWidth / 1'000'000.0)/2.0;

    // Initialize the OpenGL Shaders
    initWaterfall();

    // Setup Callbacks
    this->connectCallback = connectCallback;
    this->fakeConnectCallback = fakeConnectCallback;
    this->isConnectedCallback = isConnectedCallback;
    this->fftBuffer = fftBuffer;

    connected = false;

    filterWidth = 3'000.0;
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

        if (ImGui::Button("Fake Connect")) {
            if(fakeConnectCallback()){
                std::cout << "Connected to Fake" << std::endl;
                connected = true;
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
            ImPlot::SetNextAxesLimits(
                fft::bucketToFrequency(zoomOffset, N) / 1'000'000.0,
                fft::bucketToFrequency(zoomOffset+127, N) / 1'000'000.0,
                0,
                127,
                ImPlotCond_Always
            );
            if(ImPlot::BeginPlot("", ImVec2(-1,128))) {
                ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
                ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
                ImPlot::PlotImage(
                    "", // Waterfall
                    static_cast<intptr_t>(waterfallTexture),
                    ImVec2(fft::bucketToFrequency(0, N) / 1'000'000.0, 128),
                    ImVec2(fft::bucketToFrequency(4095, N) / 1'000'000.0, 0)
                );
                double x1 = (fft::bucketToFrequency(zoomOffset+63, N) - filterWidth/2.0) / 1'000'000.0;
                double x2 = (fft::bucketToFrequency(zoomOffset+63, N) + filterWidth/2.0) / 1'000'000.0;
                double y1 = 0;
                double y2 = 128;
                ImPlot::DragRect(0, &x1, &y1, &x2, &y2, ImVec4(0.0, 0.78, 0.0, 0.75), ImPlotDragToolFlags_NoInputs);
                ImPlot::EndPlot();
            }
        }
    }

    ImGui::End();
}

void gui::renderMain(float width, float height, float xoffset) {
    ImGui::SetNextWindowPos(ImVec2(xoffset, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height), ImGuiCond_Always);
    ImGui::Begin("Main Control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if(connected == true) {
        // Get Window Sizes:
        window = ImGui::GetCurrentWindow();
        auto widgetPos = ImGui::GetWindowContentRegionMin();
        auto widgetEndPos = ImGui::GetWindowContentRegionMax();
        widgetPos.x += window->Pos.x;
        widgetPos.y += window->Pos.y;
        widgetEndPos.x += window->Pos.x; 
        widgetEndPos.y += window->Pos.y;
        auto areaSize = ImVec2(widgetEndPos.x - widgetPos.x, (widgetEndPos.y - widgetPos.y));
        auto spectrumSize = ImVec2(areaSize.x, 200);
        auto bandplanSize = ImVec2(areaSize.x, 8);
        auto waterfallSize = ImVec2(areaSize.x, areaSize.y-220);

        // Prepare FFT data:
        std::array<int,4096> data;
        std::vector<float> spectrumData(N);
        for(int n = 0; n < N; n++) {

            // Normalization:
            double i = fftBuffer[n][0] / static_cast<double>(N); 
            double q = fftBuffer[n][1] / static_cast<double>(N); 

            // Calculate the absolute value:
            double f = sqrt(i*i + q*q);

            // Calculate the power in dBFS:
            double p = 20.0 * log10(f);

            //if (n>2500 && n<2510) {
            //    p = 0.0;
            //} else {
            //    p = -120.0;
            //}
            
            data[n] = static_cast<int>(p);
            spectrumData[n] = static_cast<float>(p);
        }

        // Average Spectrogram Data:
        // Add current spectrum data to history
        spectrumHistory[historyIndex] = spectrumData;
        historyIndex = (historyIndex + 1) % 100;

        // Calculate the average spectrum data
        std::vector<float> averagedSpectrumData(N, 0.0f);
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < 100; ++j) {
            averagedSpectrumData[i] += spectrumHistory[j][i];
            }
            averagedSpectrumData[i] /= 100.0f;
        }

        ImPlotSubplotFlags flags = ImPlotSubplotFlags_LinkAllX;
        float rowRatios[] = {8,1,8};
        if(ImPlot::BeginSubplots("", 3, 1, areaSize, flags, rowRatios)) { // Plots

            // Setup Spectrogram Plot:
            ImPlot::SetNextAxesLimits(fft::bucketToFrequency(0, N) / 1'000'000.0, fft::bucketToFrequency(4095, N) / 1'000'000.0, min, max, ImPlotCond_Always);
            if (ImPlot::BeginPlot("")) { // Spectrum
                ImPlot::SetupAxisFormat(ImAxis_Y1, "%g dB");
                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f MHz");
                ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_Opposite);
                ImPlot::PlotLine("Spectrum", frequencyBins.data(), averagedSpectrumData.data(), 4096);
                dragVFO();
                ImPlot::EndPlot();
            }

            // Setup Bandplan Plot:
            ImPlot::SetNextAxesLimits(fft::bucketToFrequency(0, N) / 1'000'000.0, fft::bucketToFrequency(4095, N) / 1'000'000.0, 0, 10, ImPlotCond_Always);
            if (ImPlot::BeginPlot("")) { // Spectrum
                ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
                ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);

                // Insert Band Plan:
                for (const auto& segment : bandplan) {
                    double x1 = segment.startFreq / 1'000'000.0;
                    double x2 = segment.endFreq / 1'000'000.0;
                    double y1 = 0;
                    double y2 = 10;
                    ImPlot::DragRect(0, &x1, &y1, &x2, &y2, segment.color, ImPlotDragToolFlags_NoInputs);
                    ImPlot::PlotText(segment.label, x1+(x2-x1)/2, y2-(y2-y1)/2.0); 
                }
                
                ImPlot::EndPlot();
            }

            // Setup Waterfall Plot:
            ImPlot::SetNextAxesLimits(fft::bucketToFrequency(0, N) / 1'000'000.0, fft::bucketToFrequency(4095, N) / 1'000'000.0, 0, 255, ImPlotCond_Always);
            if (ImPlot::BeginPlot("")) { // Waterfall
                // Setup Axis Format:
                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f MHz");
                ImPlot::SetupAxis(ImAxis_Y1, "Time", ImPlotAxisFlags_NoTickLabels);

                // Add new data to the ring buffer:
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


                ImPlot::PlotImage(
                    "", // Waterfall
                    static_cast<intptr_t>(waterfallTexture),
                    ImVec2(fft::bucketToFrequency(0, N) / 1'000'000.0, 255),
                    ImVec2(fft::bucketToFrequency(4095, N) / 1'000'000.0, 0)
                );

                dragVFO();
                renderVFO(height);
                ImPlot::EndPlot();
            }
            ImPlot::EndSubplots();
        } 
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
    zoomOffset = N/2-64;
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

    dmax = static_cast<double>(max);
    dmin = static_cast<double>(min);

    int dist = max - min;
    int aSize = dist/10;
    int bSize = dist/10;
    int cSize = dist-aSize-bSize;
    int a = max - aSize;
    int b = min + bSize;
    
    gradient.clear();

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

void gui::renderVFO(float height) {
    ImPlotPoint plotPos1 = ImPlot::PlotToPixels(filterStart, dmax);
    ImPlotPoint plotPos2 = ImPlot::PlotToPixels(filterEnd, dmin);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(plotPos1.x,0), ImVec2(plotPos2.x,height), IM_COL32(0, 200, 0, 50));
    ImGui::GetWindowDrawList()->AddRect(ImVec2(plotPos1.x,0), ImVec2(plotPos2.x,height), IM_COL32(0, 200, 0, 255));
}

void gui::dragVFO()
{
    if (ImPlot::IsPlotHovered() && (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left))) {
        ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
        filterStart = mousePos.x - (filterWidth / 1'000'000.0)/2.0;
        filterEnd = mousePos.x + (filterWidth / 1'000'000.0)/2.0;
        int newOffset = static_cast<int>(fft::frequencyToBucket(mousePos.x*1'000'000.0, N)) - 64; 
        zoomOffset = std::max(0, std::min(newOffset, static_cast<int>(N) - 128)); 
    }
}
