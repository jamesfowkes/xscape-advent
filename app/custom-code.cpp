/* Includes */
#include <Arduino.h>
#include <IRremote.h>
#include <Adafruit_NeoPixel.h>

#include "adl.h"

#include "adl-oneshot-timer.h"
#include "adl-oneshot-task.h"
#include "adl-task.h"

#include "adl-logging.h"

#include "IR-receiver.h"
#include "adafruit-neopixel-adl.h"

#include "integer-param.h"

#include "settings.h"

/* Defines, enums, typedefs */

typedef enum _mode
{
	eMODE_IDLE,
	eMODE_RUNNING,
	eMODE_PAUSED,
	eMODE_FINISHED
} eMode;

typedef enum _modeChangeRequest
{
	eMODE_CHANGE_REQUEST_START_NORMAL,
	eMODE_CHANGE_REQUEST_START_TEST,
	eMODE_CHANGE_REQUEST_PAUSE_RESUME,
	eMODE_CHANGE_REQUEST_RESET
} eModeChangeRequest;

/* Constants */

static const unsigned long START_NORMAL_CODE = 0xFF02FD;	// "OK" key
static const unsigned long START_TEST_CODE = 0xFF52AD;		// '#' button
static const unsigned long PAUSE_RESUME_CODE = 0xFFC23D;	// '>' button
static const unsigned long RESET_CODE = 0xFF22DD;			// '<' button

// Number of 500ms intervals
static const uint16_t NORMAL_COUNTDOWN_DAY_LENGTH = 295; // 295 * 500ms = 147.5s
static const uint16_t TEST_COUNTDOWN_DAY_LENGTH = 14; // 14 * 500ms = 7s
static const uint16_t LAST_DAY_COUNTDOWN_LENGTH = 120; // 120 * 500ms = 60s

static const uint8_t DAYS_IN_ADVENT = 25;
static const uint8_t TESTING_DAYS = 5;

/* Local Variables */

static eMode s_mode = eMODE_IDLE;
static uint16_t s_day_timer = 0;
static uint16_t s_lastday_countdown = 0;
static uint8_t s_advent_day = 1;
static bool s_testing = false;
static IR_Receiver * s_pIR;
static AdafruitNeoPixelADL * s_pNeoPixels;

/* Private Functions */

static uint8_t map_led_to_px_idx(uint8_t n)
{
	if (n < 5)
	{
		return n;
	}
	else if (n < 10)
	{
		return 5 + (9-n);
	}
	else if (n < 15)
	{
		return n;
	}
	else if (n < 20)
	{
		return 15 + (19-n);
	}
	else
	{
		return n;
	}
}

static void set_leds(uint8_t n, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t i;
	s_pNeoPixels->clear();
	for (i=0; i<n; i++)
	{
		s_pNeoPixels->setPixelColor(map_led_to_px_idx(i), r, g, b);
	}
	s_pNeoPixels->show();
}

static void set_leds_finished()
{
	set_leds(s_testing ? TESTING_DAYS : DAYS_IN_ADVENT, LED_FINISH_COLOUR);
}


static void set_leds_normal(uint8_t days)
{
	set_leds(days, LED_RED_COLOUR);
}

static void set_leds_idle()
{
	set_leds(1, LED_IDLE_COLOUR);
}

static void set_advent_day(uint16_t day)
{
	s_advent_day = day;
	set_leds_normal(s_advent_day);
}

static void set_mode(eMode new_mode)
{
	adl_logln(LOG_APP, "New mode %d", (uint8_t)new_mode);
	s_mode = new_mode;
}

static bool is_last_day()
{
	return s_advent_day == (s_testing ? TESTING_DAYS : DAYS_IN_ADVENT);
}

static uint16_t get_timer_reload()
{
	if (is_last_day())
	{
		return s_testing ? 20 : LAST_DAY_COUNTDOWN_LENGTH;
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
			adl_logln(LOG_APP, "End of day %u", s_advent_day);
			
			if (!is_last_day())
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
		if ((s_day_timer & 1) == 0)
		{
			s_lastday_countdown--;
			adl_logln(LOG_APP, "%d seconds remaining...", s_lastday_countdown);

			set_leds_normal(s_lastday_countdown & 1 ? s_advent_day : s_advent_day-1);
			
			if (s_lastday_countdown == 0)
			{
				adl_logln(LOG_APP, "Game finished!");
				set_leds_finished();
				set_mode(eMODE_FINISHED);
			}
		}

		s_day_timer--;
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
static ADLTask s_countdown_task(500, countdown_task_fn, NULL);

static void start_countdown(bool testing)
{
	set_advent_day(1);
	s_countdown_task.reset();
	s_testing = testing;
	s_lastday_countdown = 60;
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

static void handle_mode_change_request(eModeChangeRequest modeChangeRequest)
{
	switch(modeChangeRequest)
	{
	case eMODE_CHANGE_REQUEST_START_NORMAL:
		if (countdown_can_start(s_mode))
		{
			adl_logln(LOG_APP, "Starting countdown (normal)");
			start_countdown(false);
			set_mode(eMODE_RUNNING);
		}
		break;
	case eMODE_CHANGE_REQUEST_START_TEST:
		if (countdown_can_start(s_mode))
		{
			adl_logln(LOG_APP, "Starting countdown (test)");
			start_countdown(true);
			set_mode(eMODE_RUNNING);
		}
		break;
	case eMODE_CHANGE_REQUEST_PAUSE_RESUME:
		if (countdown_can_pause(s_mode))
		{
			adl_logln(LOG_APP, "Countdown paused");
			set_mode(eMODE_PAUSED);
		}
		else if (countdown_can_resume(s_mode))
		{
			adl_logln(LOG_APP, "Countdown resuming");
			set_mode(eMODE_RUNNING);
		}
		break;
	case eMODE_CHANGE_REQUEST_RESET:
		if (countdown_can_reset(s_mode))
		{
			adl_logln(LOG_APP, "Countdown reset");
			set_leds_idle();
			set_mode(eMODE_IDLE);
		}
		break;
	default:
		adl_logln(LOG_APP, "Code not recognised");
		break;
	}
}

static bool check_override_param(int32_t param)
{
	bool override = true;

	if (param > 0)
	{
		adl_logln(LOG_APP, "Got override %d", param);
	}

	switch(param)
	{
	case 0:
		override = false;
		break;
	case 1:
		handle_mode_change_request(eMODE_CHANGE_REQUEST_START_NORMAL);
		break;
	case 2:
		handle_mode_change_request(eMODE_CHANGE_REQUEST_START_TEST);
		break;
	case 3:
		handle_mode_change_request(eMODE_CHANGE_REQUEST_PAUSE_RESUME);
		break;
	case 4:
		handle_mode_change_request(eMODE_CHANGE_REQUEST_RESET);
		break;
	default:
		adl_logln(LOG_APP, "Override not recognised");
		override = false;
		break;
	}

	return override;
}

static void check_ir_remote()
{
	unsigned long code;
	if (s_pIR->get_code(code))
	{

		adl_logln(LOG_APP, "Got code %lx", code);

		switch(code)
		{
		case START_NORMAL_CODE:
			handle_mode_change_request(eMODE_CHANGE_REQUEST_START_NORMAL);
			break;
		case START_TEST_CODE:
			handle_mode_change_request(eMODE_CHANGE_REQUEST_START_TEST);
			break;
		case PAUSE_RESUME_CODE:
			handle_mode_change_request(eMODE_CHANGE_REQUEST_PAUSE_RESUME);
			break;
		case RESET_CODE:
			handle_mode_change_request(eMODE_CHANGE_REQUEST_RESET);
			break;
		default:
			adl_logln(LOG_APP, "Code not recognised");
			break;
		}
	}
}

static void play_intro()
{
	uint8_t i;
	for(i=0;i<DAYS_IN_ADVENT;i++)
	{
		s_pNeoPixels->setPixelColor(map_led_to_px_idx(i), 32, 32, 32);
		s_pNeoPixels->show();
		delay(50);
	}

	for(i=0;i<DAYS_IN_ADVENT;i++)
	{
		s_pNeoPixels->setPixelColor(map_led_to_px_idx(DAYS_IN_ADVENT-i-1), 0, 0, 0);
		s_pNeoPixels->show();
		delay(50);
	}
}

/* Public Functions */

void adl_custom_setup(DeviceBase * pdevices[], int ndevices, ParameterBase * pparams[], int nparams)
{
	(void)ndevices; (void)pparams; (void)nparams;
	
	s_pIR = (IR_Receiver*)pdevices[0];
	s_pNeoPixels = (AdafruitNeoPixelADL*)pdevices[1];

	adl_logln(LOG_APP, "Recognised codes:");
	adl_logln(LOG_APP, "Start normal: %lx", START_NORMAL_CODE);
	adl_logln(LOG_APP, "Start test: %lx", START_TEST_CODE);
	adl_logln(LOG_APP, "Pause/resume: %lx", PAUSE_RESUME_CODE);
	adl_logln(LOG_APP, "Reset: %lx", RESET_CODE);

	play_intro();

	set_leds_idle();
}

void adl_custom_loop(DeviceBase * pdevices[], int ndevices, ParameterBase * pparams[], int nparams)
{
	(void)pdevices; (void)ndevices; (void)pparams; (void)nparams;

	check_ir_remote();

	IntegerParam* pOverrideParam = (IntegerParam*)(pparams[0]);
	if (check_override_param(pOverrideParam->get()))
	{
		pOverrideParam->set(0);
	}

	if (s_mode == eMODE_RUNNING)
	{
		s_countdown_task.run();
	}
}
