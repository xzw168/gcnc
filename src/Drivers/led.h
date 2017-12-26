

#ifndef __LED_H
#define __LED_H


#define LED_GPIOX       	GPIOB
#define LED_PIN         	GPIO_Pin_15

#define LED_H()		   	LED_GPIOX->BSRRL = LED_PIN
#define LED_L()   		LED_GPIOX->BSRRH = LED_PIN


//≥ı ºªØ
void led_Init(void);

#endif
