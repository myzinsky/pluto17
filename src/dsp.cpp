#include "dsp.h"

fft::fft(uint64_t N) : N(N)
{
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
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
}

//void fft::processSample(std::complex<float> sample)
//{
//    fftIn[sampleCounter++] = sample;
//
//    if(sampleCounter == (N)) {
//        sampleCounter = 0;
//        fft_execute(liquidFFT);
//
//        std::vector<std::complex<float>> result = fftShift();
//
//        // TODO: try to send waterfall directly I/Q-samples ...
//        std::vector<float> waterfallSamples;
//        for(uint64_t i = 0; i < N; i++) {
//            waterfallSamples.push_back(std::abs(result[i]));
//        }    
//
//        // Normalize:
//        float maxValue = *std::max_element(std::begin(waterfallSamples), std::end(waterfallSamples));
//        transform(waterfallSamples.begin(), waterfallSamples.end(), waterfallSamples.begin(), [maxValue](float &sample){ return sample/maxValue; });
//
//        emit notifyWaterfall(waterfallSamples);
//    }
//}
//
//std::vector<std::complex<float>> fft::fftShift() 
//{
//    std::vector<std::complex<float>> result;
//    
//    result.reserve(N);
//
//    result[N/2].imag(fftOut[0].imag());
//    result[N/2].real(fftOut[0].real());
//
//    for(uint64_t i = 1; i < N/2; i++) {
//        result[N/2 + i].imag(fftOut[i].imag());
//        result[N/2 + i].real(fftOut[i].real());
//    }
//
//    for(uint64_t i = N/2+1; i < N; i++) {
//        result[i-N/2].imag(fftOut[i].imag());
//        result[i-N/2].real(fftOut[i].real());
//    }
//    return result;
//
//}