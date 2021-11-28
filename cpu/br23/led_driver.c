#include "includes.h"
#include "app_config.h"
#include "led_driver.h"

#define LED_R_GPIO		IO_PORTA_10
#define LED_G_GPIO		IO_PORTA_08
#define LED_B_GPIO		IO_PORTA_07

LED led_blue;
LED led_red;
LED led_green;
LED led_blue_green;
LED led_blue_red;
LED led_red_green;
LED led_all;

void led_init(void)
{

	gpio_set_pull_up(LED_R_GPIO, 0);
	gpio_set_pull_down(LED_R_GPIO, 0);
	gpio_set_die(LED_R_GPIO, 0);
	gpio_set_direction(LED_R_GPIO, 0);
	gpio_set_output_value(LED_R_GPIO, 1);


	gpio_set_pull_up(LED_G_GPIO, 0);
	gpio_set_pull_down(LED_G_GPIO, 0);
	gpio_set_die(LED_G_GPIO, 0);
	gpio_set_direction(LED_G_GPIO, 0);
	gpio_set_output_value(LED_G_GPIO, 1);


	gpio_set_pull_up(LED_B_GPIO, 0);
	gpio_set_pull_down(LED_B_GPIO, 0);
	gpio_set_die(LED_B_GPIO, 0);
	gpio_set_direction(LED_B_GPIO, 0);
	gpio_set_output_value(LED_B_GPIO, 1);

}

void led_red_on(void)
{
	gpio_set_output_value(LED_R_GPIO, 0);
}

void led_red_off(void)
{
	gpio_set_output_value(LED_R_GPIO, 1);
}

void led_green_on(void)
{
	gpio_set_output_value(LED_G_GPIO, 0);
}

void led_green_off(void)
{
	gpio_set_output_value(LED_G_GPIO, 1);
}

void led_blue_on(void)
{
	gpio_set_output_value(LED_B_GPIO, 0);
}

void led_blue_off(void)
{
	gpio_set_output_value(LED_B_GPIO, 1);
}


void led_blue_red_on(void)
{
	gpio_set_output_value(LED_B_GPIO, 0);
	gpio_set_output_value(LED_R_GPIO, 0);
}

void led_blue_red_off(void)
{
	gpio_set_output_value(LED_B_GPIO, 1);
	gpio_set_output_value(LED_R_GPIO, 1);
}

void led_blue_green_on(void)
{
	gpio_set_output_value(LED_B_GPIO, 0);
	gpio_set_output_value(LED_G_GPIO, 0);
}

void led_blue_green_off(void)
{
	gpio_set_output_value(LED_B_GPIO, 1);
	gpio_set_output_value(LED_G_GPIO, 1);
}

void led_red_green_on(void)
{
	gpio_set_output_value(LED_R_GPIO, 0);
	gpio_set_output_value(LED_G_GPIO, 0);
}

void led_red_green_off(void)
{
	gpio_set_output_value(LED_R_GPIO, 1);
	gpio_set_output_value(LED_G_GPIO, 1);
}

void led_all_on(void)
{
	gpio_set_output_value(LED_R_GPIO, 0);
	gpio_set_output_value(LED_G_GPIO, 0);
	gpio_set_output_value(LED_B_GPIO, 0);
}

void led_all_off(void)
{
	gpio_set_output_value(LED_R_GPIO, 1);
	gpio_set_output_value(LED_G_GPIO, 1);
	gpio_set_output_value(LED_B_GPIO, 1);
}


void led_blue_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_blue_off();
	else
		led_blue_on();

	nor_times++;

	if (nor_times >= led_blue.times) {
		led_blue_off();
		sys_timer_del(led_blue.timer_id);
		led_blue.timer_id = 0;
		led_blue_red.times = 0;
		nor_times = 0;
	}
}

void led_red_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_red_off();
	else
		led_red_on();

	nor_times++;

	if (nor_times >= led_red.times) {
		led_red_off();
		sys_timer_del(led_red.timer_id);
		led_red.timer_id = 0;
		led_blue_red.times = 0;
		nor_times = 0;
	}
}

void led_green_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_green_off();
	else
		led_green_on();

	nor_times++;

	if (nor_times >= led_green.times) {
		led_green_off();
		sys_timer_del(led_green.timer_id);
		led_green.timer_id = 0;
		led_blue_red.times = 0;
		nor_times = 0;
	}
}

void led_red_blue_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_blue_red_off();
	else
		led_blue_red_on();

	nor_times++;

	if (nor_times >= led_blue_red.times) {
		led_blue_red_off();
		sys_timer_del(led_blue_red.timer_id);
		led_blue_red.timer_id = 0;
		led_blue_red.times = 0;
		nor_times = 0;
	}
}

void led_red_green_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_red_green_off();
	else
		led_red_green_on();

	nor_times++;

	if (nor_times >= led_red_green.times) {
		led_red_green_off();
		sys_timer_del(led_red_green.timer_id);
		led_red_green.timer_id = 0;
		led_red_green.times = 0;
		nor_times = 0;
	}
}


void led_blue_green_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_blue_green_off();
	else
		led_blue_green_on();

	nor_times++;

	if (nor_times >= led_blue_green.times) {
		led_blue_green_off();
		sys_timer_del(led_blue_green.timer_id);
		led_blue_green.timer_id = 0;
		led_blue_green.times = 0;
		nor_times = 0;
	}
}

void led_all_blink(void *priv)
{
	static int nor_times = 0;

	if (nor_times % 2 == 0)
		led_all_off();
	else
		led_all_on();

	if (nor_times >= led_all.times) {
		led_all_off();
		sys_timer_del(led_all.timer_id);
		led_all.timer_id = 0;
		led_all.times = 0;
		nor_times = 0;
	}
}


void led_blink_time(int blink_gap_ms, int blink_time_ms, int led_type)
{
	int times = blink_time_ms / blink_gap_ms;

	switch (led_type) {
		case LED_RED:
			led_red.times = times;
			if (led_red.timer_id == 0)
				led_red.timer_id = sys_timer_add(NULL, led_red_blink, blink_gap_ms);
			break;

		case LED_BLUE:
			led_blue.times = times;
			if (led_blue.timer_id == 0)
				led_blue.timer_id = sys_timer_add(NULL, led_blue_blink, blink_gap_ms);
			break;

		case LED_GREEN:
			led_green.times = times;
			if (led_green.timer_id == 0)
				led_green.timer_id = sys_timer_add(NULL, led_green_blink, blink_gap_ms);
			break;

		case LED_BLUE_GREEN:
			led_blue_green.times = times;
			if (led_blue_green.timer_id == 0)
				led_blue_green.timer_id = sys_timer_add(NULL, led_blue_green_blink, blink_gap_ms);
			break;

		case LED_RED_BLUE:
			led_blue_red.times = times;
			if (led_blue_red.timer_id == 0)
				led_blue_red.timer_id = sys_timer_add(NULL, led_red_blue_blink, blink_gap_ms);
			break;

		case LED_RED_GREEN:
			led_red_green.times = times;
			if (led_red_green.timer_id == 0)
				led_red_green.timer_id = sys_timer_add(NULL, led_red_green_blink, blink_gap_ms);
			break;

		case LED_ALL:
			led_all.times = times;
			if (led_all.timer_id == 0)
				led_all.timer_id = sys_timer_add(NULL, led_all_blink, blink_gap_ms);
			break;

		default:
			break;
	}

}


void led_power_on_show(void)
{
	printf("led_power_on_show \n");
	led_blue_on();
	wdt_clear();
	//delay_2ms(500 * 3);//3s
	//wdt_clear();
	//led_blue_off();
}
void led_power_on_show_end(void)
{
	led_blue_off();
	wdt_clear();
	//delay_2ms(500 * 3);//3s
	//wdt_clear();
	//led_blue_off();
}


void led_power_off_show(void)
{
	led_green_on();
	wdt_clear();
	delay_2ms(500 * 3);//3s
	wdt_clear();
	led_green_off();
}

void led_red_toggle(void)
{
	static u8 read_led_status;

	if (read_led_status % 2)
		led_red_on();
	else
		led_red_off();

	read_led_status++;
}


void led_blue_toggle(void)
{
	static u8 blue_led_status;

	if (blue_led_status % 2)
		led_blue_on();
	else
		led_blue_off();

	blue_led_status++;
}

void led_green_toggle(void)
{
	static u8 green_led_status;

	if (green_led_status % 2)
		led_green_on();
	else
		led_green_off();

	green_led_status++;
}

/*****************************************************************************/
