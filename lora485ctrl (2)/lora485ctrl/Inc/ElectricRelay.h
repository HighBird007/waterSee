#ifndef __Relay__H
#define __Relay__H

#include "Loop.h"

#include "CRC16MODBUS.h"

//继电器的485地址

#define drainingPumpRoad 0x0B

#define powerOn 0xFF

#define powerOff 0x00

void controlDeviceStatus(uint8_t road , uint8_t openOrclose);
	
#endif

