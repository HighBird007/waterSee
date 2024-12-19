#ifndef __LORA_TASK_H__
	#define __LORA_TASK_H__

	void LoraInit(void);
	void LoraRejoin(void);
	void LoraTxPkt(uint8_t tx_buf[],uint8_t tx_size);
	void LoraTask(void);
#endif