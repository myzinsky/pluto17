#ifndef PLUTO_H
#define PLUTO_H

#include <iostream>
#include <sstream>
#include <iio.h>
#include <ad9361.h>
#include <fftw3.h>
#include <complex>
#include "dsp.h"

class pluto {
  public:
    
    pluto(uint64_t N = 4096);

    enum iodev { RX, TX };

    bool connect();
    bool isConnected() { return connected; }
    bool getSamples();
    uint64_t getN();

    fftw_complex* getFftBuffer();

  private:

    // Methods that encapsulate pluto access (i.e. driver):
    iio_scan_context* getScanContext();
    iio_context* getContext(iio_scan_context *scanContext);
    iio_device* getDevice(iio_context* context);
    bool getStreamDevice(iio_context *context, iodev d, iio_device **device);
    bool configureChannel(iio_context *context, int64_t bandwidth, int64_t sampleRate, double baseQrg, std::string port, iodev type, int chid);
    bool getPhyConfigChannel(iio_context *context, iodev d, int chid, iio_channel **channel);
    std::string getChannelName(const char* type, int id);
    void writeToChannel(iio_channel *channel, std::string what, std::string val);
    void writeToChannel(iio_channel *channel, std::string what, double val);
    void writeToChannel(iio_channel *channel, std::string what, int64_t val);
    bool setTxPower(double power);
    bool setTxQrg(int64_t qrg);
    bool setRxQrg(int64_t qrg);
    bool getLocalOscillatorChannel(iio_context *context, iodev d, iio_channel **channel);
    std::string getChannelNameModify(const char* type, int id, char modify);
    bool getStreamChannel(iio_context *context, iodev d, iio_device *device, int chid, iio_channel **channel);

    // Config:
    uint64_t sampleRate;
    uint64_t N;
    uint64_t lnbReference;
    uint64_t baseQrg; 
    uint64_t rxOffset; 
    double baseQrgTx;
    double baseQrgRx;
    int64_t bandwidthRx;
    int64_t bandwidthTx;
    
    // Context:
    iio_context *context;

    // Streaming devices:
    iio_device *tx;
    iio_device *rx;

    // Buffers:
    iio_buffer  *rxBuffer;
    iio_buffer  *txBuffer;

    // Channels:
    iio_channel *rx0i;
    iio_channel *rx0q;
    iio_channel *tx0i;
    iio_channel *tx0q;

    // Status: 
    bool connected;

    // Furier Wrapper:
    fft *fourier;
};

#endif