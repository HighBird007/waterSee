
#ifndef __LOOP__H
#define __LOOP__H

#include "main.h"

#include "tim.h"

#include "ElectricRelay.h"

#include "sensor.h"

#include "stdio.h"

#include "string.h"

#define true 1
#define false 0
#define HIGH 1
#define LOW 0

#define RS485 huart1

typedef enum {normal , loraError , relayError , pumpError }errorType;

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

void handingError(errorType);

#endif

