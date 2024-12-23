#include "head.h"

#define MAKEWORD(a, b) ((uint16_t)(((uint8_t)((a)&0xff)) | ((uint16_t)((uint8_t)((b)&0xff))) << 8))//a��λ,b��λ
#define MX_N485         5

static uint8_t 				user_tx_data_buf[100],user_tx_data_size;

uint16_t         datacyc=30;
uint8_t         preheat=0;
uint8_t         regflg=0;
uint8_t         staflg=0;
uint8_t         joinnum=0;


static uint8_t              r485Num=0;
static uint8_t              sensorcmd_buf[MX_N485][8];
static uint8_t              r485rsp_size[MX_N485];
static uint8_t              r485rsp_buf[MX_N485][20];

UART_HandleTypeDef         	huart1;
TRANS_RX_STRUCT             trans_para;

void RxRegSuccRsp(uint8_t data[],uint16_t len);


/*calc modbus crc16*/
uint16_t calc_crc(uint8_t buff[], uint8_t len){
	uint16_t tmp = 0xffff;
	uint16_t res = 0x0000;
    for(int n = 0; n < len; n++){
        tmp = buff[n] ^ tmp;
        for(int i = 0;i < 8;i++){
            if(tmp & 0x01){
                tmp = tmp >> 1;
                tmp = tmp ^ 0xa001;
            }   
            else{
                tmp = tmp >> 1;
            }   
        }   
    }   
    res = tmp >> 8;
    res = res | (tmp << 8); 
	return res;
}

void RelayStaTxPackage(void)
{
	uint8_t i=0, j=0, k=0,check_val;
	char 	str_char[100];
	uint8_t str_len;
	
	FeedDog();
	user_tx_data_buf[i++] = 0xFE;
	user_tx_data_buf[i++] = 0x00;
	user_tx_data_buf[i++] = 0x02;  //ver 2
	user_tx_data_buf[i++] = 0x03;  //

	for(j=0; j<r485Num; j++){
		user_tx_data_buf[i++] = sensorcmd_buf[j][0]; //addr
		user_tx_data_buf[i++] = r485rsp_size[j];  //Rs485(relay) rsp len
		for(k=0;k<r485rsp_size[j];k++) //rsp buff
			user_tx_data_buf[i++] = r485rsp_buf[j][k];

	}
	user_tx_data_buf[1] = i+2;
	
	check_val = U8SumCheck(user_tx_data_buf+1,i-1);
	user_tx_data_buf[i++] = check_val;
	user_tx_data_buf[i++] = 0xFE;

	user_tx_data_size     = i;
        ///FE 12 01 01 01 02 75 00 02 02 FF FF 03 02 FF FF 91 FE	
	Print("\r\ntx report pkt:",strlen("\r\ntx report pkt:"));
	for(j=0;j<user_tx_data_size;j++)
	{
		str_len = sprintf(str_char,"%02X ",user_tx_data_buf[j]);
		Print((uint8_t*)&str_char,str_len);
	}
	Print("\r\n",2);
	
	LoraTxPkt(user_tx_data_buf,user_tx_data_size);
}


void RelayCtlTxPackage(uint8_t rcode, uint8_t ai)
{
	uint8_t i=0,j=0,check_val;
	char 	str_char[100];
	uint8_t str_len;
	
	FeedDog();
	user_tx_data_buf[i++] = 0xFE;
	user_tx_data_buf[i++] = 0x00;
	user_tx_data_buf[i++] = 0x02;
	user_tx_data_buf[i++] = rcode;

	user_tx_data_buf[i++] = sensorcmd_buf[ai][0]; //addr
	user_tx_data_buf[i++] = r485rsp_size[ai];  //Rs485(relay) rsp len
	for(j=0;j<r485rsp_size[ai];j++) //rsp buff
		user_tx_data_buf[i++] = r485rsp_buf[ai][j];
		
	user_tx_data_buf[1] = i+2;
	
	check_val = U8SumCheck(user_tx_data_buf+1,i-1);
	user_tx_data_buf[i++] = check_val;
	user_tx_data_buf[i++] = 0xFE;

	user_tx_data_size     = i;
///FE 12 01 01 01 02 75 00 02 02 FF FF 03 02 FF FF 91 FE	
	Print("\r\ntx ctrl pkt:",strlen("\r\ntx ctrl pkt:"));
	for(j=0;i<user_tx_data_size;i++)
	{
		str_len = sprintf(str_char,"%02X ",user_tx_data_buf[j]);
		Print((uint8_t*)&str_char,str_len);
	}
	Print("\r\n",2);
	
	LoraTxPkt(user_tx_data_buf,user_tx_data_size);
}

void RegisterTxPackage()
{
	uint8_t i = 0,j=0,check_val;
	char 	str_char[100];
	uint8_t str_len;
	
	FeedDog();
	user_tx_data_buf[i++] = 0xFE;
	user_tx_data_buf[i++] = 0x00;  //len
	user_tx_data_buf[i++] = 0x02;  //ver
	user_tx_data_buf[i++] = 0x01;  //reg request
	
	user_tx_data_buf[1] = i+2;
	check_val = U8SumCheck(user_tx_data_buf+1,i-1);
	user_tx_data_buf[i++] = check_val;
	
	user_tx_data_buf[i++] = 0xFE;

	user_tx_data_size     = i;
///FE
	Print("\r\ntx register pkt:",strlen("\r\ntx register pkt:"));
	for(j=0;j<user_tx_data_size;j++)
	{
		str_len = sprintf(str_char,"%02X ",user_tx_data_buf[j]);
		Print((uint8_t*)&str_char,str_len);
	}
	Print("\r\n",2);
	
	LoraTxPkt(user_tx_data_buf,user_tx_data_size);
}


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
    /*
        HAL_NVIC_SetPriority(USART1_IRQn, 4, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);	
        
        */
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

void GetRelayStatus(){
	char			str_char[256];
	uint8_t		str_len,i,j, decount,roadnum;
	uint16_t regsize;

	
	//make 485 cmd
	for(i=0; i<r485Num; i++){
		//send cmd
		Usart1RxClear();
		Usart1Tx(sensorcmd_buf[i],8);
		roadnum = sensorcmd_buf[i][5];


		//wait a sec 
		decount=10;
		while(decount--)
		{
	          HAL_Delay(100);
	          FeedDog();
	          LocalTask();
		}

		//get relay rsp,  fcode==1 is relay
		if(trans_para.rx_done_flag){
			Print("\r\nRecv RS485 Info:",strlen("\r\nRecv RS485 Info:"));
	        for(j=0;j<trans_para.rx_counter;j++)
	        {
	            str_len = sprintf(str_char,"%02X ",trans_para.rx_buf[j]);
	            Print((uint8_t*)&str_char,str_len);
	        }
	        Print("\r\n",2);
			if((sensorcmd_buf[i][1]==1)&&(trans_para.rx_counter==6&&roadnum==4||trans_para.rx_counter==6&&roadnum==8||trans_para.rx_counter==7&&roadnum==16))
			{
				uint16_t clac_crc = 0;
				uint16_t msg_crc = 0;
				   
				//8路或4路继电器
				if(roadnum==8||roadnum==4){
					r485rsp_size[i]=1;
					r485rsp_buf[i][0]= trans_para.rx_buf[3];
					clac_crc = calc_crc(trans_para.rx_buf, 4);
					msg_crc = MAKEWORD(trans_para.rx_buf[4], trans_para.rx_buf[5]);
				}
				//16路继电器
				else{
					r485rsp_size[i]=2;
					r485rsp_buf[i][0]= trans_para.rx_buf[3];
					r485rsp_buf[i][1]= trans_para.rx_buf[4];
					//relay_sta= trans_para.rx_buf[3]<<8 | trans_para.rx_buf[4];
					clac_crc = calc_crc(trans_para.rx_buf, 5);
					msg_crc = MAKEWORD(trans_para.rx_buf[5], trans_para.rx_buf[6]);
				}

				//chk rsp
				str_len = sprintf(str_char,"recv 485 addr=%d, send 485 addr=%d;  calc_crc=%04X, msg_crc=%04X ",trans_para.rx_buf[0], sensorcmd_buf[i][0], clac_crc, msg_crc);
				Print((uint8_t*)&str_char,str_len);
				if(trans_para.rx_buf[0] == sensorcmd_buf[i][0] /*&& clac_crc == msg_crc*/)
				{
					staflg = 1;
					str_len = sprintf(str_char,"relay_sta[%d], size[%d], sta0[%d], sta1[%d]\r\n",
						i,r485rsp_size[i],r485rsp_buf[i][0],r485rsp_buf[i][1]);
					Print((uint8_t*)&str_char,str_len);
				}
			}
			else if(sensorcmd_buf[i][1]!=1){
				regsize = MAKEWORD(sensorcmd_buf[i][5], sensorcmd_buf[i][4]);
				if(trans_para.rx_counter!=3+regsize*2+2){
					str_len = sprintf(str_char,"RS485 Rsp Len:%d != %d.\r\n",trans_para.rx_counter, 3+regsize*2+2);
					Print((uint8_t*)&str_char,str_len);
					return;
				}
				r485rsp_size[i] = regsize*2; 
				Print("\r\nRS485 Sensor Rsp Info:",strlen("\r\nRS485 Sensor Rsp Info:"));
				for(j=0;j<regsize*2;j++){
					r485rsp_buf[i][j]=trans_para.rx_buf[3+j];
					str_len = sprintf(str_char,"%02X ",r485rsp_buf[i][j]);
					Print((uint8_t*)&str_char,str_len);
				}
				Print("\r\n",2);
				staflg = 1;
			}
			
			
		}
		//timeout
		else{
	          staflg = 0;
	          str_len = sprintf(str_char,"relay_sta time out!!!\r\n");
	          Print((uint8_t*)&str_char,str_len);
		}

		FeedDog();
        Usart1RxClear();
	}
}


void SendRelayCtl(uint8_t addr,uint8_t road, uint8_t opt){
	char			str_char[256];
	uint8_t		str_len,i,ai,mask,decount=10;

	//make 485 cmd
    //                         0   1    2    3     4    5   6    7
	uint8_t send_buf[8]   = {0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00};
	send_buf[0] = addr;
    uint8_t rroad_offset = road-1;
	send_buf[3] = rroad_offset;
	if(opt==1)
        send_buf[4] = 0xff;
	else
        send_buf[4] = 0x00;

	uint16_t clac_crc = calc_crc(send_buf, 6);
	send_buf[6] = (clac_crc & 0xff00)>>8;
	send_buf[7] = clac_crc & 0xff;
	//send cmd
	Usart1RxClear();
	Usart1Tx(send_buf,8);
	while(decount--)
	{
          HAL_Delay(100);
          FeedDog();
          LocalTask();
	}

	for(ai=0;ai<r485Num;ai++){
		if(sensorcmd_buf[ai][0]==addr)
			break;
	}

	//wait until get rsp
	if((trans_para.rx_done_flag)&&(trans_para.rx_counter==8)){
          //chk rsp,return
          for(i=0;i<8;i++){
            if(trans_para.rx_buf[i] != send_buf[i]){
              str_len = sprintf(str_char,"ctrl relay send&recv not match!!!\r\n");
              Print((uint8_t*)&str_char,str_len);
              RelayCtlTxPackage(0x13, ai);
              return;
            }
          }
          str_len = sprintf(str_char,"ctrl relay(addr=%d) succeed!\r\n", addr);
          Print((uint8_t*)&str_char,str_len);

		  	
          if(opt == 1){
		  	if((r485rsp_size[ai]==1||r485rsp_size[ai]==2)&&road<=8){
				mask = 1<<rroad_offset;
				r485rsp_buf[ai][0] |= mask;	
		  	}else if(r485rsp_size[ai]==2 && road>8 && road<=16){
				mask = 1<<rroad_offset-8;
				r485rsp_buf[ai][1] |= mask; 
			}
          }else if(opt==0){ 
          	if((r485rsp_size[ai]==1||r485rsp_size[ai]==2)&&road<=8){
				mask &= ~(1<<rroad_offset);
				r485rsp_buf[ai][0] &= mask;	
		  	}else if(r485rsp_size[ai]==2 && road>8 && road<=16){
				mask &= ~(1<<(rroad_offset-8));
				r485rsp_buf[ai][1] &= mask; 
			}
          }
          RelayCtlTxPackage(0x11, ai);
        }
        //timeout
        else{
            str_len = sprintf(str_char,"ctrl relay time out!!!\r\n");
            Print((uint8_t*)&str_char,str_len);
            RelayCtlTxPackage(0x12, ai);
         }
         Usart1RxClear();
}

/**接收到数据包后，进行包分�?*/	
void StackRxPacket(uint8_t data[],uint16_t len)
{
	char			str_char[256];
	uint16_t		str_len;

	if((data[0] == 0xFE)&&(data[len-1] == 0xFE)&&(data[3] == 0x02))
	{
		RxRegSuccRsp(data,len);
	}

	//Ctrl Machine By Relay
    //FE 09 01 E2 01 01 00 00 EF 
	else if((data[0] == 0xFE)&&(data[len-1] == 0xFE)&&(data[3] == 0x10))
	{
		//:FE 09 01 E2 01 03 01 00 EF 
		uint8_t addr = data[4];
		uint8_t road = data[5];
		uint8_t opt = data[6];
		str_len = sprintf(str_char,"recv ctrl from server, addr=%d, road=%d, opt=%d\r\n", addr, road, opt);
		Print((uint8_t*)&str_char,str_len);
		SendRelayCtl(addr, road, opt);
		
		//StackLoraParaWrite(&lora_para); 
	}
	else{
		str_len = sprintf(str_char,"downlink msg delimiter!= 0xfe or type:{%d} unkown, ignore this msg.\r\n",data[3]);
		Print((uint8_t*)&str_char,str_len);
		return;

	}
}


/**接收到数据包后，进行包分�?*/	
void RxRegSuccRsp(uint8_t data[],uint16_t len)
{
	char			str_char[256];
	uint16_t		str_len;

	uint16_t        i,j=0;
	uint8_t         addr, msg_len, cmd_len;


	msg_len = data[1];
	datacyc = MAKEWORD(data[5], data[4]);
	preheat = MAKEWORD(data[7], data[6]);
	
	str_len = sprintf(str_char,"recv register rsp from svr, msg_len=%d, datacyc=%d, preheat=%d .\r\n",msg_len, datacyc, preheat);
	Print((uint8_t*)&str_char,str_len);
	i=8; //数据区起始偏移量
    r485Num = 0;
	//get R485 Cmd from Register Rsp Msg
	while((i<msg_len-2) && r485Num<MX_N485){
		addr = data[i++];
		cmd_len = data[i++];
		/*fcode(1字节)+regstart(2字节)+reglen(1字节)*/
		if(cmd_len!=5){
			str_len = sprintf(str_char,"R485 Cmd len:{%d} != 5, register fail.\r\n",cmd_len);
			Print((uint8_t*)&str_char,str_len);
			return;
		}
		sensorcmd_buf[r485Num][0] = addr;
		for(j=1; j<=cmd_len; j++){
			sensorcmd_buf[r485Num][j] = data[i++];
		}
		uint16_t crc16= calc_crc(sensorcmd_buf[r485Num], 6);
		sensorcmd_buf[r485Num][6] = (uint8_t)(crc16>>8)&0xff;
		sensorcmd_buf[r485Num][7] = (uint8_t)crc16&0xff;
                
        Print("\r\nRx Package, Make RS485 Cmd:",strlen("\r\nRx Package, Make RS485 Cmd:"));
        for(j=0;j<8;j++)
        {
           str_len = sprintf(str_char,"%02X ",sensorcmd_buf[r485Num][j]);
           Print((uint8_t*)&str_char,str_len);
        }
        Print("\r\n",2);
		r485Num++;
	}

	//register succeed
	regflg=1;
	FeedDog();
    Print("register succeed.\r\n",strlen("register succeed.\r\n"));
}


void SensorTask(void)
{  
	uint8_t decount=0;
	static uint32_t pre_ms_counter = 0;
	//如果没有入网 则启动入网流程
	if(!stack_context.join_net_flag)
	{
		decount=joinnum++;
		if(joinnum>10){
			while(decount--)
			{
	          HAL_Delay(1000);
	          FeedDog();
	          LocalTask();
			}
		}
		return;
	}
    
               loop();
        
	if(sys_misc_para.ms_counter < pre_ms_counter + datacyc*1000) {  
		return; 
	}  

	//pre_ms_counter = sys_misc_para.ms_counter;

/*
	if(regflg!=1){
		RegisterTxPackage();
		//return;
	}
	
	GetRelayStatus();
	if(staflg==1)
		RelayStaTxPackage();
       // LoraTxPkt("test",4);
      */

	Usart1RxClear();
}

