#include <Adafruit_NeoPixel.h>

#include "adl.h"

#include "adafruit-neopixel-adl.h"

/*
 * Class Private Functions
 */

static bool valid_rgb_value(int32_t val)
{
    return (val >= 0) && (val <= 255);
}

static bool valid_rgb_values(int32_t(&rgb)[3])
{
    return valid_rgb_value(rgb[0]) && valid_rgb_value(rgb[1]) && valid_rgb_value(rgb[2]);
}

/*
 * Class ADL Functions
 */

void AdafruitNeoPixelADL::set_pixels(uint8_t range_min, uint8_t range_max, uint8_t r, uint8_t g, uint8_t b)
{
    if (range_min < range_max)
    {
        for (uint8_t i=range_min; i<range_max+1; i++)
        {
            m_pixels.setPixelColor(i, r, g, b);
        }
        m_pixels.show();
    }
}

void AdafruitNeoPixelADL::set_pixels(uint8_t range_min, uint8_t range_max, const uint8_t rgb[3])
{
    this->set_pixels(range_min, range_max, rgb[0], rgb[1], rgb[2]);
}

int AdafruitNeoPixelADL::handle_command(char const * const command, char * reply)
{
    int32_t rgb[3];

    int32_t range_min;
    int32_t range_max;

    char * end_of_range = NULL;

    bool valid_range = adl_convert_numeric_range(command, range_min, range_max, &end_of_range);
    valid_range &= range_min < this->m_npixels;
    valid_range &= range_max < this->m_npixels;

    if (valid_range && (*end_of_range == ':'))
    {
        end_of_range++;        
        uint8_t parsed_count = adl_parse_comma_separated_numerics(end_of_range, rgb);

        if (parsed_count == 3)
        {
            if (valid_rgb_values(rgb))
            {
                set_pixels(range_min, range_max, rgb[0], rgb[1], rgb[2]);
                return sprintf(reply, "%u-%u: %u,%u,%u",
                    (uint8_t)range_min, (uint8_t)range_max,
                    (uint8_t)rgb[0], (uint8_t)rgb[1], (uint8_t)rgb[2]);
            }
            else
            {
                return adl_msg_invalid_values(reply);
            }
        }
        else
        {
            return adl_msg_wrong_number_of_values(reply, parsed_count, end_of_range);
        }
    }
    else
    {
        return adl_msg_invalid_range(reply);
    }
}

void AdafruitNeoPixelADL::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
    m_pixels.setPixelColor(n, r, g, b);
}

void AdafruitNeoPixelADL::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    m_pixels.setPixelColor(n, r, g, b, w);
}

void AdafruitNeoPixelADL::setPixelColor(uint16_t n, uint32_t c)
{
    m_pixels.setPixelColor(n, c);
}

void AdafruitNeoPixelADL::show()
{
    m_pixels.show();   
}

void AdafruitNeoPixelADL::clear()
{
    m_pixels.clear();   
}

/*
 * Class ADL Functions
 */

AdafruitNeoPixelADL::AdafruitNeoPixelADL(uint8_t pin, uint16_t npixels, uint16_t type) : 
    m_pixels(npixels,pin,type), m_npixels(npixels)
{
    m_pixels.begin();
}

void AdafruitNeoPixelADL::reset()
{
    m_pixels.clear();
    m_pixels.show();
}

void AdafruitNeoPixelADL::tick()
{

}

void AdafruitNeoPixelADL::setup()
{
    this->reset();
}

int AdafruitNeoPixelADL::command_handler(char const * const command, char * reply)
{
    int reply_length;

    if (command[0] == 'R')
    {
        this->reset();
        strcpy(reply, "ROK");
        reply_length = strlen(reply);
    }
    else
    {
        reply_length = handle_command(command, reply);
    }

    return reply_length;
}

Adafruit_NeoPixel& AdafruitNeoPixelADL::pixels()
{
    return m_pixels;
}

uint8_t AdafruitNeoPixelADL::npixels()
{
    return m_npixels;
}
