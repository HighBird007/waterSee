#include "Loop.h"
//继电器地址0x02   多功能水质检测仪ZW 0x01   污浊传感器ZD0x03 排水泵继电器地址 0x11 屏幕包含许多地址 从0B 第一个鱼池开始 0c第二个鱼池

//如果定义了下面 则水位变化采用中断
//#define ITLevel 1

uint8_t measureNum = 6 ; //测量轮数 一个池子测量多少次最初时3次  //系统主频80mhz 当分频器时60000  计数器时 40000 则是30s触发一次 所以发送lora数据的频率就是   30s * 需要测量的数据次数
uint8_t flushNum = 3 ;//需要完成的冲洗次数 目前3
uint8_t nodeNum = 11;//目前系统多少个池子
uint8_t flushTimeOut = 5 ; //参数设置超时时间 超时时间 = flushTimeOut * 30s 预防某一个池子水泵 或者 继电器没电了 导致程序无法工作下去
uint8_t screenRequestLength = 8 ;
//上面变量可以自己设置改变 目前100s发送一次检测数据

//下面变量通常不必设置
uint8_t task=1  ;//当前任务状态，冲洗flush：1, 测量measure:2
uint8_t flushCount=0 ; //当前系统冲洗了多少次
uint8_t measureCount=0; //当前系统已经测量了多少次
uint8_t node= 0 ;// 当前测量的节点（鱼池）
uint8_t flushTimeOutCount = 0 ;//当前超时定时器数值
errorType deviceStatus = normal;//表示当前设备状态
pondStruct pondSet[20];//存放鱼池结构体数组
uint8_t draining = false ; //当前的排水泵是否开启
uint8_t filling = false ;//当前的进水泵是否开启
uint8_t level = LOW;//当前的水位
uint8_t loraData[100];//组装lora协议格式 发送数据

void loop(void){
  
       judgeWaterLevel();
       
      if(deviceStatus != normal){
        
       handingError(deviceStatus);
      
      }
	if(task == 1 ){
	flushTask();
	}
	else
	if(task == 2){
	measureTask();
	}
	
}

void flushTask(){
	
	if( flushCount >= flushNum ){
		
                task = 2;
		
		flushCount = 0;
		
                flushTimeOutCount = 0 ;
                
		return;
		
	}

	if( filling == true && level == HIGH ){

                openDraining();
		
		flushCount++;
		
	}
    else if( draining == true && level==LOW ){

		openFilling();
		
    }
    /*
    else {
    
      Print("wtf",3);
    openDraining();
    
    }
*/
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

void assembleLoraData(){
  
    loraData[0] = 0xFE;  // 起始符

    // 计算总长度: 1 (起始符) + 1 (总长度) + 1 (版本号) + 1 (命令码) + 1 (节点) + 1 (40) + ZW.loraDataLength + ZD.loraDataLength + 1 (和校验) + 1 (结束符)
    loraData[1] = 4 + 2 + ZW.loraDataLength + ZD.loraDataLength + 2 ;

    loraData[2] = 0x02;  // 版本号

    loraData[3] = 0x81;  // 命令码

    loraData[4] = node + 1;  // 节点

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
    
    updatePondStructData();
    
    flushTimeOutCount = 0 ;
    
}
#define noiseCount 3

void judgeWaterLevel(){
FeedDog();
   
#ifndef ITLevel
        
      GPIO_PinState low = HAL_GPIO_ReadPin(LowFlag_GPIO_Port,LowFlag_Pin);
      GPIO_PinState high = HAL_GPIO_ReadPin(HighFlag_GPIO_Port,HighFlag_Pin);
      
      //如果此时low引脚是低电平 处于低液位
      if(low == GPIO_PIN_RESET){
      
        for(int i = 0 ; i < noiseCount ; i++){
        
          HAL_Delay(500);
          FeedDog();
          
       //  Print("low is low\n",strlen("low is low\n"));
         GPIO_PinState test = HAL_GPIO_ReadPin(LowFlag_GPIO_Port,LowFlag_Pin);
          
          if(test != GPIO_PIN_RESET){
          
         Print("low is error\n",strlen("low is error\n"));
            return ;
          }
          
        }
      // Print("sure low is low\n",strlen("sure low is low\n"));
      level = LOW;
      
      }
      //如果high引脚处于高电平 那么处于高液位
      else if( high == GPIO_PIN_SET ){
      
          for(int i = 0 ; i < noiseCount ; i++){
        
          HAL_Delay(500);
          FeedDog();
          GPIO_PinState test = HAL_GPIO_ReadPin(HighFlag_GPIO_Port,HighFlag_Pin);
          if(test != GPIO_PIN_SET){
          
         // Print("high is error\n",strlen("high is error\n"));
          return ;
          
          }
          
        }
     //  Print("sure high is high\n",strlen("sure high is high\n"));
      level = HIGH;
      
      } 
#endif

      
	FeedDog();



}
void handingError(errorType type){
  
  uint8_t errorMes[50];
             
  switch(type){
  
    case loraError:
    
    HAL_Delay(100000);
    
    break;
  
    case relayError:
    
    sprintf(errorMes,"relay error pumpId %d",node+1);
    
        pondSet[node].tp = -1;
        pondSet[node].o2 = -1;
        pondSet[node].ph = -1;
        pondSet[node].zd = -1;
        
                  // LoraTxPkt(errorMes, strlen(errorMes));
  
           Print(errorMes,strlen(errorMes));
           
                      deviceStatus = normal;
           
           flushTimeOutCount = 0 ;
           
           task=1 ;
           
           flushCount=0 ; //当前系统冲洗了多少次
           
           measureCount=0; //当前系统已经测量了多少次
                      node++;//当前水池异常推迟下一个试试
           
           node = node % nodeNum ;
           return ;
    break;
    
    case pumpError:
    
    sprintf(errorMes,"pump error pumpId %d",node+1);
    
        pondSet[node].tp = -1;
        pondSet[node].o2 = -1;
        pondSet[node].ph = -1;
        pondSet[node].zd = -1;
    
    
    break;
    
  }
  
           LoraTxPkt(errorMes, strlen(errorMes));
  
           Print(errorMes,strlen(errorMes));
           
           closeFilling();
           
           node++;//当前水池异常推迟下一个试试
           
           node = node % nodeNum ;
           
           openFilling();
           
           deviceStatus = normal;
           
           flushTimeOutCount = 0 ;
           
           task=1 ;
           
           flushCount=0 ; //当前系统冲洗了多少次
           
           measureCount=0; //当前系统已经测量了多少次

}

void initLoop(void){
  
#ifdef ITLevel
       initWaterLevelGPIO_IT();
#else
       initWaterLevelGPIO_Input();
#endif
       
        
               
        //初始化测量定时器 冲洗完成后 等待水体稳定下来后测量
        MX_TIM15_Init();
        //这个定时器是用来定时  抽水任务的 如果长时间没有将对应的变量清0 则会上报检测
        MX_TIM6_Init();
        //初始化传感器结构体
        initSensor();
        
        
	for(;node < nodeNum ;node++){
	closeFilling();
	HAL_Delay(500);
	}
        
	node = 0;
        
        openDraining();
        
        HAL_TIM_Base_Start_IT(&htim6);
        
        
        //下面的是生成模拟量测试  现场注释
        
        for(int i = 0 ; i<12;i++){
        
        pondSet[i].tp = i * 4 + 1;
        pondSet[i].o2 = i * 4 + 2;
        pondSet[i].ph = i * 4 + 3;
        pondSet[i].zd = i * 4 + 4;
        
        }
        
}



//关闭进水泵  但是不一定要打开排水泵
void closeFilling(){
                
            //    Print("Filling Close \n",strlen("Filling Close \n"));
                
		filling=false;
                
		controlDeviceStatus(node,powerOff);
}

//关闭排水阀 不一定需要开进水泵 但是打开排水泵 一定要关闭进水泵
void openDraining(){
               // Print("Draining Open \n",strlen("Draining Open \n"));
                
		filling = false ;
		controlDeviceStatus(node,powerOff);
		HAL_Delay(100);
		controlDeviceStatus(drainingPumpRoad,powerOn);
		draining = true ;
                HAL_Delay(1000);
                FeedDog();

		
}
void closeDraining(){
        //Print("Draining Close \n",strlen("Draining Close \n"));
  for(int i = 0 ; i < 10 ; i++){
  HAL_Delay(500);
  FeedDog();
  
  }
                controlDeviceStatus(drainingPumpRoad,powerOff);
		draining = false ;
}

//控制进水泵开 就意味着 排水阀必须关
void openFilling(){
                //  Print("Filling Open \n",strlen("Filling Open \n"));
		filling=true;
		controlDeviceStatus(node,powerOn);
		HAL_Delay(500);
        FeedDog();
		closeDraining();
		draining=false;
        HAL_Delay(1000);
        FeedDog();
                

}