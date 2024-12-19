#include "head.h"

static RadioOperatingModes_t OperatingMode;

static volatile RadioLoRaPacketLengthsMode_t LoRaHeaderType;

volatile uint32_t FrequencyError = 0;

static bool ImageCalibrated = false;


const RadioLoRaBandwidths_t Bandwidths[] = { LORA_BW_125, LORA_BW_250, LORA_BW_500 };
/**                                        SF12    SF11    SF10    SF9    SF8    SF7
static double RadioLoRaSymbTime[3][6] = {{ 32.768, 16.384, 8.192, 4.096, 2.048, 1.024 },  // 125 KHz
                                         { 16.384, 8.192,  4.096, 2.048, 1.024, 0.512 },  // 250 KHz
                                         { 8.192,  4.096,  2.048, 1.024, 0.512, 0.256 }}; // 500 KHz
**/
uint8_t MaxPayloadLength = 0xFF;

uint32_t RxTimeout = 0;

bool RxContinuous = false;

PacketStatus_t RadioPktStatus;
uint8_t RadioRxPayload[255];
uint8_t Radiosize;
int8_t Radiorssi;
int8_t Radiosnr;
void RadioOnDioIrq( void* context );

void RadioOnTxTimeoutIrq( void* context );

void RadioOnRxTimeoutIrq( void* context );


static uint8_t RadioPublicNetworkFlag = true;

SX126x_t SX126x;

SPI_HandleTypeDef 			hspi;

void MX_MCU_SPI_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOA_CLK_ENABLE( );
	GPIO_InitStructure.Pin 			= GPIO_PIN_4;
	GPIO_InitStructure.Pull 		= GPIO_PULLUP;
	GPIO_InitStructure.Mode 		= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed 		= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure );
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
	
	GPIO_InitStructure.Pin 			= GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStructure.Mode 		= GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull 		= GPIO_NOPULL;
    GPIO_InitStructure.Speed 		= GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStructure.Alternate 	= GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	__HAL_RCC_SPI1_CLK_ENABLE();
	hspi.Instance 					= SPI1;
	hspi.Init.Mode 					= SPI_MODE_MASTER;
	hspi.Init.Direction 			= SPI_DIRECTION_2LINES;
	hspi.Init.DataSize 				= SPI_DATASIZE_8BIT;
	hspi.Init.CLKPolarity 			= SPI_POLARITY_LOW;
	hspi.Init.CLKPhase 				= SPI_PHASE_1EDGE;
	hspi.Init.NSS 					= SPI_NSS_SOFT;
	hspi.Init.BaudRatePrescaler  	= SPI_BAUDRATEPRESCALER_4;
	hspi.Init.FirstBit 				= SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode 				= SPI_TIMODE_DISABLE;
	hspi.Init.CRCCalculation 		= SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial 		= 7;

	HAL_SPI_Init(&hspi);
    __HAL_SPI_ENABLE(&hspi);
}
void MX_MCU_SPI_DEINIT(void)
{
	HAL_SPI_DeInit(&hspi);
    __HAL_RCC_SPI1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
} 
/**SX127X驱动代码开始**/
void MX_SX126X_GPIO_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	 
	__HAL_RCC_GPIOB_CLK_ENABLE( );
	GPIO_InitStructure.Pin  		= GPIO_PIN_0;	///DIO0
    GPIO_InitStructure.Pull 		= GPIO_NOPULL;
	GPIO_InitStructure.Mode 		= GPIO_MODE_IT_RISING;
	GPIO_InitStructure.Speed 		= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure );
	
	HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	///ANT
	GPIO_InitStructure.Pin 		= GPIO_PIN_1;	///DIO1
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Mode 	= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure );
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
	
	///DIO8,busy检测
	__HAL_RCC_GPIOA_CLK_ENABLE( );
	GPIO_InitStructure.Pin 		= GPIO_PIN_8;	
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Mode 	= GPIO_MODE_INPUT;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure );
	
	
}
void MX_SX126X_RESET( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE( );
	GPIO_InitStructure.Pin 		= GPIO_PIN_13;	
	GPIO_InitStructure.Pull 	= GPIO_PULLUP;
	GPIO_InitStructure.Mode 	= GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed 	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure );
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
	HAL_Delay( 10 );
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_Delay( 20 );
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
    HAL_Delay( 10);
}
uint8_t MX_SX126X_SPI_WRITE_READ(uint8_t data_in)
{
    uint8_t data_out;
	
	IRQ_DISABLE();
    while((SPI1->SR & SPI_FLAG_TXE ) == RESET );
    *(__IO uint8_t *)&SPI1->DR = data_in;
	while((SPI1->SR & SPI_FLAG_RXNE ) == RESET );
    data_out = *(__IO uint8_t *)& SPI1->DR;
	IRQ_ENABLE();
	
    return( data_out );
}
void MX_SX126X_CS_LOW(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
}
void MX_SX126X_CS_HIGH(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
}

uint8_t SX126xInit( void )
{
	uint8_t default_val;
	
	MX_MCU_SPI_DEINIT();
	
	MX_MCU_SPI_INIT();
	
	MX_SX126X_GPIO_INIT();
	
    MX_SX126X_RESET();
	
    SX126xWakeup( );
	
    SX126xSetStandby( STDBY_RC );

	SX126xSetRegulatorMode( USE_DCDC );

    SX126xSetDio2AsRfSwitchCtrl( true );
    SX126xSetOperatingMode( MODE_STDBY_RC );
	
    SX126xSetStandby( STDBY_RC );

    SX126xSetBufferBaseAddress( 0x00, 0x00 );
    SX126xSetTxParams( 0, RADIO_RAMP_200_US );
	///irqMask dio1Mask dio2Mask dio3Mask
    SX126xSetDioIrqParams( IRQ_RADIO_ALL, IRQ_RADIO_ALL, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
	
	default_val = SX126xReadRegister( REG_OCP);
	if(default_val == 0x38)
	{
		Print("\r\nsx126x init ok...\r\n",strlen("\r\nsx126x init ok...\r\n"));
        return true;
	}
	else
	{
		Print("\r\nsx126x init fail...\r\n",strlen("\r\nsx126x init fail...\r\n"));
        return false;
	}
}
/**延时函数**/
void SX126xDelayMs(uint8_t ms)
{
	uint16_t i,j,k;
	for(k=0;k<ms;k++)
	{
		for(i=0;i<200;i++)
		{
			for(j=0;j<200;j++);
		}
	}
}
RadioState_t RadioGetStatus( void )
{
    switch( SX126xGetOperatingMode( ) )
    {
        case MODE_TX:
            return RF_TX_RUNNING;
        case MODE_RX:
            return RF_RX_RUNNING;
        case MODE_CAD:
            return RF_CAD;
        default:
            return RF_IDLE;
    }
}

void RadioSetModem( RadioModems_t modem )
{
	SX126xSetPacketType( PACKET_TYPE_LORA );
	RadioSetPublicNetwork( RadioPublicNetworkFlag );
}

bool RadioIsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh )
{
    bool status = true;
    int16_t rssi = 0;

    RadioSleep( );

    RadioSetModem( modem );

    SX126xSetRfFrequency( freq );

    RadioRx( 0 );

    SX126xDelayMs( 1 );

	rssi = RadioRssi( modem );

	if( rssi > rssiThresh )
	{
		status = false;
	}

    RadioSleep( );
    return status;
}

uint32_t RadioRandom( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    RadioSetModem( MODEM_LORA );

    SX126xSetRx( 0 );

    for( i = 0; i < 32; i++ )
    {
        SX126xDelayMs( 1 );
        rnd |= ( ( uint32_t )SX126xGetRssiInst( ) & 0x01 ) << i;
    }

    RadioSleep( );

    return rnd;
}

void RadioSetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    RxContinuous = rxContinuous;
    if( rxContinuous == true )
    {
        symbTimeout = 0;
    }
    if( fixLen == true )
    {
        MaxPayloadLength = payloadLen;
    }
    else
    {
        MaxPayloadLength = 0xFF;
    }

	SX126xSetStopRxTimerOnPreambleDetect( false );
	SX126xSetLoRaSymbNumTimeout( symbTimeout );
	SX126x.ModulationParams.PacketType = PACKET_TYPE_LORA;
	SX126x.ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t )datarate;
	SX126x.ModulationParams.Params.LoRa.Bandwidth = Bandwidths[bandwidth];
	SX126x.ModulationParams.Params.LoRa.CodingRate = ( RadioLoRaCodingRates_t )coderate;

	if( ( ( bandwidth == 0 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
	( ( bandwidth == 1 ) && ( datarate == 12 ) ) )
	{
		SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x01;
	}
	else
	{
		SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x00;
	}

	SX126x.PacketParams.PacketType = PACKET_TYPE_LORA;

	if( ( SX126x.ModulationParams.Params.LoRa.SpreadingFactor == LORA_SF5 ) ||
		( SX126x.ModulationParams.Params.LoRa.SpreadingFactor == LORA_SF6 ) )
	{
		if( preambleLen < 12 )
		{
			SX126x.PacketParams.Params.LoRa.PreambleLength = 12;
		}
		else
		{
			SX126x.PacketParams.Params.LoRa.PreambleLength = preambleLen;
		}
	}
	else
	{
		SX126x.PacketParams.Params.LoRa.PreambleLength = preambleLen;
	}

	SX126x.PacketParams.Params.LoRa.HeaderType = ( RadioLoRaPacketLengthsMode_t )fixLen;

	SX126x.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength;
	SX126x.PacketParams.Params.LoRa.CrcMode = ( RadioLoRaCrcModes_t )crcOn;
	SX126x.PacketParams.Params.LoRa.InvertIQ = ( RadioLoRaIQModes_t )iqInverted;

	RadioSetModem( ( SX126x.ModulationParams.PacketType == PACKET_TYPE_GFSK ) ? MODEM_FSK : MODEM_LORA );
	SX126xSetModulationParams( &SX126x.ModulationParams );
	SX126xSetPacketParams( &SX126x.PacketParams );

	/// WORKAROUND - Optimizing the Inverted IQ Operation, see DS_SX1261-2_V1.2 datasheet chapter 15.4
	if( SX126x.PacketParams.Params.LoRa.InvertIQ == LORA_IQ_INVERTED )
	{
		/// RegIqPolaritySetup = @address 0x0736
		SX126xWriteRegister( 0x0736, SX126xReadRegister( 0x0736 ) & ~( 1 << 2 ) );
	}
	else
	{
		/// RegIqPolaritySetup @address 0x0736
		SX126xWriteRegister( 0x0736, SX126xReadRegister( 0x0736 ) | ( 1 << 2 ) );
	}
	/// WORKAROUND END

	/// Timeout Max, Timeout handled directly in SetRx function
	RxTimeout = 0xFFFF;
}

void RadioSetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{

	SX126x.ModulationParams.PacketType = PACKET_TYPE_LORA;
	SX126x.ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t ) datarate;
	SX126x.ModulationParams.Params.LoRa.Bandwidth =  Bandwidths[bandwidth];
	SX126x.ModulationParams.Params.LoRa.CodingRate= ( RadioLoRaCodingRates_t )coderate;

	if( ( ( bandwidth == 0 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
	( ( bandwidth == 1 ) && ( datarate == 12 ) ) )
	{
		SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x01;
	}
	else
	{
		SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x00;
	}

	SX126x.PacketParams.PacketType = PACKET_TYPE_LORA;

	if( ( SX126x.ModulationParams.Params.LoRa.SpreadingFactor == LORA_SF5 ) ||
		( SX126x.ModulationParams.Params.LoRa.SpreadingFactor == LORA_SF6 ) )
	{
		if( preambleLen < 12 )
		{
			SX126x.PacketParams.Params.LoRa.PreambleLength = 12;
		}
		else
		{
			SX126x.PacketParams.Params.LoRa.PreambleLength = preambleLen;
		}
	}
	else
	{
		SX126x.PacketParams.Params.LoRa.PreambleLength = preambleLen;
	}

	SX126x.PacketParams.Params.LoRa.HeaderType = ( RadioLoRaPacketLengthsMode_t )fixLen;
	SX126x.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength;
	SX126x.PacketParams.Params.LoRa.CrcMode = ( RadioLoRaCrcModes_t )crcOn;
	SX126x.PacketParams.Params.LoRa.InvertIQ = ( RadioLoRaIQModes_t )iqInverted;

	RadioStandby( );
	RadioSetModem( ( SX126x.ModulationParams.PacketType == PACKET_TYPE_GFSK ) ? MODEM_FSK : MODEM_LORA );
	SX126xSetModulationParams( &SX126x.ModulationParams );
	SX126xSetPacketParams( &SX126x.PacketParams );

    /// WORKAROUND - Modulation Quality with 500 kHz LoRa?Bandwidth, see DS_SX1261-2_V1.2 datasheet chapter 15.1
    if( ( modem == MODEM_LORA ) && ( SX126x.ModulationParams.Params.LoRa.Bandwidth == LORA_BW_500 ) )
    {
        /// RegTxModulation = @address 0x0889
        SX126xWriteRegister( 0x0889, SX126xReadRegister( 0x0889 ) & ~( 1 << 2 ) );
    }
    else
    {
        /// RegTxModulation = @address 0x0889
        SX126xWriteRegister( 0x0889, SX126xReadRegister( 0x0889 ) | ( 1 << 2 ) );
    }
    /// WORKAROUND END

    SX126xSetRfTxPower( power );
}

bool RadioCheckRfFrequency( uint32_t frequency )
{
    return true;
}

uint32_t RadioTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;
/**
	double ts = RadioLoRaSymbTime[SX126x.ModulationParams.Params.LoRa.Bandwidth - 4][12 - SX126x.ModulationParams.Params.LoRa.SpreadingFactor];
	// time of preamble
	double tPreamble = ( SX126x.PacketParams.Params.LoRa.PreambleLength + 4.25 ) * ts;
	// Symbol length of payload and time
	double tmp = ceil( ( 8 * pktLen - 4 * SX126x.ModulationParams.Params.LoRa.SpreadingFactor +
						 28 + 16 * SX126x.PacketParams.Params.LoRa.CrcMode -
						 ( ( SX126x.PacketParams.Params.LoRa.HeaderType == LORA_PACKET_FIXED_LENGTH ) ? 20 : 0 ) ) /
						 ( double )( 4 * ( SX126x.ModulationParams.Params.LoRa.SpreadingFactor -
						 ( ( SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
						 ( ( SX126x.ModulationParams.Params.LoRa.CodingRate % 4 ) + 4 );
	double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
	double tPayload = nPayload * ts;
	// Time on air
	double tOnAir = tPreamble + tPayload;
	// return milli seconds
	airTime = floor( tOnAir + 0.999 );
**/
    return airTime;
}

void RadioSend( uint8_t *buffer, uint8_t size )
{
    SX126xSetDioIrqParams( IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );


    SX126x.PacketParams.Params.LoRa.PayloadLength = size;

    SX126xSetPacketParams( &SX126x.PacketParams );

    SX126xSendPayload( buffer, size, 0 );
}

void RadioSleep( void )
{
    SleepParams_t params = { 0 };

    params.Fields.WarmStart = 1;
    SX126xSetSleep( params );

    SX126xDelayMs( 2 );
}

void RadioStandby( void )
{
    SX126xSetStandby( STDBY_RC );
}

void RadioRx( uint32_t timeout )
{
    SX126xSetDioIrqParams( IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );

    if( RxContinuous == true )
    {
        SX126xSetRx( 0xFFFFFF ); 
    }
    else
    {
        SX126xSetRx( RxTimeout << 6 );
    }
}

void RadioRxBoosted( uint32_t timeout )
{
    SX126xSetDioIrqParams( IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );

    if( RxContinuous == true )
    {
        SX126xSetRxBoosted( 0xFFFFFF ); 
    }
    else
    {
        SX126xSetRxBoosted( RxTimeout << 6 );
    }
}

void RadioStartCad( void )
{
    SX126xSetCad( );
}

int16_t RadioRssi( RadioModems_t modem )
{
    return SX126xGetRssiInst( );
}

void RadioSetMaxPayloadLength(uint8_t max )
{
	SX126x.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength = max;
	SX126xSetPacketParams( &SX126x.PacketParams );
}

void RadioSetPublicNetwork( uint8_t public_flag )
{
	RadioPublicNetworkFlag = public_flag;
	
    if(RadioPublicNetworkFlag)
    {
        SX126xWriteRegister( REG_LR_SYNCWORD, ( LORA_MAC_PUBLIC_SYNCWORD >> 8 ) & 0xFF );
        SX126xWriteRegister( REG_LR_SYNCWORD + 1, LORA_MAC_PUBLIC_SYNCWORD & 0xFF );
    }
    else
    {
        SX126xWriteRegister( REG_LR_SYNCWORD, ( LORA_MAC_PRIVATE_SYNCWORD >> 8 ) & 0xFF );
        SX126xWriteRegister( REG_LR_SYNCWORD + 1, LORA_MAC_PRIVATE_SYNCWORD & 0xFF );
    }
}

uint32_t RadioGetWakeupTime( void )
{
    return SX126xGetBoardTcxoWakeupTime( ) + RADIO_WAKEUP_TIME;
}

uint8_t SX126xReadRxPkt(uint8_t rx_buf[],void *stack_status)
{
	char 	str_char[100];
	uint8_t i,str_len;
	
	sys_misc_para.lora_rx_led_on_flag = true;
	
	memcpy(rx_buf,RadioRxPayload, Radiosize);
	((LORA_STATUS_T*)stack_status)->rssi = Radiorssi;
	((LORA_STATUS_T*)stack_status)->snr  = Radiosnr;
	
	Print("sx126X rx fifo:",strlen("sx126X rx fifo:"));
	for(i=0;i<Radiosize;i++)
	{
		str_len = sprintf(str_char,"%02X ",RadioRxPayload[i]);
		Print((uint8_t*)&str_char,str_len);
	}
	Print("\r\n",2);

	str_len = sprintf(str_char,"rssi = %d,snr = %d\r\n", Radiorssi,Radiosnr);
	Print((uint8_t*)&str_char,str_len);
	Print("\r\n",2);
		
	return Radiosize;
}
/**DIO0引脚中断**/
void EXTI0_IRQHandler(void)
{
	uint16_t irqRegs;
	
	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
        
        irqRegs = SX126xGetIrqStatus( );
        SX126xClearIrqStatus( IRQ_RADIO_ALL );

        if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
        {
            SX126xSetOperatingMode( MODE_STDBY_RC );
            StackTxDoneCallback();
        }

        if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
        {
            if( RxContinuous == false )
            {
                SX126xSetOperatingMode( MODE_STDBY_RC );

                SX126xWriteRegister( 0x0902, 0x00 );
                SX126xWriteRegister( 0x0944, SX126xReadRegister( 0x0944 ) | ( 1 << 1 ) );
            }
            SX126xGetPayload( RadioRxPayload, &Radiosize , 255 );
            SX126xGetPacketStatus( &RadioPktStatus );
            StackRxDoneCallback();
        }

        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
        {
            if( RxContinuous == false )
            {
                SX126xSetOperatingMode( MODE_STDBY_RC );
            }
           StackRxErrorCallback();
        }

        if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
        {
            if( SX126xGetOperatingMode( ) == MODE_TX )
            {
                SX126xSetOperatingMode( MODE_STDBY_RC );
                StackRxTimeoutCallback( );
            }
            else if( SX126xGetOperatingMode( ) == MODE_RX )
            {
                SX126xSetOperatingMode( MODE_STDBY_RC );
                StackRxTimeoutCallback( );
            }
        }

        if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
        {
            if( RxContinuous == false )
            {
                SX126xSetOperatingMode( MODE_STDBY_RC );
            }
            StackRxTimeoutCallback( );
        }
    }
}


RadioOperatingModes_t SX126xGetOperatingMode( void )
{
    return OperatingMode;
}

void SX126xSetOperatingMode( RadioOperatingModes_t mode )
{
    OperatingMode = mode;
}

void SX126xCheckDeviceReady( void )
{
    if( ( SX126xGetOperatingMode( ) == MODE_SLEEP ) || ( SX126xGetOperatingMode( ) == MODE_RX_DC ) )
    {
        SX126xWakeup( );
        /// Switch is turned off when device is in sleep mode and turned on is all other modes
        SX126xAntSwOn( );
    }
    SX126xWaitOnBusy( );
}

void SX126xSetPayload( uint8_t *payload, uint8_t size )
{
    SX126xWriteBuffer( 0x00, payload, size );
}

uint8_t SX126xGetPayload( uint8_t *buffer, uint8_t *size,  uint8_t maxSize )
{
    uint8_t offset = 0;

    SX126xGetRxBufferStatus( size, &offset );
    if( *size > maxSize )
    {
        return 1;
    }
    SX126xReadBuffer( offset, buffer, *size );
    return 0;
}

void SX126xSendPayload( uint8_t *payload, uint8_t size, uint32_t timeout )
{
    SX126xSetPayload( payload, size );
    SX126xSetTx( timeout );
}

uint8_t SX126xSetSyncWord( uint8_t *syncWord )
{
    SX126xWriteRegisters( REG_LR_SYNCWORDBASEADDRESS, syncWord, 8 );
    return 0;
}

uint32_t SX126xGetRandom( void )
{
    uint8_t buf[] = { 0, 0, 0, 0 };

    /// Set radio in continuous reception
    SX126xSetRx( 0 );

    SX126xDelayMs( 1 );

    SX126xReadRegisters( RANDOM_NUMBER_GENERATORBASEADDR, buf, 4 );

    SX126xSetStandby( STDBY_RC );

    return ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
}

void SX126xSetSleep( SleepParams_t sleepConfig )
{
    SX126xAntSwOff( );

    uint8_t value = ( ( ( uint8_t )sleepConfig.Fields.WarmStart << 2 ) |
                      ( ( uint8_t )sleepConfig.Fields.Reset << 1 ) |
                      ( ( uint8_t )sleepConfig.Fields.WakeUpRTC ) );
    SX126xWriteCommand( RADIO_SET_SLEEP, &value, 1 );
    SX126xSetOperatingMode( MODE_SLEEP );
}

void SX126xSetStandby( RadioStandbyModes_t standbyConfig )
{
    SX126xWriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 );
    if( standbyConfig == STDBY_RC )
    {
        SX126xSetOperatingMode( MODE_STDBY_RC );
    }
    else
    {
        SX126xSetOperatingMode( MODE_STDBY_XOSC );
    }
}

void SX126xSetFs( void )
{
    SX126xWriteCommand( RADIO_SET_FS, 0, 0 );
    SX126xSetOperatingMode( MODE_FS );
}

void SX126xSetTx( uint32_t timeout )
{
    uint8_t buf[3];

    SX126xSetOperatingMode( MODE_TX );

    buf[0] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( timeout & 0xFF );
    SX126xWriteCommand( RADIO_SET_TX, buf, 3 );
}

void SX126xSetRx( uint32_t timeout )
{
    uint8_t buf[3];

    SX126xSetOperatingMode( MODE_RX );

    buf[0] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( timeout & 0xFF );
    SX126xWriteCommand( RADIO_SET_RX, buf, 3 );
}

void SX126xSetRxBoosted( uint32_t timeout )
{
    uint8_t buf[3];

    SX126xSetOperatingMode( MODE_RX );

    SX126xWriteRegister( REG_RX_GAIN, 0x96 ); /// max LNA gain, increase current by ~2mA for around ~3dB in sensivity

    buf[0] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( timeout & 0xFF );
    SX126xWriteCommand( RADIO_SET_RX, buf, 3 );
}

void SX126xSetRxDutyCycle( uint32_t rxTime, uint32_t sleepTime )
{
    uint8_t buf[6];

    buf[0] = ( uint8_t )( ( rxTime >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( rxTime >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( rxTime & 0xFF );
    buf[3] = ( uint8_t )( ( sleepTime >> 16 ) & 0xFF );
    buf[4] = ( uint8_t )( ( sleepTime >> 8 ) & 0xFF );
    buf[5] = ( uint8_t )( sleepTime & 0xFF );
    SX126xWriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 6 );
    SX126xSetOperatingMode( MODE_RX_DC );
}

void SX126xSetCad( void )
{
    SX126xWriteCommand( RADIO_SET_CAD, 0, 0 );
    SX126xSetOperatingMode( MODE_CAD );
}

void SX126xSetTxContinuousWave( void )
{
    SX126xWriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

void SX126xSetTxInfinitePreamble( void )
{
    SX126xWriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 );
}

void SX126xSetStopRxTimerOnPreambleDetect( bool enable )
{
    SX126xWriteCommand( RADIO_SET_STOPRXTIMERONPREAMBLE, ( uint8_t* )&enable, 1 );
}

void SX126xSetLoRaSymbNumTimeout( uint8_t SymbNum )
{
    SX126xWriteCommand( RADIO_SET_LORASYMBTIMEOUT, &SymbNum, 1 );
}

void SX126xSetRegulatorMode( RadioRegulatorMode_t mode )
{
    SX126xWriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}

void SX126xCalibrate( CalibrationParams_t calibParam )
{
    uint8_t value = ( ( ( uint8_t )calibParam.Fields.ImgEnable << 6 ) |
                      ( ( uint8_t )calibParam.Fields.ADCBulkPEnable << 5 ) |
                      ( ( uint8_t )calibParam.Fields.ADCBulkNEnable << 4 ) |
                      ( ( uint8_t )calibParam.Fields.ADCPulseEnable << 3 ) |
                      ( ( uint8_t )calibParam.Fields.PLLEnable << 2 ) |
                      ( ( uint8_t )calibParam.Fields.RC13MEnable << 1 ) |
                      ( ( uint8_t )calibParam.Fields.RC64KEnable ) );

    SX126xWriteCommand( RADIO_CALIBRATE, &value, 1 );
}

void SX126xCalibrateImage( uint32_t freq )
{
    uint8_t calFreq[2];

    if( freq > 900000000 )
    {
        calFreq[0] = 0xE1;
        calFreq[1] = 0xE9;
    }
    else if( freq > 850000000 )
    {
        calFreq[0] = 0xD7;
        calFreq[1] = 0xDB;
    }
    else if( freq > 770000000 )
    {
        calFreq[0] = 0xC1;
        calFreq[1] = 0xC5;
    }
    else if( freq > 460000000 )
    {
        calFreq[0] = 0x75;
        calFreq[1] = 0x81;
    }
    else if( freq > 425000000 )
    {
        calFreq[0] = 0x6B;
        calFreq[1] = 0x6F;
    }
    SX126xWriteCommand( RADIO_CALIBRATEIMAGE, calFreq, 2 );
}

void SX126xSetPaConfig( uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSel, uint8_t paLut )
{
    uint8_t buf[4];

    buf[0] = paDutyCycle;
    buf[1] = hpMax;
    buf[2] = deviceSel;
    buf[3] = paLut;
    SX126xWriteCommand( RADIO_SET_PACONFIG, buf, 4 );
}

void SX126xSetRxTxFallbackMode( uint8_t fallbackMode )
{
    SX126xWriteCommand( RADIO_SET_TXFALLBACKMODE, &fallbackMode, 1 );
}

void SX126xSetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask )
{
    uint8_t buf[8];

    buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( irqMask & 0x00FF );
    buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
    buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
    buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
    buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
    buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
    buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
    SX126xWriteCommand( RADIO_CFG_DIOIRQ, buf, 8 );
}

uint16_t SX126xGetIrqStatus( void )
{
    uint8_t irqStatus[2];

    SX126xReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
    return ( irqStatus[0] << 8 ) | irqStatus[1];
}

void SX126xSetDio2AsRfSwitchCtrl( uint8_t enable )
{
    SX126xWriteCommand( RADIO_SET_RFSWITCHMODE, &enable, 1 );
}

void SX126xSetDio3AsTcxoCtrl( RadioTcxoCtrlVoltage_t tcxoVoltage, uint32_t timeout )
{
    uint8_t buf[4];

    buf[0] = tcxoVoltage & 0x07;
 ///   buf[1] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
 ///   buf[2] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
 ///   buf[3] = ( uint8_t )( timeout & 0xFF );

	buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x64;
	
	
    SX126xWriteCommand( RADIO_SET_TCXOMODE, buf, 4 );
}

void SX126xSetRfFrequency( uint32_t frequency )
{
    uint8_t buf[4];
    uint32_t freq = 0;

    if( ImageCalibrated == false )
    {
        SX126xCalibrateImage( frequency );
        ImageCalibrated = true;
    }

    freq = ( uint32_t )( ( double )frequency / ( double )FREQ_STEP );
    buf[0] = ( uint8_t )( ( freq >> 24 ) & 0xFF );
    buf[1] = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    buf[2] = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    buf[3] = ( uint8_t )( freq & 0xFF );
    SX126xWriteCommand( RADIO_SET_RFFREQUENCY, buf, 4 );
}

void SX126xSetPacketType( RadioPacketTypes_t packetType )
{
    SX126xWriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
}


void SX126xSetTxParams( int8_t power, RadioRampTimes_t rampTime )
{
    uint8_t buf[2];

	/// WORKAROUND - Better Resistance of the SX1262 Tx to Antenna Mismatch, see DS_SX1261-2_V1.2 datasheet chapter 15.2
	/// RegTxClampConfig = @address 0x08D8
	SX126xWriteRegister( 0x08D8, SX126xReadRegister( 0x08D8 ) | ( 0x0F << 1 ) );
	/// WORKAROUND END

	SX126xSetPaConfig( 0x04, 0x07, 0x00, 0x01 );
	if( power > 22 )
	{
		power = 22;
	}
	else if( power < -9 )
	{
		power = -9;
	}
	SX126xWriteRegister( REG_OCP, 0x38 ); /// current max 160mA for the whole device

    buf[0] = power;
    buf[1] = ( uint8_t )rampTime;
    SX126xWriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void SX126xSetModulationParams( ModulationParams_t *modulationParams )
{
    uint8_t buf[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    /// Check if required configuration corresponds to the stored packet type
    /// If not, silently update radio packet type

    SX126xSetPacketType( modulationParams->PacketType );

	buf[0] = modulationParams->Params.LoRa.SpreadingFactor;
	buf[1] = modulationParams->Params.LoRa.Bandwidth;
	buf[2] = modulationParams->Params.LoRa.CodingRate;
	buf[3] = modulationParams->Params.LoRa.LowDatarateOptimize;

	SX126xWriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 4 );
}

void SX126xSetPacketParams( PacketParams_t *packetParams )
{
    uint8_t buf[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    SX126xSetPacketType( packetParams->PacketType );

	buf[0] = ( packetParams->Params.LoRa.PreambleLength >> 8 ) & 0xFF;
	buf[1] = packetParams->Params.LoRa.PreambleLength;
	buf[2] = LoRaHeaderType = packetParams->Params.LoRa.HeaderType;
	buf[3] = packetParams->Params.LoRa.PayloadLength;
	buf[4] = packetParams->Params.LoRa.CrcMode;
	buf[5] = packetParams->Params.LoRa.InvertIQ;
    
    SX126xWriteCommand( RADIO_SET_PACKETPARAMS, buf, 6 );
}

void SX126xSetCadParams( RadioLoRaCadSymbols_t cadSymbolNum, uint8_t cadDetPeak, uint8_t cadDetMin, RadioCadExitModes_t cadExitMode, uint32_t cadTimeout )
{
    uint8_t buf[7];

    buf[0] = ( uint8_t )cadSymbolNum;
    buf[1] = cadDetPeak;
    buf[2] = cadDetMin;
    buf[3] = ( uint8_t )cadExitMode;
    buf[4] = ( uint8_t )( ( cadTimeout >> 16 ) & 0xFF );
    buf[5] = ( uint8_t )( ( cadTimeout >> 8 ) & 0xFF );
    buf[6] = ( uint8_t )( cadTimeout & 0xFF );
    SX126xWriteCommand( RADIO_SET_CADPARAMS, buf, 7 );
    SX126xSetOperatingMode( MODE_CAD );
}

void SX126xSetBufferBaseAddress( uint8_t txBaseAddress, uint8_t rxBaseAddress )
{
    uint8_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    SX126xWriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

RadioStatus_t SX126xGetStatus( void )
{
    uint8_t stat = 0;
    RadioStatus_t status = { .Value = 0 };

    stat = SX126xReadCommand( RADIO_GET_STATUS, NULL, 0 );
    status.Fields.CmdStatus = ( stat & ( 0x07 << 1 ) ) >> 1;
    status.Fields.ChipMode = ( stat & ( 0x07 << 4 ) ) >> 4;
    return status;
}

int8_t SX126xGetRssiInst( void )
{
    uint8_t buf[1];
    int8_t rssi = 0;

    SX126xReadCommand( RADIO_GET_RSSIINST, buf, 1 );
    rssi = -buf[0] >> 1;
    return rssi;
}

void SX126xGetRxBufferStatus( uint8_t *payloadLength, uint8_t *rxStartBufferPointer )
{
    uint8_t status[2];

    SX126xReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

    /// In case of LORA fixed header, the payloadLength is obtained by reading
    /// the register REG_LR_PAYLOADLENGTH
    if( LoRaHeaderType == LORA_PACKET_FIXED_LENGTH )
    {
        *payloadLength = SX126xReadRegister( REG_LR_PAYLOADLENGTH );
    }
    else
    {
        *payloadLength = status[0];
    }
    *rxStartBufferPointer = status[1];
}

void SX126xGetPacketStatus( PacketStatus_t *pktStatus )
{
    uint8_t status[3];

    SX126xReadCommand( RADIO_GET_PACKETSTATUS, status, 3 );

	pktStatus->Params.LoRa.RssiPkt = -status[0] >> 1;
	/// Returns SNR value [dB] rounded to the nearest integer value
	pktStatus->Params.LoRa.SnrPkt = ( ( ( int8_t )status[1] ) + 2 ) >> 2;
	Radiosnr = pktStatus->Params.LoRa.SnrPkt;
	pktStatus->Params.LoRa.SignalRssiPkt = -status[2] >> 1;
	Radiorssi = pktStatus->Params.LoRa.SignalRssiPkt;
	pktStatus->Params.LoRa.FreqError = FrequencyError;
}

RadioError_t SX126xGetDeviceErrors( void )
{
    uint8_t err[] = { 0, 0 };
    RadioError_t error = { .Value = 0 };

    SX126xReadCommand( RADIO_GET_ERROR, ( uint8_t* )err, 2 );
    error.Fields.PaRamp     = ( err[0] & ( 1 << 0 ) ) >> 0;
    error.Fields.PllLock    = ( err[1] & ( 1 << 6 ) ) >> 6;
    error.Fields.XoscStart  = ( err[1] & ( 1 << 5 ) ) >> 5;
    error.Fields.ImgCalib   = ( err[1] & ( 1 << 4 ) ) >> 4;
    error.Fields.AdcCalib   = ( err[1] & ( 1 << 3 ) ) >> 3;
    error.Fields.PllCalib   = ( err[1] & ( 1 << 2 ) ) >> 2;
    error.Fields.Rc13mCalib = ( err[1] & ( 1 << 1 ) ) >> 1;
    error.Fields.Rc64kCalib = ( err[1] & ( 1 << 0 ) ) >> 0;
    return error;
}

void SX126xClearDeviceErrors( void )
{
    uint8_t buf[2] = { 0x00, 0x00 };
    SX126xWriteCommand( RADIO_CLR_ERROR, buf, 2 );
}

void SX126xClearIrqStatus( uint16_t irq )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( ( uint16_t )irq >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( ( uint16_t )irq & 0x00FF );
    SX126xWriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}
 

void SX126xIoTcxoInit( void )
{
    CalibrationParams_t calibParam;
	/// convert from ms to SX126x time base
    SX126xSetDio3AsTcxoCtrl( TCXO_CTRL_3_3V, SX126xGetBoardTcxoWakeupTime( ) << 6 ); 
    calibParam.Value = 0x7F;
    SX126xCalibrate( calibParam );
}

uint32_t SX126xGetBoardTcxoWakeupTime( void )
{
    return BOARD_TCXO_WAKEUP_TIME;
}




void SX126xWakeup( void )
{
    IRQ_DISABLE();;

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ( RADIO_GET_STATUS );
    MX_SX126X_SPI_WRITE_READ( 0x00 );

    MX_SX126X_CS_HIGH();

    /// Wait for chip to be ready.
    SX126xWaitOnBusy( );

    IRQ_ENABLE();
}

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ( ( uint8_t )command );

    for( i = 0; i < size; i++ )
    {
        MX_SX126X_SPI_WRITE_READ( buffer[i] );
    }

    MX_SX126X_CS_HIGH();

    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

uint8_t SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint8_t status = 0;
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ( ( uint8_t )command );
    status = MX_SX126X_SPI_WRITE_READ( 0x00 );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = MX_SX126X_SPI_WRITE_READ( 0 );
    }

    MX_SX126X_CS_HIGH();

    SX126xWaitOnBusy( );

    return status;
}
///操作寄存器
void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();
    
    MX_SX126X_SPI_WRITE_READ( RADIO_WRITE_REGISTER );
    MX_SX126X_SPI_WRITE_READ( ( address & 0xFF00 ) >> 8 );
    MX_SX126X_SPI_WRITE_READ( address & 0x00FF );
    
    for( i = 0; i < size; i++ )
    {
        MX_SX126X_SPI_WRITE_READ( buffer[i] );
    }

    MX_SX126X_CS_HIGH();

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}
void SX126xWaitOnBusy( void )
{
    while(GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8));
}
///操作寄存器
void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ( RADIO_READ_REGISTER );
    MX_SX126X_SPI_WRITE_READ( ( address & 0xFF00 ) >> 8 );
    MX_SX126X_SPI_WRITE_READ( address & 0x00FF );
    MX_SX126X_SPI_WRITE_READ( 0 );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = MX_SX126X_SPI_WRITE_READ( 0 );
    }
    MX_SX126X_CS_HIGH();

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}
///操作FIFO
void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ( RADIO_WRITE_BUFFER );
    MX_SX126X_SPI_WRITE_READ( offset );
    for( i = 0; i < size; i++ )
    {
        MX_SX126X_SPI_WRITE_READ( buffer[i] );
    }
    MX_SX126X_CS_HIGH();

    SX126xWaitOnBusy( );
}
///操作FIFO
void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
	uint16_t i;
	
    SX126xCheckDeviceReady( );

    MX_SX126X_CS_LOW();

    MX_SX126X_SPI_WRITE_READ(RADIO_READ_BUFFER);
    MX_SX126X_SPI_WRITE_READ( offset );
    MX_SX126X_SPI_WRITE_READ( 0 );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = MX_SX126X_SPI_WRITE_READ( 0 );
    }
    MX_SX126X_CS_HIGH();

    SX126xWaitOnBusy( );
}

void SX126xSetRfTxPower( int8_t power )
{
    SX126xSetTxParams( power, RADIO_RAMP_40_US );
}
void SX126xAntSwOn( void )
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
}

void SX126xAntSwOff( void )
{
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
}
