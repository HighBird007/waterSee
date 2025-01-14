#include "screen.h"

uint8_t ttt[50] = {0x00, 0x03, 0x10};  // 初始化数组

void sendDataToScreen() {
    int curID = dataArr[0] - 10;  // 获取当前池塘ID

    ttt[0] = dataArr[0];  // 设置数据的第一个字节（设备地址或其他）

    float dataNum;
    int index = 3;  // 从 ttt[3] 开始填充数据
    uint8_t *tempArr = NULL;

    // 大端对齐
    // 温度
    dataNum = pondSet[curID - 1].tp;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // 大端：高字节先存
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // 低字节最后存

    // 溶解氧
    dataNum = pondSet[curID - 1].o2;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // 大端：高字节先存
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // 低字节最后存

    // pH
    dataNum = pondSet[curID - 1].ph;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // 大端：高字节先存
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // 低字节最后存

    // 浊度
    dataNum = pondSet[curID - 1].zd;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // 大端：高字节先存
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // 低字节最后存

    // 计算 Modbus CRC16 校验
    unsigned short crc = getModbusCRC16(ttt, index);  // index 是已经填充的数据长度
    
    // 将 CRC 校验结果加入到 ttt[index] 和 ttt[index+1]
    ttt[index++] = crc & 0xFF;        // CRC 低字节
    ttt[index++] = (crc >> 8) & 0xFF; // CRC 高字节

    // 发送数据
    HAL_UART_Transmit_DMA(&huart1, ttt, index);  // 发送数据
}
