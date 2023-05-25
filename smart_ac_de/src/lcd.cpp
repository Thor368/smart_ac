#include "config.h"

#include <Arduino.h>

#define lcd_wifi_enable Serial.print("vis pwifi,1\0xFF\0xFF\0xFF")
#define lcd_wifi_disable Serial.print("vis pwifi,0\0xFF\0xFF\0xFF")

#define lcd_min_brightness Serial.printf("dim=1\0xFF\0xFF\0xFF")
#define lcd_off_brightness Serial.printf("dim=5\0xFF\0xFF\0xFF")
#define lcd_full_brightness Serial.printf("dim=100\0xFF\0xFF\0xFF")

#define lcd_cond_pump_enable Serial.print("vis ppump,1")
#define lcd_cond_pump_disable Serial.print("vis ppump,0")

#define lcd_compressor_on {Serial.print("vis ledcon,1");Serial.print("vis ledcoff,0");}
#define lcd_compressor_off {Serial.print("vis ledcon,0");Serial.print("vis ledcoff,1");}

uint32_t lcd_timer = 0;

void lcd_update(void)
{
  if (millis() > lcd_timer)
  {
    lcd_timer = millis() + 1000;

 		Serial.printf("tset.txt=\"%d\"\xFF\xFF\xFF", settemp);
    Serial.printf("tsleep.txt=\"%d\"\xFF\xFF\xFF", sleep_time);
  }

  // display wifi status
  if (WiFi.status() == WL_CONNECTED)
    lcd_enable_wifi;
  else
    lcd_disable_wifi;
  
	//display auto-dim
	if (power)
  {
  	uint32_t LDR = analogRead(A0);
  	if (LDR >= 18)
  		lcd_full_brightness;
  	if (LDR <= 18)
  		lcd_min_brightness;
  }
  else
    lcd_off_brightness;

  // display compressor status
  if (compressor)
    lcd_compressor_on;
  else
    lcd_compressor_off;

  // display condensate pump status
  if (condensate_pump)
    lcd_cond_pump_enable;
  else
    lcd_cond_pump_disable;

	//read input from display
	serialin = Serial.readStringUntil('\r');
	if(serialin.equals("refreshsettemp"))
	{
	}
  if(serialin.equals("resetwifi"))
  {
    resetwifi();
  }
  if(serialin.equals("pmodeon"))
  {
    settemp = 5.00;
    partymode= true;
  }
  if(serialin.equals("pmodeoff"))
  {
    settemp = 22.00;
    partymode = false;
  }
	if(serialin.equals("refreshsleeptimer"))
	{
		Serial.print("tsleep.txt=\"");
		Serial.print(sleeptime);
		Serial.print("\"");
		ce();
	}
	if(serialin.equals("TU"))
	{
		settempup();
	}
	if(serialin.equals("TD"))
	{
		settempdown();
	}
	if(serialin.equals("SU"))
	{
		setsleepup();
	}
	if(serialin.equals("SD"))
	{
		setsleepdown();
	}
	if(serialin.equals("SF"))
	{
		setfan();
	}
	if(serialin.equals("PO"))
	{
		setpower();
	}
	if(serialin.equals("drain"))
	{
		drainstate = true;
	}
	if(serialin.equals("pmon"))
	{
    settemp = 5.0;
    fan = 3;
	}
  if(serialin.equals("pmoff"))
  {
    settemp = 20.0;
    fan = 1;
  }
  if(serialin.equals("refreshmain"))
  {
    refreshmain();
  }
}