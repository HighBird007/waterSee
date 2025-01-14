#include "screen.h"

uint8_t ttt[50] = {0x00, 0x03, 0x10};  // ��ʼ������

void sendDataToScreen() {
    int curID = dataArr[0] - 10;  // ��ȡ��ǰ����ID

    ttt[0] = dataArr[0];  // �������ݵĵ�һ���ֽڣ��豸��ַ��������

    float dataNum;
    int index = 3;  // �� ttt[3] ��ʼ�������
    uint8_t *tempArr = NULL;

    // ��˶���
    // �¶�
    dataNum = pondSet[curID - 1].tp;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // ��ˣ����ֽ��ȴ�
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // ���ֽ�����

    // �ܽ���
    dataNum = pondSet[curID - 1].o2;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // ��ˣ����ֽ��ȴ�
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // ���ֽ�����

    // pH
    dataNum = pondSet[curID - 1].ph;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // ��ˣ����ֽ��ȴ�
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // ���ֽ�����

    // �Ƕ�
    dataNum = pondSet[curID - 1].zd;
    tempArr = (uint8_t*)&dataNum;
    ttt[index++] = tempArr[3];  // ��ˣ����ֽ��ȴ�
    ttt[index++] = tempArr[2];
    ttt[index++] = tempArr[1];
    ttt[index++] = tempArr[0];  // ���ֽ�����

    // ���� Modbus CRC16 У��
    unsigned short crc = getModbusCRC16(ttt, index);  // index ���Ѿ��������ݳ���
    
    // �� CRC У�������뵽 ttt[index] �� ttt[index+1]
    ttt[index++] = crc & 0xFF;        // CRC ���ֽ�
    ttt[index++] = (crc >> 8) & 0xFF; // CRC ���ֽ�

    // ��������
    HAL_UART_Transmit_DMA(&huart1, ttt, index);  // ��������
}
