#include "sensor.h"
#include "usart.h"
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

	getSensorData(&ZW,0x50,6);
        
        getSensorData(&ZW,0x62,10);
		
}
void ZDRead(){
	
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

#define errorNum 5

//LENGTH翻译不好  其实是寄存器的个数
void getSensorData(sensorStruct * s , uint8_t startReg , uint8_t length){
	
	read[0] = s->addr;
	
	read[3] = startReg;
	
	read[5] = length;
	
	uint16_t crc = getModbusCRC16(read,6);
	
	read[6] = crc&0xFF;
	
	read[7] = (crc>>8)&0xFF;
        
	FeedDog();
        
        HAL_UART_Abort_IT(&huart1);
        
        for(int i = 0; i < errorNum; i++){
        
        
        FeedDog();
        
        HAL_UART_Transmit(&RS485,read,8,1000);
       
	if(HAL_UART_Receive(&RS485,s->sensorData,length*2+3+2 ,1000) == HAL_OK){
          
          	FeedDog();
                
                uint16_t crcCheck = getModbusCRC16(s->sensorData,length*2+3);

                // 验证 CRC 校验码是否匹配
                if ((crcCheck&0xFF == s->sensorData[length*2+3])&&((crcCheck>>8)&0xFF == s->sensorData[length*2+4])) {
                
    		sensorToLora(s,length);
                
                HAL_UART_Receive_IT(&huart1,dataArr,screenRequestLength);
                
                return ;
                
                } else {
                  
                continue;
                
                }
                
                
	} else {
        
         if(i>= errorNum -1 ){
           
           memset(s->sensorData, 0xFF, sizeof(s->sensorData));  // 将sensorData全设为0xFF
           
           sensorToLora(s,length);
           
           HAL_UART_Receive_IT(&huart1,dataArr,screenRequestLength);      
           
           return ;
           
         }
        HAL_Delay(100);
        FeedDog();
        }       
       
        }
             
}

void updatePondStructData(){
    
    //传感器传输过来的 以及 单片机都是小端对齐 
    memcpy(&pondSet[node].tp,ZW.toLora,4);
    
    memcpy(&pondSet[node].o2,ZW.toLora+8,4);
    
    memcpy(&pondSet[node].ph,ZW.toLora+24,4);
    
    memcpy(&pondSet[node].zd,ZD.toLora,4);
  
}
