#include "head.h"
void NMI_Handler(void)
{
	while (1)
	{
      
	}
}

void HardFault_Handler(void)
{
	while (1)
	{
	}
}

void MemManage_Handler(void)
{
	while (1)
	{
	}
}

void BusFault_Handler(void)
{
	while (1)
	{
	}
}

void UsageFault_Handler(void)
{
	while (1)
	{
	}
}

void DebugMon_Handler(void)
{
	while (1)
	{
		
	}
}

void PVD_IRQHandler(void)
{
	while (1)
	{
		
	}
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    
	Usart1RxIsDone();
	LocalRxIsDone();
	sys_misc_para.ms_counter++;
	if(sys_misc_para.ms_counter >= 0x5265C00)
	{
		while(1);
	}

}


