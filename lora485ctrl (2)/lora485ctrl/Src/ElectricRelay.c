#include "ElectricRelay.h"

extern UART_HandleTypeDef RS485;

uint8_t DAMT0FFF_MT_CMD[8] = {relayAdr,0x05,0x00};

void controlDeviceStatus( uint8_t road , uint8_t openOrclose){
	
        DAMT0FFF_MT_CMD[3] = road;
  
	DAMT0FFF_MT_CMD[4] = openOrclose ;
	
	DAMT0FFF_MT_CMD[5] = 0x00;
	
	uint16_t crc = getModbusCRC16(DAMT0FFF_MT_CMD,6);
	
	DAMT0FFF_MT_CMD[6] = crc&0xFF;
	
	DAMT0FFF_MT_CMD[7] = (crc>>8)&0xFF;
	
        FeedDog();
        
        for(int i = 0 ;i < 10 ;i++){
        
         HAL_UART_Transmit(&RS485,DAMT0FFF_MT_CMD,8,500);
         FeedDog();
         if(HAL_UART_Receive(&RS485,DAMT0FFF_MT_CMD,8,500)!=HAL_OK&&i==9){
         
          //error´¦Àí
         
         return ;
         }
         else return ;
        
        }
}

