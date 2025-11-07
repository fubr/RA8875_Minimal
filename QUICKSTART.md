# Quick Start Guide

Get up and running with RA8875_Minimal in 5 minutes!

## Step 1: Install Dependencies

You'll need these libraries:
- **LVGL** (version 8.x) - Install via Library Manager or PlatformIO
- **SPI** (built-in)
- **Wire** (built-in)

## Step 2: Install RA8875_Minimal

### Arduino IDE
1. Download this repository as ZIP
2. Sketch → Include Library → Add .ZIP Library
3. Select the downloaded file

### PlatformIO
Add to `platformio.ini`:
```ini
lib_deps = 
    lvgl/lvgl @ ^8.3.0
    https://github.com/fubr/RA8875_Minimal.git
```

## Step 3: Wire Your Display

### Display (SPI)
```
Board MOSI → RA8875 SDI
Board MISO → RA8875 SDO
Board SCK  → RA8875 SCK
Pin 10     → RA8875 CS (or your choice)
Pin 9      → RA8875 RST (or your choice)
```

### Touch (I2C - FT5206)
```
Board SDA → FT5206 SDA
Board SCL → FT5206 SCL
Pin 8     → FT5206 INT (optional)
```

## Step 4: Configure LVGL

Create `lv_conf.h` in your project (or use the one from LVGL examples):

Key settings:
```c
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0  // or 1 if colors are wrong
#define LV_MEM_SIZE (48 * 1024U)  // Adjust based on your board's RAM
```

## Step 5: Run the Example

Open `File → Examples → RA8875_Minimal → Basic` and upload!

You should see:
- Display lights up
- "Hello RA8875_Minimal!" text
- A button you can touch

## Troubleshooting

### Display doesn't turn on
- Check SPI wiring
- Verify CS and RST pin numbers in code
- Check power supply (5V)

### Touch doesn't work
- Check I2C wiring (SDA/SCL)
- Verify FT5206 address (0x38)
- Try without interrupt pin first

### Wrong colors
- Toggle `LV_COLOR_16_SWAP` in lv_conf.h

### Display flickers
- Increase LVGL buffer size (more lines)
- Check SPI signal quality (shorter wires)

## Need Help?

- Check the full [README.md](README.md) for detailed API reference
- Look at existing [Issues](../../issues) on GitHub
- Open a new issue with hardware details and code

## Next Steps

Once the example works:
1. Explore LVGL documentation: https://docs.lvgl.io/
2. Create your own UI layouts
3. Customize colors and styles
4. Add animations and effects

Happy making! 🚀
