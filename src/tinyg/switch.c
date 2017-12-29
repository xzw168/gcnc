/*
 * switch.c - switch handling functions
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2016 Alden S. Hart, Jr.
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software. If not, see <http://www.gnu.org/licenses/>.
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
/* Switch Modes
 *
 *	The switches are considered to be homing switches when machine_state is
 *	MACHINE_HOMING. At all other times they are treated as limit switches:
 *	  - Hitting a homing switch puts the current move into feedhold
 *	  - Hitting a limit switch causes the machine to shut down and go into lockdown until reset
 *
 * 	The normally open switch modes (NO) trigger an interrupt on the falling edge
 *	and lockout subsequent interrupts for the defined lockout period. This approach
 *	beats doing debouncing as an integration as switches fire immediately.
 *
 * 	The normally closed switch modes (NC) trigger an interrupt on the rising edge
 *	and lockout subsequent interrupts for the defined lockout period. Ditto on the method.
 */

//#include <avr/interrupt.h>

#include "tinyg.h"
#include "config.h"
#include "switch.h"
#include "hardware.h"
#include "controller.h"
#include "canonical_machine.h"
#include "text_parser.h"
#include "report.h"

static bool _read_raw_switch(const uint8_t sw_num);
static void _dispatch_switch(const uint8_t sw_num);
switches_t sw;

void switch_init(void)
{

	reset_switches();
}

void reset_switches()
{
    for (uint8_t i=0; i < NUM_SWITCHES; i++) {
        if (sw.s[i].mode == SW_MODE_DISABLED) {
            sw.s[i].state = SW_DISABLED;
            sw.s[i].edge = SW_EDGE_NONE;
            continue;
        }
        if (sw.s[i].mode == SW_MODE_PROBE) {
            sw.s[i].type = SW_ACTIVE_LO;            // This is a hack. Not all probes are NO
        }
        else {
            sw.s[i].type = sw.switch_type;          // switches inherit global switch type
        }
        sw.s[i].state = _read_raw_switch(i);        // set initial conditions
        sw.s[i].edge = SW_EDGE_NONE;
        sw.s[i].lockout_ms = SW_LOCKOUT_MS;
        //Timeout_clear(&sw.s[i].timeout);            //xzw168 clear lockout timer
	}
}

/*
 * get_switch_mode() - return switch mode setting
 */

uint8_t get_switch_mode(const uint8_t sw_num) { return (sw.s[sw_num].mode);}



static bool _read_raw_switch(const uint8_t sw_num)
{
    return SW_IN(sw_num);	    // XOR to correct for ACTIVE mode. Casts to bool.
}

static void _dispatch_switch(const uint8_t sw_num)
{
    //*** process 'no action' conditions ***//
	if (sw.s[sw_num].mode == SW_MODE_DISABLED) {        // input is disabled (not supposed to happen)
        return;
    }
    /*if ((Timeout_isSet(&sw.s[sw_num].timeout)) &&
        (!Timeout_isPast(&sw.s[sw_num].timeout))) {     // input is in lockout period
        return;
    }*/ //xzw168
	bool raw_switch = _read_raw_switch(sw_num);         // no change in state (not supposed to happen)
    if (sw.s[sw_num].state == raw_switch) {
        return;
    }

    // read the switch, set edges and start lockout timer
    sw.s[sw_num].state = raw_switch;                    // 1 = switch hit, 0 = switch unhit
    sw.s[sw_num].edge = raw_switch;                     // 1 = leading edge, 0 = trailing edge
    //Timeout_set(&sw.s[sw_num].timeout, sw.s[sw_num].lockout_ms); //xzw168

    //*** execute the functions ***/

	sr_request_status_report(SR_REQUEST_ASAP);          // generate incremental status report to show any changes

    // functions that trigger on either edge
	if (cm.cycle_state == CYCLE_HOMING) {
    	cm_request_feedhold();
        sw.s[sw_num].edge = SW_EDGE_NONE;
        return;
    }

    // functions that only trigger on the LEADING_EDGE
    if (sw.s[sw_num].edge == SW_EDGE_TRAILING) {
        sw.s[sw_num].edge = SW_EDGE_NONE;
        return;
    } else {
        sw.s[sw_num].edge = SW_EDGE_NONE;
    }

	if (cm.cycle_state == CYCLE_PROBE) {
    	cm_request_feedhold();
        return;
    }
	if (sw.s[sw_num].mode & SW_LIMIT_BIT) {
    	controller_assert_limit_condition(sw_num+1);
	}
}

/*
 * read_switch() - read, store and return current state of switch
 */

swState read_switch(const uint8_t sw_num)
{
    if (sw.s[sw_num].mode == SW_MODE_DISABLED) {
        return (SW_DISABLED);
    }
    sw.s[sw_num].state = _read_raw_switch(sw_num);    // read pin
    return (sw.s[sw_num].state);
}

/*
 * find_probe_switch() - return probe switch number or error
 *
 *  Probes are only supported on XYZ axes (no rotaries)
 *
 *  Returns
 *      0-N - number of switch configured for PROBE
 *      -1  - no probe switch found on XYZ
 *      -2  - multiple probe switches found
 *      -3  - probe switch found on ABC axes
 */

int8_t find_probe_switch()
{
    uint8_t axis;
    int8_t probe = -1;          // probe switch

    // test for probe input on rotary axes
    // This is a hack that assumes there are no inputs mapped to B or C axes. OK for now.
    if ((sw.s[SW_MIN_A].mode == SW_MODE_PROBE) ||
        (sw.s[SW_MAX_A].mode == SW_MODE_PROBE)) {
        return (-3);
    }

    // test for one and only one probe input on XYZ axes
	for (axis=AXIS_X; axis<AXIS_A; axis++ ) {
    	if (sw.s[MIN_SWITCH(axis)].mode == SW_MODE_PROBE) {
            if (probe == -1) {
                probe = MIN_SWITCH(axis);
            } else {
                return (-2);    // multiple probe switches were found
            }
    	}
    	if (sw.s[MAX_SWITCH(axis)].mode == SW_MODE_PROBE) {
        	if (probe == -1) {
            	probe = MAX_SWITCH(axis);
            } else {
                return (-2);    // multiple probe switches were found
            }
    	}
    }
    return (probe);
}

/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 * These functions are not part of the NIST defined functions
 ***********************************************************************************/

stat_t sw_set_st(nvObj_t *nv)			// switch type (global)
{
	set_01(nv);
	switch_init();
	return (STAT_OK);
}

stat_t sw_set_sw(nvObj_t *nv)			// switch setting
{
	if (nv->value_int > SW_MODE_MAX_VALUE) {
        return (STAT_INPUT_VALUE_RANGE_ERROR);
    }
	set_ui8(nv);
	switch_init();
	return (STAT_OK);
}

/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

static const char fmt_st[] PROGMEM = "[st]  switch type%18d [0=NO,1=NC]\n";
void sw_print_st(nvObj_t *nv) { text_print(nv, fmt_st);}

static const char fmt_in[] PROGMEM = "Input %s state:    %2d\n";
void sw_print_in(nvObj_t *nv) { printf_P(fmt_in, nv->token, (int8_t)nv->value_int); }

#endif
