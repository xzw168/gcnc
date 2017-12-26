

#include "main.h"

static void ADC_WatchdogConfig(void)
{
  ADC_AnalogWatchdogSingleChannelConfig(ENC_ADCx,ENC_ADC_CH);//配置模拟看门狗保护的单通道
  ADC_AnalogWatchdogThresholdsConfig(ENC_ADCx,410,100);//配置模拟看门狗的高低阈值
  ADC_AnalogWatchdogCmd(ENC_ADCx,ADC_AnalogWatchdog_SingleRegEnable);//在单个/所有常规或注入的通道上启用或禁用模拟看门狗。
	
  
}

static void NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_Init(&NVIC_InitStructure);

	ADC_ITConfig(ENC_ADCx,ADC_IT_AWD,ENABLE);
}

static void adc_Config(void)
{
	ADC_InitTypeDef       ADC_InitStructure;	
	
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12位模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;//非扫描模式	
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//DISABLE;//连续转换模式
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁止触发检测，使用软件触发
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//右对齐	
	ADC_InitStructure.ADC_NbrOfConversion = 1;//1个转换在规则序列中 也就是只转换规则序列1 
	ADC_Init(ENC_ADCx, &ADC_InitStructure);
    	
	ADC_RegularChannelConfig(ENC_ADCx, ENC_ADC_CH, 1, ADC_SampleTime_480Cycles );	//ADC5,ADC通道,480个周期,提高采样时间可以提高精确度
	
	ADC_Cmd(ENC_ADCx, ENABLE);//开启AD转换器
}

static void gpio_Config(void)
{
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;

	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 		/*设置引脚模式*/  
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;		/*设置引脚的输出类型*/
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 		/*设置引脚上拉模式*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 	/*设置引脚速率为50MHz */ 
	
	GPIO_InitStructure.GPIO_Pin = ENC_A_PIN;			/*选择要控制的GPIO引脚*/
	GPIO_Init(ENC_A_GPIO_PORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(ENC_A_GPIO_PORT,ENC_A_SOURCE,ENC_A_AF);
	GPIO_InitStructure.GPIO_Pin = ENC_B_PIN;			/*选择要控制的GPIO引脚*/
	GPIO_Init(ENC_B_GPIO_PORT, &GPIO_InitStructure);	
	GPIO_PinAFConfig(ENC_B_GPIO_PORT,ENC_B_SOURCE,ENC_B_AF);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入 
	GPIO_InitStructure.GPIO_Pin = ENC_ADC_PIN;
	GPIO_Init(ENC_ADC_GPIO_PORT, &GPIO_InitStructure);
	
}

static void tim_Config(void)
{
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
	TIM_ICInitTypeDef TIM_ICInitStructure;
	
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // No prescaling 
	TIM_TimeBaseStructure.TIM_Period = 200; 
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;   
	TIM_TimeBaseInit(ENC_TIMx, &TIM_TimeBaseStructure);
	
	TIM_EncoderInterfaceConfig(ENC_TIMx, TIM_EncoderMode_TI1, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 15;
	TIM_ICInit(ENC_TIMx, &TIM_ICInitStructure);
	
	TIM_Cmd(ENC_TIMx, ENABLE);
	
}


//获得ADC值
//ch:通道值 @ref ADC_channels  0~16：ADC_Channel_0~ADC_Channel_16
//返回值:转换结果
static u16 Get_Adc(u8 ch)   
{
	 //设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ENC_ADCx, ch, 1, ADC_SampleTime_480Cycles );	//ENC_ADCx,ADC通道,480个周期,提高采样时间可以提高精确度			    
  
	ADC_SoftwareStartConv(ENC_ADCx);		//使能指定的ENC_ADCx的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ENC_ADCx, ADC_FLAG_EOC ));//等待转换结束

	return ADC_GetConversionValue(ENC_ADCx);	//返回最近一次ENC_ADCx规则组的转换结果
}
//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
static u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);//获取通道转换值
		rt_thread_delay(1);
	}
	return temp_val/times;
} 

//获取
u32 enc_get(void)
{
	return ENC_TIMx->CNT;
}

//获取
u16 enc_GetAdc(void)
{
	return Get_Adc_Average(ENC_ADC_CH,10);	//读取通道16内部温度传感器通道,10次取平均
}

void ADC_IRQHandler(void)
{
  ADC_ITConfig(ENC_ADCx,ADC_IT_AWD,DISABLE);
  if(SET == ADC_GetFlagStatus(ENC_ADCx,ADC_FLAG_AWD))
  {
    ADC_ClearFlag(ENC_ADCx,ADC_FLAG_AWD);
    ADC_ClearITPendingBit(ENC_ADCx,ADC_IT_AWD);
      printf("ADC AWD is happened.\r\n");
   }
   ADC_ITConfig(ENC_ADCx,ADC_IT_AWD,ENABLE);
}


//初始化
void enc_Init(void)
{

	gpio_Config();
	tim_Config();
	adc_Config();
	ADC_WatchdogConfig();
	NVIC_Config();
	ADC_SoftwareStartConv(ENC_ADCx);		//使能指定的ENC_ADCx的软件转换启动功能
}


