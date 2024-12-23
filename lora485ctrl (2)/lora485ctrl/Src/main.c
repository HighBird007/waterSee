#define   MAIN_PARA_GLOBAL
#include "head.h"
#include "Loop.h"
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  /*
  while (1)
  {
  }
*/
  /* USER CODE END Error_Handler_Debug */
}


extern char soft_ver[];
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit;

	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
					  |RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 10;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
					  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
	{
		
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInit.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		
	}

	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
	}

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/8000);
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK_DIV8);
	HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}
void LedTask(void)
{	
	static uint8_t  lora_led_flag = 0,comm_led_flag = 0;
	static uint8_t  lora_led_counter = 0,comm_led_counter = 0;
	static uint32_t pre_ms_counter = 0;
	
	if((sys_misc_para.lora_tx_led_on_flag)||(sys_misc_para.lora_rx_led_on_flag)||
	   (sys_misc_para.comm_tx_led_on_flag)||(sys_misc_para.comm_rx_led_on_flag))
	{
		if(((sys_misc_para.ms_counter % 50) == 0)&&(pre_ms_counter != sys_misc_para.ms_counter))
	    {
			pre_ms_counter = sys_misc_para.ms_counter;
		
			if(sys_misc_para.lora_tx_led_on_flag)
			{
				if(lora_led_flag)
				{
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
					lora_led_flag = false;
					lora_led_counter++;
					if(lora_led_counter > 16)
					{
						lora_led_counter = 0;
						sys_misc_para.lora_tx_led_on_flag = false;
					}
				}
				else
				{
					lora_led_flag = true;
					lora_led_counter++;
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);
				}
			}
			else if(sys_misc_para.lora_rx_led_on_flag)
			{
				if(lora_led_flag)
				{
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
					lora_led_flag = false;
					lora_led_counter++;
					if(lora_led_counter > 3)
					{
						lora_led_counter = 0;
						sys_misc_para.lora_rx_led_on_flag = false;
					}
				}
				else
				{
					lora_led_flag = true;
					lora_led_counter++;
					HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_RESET);
				}
			}
			
			if(sys_misc_para.comm_tx_led_on_flag)
			{
				if(comm_led_flag)
				{
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
					comm_led_flag = false;
					comm_led_counter++;
					if(comm_led_counter > 16)
					{
						comm_led_counter = 0;
						sys_misc_para.comm_tx_led_on_flag = false;
					}
				}
				else
				{
					comm_led_flag = true;
					comm_led_counter++;
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
				}
			}
			else if(sys_misc_para.comm_rx_led_on_flag)
			{
				if(comm_led_flag)
				{
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
					comm_led_flag = false;
					comm_led_counter++;
					if(comm_led_counter > 3)
					{
						comm_led_counter = 0;
						sys_misc_para.comm_rx_led_on_flag = false;
					}
				}
				else
				{
					comm_led_flag = true;
					comm_led_counter++;
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
				}
			}
		}
	}
	else
	{
		if(lora_led_flag)
		{
			lora_led_flag    = false;
			lora_led_counter = 0;
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
		}
		if(comm_led_flag)
		{
			comm_led_flag    = false;
			comm_led_counter = 0;
			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
		}
	}
}

void SysMiscParaInit(void)
{
	sys_misc_para.lora_tx_led_on_flag = false;
	sys_misc_para.lora_rx_led_on_flag = false;
	
	sys_misc_para.comm_tx_led_on_flag = false;
	sys_misc_para.comm_rx_led_on_flag = false;
}

uint8_t ChipIDCheck(void)
{
    uint32_t temp0,temp1,temp2;
	uint32_t decode_0,decode_1,decode_2;
	uint8_t  flash_id[24];
	uint8_t  flash_encode_id[24];


	temp0 = *(__IO uint32_t*)(0x1FFF7590);
    temp1 = *(__IO uint32_t*)(0x1FFF7594);
	temp2 = *(__IO uint32_t*)(0x1FFF7598);

	McuFlashRead(FLASH_MCU_ID_ADDR,flash_id,24);
	
	flash_encode_id[1]  = 0x68 ^ flash_id[1];
	flash_encode_id[2]  = 0xf2 ^ flash_id[2];
	flash_encode_id[4]  = 0xd1 ^ flash_id[4];
	flash_encode_id[5]  = 0x05 ^ flash_id[5];
	decode_0  = flash_encode_id[5] << 24;
	decode_0 += flash_encode_id[4] << 16;
	decode_0 += flash_encode_id[2] << 8;
	decode_0 += flash_encode_id[1];
	
	flash_encode_id[7]  = 0x7a ^ flash_id[7];
	flash_encode_id[8]  = 0xc4 ^ flash_id[8];
	flash_encode_id[10] = 0x5b ^ flash_id[10];
	flash_encode_id[11] = 0xab ^ flash_id[11];
	decode_1  = flash_encode_id[11] << 24;
	decode_1 += flash_encode_id[10] << 16;
	decode_1 += flash_encode_id[8] << 8;
	decode_1 += flash_encode_id[7];
	
	flash_encode_id[13] = 0x15 ^ flash_id[13];
	flash_encode_id[14] = 0x26 ^ flash_id[14];
	flash_encode_id[16] = 0x37 ^ flash_id[16];
	flash_encode_id[17] = 0xfa ^ flash_id[17];
	decode_2  = flash_encode_id[17] << 24;
	decode_2 += flash_encode_id[16] << 16;
	decode_2 += flash_encode_id[14] << 8;
	decode_2 += flash_encode_id[13];
	if((temp0 == decode_0)&&(temp1 == decode_1)&&(temp2 == decode_2))
	{
		return true;
	}

	return false;
}


int main(void)
{
	sys_misc_para.ms_counter            = 0;
	HAL_Init();
	
	SystemClock_Config();

	MX_GPIO_INIT();

	FeedDog();
	
        LocalUsartInit();
        
	Print("\r\nsystem start,...\r\n",strlen("\r\nsystem start...\r\n"));
	Print((uint8_t*)soft_ver,strlen(soft_ver));

        Usart1Init();

	SysMiscParaInit();
	
	FeedDog();
        
         initLoop();
	while(1)
	{     
		LoraTask();
		LocalTask();
                SensorTask();
		LedTask();
		FeedDog();
	}
}
