
#ifndef __LOOP__H
#define __LOOP__H

#include "main.h"
#include "tim.h"
#include "ElectricRelay.h"
#include "sensor.h"
#include "stdio.h"
#include "GPIO.h"
#include "string.h"
#include "head.h"
#include "usart.h"
#include "stdio.h"

#define true 1
#define false 0
#define HIGH 1
#define LOW 0

#define RS485 huart1

#define relayAdr 0x02
#define ZWADDR 0x01
#define ZDADDR 0x03

typedef enum {normal , loraError , relayError , pumpError }errorType;

typedef struct{
    
    float tp;
    float ph;
    float o2;
    float zd;
    
}pondStruct;

extern pondStruct pondSet[];

extern errorType deviceStatus;

extern uint8_t node;

extern uint8_t measureCount;

extern uint8_t level ;

extern uint8_t screenRequestLength;

void flushTask();
void measureTask();
void initWaterLevelGPIO(void);

void initRS485UART(void);

void initLoop(void);

void loop(void);

void judgeWaterLevel();

void closeFilling(void);

void openFilling(void);
	
void closeDraining(void);

void openDraining(void);

void assembleLoraData();

void handingError(errorType);

#endif

