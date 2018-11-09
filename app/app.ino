















/*
app
Created on 2018-11-09
By the Arduino Description Language tool.
 

		Application for the Xscape advent timer
	
*/





#include <stdint.h>

#include "adl.h"


#include <IRremote.h>

#include <Adafruit_NeoPixel.h>



    #include "adl-util-limited-range-int.h"



#include "IR-receiver.h"

#include "adafruit-neopixel-adl.h"



#include "integer-param.h"



static IR_Receiver s_ir_receiver = IR_Receiver(11);

static AdafruitNeoPixelADL s_neopixels = AdafruitNeoPixelADL(3, 25, NEO_GRB + NEO_KHZ800);


static DeviceBase * s_devices[] = 
{
    
    &s_ir_receiver
    
    ,
    
    
    &s_neopixels
    
    
};


static IntegerParam s_ir_command = IntegerParam(0, 0, 4, false, false);


static ParameterBase * s_params[] = 
{
    
    &s_ir_command
    
    
};






int handle_device1_command(char const * const command, char * reply)
{
    return s_ir_receiver.command_handler(command, reply);
}

int handle_device2_command(char const * const command, char * reply)
{
    return s_neopixels.command_handler(command, reply);
}


static COMMAND_HANDLER adl_devices[] = {
    
    handle_device1_command,
    
    handle_device2_command,
    
};

COMMAND_HANDLER& adl_get_device_cmd_handler(DEVICE_ADDRESS address)
{
    return adl_devices[address-1];
}

DeviceBase& adl_get_device(DEVICE_ADDRESS address)
{
    return *s_devices[address-1];
}


int handle_param1_command(char const * const command, char * reply)
{
    return s_ir_command.command_handler(command, reply);
}


static COMMAND_HANDLER adl_params[] = {
    
    handle_param1_command,
    
};

COMMAND_HANDLER& adl_get_param_cmd_handler(PARAM_ADDRESS address)
{
    return adl_params[address-1];
}

ParameterBase& adl_get_param(PARAM_ADDRESS address)
{
    return *s_params[address-1];
}



void adl_board_send(char * to_send)
{
    Serial.println(to_send);
}


void setup()
{
    adl_on_setup_start();

    Serial.begin(115200);
    
    adl_nonvolatile_setup();

    adl_logging_setup(Serial);

    
    // Setup for IR Receiver
    s_ir_receiver.setup();
    // END IR Receiver setup
    
    // Setup for NeoPixels
    s_neopixels.setup();
    // END NeoPixels setup
    

    
    // Setup for IR Command
    s_ir_command.setup();
    // END IR Command setup
    

    adl_custom_setup(s_devices, ADL_DEVICE_COUNT, s_params, ADL_PARAM_COUNT);

    adl_on_setup_complete();
    
    if (0)
    {
        adl_delay_start( 0 );
    }
}



void loop()
{
    adl_handle_any_pending_commands();
    adl_service_timer();
    adl_custom_loop(s_devices, ADL_DEVICE_COUNT, s_params, ADL_PARAM_COUNT);
}



        void serialEvent()
        {
            while (Serial.available())
            {
                adl_add_char((char)Serial.read());
            }
        }



