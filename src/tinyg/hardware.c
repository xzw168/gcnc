

#include "tinyg.h"		// #1
#include "config.h"		// #2
#include "hardware.h"
#include "switch.h"
#include "controller.h"
#include "text_parser.h"




static void _port_bindings(float hw_version)
{

}

void hardware_init()
{

}



enum {
	LOTNUM0=8,  // Lot Number Byte 0, ASCII
	LOTNUM1,    // Lot Number Byte 1, ASCII
	LOTNUM2,    // Lot Number Byte 2, ASCII
	LOTNUM3,    // Lot Number Byte 3, ASCII
	LOTNUM4,    // Lot Number Byte 4, ASCII
	LOTNUM5,    // Lot Number Byte 5, ASCII
	WAFNUM =16, // Wafer Number
	COORDX0=18, // Wafer Coordinate X Byte 0
	COORDX1,    // Wafer Coordinate X Byte 1
	COORDY0,    // Wafer Coordinate Y Byte 0
	COORDY1,    // Wafer Coordinate Y Byte 1
};

static void _get_id(char *id)
{
#ifdef __AVR //xzw169
	char printable[33]; strcpy_P(printable, PSTR("ABCDEFGHJKLMNPQRSTUVWXYZ23456789"));
	uint8_t i;

	//NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc; 	// Load NVM Command register to read the calibration row

	for (i=0; i<6; i++) {
		id[i] = printable[i];
	}
	id[i++] = '-';
	id[i++] = printable[1];
	id[i++] = printable[2];
	id[i++] = printable[3];
	id[i] = 0;

	//NVM_CMD = NVM_CMD_NO_OPERATION_gc; 	 	// Clean up NVM Command register
#endif
}

/*
 * Hardware Reset Handlers
 *
 * hw_request_hard_reset()
 * hw_hard_reset()			- hard reset using watchdog timer
 * hw_hard_reset_handler()	- controller's rest handler
 */
void hw_request_hard_reset() { cs.hard_reset_requested = true; }

void hw_hard_reset(void)			// software hard reset using the watchdog timer xzw168
{
#ifdef __AVR1
	wdt_enable(WDTO_15MS);
	while (true);					// loops for about 15ms then resets
#endif
}

stat_t hw_hard_reset_handler(void)
{
	if (cs.hard_reset_requested == false) {
        return (STAT_NOOP);
    }
	hw_hard_reset();				// hard reset - identical to hitting RESET button
	return (STAT_EAGAIN);
}

/*
 * Bootloader Handlers
 *
 * hw_request_bootloader()
 * hw_request_bootloader_handler() - executes a software reset using CCPWrite
 */

void hw_request_bootloader() { cs.bootloader_requested = true;}

stat_t hw_bootloader_handler(void)
{
	return (STAT_NOOP);				// never gets here but keeps the compiler happy
}

/***** END OF SYSTEM FUNCTIONS *****/


/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 ***********************************************************************************/

/*
 * hw_get_id() - get device ID (signature)
 */

stat_t hw_get_id(nvObj_t *nv)
{
	char tmp[SYS_ID_LEN];
	_get_id(tmp);
	nv->valuetype = TYPE_STRING;
	ritorno(nv_copy_string(nv, tmp));
	return (STAT_OK);
}

/*
 * hw_run_boot() - invoke boot form the cfgArray
 */
stat_t hw_run_boot(nvObj_t *nv)
{
	hw_request_bootloader();
	return(STAT_OK);
}

/*
 * hw_set_hv() - set hardware version number
 */
stat_t hw_set_hv(nvObj_t *nv)
{
	if (nv->value_flt > TINYG_HARDWARE_VERSION_MAX) {
        return (STAT_INPUT_EXCEEDS_MAX_VALUE);
    }
	set_flt(nv);					// record the hardware version
	_port_bindings(nv->value_flt);	// reset port bindings
	switch_init();					// re-initialize the GPIO ports
	return (STAT_OK);
}

/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

static const char fmt_fb[] PROGMEM = "[fb]  firmware build%18.2f\n";
static const char fmt_fv[] PROGMEM = "[fv]  firmware version%16.2f\n";
static const char fmt_hp[] PROGMEM = "[hp]  hardware platform%12d\n";
static const char fmt_hv[] PROGMEM = "[hv]  hardware version%16.2f\n";
static const char fmt_id[] PROGMEM = "[id]  TinyG ID%30s\n";

void hw_print_fb(nvObj_t *nv) { text_print(nv, fmt_fb);}
void hw_print_fv(nvObj_t *nv) { text_print(nv, fmt_fv);}
void hw_print_hp(nvObj_t *nv) { text_print(nv, fmt_hp);}
void hw_print_hv(nvObj_t *nv) { text_print(nv, fmt_hv);}
void hw_print_id(nvObj_t *nv) { text_print(nv, fmt_id);}

#endif //__TEXT_MODE
