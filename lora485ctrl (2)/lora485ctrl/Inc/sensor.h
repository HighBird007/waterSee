#ifndef sensor_H
#define sensor_H

#include "CRC16MODBUS.h"

#include "Loop.h"

#include "string.h"

#define ZWADDR 0x01

#define ZDADDR 0x03

typedef struct{
	
	uint8_t addr;
	//传感器的原始数据
	uint8_t sensorData[100];
	//数据转为lora关键数据
	uint8_t toLora[100];
	//toLora的有效长度
	uint8_t loraDataLength;
	//toLora的有效长度理论应该值
	uint8_t loraDataLengthShould;
	//每次塞入数据的数量 目前无用
	uint8_t num;
	//1为数据有效 否则无效
	uint8_t flag;
	
}sensorStruct;

extern sensorStruct ZW;

extern sensorStruct ZD;
//初始化传感器结构体
void initSensor(void);
//s是传感器结构体 length是  ###寄存器的数量   
void getSensorData(sensorStruct * s , uint8_t startReg , uint8_t length);

void ZWRead(void);

void ZDRead(void);

//将传感器的数据放入对应的池子结构体  此函数应该在传感器数据放入toLora数组中后使用
void updatePondStructData(void);

#endif

