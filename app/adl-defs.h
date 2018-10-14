#ifndef _ADL_DEFS_H_
#define _ADL_DEFS_H_

enum adl_devices
{
	ADL_DEVICE_BOARD,

	ADL_DEVICE_IR_RECEIVER,

};
typedef enum adl_devices ADL_DEVICES;

enum adl_parameters
{

};
typedef enum adl_parameters ADL_PARAMETERS;

static const int ADL_DEVICE_COUNT = 1;
static const int ADL_PARAM_COUNT = 0;

static const int ADL_BUFFER_SIZE = 64;

static const unsigned long ADL_TICK_MS = 10;

#endif