
#ifndef __ADC_TEMP_H
#define __ADC_TEMP_H




//初始化
void adc_temp_Init(void);
//得到温度值
//返回值:温度值(扩大了100倍,单位:℃.)
short adc_temp_Get(void);

#endif

