#ifndef BMP_H
#define BMP_H

#include <Adafruit_BMP280.h>

class BMP {
public:
    BMP(int csPin);
    bool begin();
    float readPressure();
    float readTemperature();

private:
    Adafruit_BMP280 bmp;
};

#endif
