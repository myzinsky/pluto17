#include "pluto.h"

pluto::pluto(uint64_t N) : N(N)
{
    std::cout << "Pluto created" << std::endl;

    connected = false;

    // TODO: Check right values:
    sampleRate = 576'000;
    baseQrg = 10'489'750'000;
    rxOffset = 9'749'975'946;
    baseQrgRx = baseQrg - rxOffset;
    std::cout << "baseQrgRx = " << baseQrgRx << std::endl;
    baseQrgTx = double(2400000UL - 30UL);
    bandwidthRx = 576'000;
    bandwidthTx = 100'000;
    rxBuffer = nullptr;
    txBuffer = nullptr;

    fourier = new fft(N);
}

iio_scan_context* pluto::getScanContext()
{
    return iio_create_scan_context(nullptr,0);
}

iio_context* pluto::getContext(iio_scan_context *scanContext)
{
    struct iio_context_info **info;
    int ret = iio_scan_context_get_info_list(scanContext, &info);
    return iio_create_context_from_uri("ip:192.168.178.85");
}

iio_device* pluto::getDevice(iio_context* context)
{
    return iio_context_find_device(context, "ad9361-phy");
}

bool pluto::getStreamDevice(iio_context *context, iodev d, iio_device **device)
{
    if(d == TX) {
        *device = iio_context_find_device(context, "cf-ad9361-dds-core-lpc");
        return *device != nullptr;
    } else if (d == RX) { 
        *device = iio_context_find_device(context, "cf-ad9361-lpc");
        return *device != nullptr;
    }
    return false;
}

bool pluto::configureChannel(iio_context *context, int64_t bandwidth, int64_t sampleRate, double baseQrg, std::string port, iodev type, int chid)
{
    iio_channel *channel = nullptr;

    if(!getPhyConfigChannel(context, type, chid, &channel)) {
        return false;
    }

    writeToChannel(channel, "rf_port_select", port.c_str());
    writeToChannel(channel, "rf_bandwidth", bandwidth);
    writeToChannel(channel, "sampling_frequency", sampleRate);

    if(type == TX) {
        setTxPower(0.0);
        setTxQrg(baseQrgRx); // TODO set TX QRG
    } else {
        writeToChannel(channel, "gain_control_mode", "slow_attack");
        setRxQrg(baseQrgRx);
    }

    return true;
}

// Finds AD9361 phy IIO configuration channel with id chid
bool pluto::getPhyConfigChannel(iio_context *context, iodev d, int chid, iio_channel **channel) 
{
    if(d == RX) {
        *channel = iio_device_find_channel(getDevice(context), getChannelName("voltage", chid).c_str(), false);
        return *channel != NULL;
    } else if (d == TX) {
        *channel = iio_device_find_channel(getDevice(context), getChannelName("voltage", chid).c_str(), true);
        return *channel != NULL;
    }
    return false;
}

std::string pluto::getChannelName(const char* type, int id) 
{
    std::stringstream ss;
    ss << type << id;
    std::cout << ss.str() << std::endl;
    return ss.str();
}

void pluto::writeToChannel(iio_channel *channel, std::string what, std::string val)
{
    iio_channel_attr_write(channel, what.c_str(), val.c_str());
}

void pluto::writeToChannel(iio_channel *channel, std::string what, double val)
{
    iio_channel_attr_write_double(channel, what.c_str(), val);
}

void pluto::writeToChannel(iio_channel *channel, std::string what, int64_t val)
{
    iio_channel_attr_write_longlong(channel, what.c_str(), val);
}

bool pluto::setTxPower(double power) {
    struct iio_channel *channel = nullptr;
    if (!getPhyConfigChannel(context, TX, 0, &channel)) {
        return false;
    }

    writeToChannel(channel, "hardwaregain", power);
    return true;
}

bool pluto::setTxQrg(int64_t qrg)
{
    struct iio_channel *channel = nullptr;
    if (!getLocalOscillatorChannel(context, TX, &channel)) {
        return false;
    }
    writeToChannel(channel, "frequency", qrg);
    return true;
}

bool pluto::setRxQrg(int64_t qrg)
{
    struct iio_channel *channel = nullptr;
    if (!getLocalOscillatorChannel(context, RX, &channel)) {
        return false;
    }
    writeToChannel(channel, "frequency", qrg);
    return true;
}

// Finds AD9361 local oscillator IIO configuration channels 
bool pluto::getLocalOscillatorChannel(iio_context *context, iodev d, iio_channel **channel) 
{
    if(d == RX) {
        *channel = iio_device_find_channel(getDevice(context), getChannelName("altvoltage", 0).c_str(), true);
        return *channel != NULL;
    } else if (d == TX) {
        *channel = iio_device_find_channel(getDevice(context), getChannelName("altvoltage", 1).c_str(), true); 
        return *channel != NULL;
    }
    return false;
}

std::string pluto::getChannelNameModify(const char* type, int id, char modify) 
{
    std::stringstream ss;
    ss << type << id << "_" << modify;
    std::cout << ss.str() << std::endl;
    return ss.str();
}

bool pluto::getStreamChannel(iio_context *context, iodev d, iio_device *device, int chid, iio_channel **channel) {
    *channel = iio_device_find_channel(device, getChannelName("voltage", chid).c_str(), d == TX);
    if (!*channel)
        *channel = iio_device_find_channel(device, getChannelName("altvoltage", chid).c_str(), d == TX);
    return *channel != NULL;
}

// Connect Method:
bool pluto::connect()
{
    // Find Pluto:
    auto scanContext = getScanContext();

    if (scanContext == nullptr) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to create scan context" << std::endl;
        return false;
    }

    context = nullptr;
    context = getContext(scanContext);

    if(context == nullptr) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to create context" << std::endl;
        return false;
    }

    // Get TX and RX Stream Devices:
    if(!getStreamDevice(context, TX, &tx)) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to find TX steraming device" << std::endl;
        return false;
    }

    if(!getStreamDevice(context, RX, &rx)) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to find RX steraming device" << std::endl;
        return false;
    }


    if(!configureChannel(context, bandwidthRx, sampleRate, baseQrg, "A_BALANCED", RX, 0)) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to configure RX channel" << std::endl;
        return false;
    }

    if(!configureChannel(context, bandwidthTx, sampleRate, baseQrg, "A", TX, 0)) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to configure RX channel" << std::endl;
        return false;
    }

    bool result = true;
    result &= getStreamChannel(context, RX, rx, 0, &rx0i);
    result &= getStreamChannel(context, RX, rx, 1, &rx0q);
    result &= getStreamChannel(context, TX, tx, 0, &tx0i);
    result &= getStreamChannel(context, TX, tx, 1, &tx0q);

    if(!result) {
        std::cout << "ERROR: Cannot connect to Pluto: Unable to find streaming channel" << std::endl;
        return false;
    }

    iio_channel_enable(rx0i);
    iio_channel_enable(rx0q);
    iio_channel_enable(tx0i);
    iio_channel_enable(tx0q);

    rxBuffer = iio_device_create_buffer(rx, N, false);
    if (!rxBuffer) {
        std::cout << "Could not create RX buffer" << std::endl;
        return false;
    }

    txBuffer = iio_device_create_buffer(tx, N, false);
    if (!txBuffer) {
        std::cout << "Could not create TX buffer" << std::endl;
        return false;
    }

    //ad9361_set_bb_rate()
    ad9361_set_bb_rate(getDevice(context), round(sampleRate));

    connected = true;
    return true;
}

bool pluto::getSamples()
{
    ssize_t numberOfRxBytes = iio_buffer_refill(rxBuffer);

    if(numberOfRxBytes < 0) {
        std::cout << "ERROR: Error in Refilling rxBuffer" << std::endl;
        return false;
    }

    uint8_t *start = (uint8_t *)iio_buffer_first(rxBuffer, rx0i);
    uint8_t *end = (uint8_t *)iio_buffer_end(rxBuffer);

    // READ: Get pointers to RX buf and read IQ from RX buf port 0
    void *p_dat;
    ptrdiff_t p_inc = iio_buffer_step(rxBuffer);
    void *p_end = iio_buffer_end(rxBuffer);
    unsigned int counter = 0;
    for (p_dat = (char *)iio_buffer_first(rxBuffer, rx0i); p_dat < p_end; p_dat = static_cast<char*>(p_dat) + p_inc) {
        const int16_t i = ((int16_t*)p_dat)[0]; // Real (I)
        const int16_t q = ((int16_t*)p_dat)[1]; // Imag (Q)

        fourier->in[counter][0] = (static_cast<double>(i)/32768.0f);
        fourier->in[counter][1] = (static_cast<double>(q)/32768.0f);
        
        counter++;
    }

    fourier->processSamples();

    return true;
}

fftw_complex* pluto::getFftBuffer()
{
    return fourier->out;
}
