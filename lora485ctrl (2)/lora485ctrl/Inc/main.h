#ifndef __MAIN_H__
#define __MAIN_H__

	#ifdef MAIN_PARA_GLOBAL
		#define EXTERN_MAIN_PARA
	#else
		#define EXTERN_MAIN_PARA extern
	#endif
	
	#include "stm32l4xx.h"
	
	#define IRQ_DISABLE() (__disable_irq())
	#define IRQ_ENABLE()  (__enable_irq())

	typedef struct
	{
		uint8_t   lora_tx_led_on_flag;
		uint8_t   lora_rx_led_on_flag;
		
		uint8_t   comm_tx_led_on_flag;
		uint8_t   comm_rx_led_on_flag;
		
		uint32_t  ms_counter;
	}SYS_MISC_PARA_STRUCT;
	
	EXTERN_MAIN_PARA SYS_MISC_PARA_STRUCT   sys_misc_para;
	void LedTask(void);
	void SystemClock_Config(void);
#endif