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
    //std::cout<< "Processing samples with FFT" << std::endl;
    fftw_execute(p);
    shiftFft();
}

void fft::shiftFft()
{
    for (uint64_t i = 0; i < N / 2; ++i) {
        std::swap(out[i], out[i + N / 2]);
    }
}