#include "SerialThrustMasterBase.hh"
#include "IOException.hh"
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sstream>

using namespace idf;

SerialThrustMasterBase::SerialThrustMasterBase(const char *terminalPath, bool isMale) :
    ThrustMasterBase(isMale),
    SerialDevice(terminalPath) {}

void SerialThrustMasterBase::open() {
    SerialDevice::open();

    struct termios settings;
    tcgetattr(handle, &settings);
    cfsetspeed(&settings, B9600);
    settings.c_cflag |= (CLOCAL | CREAD);
    settings.c_cflag &= ~PARENB;
    settings.c_cflag &= ~CSTOPB;
    settings.c_cflag &= ~CSIZE;
    settings.c_cflag |= CS8;

    // Set a timeout of 0.1 seconds
    settings.c_lflag &= ~ICANON;
    settings.c_cc[VTIME] = 1;
    settings.c_cc[VMIN] = 0;

    if (tcsetattr(handle, TCSANOW, &settings) == -1) {
        std::ostringstream oss;
        oss << __FILE__ << ":" << __LINE__
            << " Failed to set serial port attributes: " << strerror(errno);
        throw IOException(oss.str());
    }
}

void SerialThrustMasterBase::update() {
    SerialDevice::update();

    char request = 'r';

    write(&request, sizeof(request));

    int bytesRemaining = 9;
    unsigned char buffer[bytesRemaining];

    while(bytesRemaining) {
        int bytesRead = read(buffer + (9 - bytesRemaining), bytesRemaining);
        if (bytesRead == 0) {
            std::ostringstream oss;
            oss << __FILE__ << ":" << __LINE__
                << " Timeout while reading";
            throw IOException(oss.str());
        }
        bytesRemaining -= bytesRead;
    }

    forwardBackwardPivot.setValue(buffer[0]);
    twist.setValue(buffer[1]);
    leftRightPivot.setValue(buffer[2]);
    leftRightTranslation.setValue(buffer[3]);
    upDownTranslation.setValue(buffer[4]);
    forwardBackwardTranslation.setValue(buffer[5]);
    trigger.setValue(buffer[8] & 1);

    processButtons(buffer[8]);
}