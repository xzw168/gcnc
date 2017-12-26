

#include "main.h"

static void ADC_WatchdogConfig(void)
{
  ADC_AnalogWatchdogSingleChannelConfig(ENC_ADCx,ENC_ADC_CH);//����ģ�⿴�Ź������ĵ�ͨ��
  ADC_AnalogWatchdogThresholdsConfig(ENC_ADCx,410,100);//����ģ�⿴�Ź��ĸߵ���ֵ
  ADC_AnalogWatchdogCmd(ENC_ADCx,ADC_AnalogWatchdog_SingleRegEnable);//�ڵ���/���г����ע���ͨ�������û����ģ�⿴�Ź���
	
  
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
	
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12λģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;//��ɨ��ģʽ	
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//DISABLE;//����ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//��ֹ������⣬ʹ���������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//�Ҷ���	
	ADC_InitStructure.ADC_NbrOfConversion = 1;//1��ת���ڹ��������� Ҳ����ֻת����������1 
	ADC_Init(ENC_ADCx, &ADC_InitStructure);
    	
	ADC_RegularChannelConfig(ENC_ADCx, ENC_ADC_CH, 1, ADC_SampleTime_480Cycles );	//ADC5,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��
	
	ADC_Cmd(ENC_ADCx, ENABLE);//����ADת����
}

static void gpio_Config(void)
{
	/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	GPIO_InitTypeDef GPIO_InitStructure;

	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 		/*��������ģʽ*/  
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;		/*�������ŵ��������*/
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 		/*������������ģʽ*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 	/*������������Ϊ50MHz */ 
	
	GPIO_InitStructure.GPIO_Pin = ENC_A_PIN;			/*ѡ��Ҫ���Ƶ�GPIO����*/
	GPIO_Init(ENC_A_GPIO_PORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(ENC_A_GPIO_PORT,ENC_A_SOURCE,ENC_A_AF);
	GPIO_InitStructure.GPIO_Pin = ENC_B_PIN;			/*ѡ��Ҫ���Ƶ�GPIO����*/
	GPIO_Init(ENC_B_GPIO_PORT, &GPIO_InitStructure);	
	GPIO_PinAFConfig(ENC_B_GPIO_PORT,ENC_B_SOURCE,ENC_B_AF);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//ģ������ 
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


//���ADCֵ
//ch:ͨ��ֵ @ref ADC_channels  0~16��ADC_Channel_0~ADC_Channel_16
//����ֵ:ת�����
static u16 Get_Adc(u8 ch)   
{
	 //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ENC_ADCx, ch, 1, ADC_SampleTime_480Cycles );	//ENC_ADCx,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��			    
  
	ADC_SoftwareStartConv(ENC_ADCx);		//ʹ��ָ����ENC_ADCx�����ת����������	
	 
	while(!ADC_GetFlagStatus(ENC_ADCx, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ENC_ADCx);	//�������һ��ENC_ADCx�������ת�����
}
//��ȡͨ��ch��ת��ֵ��ȡtimes��,Ȼ��ƽ�� 
//ch:ͨ�����
//times:��ȡ����
//����ֵ:ͨ��ch��times��ת�����ƽ��ֵ
static u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);//��ȡͨ��ת��ֵ
		rt_thread_delay(1);
	}
	return temp_val/times;
} 

//��ȡ
u32 enc_get(void)
{
	return ENC_TIMx->CNT;
}

//��ȡ
u16 enc_GetAdc(void)
{
	return Get_Adc_Average(ENC_ADC_CH,10);	//��ȡͨ��16�ڲ��¶ȴ�����ͨ��,10��ȡƽ��
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


//��ʼ��
void enc_Init(void)
{

	gpio_Config();
	tim_Config();
	adc_Config();
	ADC_WatchdogConfig();
	NVIC_Config();
	ADC_SoftwareStartConv(ENC_ADCx);		//ʹ��ָ����ENC_ADCx�����ת����������
}


