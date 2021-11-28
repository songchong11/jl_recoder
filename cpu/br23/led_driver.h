#ifndef _LED_DRIVER_H_
#define _LED_DRIVER_H_

#include "generic/typedef.h"

enum {
	LED_RED,
	LED_BLUE,
	LED_GREEN,
	LED_BLUE_GREEN,
	LED_RED_BLUE,
	LED_RED_GREEN,
	LED_ALL,
};

typedef struct led_device {

	u16 timer_id;
	u16 times;
} LED;


void led_init(void);

void led_red_on(void);

void led_red_off(void);

void led_green_on(void);

void led_green_off(void);

void led_blue_on(void);

void led_blue_off(void);

void led_power_on_show(void);
void led_power_on_show_end(void);

void led_power_off_show(void);
void led_green_toggle(void);
void led_blue_toggle(void);
void led_red_toggle(void);
void led_blink_time(int blink_gap_ms, int blink_time_ms, int led_type);


#endif
