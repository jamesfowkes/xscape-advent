#ifndef _ADAFRUIT_NEOPIXEL_ADL_H_
#define _ADAFRUIT_NEOPIXEL_ADL_H_

class AdafruitNeoPixelADL : public DeviceBase
{
public:
    AdafruitNeoPixelADL(uint8_t pin, uint16_t npixels, uint16_t type);
    void setup();
    void reset();
    void tick();
    int command_handler(char const * const command, char * reply);
    Adafruit_NeoPixel& pixels();
    void set_pixels(uint8_t range_min, uint8_t range_max, uint8_t r, uint8_t g, uint8_t b);
    void set_pixels(uint8_t range_min, uint8_t range_max, const uint8_t rgb[3]);
    uint8_t npixels();
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void setPixelColor(uint16_t n, uint32_t c);
    void show();
    void clear();
private:
    int handle_command(char const * const command, char * reply);
    Adafruit_NeoPixel m_pixels;
    uint8_t m_npixels;
};

#endif