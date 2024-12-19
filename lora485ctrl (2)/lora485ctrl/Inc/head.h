#ifndef __HEAD_H__
    #define __HEAD_H__
	
	#include "stm32l4xx_hal.h"
	#include "stm32l4xx.h"
	
	#define IRQ_DISABLE() (__disable_irq())
	#define IRQ_ENABLE()  (__enable_irq())
	#define 	true 		 1
    #define 	false 		 0
	
	#include <stdbool.h> 
	#include <string.h>
	#include <math.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <time.h>
	#include <stdio.h>
	
	
	#include "local_task.h"
    #include "lora_task.h"
	#include "main.h"
	
	#include "mcu_hw.h"
	
	#include "aes.h"
	#include "cmac.h"
	#include "sx126x.h"
	#include "lora_mac.h"
	#include "lora_mac_crypto.h"
	
    #include "sensor_task.h"

	#include "bsp_lib.h"
#endif