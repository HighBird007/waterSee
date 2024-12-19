#include "head.h"

static uint32_t GetPage(uint32_t Addr)
{
	uint32_t page = 0;

	if (Addr <= (FLASH_BASE + FLASH_BANK_SIZE))
	{
		page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
	}

	return page;
}
static uint32_t GetBank(uint32_t Addr)
{
	return FLASH_BANK_1;
}
uint8_t McuFlashErase(uint32_t start_addr,uint32_t erase_len)
{
	uint32_t error = 0;
	uint32_t FirstPage = 0, NbOfPages = 0, BankNumber = 0;
	HAL_StatusTypeDef status = HAL_ERROR;
	FLASH_EraseInitTypeDef EraseInitStruct;
	
	FeedDog();
	HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG( FLASH_FLAG_EOP |  FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
    
	FirstPage  = GetPage(start_addr);
	NbOfPages  = GetPage(start_addr+erase_len) - FirstPage;
	if(NbOfPages == 0)
	{
		NbOfPages = 1;
	}
	BankNumber = GetBank(start_addr);
	
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks       = BankNumber;
	EraseInitStruct.Page        = FirstPage;
	EraseInitStruct.NbPages     = NbOfPages;
	
	status = HAL_FLASHEx_Erase(&EraseInitStruct, &error);
	HAL_FLASH_Lock();
	FeedDog();
	if(status == HAL_OK)
	{
		return true;
	}
	return false;
}
uint8_t McuFlashWrite(uint32_t start_addr,uint8_t wr_buf[],uint16_t len)
{
	uint32_t end_addr = start_addr+len;
	
	#pragma pack (8)
	uint8_t  tmp_buf[1024];
	#pragma pack ()
	
	uint64_t data_64,*p_buf = (uint64_t*)tmp_buf;
	HAL_StatusTypeDef status = HAL_ERROR;
	
	if(len > 1024)
	{
		return false;
	}
	
	memcpy(tmp_buf,wr_buf,len);
	
	if(end_addr <= start_addr)
	{
		return false;
	}
	HAL_FLASH_Unlock();
	for(;start_addr<end_addr;)
	{
		data_64 = *p_buf;
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, start_addr, data_64);
		if(status != HAL_OK)
			break;
		start_addr+= 8;
		p_buf     += 1;
		FeedDog();
	}
	HAL_FLASH_Lock();
	if(status == HAL_OK)
	{
		return true;
	}
	return false;
}
void McuFlashRead(uint32_t start_addr,uint8_t rd_buf[],uint16_t len)
{
	uint16_t i;
	uint32_t p_addr;
	
	p_addr = start_addr;
	for(i=0;i<len;i++)
	{
		rd_buf[i] = *(uint8_t*)p_addr;
		p_addr++;
	}
}
void FeedDog(void)
{
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_9);
}
void MX_GPIO_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	__HAL_RCC_GPIOA_CLK_ENABLE( );
	GPIO_InitStructure.Pin 		= GPIO_PIN_1;	
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Mode 	= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure );
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,GPIO_PIN_SET);
	
	__HAL_RCC_GPIOB_CLK_ENABLE( );
	GPIO_InitStructure.Pin 		= GPIO_PIN_8|GPIO_PIN_9;	
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Mode 	= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure );
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
	
	HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_9);
}
