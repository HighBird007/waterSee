#include "head.h"

#define LOCAL_COMM_CMD_DEVINFO_SET    		0X04	
#define LOCAL_COMM_CMD_DEVINFO_GET    		0X05

#define LOCAL_COMM_CMD_FACTORY              0XFF
#define LOCAL_COMM_CMD_REJOIN               0XF0

#define	LOCAL_COMM_LEN_INDEX				0X01
#define	LOCAL_COMM_CMD_INDEX				0X03
#define	LOCAL_COMM_DATA_INDEX				0X04

#define LOCAL_COMM_ACK_DEVINFO_SET    		0X84	
#define LOCAL_COMM_ACK_DEVINFO_GET    		0X85
	
typedef enum
{
	PARA_TYPE_MOTE_CLASS_TYPE=1, ///A，B，C，1字节
	PARA_TYPE_MOTE_OTAA_TYPE, ///OTAA模式，1字节
	PARA_TYPE_MOTE_DEVEUI,      ///16字符
	PARA_TYPE_MOTE_APPEUI,      ///16字符
	PARA_TYPE_MOTE_APPKEY,     ///32字符
	PARA_TYPE_MOTE_APPSKEY,    ///32字符
	PARA_TYPE_MOTE_NWKSKEY,   ///32字符
	
	PARA_TYPE_FREQ,           ///频点，4字节
	PARA_TYPE_RX1_DELAY,      ///频点，2字节
}DEV_INFO_PARA_TYPE;
	
UART_HandleTypeDef         	huart2;
LOCAL_RX_STRUCT             local_para;

void LocalUsartInit(void)
{
	GPIO_InitTypeDef 	GPIO_InitStruct;
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	__HAL_RCC_USART2_CLK_ENABLE();
	
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&huart2);
    
    HAL_NVIC_SetPriority(USART2_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
	__HAL_UART_ENABLE_IT(&huart2,UART_IT_RXNE);	
}
void USART2_IRQHandler(void)
{
	if(__HAL_UART_GET_FLAG((&huart2),USART_ISR_RXNE) != RESET)
	{
		if(local_para.rx_counter >= LOCAL_RX_MAX_FRAME_LEN-1)
		{
			local_para.rx_counter--;
		}
		local_para.rx_buf[local_para.rx_counter++] =  READ_REG((&huart2)->Instance->RDR);
		local_para.rx_timeout_msec = LOCAL_RX_FRAME_TIMEOUT;
	}
	__HAL_UART_CLEAR_FLAG((&huart2),USART_ISR_PE|USART_ISR_FE|USART_ISR_NE|USART_ISR_ORE);
}
void Print(uint8_t data[],uint16_t len)
{
  /*
	uint16_t i;
	
	FeedDog();
	for(i=0;i<len;i++)
	{
		while((__HAL_UART_GET_FLAG((&huart2),UART_FLAG_TXE)?SET:RESET) == RESET);
		(&huart2)->Instance->TDR =data[i];
	}
        */
          HAL_UART_Transmit(&huart2,data,len,500);
	FeedDog();
}
void LocalRxClear(void)
{
	local_para.rx_counter 		= 0;
	local_para.rx_buf[0] 		= 0x00;
	local_para.rx_done_flag 	= false;
	local_para.rx_timeout_msec  = 0;
}

void LocalRxIsDone(void)
{
	if(local_para.rx_timeout_msec > 0)
	{
		local_para.rx_timeout_msec--;
		if(local_para.rx_timeout_msec == 0)
		{
			local_para.rx_done_flag = true;
		}
	}				
}
void MoteDevInfoSet(uint8_t data[],uint16_t len)
{
	uint8_t  res = true;
	uint16_t i=0;
	
	uint8_t  ack_data[256];

	while(i < len)
	{
		switch(data[i++])
		{
			case PARA_TYPE_MOTE_CLASS_TYPE:
			{
				i++;
				dev_info.class_type = data[i++];
				break;
			}
			case PARA_TYPE_MOTE_OTAA_TYPE:
			{
				i++;
				dev_info.otaa_type = data[i++];
				break;
			}
			case PARA_TYPE_MOTE_DEVEUI:
			{
				i++;
				memcpy(dev_info.dev_eui,data+i,8);
				i += 8;
				break;
			}
			case PARA_TYPE_MOTE_APPEUI:
			{
				i++;
				memcpy(dev_info.app_eui,data+i,8);
				i += 8;
				break;
			}
			case PARA_TYPE_MOTE_APPKEY:
			{
				i++;
				memcpy(dev_info.app_key,data+i,16);
				i += 16;
				break;
			}
			case PARA_TYPE_MOTE_APPSKEY:
			{
				i++;
				memcpy(dev_info.app_skey,data+i,16);
				i += 16;
				break;
			}
			case PARA_TYPE_MOTE_NWKSKEY:
			{
				i++;
				memcpy(dev_info.nwk_skey,data+i,16);
				i += 16;
				break;
			}
			case PARA_TYPE_FREQ:
			{
				i++;
				memcpy((uint8_t*)&dev_info.freq,data+i,4);
				i += 4;
				break;
			}
			case PARA_TYPE_RX1_DELAY:
			{
				i++;
				memcpy((uint8_t*)&dev_info.rx1_delay,data+i,2);
				i += 2;
				break;
			}
		}
	}
	
	StackDevInfoWrite(&dev_info);
	
	LoraRejoin();
	
	ack_data[0] = '*';
	ack_data[1] = 7;
	ack_data[2] = 0;
	ack_data[3] = LOCAL_COMM_ACK_DEVINFO_SET;
	ack_data[4] = res;
	ack_data[5] = U8SumCheck(ack_data+1,4);
	ack_data[6] = '#';
	Print(ack_data,7);
}
void MoteDevInfoGet(uint8_t data[],uint16_t len)
{
	uint16_t j=0;
	uint8_t  check_sum,ack_data[256];
	
	j = 0;
	ack_data[j++] = '*';
	ack_data[j++] = 0;
	ack_data[j++] = 0;
	ack_data[j++] = LOCAL_COMM_ACK_DEVINFO_GET;
	
	ack_data[j++] = PARA_TYPE_MOTE_CLASS_TYPE;
	ack_data[j++] = 0x01;
	ack_data[j++] = dev_info.class_type;
	
	ack_data[j++] = PARA_TYPE_MOTE_OTAA_TYPE;
	ack_data[j++] = 0x01;
	ack_data[j++] = dev_info.otaa_type;
	
	ack_data[j++] = PARA_TYPE_MOTE_DEVEUI;
	ack_data[j++] = 0x08;
	memcpy(ack_data+j,dev_info.dev_eui,8);
	j += 8;
	if(dev_info.otaa_type == 0x01)
	{
		ack_data[j++] = PARA_TYPE_MOTE_APPEUI;
		ack_data[j++] = 0x08;
		memcpy(ack_data+j,dev_info.app_eui,8);
		j += 8;
		ack_data[j++] = PARA_TYPE_MOTE_APPKEY;
		ack_data[j++] = 0x10;
		memcpy(ack_data+j,dev_info.app_key,16);
		j += 16;
	}
	else
	{
		ack_data[j++] = PARA_TYPE_MOTE_APPSKEY;
		ack_data[j++] = 0x10;
		memcpy(ack_data+j,dev_info.app_skey,16);
		j += 16;
		ack_data[j++] = PARA_TYPE_MOTE_NWKSKEY;
		ack_data[j++] = 0x10;
		memcpy(ack_data+j,dev_info.nwk_skey,16);
		j += 16;
	}
	
	ack_data[j++] = PARA_TYPE_FREQ;
	ack_data[j++] = 0x04;
	ack_data[j++] = dev_info.freq;
	ack_data[j++] = dev_info.freq >> 8;
	ack_data[j++] = dev_info.freq >> 16;
	ack_data[j++] = dev_info.freq >> 24;
	
	ack_data[j++] = PARA_TYPE_RX1_DELAY;
	ack_data[j++] = 0x02;
	ack_data[j++] = dev_info.rx1_delay;
	ack_data[j++] = dev_info.rx1_delay >> 8;
	
	ack_data[1] = j+2;
	ack_data[2] = (j+2) >> 8;
	check_sum = U8SumCheck(ack_data+1,j-1);
	ack_data[j++] = check_sum;
	ack_data[j++] = '#';
	Print(ack_data,j);
}
void LocalRxProcess(void)
{
	uint8_t  check_val,*rx_data = local_para.rx_buf;
	uint16_t total_len,rx_len;
	
	total_len = local_para.rx_counter;
	if(total_len < 7)
	{
		return;
	}
	if(('*' != rx_data[0]) || ('#' != rx_data[total_len-1])) 
	{
		return;
	}
	
	rx_len = rx_data[LOCAL_COMM_LEN_INDEX];
	rx_len = (rx_data[LOCAL_COMM_LEN_INDEX+1] << 8) + rx_len;

	if(rx_len != total_len)
	{
		return;
	}
			
	check_val = U8SumCheck(rx_data+1,rx_len-3);
	
	if(check_val != rx_data[rx_len-2])
	{
		return;
	}
	///内容长度		
	rx_len = rx_len - 6;		
	if(rx_data[LOCAL_COMM_CMD_INDEX] == LOCAL_COMM_CMD_DEVINFO_SET)
	{
		MoteDevInfoSet(rx_data+LOCAL_COMM_DATA_INDEX,rx_len);
	}
	else if(rx_data[LOCAL_COMM_CMD_INDEX] == LOCAL_COMM_CMD_DEVINFO_GET)
	{
		MoteDevInfoGet(rx_data+LOCAL_COMM_DATA_INDEX,rx_len);
	}
	else if(rx_data[LOCAL_COMM_CMD_INDEX] == LOCAL_COMM_CMD_FACTORY)
	{
		StackDevInfoFactory();
	}
	else if(rx_data[LOCAL_COMM_CMD_INDEX] == LOCAL_COMM_CMD_REJOIN)
	{
		Print("rx rejoin cmd.\r\n",strlen("rx rejoin cmd.\r\n"));
		LoraRejoin();
	}
}
void LocalTask(void)
{
	if(!local_para.rx_done_flag)
	{
		return;
	}
	LocalRxProcess();
	LocalRxClear();
}