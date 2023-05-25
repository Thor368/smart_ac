#include "config.h"

#include <Arduino.h>

enum sm_state_t
{
  sm_init,
  sm_comp_lockout,
  sm_idle
} sm_state;

uint32_t timer = 0;

bool compressor = false;
bool condensate_pump = false;
bool coolant_pump = false;
bool partymode = false;


void sm_update(void)
{
  switch (sm_state) {
    case sm_init:
      compressor = false;
      condensate_pump = false;
      coolant_pump = false;
      partymode = false;

      timer = millis() + 60000;
      sm_state = sm_comp_lockout;
      break;

    case sm_comp_lockout:
      if (millis() > timer)
        sm_state = sm_idle;
      break;

    case sm_idle:
      break;
  }
}