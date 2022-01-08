#ifndef _MISC_DRIVER_H_
#define _MISC_DRIVER_H_

#include "generic/typedef.h"

void misc_driver_init(void);

void bes_power_on(void);
void bes_power_off(void);
void bes_start_recoder(void);
void bes_stop_recoder(void);
void check_charge_usb_status(void *priv);
void check_4G_power_status(void);

#endif
