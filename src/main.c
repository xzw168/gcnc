



#include "main.h"




static void rcc_Init(void)
{
	RCC_APB2PeriphClockCmd(
	RCC_APB2Periph_SYSCFG|
	RCC_APB2Periph_USART1|
	RCC_APB2Periph_ADC1|
	RCC_APB2Periph_ADC2|
	RCC_APB2Periph_ADC3, ENABLE);  
	
	RCC_AHB1PeriphClockCmd(
	RCC_AHB1Periph_GPIOA|
	RCC_AHB1Periph_GPIOB|
	RCC_AHB1Periph_GPIOC|
	RCC_AHB1Periph_GPIOD|
	RCC_AHB1Periph_GPIOE|
	RCC_AHB1Periph_DMA1 |
	RCC_AHB1Periph_DMA2, ENABLE);
	
	RCC_APB1PeriphClockCmd(
	RCC_AHB1Periph_DMA1|
	RCC_AHB1Periph_DMA2|
	RCC_APB1Periph_TIM2|
	RCC_APB1Periph_TIM3|
	RCC_APB1Periph_TIM4|
	RCC_APB1Periph_TIM5|
	RCC_APB1Periph_TIM6|
	RCC_APB1Periph_TIM7|
	RCC_APB1Periph_TIM12|
	RCC_APB1Periph_TIM13|
	RCC_APB1Periph_TIM14, ENABLE);
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);
	
	
	
}

void ting_main_Init(void);
void ting_Loop(void);


int main(void)
{
	rcc_Init();
	xuart_Init();
	xyz_io_init();
	//xio_tim_Init();
    //ting_main_Init();
	
	// SystemFrequency / 1000    1ms中断一次
	// SystemFrequency / 100000	 10us中断一次
	// SystemFrequency / 1000000 1us中断一次
	if (SysTick_Config(SystemCoreClock / 1000))
	{ 
		/* Capture error */ 
		while (1);
	}
	
	while(1){
		ting_Loop();
	}
	
    //return 0;
}


void assert_failed(u8* file, u32 line)
{
    while (1) ;
}




