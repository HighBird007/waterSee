
#include "usart.h"
#include "Loop.h"
#include "DMA.h"
//所有收数据 首先需要放到这个数组中
uint8_t dataArr[100];


UART_HandleTypeDef huart1;

uint8_t dataRecFlag = false ;

/* USART1 init function */

#include "usart.h"

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

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
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

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

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}


#include "stdlib.h"   // 包含rand函数
#include "string.h"   // 用于 memcpy 函数



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1) {
        
        
        int curID = dataArr[0] - 10;
        
        uint8_t t[50];
        sprintf(t, "cur ID %d \n", curID);
        Print(t, strlen(t));
        uint8_t ttt[50] = {dataArr[0], 0x03, 0x04};
        /*
        // 生成随机浮动数（范围从1.0到10.0）
        float randvalue = 1.0f + (rand() % 1000) / 10.0f;  // 示例：随机浮动数在 [1.0, 10.0) 范围内
        
        // 将浮动数转换为字节数组
        uint8_t *tempArr = (uint8_t*)&randvalue; // uint8_t指针，指向randvalue的内存地址
        
        uint8_t ttt[50] = {dataArr[0], 0x03, 0x04};
        
        // 将浮动数的字节放入 ttt 数组中
        ttt[6] = tempArr[0];  // 浮动数高字节
        ttt[5] = tempArr[1];
        ttt[4] = tempArr[2];
        ttt[3] = tempArr[3];  // 浮动数低字节
        */
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
        
        // 重新开始接收数据
        HAL_UART_Receive_IT(&huart1, dataArr, screenRequestLength);
    }
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  UNUSED(Size);


}
/* USER CODE END 1 */
