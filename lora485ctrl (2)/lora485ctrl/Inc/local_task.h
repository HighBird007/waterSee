#ifndef __LOCAL_TASK_H__
	#define __LOCAL_TASK_H__

	#include "stm32l4xx.h"

    #define LOCAL_RX_MAX_FRAME_LEN  		256
	#define LOCAL_RX_FRAME_TIMEOUT   	    50
	
	typedef struct
	{
		uint16_t rx_counter;
		uint16_t rx_timeout_msec;
		uint8_t  rx_done_flag;
		uint8_t  rx_buf[LOCAL_RX_MAX_FRAME_LEN];
	}LOCAL_RX_STRUCT;
	
	void LocalUsartInit(void);
	void LocalRxIsDone(void);
	void Print(uint8_t data[],uint16_t len);
	void LocalTask(void);
#endif