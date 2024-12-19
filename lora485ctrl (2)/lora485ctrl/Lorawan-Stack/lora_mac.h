#ifndef __LORAMAC_H__
#define __LORAMAC_H__
	#include <stdbool.h>
    #include <stdint.h>
	
	#ifdef LORA_MAC_PARA_GLOBAL
		#define LORA_MAC_PARA_GLOBAL
	#else
		#define LORA_MAC_PARA_GLOBAL extern
	#endif
	#define MOTE_UP_START_FREQ                          470300000
	#define GW_DOWN_START_FREQ                          500300000
	#define CHAN_STEP_FREQ                              200000
	
	#define STACK_PRIVATE_SYNCWORD                   	0x12
	#define STACK_PUBLIC_SYNCWORD                    	0x34			
	
	#define	STACK_MAX_UP_CHAN_NUM					    96
	#define	STACK_MAX_DW_CHAN_NUM						48
	
	#define STACK_MAX_CMD_LENGTH                 		15
	#define STACK_FRMPAYLOAD_OVERHEAD            		13 	
	#define STACK_MAX_PAYLOAD                  	 		255
		
	#define BEACON_INTERVAL                             128000		
	#define STACK_MAX_RX_WINDOW							3000
	#define STACK_MAX_TX_WINDOW							3000
	
	#define SF_12										12
	#define MAX_FCNT_GAP                                16384		
	
	#define UP_LINK                                     0
	#define DOWN_LINK                                   1
	#define LORAMAC_MFR_LEN                             4

	#define STACK_TX_POWER_STEP                       	8
	#define STACK_MIN_TX_POWER                        	2
	#define STACK_MAX_TX_POWER                        	17

	#define ADR_ACK_LIMIT                               64
	#define ADR_ACK_DELAY                               32

	#define DR_0                                        0  	
	#define DR_1                                        1  	
	#define DR_2                                        2  	
	#define DR_3                                        3  	
	#define DR_4                                        4  	
	#define DR_5                                        5 
	
	#define MAX_RX_PAYLOAD                              235
	
	#define LORA_PORT									88
	///发包之前，判断信道是否空闲，若设定为-90，则-90~-142为空闲
	#define FREE_CHAN_THRESHOLD							-90
	///confirm帧，若收不到服务器端ACK，最大发送次数
	#define MAX_CONFIRM_RETRY_NUM						1
	
	#define CHAN_GROUP_NUM   							12

	#define RX2_DEFAULT_DR 					    		DR_0
	#define JOIN_ACCEPT_DELAY   						5000
	#define RX1_INDEX									1
	#define RX2_INDEX									2

	
	#define INVALID_FREQ                                0XFFFFFFFF
	#define INVALID_CHAN                                0XFF

	

	typedef struct
	{
		uint32_t 	up_fcnt;
		uint32_t	down_fcnt;
		uint32_t	last_rx_fcnt;
	}PKT_SEQ_T;

	typedef union
	{
		uint8_t value;
		struct hdr_bits
		{
			uint8_t major           : 2;
			uint8_t rfu             : 3;
			uint8_t m_type          : 3;
		}bits;
	}STACK_HEADER_T;
	
	typedef union
	{
		uint8_t value;
		struct ctrl_bits
		{
			uint8_t fopts_len        	: 4;
			uint8_t fpending        	: 1;
			uint8_t ack             	: 1;
			uint8_t adr_ack_req       	: 1;
			uint8_t adr             	: 1;
		}bits;
	}STACK_FRAME_CTRL_T;
	
	
	typedef struct
	{
		uint32_t addr;
		uint8_t  app_skey[16];
		uint8_t  nwk_skey[16];
		uint32_t down_fcnt;
		uint32_t last_rx_fcnt;
	}MULTICAST_T;
	
	typedef struct
	{
		uint8_t  	join_net_flag;
		
		char     	class_type;
		
		uint32_t    tx_freq;
		uint8_t  	tx_dr;
		uint8_t  	tx_pwr;
		
		uint32_t    rx1_freq;
		uint16_t 	rx1_delay;
		uint8_t  	rx1_offset;
		
		uint32_t 	rx2_freq;
		uint8_t  	rx2_dr;
		uint16_t 	rx2_delay;
		
		uint16_t 	adr_ack_counter;
		
		PKT_SEQ_T 	seq;
		MULTICAST_T multi;
		
		uint8_t    chan_mask_group[CHAN_GROUP_NUM];
		
		uint8_t  	stack_nwk_skey[16];
		uint8_t  	stack_app_skey[16];
		uint16_t 	stack_dev_nonce;
		uint32_t 	stack_net_id;
		uint32_t 	stack_dev_addr;
		uint8_t     check;
	}STACK_CONTEXT_T;
	
	
	
	typedef enum
	{
		MOTE_MAC_LINK_CHECK_REQ          = 0x02,
		MOTE_MAC_LINK_ADR_ANS            = 0x03,
		MOTE_MAC_DUTY_CYCLE_ANS          = 0x04,
		MOTE_MAC_RX_PARAM_SETUP_ANS      = 0x05,
		MOTE_MAC_DEV_STATUS_ANS          = 0x06,
		MOTE_MAC_NEW_CHANNEL_ANS         = 0x07,
		MOTE_MAC_RX_TIMING_SETUP_ANS     = 0x08,
		MOTE_MAC_TX_PARAM_SETUP_REQ      = 0X09,
		MOTE_MAC_DICHANNEL_ANS           = 0X0A,
		
	}STACK_MOTE_CMD_T;
	
	typedef enum
	{
		SRV_MAC_LINK_CHECK_ANS           = 0x02,
		SRV_MAC_LINK_ADR_REQ             = 0x03,
		SRV_MAC_DUTY_CYCLE_REQ           = 0x04,
		SRV_MAC_RX_PARAM_SETUP_REQ       = 0x05,
		SRV_MAC_DEV_STATUS_REQ           = 0x06,
		SRV_MAC_NEW_CHANNEL_REQ          = 0x07,
		SRV_MAC_RX_TIMING_SETUP_REQ      = 0x08,
		SRV_MAC_TX_PARAM_SETUP_REQ       = 0X09,
		SRV_MAC_DICHANNEL_REQ            = 0X0A,
	}STACK_SRV_CMD_T;
	
	typedef enum
	{
		BAT_LEVEL_EXT_SRC                = 0x00,
		BAT_LEVEL_EMPTY                  = 0x01,
		BAT_LEVEL_FULL                   = 0xFE,
		BAT_LEVEL_NO_MEASURE             = 0xFF,
	}STACK_BAT_LEVEL_T;
	
	typedef enum
	{
		FRAME_TYPE_JOIN_REQ              = 0x00,
		FRAME_TYPE_JOIN_ACCEPT           = 0x01,
		FRAME_TYPE_DATA_UNCONFIRMED_UP   = 0x02,
		FRAME_TYPE_DATA_UNCONFIRMED_DOWN = 0x03,
		FRAME_TYPE_DATA_CONFIRMED_UP     = 0x04,
		FRAME_TYPE_DATA_CONFIRMED_DOWN   = 0x05,
		FRAME_TYPE_RFU                   = 0x06,
		FRAME_TYPE_PROPRIETARY           = 0x07,
	}STACK_FRAME_TYPE_T;
	
	typedef enum
	{
		NONE_ERROR=0,
		RADIO_INIT_ERROR,
		NOT_JOIN_STATE,
		LORA_STACK_BUSY,
		TX_LENGTH_ERROR,
		TX_FRAME_TYPE_ERROR,
		TX_CHAN_BUSY_ERROR,
		TX_DUTY_CYCLE_BUSY_ERROR,
		TX_FAIL_ERROR,
		RX_TIMEOUT_ERROR,
		RX_CRC_FAIL_ERROR,
		RX_LENGTH_ERROR,
		RX_MIC_CHECK_ERROR,
		RX_ADDRESS_ERROR,
		RX_FRAME_TYPE_ERROR,
		RX_FRAME_GAP_ERROR,
	}ERROR_TYPE_T;

	typedef struct
	{
		ERROR_TYPE_T error_code;
		
		uint8_t		 tx_done_flag;	
		
		uint8_t		 rx_done_flag;
		
		uint8_t      rx_timeout_flag;
		
		uint8_t		 crc_error_flag;

		uint8_t 	 rx_ack_flag;
		
		uint8_t 	 rx_data_flag;
		
		uint8_t 	 rx_slot_num;
		
		uint8_t		 multi_cast_frame_flag;
		
		uint8_t		 proprietary_frame_flag;
		
		uint8_t		 ack_server_flag;
		
		uint8_t		 fpending_flag;
		
		int16_t		 rssi;
		int16_t		 snr;
		
		uint8_t 	 rx_cmd_flag;
		uint8_t 	 rx_cmd_id;
		
		uint8_t		 demod_margin;
		uint8_t		 gw_cnt;
		
		uint8_t      total_tx_count;
		uint8_t      cur_tx_index;
		
		uint32_t     rx_freq;
		uint8_t      rx_dr;
	}LORA_STATUS_T;
	
	typedef struct
	{
		char     	class_type;	
		uint8_t     otaa_type;	
		uint8_t 	dev_eui[8];
		uint8_t 	app_eui[8];
		uint8_t 	app_key[16];
		uint8_t  	app_skey[16];
		uint8_t  	nwk_skey[16];
		
		uint32_t    freq;
		uint16_t 	rx1_delay;
	}DEV_INFO_T;
	
	typedef struct
	{
		uint8_t     public_net_flag;
		uint8_t     port;
		uint8_t     retry;
		uint8_t     pwr;
		uint8_t     dr;
		uint16_t 	rx1_delay;
		uint32_t    freq;
		
		uint8_t     cheack;
	}LORA_PARA_T;
	

	LORA_MAC_PARA_GLOBAL STACK_CONTEXT_T 	 stack_context;
	LORA_MAC_PARA_GLOBAL LORA_PARA_T         lora_para;
	LORA_MAC_PARA_GLOBAL DEV_INFO_T 		 dev_info;
	
	uint8_t StackGetMaxTxSize(void);
	
	void 	StackRxDoneCallback(void);
	void 	StackRxTimeoutCallback(void);
	void 	StackTxDoneCallback(void);
	void    StackRxErrorCallback(void);
	uint8_t	StackAddCmdOrAck(uint8_t cmd, uint8_t cmd_buf[], uint8_t cmd_len);
	void 	StackProCmdOrAck(uint8_t *cmd_payload, uint8_t cmd_len);
	
	void 	StackTxRxFlagInit(void);
	void    StackRxWinParaSet( uint8_t win_index,uint8_t rx_continuous);
	
	void 	StackRxPkt(void);
	void    StackIntoClassC(void);
	void    StackClassCRx(void);
	
	void 	StackParaInit(void);
	
	void    StackTxCmd(uint8_t cmd_id,uint8_t cmd_buf[],uint8_t cmd_len);
	void    StackTxPacket(uint8_t tx_buf[],uint8_t tx_size,STACK_FRAME_TYPE_T frame_type);
	
	void 	StackDevInfoRead(DEV_INFO_T *para);
	uint8_t StackDevInfoWrite(DEV_INFO_T *para);

	void    StackSetChannelDefault(void);
	
	uint8_t StackContextWrite(void);
	
	void    StackDevInfoFactory(void);
	uint8_t StackLoraParaWrite(LORA_PARA_T *para);
	void    StackLoraParaRead(LORA_PARA_T *para);
	void    StackParaPrint(void);
	
	void    StackContextSave(void);
	void    StackLoraParaDefault(void);
	void    StackJoin(void);
	uint8_t StackInit(void);
#endif
