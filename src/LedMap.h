#ifndef _LEDMAP_H
#define _LEDMAP_H

#include <stdint.h>

typedef struct{
    uint8_t State;
    uint8_t Direction;
} LedMap;

const LedMap LED_0 = {0x04, 0x0C};
const LedMap LED_1 = {0x02, 0x03};
const LedMap LED_2 = {0x01, 0x03};
const LedMap LED_3 = {0x01, 0x05};
const LedMap LED_4 = {0x01, 0x09};
const LedMap LED_5 = {0x08, 0x09};
const LedMap LED_6 = {0x08, 0x0C};
const LedMap LED_7 = {0x8, 0x0A};
const LedMap LED_8 = {0x2, 0x06};
const LedMap LED_9 = {0x02, 0x0A};
const LedMap LED_10 = {0x04, 0x06};
const LedMap LED_11 = {0x04, 0x05};
const LedMap LEDS_OFF = {0,0};

#define number_of_leds 13
const LedMap* all_leds[number_of_leds] = 
{
  &LED_0,
  &LED_1,
  &LED_2,
  &LED_3,
  &LED_4,
  &LED_5,
  &LED_6,
  &LED_7,
  &LED_8,
  &LED_9,
  &LED_10,
  &LED_11,
  &LEDS_OFF
};

#endif //_PINMAP_H
