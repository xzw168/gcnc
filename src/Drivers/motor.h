

#ifndef __MOTOR_H
#define __MOTOR_H

void xyz_io_Clk_H(uint8_t axis);
void xyz_io_Clk_L(uint8_t axis);

#define MOTOR_1_CLK_H()		xyz_io_Clk_H(1)
#define MOTOR_1_CLK_L()		xyz_io_Clk_L(1)
#define MOTOR_2_CLK_H()		xyz_io_Clk_H(4)
#define MOTOR_2_CLK_L()		xyz_io_Clk_L(4)
#define MOTOR_3_CLK_H()		xyz_io_Clk_H(3)
#define MOTOR_3_CLK_L()		xyz_io_Clk_L(3)
#define MOTOR_4_CLK_H()		xyz_io_Clk_H(2)
#define MOTOR_4_CLK_L()		xyz_io_Clk_L(2)
#define MOTOR_5_CLK_H()		xyz_io_Clk_H(5)
#define MOTOR_5_CLK_L()		xyz_io_Clk_L(5)
#define MOTOR_6_CLK_H()	
#define MOTOR_6_CLK_L()

#define MOTOR_1_DIR_H()		xyz_io_Dir_H(1)
#define MOTOR_1_DIR_L()		xyz_io_Dir_L(1)
#define MOTOR_2_DIR_H()		xyz_io_Dir_H(4)
#define MOTOR_2_DIR_L()		xyz_io_Dir_L(4)
#define MOTOR_3_DIR_H()	    xyz_io_Dir_H(3)
#define MOTOR_3_DIR_L()		xyz_io_Dir_L(3)
#define MOTOR_4_DIR_H()		xyz_io_Dir_H(2)
#define MOTOR_4_DIR_L()		xyz_io_Dir_L(2)
#define MOTOR_5_DIR_H()		xyz_io_Dir_H(5)
#define MOTOR_5_DIR_L()		xyz_io_Dir_L(5)
#define MOTOR_6_DIR_H()		
#define MOTOR_6_DIR_L()

#define MOTOR_1_EN_ENABLE()
#define MOTOR_1_EN_DISABLE()
#define MOTOR_2_EN_ENABLE()
#define MOTOR_2_EN_DISABLE()
#define MOTOR_3_EN_ENABLE()
#define MOTOR_3_EN_DISABLE()
#define MOTOR_4_EN_ENABLE()
#define MOTOR_4_EN_DISABLE()
#define MOTOR_5_EN_ENABLE()
#define MOTOR_5_EN_DISABLE()
#define MOTOR_6_EN_ENABLE()
#define MOTOR_6_EN_DISABLE()

/*
#define TIMER_DDA_PER()
#define TIMER_EXEC_PER()
#define TIMER_LOAD_PER()
#define TIMER_DWELL_PER()
//#define TIMER_DDA_DISABLE() 
//#define TIMER_DDA_ENABLE()
//#define TIMER_EXEC_DISABLE() 
//#define TIMER_EXEC_ENABLE()
//#define TIMER_LOAD_DISABLE() 
//#define TIMER_LOAD_ENABLE()
//#define TIMER_DWELL_DISABLE() 
//#define TIMER_DWELL_ENABLE()

void TIMER_DDA_DISABLE(void);
void TIMER_DDA_ENABLE(void);
void TIMER_EXEC_DISABLE(void);
void TIMER_EXEC_ENABLE(void);
void TIMER_LOAD_DISABLE(void);
void TIMER_LOAD_ENABLE(void);
void TIMER_DWELL_DISABLE(void);
void TIMER_DWELL_ENABLE(void);
*/
#endif
