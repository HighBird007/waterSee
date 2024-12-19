#include "head.h"

UART_HandleTypeDef         	huart1;
TRANS_RX_STRUCT             trans_para;

void Usart1Init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStruct;
	__HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	__HAL_RCC_USART1_CLK_ENABLE();
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&huart1);
    
    HAL_NVIC_SetPriority(USART1_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);	
}
void USART1_IRQHandler(void)
{
	if(__HAL_UART_GET_FLAG((&huart1),USART_ISR_RXNE) != RESET)
	{
		if(trans_para.rx_counter >= TRANS_RX_MAX_FRAME_LEN-1)
		{
			trans_para.rx_counter--;
		}
		trans_para.rx_buf[trans_para.rx_counter++] =  READ_REG((&huart1)->Instance->RDR);
		trans_para.rx_timeout_msec = TRANS_RX_FRAME_TIMEOUT;
	}
	__HAL_UART_CLEAR_FLAG((&huart1),USART_ISR_PE|USART_ISR_FE|USART_ISR_NE|USART_ISR_ORE);
}
void Usart1Tx(uint8_t data[],uint16_t len)
{
	uint16_t i;
	
	FeedDog();
	for(i=0;i<len;i++)
	{
		while((__HAL_UART_GET_FLAG((&huart1),UART_FLAG_TXE)?SET:RESET) == RESET);
		(&huart1)->Instance->TDR =data[i];
	}
}
void Usart1RxClear(void)
{
	trans_para.rx_counter 		= 0;
	trans_para.rx_buf[0] 		= 0x00;
	trans_para.rx_done_flag 	= false;
	trans_para.rx_timeout_msec  = 0;
}

void Usart1RxIsDone(void)
{
	if(trans_para.rx_timeout_msec > 0)
	{
		trans_para.rx_timeout_msec--;
		if(trans_para.rx_timeout_msec == 0)
		{
			trans_para.rx_done_flag = true;
			sys_misc_para.comm_rx_led_on_flag = true;
		}
	}				
}
void TransRxProcess(void)
{
	if(trans_para.rx_counter > 0)
	{
		LoraTxPkt(trans_para.rx_buf,trans_para.rx_counter);
	}
}

void TransTask(void)
{
	if(!trans_para.rx_done_flag)
	{
		return;
	}
	TransRxProcess();
	Usart1RxClear();
}