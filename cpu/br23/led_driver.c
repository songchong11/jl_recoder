#include "includes.h"
#include "app_config.h"
#include "led_driver.h"

#define LED_R_GPIO		IO_PORTA_10
#define LED_G_GPIO		IO_PORTA_08
#define LED_B_GPIO		IO_PORTA_07

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

void led_blink_time(int blink_gap_2ms, int blink_time)
{
	int g;

	for (int i = 0; g < blink_time; i++) {
		g = i * 2 * blink_gap_2ms;
		led_red_on();
		delay_2ms(blink_gap_2ms);
		led_red_off();
	}
}


void led_power_on_show(void)
{
	printf("led_power_on_show \n");
	led_blue_on();
	wdt_clear();
	delay_2ms(500 * 3);//3s
	wdt_clear();
	led_blue_off();
}


void led_power_off_show(void)
{
	led_green_on();
	wdt_clear();
	delay_2ms(500 * 3);//3s
	wdt_clear();
	led_green_off();
}



/*****************************************************************************/
