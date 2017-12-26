

#ifndef __ENC_H
#define __ENC_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
	 
//���Ŷ���
/*******************************************************/

#define ENC_A_PIN                  GPIO_Pin_0
#define ENC_A_GPIO_PORT            GPIOA
#define ENC_A_SOURCE               GPIO_PinSource0
#define ENC_A_AF           		   GPIO_AF_TIM2
#define ENC_B_PIN                  GPIO_Pin_3 
#define ENC_B_GPIO_PORT            GPIOB
#define ENC_B_SOURCE               GPIO_PinSource3 
#define ENC_B_AF           		   GPIO_AF_TIM2
#define ENC_ADC_PIN                GPIO_Pin_3
#define ENC_ADC_GPIO_PORT          GPIOA
#define ENC_ADC_CH                 ADC_Channel_3
#define ENC_ADCx                   ADC3
#define ENC_ADC_SOURCE             GPIO_PinSource3
#define ENC_ADC_AF           	   GPIO_AF_TIM2
/************************************************************/
	 
#define ENC_TIMx    TIM2	

	 
//��ʼ��
void enc_Init(void);
//��ȡ
u32 enc_get(void);	 
//��ȡ
u16 enc_GetAdc(void);

#ifdef __cplusplus
}
#endif

#endif

