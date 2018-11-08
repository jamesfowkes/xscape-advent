/*
 * C/C++ Includes
 */

#include <stdint.h>
#include <stdarg.h>

/* Arduino Includes */

#include "Arduino.h"

/*
 * ADL Includes
 */

#include "adl.h"

/*
 * Private Data
 */

static const uint8_t LOG_BUFFER_SIZE = 64;

static const char s_adl_log_prefix[] PROGMEM = "ADL";


static const char s_application_name[] PROGMEM = "APP";


static char const * const s_adl_log_module_prefixes[] PROGMEM = {
    s_adl_log_prefix,

    s_application_name,


};

static Print * s_p_log_stream = NULL;
static char s_buf[LOG_BUFFER_SIZE];
static char s_prefix_buf[7];

/*
 * Private Functions
 */

static const char * get_prefix_ptr(ADL_LOG_MODULES module)
{
    return (const char *)pgm_read_ptr(&(s_adl_log_module_prefixes[module]));
}

static void print_prefix(ADL_LOG_MODULES module)
{
    adl_board_read_progmem(s_prefix_buf, get_prefix_ptr(module));
    s_p_log_stream->print(s_prefix_buf);
    s_p_log_stream->print(": ");
}

/*
 * Public Functions
 */

void adl_logging_setup(Print& log_printer)
{
    s_p_log_stream = &log_printer;
}

void adl_log(ADL_LOG_MODULES module, char const * const fmt, ...)
{
    print_prefix(module);
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_buf, LOG_BUFFER_SIZE, fmt, args);
    s_p_log_stream->print(s_buf);
    va_end(args);
}

void adl_logln(ADL_LOG_MODULES module, char const * const fmt, ...)
{
    print_prefix(module);
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_buf, LOG_BUFFER_SIZE, fmt, args);
    s_p_log_stream->println(s_buf);
    va_end(args);
}
