#include "src/config.h"
#include "src/lcd.h"
#include "src/sm.h"
#include "src/web.h"
#include "src/IO.h"

#include <WiFiManager.h>

uint32_t sleep_timer = 0;
bool power = 0;

void setup()
{
  IO_setup();

  WiFiManager wifiManager;
}

void loop()
{
  lcd_update();

  web_update();

  if (!sleep_timer) {}
  else if (millis() > sleep_timer)
  {
    sleep_timer = 0;

    power = false;
  }

}

void resetwifi()
{
  //wifiManager.resetSettings();
  WiFiManager wifiManager;
  wifiManager.autoConnect("LET Smart AC #1 Setup", "LET-S3tup!");
}