
#include "main.h"


u16  xyz_io_buf[1024];

const XYZ_IO_TB_Tab_t xyz_io_TB_Tab={ 

TB_CLK1_PIN,TB_CLK2_PIN,TB_CLK3_PIN,TB_CLK4_PIN,TB_CLK5_PIN,
TB_DIR1_PIN,TB_DIR2_PIN,TB_DIR3_PIN,TB_DIR4_PIN,TB_DIR5_PIN,
	
TB_MA1_GPIOX,TB_MB1_GPIOX,TB_MC1_GPIOX,
TB_MA2_GPIOX,TB_MB2_GPIOX,TB_MC2_GPIOX,
TB_MA3_GPIOX,TB_MB3_GPIOX,TB_MC3_GPIOX,
TB_MA4_GPIOX,TB_MB4_GPIOX,TB_MC4_GPIOX,
TB_MA5_GPIOX,TB_MB5_GPIOX,TB_MC5_GPIOX, 
	
TB_MA1_PIN,TB_MB1_PIN,TB_MC1_PIN,
TB_MA2_PIN,TB_MB2_PIN,TB_MC2_PIN,
TB_MA3_PIN,TB_MB3_PIN,TB_MC3_PIN,
TB_MA4_PIN,TB_MB4_PIN,TB_MC4_PIN,
TB_MA5_PIN,TB_MB5_PIN,TB_MC5_PIN,
	
TB_TQ1_GPIOX,TB_TQ2_GPIOX,TB_TQ3_GPIOX,TB_TQ4_GPIOX,TB_TQ5_GPIOX,
TB_TQ1_PIN,TB_TQ2_PIN,TB_TQ3_PIN,TB_TQ4_PIN,TB_TQ5_PIN,	

};
	
static void dma_Config(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;	
	DMA_InitTypeDef  DMA_InitStructure;
	
	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = 84-1; //84Mhz 预分频器
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 1000000/1000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
	
		/* Channel 1, 2,3 and 4 Configuration in PWM mode */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable ;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_Pulse = 0;

	TIM_OC1Init(TIM5, &TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_Pulse = TIM_TimeBaseStructure.TIM_Period/2;
	TIM_OC2Init(TIM5, &TIM_OCInitStructure);
	
	TIM_DMACmd(TIM5, TIM_DMA_CC1,ENABLE);
	TIM_DMACmd(TIM5, TIM_DMA_CC2,ENABLE);
	
	
	  /* 配置 DMA Stream */
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  //通道选择
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)TB_CLK_DIR_GPIOX->BSRRL;//DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)xyz_io_buf;//DMA 存储器0地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//存储器到外设模式
	DMA_InitStructure.DMA_BufferSize = 1024/2;//数据传输量 
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度:8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//存储器数据长度:8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;// 使用循环模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;//中等优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//外设突发单次传输
	DMA_Init(DMA2_Stream7, &DMA_InitStructure);//初始化DMA Stream
  
	
	TIM_Cmd(TIM5, ENABLE);

	//while(1);
}

static void gpio_Config(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	TB_EN_L();
	TB_CLK_DIR_GPIOX->BSRRL=0x3FF;
	//输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//GPIO_OType_PP GPIO_OType_OD
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;;//GPIO_PuPd_UP GPIO_PuPd_NOPULL;
	
	//CLK0-5,DIR0-5
	GPIO_InitStructure.GPIO_Pin = 0x3FF;
	GPIO_Init(TB_CLK_DIR_GPIOX, &GPIO_InitStructure);

	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//GPIO_OType_PP GPIO_OType_OD
	//M2,M4
	GPIO_InitStructure.GPIO_Pin = 0xFC00;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	//M1,M3,TQ5,TQ3
	GPIO_InitStructure.GPIO_Pin = 0x00FF;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	//M5
	GPIO_InitStructure.GPIO_Pin = TB_MA5_PIN|TB_MB5_PIN|TB_MC5_PIN;
	GPIO_Init(TB_MA5_GPIOX, &GPIO_InitStructure);
	//TQ4
	GPIO_InitStructure.GPIO_Pin = TB_TQ4_PIN;
	GPIO_Init(TB_TQ4_GPIOX, &GPIO_InitStructure);
	//TQ1,TQ2,EN
	GPIO_InitStructure.GPIO_Pin = TB_TQ1_PIN|TB_TQ2_PIN|TB_EN_PIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//PWM
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = TB_PWM1_PIN|TB_PWM2_PIN|TB_PWM3_PIN|TB_PWM4_PIN|TB_PWM5_PIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(TB_PWM1_GPIOX, TB_PWM1_SOURCE , GPIO_AF_TIM4);
	GPIO_PinAFConfig(TB_PWM2_GPIOX, TB_PWM2_SOURCE , GPIO_AF_TIM3);
	GPIO_PinAFConfig(TB_PWM3_GPIOX, TB_PWM3_SOURCE , GPIO_AF_TIM3);
	GPIO_PinAFConfig(TB_PWM4_GPIOX, TB_PWM4_SOURCE , GPIO_AF_TIM3);
	GPIO_PinAFConfig(TB_PWM5_GPIOX, TB_PWM5_SOURCE , GPIO_AF_TIM3);
	
}

static void pwm_Config(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;	
	
	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = 0; //84Mhz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 4096-1;//16bit
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	/* Channel 1, 2,3 and 4 Configuration in PWM mode */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable ;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_Pulse = 440;

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	
	/* TIM1 counter enable */
	TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
	
	/* TIM1 Main Output Enable */
	//TIM_CtrlPWMOutputs(TIM1, ENABLE);
}


//设置电流
void xyz_io_SetCurrent(int axis,int ma)
{
//Io (100%) = (1/3 × Vref) ÷ RNF ; 0.11Ω ≤ RNF ≤ 0.5Ω, 0.3V ≤ Vref ≤ 1.95V
	float v=ma/1000.0*0.125/(1.0/3.0);
	u32 tim1=v/(3.32/4096);	
	u16 tim=tim1;
	if(axis==1)
		TIM4->CCR2=tim;
	else if(axis==2)
		TIM3->CCR3=tim;
	else if(axis==3)
		TIM3->CCR2=tim;
	else if(axis==4)
		TIM3->CCR4=tim;
	else if(axis==5)
		TIM3->CCR1=tim;
}
//设置TQ 30% 100%
void xyz_io_SetTQ(int axis,int v)
{
	axis--;
	if (v == 30)
	{
		xyz_io_TB_Tab.TQ_Gpio[axis]->BSRRH=xyz_io_TB_Tab.TQ_Pin[axis];
	}
	else
	{
		xyz_io_TB_Tab.TQ_Gpio[axis]->BSRRL=xyz_io_TB_Tab.TQ_Pin[axis];
	}
	
	
}
//设置细分
void xyz_io_SetSubdivided(int axis,Subdivided_e s)
{
	axis--;
	xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRH=xyz_io_TB_Tab.M_Pin[axis][0];  
	xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRH=xyz_io_TB_Tab.M_Pin[axis][1];
	xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRH=xyz_io_TB_Tab.M_Pin[axis][2];
	
	switch (s){
		case Subdivided_1_1:
			//xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			//xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		case Subdivided_1_2a:
			//xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			//xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		case Subdivided_1_2b:
			//xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		case Subdivided_1_4:
			xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			//xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			//xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		case Subdivided_1_8:
			xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			//xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		case Subdivided_1_16:
			xyz_io_TB_Tab.M_Gpio[axis][0]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][0];  
			xyz_io_TB_Tab.M_Gpio[axis][1]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][1];
			//xyz_io_TB_Tab.M_Gpio[axis][2]->BSRRL=xyz_io_TB_Tab.M_Pin[axis][2];
		break;
		default:
		break;
		
	}
}

//初始化
void xyz_io_init(void)
{
	
	gpio_Config();
	pwm_Config();
	//dma_Config();
	
	xyz_io_SetCurrent(1,1000);
	xyz_io_SetCurrent(2,1000);
	xyz_io_SetCurrent(3,1000);
	xyz_io_SetCurrent(4,3000);
	xyz_io_SetCurrent(5,1000);
	
	xyz_io_SetSubdivided(1,Subdivided_1_16);
	xyz_io_SetSubdivided(2,Subdivided_1_8);
	xyz_io_SetSubdivided(3,Subdivided_1_8);	
	xyz_io_SetSubdivided(4,Subdivided_1_8);
	xyz_io_SetSubdivided(5,Subdivided_1_8);
	
	xyz_io_SetTQ(1,100);
	xyz_io_SetTQ(2,100);
	xyz_io_SetTQ(3,100);
	xyz_io_SetTQ(4,100);
	xyz_io_SetTQ(5,100);
	
	TB_DIR_ALL_L();
	TB_EN_H();
	TB_CLK_ALL_H();

	
	
	
}
void xyz_io_Clk_H(uint8_t axis)
{
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);
	TB_CLK_H(axis);

}
void xyz_io_Clk_L(uint8_t axis)
{
	TB_CLK_L(axis);
	TB_CLK_L(axis);
}
void xyz_io_Dir_H(uint8_t axis)
{
	TB_DIR_H(axis);
}
void xyz_io_Dir_L(uint8_t axis)
{
	TB_DIR_L(axis);
}

