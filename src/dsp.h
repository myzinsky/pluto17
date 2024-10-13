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

    static uint64_t bucketToFrequency(uint64_t bucketId, uint64_t N)
    {
        uint64_t baseQrg = 10'489'750'000; // Center frequency in Hz
        uint64_t sampleRate = 576'000; // Sample rate in samples per second
        uint64_t frequencyResolution = sampleRate / N;
        int64_t frequencyOffset = static_cast<int64_t>(bucketId) - static_cast<int64_t>(N/2);
        uint64_t frequency = baseQrg + frequencyOffset * frequencyResolution;
        return frequency;
    }

    static uint64_t frequencyToBucket(uint64_t frequency, uint64_t N)
    {
        int64_t baseQrg = 10'489'750'000; // Center frequency in Hz
        int64_t sampleRate = 576'000; // Sample rate in samples per second
        int64_t frequencyResolution = sampleRate/N;
        int64_t frequencyOffset = static_cast<int64_t>(frequency) - baseQrg;
        int64_t bucketId = (frequencyOffset / frequencyResolution) + static_cast<int64_t>(N)/2;
        return static_cast<uint64_t>(bucketId);
    }

    fftw_complex *in;
    fftw_complex *out;
    fftw_complex *shifted;

    private:
    fftw_plan p;
    uint64_t N;
};;

#endif