#include "Loop.h"
//继电器地址0x02   多功能水质检测仪ZW 0x01   污浊传感器ZD0x03 排水泵继电器地址 0x11

//如果注释下面的debugmode宏定义 则不会自动改变水位
//#define debugMode 1

//如果关闭debug  一定要注意下面的三个参数
uint8_t measureNum = 1 ; //测量轮数 一个池子测量多少次最初时3次  //系统主频80mhz 当分频器时60000  计数器时 40000 则是30s触发一次 所以发送lora数据的频率就是 30s * 需要测量的数据次数
uint8_t flushNum = 1 ;//需要完成的冲洗次数 目前3
uint8_t nodeNum = 4;//目前系统多少个池子
//上面三组变量可以自己设置改变 目前100s发送一次检测数据

uint8_t task=1  ;//当前任务状态，冲洗flush：1, 测量measure:2
uint8_t flushCount=0 ; //当前系统冲洗了多少次
uint8_t measureCount=0; //当前系统已经测量了多少次
uint8_t node= 0 ;// 当前测量的节点（鱼池）

//uint8_t canMeasure = false;
uint8_t draining = false ; //当前的排水泵是否开启
uint8_t filling = false ;//当前的进水泵是否开启
uint8_t level = LOW;//当前的水位


//水位高低引脚以及中断的设置
#define LowFlag_Pin GPIO_PIN_14
#define LowFlag_GPIO_Port GPIOB
#define LowFlag_EXTI_IRQn EXTI15_10_IRQn
#define HighFlag_Pin GPIO_PIN_15
#define HighFlag_GPIO_Port GPIOB
#define HighFlag_EXTI_IRQn EXTI15_10_IRQn

/*
pb13   时低水平位置  也就是 pb14

pb12  时高液位位  对应的时pb15

*/
void initWaterLevelGPIO_IT(){
  
  
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = LowFlag_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(LowFlag_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = HighFlag_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(HighFlag_GPIO_Port, &GPIO_InitStruct);
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  
}

void  initWaterLevelGPIO_Input(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : PBPin PBPin */
  GPIO_InitStruct.Pin = LowFlag_Pin|HighFlag_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}
//如果定义了下面 则水位变化采用中断
#define ITLevel 1

void initLoop(void){
#ifdef ITLevel
       initWaterLevelGPIO_IT();
#else
       initWaterLevelGPIO_Input();
#endif
       
        //初始化测量定时器 但计时功能有待商榷 不知道当前的主频是多少
        MX_TIM15_Init();
        //初始化传感器结构体
        initSensor();
        
	for(;node < nodeNum ;node++){
	closeFilling();
	HAL_Delay(500);
        
	}
	node = 0;
	
	////当前处于高水使
	if(HAL_GPIO_ReadPin(GPIOC,HighFlag_Pin) == GPIO_PIN_SET){
		
		level = HIGH;
		openDraining();
		
	}
	//当前处于低水使
	else if(HAL_GPIO_ReadPin(GPIOC,LowFlag_Pin) == GPIO_PIN_RESET){
		
		level = LOW;
		openFilling();
		
	}
	//既不是低水位 也不是高水位 处于中间
	else {
		level= HIGH;
		openDraining();
	}
        
}


//当水位处于高水位 并且此时进水泵处于开启状态则 ++  知道三次退出冲洗事件进入 测量事件  退出时开启排水 
//测量前需要确保canMeasure是true  true 的条件是  此时进水泵开启 并且是高水位状态 如果没有则开始进水泵 

//draining 代表排水 true 代表排水 false 代表
//filling 代表水泵进水 true 代表开启水泵加水
void flushTask(){
	
	if( flushCount >= flushNum ){
		
                task = 2;
		
		flushCount = 0;
		
		return;
		
	}

	if( filling == true && level == HIGH ){

                openDraining();
		
		flushCount++;
		
	}
	//进水泵未开启 但是水位已经处于比较低的水准 则开启水泵 并且关闭排水阀 更新状态 但是flush并不会++
    else if( draining == true && level==LOW ){

		openFilling();
		
	} 
}

void measureTask(){
	
		if(measureCount >= measureNum ){

		
                FeedDog();
                 
                ZWRead();
                
                ZDRead();
                
                assembleLoraData();
                
			node++;
			 
			node = node % nodeNum ;
			
			openDraining();
			
			task = 1;
			
			measureCount = 0;
			
			return ;
			
		}
	
		if( filling == true && level == HIGH){
		
		closeFilling();
			
		HAL_TIM_Base_Start_IT(&htim15);
                
		}else if(filling == false && level == LOW){
			
		openFilling();
			
		}

}

#define noiseCount 3

void loop(void){
  
#ifndef ITLevel
        
      GPIO_PinState low = HAL_GPIO_ReadPin(LowFlag_GPIO_Port,LowFlag_Pin);
      GPIO_PinState high = HAL_GPIO_ReadPin(HighFlag_GPIO_Port,HighFlag_Pin);
      
      //如果此时low引脚是低电平 处于低液位
      if(low == GPIO_PIN_RESET){
      
        for(int i = 0 ; i < noiseCount ; i++){
        
          HAL_Delay(500);
          
          GPIO_PinState test = HAL_GPIO_ReadPin(LowFlag_GPIO_Port,LowFlag_Pin);
          
          if(test != GPIO_PIN_RESET)return ;
          
        }
        
      level = LOW;
      
      }
      //如果high引脚处于高电平 那么处于高液位
      else if( high == GPIO_PIN_SET ){
      
          for(int i = 0 ; i < noiseCount ; i++){
        
          HAL_Delay(500);
          
          GPIO_PinState test = HAL_GPIO_ReadPin(HighFlag_GPIO_Port,HighFlag_Pin);
          
          if(test != GPIO_PIN_SET)return ;
          
        }
        
      level = HIGH;
      
      } 
#endif

	 FeedDog();
	if(task == 1 ){
	flushTask();
	}
	else
	if(task == 2){
	measureTask();
	}
	
}
//组装lora协议格式 发送数据
uint8_t loraData[100];

void assembleLoraData(){
  
    loraData[0] = 0xFE;  // 起始符

    // 计算总长度: 1 (起始符) + 1 (总长度) + 1 (版本号) + 1 (命令码) + 1 (节点) + 1 (40) + ZW.loraDataLength + ZD.loraDataLength + 1 (和校验) + 1 (结束符)
    loraData[1] = 4 + 2 + ZW.loraDataLength + ZD.loraDataLength + 2 ;

    loraData[2] = 0x02;  // 版本号

    loraData[3] = 0x81;  // 命令码

    loraData[4] = node;  // 节点

    loraData[5] = 40;    // 固定值

    // 拷贝 ZW 数据，首先是前 12 字节
    memcpy(loraData + 6, ZW.toLora, ZW.loraDataLengthShould);

    // 拷贝 ZD 数据
    memcpy(loraData + 6 + ZW.loraDataLength, ZD.toLora, ZD.loraDataLengthShould);

    // 计算所有字节的和
    uint8_t check_val = U8SumCheck(loraData + 1, 45);

    // 添加和校验到数据中
    loraData[6 + ZW.loraDataLength + ZD.loraDataLength] = check_val;

    // 添加结束符
    loraData[7 + ZW.loraDataLength + ZD.loraDataLength] = 0xFE;  // 结束符

    // 发送数据包
    LoraTxPkt(loraData, loraData[1]);
    
}


void EXTI15_10_IRQHandler(void)
{

  HAL_GPIO_EXTI_IRQHandler(LowFlag_Pin);
  HAL_GPIO_EXTI_IRQHandler(HighFlag_Pin);

}

//重写gpio的中断函数
uint32_t lastTime;
uint32_t currentTime;
uint8_t acceptTime =5;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == LowFlag_Pin){
		
		level = LOW;
		//Print("low\n",5);
		
	}else if(GPIO_Pin == HighFlag_Pin){
		//Print("high\n",6);
		level = HIGH;
		
	}
	
	lastTime = currentTime;
	
}


//测量一分钟间隔
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	
	if(htim == &htim15){
		
		++measureCount;
 
		if(measureCount >= measureNum){

                HAL_TIM_Base_Stop_IT(&htim15);
		
		}
	}
}




void TIM1_BRK_TIM15_IRQHandler(void)
{
  
   HAL_TIM_IRQHandler(&htim15);
   
}

//关闭进水泵  但是不一定要打开排水泵
void closeFilling(){
                
                Print("Filling Close \n",strlen("Filling Close \n"));
                
		filling=false;
                
		controlDeviceStatus(node,powerOff);
}

//关闭排水阀 不一定需要开进水泵 但是打开排水泵 一定要关闭进水泵
void openDraining(){
                Print("Draining Open \n",strlen("Draining Open \n"));
                
		filling = false ;
		controlDeviceStatus(node,powerOff);
		HAL_Delay(100);
		controlDeviceStatus(drainingPumpRoad,powerOn);
		draining = true ;
                HAL_Delay(1000);
                FeedDog();
#ifdef debugMode
		levelToLow();
#endif
		
}
void closeDraining(){
                Print("Draining Close \n",strlen("Draining Close \n"));
                controlDeviceStatus(drainingPumpRoad,powerOff);
		draining = false ;
}

//控制进水泵开 就意味着 排水阀必须关
void openFilling(){
                  Print("Filling Open \n",strlen("Filling Open \n"));
		filling=true;
		controlDeviceStatus(node,powerOn);
		HAL_Delay(500);
        FeedDog();
		closeDraining();
		draining=false;
        HAL_Delay(1000);
        FeedDog();
                
#ifdef debugMode
		levelToHigh();
#endif
}

void levelToHigh(){
  
	level = HIGH;
        Print("HIGH",4);
        
}
void levelToLow(){
  
        level = LOW;
        Print("LOW",3);
}
