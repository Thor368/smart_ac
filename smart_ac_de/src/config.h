#include <Arduino.h>

#define temp_setpoint_min 15.0
#define temp_setpoint_max 30.0
#define sleeptime_max 300

#define adr_sensor_interieur { 0x28, 0x99, 0x8E, 0x56, 0xB5, 0x01, 0x3C, 0xD1 }
#define adr_sensor_exterieur { 0x28, 0xF9, 0x17, 0x56, 0xB5, 0x01, 0x3C, 0x56 }

extern bool power;
extern bool compressor;
extern bool condensate_pump;
extern bool coolant_pump;
extern bool lockout;
extern bool partymode;

extern uint32_t fan_mode;

extern uint32_t sleep_timer;

extern float temp_setpoint;
extern float temp_interior;
extern float temp_outside;