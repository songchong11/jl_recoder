#include "includes.h"
#include "misc_driver.h"
#include "led_driver.h"

#define BES_PWR_GPIO	IO_PORTA_12
#define BES_RECODER_GPIO	IO_PORTB_07


#define CE_GPIO			IO_PORTC_03
#define SYSOFF_GPIO		IO_PORTC_02
#define EN01_GPIO		IO_PORTC_01
#define EN02_GPIO		IO_PORTC_00
#define USB_DECT		IO_PORTA_09

int usb_check_timer;

void misc_driver_init(void)
{

	/*bes power gpio init*/
	gpio_set_pull_up(BES_PWR_GPIO, 0);
	gpio_set_pull_down(BES_PWR_GPIO, 0);
	gpio_set_die(BES_PWR_GPIO, 0);
	gpio_set_direction(BES_PWR_GPIO, 0);
	gpio_set_output_value(BES_PWR_GPIO, 0);

	/*bes start or stop gpio init*/
	gpio_set_pull_up(BES_RECODER_GPIO, 0);
	gpio_set_pull_down(BES_RECODER_GPIO, 0);
	gpio_set_die(BES_RECODER_GPIO, 0);
	gpio_set_direction(BES_RECODER_GPIO, 0);
	gpio_set_output_value(BES_RECODER_GPIO, 1);

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


	gpio_direction_input(USB_DECT);
	gpio_set_pull_down(USB_DECT, 0);
	gpio_set_pull_up(USB_DECT, 0);
	gpio_set_die(USB_DECT, 1);

	// add a timer to check the usb plug
	printf("start a timer to check usb plug in\n");

	if (usb_check_timer == 0) {
		usb_check_timer = sys_timer_add(NULL, check_charge_usb_status, 100);
	}

	bes_power_on();

}

void bes_power_on(void)
{
	printf("bes_power_on\r\n");
	gpio_set_output_value(BES_PWR_GPIO, 1);
	delay_2ms(500);
	gpio_set_output_value(BES_PWR_GPIO, 0);
}

void bes_power_off(void)// TODO:check
{
	printf("bes_power_off\r\n");
	gpio_set_output_value(BES_PWR_GPIO, 1);
	delay_2ms(500);
	gpio_set_output_value(BES_PWR_GPIO, 0);

}

extern u32 rx_total;
void bes_start_recoder(void)
{
	bes_power_on();
	delay_2ms(30);
	printf("bes_start_recoder\r\n");
	gpio_set_output_value(BES_RECODER_GPIO, 0);
	rx_total = 0;
	led_blue_on();
}

void bes_stop_recoder(void)
{
	printf("bes_stop_recoder\r\n");
	gpio_set_output_value(BES_RECODER_GPIO, 1);
	led_blue_off();
}

void check_charge_usb_status(void *priv)
{
	static int pre_status = 0;

	int ret = gpio_read(USB_DECT);

	wdt_clear();

	if (ret != pre_status) {

		if (ret == 1) {
			printf("usb charge insert\n");
			//gpio_set_output_value(SYSOFF_GPIO, 1);
			gpio_set_output_value(CE_GPIO, 0);//CE low start charing
			gpio_set_output_value(EN01_GPIO, 1); //500mA
			gpio_set_output_value(EN02_GPIO, 0);

		} else {
			printf("usb charge remove\n");
			gpio_set_output_value(CE_GPIO, 1);//CE high stop charing
			//gpio_set_output_value(SYSOFF_GPIO, 0);
			gpio_set_output_value(EN01_GPIO, 1); //standby mode
			gpio_set_output_value(EN02_GPIO, 1);

		}
	}

	pre_status = ret;

}

/*****************************************************************************/
