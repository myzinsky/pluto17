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