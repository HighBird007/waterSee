
#ifndef __MCU_HW_H__
#define __MCU_HW_H__

	#define MCU_PAGE_SIZE					0x800
	#define MCU_MAX_PAGE					64
	#define MCU_MAX_ADDR			        0X08020000
	
	///最后3页，任何情况下，用户不可用
	#define FLASH_LORA_CONTEXT_ADDR    		(MCU_MAX_ADDR - 4*MCU_PAGE_SIZE)
	#define FLASH_MCU_ID_ADDR    		    (MCU_MAX_ADDR - 3*MCU_PAGE_SIZE)
	#define FLASH_LORA_PARA_ADDR    		(MCU_MAX_ADDR - 2*MCU_PAGE_SIZE)
	#define DEV_INFO_PARA_ADDR    		    (MCU_MAX_ADDR -   MCU_PAGE_SIZE)
	
	uint8_t McuFlashErase(uint32_t start_addr,uint32_t erase_len);
	uint8_t McuFlashWrite(uint32_t start_addr,uint8_t wr_buf[],uint16_t len);
	void    McuFlashRead(uint32_t start_addr,uint8_t rd_buf[],uint16_t len);
	void    FeedDog(void);
	void    MX_GPIO_INIT(void);
#endif
