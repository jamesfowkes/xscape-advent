#include <EEPROM.h>

#include "adl.h"


    #include "adl-util-limited-range-int.h"


int adl_board_read_progmem(char * const dst, char const * const src)
{
    return(strlen(strcpy_P(dst, src)));
}

uint16_t adl_board_max_adc_reading()
{
	return 1023U;
}