/* Includes */
#include <Arduino.h>
#include <IRremote.h>
#include <Adafruit_NeoPixel.h>

#include "device.h"
#include "parameter.h"
#include "adl.h"

#include "adl-oneshot-timer.h"
#include "adl-oneshot-task.h"
#include "adl-task.h"

#include "adl-logging.h"

#include "IR-receiver.h"
#include "adafruit-neopixel-adl.h"

#include "settings.h"

/* Defines, enums, typedefs */

typedef enum _mode
{
	eMODE_IDLE,
	eMODE_RUNNING,
	eMODE_PAUSED,
	eMODE_FINISHED
} eMode;

/* Constants */

static const unsigned long START_NORMAL_CODE = 0xFF02FD;	// "OK" key
static const unsigned long START_TEST_CODE = 0xFF52AD;		// '#' button
static const unsigned long PAUSE_RESUME_CODE = 0xFFC23D;	// '>' button
static const unsigned long RESET_CODE = 0xFF22DD;			// '<' button

static const uint16_t NORMAL_COUNTDOWN_DAY_LENGTH = 144;
static const uint16_t TEST_COUNTDOWN_DAY_LENGTH = 1;

static const uint8_t DAYS_IN_ADVENT = 25;

/* Local Variables */

static eMode s_mode = eMODE_IDLE;
static uint8_t s_day_timer = 0;
static uint8_t s_advent_day = 1;
static bool s_testing = false;

static IR_Receiver * s_pIR;
static AdafruitNeoPixelADL * s_pNeoPixels;

/* Private Functions */

static void set_leds(uint8_t n, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t i;
	s_pNeoPixels->pixels().clear();
	for (i=0; i<n; i++)
	{
		s_pNeoPixels->pixels().setPixelColor(i, r, g, b);
	}
	s_pNeoPixels->pixels().show();
}

static void set_leds_finished()
{
	set_leds(DAYS_IN_ADVENT, LED_FINISH_COLOUR);
}


static void set_leds_normal(uint8_t days)
{
	set_leds(days, LED_RED_COLOUR);
}

static void set_advent_day(uint16_t day)
{
	s_advent_day = day;
	set_leds_normal(s_advent_day);
}

static void set_mode(eMode new_mode)
{
	logln(LOG_APP, "New mode %d", (uint8_t)new_mode);
	s_mode = new_mode;
}

static bool is_last_day()
{
	return s_advent_day == 25;
}

static uint8_t get_timer_reload()
{
	if (is_last_day())
	{
		return s_testing ? 10 : NORMAL_COUNTDOWN_DAY_LENGTH;
	}
	else
	{
		return s_testing ? TEST_COUNTDOWN_DAY_LENGTH : NORMAL_COUNTDOWN_DAY_LENGTH;
	}
}

static void run_normal_day()
{
	if (s_day_timer > 0)
	{
		s_day_timer--;

		if (s_day_timer == 0)
		{
			logln(LOG_APP, "End of day %u", s_advent_day);
			
			if (s_advent_day<DAYS_IN_ADVENT)
			{
				set_advent_day(s_advent_day+1);
			}
			s_day_timer = get_timer_reload();
			set_leds_normal(s_advent_day);
		}
	}
}

static void run_last_day()
{
	if (s_day_timer > 0)
	{
		logln(LOG_APP, "%d seconds remaining...", s_day_timer);
		s_day_timer--;

		if (s_day_timer & 1)
		{
			set_leds_normal(s_advent_day);
		}
		else
		{
			set_leds_normal(s_advent_day-1);
		}

		if (s_day_timer == 0)
		{
			logln(LOG_APP, "Game finished!");
			set_leds_finished();
			set_mode(eMODE_FINISHED);
		}
	}
}

static void countdown_task_fn(ADLTask& thisTask, void * pTaskData)
{
	(void)thisTask;
	(void)pTaskData;

	if (is_last_day())
	{
		run_last_day();
	}
	else
	{
		run_normal_day();
	}
	
}
static ADLTask s_countdown_task(1000, countdown_task_fn, NULL);

static void start_countdown(bool testing)
{
	set_advent_day(1);
	s_countdown_task.reset();
	s_testing = testing;
	s_day_timer = s_testing ? TEST_COUNTDOWN_DAY_LENGTH : NORMAL_COUNTDOWN_DAY_LENGTH;
}

static bool countdown_can_start(eMode mode)
{
	return mode == eMODE_IDLE;
}

static bool countdown_can_reset(eMode mode)
{
	return (mode == eMODE_PAUSED) || (mode == eMODE_IDLE) || (mode == eMODE_FINISHED);
}

static bool countdown_can_pause(eMode mode)
{
	return (mode == eMODE_RUNNING);
}

static bool countdown_can_resume(eMode mode)
{
	return (mode == eMODE_PAUSED);
}

static void check_ir_remote()
{
	unsigned long code;
	if (s_pIR->get_code(code))
	{

		logln(LOG_APP, "Got code %lx", code);

		switch(code)
		{
		case START_NORMAL_CODE:
			if (countdown_can_start(s_mode))
			{
				logln(LOG_APP, "Starting countdown (normal)");
				start_countdown(false);
				set_mode(eMODE_RUNNING);
			}
			break;
		case START_TEST_CODE:
			if (countdown_can_start(s_mode))
			{
				logln(LOG_APP, "Starting countdown (test)");
				start_countdown(true);
				set_mode(eMODE_RUNNING);
			}
			break;
		case PAUSE_RESUME_CODE:
			if (countdown_can_pause(s_mode))
			{
				logln(LOG_APP, "Countdown paused");
				set_mode(eMODE_PAUSED);
			}
			else if (countdown_can_resume(s_mode))
			{
				logln(LOG_APP, "Countdown resuming");
				set_mode(eMODE_RUNNING);
			}
			break;
		case RESET_CODE:
			if (countdown_can_reset(s_mode))
			{
				logln(LOG_APP, "Countdown reset");
				set_advent_day(1);
				set_mode(eMODE_IDLE);
			}
			break;
		default:
			logln(LOG_APP, "Code not recognised");
			break;
		}
	}
}

static void play_intro()
{
	uint8_t i;
	for(i=0;i<DAYS_IN_ADVENT;i++)
	{
		s_pNeoPixels->pixels().setPixelColor(i, 32, 32, 32);
		s_pNeoPixels->pixels().show();
		delay(50);
	}

	for(i=0;i<DAYS_IN_ADVENT;i++)
	{
		s_pNeoPixels->pixels().setPixelColor(DAYS_IN_ADVENT-i-1, 0, 0, 0);
		s_pNeoPixels->pixels().show();
		delay(50);
	}
}

/* Public Functions */

void adl_custom_setup(DeviceBase * pdevices[], int ndevices, ParameterBase * pparams[], int nparams)
{
	(void)ndevices; (void)pparams; (void)nparams;
	
	s_pIR = (IR_Receiver*)pdevices[0];
	s_pNeoPixels = (AdafruitNeoPixelADL*)pdevices[1];

	logln(LOG_APP, "Recognised codes:");
	logln(LOG_APP, "Start normal: %lx", START_NORMAL_CODE);
	logln(LOG_APP, "Start test: %lx", START_TEST_CODE);
	logln(LOG_APP, "Pause/resume: %lx", PAUSE_RESUME_CODE);
	logln(LOG_APP, "Reset: %lx", RESET_CODE);

	play_intro();
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
