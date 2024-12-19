
#ifndef __LOOP__H
#define __LOOP__H

#include "main.h"

#include "tim.h"

#include "ElectricRelay.h"

#include "sensor.h"


#define true 1
#define false 0
#define HIGH 1
#define LOW 0

#define RS485 huart1

//水位高低引脚以及中断的设置
#define HighFlag_Pin GPIO_PIN_1
#define HighFlag_GPIO_Port GPIOA
#define HighFlag_EXTI_IRQn EXTI1_IRQn
#define LowFlag_Pin GPIO_PIN_4
#define LowFlag_GPIO_Port GPIOA
#define LowFlag_EXTI_IRQn EXTI4_IRQn

extern uint8_t node;

extern uint8_t measureCount;

extern uint8_t level ;

void initWaterLevelGPIO(void);

void initRS485UART(void);

void initLoop(void);

void loop(void);

void closeFilling(void);

void openFilling(void);
	
void closeDraining(void);

void openDraining(void);

void assembleLoraData();

#endif

