
#include "main.h"


//³õÊ¼»¯
void led_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	
		//Êä³ö
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//GPIO_OType_PP GPIO_OType_OD
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;;//GPIO_PuPd_UP GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_Init(LED_GPIOX, &GPIO_InitStructure);
	
}
