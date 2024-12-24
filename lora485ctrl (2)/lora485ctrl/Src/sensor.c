#include "sensor.h"

sensorStruct ZW;

sensorStruct ZD;

extern UART_HandleTypeDef RS485;

uint8_t read[8] = {0x00,0x03,0x00,0x50,0x00,0x02};

void initSensor(void){

	ZW.addr = ZWADDR;
	
	ZD.addr = ZDADDR;
	
	ZW.flag = 0;
	
	ZD.flag = 0;
	
	ZD.loraDataLength = 0;
	
	ZW.loraDataLength = 0;
	
	ZW.loraDataLengthShould = 32;
	
	ZD.loraDataLengthShould = 8;
	
	ZW.num = 40;
	
	ZD.num = 4;
	
}
void ZWRead(){
	Print("measure zw\n",strlen("measure zw\n"));
	getSensorData(&ZW,0x50,6);
        
        getSensorData(&ZW,0x62,10);
		
}
void ZDRead(){
	
  Print("zd smeasure\n",strlen("zd measure\n"));
	getSensorData(&ZD,0x36,2);
	
	getSensorData(&ZD,0x56,2);

}

void sensorToLora(sensorStruct * s,uint8_t length){
  
	FeedDog();
        
	if(s->loraDataLength == s->loraDataLengthShould || s->loraDataLength == 0){
        
	memcpy(s->toLora,s->sensorData+3,length*2);
	 
	s->loraDataLength = length*2;
	
        
	}else {
	
	memcpy(s->toLora+(s->loraDataLength ),s->sensorData+3,length*2);
	
	s->loraDataLength += length*2;
	
	}
        
}

void getSensorData(sensorStruct * s , uint8_t startReg , uint8_t length){
	
	read[0] = s->addr;
	
	read[3] = startReg;
	
	read[5] = length;
	
	uint16_t crc = getModbusCRC16(read,6);
	
	read[6] = crc&0xFF;
	
	read[7] = (crc>>8)&0xFF;
        
	FeedDog();
        

        for(int i = 0; i < 10; i++){
        
        
        FeedDog();
        
        HAL_UART_Transmit(&RS485,read,8,1000);
       
	if(HAL_UART_Receive(&RS485,s->sensorData,length*2+3+2 ,1000) == HAL_OK){
          
          	FeedDog();
                
		sensorToLora(s,length);

                return ;
                
	} else {
        
         if(i==9){
           
           memset(s->sensorData, 0xFF, sizeof(s->sensorData));  // 将sensorData全设为0xFF
           
           sensorToLora(s,length);
           
           Print("sensor error\n",strlen("sensor error\n"));
                      
           return ;
           
           
         }
        Print("defeat",5);
        HAL_Delay(500);
        FeedDog();
        }       
       
        }
             
}
