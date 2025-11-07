/*
 * RA8875_Minimal Basic Example
 * 
 * Demonstrates basic usage of RA8875_Minimal library with LVGL
 * This example creates a simple label and button on the display
 * 
 * Hardware:
 * - RA8875 480x272 display with FT5206 touch controller
 * - Any Arduino-compatible board (tested on Teensy 4.1)
 * 
 * Connections:
 * Display (SPI):
 *   - MOSI -> RA8875 SDI
 *   - MISO -> RA8875 SDO
 *   - SCK  -> RA8875 SCK
 *   - CS   -> Pin 10 (or your choice)
 *   - RST  -> Pin 9 (or your choice)
 * 
 * Touch (I2C):
 *   - SDA -> FT5206 SDA
 *   - SCL -> FT5206 SCL
 *   - INT -> Pin 8 (optional, or your choice)
 */

#include <RA8875_Minimal.h>
#include <lvgl.h>

// Pin definitions - adjust for your hardware
#define RA8875_CS   10
#define RA8875_RST  9
#define TOUCH_INT   8

// Create display instance
RA8875_Minimal tft(RA8875_CS, RA8875_RST);

// LVGL display buffer (10 lines)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 10];

// LVGL display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    int32_t x = area->x1;
    int32_t y = area->y1;
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // Send pixel data to display
    tft.writeRect(x, y, w, h, (uint16_t*)color_p);
    
    // Tell LVGL we're done
    lv_disp_flush_ready(disp);
}

// LVGL touch read callback
void my_touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    static uint16_t last_x = 0;
    static uint16_t last_y = 0;
    
    if (tft.touched()) {
        tft.updateTS();
        uint16_t coordinates[2][2];
        tft.getTScoordinates(coordinates);
        
        data->state = LV_INDEV_STATE_PR;
        data->point.x = coordinates[0][0];
        data->point.y = coordinates[0][1];
        
        last_x = coordinates[0][0];
        last_y = coordinates[0][1];
    } else {
        data->state = LV_INDEV_STATE_REL;
        data->point.x = last_x;
        data->point.y = last_y;
    }
}

// Button event handler
void btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        Serial.println("Button clicked!");
        
        // Change label text
        lv_obj_t * label = lv_event_get_user_data(e);
        lv_label_set_text(label, "Button Pressed!");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("RA8875_Minimal Example Starting...");
    
    // Initialize display
    if (!tft.begin(RA8875_Minimal::RA8875_480x272)) {
        Serial.println("RA8875 initialization failed!");
        while(1) delay(100);
    }
    Serial.println("RA8875 initialized successfully!");
    
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
    
    Serial.println("LVGL initialized!");
    
    // Create UI
    // Label
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello RA8875_Minimal!");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);
    
    // Button
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, label);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn, 120, 50);
    
    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);
    
    Serial.println("UI created! Touch the button!");
}

void loop() {
    // Let LVGL handle tasks
    lv_timer_handler();
    delay(5);
}
