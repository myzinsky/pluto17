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

    fftw_complex *in;
    fftw_complex *out;

    private:
    fftw_plan p;
    uint64_t N;
};

#endif