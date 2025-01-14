
#include "usart.h"
#include "Loop.h"
#include "DMA.h"
//所有收数据 首先需要放到这个数组中
uint8_t dataArr[100];

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
__HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);
  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA1_Channel5;
    hdma_usart1_rx.Init.Request = DMA_REQUEST_2;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Request = DMA_REQUEST_2;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}


#include "stdlib.h"   // 包含rand函数
#include "string.h"   // 用于 memcpy 函数

volatile uint8_t RS485Flag = false ;

uint8_t RS485SendCmd(UART_HandleTypeDef *huart,uint8_t* cmd,uint8_t length){
      
   
   //HAL_UARTEx_ReceiveToIdle_DMA(&RS485,dataArr,100);
   
   RS485Flag = false;
          
   for(int i = 0 ;i < 5 ; i++){
  
   FeedDog();
   
   HAL_UART_Transmit(huart,cmd,length,1000);
   
   FeedDog();
   
   HAL_Delay(500);
   
   FeedDog();
   
   if( RS485Flag == true ){
    
    return true;
   
   }
   
   }
   
   return false;

}

//arr存放数据的数组  length为数组总长度 crc默认在最后两位 low high
uint8_t checkCRC(uint8_t* arr, uint8_t length) {
    // 获取 Modbus CRC16 校验值
    uint16_t crc = getModbusCRC16(arr, length - 2);

    // 通过与运算提取低字节和高字节进行校验
    return ((crc & 0xFF) == arr[length - 2]) && (((crc >> 8) & 0xFF) == arr[length - 1]);
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if(huart == &RS485){

     
    if(checkCRC(dataArr,Size) == false ){
      
    Print("crc error",strlen("crc error"));
       
    HAL_UARTEx_ReceiveToIdle_DMA(&RS485,dataArr,100);
        
    return ;
    
    }
    
    uint8_t addr = dataArr[0];
    
    
    switch(addr){
    
    case relayAdr:
      
    
    RS485Flag = true;
    
    break;
    
    case ZWADDR:
    
    RS485Flag = true;
    
    break;
    
    case ZDADDR:
    
    RS485Flag = true;
    
    break;
    
    default:
      
    
    sendDataToScreen();
    
    break;

    }
    
    HAL_UARTEx_ReceiveToIdle_DMA(&RS485,dataArr,100);
  }

}

extern uint8_t draining ; //当前的排水泵是否开启
extern uint8_t filling  ;//当前的进水泵是否开启
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

      uint8_t dummy;
    // 读取接收数据寄存器（清除溢出错误）
      controlDeviceAllOFF();
      draining = false ; //当前的排水泵是否开启
      filling = false ;//当前的进水泵是否开启
      Print("uart ore \n", strlen("uart ore \n"));
      __HAL_UART_CLEAR_OREFLAG(&huart1);
      HAL_UART_Receive_IT(&huart1,&dummy,1);
      HAL_UARTEx_ReceiveToIdle_DMA(&RS485,dataArr,100);

}
/* USER CODE END 1 */
