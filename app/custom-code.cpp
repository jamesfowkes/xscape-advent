/* Includes */
#include <Arduino.h>
#include <IRremote.h>

#include "device.h"
#include "parameter.h"
#include "adl.h"

#include "adl-oneshot-timer.h"
#include "adl-oneshot-task.h"
#include "adl-task.h"

#include "IR-receiver.h"

/* Defines, enums, typedefs */

typedef enum _mode
{
	eMODE_IDLE,
	eMODE_RUNNING,
	eMODE_PAUSED,
	eMODE_FINISHED
} eMode;

/* Constants */

static const unsigned long START_NORMAL_CODE = 0xAAAA;
static const unsigned long START_TEST_CODE = 0xBBBB;
static const unsigned long PAUSE_RESUME_CODE = 0xCCCC;
static const unsigned long RESET_CODE = 0xDDDD;

static const uint16_t NORMAL_COUNTDOWN_PERIOD = 144000;
static const uint16_t TEST_COUNTDOWN_PERIOD = 1000;

static const uint8_t COUNTDOWN_START_VALUE = 25;

/* Local Variables */

static eMode s_mode = eMODE_IDLE;
static uint8_t s_countdown = 0;

/* Private Functions */

static void set_leds(uint16_t countdown)
{

}

static void set_countdown(uint16_t count)
{
	s_countdown = count;
	set_leds(s_countdown);
}

static void countdown_task_fn(ADLOneShotTask * pThisTask, void * pTaskData)
{
	(void)pThisTask;
	(void)pTaskData;
	if (s_countdown)
	{
		set_countdown(s_countdown-1);
	}

	set_leds(s_countdown);

	if (s_countdown == 0)
	{
		s_mode = eMODE_FINISHED;
	}
}

static ADLTask s_countdown_task(0, countdown_task_fn, NULL);

static IR_Receiver * s_pIR;

static void start_countdown(uint16_t period)
{
	s_countdown_task.set_period(period);
	set_countdown(COUNTDOWN_START_VALUE);
}

static bool countdown_can_start(eMode mode)
{
	return mode == eMODE_IDLE;
}

static bool countdown_can_reset(eMode mode)
{
	(mode == eMODE_PAUSED) || (mode == eMODE_IDLE);
}

static bool countdown_can_pause(eMode mode)
{
	(mode == eMODE_RUNNING);
}

static bool countdown_can_resume(eMode mode)
{
	(mode == eMODE_PAUSED);
}

static void check_ir_remote()
{
	unsigned long code;
	if (s_pIR->get_code(code))
	{
		switch(code)
		{
		case START_NORMAL_CODE:
			if (countdown_can_start(s_mode))
			{
				start_countdown(NORMAL_COUNTDOWN_PERIOD);
			}
			break;
		case START_TEST_CODE:
			if (countdown_can_start(s_mode))
			{
				start_countdown(TEST_COUNTDOWN_PERIOD);
			}
			break;
		case PAUSE_RESUME_CODE:
			if (countdown_can_pause(s_mode))
			{
				s_mode = eMODE_PAUSED;
			}
			else if (countdown_can_resume(s_mode))
			{
				s_mode = eMODE_RUNNING;	
			}
			break;
		case RESET_CODE:
			if (countdown_can_reset(s_mode))
			{
				set_countdown(COUNTDOWN_START_VALUE);
			}
			break;
		}
	}
}

/* Public Functions */

void adl_custom_setup(DeviceBase * pdevices[], int ndevices, ParameterBase * pparams[], int nparams)
{
	(void)ndevices; (void)pparams; (void)nparams;
	s_pIR = (IR_Receiver *)pdevices[0];
}

void adl_custom_loop(DeviceBase * pdevices[], int ndevices, ParameterBase * pparams[], int nparams)
{
	(void)pdevices; (void)ndevices; (void)pparams; (void)nparams;

	check_ir_remote();

	if (s_mode == eMODE_RUNNING)
	{
		s_countdown_task.run();
	}
}
