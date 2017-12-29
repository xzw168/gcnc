/*
 * stepper.c - stepper motor controls
 * This file is part of the TinyG project
 *
 * Copyright (c) 2010 - 2016 Alden S. Hart, Jr.
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
/* 	This module provides the low-level stepper drivers and some related functions.
 *	See stepper.h for a detailed explanation of this module.
 */

#include "tinyg.h"
#include "config.h"
#include "stepper.h"
#include "encoder.h"
#include "planner.h"
#include "report.h"
#include "hardware.h"
#include "text_parser.h"
#include "util.h"

/**** Allocate structures ****/

stConfig_t st_cfg;
stPrepSingleton_t st_pre;
static stRunSingleton_t st_run;

/**** Setup local functions ****/

static void _load_move(void);
static void _request_load_move(void);


// handy macro
#define _f_to_period(f) (uint16_t)((float)F_CPU / (float)f)

/**** Setup motate ****/



/************************************************************************************
 **** CODE **************************************************************************
 ************************************************************************************/
/*
 * stepper_init() - initialize stepper motor subsystem
 *
 *	Notes:
 *	  - This init requires sys_init() to be run beforehand
 * 	  - microsteps are setup during config_init()
 *	  - motor polarity is setup during config_init()
 *	  - high level interrupts must be enabled in main() once all inits are complete
 */
/*	NOTE: This is the bare code that the Motate timer calls replace.
 *	NB: requires: #include <component_tc.h>
 *
 *	REG_TC1_WPMR = 0x54494D00;			// enable write to registers
 *	TC_Configure(TC_BLOCK_DDA, TC_CHANNEL_DDA, TC_CMR_DDA);
 *	REG_RC_DDA = TC_RC_DDA;				// set frequency
 *	REG_IER_DDA = TC_IER_DDA;			// enable interrupts
 *	NVIC_EnableIRQ(TC_IRQn_DDA);
 *	pmc_enable_periph_clk(TC_ID_DDA);
 *	TC_Start(TC_BLOCK_DDA, TC_CHANNEL_DDA);
 */

void stepper_init()
{
	memset(&st_run, 0, sizeof(st_run));			// clear all values, pointers and status
	stepper_init_assertions();


	TIMER_DDA_DISABLE();
	TIMER_EXEC_DISABLE();
	TIMER_LOAD_DISABLE();
	TIMER_DWELL_DISABLE();

	stepper_reset();                            // reset steppers to known state


}

/*
 * stepper_reset() - reset stepper internals
 */

void stepper_reset()
{


    st_run.dda_ticks_downcount = 0;                     // signal the runtime is not busy
    st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;    // set to EXEC or it won't restart

	for (uint8_t motor=0; motor<MOTORS; motor++) {      // Note: do not initialize prev_direction
		st_run.mot[motor].substep_accumulator = 0;      // will become max negative during per-motor setup;
		st_pre.mot[motor].corrected_steps = 0;          // diagnostic only - no action effect
	}
 	mp_set_steps_to_runtime_position();                 // reset encoder to agree with the above
}


/*
 * stepper_init_assertions() - test assertions, return error code if violation exists
 * stepper_test_assertions() - test assertions, return error code if violation exists
 */

void stepper_init_assertions()
{
	st_run.magic_end = MAGICNUM;
	st_run.magic_start = MAGICNUM;
	st_pre.magic_end = MAGICNUM;
	st_pre.magic_start = MAGICNUM;
}

stat_t stepper_test_assertions()
{
    if ((BAD_MAGIC(st_run.magic_start)) || (BAD_MAGIC(st_run.magic_end)) ||
        (BAD_MAGIC(st_pre.magic_start)) || (BAD_MAGIC(st_pre.magic_end))) {
        return(cm_panic_P(STAT_STEPPER_ASSERTION_FAILURE, PSTR("stepper_test_assertions()")));
    }
    return (STAT_OK);
}


/*
 * st_runtime_isbusy() - return TRUE if runtime is busy:
 *
 *	Busy conditions:
 *	- motors are running
 *	- dwell is running
 */

uint8_t st_runtime_isbusy()
{
	if (st_run.dda_ticks_downcount == 0) {
		return (false);
	}
	return (true);
}

/*
 * st_clc() - clear counters
 */

stat_t st_clc(nvObj_t *nv)	// clear diagnostic counters, reset stepper prep
{
	stepper_reset();
	return(STAT_OK);
}

/*
 * Motor power management functions
 *
 * _deenergize_motor()		 - remove power from a motor
 * _energize_motor()		 - apply power to a motor
 * _set_motor_power_level()	 - set the actual Vref to a specified power level
 *
 * st_energize_motors()		 - apply power to all motors
 * st_deenergize_motors()	 - remove power from all motors
 * st_motor_power_callback() - callback to manage motor power sequencing
 */

static uint8_t _motor_is_enabled(uint8_t motor)
{
	return (1);//xzw169
}
//�жϵ���ĵ�Դ
static void _deenergize_motor(const uint8_t motor)
{
	MOTOR_EN_DISABLE(motor);
	st_run.mot[motor].power_state = MOTOR_OFF;

}

static void _energize_motor(const uint8_t motor)
{
	if (st_cfg.mot[motor].power_mode == MOTOR_DISABLED) {
		_deenergize_motor(motor);
		return;
	}
    MOTOR_EN_ENABLE(motor);
	st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
}



void st_energize_motors()
{
	for (uint8_t motor = MOTOR_1; motor < MOTORS; motor++) {
		_energize_motor(motor);
		st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;
	}

}

void st_deenergize_motors()
{
	for (uint8_t motor = MOTOR_1; motor < MOTORS; motor++) {
		_deenergize_motor(motor);
	}

}

/*
 * st_motor_power_callback() - callback to manage motor power sequencing
 *
 *	Handles motor power-down timing, low-power idle, and adaptive motor power
 */
stat_t st_motor_power_callback() 	// called by controller
{
	// manage power for each motor individually
	for (uint8_t m = MOTOR_1; m < MOTORS; m++) {

		// de-energize motor if it's set to MOTOR_DISABLED
		if (st_cfg.mot[m].power_mode == MOTOR_DISABLED) {
			_deenergize_motor(m);
			continue;
		}

		// energize motor if it's set to MOTOR_ALWAYS_POWERED
		if (st_cfg.mot[m].power_mode == MOTOR_ALWAYS_POWERED) {
			if (! _motor_is_enabled(m)) _energize_motor(m);
			continue;
		}

		// start a countdown if MOTOR_POWERED_IN_CYCLE or MOTOR_POWERED_ONLY_WHEN_MOVING
		if (st_run.mot[m].power_state == MOTOR_POWER_TIMEOUT_START) {
			st_run.mot[m].power_state = MOTOR_POWER_TIMEOUT_COUNTDOWN;
			st_run.mot[m].power_systick = SysTickTimer_getValue() +
										  (uint32_t)(st_cfg.motor_power_timeout * 1000);
		}

		// do not process countdown if in a feedhold
		if (cm_get_motion_state() == MOTION_HOLD) {
			continue;
		}

		// do not process countdown if in a feedhold
//		if (cm_get_combined_state() == COMBINED_HOLD) {
//			continue;
//		}

		// run the countdown if you are in a countdown
		if (st_run.mot[m].power_state == MOTOR_POWER_TIMEOUT_COUNTDOWN) {
			if (SysTickTimer_getValue() > st_run.mot[m].power_systick ) {
				st_run.mot[m].power_state = MOTOR_IDLE;
				_deenergize_motor(m);
                sr_request_status_report(SR_REQUEST_TIMED);		// request a status report when motors shut down
			}
		}
	}
	return (STAT_OK);
}


/******************************
 * Interrupt Service Routines *
 ******************************/

/***** Stepper Interrupt Service Routine ************************************************
 * ISR - DDA timer interrupt routine - service ticks from DDA timer
 */

#ifdef __AVR
/*
 *	Uses direct struct addresses and literal values for hardware devices - it's faster than
 *	using indexed timer and port accesses. I checked. Even when -0s or -03 is used.
 */
void TIMER_DDA_ISR_vect(void)
{
	if ((st_run.mot[MOTOR_1].substep_accumulator += st_run.mot[MOTOR_1].substep_increment) > 0) {
		MOTOR_CLK_H(MOTOR_1); 
		st_run.mot[MOTOR_1].substep_accumulator -= st_run.dda_ticks_X_substeps;
		INCREMENT_ENCODER(MOTOR_1);
	}
	if ((st_run.mot[MOTOR_2].substep_accumulator += st_run.mot[MOTOR_2].substep_increment) > 0) {
		MOTOR_CLK_H(MOTOR_2);
		st_run.mot[MOTOR_2].substep_accumulator -= st_run.dda_ticks_X_substeps;
		INCREMENT_ENCODER(MOTOR_2);
	}
	if ((st_run.mot[MOTOR_3].substep_accumulator += st_run.mot[MOTOR_3].substep_increment) > 0) {
		MOTOR_CLK_H(MOTOR_3);
		st_run.mot[MOTOR_3].substep_accumulator -= st_run.dda_ticks_X_substeps;
		INCREMENT_ENCODER(MOTOR_3);
	}
	if ((st_run.mot[MOTOR_4].substep_accumulator += st_run.mot[MOTOR_4].substep_increment) > 0) {
		MOTOR_CLK_H(MOTOR_4);
		st_run.mot[MOTOR_4].substep_accumulator -= st_run.dda_ticks_X_substeps;
		INCREMENT_ENCODER(MOTOR_4);
	}

	// pulse stretching for using external drivers.- turn step bits off
	MOTOR_CLK_L(MOTOR_1);
	MOTOR_CLK_L(MOTOR_2);
	MOTOR_CLK_L(MOTOR_3);
	MOTOR_CLK_L(MOTOR_4);
	if (--st_run.dda_ticks_downcount != 0) return;

	TIMER_DDA_DISABLE();                                //����DDA��ʱ��
	_load_move();										// load the next move
}
#endif // __AVR


/***** Dwell Interrupt Service Routine **************************************************
 * ISR - DDA timer interrupt routine - service ticks from DDA timer
 */

#ifdef __AVR
void TIMER_DWELL_ISR_vect() {								// DWELL timer interrupt
	if (--st_run.dda_ticks_downcount == 0) {
		TIMER_DWELL_DISABLE();//����DWELL��ʱ��
		_load_move();
	}
}
#endif


/****************************************************************************************
 * Exec sequencing code		- computes and prepares next load segment
 * st_request_exec_move()	- SW interrupt to request to execute a move
 * exec_timer interrupt		- interrupt handler for calling exec function
 */

#ifdef __AVR
void st_request_exec_move()
{
	if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {// bother interrupting
		TIMER_EXEC_PER(EXEC_TIMER_PERIOD);
		TIMER_EXEC_ENABLE();
	}
}

void TIMER_EXEC_ISR_vect() {								// exec move SW interrupt
	TIMER_EXEC_DISABLE();

	// exec_move
	if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_EXEC) {
		if (mp_exec_move() != STAT_NOOP) {
			st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER; // ��ת����
			_request_load_move();
		}
	}
}
#endif // __AVR



/****************************************************************************************
 * Loader sequencing code
 * st_request_load_move() - fires a software interrupt (timer) to request to load a move
 * load_move interrupt	  - interrupt handler for running the loader
 *
 *	_load_move() can only be called be called from an ISR at the same or higher level as
 *	the DDA or dwell ISR. A software interrupt has been provided to allow a non-ISR to
 *	request a load (see st_request_load_move())
 */

#ifdef __AVR
static void _request_load_move()
{
	if (st_runtime_isbusy()) {
		return;													// don't request a load if the runtime is busy
	}
	if (st_pre.buffer_state == PREP_BUFFER_OWNED_BY_LOADER) {	// bother interrupting
		TIMER_LOAD_PER(LOAD_TIMER_PERIOD);
		TIMER_LOAD_ENABLE();
	}
}

void TIMER_LOAD_ISR_vect(void) {										// load steppers SW interrupt
	TIMER_LOAD_DISABLE();						// disable SW interrupt timer
	_load_move();
}
#endif // __AVR



/****************************************************************************************
 * _load_move() - Dequeue move and load into stepper struct
 *
 *	This routine can only be called be called from an ISR at the same or
 *	higher level as the DDA or dwell ISR. A software interrupt has been
 *	provided to allow a non-ISR to request a load (see st_request_load_move())
 *
 *	In aline() code:
 *	 - All axes must set steps and compensate for out-of-range pulse phasing.
 *	 - If axis has 0 steps the direction setting can be omitted
 *	 - If axis has 0 steps the motor must not be enabled to support power mode = 1
 */
/****** WARNING - THIS CODE IS SPECIFIC TO AVR. SEE G2 FOR ARM CODE ******/

static void _load_move()
{
	// Be aware that dda_ticks_downcount must equal zero for the loader to run.
	// So the initial load must also have this set to zero as part of initialization
	if (st_runtime_isbusy()) {
		return;													// exit if the runtime is busy
	}
	if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_LOADER) {	// if there are no moves to load...
//		for (uint8_t motor = MOTOR_1; motor < MOTORS; motor++) {
//			st_run.mot[motor].power_state = MOTOR_POWER_TIMEOUT_START;	// ...start motor power timeouts
//		}
		return;
	}
	// handle aline loads first (most common case)
	if (st_pre.block_type == BLOCK_TYPE_ALINE) {

		//**** setup the new segment ****

		st_run.dda_ticks_downcount = st_pre.dda_ticks;
		st_run.dda_ticks_X_substeps = st_pre.dda_ticks_X_substeps;

		//**** MOTOR_1 LOAD ****

		int i;
		for(i=0;i<MOTORS;i++){
			
			if ((st_run.mot[i].substep_increment = st_pre.mot[i].substep_increment) != 0) {
				if (st_pre.mot[i].accumulator_correction_flag == true) {
					st_pre.mot[i].accumulator_correction_flag = false;
					st_run.mot[i].substep_accumulator *= st_pre.mot[i].accumulator_correction;
				}

				if (st_pre.mot[i].direction != st_pre.mot[i].prev_direction) {
					st_pre.mot[i].prev_direction = st_pre.mot[i].direction;
					st_run.mot[i].substep_accumulator = -(st_run.dda_ticks_X_substeps + st_run.mot[i].substep_accumulator);
					if (st_pre.mot[i].direction == DIRECTION_CW)
					MOTOR_DIR_L(i); else
					MOTOR_DIR_H(i);
				}
				SET_ENCODER_STEP_SIGN(i, st_pre.mot[i].step_sign);
				if (st_cfg.mot[i].power_mode != MOTOR_DISABLED) {
					MOTOR_EN_ENABLE(i);             // ���ʹ��
					st_run.mot[i].power_state = MOTOR_POWER_TIMEOUT_START;// set power management state
				}

			} else {  // Motor has 0 steps; might need to energize motor for power mode processing
				if (st_cfg.mot[i].power_mode == MOTOR_POWERED_IN_CYCLE) {
					MOTOR_EN_ENABLE(i);             // ���ʹ��
					st_run.mot[i].power_state = MOTOR_POWER_TIMEOUT_START;
				}
			}
			ACCUMULATE_ENCODER(i);			
		}
		

		//**** do this last ****

		TIMER_DDA_PER(st_pre.dda_period);
		TIMER_DDA_ENABLE();

	// handle dwells
	} else if (st_pre.block_type == BLOCK_TYPE_DWELL) {
		st_run.dda_ticks_downcount = st_pre.dda_ticks;
		TIMER_DWELL_PER(st_pre.dda_period);
		TIMER_DWELL_ENABLE();

	// handle synchronous commands
	} else if (st_pre.block_type == BLOCK_TYPE_COMMAND) {
		mp_runtime_command(st_pre.bf);
	}

	// all other cases drop to here (e.g. Null moves after Mcodes skip to here)
	st_pre.block_type = BLOCK_TYPE_NULL;
	st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;	// we are done with the prep buffer - flip the flag back
	st_request_exec_move();								// exec and prep next move
}

/***********************************************************************************
 * st_prep_line() - Prepare the next move for the loader
 *
 *	This function does the math on the next pulse segment and gets it ready for
 *	the loader. It deals with all the DDA optimizations and timer setups so that
 *	loading can be performed as rapidly as possible. It works in joint space
 *	(motors) and it works in steps, not length units. All args are provided as
 *	floats and converted to their appropriate integer types for the loader.
 *
 * Args:
 *	  - travel_steps[] are signed relative motion in steps for each motor. Steps are
 *		floats that typically have fractional values (fractional steps). The sign
 *		indicates direction. Motors that are not in the move should be 0 steps on input.
 *
 *	  - following_error[] is a vector of measured errors to the step count. Used for correction.
 *
 *	  - segment_time - how many minutes the segment should run. If timing is not
 *		100% accurate this will affect the move velocity, but not the distance traveled.
 *
 * NOTE:  Many of the expressions are sensitive to casting and execution order to avoid long-term
 *		  accuracy errors due to floating point round off. One earlier failed attempt was:
 *		    dda_ticks_X_substeps = (int32_t)((microseconds/1000000) * f_dda * dda_substeps);
 */

stat_t st_prep_line(float travel_steps[], float following_error[], float segment_time)
{
	// trap conditions that would prevent queueing the line
	if (st_pre.buffer_state != PREP_BUFFER_OWNED_BY_EXEC) {
		return (cm_panic_P(STAT_INTERNAL_ERROR, PSTR("st_prep_line")));
	} else if (isinf(segment_time)) {
        return (cm_panic_P(STAT_PREP_LINE_MOVE_TIME_IS_INFINITE, PSTR("st_prep_line")));	// never supposed to happen
	} else if (isnan(segment_time)) {
        return (cm_panic_P(STAT_PREP_LINE_MOVE_TIME_IS_NAN, PSTR("st_prep_line")));		// never supposed to happen
	} else if (segment_time < EPSILON) {
        return (STAT_MINIMUM_TIME_MOVE);
	}
	// setup segment parameters
	// - dda_ticks is the integer number of DDA clock ticks needed to play out the segment
	// - ticks_X_substeps is the maximum depth of the DDA accumulator (as a negative number)

	st_pre.dda_period = _f_to_period(FREQUENCY_DDA);
	st_pre.dda_ticks = (int32_t)(segment_time * 60 * FREQUENCY_DDA);// NB: converts minutes to seconds
	st_pre.dda_ticks_X_substeps = st_pre.dda_ticks * DDA_SUBSTEPS;

	// setup motor parameters

	float correction_steps;
	for (uint8_t motor=0; motor<MOTORS; motor++) {	// I want to remind myself that this is motors, not axes

		// Skip this motor if there are no new steps. Leave all other values intact.
		if (fp_ZERO(travel_steps[motor])) { st_pre.mot[motor].substep_increment = 0; continue;}

		// Setup the direction, compensating for polarity.
		// Set the step_sign which is used by the stepper ISR to accumulate step position

		if (travel_steps[motor] >= 0) {					// positive direction
			st_pre.mot[motor].direction = DIRECTION_CW ^ st_cfg.mot[motor].polarity;
			st_pre.mot[motor].step_sign = 1;
		} else {
			st_pre.mot[motor].direction = DIRECTION_CCW ^ st_cfg.mot[motor].polarity;
			st_pre.mot[motor].step_sign = -1;
		}

		// Detect segment time changes and setup the accumulator correction factor and flag.
		// Putting this here computes the correct factor even if the motor was dormant for some
		// number of previous moves. Correction is computed based on the last segment time actually used.

		if (fabs(segment_time - st_pre.mot[motor].prev_segment_time) > 0.0000001) { // highly tuned FP != compare
			if (fp_NOT_ZERO(st_pre.mot[motor].prev_segment_time)) {					// special case to skip first move
				st_pre.mot[motor].accumulator_correction_flag = true;
				st_pre.mot[motor].accumulator_correction = segment_time / st_pre.mot[motor].prev_segment_time;
			}
			st_pre.mot[motor].prev_segment_time = segment_time;
		}

#ifdef __STEP_CORRECTION
		// 'Nudge' correction strategy. Inject a single, scaled correction value then hold off

		if ((--st_pre.mot[motor].correction_holdoff < 0) &&
			(fabs(following_error[motor]) > STEP_CORRECTION_THRESHOLD)) {

			st_pre.mot[motor].correction_holdoff = STEP_CORRECTION_HOLDOFF;
			correction_steps = following_error[motor] * STEP_CORRECTION_FACTOR;

			if (correction_steps > 0) {
				correction_steps = min3(correction_steps, fabs(travel_steps[motor]), STEP_CORRECTION_MAX);
			} else {
				correction_steps = max3(correction_steps, -fabs(travel_steps[motor]), -STEP_CORRECTION_MAX);
			}
			st_pre.mot[motor].corrected_steps += correction_steps;
			travel_steps[motor] -= correction_steps;
		}
#endif
		// Compute substeb increment. The accumulator must be *exactly* the incoming
		// fractional steps times the substep multiplier or positional drift will occur.
		// Rounding is performed to eliminate a negative bias in the uint32 conversion
		// that results in long-term negative drift. (fabs/round order doesn't matter)

		st_pre.mot[motor].substep_increment = round(fabs(travel_steps[motor] * DDA_SUBSTEPS));
	}
	st_pre.block_type = BLOCK_TYPE_ALINE;
	st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;	// signal that prep buffer is ready
	return (STAT_OK);
}

/*
 * st_prep_null() - Keeps the loader happy. Otherwise performs no action
 */

void st_prep_null()
{
	st_pre.block_type = BLOCK_TYPE_NULL;
	st_pre.buffer_state = PREP_BUFFER_OWNED_BY_EXEC;	// signal that prep buffer is empty
}

/*
 * st_prep_command() - Stage command to execution
 */

void st_prep_command(void *bf)
{
	st_pre.block_type = BLOCK_TYPE_COMMAND;
	st_pre.bf = (mpBuf_t *)bf;
	st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;	// signal that prep buffer is ready
}

/*
 * st_prep_dwell() 	 - Add a dwell to the move buffer
 */

void st_prep_dwell(float microseconds)
{
	st_pre.block_type = BLOCK_TYPE_DWELL;
	st_pre.dda_period = _f_to_period(FREQUENCY_DWELL);
	st_pre.dda_ticks = (uint32_t)((microseconds/1000000) * FREQUENCY_DWELL);
	st_pre.buffer_state = PREP_BUFFER_OWNED_BY_LOADER;	// signal that prep buffer is ready
}

//����ϸ��
static void _set_hw_microsteps(const uint8_t motor, const uint8_t microsteps)
{

#ifdef __AVR1 //xzw169
	if (microsteps == 8) {
		hw.st_port[motor]->OUTSET = MICROSTEP_BIT_0_bm;
		hw.st_port[motor]->OUTSET = MICROSTEP_BIT_1_bm;
	} else if (microsteps == 4) {
		hw.st_port[motor]->OUTCLR = MICROSTEP_BIT_0_bm;
		hw.st_port[motor]->OUTSET = MICROSTEP_BIT_1_bm;
	} else if (microsteps == 2) {
		hw.st_port[motor]->OUTSET = MICROSTEP_BIT_0_bm;
		hw.st_port[motor]->OUTCLR = MICROSTEP_BIT_1_bm;
	} else if (microsteps == 1) {
		hw.st_port[motor]->OUTCLR = MICROSTEP_BIT_0_bm;
		hw.st_port[motor]->OUTCLR = MICROSTEP_BIT_1_bm;
	}
#endif // __AVR
}


/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 ***********************************************************************************/

/* HELPERS
 * _get_motor() - helper to return motor number as an index
 */

static int8_t _get_motor(const nvObj_t *nv)
{
    return ((nv->group[0] ? nv->group[0] : nv->token[0]) - 0x31);
}

/*
 * _set_motor_steps_per_unit() - what it says
 * This function will need to be rethought if microstep morphing is implemented
 */

static void _set_motor_steps_per_unit(nvObj_t *nv)
{
	uint8_t m = _get_motor(nv);
	st_cfg.mot[m].units_per_step = (st_cfg.mot[m].travel_rev * st_cfg.mot[m].step_angle) / (360 * st_cfg.mot[m].microsteps); // unused
    st_cfg.mot[m].steps_per_unit = (360 * st_cfg.mot[m].microsteps) / (st_cfg.mot[m].travel_rev * st_cfg.mot[m].step_angle);
	stepper_reset();
}

/* PER-MOTOR FUNCTIONS
 * st_set_sa() - set motor step angle
 * st_set_tr() - set travel per motor revolution
 * st_set_mi() - set motor microsteps
 * st_set_pm() - set motor power mode
 * st_set_pl() - set motor power level
 */

stat_t st_set_sa(nvObj_t *nv)			// motor step angle
{
	set_flt(nv);
	_set_motor_steps_per_unit(nv);
	return(STAT_OK);
}

stat_t st_set_tr(nvObj_t *nv)			// motor travel per revolution
{
	set_flu(nv);
	_set_motor_steps_per_unit(nv);
	return(STAT_OK);
}

stat_t st_set_mi(nvObj_t *nv)			// motor microsteps
{
    uint32_t mi = nv->value_int;
	if ((mi != 1) && (mi != 2) && (mi != 4) && (mi != 8)) {
		nv_add_message_P(PSTR("*** WARNING *** Setting non-standard microstep value"));
	}
	set_u32(nv);						// set it anyway, even if it's unsupported. It could also be > 255
	_set_motor_steps_per_unit(nv);
	_set_hw_microsteps(_get_motor(nv), nv->value_int);
	return (STAT_OK);
}

stat_t st_set_pm(nvObj_t *nv)			// motor power mode
{
	if (nv->value_int >= MOTOR_POWER_MODE_MAX_VALUE) {
        return (STAT_INPUT_VALUE_RANGE_ERROR);
    }
	set_ui8(nv);
	return (STAT_OK);
	// NOTE: The motor power callback makes these settings take effect immediately
}

/*
 * st_set_pl() - set motor power level
 *
 *	Input value may vary from 0.000 to 1.000 The setting is scaled to allowable PWM range.
 *	This function sets both the scaled and dynamic power levels, and applies the
 *	scaled value to the vref.
 */
stat_t st_set_pl(nvObj_t *nv)	// motor power level
{

	return(STAT_OK);
}

/*
 * st_get_pwr()	- get motor enable power state
 */
stat_t st_get_pwr(nvObj_t *nv)
{
	nv->value_int = _motor_is_enabled(_get_motor(nv));
	nv->valuetype = TYPE_INTEGER;
	return (STAT_OK);
}

/* GLOBAL FUNCTIONS (SYSTEM LEVEL)
 *
 * st_set_mt() - set motor timeout in seconds
 * st_set_md() - disable motor power
 * st_set_me() - enable motor power
 *
 * Calling me or md with NULL will enable or disable all motors
 * Setting a value of 0 will enable or disable all motors
 * Setting a value from 1 to MOTORS will enable or disable that motor only
 */

stat_t st_set_mt(nvObj_t *nv)
{
	st_cfg.motor_power_timeout = min(MOTOR_TIMEOUT_SECONDS_MAX, max(nv->value_flt, MOTOR_TIMEOUT_SECONDS_MIN));
	return (STAT_OK);
}

stat_t st_set_md(nvObj_t *nv)	// Make sure this function is not part of initialization --> f00
{
	if ((nv->value_int == 0) || (nv->valuetype == TYPE_NULL)) {
		st_deenergize_motors();
	} else {
        uint8_t motor = nv->value_int;
        if (motor > MOTORS) {
            return (STAT_INPUT_VALUE_RANGE_ERROR);
        }
        _deenergize_motor(motor-1);     // adjust so that motor 1 is actually 0 (etc)
	}
	return (STAT_OK);
}

stat_t st_set_me(nvObj_t *nv)	// Make sure this function is not part of initialization --> f00
{
	if ((nv->value_int == 0) || (nv->valuetype == TYPE_NULL)) {
		st_energize_motors();
	} else {
        uint8_t motor = nv->value_int;
        if (motor > MOTORS) {
            return (STAT_INPUT_VALUE_RANGE_ERROR);
        }
		_energize_motor(motor-1);     // adjust so that motor 1 is actually 0 (etc)
	}
	return (STAT_OK);
}

/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

static const char msg_units0[] PROGMEM = " in";	// used by generic print functions
static const char msg_units1[] PROGMEM = " mm";
static const char msg_units2[] PROGMEM = " deg";
static const char *const msg_units[] PROGMEM = { msg_units0, msg_units1, msg_units2 };
#define DEGREE_INDEX 2

static const char fmt_me[] PROGMEM = "motors energized\n";
static const char fmt_md[] PROGMEM = "motors de-energized\n";
static const char fmt_mt[] PROGMEM = "[mt]  motor idle timeout%14.2f Sec\n";
static const char fmt_0ma[] PROGMEM = "[%s%s] m%s map to axis%15d [0=X,1=Y,2=Z...]\n";
static const char fmt_0sa[] PROGMEM = "[%s%s] m%s step angle%20.3f%s\n";
static const char fmt_0tr[] PROGMEM = "[%s%s] m%s travel per revolution%10.4f%s\n";
static const char fmt_0mi[] PROGMEM = "[%s%s] m%s microsteps%16d [1,2,4,8]\n";
static const char fmt_0po[] PROGMEM = "[%s%s] m%s polarity%18d [0=normal,1=reverse]\n";
static const char fmt_0pm[] PROGMEM = "[%s%s] m%s power management%10d [0=disabled,1=always on,2=in cycle,3=when moving]\n";
static const char fmt_0pl[] PROGMEM = "[%s%s] m%s motor power level%13.3f [0.000=minimum, 1.000=maximum]\n";
static const char fmt_pwr[] PROGMEM = "Motor %c power enabled state:%2.0f\n";

void st_print_mt(nvObj_t *nv) { text_print_flt(nv, fmt_mt);}
void st_print_me(nvObj_t *nv) { text_print_nul(nv, fmt_me);}
void st_print_md(nvObj_t *nv) { text_print_nul(nv, fmt_md);}

static void _print_motor_ui8(nvObj_t *nv, const char *format)
{
    char msg[NV_MESSAGE_LEN];
    sprintf_P(msg, format, nv->group, nv->token, nv->group, (uint8_t)nv->value_int);
    text_finalize_message(msg);
}

static void _print_motor_flt_units(nvObj_t *nv, const char *format, uint8_t units)
{
    char msg[NV_MESSAGE_LEN];
    sprintf_P(msg, format, nv->group, nv->token, nv->group, nv->value_flt, GET_TEXT_ITEM(msg_units, units));
    text_finalize_message(msg);
}

static void _print_motor_flt(nvObj_t *nv, const char *format)
{
    char msg[NV_MESSAGE_LEN];
    sprintf_P(msg, format, nv->group, nv->token, nv->group, nv->value_flt);
    text_finalize_message(msg);
}

static void _print_motor_pwr(nvObj_t *nv, const char *format)
{
    char msg[NV_MESSAGE_LEN];
    sprintf_P(msg, format, nv->token[0], nv->value_flt);
    text_finalize_message(msg);
}

void st_print_ma(nvObj_t *nv) { _print_motor_ui8(nv, fmt_0ma);}
void st_print_sa(nvObj_t *nv) { _print_motor_flt_units(nv, fmt_0sa, DEGREE_INDEX);}
void st_print_tr(nvObj_t *nv) { _print_motor_flt_units(nv, fmt_0tr, cm_get_units_mode(MODEL));}
void st_print_mi(nvObj_t *nv) { _print_motor_ui8(nv, fmt_0mi);}
void st_print_po(nvObj_t *nv) { _print_motor_ui8(nv, fmt_0po);}
void st_print_pm(nvObj_t *nv) { _print_motor_ui8(nv, fmt_0pm);}
void st_print_pl(nvObj_t *nv) { _print_motor_flt(nv, fmt_0pl);}
void st_print_pwr(nvObj_t *nv){ _print_motor_pwr(nv, fmt_pwr);}

#endif // __TEXT_MODE
