#ifndef GPIO_H
#define GPIO_H
#include "main.h"
#include "Loop.h"
//ˮλ�ߵ������Լ��жϵ�����
#define LowFlag_Pin GPIO_PIN_14
#define LowFlag_GPIO_Port GPIOB
#define LowFlag_EXTI_IRQn EXTI15_10_IRQn
#define HighFlag_Pin GPIO_PIN_15
#define HighFlag_GPIO_Port GPIOB
#define HighFlag_EXTI_IRQn EXTI15_10_IRQn

/*
pb13   ʱ��ˮƽλ��  Ҳ���� pb14

pb12  ʱ��Һλλ  ��Ӧ��ʱpb15

*/

void initWaterLevelGPIO_IT(void);

void  initWaterLevelGPIO_Input(void);
    
    
#endif