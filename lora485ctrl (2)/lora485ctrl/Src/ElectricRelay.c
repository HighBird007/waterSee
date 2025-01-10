#include "ElectricRelay.h"
#include "usart.h"
extern UART_HandleTypeDef RS485;

uint8_t DAMT0FFF_MT_CMD[8] = {relayAdr,0x05,0x00};

uint8_t DAMT0FFF_MT_RECMD[8];

void controlDeviceStatus( uint8_t road , uint8_t openOrclose){
	
        DAMT0FFF_MT_CMD[3] = road;
  
	DAMT0FFF_MT_CMD[4] = openOrclose ;
	
	DAMT0FFF_MT_CMD[5] = 0x00;
	
	uint16_t crc = getModbusCRC16(DAMT0FFF_MT_CMD,6);
	
	DAMT0FFF_MT_CMD[6] = crc&0xFF;
	
	DAMT0FFF_MT_CMD[7] = (crc>>8)&0xFF;
	
        FeedDog();
        
        HAL_UART_Abort_IT(&huart1);
        
        for(int i = 0 ;i < 10 ;i++){
        
         HAL_UART_Transmit(&RS485,DAMT0FFF_MT_CMD,8,500);
         
         FeedDog();
         
         if(HAL_UART_Receive(&RS485,DAMT0FFF_MT_RECMD,8,500)==HAL_OK){
        
                         // 验证 CRC 校验码是否匹配
                if ((DAMT0FFF_MT_RECMD[6] == DAMT0FFF_MT_CMD[6])&&((DAMT0FFF_MT_RECMD[7] == DAMT0FFF_MT_CMD[7]))) {
                
                 // Print("relay success",strlen("relay success"));
                
                HAL_UART_Receive_IT(&huart1,dataArr,screenRequestLength);
                
                return ;
                
                } else {
                 // Print("relay crc err",strlen("relay crc err"));
                continue;
                
                }
         }
         else if(i>=9){
        
           
           handingError(relayError);
           
           HAL_UART_Receive_IT(&huart1,dataArr,screenRequestLength);
           
           return ;
         
         }
        
        }
        
}

