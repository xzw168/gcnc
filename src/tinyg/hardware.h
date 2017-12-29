/*
 * hardware.h - system hardware configuration
 *				THIS FILE IS HARDWARE PLATFORM SPECIFIC - AVR Xmega version
 *
 * This file is part of the TinyG project
 *
 * Copyright (c) 2013 - 2016 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2016 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * INTERRUPT USAGE - TinyG uses a lot of them all over the place
 *
 *	HI	Stepper DDA pulse generation		(set in stepper.h)
 *	HI	Stepper load routine SW interrupt	(set in stepper.h)
 *	HI	Dwell timer counter 				(set in stepper.h)
 *  LO	Segment execution SW interrupt		(set in stepper.h)
 *	MED	GPIO1 switch port					(set in gpio.h)
 *  MED	Serial RX for USB & RS-485			(set in xio_usart.h)
 *  MED	Serial TX for USB & RS-485			(set in xio_usart.h) (* see note)
 *	LO	Real time clock interrupt			(set in xmega_rtc.h)
 *
 *	(*) The TX cannot run at LO level or exception reports and other prints
 *		called from a LO interrupt (as in prep_line()) will kill the system in a
 *		permanent sleep_mode() call in xio_putc_usb() (xio.usb.c) as no interrupt
 *		can release the sleep mode.
 */

#ifndef HARDWARE_H_ONCE
#define HARDWARE_H_ONCE

/*--- Hardware platform enumerations ---*/

typedef enum {
	HM_PLATFORM_NONE = 0,

	HW_PLATFORM_TINYG_XMEGA,	// TinyG code base on Xmega boards.
								//	hwVersion 7 = TinyG v7 and earlier
								//	hwVersion 8 = TinyG v8

	HW_PLATFORM_G2_DUE,			// G2 code base on native Arduino Due

	HW_PLATFORM_TINYG_V9		// G2 code base on v9 boards
								//  hwVersion 0 = v9c
								//  hwVersion 1 = v9d
								//  hwVersion 2 = v9f
								//  hwVersion 3 = v9h
								//  hwVersion 4 = v9i
} hwPlatform;

#define HW_VERSION_TINYGV6		6
#define HW_VERSION_TINYGV7		7
#define HW_VERSION_TINYGV8		8

#define HW_VERSION_TINYGV9C		0
#define HW_VERSION_TINYGV9D		1
#define HW_VERSION_TINYGV9F		2
#define HW_VERSION_TINYGV9H		3
#define HW_VERSION_TINYGV9I		4

////////////////////////////
/////// AVR VERSION ////////
////////////////////////////

#include "config.h"						// needed for the stat_t typedef
//#include <avr/interrupt.h>
//#include "xmega/xmega_rtc.h"			// Xmega only. Goes away with RTC refactoring

// uncomment once motate Xmega port is available
//#include "motatePins.h"
//#include "motateTimers.h"				// for Motate::timer_number

/*************************
 * Global System Defines *
 *************************/

#undef F_CPU							// CPU clock - set for delays
#define F_CPU 32000000UL				// should always precede <avr/delay.h>
#define MILLISECONDS_PER_TICK 1			// MS for system tick (systick * N)
#define SYS_ID_LEN 12					// length of system ID string from sys_get_id()

/************************************************************************************
 **** AVR XMEGA SPECIFIC HARDWARE ***************************************************
 ************************************************************************************/

// Clock Crystal Config. Pick one:
//#define __CLOCK_INTERNAL_32MHZ TRUE	// use internal oscillator
//#define __CLOCK_EXTERNAL_8MHZ	TRUE	// uses PLL to provide 32 MHz system clock
#define __CLOCK_EXTERNAL_16MHZ TRUE		// uses PLL to provide 32 MHz system clock

/*** Motor, output bit & switch port assignments ***
 *** These are not all the same, and must line up in multiple places in gpio.h ***
 * Sorry if this is confusing - it's a board routing issue
 */






/*
 * Port setup - Stepper / Switch Ports:
 *	b0	(out) step			(SET is step,  CLR is rest)
 *	b1	(out) direction		(CLR = Clockwise)
 *	b2	(out) motor enable 	(CLR = Enabled)
 *	b3	(out) microstep 0
 *	b4	(out) microstep 1
 *	b5	(out) output bit for GPIO port1
 *	b6	(in) min limit switch on GPIO 2 (note: motor controls and GPIO2 port mappings are not the same)
 *	b7	(in) max limit switch on GPIO 2 (note: motor controls and GPIO2 port mappings are not the same)
 */
#define MOTOR_PORT_DIR_gm 0x3F	// dir settings: lower 6 out, upper 2 in
//#define MOTOR_PORT_DIR_gm 0x00	// dir settings: all inputs

typedef enum {			    // motor control port bit positions
	STEP_BIT_bp = 0,		// bit 0
	DIRECTION_BIT_bp,		// bit 1
	MOTOR_ENABLE_BIT_bp,	// bit 2
	MICROSTEP_BIT_0_bp,		// bit 3
	MICROSTEP_BIT_1_bp,		// bit 4
	GPIO1_OUT_BIT_bp,		// bit 5 (4 gpio1 output bits; 1 from each axis)
	SW_MIN_BIT_bp,			// bit 6 (4 input bits for homing/limit switches)
	SW_MAX_BIT_bp			// bit 7 (4 input bits for homing/limit switches)
} cfgPortBits;

#define STEP_BIT_bm			(1<<STEP_BIT_bp)
#define DIRECTION_BIT_bm	(1<<DIRECTION_BIT_bp)
#define MOTOR_ENABLE_BIT_bm (1<<MOTOR_ENABLE_BIT_bp)
#define MICROSTEP_BIT_0_bm	(1<<MICROSTEP_BIT_0_bp)
#define MICROSTEP_BIT_1_bm	(1<<MICROSTEP_BIT_1_bp)
#define GPIO1_OUT_BIT_bm	(1<<GPIO1_OUT_BIT_bp)	// spindle and coolant output bits
#define SW_MIN_BIT_bm		(1<<SW_MIN_BIT_bp)		// minimum switch inputs
#define SW_MAX_BIT_bm		(1<<SW_MAX_BIT_bp)		// maximum switch inputs

/* Bit assignments for GPIO1_OUTs for spindle, PWM and coolant */

#define SPINDLE_BIT			0x08		// spindle on/off
#define SPINDLE_DIR			0x04		// spindle direction, 1=CW, 0=CCW
#define SPINDLE_PWM			0x02		// spindle PWMs output bit
#define MIST_COOLANT_BIT	0x01		// coolant on/off - these are the same due to limited ports
#define FLOOD_COOLANT_BIT	0x01		// coolant on/off

#define SPINDLE_LED			0
#define SPINDLE_DIR_LED		1
#define SPINDLE_PWM_LED		2
#define COOLANT_LED			3

#define INDICATOR_LED		SPINDLE_DIR_LED	// can use the spindle direction as an indicator LED

/* Timer assignments - see specific modules for details) */



/* Timer setup for stepper and dwells */

#define FREQUENCY_DDA 		(float)50000	// DDA frequency in hz.
#define FREQUENCY_DWELL		(float)10000	// Dwell count frequency in hz.
#define LOAD_TIMER_PERIOD 	100				// cycles you have to shut off SW interrupt
#define EXEC_TIMER_PERIOD 	100				// cycles you have to shut off SW interrupt
#define EXEC_TIMER_PERIOD_LONG 100			// cycles you have to shut off SW interrupt





/*** function prototypes ***/

void hardware_init(void);			// master hardware init
void hw_request_hard_reset();
void hw_hard_reset(void);
stat_t hw_hard_reset_handler(void);

void hw_request_bootloader(void);
stat_t hw_bootloader_handler(void);
stat_t hw_run_boot(nvObj_t *nv);

stat_t hw_set_hv(nvObj_t *nv);
stat_t hw_get_id(nvObj_t *nv);

#ifdef __TEXT_MODE

	void hw_print_fb(nvObj_t *nv);
	void hw_print_fv(nvObj_t *nv);
	void hw_print_hp(nvObj_t *nv);
	void hw_print_hv(nvObj_t *nv);
	void hw_print_id(nvObj_t *nv);

#else

	#define hw_print_fb tx_print_stub
	#define hw_print_fv tx_print_stub
	#define hw_print_hp tx_print_stub
	#define hw_print_hv tx_print_stub
	#define hw_print_id tx_print_stub

#endif // __TEXT_MODE

#endif	// end of include guard: HARDWARE_H_ONCE
