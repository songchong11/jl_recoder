#include "includes.h"
#include "app_config.h"

#define LED_R_GPIO		IO_PORTA_10
#define LED_G_GPIO		IO_PORTA_08
#define LED_B_GPIO		IO_PORTA_07

void led_init(void)
{

	gpio_set_pull_up(LED_R_GPIO, 1);
	gpio_set_pull_down(LED_R_GPIO, 1);
	gpio_set_die(LED_R_GPIO, 1);
	gpio_set_direction(LED_R_GPIO, 0);
	gpio_set_output_value(LED_R_GPIO, 1);


	gpio_set_pull_up(LED_G_GPIO, 1);
	gpio_set_pull_down(LED_G_GPIO, 1);
	gpio_set_die(LED_G_GPIO, 1);
	gpio_set_direction(LED_G_GPIO, 0);
	gpio_set_output_value(LED_G_GPIO, 1);


	gpio_set_pull_up(LED_B_GPIO, 1);
	gpio_set_pull_down(LED_B_GPIO, 1);
	gpio_set_die(LED_B_GPIO, 1);
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



/*****************************************************************************/
