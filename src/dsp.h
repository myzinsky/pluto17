#ifndef FFT_H
#define FFT_H

#include <iostream>
#include <complex>
#include <fftw3.h>

// TODO N evtl. Templaten
class fft 
{
    public:
    fft(uint64_t N = 4096);
    ~fft();
    void processSamples();
    void shiftFft();
    void hammingWindow();

    fftw_complex *in;
    fftw_complex *out;
    fftw_complex *shifted;

    private:
    fftw_plan p;
    uint64_t N;
};

#endif