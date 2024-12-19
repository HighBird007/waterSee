#include "head.h"
#define NO_ACK_REJOIN_THRESHOLD 64

extern uint8_t  			stack_rx_pkt_buf[];
extern uint8_t              stack_rx_pkt_size;
extern LORA_STATUS_T		stack_status;

extern uint8_t  			cmd_tx_pkt_buf[STACK_MAX_PAYLOAD];
extern uint8_t  			cmd_tx_pkt_size;
extern STACK_FRAME_TYPE_T   cmd_tx_pkt_type;


static uint8_t  			s_lora_init_flag;
static uint32_t 			s_tx_counter = 0;
/**数据解析**/	
void AppTaskClassCRxPacket(void)
{
    char      		str_char[50];
	uint8_t   		str_len,i;

	if(stack_status.crc_error_flag)
	{
		return;
	}
	if(stack_status.error_code != NONE_ERROR)
	{
		return;
	}
	
	if(stack_status.rx_ack_flag)
	{
		Print("rx-gateway-ack\r\n",strlen("rx-gateway-ack\r\n"));
		s_tx_counter = 0;
	}

	///接收成功,获取接收数据
	if(stack_status.rx_data_flag)
	{
		s_tx_counter = 0;
		///用户去做数据解析
		Print("\r\nrx-server-data(hex):",strlen("\r\nrx-server-data(hex):"));
		for(i=0;i<stack_rx_pkt_size;i++)
		{
			str_len = sprintf(str_char,"%02X ",stack_rx_pkt_buf[i]);
			Print((uint8_t*)&str_char,str_len);
		}
		Print("\r\n",2);
		
		sys_misc_para.comm_tx_led_on_flag = true;
		//Usart1Tx(stack_rx_pkt_buf,stack_rx_pkt_size);
		StackRxPacket(stack_rx_pkt_buf,stack_rx_pkt_size);
	}
}
/**数据解析**/	
void AppTaskClassARxPacket(void)
{
    char      		str_char[50];
	uint8_t   		str_len,i;

	if(!stack_status.rx_done_flag)
	{
		return;
	}
	
	if(stack_status.error_code != NONE_ERROR)
	{
		return;
	}

	if(stack_status.rx_ack_flag)
	{
		Print("rx-gateway-ack\r\n",strlen("rx-gateway-ack\r\n"));
		s_tx_counter = 0;
	}
	///接收成功,获取接收数据
	if(stack_status.rx_data_flag)
	{
		s_tx_counter = 0;
		Print("\r\nrx-server-data(hex):",strlen("\r\nrx-server-data(hex):"));
		for(i=0;i<stack_rx_pkt_size;i++)
		{
			str_len = sprintf(str_char,"%02X ",stack_rx_pkt_buf[i]);
			Print((uint8_t*)&str_char,str_len);
		}
		Print("\r\n",2);
		
		sys_misc_para.comm_tx_led_on_flag = true;
		
		StackRxPacket(stack_rx_pkt_buf,stack_rx_pkt_size);
	}
}
void LoraTxPkt(uint8_t tx_buf[],uint8_t tx_size)
{
	uint8_t max_tx_len;
	
	STACK_FRAME_TYPE_T  s_frame_type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
	
	if(!stack_context.join_net_flag)
	{
	///	Print("tx pkt fail:mote not join net.\r\n",strlen("tx pkt fail:mote not join net.\r\n"));
	///	return;
	}
	
	max_tx_len = StackGetMaxTxSize();
	
	if(tx_size > max_tx_len)
	{
		Print("tx pkt fail:pkt too len.\r\n",strlen("tx pkt fail:pkt too len.\r\n"));
		return;
	}
	
	s_tx_counter++;
	if((s_tx_counter % 8) == 0)
	{
		s_frame_type = FRAME_TYPE_DATA_CONFIRMED_UP;
	}

	
	StackTxPacket(tx_buf,tx_size,s_frame_type);
	
	AppTaskClassARxPacket();
	
	if(dev_info.class_type == 'C')
	{
		StackIntoClassC();
	}
	
	if(s_tx_counter >= NO_ACK_REJOIN_THRESHOLD)
	{
		s_tx_counter = 0;
		LoraRejoin();
	}
}

void LoraInit(void)
{
	s_lora_init_flag = false;
	s_tx_counter = 0;
}

void LoraRejoin(void)
{
	s_lora_init_flag = false;
	stack_context.join_net_flag = false;
	StackContextWrite();
}

void LoraTask(void)
{
	if(!s_lora_init_flag)
	{
		s_lora_init_flag = StackInit();
		return;
	}
	if(!stack_context.join_net_flag)
	{
		StackJoin();
		return;
	}

	if(dev_info.class_type == 'C')
	{
		if(stack_status.rx_done_flag)
		{
			StackClassCRx();
			AppTaskClassCRxPacket();
			StackIntoClassC();
		}
	}
}