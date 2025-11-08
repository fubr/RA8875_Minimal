#ifndef RA8875_MINIMAL_H
#define RA8875_MINIMAL_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// FT5206 I2C address
#define FT5206_ADDR 0x38

class RA8875_Minimal {
public:
    RA8875_Minimal(uint8_t cs, uint8_t rst = 255);
    
    bool begin(uint16_t displayType);
    void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* data);
    
    // Touch functions
    bool touched();
    void updateTS();
    void getTScoordinates(uint16_t coords[2][2]);
    void useCapINT(uint8_t pin);
    void enableCapISR(bool state);
    
    // Display control
    void backlight(bool on);
    void displayOn(bool on);
    void setRotation(uint8_t rotation);
    
    // Constants for display type
    static constexpr uint16_t RA8875_480x272 = 0;
    static constexpr uint16_t RA8875_800x480 = 1;
    
private:
    uint8_t _cs, _rst;
    uint8_t _rotation;
    uint16_t _width, _height;
    uint8_t _intPin = 255;
    volatile bool _touchDetected;
    TwoWire* _wire;
    
    // Touch state caching
    bool _lastTouchState = false;
    uint8_t _currentTouches = 0;     // Current number of touches
    uint16_t _cachedCoords[2][2]; // Cache coordinates for getTScoordinates()
    
    // Basic SPI communication
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    uint8_t readData();
    uint8_t readStatus();
    void initialize(uint16_t displayType);
    
    // Touch controller
    void touchInit();
    uint16_t touchReadReg(uint8_t reg);
};

#endif