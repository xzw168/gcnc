
#ifndef __TB__IO_H
#define __TB__IO_H

#define XYZ_IO_AXIS_NUM     5 //总轴数

#define TB_CLK_DIR_GPIOX    GPIOE
#define TB_CLK1_PIN   		GPIO_Pin_3
#define TB_DIR1_PIN   		GPIO_Pin_5
#define TB_CLK2_PIN   		GPIO_Pin_8
#define TB_DIR2_PIN   		GPIO_Pin_7
#define TB_CLK3_PIN   		GPIO_Pin_4
#define TB_DIR3_PIN   		GPIO_Pin_6
#define TB_CLK4_PIN   		GPIO_Pin_2
#define TB_DIR4_PIN   		GPIO_Pin_9
#define TB_CLK5_PIN   		GPIO_Pin_0
#define TB_DIR5_PIN   		GPIO_Pin_1



#define TB_PWM1_GPIOX       GPIOB
#define TB_PWM1_SOURCE      GPIO_PinSource7
#define TB_PWM1_PIN         GPIO_Pin_7
#define TB_PWM2_GPIOX       GPIOB
#define TB_PWM2_SOURCE      GPIO_PinSource0
#define TB_PWM2_PIN         GPIO_Pin_0
#define TB_PWM3_GPIOX       GPIOB
#define TB_PWM3_SOURCE      GPIO_PinSource5
#define TB_PWM3_PIN         GPIO_Pin_5
#define TB_PWM4_GPIOX       GPIOB
#define TB_PWM4_SOURCE      GPIO_PinSource1
#define TB_PWM4_PIN         GPIO_Pin_1
#define TB_PWM5_GPIOX       GPIOB
#define TB_PWM5_SOURCE      GPIO_PinSource4
#define TB_PWM5_PIN         GPIO_Pin_4

#define TB_TQ1_GPIOX       	GPIOB
#define TB_TQ1_PIN         	GPIO_Pin_6
#define TB_TQ2_GPIOX       	GPIOB
#define TB_TQ2_PIN         	GPIO_Pin_2
#define TB_TQ3_GPIOX       	GPIOD
#define TB_TQ3_PIN         	GPIO_Pin_4
#define TB_TQ4_GPIOX       	GPIOA
#define TB_TQ4_PIN         	GPIO_Pin_6
#define TB_TQ5_GPIOX       	GPIOD
#define TB_TQ5_PIN         	GPIO_Pin_0

#define TB_MA1_GPIOX       	GPIOD
#define TB_MA1_PIN         	GPIO_Pin_7
#define TB_MB1_GPIOX       	GPIOD
#define TB_MB1_PIN         	GPIO_Pin_6
#define TB_MC1_GPIOX       	GPIOD
#define TB_MC1_PIN         	GPIO_Pin_5
#define TB_MA2_GPIOX       	GPIOE
#define TB_MA2_PIN         	GPIO_Pin_12
#define TB_MB2_GPIOX       	GPIOE
#define TB_MB2_PIN         	GPIO_Pin_11
#define TB_MC2_GPIOX       	GPIOE
#define TB_MC2_PIN         	GPIO_Pin_10
#define TB_MA3_GPIOX       	GPIOD
#define TB_MA3_PIN         	GPIO_Pin_3
#define TB_MB3_GPIOX       	GPIOD
#define TB_MB3_PIN         	GPIO_Pin_2
#define TB_MC3_GPIOX       	GPIOD
#define TB_MC3_PIN         	GPIO_Pin_1
#define TB_MA4_GPIOX       	GPIOE
#define TB_MA4_PIN         	GPIO_Pin_15
#define TB_MB4_GPIOX       	GPIOE
#define TB_MB4_PIN         	GPIO_Pin_14
#define TB_MC4_GPIOX       	GPIOE
#define TB_MC4_PIN         	GPIO_Pin_13
#define TB_MA5_GPIOX       	GPIOC
#define TB_MA5_PIN         	GPIO_Pin_12
#define TB_MB5_GPIOX       	GPIOC
#define TB_MB5_PIN         	GPIO_Pin_11
#define TB_MC5_GPIOX       	GPIOC
#define TB_MC5_PIN         	GPIO_Pin_10

#define TB_EN_GPIOX       	GPIOB
#define TB_EN_PIN         	GPIO_Pin_8

//#define ETH_RES_GPIOX       GPIOB
//#define ETH_RES_PIN         GPIO_Pin_14
#define ETH_LED_GPIOX       GPIOB
#define ETH_LED_PIN         GPIO_Pin_15

#define TB_EN_H()		    TB_EN_GPIOX->BSRRL = TB_EN_PIN
#define TB_EN_L()   		TB_EN_GPIOX->BSRRH = TB_EN_PIN

#define TB_CLK1_H()		   	TB_CLK_DIR_GPIOX->BSRRL = TB_CLK1_PIN
#define TB_CLK1_L()   		TB_CLK_DIR_GPIOX->BSRRH = TB_CLK1_PIN
#define TB_DIR1_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_DIR1_PIN
#define TB_DIR1_L()   		TB_CLK_DIR_GPIOX->BSRRH = TB_DIR1_PIN
#define TB_CLK2_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_CLK2_PIN
#define TB_CLK2_L()   		TB_CLK_DIR_GPIOX->BSRRH = TB_CLK2_PIN
#define TB_DIR2_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_DIR2_PIN
#define TB_DIR2_L() 		TB_CLK_DIR_GPIOX->BSRRH = TB_DIR2_PIN
#define TB_CLK3_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_CLK3_PIN
#define TB_CLK3_L() 		TB_CLK_DIR_GPIOX->BSRRH = TB_CLK3_PIN
#define TB_DIR3_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_DIR3_PIN
#define TB_DIR3_L()  		TB_CLK_DIR_GPIOX->BSRRH = TB_DIR3_PIN
#define TB_CLK4_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_CLK4_PIN
#define TB_CLK4_L()  		TB_CLK_DIR_GPIOX->BSRRH = TB_CLK4_PIN
#define TB_DIR4_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_DIR4_PIN
#define TB_DIR4_L() 		TB_CLK_DIR_GPIOX->BSRRH = TB_DIR4_PIN
#define TB_CLK5_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_CLK5_PIN
#define TB_CLK5_L()  		TB_CLK_DIR_GPIOX->BSRRH = TB_CLK5_PIN
#define TB_DIR5_H()        	TB_CLK_DIR_GPIOX->BSRRL = TB_DIR5_PIN
#define TB_DIR5_L()   		TB_CLK_DIR_GPIOX->BSRRH = TB_DIR5_PIN

#define TB_1_M1_H()         TB_MA1_GPIOX->BSRRL = TB_MA1_PIN
#define TB_1_M1_L()         TB_MA1_GPIOX->BSRRH = TB_MA1_PIN
#define TB_1_M2_H()         TB_MB1_GPIOX->BSRRL = TB_MB1_PIN
#define TB_1_M2_L()         TB_MB1_GPIOX->BSRRH = TB_MB1_PIN
#define TB_1_M3_H()         TB_MC1_GPIOX->BSRRL = TB_MC1_PIN
#define TB_1_M3_L()         TB_MC1_GPIOX->BSRRH = TB_MC1_PIN
#define TB_1_TQ_H()   	    TB_TQ1_GPIOX->BSRRL = TB_TQ1_PIN	
#define TB_1_TQ_L()         TB_TQ1_GPIOX->BSRRH = TB_TQ1_PIN
#define TB_2_M1_H()         TB_MA3_GPIOX->BSRRL = TB_MA3_PIN
#define TB_2_M1_L()         TB_MA3_GPIOX->BSRRH = TB_MA3_PIN
#define TB_2_M2_H()         TB_MB3_GPIOX->BSRRL = TB_MB3_PIN
#define TB_2_M2_L()         TB_MB3_GPIOX->BSRRH = TB_MB3_PIN
#define TB_2_M3_H()         TB_MC3_GPIOX->BSRRL = TB_MC3_PIN
#define TB_2_M3_L()         TB_MC3_GPIOX->BSRRH = TB_MC3_PIN
#define TB_2_TQ_H()   	    TB_TQ3_GPIOX->BSRRL = TB_TQ3_PIN	
#define TB_2_TQ_L()         TB_TQ3_GPIOX->BSRRH = TB_TQ3_PIN
#define TB_3_M1_H()         TB_MA5_GPIOX->BSRRL = TB_MA5_PIN
#define TB_3_M1_L()         TB_MA5_GPIOX->BSRRH = TB_MA5_PIN
#define TB_3_M2_H()         TB_MB5_GPIOX->BSRRL = TB_MB5_PIN
#define TB_3_M2_L()         TB_MB5_GPIOX->BSRRH = TB_MB5_PIN
#define TB_3_M3_H()         TB_MC5_GPIOX->BSRRL = TB_MC5_PIN
#define TB_3_M3_L()         TB_MC5_GPIOX->BSRRH = TB_MC5_PIN
#define TB_3_TQ_H()   	    TB_TQ5_GPIOX->BSRRL = TB_TQ5_PIN	
#define TB_3_TQ_L()         TB_TQ5_GPIOX->BSRRH = TB_TQ5_PIN
#define TB_4_M1_H()         TB_MA2_GPIOX->BSRRL = TB_MA2_PIN
#define TB_4_M1_L()         TB_MA2_GPIOX->BSRRH = TB_MA2_PIN
#define TB_4_M2_H()         TB_MB2_GPIOX->BSRRL = TB_MB2_PIN
#define TB_4_M2_L()         TB_MB2_GPIOX->BSRRH = TB_MB2_PIN
#define TB_4_M3_H()         TB_MC2_GPIOX->BSRRL = TB_MC2_PIN
#define TB_4_M3_L()         TB_MC2_GPIOX->BSRRH = TB_MC2_PIN
#define TB_4_TQ_H()   	    TB_TQ2_GPIOX->BSRRL = TB_TQ2_PIN	
#define TB_4_TQ_L()         TB_TQ2_GPIOX->BSRRH = TB_TQ2_PIN
#define TB_5_M1_H()         TB_MA4_GPIOX->BSRRL = TB_MA4_PIN
#define TB_5_M1_L()         TB_MA4_GPIOX->BSRRH = TB_MA4_PIN
#define TB_5_M2_H()         TB_MB4_GPIOX->BSRRL = TB_MB4_PIN
#define TB_5_M2_L()         TB_MB4_GPIOX->BSRRH = TB_MB4_PIN
#define TB_5_M3_H()         TB_MC4_GPIOX->BSRRL = TB_MC4_PIN
#define TB_5_M3_L()         TB_MC4_GPIOX->BSRRH = TB_MC4_PIN
#define TB_5_TQ_H()   	    TB_TQ4_GPIOX->BSRRL = TB_TQ4_PIN	
#define TB_5_TQ_L()         TB_TQ4_GPIOX->BSRRH = TB_TQ4_PIN

//#define ETH_RES_H()		    ETH_RES_GPIOX->BSRRL = ETH_RES_PIN
//#define ETH_RES_L()   		ETH_RES_GPIOX->BSRRH = ETH_RES_PIN
#define ETH_LED_H()		    ETH_LED_GPIOX->BSRRL = ETH_LED_PIN
#define ETH_LED_L()   		ETH_LED_GPIOX->BSRRH = ETH_LED_PIN


#define  TB_CLK_H(axis)     TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[axis-1]
#define  TB_CLK_L(axis)     TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[axis-1]
#define  TB_CLK_ALL_H()     TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[0];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[1];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[2];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[3];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.CLK_Pin[4]
#define  TB_CLK_ALL_L()     TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[0];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[1];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[2];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[3];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.CLK_Pin[4]
#define  TB_DIR_H(axis)     TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[axis-1]
#define  TB_DIR_L(axis)     TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[axis-1]
#define  TB_DIR_ALL_H()     TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[0];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[1];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[2];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[3];TB_CLK_DIR_GPIOX->BSRRL = xyz_io_TB_Tab.DIR_Pin[4]
#define  TB_DIR_ALL_L()     TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[0];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[1];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[2];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[3];TB_CLK_DIR_GPIOX->BSRRH = xyz_io_TB_Tab.DIR_Pin[4]

typedef enum {
	Subdivided_1_1=1,
    Subdivided_1_2a,
    Subdivided_1_2b,
	Subdivided_1_4,
    Subdivided_1_8,
    Subdivided_1_16,
}Subdivided_e;

typedef struct
{
	
  //GPIO_TypeDef *CLK_Gpio[XYZ_IO_AXIS_NUM];
  u32 	CLK_Pin[XYZ_IO_AXIS_NUM];
	
  //GPIO_TypeDef *DIR_Gpio[XYZ_IO_AXIS_NUM];
  u32 	DIR_Pin[XYZ_IO_AXIS_NUM];
	
  GPIO_TypeDef *M_Gpio[XYZ_IO_AXIS_NUM][3];
  u32 	M_Pin[XYZ_IO_AXIS_NUM][3];
  
  GPIO_TypeDef *TQ_Gpio[XYZ_IO_AXIS_NUM];
  u32 	TQ_Pin[XYZ_IO_AXIS_NUM];
	
} XYZ_IO_TB_Tab_t;


extern const XYZ_IO_TB_Tab_t xyz_io_TB_Tab;

//初始化
void xyz_io_init(void);


#endif

