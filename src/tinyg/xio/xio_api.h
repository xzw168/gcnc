
#ifndef xio_api_h
#define xio_api_h


#include "xio_motor.h"
#include "xio_tim.h"
#include "xio_sw.h"


#define M_PI       3.14159265358979323846   // pi


#define strncpy_P(d,s,l) (char_t *)strncpy((char *)d, (char *)s, l)
#define strcpy_P(d,s) (char_t *)strcpy((char *)d, (char *)s)
#define strcmp_P(d,s) strcmp((char *)d, (char *)s)

#define printf_P(...)  printf(__VA_ARGS__)		
//#define fprintf_P(f,...) printf(__VA_ARGS__)
#define sprintf_P sprintf

#define GET_TOKEN_BYTE(a) (char)cfgArray[i].a	    
#define GET_TABLE_WORD(a)  cfgArray[nv->index].a	
#define GET_TABLE_FLOAT(a) cfgArray[nv->index].a	


#define GET_TEXT_ITEM(b,a) b[a]						
#define GET_UNITS(a) msg_units[cm_get_units_mode(a)]	




#endif
