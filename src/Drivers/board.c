/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      first implementation
 */


#include "main.h"
#include "board.h"


/**
 * @addtogroup STM32
 */

/*@{*/

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void  SysTick_Configuration(void)
{
    RCC_ClocksTypeDef  rcc_clocks;
    rt_uint32_t         cnts;

    RCC_GetClocksFreq(&rcc_clocks);

    cnts = (rt_uint32_t)rcc_clocks.HCLK_Frequency / RT_TICK_PER_SECOND;
    cnts = cnts / 8;

    SysTick_Config(cnts);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

static void rcc_Init(void)
{
	RCC_APB2PeriphClockCmd(
	RCC_APB2Periph_SYSCFG|
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

/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
	rcc_Init();
	xyz_io_init();
	//adc_temp_Init();
	//enc_Init();
	led_Init();
	
	
    /* NVIC Configuration */
    NVIC_Configuration();

    /* Configure the SysTick */
    SysTick_Configuration();

    //stm32_hw_usart_init();
    //stm32_hw_pin_init();
    
#ifdef RT_USING_CONSOLE
    rt_console_set_device(CONSOLE_DEVICE);
#endif
}

/*@}*/
