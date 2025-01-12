#ifndef GPIO_H
#define GPIO_H
#include "main.h"
#include "Loop.h"
//水位高低引脚以及中断的设置
#define LowFlag_Pin GPIO_PIN_14
#define LowFlag_GPIO_Port GPIOB
#define LowFlag_EXTI_IRQn EXTI15_10_IRQn
#define HighFlag_Pin GPIO_PIN_15
#define HighFlag_GPIO_Port GPIOB
#define HighFlag_EXTI_IRQn EXTI15_10_IRQn

/*
pb13   时低水平位置  也就是 pb14

pb12  时高液位位  对应的时pb15

*/

void initWaterLevelGPIO_IT(void);

void  initWaterLevelGPIO_Input(void);
    
    
#endif