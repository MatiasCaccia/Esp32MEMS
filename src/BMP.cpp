#include "BMP.h"

BMP::BMP(int csPin) : bmp(csPin) {}

bool BMP::begin() {
    return bmp.begin();
}

float BMP::readPressure() {
    return bmp.readPressure();
}

float BMP::readTemperature() {
    return bmp.readTemperature();
}




