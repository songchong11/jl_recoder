#include "includes.h"
#include "misc_driver.h"

#define BES_PWR_GPIO	IO_PORTA_12

#define CE_GPIO			IO_PORTC_03
#define SYSOFF_GPIO		IO_PORTC_02
#define EN01_GPIO		IO_PORTC_01
#define EN02_GPIO		IO_PORTC_00

void misc_driver_init(void)
{

	/*bes power gpio init*/
	gpio_set_pull_up(BES_PWR_GPIO, 1);
	gpio_set_pull_down(BES_PWR_GPIO, 0);
	gpio_set_die(BES_PWR_GPIO, 0);
	gpio_set_direction(BES_PWR_GPIO, 0);
	gpio_set_output_value(BES_PWR_GPIO, 0);

	/*charge ic init*/
	gpio_set_pull_up(SYSOFF_GPIO, 0);
	gpio_set_pull_down(SYSOFF_GPIO, 0);
	gpio_set_die(SYSOFF_GPIO, 0);
	gpio_set_direction(SYSOFF_GPIO, 0);
	gpio_set_output_value(SYSOFF_GPIO, 0);


	gpio_set_pull_up(EN01_GPIO, 0);
	gpio_set_pull_down(EN01_GPIO, 0);
	gpio_set_die(EN01_GPIO, 0);
	gpio_set_direction(EN01_GPIO, 0);
	gpio_set_output_value(EN01_GPIO, 1);

	gpio_set_pull_up(EN02_GPIO, 0);
	gpio_set_pull_down(EN02_GPIO, 0);
	gpio_set_die(EN02_GPIO, 0);
	gpio_set_direction(EN02_GPIO, 0);
	gpio_set_output_value(EN02_GPIO, 1);

	gpio_set_pull_up(CE_GPIO, 0);
	gpio_set_pull_down(CE_GPIO, 0);
	gpio_set_die(CE_GPIO, 0);
	gpio_set_direction(CE_GPIO, 0);
	gpio_set_output_value(CE_GPIO, 1);

	bes_power_on();
}

void bes_power_on(void)
{
	printf("bes_power_on\r\n");
	gpio_set_output_value(BES_PWR_GPIO, 1);
	delay_2ms(100);
	gpio_set_output_value(BES_PWR_GPIO, 0);
}

void bes_power_off(void)// TODO:check
{
	printf("bes_power_off\r\n");
	gpio_set_output_value(BES_PWR_GPIO, 1);
	delay_2ms(40);
	gpio_set_output_value(BES_PWR_GPIO, 0);

}



/*****************************************************************************/
