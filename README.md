# RA8875_Minimal

A lightweight, memory-efficient library for RA8875-based displays, optimized for use with LVGL (Light and Versatile Graphics Library).

## Why This Library?

The full RA8875 library is feature-rich but uses significant memory and code space. If you're using LVGL for graphics rendering, you don't need all those features - LVGL handles all the drawing operations. This library provides only what's essential:

- ✅ Display initialization
- ✅ Fast pixel buffer transfers (optimized for LVGL)
- ✅ FT5206 capacitive touch support
- ✅ Display power and backlight control
- ✅ Rotation support (0° and 180°)

## Size Comparison

| Feature | Full RA8875 Library | RA8875_Minimal |
|---------|-------------------|----------------|
| Compiled Size | ~100KB+ | ~5-8KB |
| Code Lines | Thousands | 479 |
| RAM Overhead | High | Minimal |
| Features | Everything | Display + Touch only |

**Result: ~95% size reduction!**

## Hardware Support

- **Display Controller:** RA8875
- **Resolution:** 480x272 (can be extended for 800x480)
- **Touch Controller:** FT5206 (capacitive, I2C)
- **Interface:** SPI (display) + I2C (touch)
- **Tested on:** Teensy 4.1 (should work on any Arduino-compatible board)

## Installation

### Arduino IDE
1. Download this repository as ZIP
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps = 
    https://github.com/fubr/RA8875_Minimal.git
```

Or clone directly into your project's `lib` folder.

## Usage Example

### Basic Setup with LVGL

```cpp
#include <RA8875_Minimal.h>
#include <lvgl.h>

#define RA8875_CS 10
#define RA8875_RST 9
#define TOUCH_INT 8

RA8875_Minimal tft(RA8875_CS, RA8875_RST);

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 10]; // 10-line buffer

// LVGL display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    int32_t x = area->x1;
    int32_t y = area->y1;
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    tft.writeRect(x, y, w, h, (uint16_t*)color_p);
    lv_disp_flush_ready(disp);
}

// LVGL touch read callback
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    if (tft.touched()) {
        tft.updateTS();
        uint16_t coordinates[2][2];
        tft.getTScoordinates(coordinates);
        
        data->state = LV_INDEV_STATE_PR;
        data->point.x = coordinates[0][0];
        data->point.y = coordinates[0][1];
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize display
    if (!tft.begin(RA8875_Minimal::RA8875_480x272)) {
        Serial.println("RA8875 initialization failed!");
        while(1);
    }
    
    // Optional: use interrupt pin for touch
    tft.useCapINT(TOUCH_INT);
    
    // Turn on display and backlight
    tft.displayOn(true);
    tft.backlight(true);
    
    // Initialize LVGL
    lv_init();
    
    // Setup LVGL display
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 480 * 10);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 272;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Setup LVGL touch input
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    // Create your LVGL UI here
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello RA8875_Minimal!");
    lv_obj_center(label);
}

void loop() {
    lv_timer_handler();
    delay(5);
}
```

## API Reference

### Initialization

```cpp
RA8875_Minimal tft(uint8_t cs, uint8_t rst);
```
Create display instance with CS and RST pin numbers.

```cpp
bool begin(uint16_t displayType);
```
Initialize the display. Use `RA8875_Minimal::RA8875_480x272` for 480x272 displays.

### Display Control

```cpp
void displayOn(bool on);
```
Turn display on/off.

```cpp
void backlight(bool on);
```
Control backlight (on/off at full brightness).

```cpp
void setRotation(uint8_t rotation);
```
Set display rotation (0 = normal, 2 = 180° flip).

### Pixel Output

```cpp
void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* data);
```
Write a rectangle of RGB565 pixel data. Optimized for bulk transfers at 20MHz SPI.

### Touch Input (FT5206)

```cpp
bool touched();
```
Check if screen is currently touched.

```cpp
void updateTS();
```
Update touch state (clears interrupt if using interrupt pin).

```cpp
void getTScoordinates(uint16_t coords[2][2]);
```
Get touch coordinates. First touch point is at `coords[0][0]` (X) and `coords[0][1]` (Y).

```cpp
void useCapINT(uint8_t pin);
```
Optional: specify interrupt pin for touch detection (active low).

```cpp
void enableCapISR(bool state);
```
Enable/disable touch interrupt (always enabled in this minimal version).

## Wiring

### SPI (Display)
- **MOSI** → RA8875 SDI
- **MISO** → RA8875 SDO  
- **SCK** → RA8875 SCK
- **CS** → Your chosen CS pin (e.g., pin 10)
- **RST** → Your chosen RST pin (e.g., pin 9)

### I2C (Touch - FT5206)
- **SDA** → FT5206 SDA
- **SCL** → FT5206 SCL
- **INT** (optional) → Your chosen interrupt pin (e.g., pin 8)

## Performance Tips

1. **SPI Speed:** Display writes use 20MHz SPI for maximum speed
2. **Buffer Size:** Larger LVGL buffers = fewer flush operations = smoother rendering
3. **Interrupt Pin:** Using the touch interrupt pin is more efficient than polling

## Limitations

This is a **minimal** library focused on LVGL integration. It does NOT include:
- Text rendering
- Graphic primitives (lines, circles, rectangles, etc.)
- Built-in fonts
- Resistive touch support
- Hardware acceleration features
- Layer management
- BTE (Block Transfer Engine)

**If you need these features, use the full RA8875 library instead.**

## Tested Configurations

- ✅ Teensy 4.1 + LVGL 8.3.x + 480x272 RA8875 display
- ✅ PlatformIO
- ✅ Arduino IDE

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

MIT License - see LICENSE file for details.

## Credits

Created for makers who need a lightweight RA8875 solution when using LVGL. Inspired by the need to optimize RAM usage on Teensy 4.1 projects.

## Changelog

### v1.0.0 (2025-11-07)
- Initial release
- 480x272 display support
- FT5206 capacitive touch support
- LVGL optimization
- Rotation support (0° and 180°)
