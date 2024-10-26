#include "dsp.h"

fft::fft(uint64_t N) : N(N)
{
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    shifted = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

fft::~fft()
{
    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);
}

void fft::processSamples()
{
    hammingWindow();
    fftw_execute(p);
    shiftFft();
}

void fft::shiftFft()
{
    // Perform an FFT-Shift to bring the zero frequency component to the center
    for (uint64_t i = 0; i < N / 2; ++i) {
        std::swap(out[i], out[i + N / 2]);
    }
}

void fft::hammingWindow()
{
    // https://de.wikipedia.org/wiki/Fensterfunktion#Hamming-Fenster
    for(uint64_t i = 0; i < N; i++) {
        in[i][0] *= 0.54 - 0.46 * cos(2 * M_PI * i / N);
        in[i][1] *= 0.54 - 0.46 * cos(2 * M_PI * i / N);
    }
}

ssb::ssb(uint64_t N) : N(N)
{
    // Mixer:
    mixer = nco_crcf_create(LIQUID_VCO);
    //nco_crcf_set_frequency(mixer, ); //TDOD continue heare!

    // Rational rate resampler with low-pass filter:
    unsigned int sample_rate_raw = 576'000; // TODO: Get from pluto
    unsigned int sample_rate_wav = 48'000;
    unsigned int gcd = liquid_gcd(sample_rate_raw, sample_rate_wav);
    Q = sample_rate_raw / gcd; // input rate
    P = sample_rate_wav / gcd; // output rate
    unsigned int m = 12; // filter semi-length
    float bw = 3.0e3f / (float)sample_rate_raw; // filter bandwidth
    float As = 40.0f; // stop-band suppression [dB]
    buf_len = P > Q ? P : Q;
    std::cout << "XXXXXXXX " << gcd << " " << buf_len << std::endl;
    resamp = rresamp_crcf_create_kaiser(P,Q,m,bw,As);
    rresamp_crcf_print(resamp);

    // allocate buffers for sample processing (two each complex and real)
    buf_0 = new std::complex<float>[buf_len];
    buf_1 = new std::complex<float>[buf_len];
    buf_2 = new float[buf_len];
    out = new float[buf_len];

    // Demodulator:
    float mod_index = 0.1f; // Modulation index (bandwidth)
    liquid_ampmodem_type type = LIQUID_AMPMODEM_USB; 
    int suppressed = 1; // Suppressed carrier
    demod = ampmodem_create(mod_index, type, suppressed);

    // AGC:
    agc = agc_rrrf_create();
    agc_rrrf_set_bandwidth(agc, 400.0f / (float)sample_rate_wav); // agc response
    agc_rrrf_set_scale(agc, 0.05f); // output audio scale (avoid clipping)
}

ssb::~ssb()
{
    nco_crcf_destroy(mixer);
    rresamp_crcf_destroy(resamp);
    ampmodem_destroy(demod);
}

// https://gist.github.com/jgaeddert/846e781dbef25fd396f577d038e7d430
void ssb::demodulate(uint64_t carrier)
{ 
    // Set the mixer frequency
    //int nco_crcf_adjust_frequency(mixer, 0.0 );

    // carrier/N*2*M_PI
    float f = carrier/N*2*M_PI; //(4.0*M_PI*(float)(carrier) / 576'000.0f)-2*M_PI;
    nco_crcf_set_frequency(mixer, f);

    // mix down 'Q' samples
    nco_crcf_mix_block_down(mixer, buf_0, buf_1, Q);

    // resample 'Q' samples in buf_1 into 'P' samples in buf_0
    rresamp_crcf_execute(resamp, buf_1, buf_0);

    // perform amplitude demodulation
    ampmodem_demodulate_block(demod, buf_0, P, buf_2);

    // apply automatic gain control
    agc_rrrf_execute_block(agc, buf_2, P, out);

    // Demodulate the signal
    //for (int i=0; i<N; i++) {
    //    ampmodem_demodulate(demod, in[i], &out[i]);
    //}

}

audio::audio(float* buffer) : buffer(buffer)
{
    err = Pa_Initialize();
    if (err != paNoError) {
        throw std::runtime_error("PortAudio error: (1) " + std::string(Pa_GetErrorText(err)));
    }

    err = Pa_OpenDefaultStream(&stream,
                               0,          // no input channels
                               1,          // mono output
                               paFloat32,  // 32 bit floating point output
                               48000,      // sample rate
                               256,        // frames per buffer
                               nullptr,    // no callback, use blocking API
                               nullptr);   // no callback userData
    if (err != paNoError) {
        Pa_Terminate();
        throw std::runtime_error("PortAudio error: (2) " + std::string(Pa_GetErrorText(err)));
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        Pa_CloseStream(stream);
        Pa_Terminate();
        throw std::runtime_error("PortAudio error: (3) " + std::string(Pa_GetErrorText(err)));
    }
}

audio::~audio()
{
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}

void audio::playback(uint64_t N)
{
    err = Pa_WriteStream(stream, buffer, N);
    //if (err != paNoError) {
    //    Pa_StopStream(stream);
    //    Pa_CloseStream(stream);
    //    Pa_Terminate();
    //    throw std::runtime_error("PortAudio error: (6) " + std::string(Pa_GetErrorText(err)));
    //}

}