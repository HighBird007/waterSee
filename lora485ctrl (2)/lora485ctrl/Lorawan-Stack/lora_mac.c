#define LORA_MAC_PARA_GLOBAL

#include "head.h"

#define CN470_STACK

#ifdef  CN470_STACK
	#define DEFAULT_FREQ   								472300000
	#define DEFAULT_RX2_FREQ       						505300000
	char soft_ver[] = {"LD10-485-9600-transfer-cn470-lorawan-sf7-base-xc30-v1.8\r\n"};
#else
	#define DEFAULT_FREQ   								433175000
	#define DEFAULT_RX2_FREQ       						434665000
	char soft_ver[] = {"XC30-485-transfer-lorawan-EU433-v1.6\r\n"};
#endif

uint8_t   			tx_pwr_array[STACK_TX_POWER_STEP] = { 17,16,14,12,10,8,5,2};

uint8_t  			stack_tx_pkt_buf[STACK_MAX_PAYLOAD];
uint8_t  			stack_rx_pkt_buf[STACK_MAX_PAYLOAD];
uint8_t  			stack_rx_pkt_size;

uint8_t  			sx1276_pkt_buf[STACK_MAX_PAYLOAD];
uint8_t 			stack_tx_pkt_size;
uint8_t 			sx1276_pkt_size;

uint8_t  			stack_tx_cmd_buf[STACK_MAX_CMD_LENGTH];
uint8_t  			stack_tx_cmd_size;

uint16_t 			stack_dev_nonce;

uint8_t             is_joining_flag = false;

int16_t             free_chan_threshold = -100;

uint8_t 			*plora_dev_eui;
uint8_t 			*plora_app_eui;
uint8_t 			*plora_app_key;

uint8_t  			duty_cycle_on_flag = false;
uint8_t  			duty_cycle_pl      = 0;
LORA_STATUS_T		stack_status;

static LORA_PARA_T      lora_para;

uint8_t StackGetVersion(char ver[])
{
	memcpy(ver,soft_ver,strlen(soft_ver));
	return strlen(soft_ver);
}
char StackGetClassType(void)
{
	return dev_info.class_type;
}
uint8_t StackGetMaxTxSize(void)
{
	uint8_t sf_payload_array[6] = { 51, 51, 51, 115, 222, 222};
	if(stack_context.tx_dr > DR_5)
	{
		stack_context.tx_dr = DR_5;
	}
	return sf_payload_array[stack_context.tx_dr]-stack_tx_cmd_size;
}
void StackGetState(LORA_STATUS_T *status)
{
	memcpy((uint8_t*)status,(uint8_t*)&stack_status,sizeof(LORA_STATUS_T));
	memset((uint8_t*)&stack_status,0,sizeof(LORA_STATUS_T));
}
uint8_t StackGetJoinNetFlag(void)
{
	return stack_context.join_net_flag;
}
uint8_t StackGetAppData(uint8_t rx_buf[])
{
	memcpy(rx_buf,stack_rx_pkt_buf,stack_rx_pkt_size);
	return stack_rx_pkt_size;
}

uint8_t StackFreqIsInRange(uint32_t freq,uint8_t is_up_flag)
{
	uint8_t i;
	if(is_up_flag)
	{
		for(i=0;i<STACK_MAX_UP_CHAN_NUM;i++)
		{
			if(MOTE_UP_START_FREQ+CHAN_STEP_FREQ*i == freq)
			{
				return true;
			}
		}
	}
	else
	{
		for(i=0;i<STACK_MAX_DW_CHAN_NUM;i++)
		{
			if(GW_DOWN_START_FREQ+CHAN_STEP_FREQ*i == freq)
			{
				return true;
			}
		}
	}
	return false;
}
uint8_t StackGetFreqChan(uint32_t freq,uint8_t is_up_flag)
{
	uint8_t i = 0;
	
	if(is_up_flag)
	{
		for(i=0;i<STACK_MAX_UP_CHAN_NUM;i++)
		{
			if(MOTE_UP_START_FREQ+CHAN_STEP_FREQ*i == freq)
			{
				break;
			}
		}
	}
	else
	{
		for(i=0;i<STACK_MAX_DW_CHAN_NUM;i++)
		{
			if(GW_DOWN_START_FREQ+CHAN_STEP_FREQ*i == freq)
			{
				break;
			}
		}
	}
	return i;
}
uint32_t StackGetChanFreq(uint8_t chan,uint8_t is_up_flag)
{
	uint32_t rx_freq;
	if(is_up_flag)
	{
		rx_freq = MOTE_UP_START_FREQ+CHAN_STEP_FREQ*chan;
	}
	else
	{
		rx_freq = GW_DOWN_START_FREQ+CHAN_STEP_FREQ*chan;
	}
	return rx_freq;
}
void StackTxDoneCallback(void)
{
    stack_tx_cmd_size = 0;
	stack_status.ack_server_flag 	= false;
	stack_status.tx_done_flag 		= true;
}
void StackRxTimeoutCallback(void)
{	
	stack_status.rx_timeout_flag 	= true;
}
void StackRxDoneCallback(void)
{
	stack_status.rx_done_flag  		= true;
}
void StackRxErrorCallback(void)
{
	stack_status.crc_error_flag  		= true;
}
void StackRxLoRaPacket(void)
{
	STACK_HEADER_T 		macHdr;
    STACK_FRAME_CTRL_T 	fctrl;
	uint8_t 			port,frameLen,payload_start_index,header_len = 0;
    uint32_t 			address,mic,mic_rx;
    uint16_t 			current_rx_pkt_index = 0;
    uint16_t 			seq_diff = 0;
    uint32_t 			temp_pkt_index = 0;
    uint8_t 			*nwkSKey,*appSKey;
	char 				str_char[200];
	uint8_t 			str_len;
	
	macHdr.value = sx1276_pkt_buf[header_len++];
	if(macHdr.bits.m_type == FRAME_TYPE_PROPRIETARY)
	{
		MemCpy(stack_rx_pkt_buf,sx1276_pkt_buf+1,sx1276_pkt_size-1);
		stack_rx_pkt_size = sx1276_pkt_size-1;
		stack_status.proprietary_frame_flag = true;
		return;
	}
	
	if((macHdr.bits.m_type != FRAME_TYPE_DATA_CONFIRMED_DOWN)&&
	   (macHdr.bits.m_type != FRAME_TYPE_DATA_UNCONFIRMED_DOWN))
	{
		Print("frame type error.\r\n",strlen("frame type error.\r\n"));
		stack_status.error_code = RX_FRAME_TYPE_ERROR;
		return;
	}
	if((sx1276_pkt_size < 12)||(sx1276_pkt_size > MAX_RX_PAYLOAD))
	{
		Print("frame size error.\r\n",strlen("frame size error.\r\n"));
		stack_status.error_code = RX_LENGTH_ERROR;
		return;
	}
	address  = sx1276_pkt_buf[header_len++];
	address |= sx1276_pkt_buf[header_len++] << 8;
	address |= sx1276_pkt_buf[header_len++] << 16;
	address |= sx1276_pkt_buf[header_len++] << 24;
	if(address == stack_context.stack_dev_addr)
	{
		stack_status.multi_cast_frame_flag = false;
		temp_pkt_index = stack_context.seq.down_fcnt;
		nwkSKey = stack_context.stack_nwk_skey;
		appSKey = stack_context.stack_app_skey;
	}
	else if(address == stack_context.multi.addr)
	{
		stack_status.multi_cast_frame_flag = true;
		temp_pkt_index = stack_context.multi.down_fcnt;
		nwkSKey = stack_context.multi.app_skey;
		appSKey = stack_context.multi.nwk_skey;
	}
	else
	{
		Print("frame addr error.\r\n",strlen("frame addr error."));
		stack_status.error_code = RX_ADDRESS_ERROR;
		return;
	}
	fctrl.value = sx1276_pkt_buf[header_len++];
	current_rx_pkt_index  = ( uint16_t )sx1276_pkt_buf[header_len++];	
	current_rx_pkt_index |= ( uint16_t )sx1276_pkt_buf[header_len++] << 8;
	
	payload_start_index = 8 + fctrl.bits.fopts_len;
	
	mic_rx  =   ( uint32_t )sx1276_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN];
	mic_rx |= ( ( uint32_t )sx1276_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 1] << 8 );
	mic_rx |= ( ( uint32_t )sx1276_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 2] << 16 );
	mic_rx |= ( ( uint32_t )sx1276_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 3] << 24 );
	
	if(stack_status.multi_cast_frame_flag)
	{
		if(current_rx_pkt_index < stack_context.multi.last_rx_fcnt)
		{
			seq_diff = (0x10000 - stack_context.multi.last_rx_fcnt) + current_rx_pkt_index;
		}
		else
		{
			seq_diff = current_rx_pkt_index - stack_context.multi.last_rx_fcnt;
		}
		
		temp_pkt_index += seq_diff;
	}
	else
	{
		if(current_rx_pkt_index < stack_context.seq.last_rx_fcnt)
		{
			seq_diff = (0x10000 - stack_context.seq.last_rx_fcnt) + current_rx_pkt_index;
		}
		else
		{
			seq_diff = current_rx_pkt_index - stack_context.seq.last_rx_fcnt;
		}
		
		temp_pkt_index = current_rx_pkt_index;
		
	}
	LoRaMacComputeMic(  sx1276_pkt_buf, 
						sx1276_pkt_size - LORAMAC_MFR_LEN, 
						nwkSKey, 
						address, 
						DOWN_LINK, 
						temp_pkt_index, 
						&mic );
	if(mic_rx != mic)
	{
		temp_pkt_index = current_rx_pkt_index + 0x10000;
		LoRaMacComputeMic(  sx1276_pkt_buf, 
						sx1276_pkt_size - LORAMAC_MFR_LEN, 
						nwkSKey, 
						address, 
						DOWN_LINK, 
						temp_pkt_index, 
						&mic );
		
		if(mic_rx != mic)
		{		
			Print("frame mic error.\r\n",strlen("frame mic error.\r\n"));
			stack_status.error_code = RX_MIC_CHECK_ERROR;
			return;
		}
	}

	stack_status.fpending_flag = fctrl.bits.fpending;
	if( macHdr.bits.m_type == FRAME_TYPE_DATA_CONFIRMED_DOWN )
		stack_status.ack_server_flag = true;
	else
		stack_status.ack_server_flag = false;
	
	if(stack_status.multi_cast_frame_flag)
	{
		stack_context.multi.down_fcnt 	= temp_pkt_index;
		stack_context.multi.last_rx_fcnt  = current_rx_pkt_index;
		str_len = sprintf(str_char,"rx_multi_down_fcnt = %d\r\n",stack_context.multi.down_fcnt);
		Print((uint8_t*)&str_char,str_len);
	}
	else
	{
		stack_context.seq.down_fcnt 		= temp_pkt_index;
		stack_context.seq.last_rx_fcnt  	= current_rx_pkt_index;
		str_len = sprintf(str_char,"rx_down_fcnt = %d\r\n",stack_context.seq.down_fcnt);
		Print((uint8_t*)&str_char,str_len);
	}
	
	if(fctrl.bits.ack == 1)
	{
		stack_status.rx_ack_flag = true;
	}
	else
	{
		stack_status.rx_ack_flag = false;
	}
	
	stack_context.adr_ack_counter =0;
	
	if(((sx1276_pkt_size - 4) - payload_start_index) > 0)
	{
		port = sx1276_pkt_buf[payload_start_index++];
		frameLen = ( sx1276_pkt_size - 4 ) - payload_start_index;
		if( port == 0 )
		{
			if(fctrl.bits.fopts_len == 0)
			{
				LoRaMacPayloadDecrypt(sx1276_pkt_buf + payload_start_index,frameLen,nwkSKey,address,DOWN_LINK,temp_pkt_index,stack_rx_pkt_buf );
				StackProCmdOrAck(stack_rx_pkt_buf,frameLen);
				stack_status.rx_cmd_flag = true;
				return;
			}
			else
			{
				Print("frame port zero error.\r\n",strlen("frame port zero error.\r\n"));
				return;
			}
		}
		else
		{
			if( fctrl.bits.fopts_len > 0 )
			{
				StackProCmdOrAck(sx1276_pkt_buf+8,fctrl.bits.fopts_len);
				stack_status.rx_cmd_flag = true;
			}
			LoRaMacPayloadDecrypt(sx1276_pkt_buf + payload_start_index,frameLen,appSKey,address,DOWN_LINK,temp_pkt_index,stack_rx_pkt_buf );
			stack_rx_pkt_size = frameLen;

			if(!stack_status.multi_cast_frame_flag)
				stack_status.rx_data_flag = true;
		}
	}
	else
	{
		if(fctrl.bits.fopts_len > 0)
		{
			StackProCmdOrAck(sx1276_pkt_buf+8, payload_start_index-8);
			stack_status.rx_cmd_flag = true;
		}
	}
}
void StackProCmdOrAck(uint8_t *cmd_payload, uint8_t cmd_len)
{
	uint8_t  mac_cmd,tmp_dr,tmp_pwr,tmp_mask_l,tmp_mask_h,ch_mask_cntl,redundancy;
	uint8_t  i,j,dl_setting,rx1_offset,rx2_dr,tmp_delay,tmp_duty_pl,add_ack_flag = false;
    uint8_t  ack_txt[2];
	uint32_t freq;
	
	char str_char[20];
	uint8_t str_len;

	Print("\r\nrx cmd or cmd-ack:",strlen("\r\nrx cmd or cmd-ack:"));
	for(j=0;j<cmd_len;j++)
	{
		str_len = sprintf(str_char,"%02X ",cmd_payload[j]);
		Print((uint8_t*)&str_char,str_len);
	}
	Print("\r\n",2);
		
	while(cmd_len > 0)
	{
		mac_cmd = *cmd_payload++;
		cmd_len--;
		ack_txt[0] = 0x00;
		ack_txt[1] = 0x00;
		switch(mac_cmd)
		{
			case SRV_MAC_LINK_CHECK_ANS:
			{
				if(cmd_len >= 2)
				{
					stack_status.demod_margin = *cmd_payload++;
					stack_status.gw_cnt  = *cmd_payload++;
					cmd_len -= 2;
				}
				else
				{
					cmd_len = 0;
				}
				break;
			}
			case SRV_MAC_LINK_ADR_REQ:
			{
				if(cmd_len < 4)
				{
					cmd_len = 0;
					break;
				}
				cmd_len -= 4;

				tmp_dr  = (*cmd_payload) >> 4;
				tmp_pwr = (*cmd_payload) & 0X0F;
				ack_txt[0] |= 0x06;
					
				cmd_payload++;
				
				tmp_mask_l     	= *cmd_payload++;
				tmp_mask_h     	= *cmd_payload++;
				redundancy   	= *cmd_payload++;
				ch_mask_cntl 	= (redundancy >> 4)&0x07;
				if(ch_mask_cntl <= 6)
				{
					ack_txt[0] |= 0x01;
				}
				if((ack_txt[0] & 0x07) == 0x07)
				{
				//	stack_context.tx_dr  = tmp_dr;
					stack_context.tx_pwr = tmp_pwr;
					if(ch_mask_cntl == 6)
					{
						for(i=0;i<CHAN_GROUP_NUM;i++)
						{
							stack_context.chan_mask_group[i] = 0xff;
						}
					}
					else
					{
						stack_context.chan_mask_group[2*ch_mask_cntl]   = tmp_mask_l;
						stack_context.chan_mask_group[2*ch_mask_cntl+1] = tmp_mask_h;
					}
				}	
				
				if(!add_ack_flag)
				{
					StackAddCmdOrAck(MOTE_MAC_LINK_ADR_ANS,ack_txt,1);
					add_ack_flag = true;
				}
				break;
			}
			case SRV_MAC_DUTY_CYCLE_REQ:
			{
				if(cmd_len >= 1)
				{
					cmd_len -= 1;
					tmp_duty_pl = *cmd_payload++;
					if(tmp_duty_pl == 0)
					{
						duty_cycle_on_flag = false;
						ack_txt[0] = 0x01;
					}
					else
					{
						if(duty_cycle_on_flag)
						{
							duty_cycle_pl = tmp_duty_pl & 0x0f;
							ack_txt[0] = 0x01;
						}
					}
					StackAddCmdOrAck(MOTE_MAC_DUTY_CYCLE_ANS,ack_txt,1);
				}
				break;
			}
			case SRV_MAC_RX_PARAM_SETUP_REQ:	
			{			
				if(cmd_len < 4)
				{
					cmd_len = 0;
					break;
				}
				cmd_len -= 4;
				dl_setting = *cmd_payload++;
				rx1_offset = (dl_setting >> 4) & 0x07;
				rx2_dr = dl_setting & 0x0F;
				if(rx1_offset < 6)
				{
					ack_txt[0] = 0x04;
				}
				if(rx2_dr < 6)
				{
					ack_txt[0] |= 0x02;
				}
				freq  =  *cmd_payload++;
				freq |= (*cmd_payload++) << 8;
				freq |= (*cmd_payload++) << 16;
				freq *= 100;
				if(StackFreqIsInRange(freq,false))
				{
					ack_txt[0] |= 0x01;
				}
				
				if((ack_txt[0] & 0x07) == 0x07)
				{
					stack_context.rx1_offset 	= rx1_offset;
					stack_context.rx2_dr 		= rx2_dr;
					stack_context.rx2_freq 	= freq;
				}
				StackAddCmdOrAck(MOTE_MAC_RX_PARAM_SETUP_ANS,ack_txt,1);
				break;
			}
			case SRV_MAC_DEV_STATUS_REQ:
			{
				break;
			}
			case SRV_MAC_NEW_CHANNEL_REQ:	
			{
				if(cmd_len >= 5)
				{	
					cmd_len     -= 5;
					cmd_payload += 5;
				}
				else
				{
					cmd_len = 0;
				}
				ack_txt[0]   = 0x00;
				StackAddCmdOrAck(MOTE_MAC_NEW_CHANNEL_ANS,ack_txt,1);
				break;
			}
			case SRV_MAC_RX_TIMING_SETUP_REQ:
			{
				if(cmd_len < 1)
				{	
					break;
				}
				
				cmd_len -= 1;
				tmp_delay = *cmd_payload++;
				tmp_delay &= 0x0F;
				if(tmp_delay == 0)
				{
					tmp_delay = 1;
				}
				stack_context.rx1_delay = (uint32_t)(tmp_delay * 1e3)-RADIO_WAKEUP_TIME;
				stack_context.rx2_delay = stack_context.rx1_delay + (uint32_t)(1e3);
				StackAddCmdOrAck(MOTE_MAC_RX_TIMING_SETUP_ANS,NULL,0);
				break;
			}
			case SRV_MAC_DICHANNEL_REQ:
			{
				if(cmd_len < 4)
				{	
					cmd_len = 0;
					break;
				}
				
				cmd_len     -= 4;
				cmd_payload += 4;
				ack_txt[0]   = 0x00;
				StackAddCmdOrAck(MOTE_MAC_DICHANNEL_ANS,ack_txt,1);
				break;
			}
			case SRV_MAC_TX_PARAM_SETUP_REQ:
			{
				///not need ack
				if(cmd_len < 1)
				{	
					cmd_len = 0;
					break;
				}
				
				cmd_len     -= 1;
				cmd_payload += 1;
				break;
			}
			
			default:
				break;
		}
	}
}
uint8_t StackAddCmdOrAck(uint8_t cmd, uint8_t cmd_buf[], uint8_t cmd_len)
{
	uint8_t add_cmd_flag = true;
	
	switch(cmd)
    {
        case MOTE_MAC_LINK_CHECK_REQ:
		case MOTE_MAC_DUTY_CYCLE_ANS:
		case MOTE_MAC_RX_TIMING_SETUP_ANS:
        case MOTE_MAC_LINK_ADR_ANS:
        case MOTE_MAC_RX_PARAM_SETUP_ANS:
		case MOTE_MAC_DEV_STATUS_ANS:
		{
            if((stack_tx_cmd_size + cmd_len +1) < ( STACK_MAX_CMD_LENGTH - 2 ))
            {
                stack_tx_cmd_buf[stack_tx_cmd_size++] = cmd;
                MemCpy(stack_tx_cmd_buf+stack_tx_cmd_size,cmd_buf,cmd_len);
                stack_tx_cmd_size += cmd_len; 
				add_cmd_flag = true;
            }
            break;
		}
        default:
            break;
    }
	return add_cmd_flag;
}
void StackAfterJoinParameterInit(void)
{
    stack_tx_cmd_size 	   				= 0;	
	stack_context.seq.up_fcnt 			= 0;
    stack_context.seq.down_fcnt 		= 0;
	stack_context.seq.last_rx_fcnt  	= 0;
	
	stack_context.multi.down_fcnt      	= 0;
	stack_context.multi.last_rx_fcnt   	= 0; 

	stack_context.tx_pwr                = STACK_MAX_TX_POWER;
	
	stack_context.join_net_flag         = true;
	
	StackContextWrite();
}

void StackRxJoinAccept(void)
{
	STACK_HEADER_T 		macHdr;
	uint8_t 			header_len = 0;
	uint32_t 			mic_rx = 0,mic = 0;
	
	macHdr.value = sx1276_pkt_buf[header_len++];
	if(macHdr.bits.m_type != FRAME_TYPE_JOIN_ACCEPT)
	{
		return;
	}
	LoRaMacJoinDecrypt( sx1276_pkt_buf + 1, 
						sx1276_pkt_size - 1, 
						plora_app_key, 
						stack_rx_pkt_buf + 1 );
	stack_rx_pkt_buf[0] = macHdr.value;
	LoRaMacJoinComputeMic( stack_rx_pkt_buf, 
						   sx1276_pkt_size - LORAMAC_MFR_LEN, 
						   plora_app_key, 
						   &mic );
	mic_rx |=   ( uint32_t )stack_rx_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN];
	mic_rx |= ( ( uint32_t )stack_rx_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 1] << 8 );
	mic_rx |= ( ( uint32_t )stack_rx_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 2] << 16 );
	mic_rx |= ( ( uint32_t )stack_rx_pkt_buf[sx1276_pkt_size - LORAMAC_MFR_LEN + 3] << 24 );
	if( mic_rx != mic )
	{
		return;
	}
	LoRaMacJoinComputeSKeys( plora_app_key, stack_rx_pkt_buf + 1, stack_dev_nonce, stack_context.stack_nwk_skey, stack_context.stack_app_skey );
		
	stack_context.stack_net_id  =   ( uint32_t )stack_rx_pkt_buf[4];
	stack_context.stack_net_id |= ( ( uint32_t )stack_rx_pkt_buf[5] << 8 );
	stack_context.stack_net_id |= ( ( uint32_t )stack_rx_pkt_buf[6] << 16 );
	stack_context.stack_dev_addr  =   ( uint32_t )stack_rx_pkt_buf[7];
	stack_context.stack_dev_addr |= ( ( uint32_t )stack_rx_pkt_buf[8] << 8 );
	stack_context.stack_dev_addr |= ( ( uint32_t )stack_rx_pkt_buf[9] << 16 );
	stack_context.stack_dev_addr |= ( ( uint32_t )stack_rx_pkt_buf[10] << 24 );
	
	stack_context.rx1_offset = ( stack_rx_pkt_buf[11] >> 4 ) & 0x07;
	stack_context.rx2_dr 		     =   stack_rx_pkt_buf[11] & 0x0F;
	
	stack_context.rx1_delay     = ( stack_rx_pkt_buf[12] & 0x0F );
	
	if( stack_context.rx1_delay == 0 )
	{
		stack_context.rx1_delay = 1;
	}
	stack_context.rx1_delay = (stack_context.rx1_delay*1000)-RADIO_WAKEUP_TIME;
	stack_context.rx2_delay = stack_context.rx1_delay + 1000;
	
	StackAfterJoinParameterInit();
}
void StackRxWinParaSet( uint8_t win_index,uint8_t continuous_flag)
{
	uint8_t  datarate = 0;
    uint16_t symb_timeout = 5;     
	uint32_t rx_freq = 0;
	
	if(win_index == 1)
	{
		if(stack_context.tx_dr > stack_context.rx1_offset)
		{
			datarate = stack_context.tx_dr - stack_context.rx1_offset;
		}
		else
		{
			datarate = DR_0;
		}
		rx_freq  = stack_context.rx1_freq;
		
	}
	else if(win_index == 2)
	{
		datarate = stack_context.rx2_dr;
		rx_freq  = stack_context.rx2_freq;
	}
	stack_status.rx_freq = rx_freq;
	stack_status.rx_dr   = datarate;
	
	if(datarate == DR_3)
    {
        symb_timeout = 8;
    }
	else if( datarate == DR_4 )
	{
		 symb_timeout = 8;
	}
    else
    {
        symb_timeout = 10;
    }

	SX126xSetRfFrequency( rx_freq );
	
	RadioSetMaxPayloadLength(STACK_MAX_PAYLOAD);

	RadioSetRxConfig( 	MODEM_LORA, 
						0, 
						12-datarate, 
						1, 
						0, 
						8, 
						symb_timeout, 
						false, 
						0, 
						false, 
						0, 
						0, 
						true,   ///IQ_INVERT
						continuous_flag );///RX CONTINUE FLAG
	
	RadioRx(3000);
}

void StackTxRxFlagInit(void)
{
	stack_status.error_code 		= NONE_ERROR;
	stack_status.tx_done_flag  		= false;
	stack_status.rx_timeout_flag    = false;
	stack_status.rx_ack_flag   		= false;
	stack_status.rx_done_flag  		= false;
	stack_status.rx_data_flag		= false;
	stack_rx_pkt_size               = 0;
}
	
///获取空闲信道
uint8_t StackGetFreeChan(void)
{
	uint8_t  i,j,freq_index,free_flag = false;
	uint8_t  rand_val,rand_array[8] = {0,1,2,3,4,5,6,7};
/**	uint16_t rand_16;
	
	rand_16 = (dev_info.dev_eui[7]*12345 + rand())%5000;
	
	FeedDog();
	while(rand_16 > 0)
	{
		if(rand_16 >= 1000)
		{
			HAL_Delay(1000);
			rand_16 -= 1000;
		}
		else
		{
			HAL_Delay(rand_val);
			rand_16 = 0;
		}
		LocalTask();
		FeedDog();
	}**/
	
	
	for(i=0;i<dev_info.dev_eui[7];i++)
		rand_val = rand();
	
	for(i=0;i<8;i++)
	{
		while(1)
		{
			rand_val = (dev_info.dev_eui[7] + rand()) % 8;
			for(j=0;j<i;j++)
			{
				if(rand_val == rand_array[j])
				{
					break;
				}
			}
			if(j==i)
			{
				rand_array[i] = rand_val;
                break;
			}
		}
	}
	
	for(j=0;j<8;j++)
	{
		stack_context.tx_freq = lora_para.freq + rand_array[j]*CHAN_STEP_FREQ;
		free_flag    = RadioIsChannelFree(MODEM_LORA,stack_context.tx_freq,free_chan_threshold);
		if(free_flag)
		{
			///470异频
			#ifdef  CN470_STACK
				freq_index  = StackGetFreqChan(stack_context.tx_freq,true);
				freq_index  = freq_index % 48;
				stack_context.rx1_freq  = StackGetChanFreq(freq_index,false);
			#else
			///433、470同频
				freq_index = freq_index;
				stack_context.rx1_freq  = stack_context.tx_freq;
			#endif
			return true;
		}
	}

	stack_context.tx_freq = lora_para.freq + rand_array[7]*CHAN_STEP_FREQ;
	RadioIsChannelFree(MODEM_LORA,stack_context.tx_freq,free_chan_threshold);
	freq_index  = StackGetFreqChan(stack_context.tx_freq,true);
	freq_index  = freq_index % 48;
	stack_context.rx1_freq  = StackGetChanFreq(freq_index,false);
	return true;
}

void StackTxOnFreeChan(void)
{
	uint8_t  	free_chan_flag;
	char 	 	str_char[200];
	uint8_t  	str_len;
	
    uint32_t    rx1_delay,rx2_delay,tx_done_ms,pre_ms,cur_ms;
	StackTxRxFlagInit();
	
	free_chan_flag = StackGetFreeChan();
	if(!free_chan_flag)
	{
		///打印信道忙
		Print("tx chan busy,tx fail.\r\n",strlen("tx chan busy,tx fail.\r\n"));
		stack_status.error_code = TX_CHAN_BUSY_ERROR;

		return;
	}

	str_len = sprintf(str_char,"tx_freq  = %d,tx_dr  = %d,tx_pwr = %d,\r\n",
							stack_context.tx_freq,
							stack_context.tx_dr,
							lora_para.pwr);
	Print((uint8_t*)&str_char,str_len);

	str_len = sprintf(str_char,"rx1_freq = %d,rx1_dr = %d,rx2_freq = %d,rx2_dr = %d,wait rx...\r\n",
						stack_context.rx1_freq,
						stack_context.tx_dr,
						stack_context.rx2_freq,
						stack_context.rx2_dr);
	//Print((uint8_t*)&str_char,str_len);

	sys_misc_para.lora_tx_led_on_flag = true;
	
	RadioSend(stack_tx_pkt_buf, stack_tx_pkt_size);
	///第1次等待，等发送完成
	pre_ms = HAL_GetTick();
    //这个循环在发送一次后  无法跳出 print输出的
    /*
  2058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d 42058d d 42058d 42058d 42058d 4都是这个
  注释循环 便无法发送数据
*/
	while(!stack_status.tx_done_flag)
	{
		cur_ms = HAL_GetTick();
		if(cur_ms >= pre_ms + STACK_MAX_TX_WINDOW)
		{
			break;
		}
		FeedDog();
		LocalTask();
		LedTask();
		FeedDog();
	}
        
 
        
	tx_done_ms    = cur_ms;
	RadioSleep();
	if(!stack_status.tx_done_flag)
	{
		stack_status.error_code = TX_FAIL_ERROR;
		return;
	}
	if(is_joining_flag)
	{
		rx1_delay = JOIN_ACCEPT_DELAY-RADIO_WAKEUP_TIME;
		rx2_delay = JOIN_ACCEPT_DELAY+1000;
	}
	else
	{
		rx1_delay = stack_context.rx1_delay;
		rx2_delay = stack_context.rx2_delay;
	}
        
	///RX1
	while(!(cur_ms >= tx_done_ms + rx1_delay))
	{
		cur_ms = HAL_GetTick();
		LocalTask();
		LedTask();
		FeedDog();
	}
        
        
        
	stack_status.rx_slot_num = RX1_INDEX;
	StackRxWinParaSet(1,false);
	pre_ms = HAL_GetTick();
	while((!stack_status.rx_done_flag)&&(!stack_status.rx_timeout_flag))
	{
		cur_ms = HAL_GetTick();
		if(cur_ms >= pre_ms + STACK_MAX_RX_WINDOW)
		{
			break;
		}
		LocalTask();
		LedTask();
		FeedDog();
	}
	if(stack_status.rx_done_flag)
	{
		return;
	}
	
	if((dev_info.class_type == 'A')||(is_joining_flag))
	{
		RadioSleep();
		cur_ms = HAL_GetTick();
		while(!(cur_ms >= tx_done_ms + rx2_delay))
		{
			cur_ms = HAL_GetTick();
			LocalTask();
			LedTask();
			FeedDog();
		}
		
		stack_status.rx_timeout_flag = false;
		StackRxWinParaSet(2,false);
		stack_status.rx_slot_num = RX2_INDEX;	
		pre_ms = HAL_GetTick();
		while((!stack_status.rx_done_flag)&&(!stack_status.rx_timeout_flag))
		{
			cur_ms = HAL_GetTick();
			if(cur_ms >= pre_ms + STACK_MAX_RX_WINDOW)
			{
				break;
			}
			LocalTask();
			LedTask();
			FeedDog();
		}
	}
}
void StackTxPacket(uint8_t tx_buf[],uint8_t tx_size,STACK_FRAME_TYPE_T frame_type)
{
    STACK_HEADER_T 		macHdr;
	STACK_FRAME_CTRL_T 	fctrl;
	uint32_t 			mic;
	uint8_t 			header_len = 0;
	uint8_t 			tmp_buf[STACK_MAX_PAYLOAD];
	char 				str_char[50];
	uint8_t 			str_len;
	uint8_t 			i,re_tx_num,port;
    
	macHdr.value = 0;
	fctrl.value = 0;
	port = lora_para.port;
	
	if(!stack_context.join_net_flag)
	{
	///	stack_status.error_code = NOT_JOIN_STATE;
	///	return;
	}
	
	if(tx_size > StackGetMaxTxSize())
	{
		stack_status.error_code = TX_LENGTH_ERROR;
		return;
	}

	if(tx_size == 0)
    {
		tx_buf = NULL;
    }
	
	if(stack_status.ack_server_flag)
		fctrl.bits.ack = 1;
	
	if(frame_type == FRAME_TYPE_PROPRIETARY)
	{
		macHdr.bits.m_type = FRAME_TYPE_PROPRIETARY;
		stack_tx_pkt_buf[header_len++] = macHdr.value;
		MemCpy(stack_tx_pkt_buf+header_len,tx_buf,tx_size);
		stack_tx_pkt_size = header_len + tx_size;
		re_tx_num = 1;
		goto TX_PKT_LAB;
	}
	
	if(frame_type == FRAME_TYPE_DATA_CONFIRMED_UP)
	{
		macHdr.bits.m_type = FRAME_TYPE_DATA_CONFIRMED_UP;
		re_tx_num = lora_para.retry;

		Print("\r\n\r\nup confirm pkt\r\n",strlen("\r\n\r\nup confirm pkt\r\n"));
	///	Print(tx_buf,tx_size);
	}
	else if(frame_type == FRAME_TYPE_DATA_UNCONFIRMED_UP)
	{
		macHdr.bits.m_type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
		re_tx_num = 1;
		Print("\r\n\r\nup unconfirm pkt",strlen("\r\n\r\nup unconfirm pkt"));
	///	Print(tx_buf,tx_size);
	}
	else
	{
		stack_status.error_code = TX_FRAME_TYPE_ERROR;
		return;
	}
	
	stack_tx_pkt_buf[header_len++] = macHdr.value;
	stack_tx_pkt_buf[header_len++] = ( stack_context.stack_dev_addr ) & 0xFF;
	stack_tx_pkt_buf[header_len++] = ( stack_context.stack_dev_addr >> 8 ) & 0xFF;
	stack_tx_pkt_buf[header_len++] = ( stack_context.stack_dev_addr >> 16 ) & 0xFF;
	stack_tx_pkt_buf[header_len++] = ( stack_context.stack_dev_addr >> 24 ) & 0xFF;
	stack_tx_pkt_buf[header_len++] =   fctrl.value;
	stack_tx_pkt_buf[header_len++] =   stack_context.seq.up_fcnt & 0xFF;
	stack_tx_pkt_buf[header_len++] = ( stack_context.seq.up_fcnt >> 8 ) & 0xFF;
	if((tx_buf != NULL)&&(tx_size != 0x00))
	{
		if((stack_tx_cmd_size != 0)&&(stack_tx_cmd_size <= STACK_MAX_CMD_LENGTH))
		{
			fctrl.bits.fopts_len  = stack_tx_cmd_size;
			stack_tx_pkt_buf[5] = fctrl.value;
			for(i=0;i<stack_tx_cmd_size;i++)
			{
				stack_tx_pkt_buf[header_len++] = stack_tx_cmd_buf[i];
			}
		}
		else
		{
			stack_tx_cmd_size = 0x00;
		}
	}
	else
	{
		if((stack_tx_cmd_size != 0)&&(stack_tx_cmd_size <= STACK_MAX_CMD_LENGTH))
		{
			tx_buf 		= stack_tx_cmd_buf;
			tx_size 	= stack_tx_cmd_size;
			port 		= 0;
		}
	}
	

	
	if((tx_buf != NULL)&&(tx_size != 0x00))
	{
		stack_tx_pkt_buf[header_len++] = port;
		if( port == 0 )
		{
			LoRaMacPayloadEncrypt((uint8_t* )tx_buf, 
											 tx_size, 
											 stack_context.stack_nwk_skey, 
											 stack_context.stack_dev_addr, 
											 UP_LINK, 
											 stack_context.seq.up_fcnt, 
											 tmp_buf);
		}
		else
		{
			LoRaMacPayloadEncrypt((uint8_t* )tx_buf, 
											 tx_size, 
											 stack_context.stack_app_skey, 
											 stack_context.stack_dev_addr, 
											 UP_LINK, 
											 stack_context.seq.up_fcnt, 
											 tmp_buf);
		}
		MemCpy( stack_tx_pkt_buf + header_len, tmp_buf, tx_size );
	}
	stack_tx_pkt_size 						= header_len + tx_size;
	LoRaMacComputeMic(stack_tx_pkt_buf, 
					  stack_tx_pkt_size, 
					  stack_context.stack_nwk_skey, 
					  stack_context.stack_dev_addr, 
					  UP_LINK, 
					  stack_context.seq.up_fcnt, 
					  &mic);
	stack_tx_pkt_buf[stack_tx_pkt_size + 0] =   mic & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size + 1] = ( mic >> 8 ) & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size + 2] = ( mic >> 16 ) & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size + 3] = ( mic >> 24 ) & 0xFF;
	stack_tx_pkt_size += LORAMAC_MFR_LEN;
TX_PKT_LAB:	
	str_len = sprintf(str_char,"tx_up_fcnt = %d\r\n",stack_context.seq.up_fcnt);
	Print((uint8_t*)&str_char,str_len);

    for(i=0;i<re_tx_num;i++)
	{
		stack_status.total_tx_count = re_tx_num;
		stack_status.cur_tx_index   = i+1;
        RadioSetMaxPayloadLength( stack_tx_pkt_size );
        RadioSetTxConfig(MODEM_LORA,
                          lora_para.pwr, 
                          0, 
                          0, 
                          12-stack_context.tx_dr, 
                          1, 
                          8, 
                          false, 
                          true, 
                          0, 
                          0, 
                          false, 
                          STACK_MAX_TX_WINDOW);
	
		StackTxOnFreeChan();
		if(stack_status.rx_done_flag)
		{
			sx1276_pkt_size = SX126xReadRxPkt(sx1276_pkt_buf,(void*)&stack_status);
			if(stack_status.crc_error_flag)
			{
				stack_status.error_code = RX_CRC_FAIL_ERROR;
			}
			else
			{
				StackRxLoRaPacket();
			}
			if(stack_status.rssi > -70)
			{
				stack_context.tx_dr = 5;
			}
			stack_context.join_net_flag = true;
			break;
		}
	} 
	
	if(frame_type != FRAME_TYPE_PROPRIETARY)
	{
		stack_context.seq.up_fcnt++;
	}

	StackContextWrite();
	
	RadioSleep();
}
void StackIntoClassC(void)
{
	StackTxRxFlagInit();
	stack_status.rx_slot_num = RX2_INDEX;
	StackRxWinParaSet(RX2_INDEX,true);
}
void StackClassCRx(void)
{
	sx1276_pkt_size = SX126xReadRxPkt(sx1276_pkt_buf,(void*)&stack_status);
	if(stack_status.crc_error_flag)
	{
		stack_status.error_code = RX_CRC_FAIL_ERROR;
	}
	else
	{
		StackRxLoRaPacket();
	}
}
void StackTxCmd(uint8_t cmd_id,uint8_t cmd_buf[],uint8_t cmd_len)
{
	if(StackAddCmdOrAck(cmd_id,cmd_buf,cmd_len))
	{
		StackTxPacket(NULL,0,FRAME_TYPE_DATA_CONFIRMED_UP);
	}
}

void StackJoin(void)
{
	uint8_t  		header_len = 0;
	uint32_t 		mic = 0;
	STACK_HEADER_T 	macHdr;
///	char 	        str_char[50];
///	uint8_t         i,str_len;
	uint32_t        rand_val;
    uint8_t         i;
	
	for(i=0;i<dev_info.dev_eui[7];i++)
		rand_val = rand();
	
	rand_val = (dev_info.dev_eui[7]*1234567 + rand()) % 30000;
	
	FeedDog();
	while(rand_val > 0)
	{
		if(rand_val >= 1000)
		{
			HAL_Delay(1000);
			rand_val -= 1000;
		}
		else
		{
			HAL_Delay(rand_val);
			rand_val = 0;
		}
		LocalTask();
		FeedDog();
	}
	
	Print("\r\n\r\nmote join start...\r\n",strlen("\r\n\r\nmote join start...\r\n"));
	
	macHdr.value 						 = 0;
	macHdr.bits.m_type  				 = FRAME_TYPE_JOIN_REQ;
    stack_tx_pkt_buf[header_len++] 		 = macHdr.value;
	stack_tx_pkt_size 					 = header_len;
	MemCpyRev( stack_tx_pkt_buf + stack_tx_pkt_size, plora_app_eui, 8 );
	stack_tx_pkt_size 					 += 8;
	MemCpyRev( stack_tx_pkt_buf + stack_tx_pkt_size, plora_dev_eui, 8 );
	stack_tx_pkt_size 					 += 8;
	stack_dev_nonce = RadioRandom();
///	str_len = sprintf(str_char,"dev_nonce = %d;\r\n",stack_dev_nonce);
///	Print((uint8_t*)&str_char,str_len);
	
	stack_tx_pkt_buf[stack_tx_pkt_size++] = stack_dev_nonce & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size++] = ( stack_dev_nonce >> 8 ) & 0xFF;
	LoRaMacJoinComputeMic(stack_tx_pkt_buf,stack_tx_pkt_size & 0xFF,plora_app_key, &mic);
	stack_tx_pkt_buf[stack_tx_pkt_size++] =   mic & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size++] = ( mic >> 8 ) & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size++] = ( mic >> 16) & 0xFF;
	stack_tx_pkt_buf[stack_tx_pkt_size++] = ( mic >> 24) & 0xFF;

	RadioSetMaxPayloadLength(stack_tx_pkt_size);
	RadioSetTxConfig(	MODEM_LORA,
						lora_para.pwr, 
						0, 
						0, 
						12-stack_context.tx_dr, 
						1, 
						8, 
						false, 
						true, 
						0, 
						0, 
						false, 
						STACK_MAX_TX_WINDOW);
						
	stack_context.tx_dr          = stack_context.tx_dr;
	stack_context.rx1_offset     = 0;
	stack_context.rx2_freq       = DEFAULT_RX2_FREQ;
	stack_context.rx2_dr         = DR_0;
	
///	stack_context.rx1_delay 	 = JOIN_ACCEPT_DELAY-RADIO_WAKEUP_TIME;
///	stack_context.rx2_delay      = JOIN_ACCEPT_DELAY+1000;
	is_joining_flag = true;
	StackTxOnFreeChan();
	is_joining_flag = false;
	if(stack_status.rx_done_flag)
	{
		sx1276_pkt_size = SX126xReadRxPkt(sx1276_pkt_buf,(void*)&stack_status);
		StackRxJoinAccept();
		
		if(stack_status.rssi > -70)
		{
			stack_context.tx_dr = 5;
		}
	}
	RadioSleep();
	
	if(stack_context.join_net_flag)
	{
		Print("mote join net ok\r\n",strlen("mote join net ok\r\n"));
		if(dev_info.class_type == 'C')
		{
			StackIntoClassC();
		}
	}
	
}

uint8_t StackContextWrite(void)
{
	uint8_t tmp_len,exe_flag;
	
	tmp_len   = ((uint8_t*)&stack_context.check) - ((uint8_t*)&stack_context.join_net_flag);
	stack_context.check = U8SumCheck((uint8_t*)&stack_context.join_net_flag,tmp_len);
	exe_flag = McuFlashErase(FLASH_LORA_CONTEXT_ADDR,MCU_PAGE_SIZE);
	if(exe_flag)
	{
		exe_flag = McuFlashWrite(FLASH_LORA_CONTEXT_ADDR,(uint8_t*)&stack_context.join_net_flag,sizeof(STACK_CONTEXT_T));
		if(exe_flag)
		{
			return true;
		}
	}
	
	return false;
}
void StackContextRead(void)
{
	uint8_t check_val,tmp_len;
	
    McuFlashRead(FLASH_LORA_CONTEXT_ADDR,(uint8_t*)&stack_context.join_net_flag,sizeof(STACK_CONTEXT_T));
	tmp_len   = ((uint8_t*)&stack_context.check) - ((uint8_t*)&stack_context.join_net_flag);
	check_val = U8SumCheck((uint8_t*)&stack_context.join_net_flag,tmp_len);
	if(check_val != stack_context.check)
	{
		stack_context.join_net_flag = false;
	}
	stack_context.join_net_flag = false;
	stack_context.tx_pwr         = 22;
    stack_context.tx_dr          = DR_5;
}

void StackLoraParaDefault(void)
{						  
	LORA_PARA_T 	para;
	
	para.public_net_flag   = 1;
///	para.adr_flag   = 0;
	para.port       = 2;

	para.retry      = 3;
	para.pwr        = 22;
	para.dr         = DR_5;
	para.freq       = dev_info.freq;
	memcpy(((uint8_t*)&lora_para.public_net_flag),&para.public_net_flag,sizeof(LORA_PARA_T));
	
///	StackLoraParaWrite(&para);
}
void StackDevInfoRead(DEV_INFO_T *para)
{
	uint8_t  default_dev_eui[8]     = {0x8D,0x00,0x00,0x01,0x00,0x00,0x00,0x01};
	
	uint8_t  default_app_eui[8]     = {0x70,0xB3,0xD5,0x7E,0xD0,0x00,0xFC,0x10};
	uint8_t  default_app_key[16]    = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};
									   
	uint8_t  default_app_skey[16]   = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};
	uint8_t  default_nwk_skey[16]   = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};

	uint8_t  default_ff[8]          = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
									  
	McuFlashRead(DEV_INFO_PARA_ADDR,(uint8_t*)&(para->class_type),sizeof(DEV_INFO_T));
	if(memcmp(para->dev_eui,default_ff,8) == 0)
	{
		para->class_type = 'A';
		
		para->otaa_type = 1;
		
		memcpy(para->dev_eui,default_dev_eui,8);
		
		memcpy(para->app_eui,default_app_eui,8);
		memcpy(para->app_key,default_app_key,16);
		
		memcpy(para->app_skey,default_app_skey,16);
		memcpy(para->nwk_skey,default_nwk_skey,16);
		
		para->freq 			= DEFAULT_FREQ;
		para->rx1_delay     = 1000;
		McuFlashErase(DEV_INFO_PARA_ADDR,MCU_PAGE_SIZE);
		McuFlashWrite(DEV_INFO_PARA_ADDR,(uint8_t*)&(para->class_type),sizeof(DEV_INFO_T));
	}
}
void StackDevInfoFactory(void)
{
	uint8_t  default_dev_eui[8]     = {0x8D,0x00,0x00,0x01,0x00,0x00,0x00,0x01};
	
	uint8_t  default_app_eui[8]     = {0x70,0xB3,0xD5,0x7E,0xD0,0x00,0xFC,0x10};
	uint8_t  default_app_key[16]    = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};
									   
	uint8_t  default_app_skey[16]   = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};
	uint8_t  default_nwk_skey[16]   = {0x54,0x66,0x2D,0x27,0xA2,0x59,0x0A,0x44,
							           0x83,0xBD,0xE4,0x79,0x87,0x0E,0x35,0x99};
	
	dev_info.class_type = 'A';
	
	dev_info.otaa_type = 1;
	
	memcpy(dev_info.dev_eui,default_dev_eui,8);
	
	memcpy(dev_info.app_eui,default_app_eui,8);
	memcpy(dev_info.app_key,default_app_key,16);
	
	memcpy(dev_info.app_skey,default_app_skey,16);
	memcpy(dev_info.nwk_skey,default_nwk_skey,16);
	
	dev_info.freq 		   = DEFAULT_FREQ;
	dev_info.rx1_delay     = 1000;
	McuFlashErase(DEV_INFO_PARA_ADDR,MCU_PAGE_SIZE);
	McuFlashWrite(DEV_INFO_PARA_ADDR,(uint8_t*)&(dev_info.class_type),sizeof(DEV_INFO_T));
}
uint8_t StackDevInfoWrite(DEV_INFO_T *para)
{
	uint8_t exe_flag;
	
	exe_flag = McuFlashErase(DEV_INFO_PARA_ADDR,MCU_PAGE_SIZE);
	if(exe_flag)
	{
		exe_flag = McuFlashWrite(DEV_INFO_PARA_ADDR,(uint8_t*)&(para->class_type),sizeof(DEV_INFO_T));
		if(exe_flag)
		{
			StackDevInfoRead(&dev_info);
			return true;
		}
	}
	
	StackInit();
	
	return false;	
}
uint8_t StackLoraParaWrite(LORA_PARA_T *para)
{
	uint8_t tmp_len,exe_flag;
	
	memcpy(((uint8_t*)&lora_para.public_net_flag),para,sizeof(LORA_PARA_T));
	
	tmp_len   = ((uint8_t*)&lora_para.cheack) - ((uint8_t*)&lora_para.public_net_flag);
	lora_para.cheack = U8SumCheck((uint8_t*)&lora_para.public_net_flag,tmp_len);
	exe_flag = McuFlashErase(FLASH_LORA_PARA_ADDR,MCU_PAGE_SIZE);
	if(exe_flag)
	{
		exe_flag = McuFlashWrite(FLASH_LORA_PARA_ADDR,(uint8_t*)&lora_para.public_net_flag,sizeof(LORA_PARA_T));
		if(exe_flag)
		{
			return true;
		}
	}
	
	return false;
}
void StackLoraParaRead(LORA_PARA_T *para)
{
	StackLoraParaDefault();
}

void StackParaInit(void)
{
//	uint16_t tmp_len;

	plora_dev_eui  = dev_info.dev_eui;
	plora_app_eui  = dev_info.app_eui;
	plora_app_key  = dev_info.app_key;
	
//	tmp_len = sizeof(STACK_CONTEXT_T);
//	MemSet(&stack_context.join_net_flag,0,tmp_len);	
	
    stack_context.tx_pwr         = 22;
    stack_context.tx_dr          = DR_5;
}
void StackAbpParameterInit(void)
{
	stack_context.join_net_flag  = true;

	stack_context.stack_dev_addr  = dev_info.dev_eui[4] << 24;
	stack_context.stack_dev_addr += dev_info.dev_eui[5] << 16;
	stack_context.stack_dev_addr += dev_info.dev_eui[6] << 8;
	stack_context.stack_dev_addr += dev_info.dev_eui[7];

	stack_context.tx_pwr         = 22;
	stack_context.tx_dr          = DR_5;
	
	stack_context.rx2_freq       = DEFAULT_RX2_FREQ;
	stack_context.rx2_dr         = DR_0;
	
	lora_para.rx1_delay          = dev_info.rx1_delay;
	
	stack_context.rx1_delay 	 = lora_para.rx1_delay - RADIO_WAKEUP_TIME;
	stack_context.rx2_delay      = stack_context.rx1_delay+1000;
	
	stack_tx_cmd_size 	   				= 0;	
	stack_context.seq.up_fcnt 			= 0;
    stack_context.seq.down_fcnt 		= 0;
	stack_context.seq.last_rx_fcnt  	= 0;
	
//	stack_context.multi.down_fcnt      	= 0;
//	stack_context.multi.last_rx_fcnt   	= 0; 

	memcpy(stack_context.stack_nwk_skey,dev_info.nwk_skey,16);

	memcpy(stack_context.stack_app_skey,dev_info.app_skey,16);
}
void StackParaPrint(void)
{
	char      		str_char[256];
	uint16_t   		str_len,i;
	
	str_len = sprintf(str_char,"CLASS-TYPE:  %c",dev_info.class_type);
	Print((uint8_t*)&str_char,str_len);
	
	if(dev_info.otaa_type == 1)
	{
		Print("\r\nOTAA MODE",strlen("\r\nOTAA MODE"));
	}
	else
	{
		Print("\r\nABP MODE",strlen("\r\nABP MODE"));
	}
	Print("\r\nDEV_EUI:    ",strlen("\r\nDEV_EUI:    "));
	for(i=0;i<8;i++)
	{
		str_len = sprintf(str_char,"%02X ",dev_info.dev_eui[i]);
		Print((uint8_t*)&str_char,str_len);
	}
    if(dev_info.otaa_type == 1)
	{
		Print("\r\nAPP_EUI:    ",strlen("\r\nAPP_EUI:    "));
		for(i=0;i<8;i++)
		{
			str_len = sprintf(str_char,"%02X ",dev_info.app_eui[i]);
			Print((uint8_t*)&str_char,str_len);
		}
		Print("\r\nAPP_KEY:    ",strlen("\r\nAPP_KEY:    "));
		for(i=0;i<16;i++)
		{
			str_len = sprintf(str_char,"%02X ",dev_info.app_key[i]);
			Print((uint8_t*)&str_char,str_len);
		}
	}
	else
	{
		Print("\r\nAPP_SKEY:   ",strlen("\r\nAPP_SKEY:   "));
		for(i=0;i<16;i++)
		{
			str_len = sprintf(str_char,"%02X ",dev_info.app_skey[i]);
			Print((uint8_t*)&str_char,str_len);
		}
		Print("\r\nNWK_SKEY:   ",strlen("\r\nNWK_SKEY:   "));
		for(i=0;i<16;i++)
		{
			str_len = sprintf(str_char,"%02X ",dev_info.nwk_skey[i]);
			Print((uint8_t*)&str_char,str_len);
		}
	}
	Print("\r\nTX_FREQ:    ",strlen("\r\nTX_FREQ:    "));
	for(i=0;i<8;i++)
	{
		str_len = sprintf(str_char,"%d ",lora_para.freq + i*CHAN_STEP_FREQ);
		Print((uint8_t*)&str_char,str_len);
	}
	
	str_len = sprintf(str_char,"\r\nTX_POWER:   %d,  TX_DR:  %d,  CONFIRM-RETRY:  %d\r\n",
							lora_para.pwr,lora_para.dr,lora_para.retry);
	Print((uint8_t*)&str_char,str_len);
}
uint8_t StackInit(void)
{
	uint8_t ret_flag = true;
	
	StackDevInfoRead(&dev_info);
	
    StackLoraParaRead(NULL);
	
	StackParaInit();
	
	StackParaPrint();
	
	ret_flag = SX126xInit();
	if(!ret_flag)
	{
		return false;
	}
	
    RadioSleep();
    
	StackContextRead();

	if(!dev_info.otaa_type)
	{
		StackAbpParameterInit();
		if(dev_info.class_type == 'C')
		{
			StackIntoClassC();
		}
	}
	
	if(stack_context.join_net_flag)
	{
		Print("mote already join net ok\r\n",strlen("mote already join net ok\r\n"));
	}
    return true;
}   