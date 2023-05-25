#include "config.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <IRrecv.h>

const int pin_water_level = 15;       //D0 BK
const int pin_one_wire_bus = 5;       //D1 OR
const int pin_pump_condensate = 4;    //D2
const int pin_pump_coolant = 0;       //D3
const int pin_ir_receiver = 2;        //D4 BL
const int pin_compressor = 14;        //D5
const int pin_fan_low = 12;           //D6
const int pin_fan_med = 13;           //D7
const int pin_fan_hi = 16;            //D8

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

IRrecv irrecv(irreceiver);

uint32_t fan_mode = 0;
uint32_t IO_timer = 0;

void IO_setup()
{
	Serial.begin(9600);

	sensors.begin();

	irrecv.enableIRIn();

	pinMode(pin_compressor, OUTPUT);
	pinMode(pin_fan_low, OUTPUT);
	pinMode(pin_fan_med, OUTPUT);
	pinMode(pin_fan_hi, OUTPUT);
	pinMode(pin_pump_condensate, OUTPUT);
  pinMode(pin_pump_coolant, OUTPUT);
	pinMode(pin_water_level, INPUT);

	digitalWrite(pin_compressor, HIGH);
	digitalWrite(pin_fan_low, HIGH);
  digitalWrite(pin_fan_med, HIGH);
	digitalWrite(pin_fan_hi, HIGH);
	digitalWrite(pin_pump_condensate, HIGH);
  digitalWrite(pin_pump_coolant, HIGH);
}
	

void IO_update(void)
{
  if (IO_timer < millis())
  {
    IO_timer = millis() + 1000;

    temp_interior = sensors.getTempC(deviceAddress);
    temp_outside = sensors.getTempC(deviceAddress);;
  }

  if (compressor)
    digitalWrite(pin_compressor, LOW);
  else
    digitalWrite(pin_compressor, HIGH);
  
  if (condensate_pump)
    digitalWrite(pin_pump_condensate, LOW);
  else
    digitalWrite(pin_pump_condensate, HIGH);
  
  if (coolant_pump)
    digitalWrite(pin_pump_coolant, LOW);
  else
    digitalWrite(pin_pump_coolant, LOW);
  
	// condensate
  if (!drainstate) && !digitalRead(water_level))
	{
    drainstate = millis() + 8000;

    condensate_pump = true;
  }
  else if (millis() > drainstate)
  {
    drainstate = -1;

    condensate_pump = false;
  }

  // IR decoder
  decode_results results;
  while (irrecv.decode(&results))
	{
		switch (results.value)
    {
      case 3674580118:
  			// increase setpoint
        break;
      
      case 2882851182:
        // decrease setpoint
        break;

      case 4140943054:
        // power
        break;

      case 1009394670:
        // fan
        break;
      
      case 1464959974:
        // increase sleep timer
        break;
      
      case 3638877206:
        // decrease sleep timer
        break;
    }

		irrecv.resume();
	}

  //poll tempsensors
	unsigned long ctempm = millis();
	if ((ctempm - tempm) >= temppoll)
	{
		tempm = ctempm;
		sensors.requestTemperatures();
		gettempin(tempsensorin);
		gettempout(tempsensorout);
	}
}