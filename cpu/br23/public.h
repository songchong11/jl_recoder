#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include "generic/typedef.h"
#include "misc_driver.h"
#include "led_driver.h"
#include "lwrb.h"

typedef struct re_state {

	bool recoder_state;
	bool send_pcm_state;
	bool sd_state;
	bool sim_state;
	bool creg_state;

}RECODER;


extern RECODER	recoder;

#endif
