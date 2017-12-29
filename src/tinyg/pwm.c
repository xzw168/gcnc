

#include "tinyg.h"		// #1
#include "config.h"		// #2
#include "hardware.h"
#include "text_parser.h"
#include "gpio.h"
#include "pwm.h"



pwmSingleton_t pwm;




void pwm_init()
{

}



stat_t pwm_set_freq(uint8_t chan, float freq)
{


	return (STAT_OK);
}



stat_t pwm_set_duty(uint8_t chan, float duty)
{


	return (STAT_OK);
}


/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 ***********************************************************************************/

// none


/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

static const char fmt_p1frq[] PROGMEM = "[p1frq] pwm frequency   %15.0f Hz\n";
static const char fmt_p1csl[] PROGMEM = "[p1csl] pwm cw speed lo %15.0f RPM\n";
static const char fmt_p1csh[] PROGMEM = "[p1csh] pwm cw speed hi %15.0f RPM\n";
static const char fmt_p1cpl[] PROGMEM = "[p1cpl] pwm cw phase lo %15.3f [0..1]\n";
static const char fmt_p1cph[] PROGMEM = "[p1cph] pwm cw phase hi %15.3f [0..1]\n";
static const char fmt_p1wsl[] PROGMEM = "[p1wsl] pwm ccw speed lo%15.0f RPM\n";
static const char fmt_p1wsh[] PROGMEM = "[p1wsh] pwm ccw speed hi%15.0f RPM\n";
static const char fmt_p1wpl[] PROGMEM = "[p1wpl] pwm ccw phase lo%15.3f [0..1]\n";
static const char fmt_p1wph[] PROGMEM = "[p1wph] pwm ccw phase hi%15.3f [0..1]\n";
static const char fmt_p1pof[] PROGMEM = "[p1pof] pwm phase off   %15.3f [0..1]\n";

void pwm_print_p1frq(nvObj_t *nv) { text_print_flt(nv, fmt_p1frq);}
void pwm_print_p1csl(nvObj_t *nv) { text_print_flt(nv, fmt_p1csl);}
void pwm_print_p1csh(nvObj_t *nv) { text_print_flt(nv, fmt_p1csh);}
void pwm_print_p1cpl(nvObj_t *nv) { text_print_flt(nv, fmt_p1cpl);}
void pwm_print_p1cph(nvObj_t *nv) { text_print_flt(nv, fmt_p1cph);}
void pwm_print_p1wsl(nvObj_t *nv) { text_print_flt(nv, fmt_p1wsl);}
void pwm_print_p1wsh(nvObj_t *nv) { text_print_flt(nv, fmt_p1wsh);}
void pwm_print_p1wpl(nvObj_t *nv) { text_print_flt(nv, fmt_p1wpl);}
void pwm_print_p1wph(nvObj_t *nv) { text_print_flt(nv, fmt_p1wph);}
void pwm_print_p1pof(nvObj_t *nv) { text_print_flt(nv, fmt_p1pof);}

#endif //__TEXT_MODE
