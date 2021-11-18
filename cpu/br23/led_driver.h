#ifndef _LED_DRIVER_H_
#define _LED_DRIVER_H_

#include "generic/typedef.h"

void led_init(void);

void led_red_on(void);

void led_red_off(void);

void led_green_on(void);

void led_green_off(void);

void led_blue_on(void);

void led_blue_off(void);
void led_blink_time(int blink_gap_2ms, int blink_time);

void led_power_on_show(void);
void led_power_off_show(void);

#endif
