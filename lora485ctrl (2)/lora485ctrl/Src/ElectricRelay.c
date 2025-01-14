#include "ElectricRelay.h"

uint8_t DAMT0FFF_MT_CMD[20] = {relayAdr};

void controlDeviceStatus( uint8_t road , uint8_t openOrclose){
	
        DAMT0FFF_MT_CMD[1] = 0x05;
        DAMT0FFF_MT_CMD[2] = 0x00;
        DAMT0FFF_MT_CMD[3] = road;
  
	DAMT0FFF_MT_CMD[4] = openOrclose ;
	
	DAMT0FFF_MT_CMD[5] = 0x00;
	
	uint16_t crc = getModbusCRC16(DAMT0FFF_MT_CMD,6);
	
	DAMT0FFF_MT_CMD[6] = crc&0xFF;
	
	DAMT0FFF_MT_CMD[7] = (crc>>8)&0xFF;
	
        FeedDog();
        
        if(RS485SendCmd(&RS485,DAMT0FFF_MT_CMD,8) == false ){
        
        handingError(relayError);
        
        }
        
}

void controlDeviceAllOFF(){
  
        DAMT0FFF_MT_CMD[1] = 0x0F;
        
        DAMT0FFF_MT_CMD[2] = 0x00;
        DAMT0FFF_MT_CMD[3] = 0x00;
        
	DAMT0FFF_MT_CMD[4] = 0x00;
	DAMT0FFF_MT_CMD[5] = 0x10;
        
        DAMT0FFF_MT_CMD[6] = 0x02;
        
        DAMT0FFF_MT_CMD[7] = 0x00;
        DAMT0FFF_MT_CMD[8] = 0x00;
        
	uint16_t crc = getModbusCRC16(DAMT0FFF_MT_CMD,9);
	
	DAMT0FFF_MT_CMD[9] = crc&0xFF;
	
	DAMT0FFF_MT_CMD[10] = (crc>>8)&0xFF;
	
        FeedDog();
        
        if(RS485SendCmd(&RS485,DAMT0FFF_MT_CMD,11) == false ){
        
        handingError(relayError);
        
        }

}

