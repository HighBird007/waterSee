#ifndef __TRANS_TASK_H__
	#define __TRANS_TASK_H__

	#include "stm32l4xx.h"

    #define TRANS_RX_MAX_FRAME_LEN  		222
	#define TRANS_RX_FRAME_TIMEOUT   	    25
	
	typedef struct
	{
		uint16_t rx_counter;
		uint16_t rx_timeout_msec;
		uint8_t  rx_done_flag;
		uint8_t  rx_buf[LOCAL_RX_MAX_FRAME_LEN];
	}TRANS_RX_STRUCT;
	
	void Usart1Init(void);
	void Usart1Tx(uint8_t data[],uint16_t len);
	void Usart1RxIsDone(void);
	void TransTask(void);
#endif