#include "RA8875_Minimal.h"

// RA8875 Command/Data pins
#define RA8875_DATAWRITE 0x00
#define RA8875_DATAREAD  0x40
#define RA8875_CMDWRITE  0x80
#define RA8875_CMDREAD   0xC0

// Key registers
#define RA8875_PWRR      0x01  // Power control
#define RA8875_MRWC      0x02  // Memory Read/Write Command
#define RA8875_GPIOX     0xC7  // GPIO
#define RA8875_PLLC1     0x88  // PLL Control 1
#define RA8875_PLLC2     0x89  // PLL Control 2
#define RA8875_SYSR      0x10  // System Configuration
#define RA8875_HDWR      0x14  // Horizontal Display Width
#define RA8875_HNDFTR    0x15  // Horizontal Non-Display Period Fine Tuning
#define RA8875_HNDR      0x16  // Horizontal Non-Display Period
#define RA8875_HSTR      0x17  // HSYNC Start Position
#define RA8875_HPWR      0x18  // HSYNC Pulse Width
#define RA8875_VDHR0     0x19  // Vertical Display Height 0
#define RA8875_VDHR1     0x1A  // Vertical Display Height 1
#define RA8875_VNDR0     0x1B  // Vertical Non-Display Period 0
#define RA8875_VNDR1     0x1C  // Vertical Non-Display Period 1
#define RA8875_VSTR0     0x1D  // VSYNC Start Position 0
#define RA8875_VSTR1     0x1E  // VSYNC Start Position 1
#define RA8875_VPWR      0x1F  // VSYNC Pulse Width

// Touch registers for FT5206
#define RA8875_TPCR0     0x70  // Touch Panel Control 0
#define RA8875_TPCR1     0x71  // Touch Panel Control 1
#define RA8875_TPXH      0x72  // Touch X High
#define RA8875_TPYH      0x73  // Touch Y High  
#define RA8875_TPXYL     0x74  // Touch X/Y Low
#define RA8875_INTC1     0xF0  // Interrupt Control 1
#define RA8875_INTC2     0xF1  // Interrupt Control 2

// Memory write position
#define RA8875_CURH0     0x46  // Cursor Horizontal 0
#define RA8875_CURH1     0x47  // Cursor Horizontal 1
#define RA8875_CURV0     0x48  // Cursor Vertical 0
#define RA8875_CURV1     0x49  // Cursor Vertical 1

// Display settings
#define RA8875_P1CR      0x8A  // PWM1 Control
#define RA8875_P1DCR     0x8B  // PWM1 Duty Cycle
#define RA8875_MCLR      0x8E  // Memory Clear Control

// Active window registers
#define RA8875_HSAW0     0x30  // Horizontal Start Point 0 of Active Window
#define RA8875_HSAW1     0x31  // Horizontal Start Point 1 of Active Window
#define RA8875_VSAW0     0x32  // Vertical Start Point 0 of Active Window
#define RA8875_VSAW1     0x33  // Vertical Start Point 1 of Active Window
#define RA8875_HEAW0     0x34  // Horizontal End Point 0 of Active Window
#define RA8875_HEAW1     0x35  // Horizontal End Point 1 of Active Window
#define RA8875_VEAW0     0x36  // Vertical End Point 0 of Active Window
#define RA8875_VEAW1     0x37  // Vertical End Point 1 of Active Window

// Pixel Clock Setting Register
#define RA8875_PCSR      0x04  // Pixel Clock Setting

// Memory data format
#define RA8875_DPCR      0x20  // Display Configuration Register
#define RA8875_MWCR0     0x40  // Memory Write Control Register 0
#define RA8875_MWCR1     0x41  // Memory Write Control Register 1

RA8875_Minimal::RA8875_Minimal(uint8_t cs, uint8_t rst) : _cs(cs), _rst(rst) {
    _rotation = 0;
    _touchDetected = false;
    _intPin = 255;
    _width = 0;
    _height = 0;
    _wire = &Wire;  // Default to Wire, can be changed to Wire1, Wire2, etc.
    _lastTouchState = false;
    _currentTouches = 0;
    
    // Initialize cached coordinates
    _cachedCoords[0][0] = 0;
    _cachedCoords[0][1] = 0;
    _cachedCoords[1][0] = 0;
    _cachedCoords[1][1] = 0;
}
bool RA8875_Minimal::begin(uint16_t displayType) {
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    
    if (_rst != 255) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(100);
        digitalWrite(_rst, HIGH);
        delay(100);
    }
    
    SPI.begin();
    
    initialize(displayType);
    
    // Wait for PLL to stabilize
    delay(10);
    
    // Enable display
    writeCommand(RA8875_PWRR);
    writeData(0x80);
    delay(100);
    
    writeCommand(RA8875_PWRR);
    writeData(0x80);
    delay(100);
    
    // Set active window to full screen
    writeCommand(RA8875_HSAW0);
    writeData(0x00);
    writeCommand(RA8875_HSAW1);
    writeData(0x00);
    writeCommand(RA8875_VSAW0);
    writeData(0x00);
    writeCommand(RA8875_VSAW1);
    writeData(0x00);
    writeCommand(RA8875_HEAW0);
    writeData(_width & 0xFF);
    writeCommand(RA8875_HEAW1);
    writeData(_width >> 8);
    writeCommand(RA8875_VEAW0);
    writeData(_height & 0xFF);
    writeCommand(RA8875_VEAW1);
    writeData(_height >> 8);
    
    // Clear screen to white
    writeCommand(RA8875_MCLR);
    writeData(0x80);
    delay(100);
    
    // Initialize backlight
    writeCommand(RA8875_P1CR);
    writeData(0x80);  // Enable PWM1
    writeCommand(RA8875_P1DCR);
    writeData(0xFF);  // Full brightness
    
    // Initialize touch if needed
    touchInit();
    
    return true;
}

void RA8875_Minimal::initialize(uint16_t displayType) {
    if (displayType == RA8875_480x272) {
        _width = 480;
        _height = 272;
        
        // PLL settings for 480x272
        writeCommand(RA8875_PLLC1);
        writeData(0x0A);
        delay(1);
        writeCommand(RA8875_PLLC2);
        writeData(0x02);
        delay(1);
        
        // Pixel clock setting (divide by 4 for 480x272)
        writeCommand(RA8875_PCSR);
        writeData(0x81);
        delay(1);
        
        // System configuration - 8-bit MCU interface, 16-bit color
        writeCommand(RA8875_SYSR);
        writeData(0x0C);  // 8-bit data bus, 16-bit (65K) color mode
        
        // Memory Write Control - Graphics mode, left-to-right, top-to-bottom
        writeCommand(RA8875_MWCR0);
        writeData(0x00);  // Graphics mode, normal write direction
        
        // Display Configuration - Horizontal scan, top to bottom
        writeCommand(RA8875_DPCR);
        writeData(0x00);  // Normal display, no layer
        
        // Timing parameters for 480x272
        writeCommand(RA8875_HDWR);
        writeData(0x3B);  // (480/8) - 1 = 59
        writeCommand(RA8875_HNDFTR);
        writeData(0x00);
        writeCommand(RA8875_HNDR);
        writeData(0x02);
        writeCommand(RA8875_HSTR);
        writeData(0x00);
        writeCommand(RA8875_HPWR);
        writeData(0x03);
        
        writeCommand(RA8875_VDHR0);
        writeData(0x0F);  // 272 - 1 = 271 = 0x10F
        writeCommand(RA8875_VDHR1);
        writeData(0x01);
        writeCommand(RA8875_VNDR0);
        writeData(0x02);
        writeCommand(RA8875_VNDR1);
        writeData(0x00);
        writeCommand(RA8875_VSTR0);
        writeData(0x00);
        writeCommand(RA8875_VSTR1);
        writeData(0x00);
        writeCommand(RA8875_VPWR);
        writeData(0x01);
    }
}

void RA8875_Minimal::writeRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* data) {
    // Set active window to match the rectangle we're writing
    writeCommand(RA8875_HSAW0);
    writeData(x & 0xFF);
    writeCommand(RA8875_HSAW1);
    writeData(x >> 8);
    
    writeCommand(RA8875_VSAW0);
    writeData(y & 0xFF);
    writeCommand(RA8875_VSAW1);
    writeData(y >> 8);
    
    writeCommand(RA8875_HEAW0);
    writeData((x + w - 1) & 0xFF);
    writeCommand(RA8875_HEAW1);
    writeData((x + w - 1) >> 8);
    
    writeCommand(RA8875_VEAW0);
    writeData((y + h - 1) & 0xFF);
    writeCommand(RA8875_VEAW1);
    writeData((y + h - 1) >> 8);
    
    // Set memory write cursor to start position
    writeCommand(RA8875_CURH0);
    writeData(x & 0xFF);
    writeCommand(RA8875_CURH1);
    writeData(x >> 8);
    writeCommand(RA8875_CURV0);
    writeData(y & 0xFF);
    writeCommand(RA8875_CURV1);
    writeData(y >> 8);
    
    // Start memory write command
    writeCommand(RA8875_MRWC);
    
    // Begin SPI transaction for bulk data transfer at maximum speed
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(RA8875_DATAWRITE);
    
    // Transfer pixel data - optimized bulk transfer
    uint32_t totalPixels = (uint32_t)w * h;
    uint16_t* ptr = data;
    
    // Use faster 16-bit transfer when possible
    while (totalPixels >= 8) {
        uint16_t c0 = *ptr++;
        uint16_t c1 = *ptr++;
        uint16_t c2 = *ptr++;
        uint16_t c3 = *ptr++;
        uint16_t c4 = *ptr++;
        uint16_t c5 = *ptr++;
        uint16_t c6 = *ptr++;
        uint16_t c7 = *ptr++;
        
        SPI.transfer(c0 >> 8); SPI.transfer(c0 & 0xFF);
        SPI.transfer(c1 >> 8); SPI.transfer(c1 & 0xFF);
        SPI.transfer(c2 >> 8); SPI.transfer(c2 & 0xFF);
        SPI.transfer(c3 >> 8); SPI.transfer(c3 & 0xFF);
        SPI.transfer(c4 >> 8); SPI.transfer(c4 & 0xFF);
        SPI.transfer(c5 >> 8); SPI.transfer(c5 & 0xFF);
        SPI.transfer(c6 >> 8); SPI.transfer(c6 & 0xFF);
        SPI.transfer(c7 >> 8); SPI.transfer(c7 & 0xFF);
        
        totalPixels -= 8;
    }
    
    // Handle remaining pixels
    while (totalPixels--) {
        uint16_t color = *ptr++;
        SPI.transfer(color >> 8);
        SPI.transfer(color & 0xFF);
    }
    
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
    
    // Restore active window to full screen
    writeCommand(RA8875_HSAW0);
    writeData(0x00);
    writeCommand(RA8875_HSAW1);
    writeData(0x00);
    writeCommand(RA8875_VSAW0);
    writeData(0x00);
    writeCommand(RA8875_VSAW1);
    writeData(0x00);
    writeCommand(RA8875_HEAW0);
    writeData(_width & 0xFF);
    writeCommand(RA8875_HEAW1);
    writeData(_width >> 8);
    writeCommand(RA8875_VEAW0);
    writeData(_height & 0xFF);
    writeCommand(RA8875_VEAW1);
    writeData(_height >> 8);
}

void RA8875_Minimal::touchInit() {
    // Initialize I2C for FT5206 capacitive touch controller
    _wire->begin();
    _wire->setClock(400000);  // 400kHz I2C
    
    // Check if FT5206 is present
    _wire->beginTransmission(FT5206_ADDR);
    if (_wire->endTransmission() == 0) {
        // FT5206 found, configure it
        // Set touch threshold (optional, adjust sensitivity)
        _wire->beginTransmission(FT5206_ADDR);
        _wire->write(0x80);  // Threshold register
        _wire->write(0x16);  // Threshold value (lower = more sensitive)
        _wire->endTransmission();
        
        delay(10);
    }
}

bool RA8875_Minimal::touched() {
    // Check if there's a touch AND update coordinates if so
    // Add hysteresis to prevent flickering touch detection
    
    static bool lastTouchState = false;
    static uint32_t lastTouchTime = 0;
    static uint32_t lastReleaseTime = 0;
    const uint32_t TOUCH_HOLD_TIME = 50;    // Hold "touched" state for 50ms after detection
    const uint32_t RELEASE_HOLD_TIME = 100; // Hold "released" state for 100ms after release
    
    uint32_t now = millis();
    bool isTouched = false;
    
    if (_intPin != 255) {
        // If using interrupt pin, check the pin state
        // The FT5206 INT pin is LOW when touched
        isTouched = (digitalRead(_intPin) == LOW);
    } else {
        // Fallback: Poll FT5206 touch count register
        _wire->beginTransmission(FT5206_ADDR);
        _wire->write(0x02);  // Touch count register
        _wire->endTransmission(false);
        
        _wire->requestFrom(FT5206_ADDR, 1);
        uint8_t touchCount = 0;
        if (_wire->available()) {
            touchCount = _wire->read() & 0x0F;
        }
        
        isTouched = (touchCount > 0);
    }
    
    // Apply hysteresis to prevent flickering
    if (isTouched) {
        lastTouchTime = now;
        lastTouchState = true;
        updateTS();  // Update cached coordinates
        return true;
    } else {
        // No touch detected by hardware
        if (lastTouchState) {
            // Was touched before, check if we should hold the state
            if ((now - lastTouchTime) < TOUCH_HOLD_TIME) {
                // Still within hold period, report as touched
                return true;
            } else {
                // Touch hold period expired
                lastTouchState = false;
                lastReleaseTime = now;
                return false;
            }
        } else {
            // Was already released, check release hold time
            if ((now - lastReleaseTime) < RELEASE_HOLD_TIME) {
                // Don't allow new touch during release hold period
                return false;
            }
            return false;
        }
    }
}

void RA8875_Minimal::updateTS() {
    // This function reads the touch data from FT5206
    // It should be called AFTER touched() returns true
    
    // Read all registers at once (0x00 to 0x0F)
    _wire->beginTransmission(FT5206_ADDR);
    _wire->write(0x00);
    _wire->endTransmission(false);
    
    uint8_t data[16];
    _wire->requestFrom(FT5206_ADDR, 16);
    
    for (int i = 0; i < 16 && _wire->available(); i++) {
        data[i] = _wire->read();
    }
    
    // Store the touch count
    _currentTouches = data[0x02] & 0x0F;
    
    // Cache the coordinates if there's a touch
    if (_currentTouches > 0) {
        // Parse coordinates (registers 0x03-0x06)
        uint16_t x = ((data[0x03] & 0x0F) << 8) | data[0x04];
        uint16_t y = ((data[0x05] & 0x0F) << 8) | data[0x06];
        
        // Apply rotation
        uint16_t finalX, finalY;
        if (_rotation == 0) {
            finalX = x;
            finalY = y;
        } else if (_rotation == 2) {  // 180 degree rotation
            finalX = _width - 1 - x;
            finalY = _height - 1 - y;
        }
        
        // Stabilize coordinates to prevent jitter during long press
        // Only update if moved more than 5 pixels from last position
        if (_cachedCoords[0][0] == 0 && _cachedCoords[0][1] == 0) {
            // First touch, just store it
            _cachedCoords[0][0] = finalX;
            _cachedCoords[0][1] = finalY;
        } else {
            // Check distance from last position
            int16_t dx = abs((int16_t)finalX - (int16_t)_cachedCoords[0][0]);
            int16_t dy = abs((int16_t)finalY - (int16_t)_cachedCoords[0][1]);
            
            // Only update if moved more than threshold (reduces jitter)
            if (dx > 5 || dy > 5) {
                _cachedCoords[0][0] = finalX;
                _cachedCoords[0][1] = finalY;
            }
            // Otherwise keep the old coordinates (stabilizes long press)
        }
    } else {
        // No touch, clear cached coordinates
        _cachedCoords[0][0] = 0;
        _cachedCoords[0][1] = 0;
    }
    
    _cachedCoords[1][0] = 0;
    _cachedCoords[1][1] = 0;
}

void RA8875_Minimal::getTScoordinates(uint16_t coords[2][2]) {
    // Just return the cached coordinates
    // Don't do any I2C operations here
    coords[0][0] = _cachedCoords[0][0];
    coords[0][1] = _cachedCoords[0][1];
    coords[1][0] = _cachedCoords[1][0];
    coords[1][1] = _cachedCoords[1][1];
}

void RA8875_Minimal::useCapINT(uint8_t pin) {
    _intPin = pin;
    pinMode(_intPin, INPUT_PULLUP);
}

void RA8875_Minimal::enableCapISR(bool state) {
    // Touch interrupt is always enabled in this minimal version
}

void RA8875_Minimal::backlight(bool on) {
    writeCommand(RA8875_P1CR);
    writeData(on ? 0x80 : 0x00);  // PWM1 enable/disable
    
    if (on) {
        writeCommand(RA8875_P1DCR);
        writeData(0xFF);  // Full brightness
    }
}

void RA8875_Minimal::displayOn(bool on) {
    writeCommand(RA8875_PWRR);
    writeData(on ? 0x80 : 0x00);
}

void RA8875_Minimal::setRotation(uint8_t rotation) {
    _rotation = rotation;
    
    // For 180-degree rotation, we need to control DPCR register
    // DPCR bits:
    // Bit 3: HDIR - Horizontal scan direction (0=left-to-right, 1=right-to-left)
    // Bit 2: VDIR - Vertical scan direction (0=top-to-bottom, 1=bottom-to-top)
    
    uint8_t dpcr_value = 0x00;
    
    if (rotation == 2) {  // 180 degree rotation
        dpcr_value = 0x0C;  // Set both HDIR and VDIR (flip both horizontal and vertical)
    }
    
    writeCommand(RA8875_DPCR);
    writeData(dpcr_value);
}

// Low-level SPI functions
void RA8875_Minimal::writeCommand(uint8_t cmd) {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(RA8875_CMDWRITE);
    SPI.transfer(cmd);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
}

void RA8875_Minimal::writeData(uint8_t data) {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(RA8875_DATAWRITE);
    SPI.transfer(data);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
}

uint8_t RA8875_Minimal::readData() {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(RA8875_DATAREAD);
    uint8_t data = SPI.transfer(0x00);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
    return data;
}

uint8_t RA8875_Minimal::readStatus() {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(RA8875_CMDREAD);
    uint8_t status = SPI.transfer(0x00);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
    return status;
}