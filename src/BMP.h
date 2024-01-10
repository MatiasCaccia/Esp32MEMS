#ifndef BMP_H
#define BMP_H

#include <Adafruit_BMP085.h>
#include <SPI.h>
class BMP {
public:
    BMP();
    bool begin();
    float readPressure();
    float readTemperature();

private:
    Adafruit_BMP085 bmp;
};

#endif
