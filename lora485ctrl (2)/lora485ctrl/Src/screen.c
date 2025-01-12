#include "screen.h"


void sendDataToScreen(){

        int curID = dataArr[0] - 10;
        
        uint8_t t[50];
        sprintf(t, "cur ID %d \n", curID);
        Print(t, strlen(t));
        uint8_t ttt[50] = {dataArr[0], 0x03, 0x04};

        float dataNum ;
        switch(dataArr[3]){
        
        case 0: dataNum = pondSet[curID-1].tp;break;
        case 2: dataNum = pondSet[curID-1].o2;break;
        case 4: dataNum = pondSet[curID-1].ph;break;
        case 6: dataNum = pondSet[curID-1].zd;break;
        
        default: break;
        
        }
        uint8_t* tempArr = (uint8_t*)&dataNum;
        ttt[6] = tempArr[0];
        ttt[5] = tempArr[1];
        ttt[4] = tempArr[2];
        ttt[3] = tempArr[3];
        // 计算Modbus CRC16校验
        unsigned short crc = getModbusCRC16(ttt, 7);
        
        // 将 CRC 校验结果加入到 ttt[7] 和 ttt[8]
        ttt[7] = crc & 0xFF;        // CRC低字节
        ttt[8] = (crc >> 8) & 0xFF; // CRC高字节
        
        // 发送数据
        HAL_UART_Transmit(&huart1, ttt, 9, 1000);

}