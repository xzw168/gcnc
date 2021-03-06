/*
 * config_app.c - application-specific part of configuration data
 * This file is part of the TinyG2 project
 *
 * Copyright (c) 2013 - 2016 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2016 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* This file contains application specific data for the config system:
 *	- application-specific functions and function prototypes
 *	- application-specific message and print format strings
 *	- application-specific config array
 *	- any other application-specific data or functions
 *
 * See config_app.h for a detailed description of config objects and the config table
 */

#include "tinyg.h"		// #1
#include "config.h"		// #2
#include "controller.h"
#include "canonical_machine.h"
#include "gcode_parser.h"
#include "json_parser.h"
#include "text_parser.h"
#include "settings.h"
#include "planner.h"
#include "stepper.h"
#include "switch.h"
#include "pwm.h"
#include "report.h"
#include "hardware.h"
#include "test.h"
#include "util.h"
#include "help.h"
#include "xio.h"

/*** structure allocation ***/

cfgParameters_t cfg; 				// application specific configuration parameters

/***********************************************************************************
 **** application-specific internal functions **************************************
 ***********************************************************************************/
// See config.cpp/.h for generic variables and functions that are not specific to
// TinyG or the motion control application domain

// helpers (most helpers are defined immediately above their usage so they don't need prototypes here)

static stat_t _do_motors(nvObj_t *nv);		// print parameters for all motor groups
static stat_t _do_axes(nvObj_t *nv);		// print parameters for all axis groups
static stat_t _do_offsets(nvObj_t *nv);		// print offset parameters for G54-G59,G92, G28, G30
static stat_t _do_all(nvObj_t *nv);			// print all parameters

// communications settings and functions

static stat_t set_ec(nvObj_t *nv);			// expand CRLF on TX output
static stat_t set_ee(nvObj_t *nv);			// enable character echo
static stat_t set_ex(nvObj_t *nv);			// enable XON/XOFF and RTS/CTS flow control
static stat_t set_baud(nvObj_t *nv);		// set USB baud rate
static stat_t get_rx(nvObj_t *nv);			// get bytes in RX buffer
//static stat_t get_tick(nvObj_t *nv);		// get system tick count
//static stat_t run_sx(nvObj_t *nv);		// send XOFF, XON

/***********************************************************************************
 **** CONFIG TABLE  ****************************************************************
 ***********************************************************************************
 *
 *	NOTES AND CAVEATS
 *
 *	- Token matching occurs from the most specific to the least specific. This means
 *	  that if shorter tokens overlap longer ones the longer one must precede the
 *	  shorter one. E.g. "gco" needs to come before "gc"
 *
 *	- Mark group strings for entries that have no group as nul -->  "".
 *	  This is important for group expansion.
 *
 *	- Groups do not have groups. Neither do uber-groups, e.g.
 *	  'x' is --> { "", "x",  	and 'm' is --> { "", "m",
 *
 *	- Be careful not to define groups longer than GROUP_LEN (3) and tokens longer
 *	  than TOKEN_LEN (5). (See config.h for lengths). The combined group + token
 *	  cannot exceed TOKEN_LEN. String functions working on the table assume these
 *	  rules are followed and do not check lengths or perform other validation.
 *
 *	NOTE: If the count of lines in cfgArray exceeds 255 you need to change index_t
 *	uint16_t in the config.h file.
 */

const cfgItem_t cfgArray[] PROGMEM = {
	// group token flags   p, print_func, get_func, set_func, target for get/set,   	  default value
	{ "sys", "fb", _fipn, 2, hw_print_fb, get_flt,  set_nul,  (uint32_t *)&cs.fw_build,   TINYG_FIRMWARE_BUILD , sizeof(cs.fw_build) }, // MUST BE FIRST!
	{ "sys", "fv", _fipn, 3, hw_print_fv, get_flt,  set_nul,  (uint32_t *)&cs.fw_version, TINYG_FIRMWARE_VERSION , sizeof(cs.fw_version) },
	{ "sys", "hp", _iipn, 0, hw_print_hp, get_u32,  set_ui8,  (uint32_t *)&cs.hw_platform,TINYG_HARDWARE_PLATFORM , sizeof(cs.hw_platform) },
	{ "sys", "hv", _fipn, 0, hw_print_hv, get_flt,  hw_set_hv,(uint32_t *)&cs.hw_version, TINYG_HARDWARE_VERSION , sizeof(cs.hw_version) },
	{ "sys", "id", _sn0,  0, hw_print_id, hw_get_id,set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },        // device ID (ASCII signature)

	// dynamic model attributes for reporting purposes (up front for speed)
	{ "",   "n",   _i0, 0, cm_print_line, get_u32,     set_u32,(uint32_t *)&cm.gm.linenum, 0 , sizeof(cm.gm.linenum) }, // Model line number
	{ "",   "line",_i0, 0, cm_print_line, cm_get_line, set_u32,(uint32_t *)&cm.gm.linenum, 0 , sizeof(cm.gm.linenum) }, // Active line number - model or runtime line number
	{ "",   "vel", _f0, 2, cm_print_vel,  cm_get_vel,  set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // current velocity
	{ "",   "feed",_f0, 2, cm_print_feed, cm_get_feed, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // feed rate
	{ "",   "stat",_i0, 0, cm_print_stat, cm_get_stat, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // combined machine state
	{ "",   "macs",_i0, 0, cm_print_macs, cm_get_macs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // raw machine state
	{ "",   "cycs",_i0, 0, cm_print_cycs, cm_get_cycs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // cycle state
	{ "",   "mots",_i0, 0, cm_print_mots, cm_get_mots, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // motion state
	{ "",   "hold",_i0, 0, cm_print_hold, cm_get_hold, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // feedhold state
	{ "",   "unit",_i0, 0, cm_print_unit, cm_get_unit, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // units mode
	{ "",   "coor",_i0, 0, cm_print_coor, cm_get_coor, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // coordinate system
	{ "",   "momo",_i0, 0, cm_print_momo, cm_get_momo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // motion mode
	{ "",   "plan",_i0, 0, cm_print_plan, cm_get_plan, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // plane select
	{ "",   "path",_i0, 0, cm_print_path, cm_get_path, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // path control mode
	{ "",   "dist",_i0, 0, cm_print_dist, cm_get_dist, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // distance mode
	{ "",   "admo",_i0, 0, cm_print_admo, cm_get_admo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // arc distance mode
	{ "",   "frmo",_i0, 0, cm_print_frmo, cm_get_frmo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // feed rate mode
	{ "",   "tool",_i0, 0, cm_print_tool, cm_get_toolv,set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // active tool
//	{ "",   "tick",_i0, 0, tx_print_int,  get_u32,     set_u32,(uint32_t *)&rtc.sys_ticks, 0 , sizeof(rtc.sys_ticks) },	// tick count

	{ "mpo","mpox",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// X machine position
	{ "mpo","mpoy",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Y machine position
	{ "mpo","mpoz",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Z machine position
	{ "mpo","mpoa",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// A machine position
	{ "mpo","mpob",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// B machine position
	{ "mpo","mpoc",_f0, 3, cm_print_mpo, cm_get_mpo, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// C machine position

	{ "pos","posx",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// X work position
	{ "pos","posy",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Y work position
	{ "pos","posz",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Z work position
	{ "pos","posa",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// A work position
	{ "pos","posb",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// B work position
	{ "pos","posc",_f0, 3, cm_print_pos, cm_get_pos, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// C work position

	{ "ofs","ofsx",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// X work offset
	{ "ofs","ofsy",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Y work offset
	{ "ofs","ofsz",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// Z work offset
	{ "ofs","ofsa",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// A work offset
	{ "ofs","ofsb",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// B work offset
	{ "ofs","ofsc",_f0, 3, cm_print_ofs, cm_get_ofs, set_nul,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },			// C work offset

	{ "hom","home",_i0, 0, cm_print_home,cm_get_home,cm_run_home,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },       // homing state, invoke homing cycle
	{ "hom","homx",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_X], false , sizeof(cm.homed[AXIS_X]) }, // X homed - Homing status group
	{ "hom","homy",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_Y], false , sizeof(cm.homed[AXIS_Y]) }, // Y homed
	{ "hom","homz",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_Z], false , sizeof(cm.homed[AXIS_Z]) }, // Z homed
	{ "hom","homa",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_A], false , sizeof(cm.homed[AXIS_A]) }, // A homed
	{ "hom","homb",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_B], false , sizeof(cm.homed[AXIS_B]) }, // B homed
	{ "hom","homc",_i0, 0, cm_print_pos, get_ui8, set_nul,(uint32_t *)&cm.homed[AXIS_C], false , sizeof(cm.homed[AXIS_C]) }, // C homed

	{ "prb","prbe",_i0, 0, tx_print_nul, get_ui8, set_nul,(uint32_t *)&cm.probe_state, 0 , sizeof(cm.probe_state) },       // probing state
	{ "prb","prbx",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_X], 0 , sizeof(cm.probe_results[AXIS_X]) },
	{ "prb","prby",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_Y], 0 , sizeof(cm.probe_results[AXIS_Y]) },
	{ "prb","prbz",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_Z], 0 , sizeof(cm.probe_results[AXIS_Z]) },
	{ "prb","prba",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_A], 0 , sizeof(cm.probe_results[AXIS_A]) },
	{ "prb","prbb",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_B], 0 , sizeof(cm.probe_results[AXIS_B]) },
	{ "prb","prbc",_f0, 3, tx_print_nul, get_flt, set_nul,(uint32_t *)&cm.probe_results[AXIS_C], 0 , sizeof(cm.probe_results[AXIS_C]) },

	{ "jog","jogx",_f0, 0, tx_print_nul, get_nul, cm_run_jogx, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },
	{ "jog","jogy",_f0, 0, tx_print_nul, get_nul, cm_run_jogy, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },
	{ "jog","jogz",_f0, 0, tx_print_nul, get_nul, cm_run_jogz, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },
	{ "jog","joga",_f0, 0, tx_print_nul, get_nul, cm_run_joga, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },
//	{ "jog","jogb",_i0, 0, tx_print_nul, get_nul, cm_run_jogb, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },    // FYI
//	{ "jog","jogc",_i0, 0, tx_print_nul, get_nul, cm_run_jogc, (uint32_t *)&cm.jogging_dest, 0, sizeof(cm.jogging_dest) },    // FYI

/*
	{ "pwr","pwr1",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },	// motor power enable readouts
	{ "pwr","pwr2",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },
	{ "pwr","pwr3",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },
	{ "pwr","pwr4",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },
#if (MOTORS >= 5)
	{ "pwr","pwr5",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },
#endif
#if (MOTORS >= 6)
	{ "pwr","pwr6",_i0, 0, st_print_pwr, st_get_pwr, set_nul, (uint32_t *)&cs.null, 0, sizeof(cs.null) },
#endif
*/
	// Motor parameters
	{ "1","1ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_1].motor_map,	M1_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_1].motor_map) },
	{ "1","1sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_1].step_angle,	M1_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_1].step_angle) },
	{ "1","1tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_1].travel_rev,	M1_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_1].travel_rev) },
	{ "1","1mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_1].microsteps,	M1_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_1].microsteps) },
	{ "1","1po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_1].polarity,	M1_POLARITY , sizeof(st_cfg.mot[MOTOR_1].polarity) },
	{ "1","1pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_1].power_mode,	M1_POWER_MODE , sizeof(st_cfg.mot[MOTOR_1].power_mode) },
#ifdef __ARM
	{ "1","1pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_1].power_level,M1_POWER_LEVEL , sizeof(st_cfg.mot[MOTOR_1].power_level) },
#endif
#if (MOTORS >= 2)
	{ "2","2ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_2].motor_map,	M2_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_2].motor_map) },
	{ "2","2sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_2].step_angle,	M2_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_2].step_angle) },
	{ "2","2tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_2].travel_rev,	M2_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_2].travel_rev) },
	{ "2","2mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_2].microsteps,	M2_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_2].microsteps) },
	{ "2","2po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_2].polarity,	M2_POLARITY , sizeof(st_cfg.mot[MOTOR_2].polarity) },
	{ "2","2pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_2].power_mode,	M2_POWER_MODE , sizeof(st_cfg.mot[MOTOR_2].power_mode) },
#ifdef __ARM
	{ "2","2pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_2].power_level,M2_POWER_LEVEL, sizeof(st_cfg.mot[MOTOR_2].power_level) },
#endif
#endif
#if (MOTORS >= 3)
	{ "3","3ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_3].motor_map,	M3_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_3].motor_map) },
	{ "3","3sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_3].step_angle,	M3_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_3].step_angle) },
	{ "3","3tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_3].travel_rev,	M3_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_3].travel_rev) },
	{ "3","3mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_3].microsteps,	M3_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_3].microsteps) },
	{ "3","3po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_3].polarity,	M3_POLARITY , sizeof(st_cfg.mot[MOTOR_3].polarity) },
	{ "3","3pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_3].power_mode,	M3_POWER_MODE , sizeof(st_cfg.mot[MOTOR_3].power_mode) },
#ifdef __ARM
	{ "3","3pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_3].power_level,M3_POWER_LEVEL , sizeof(st_cfg.mot[MOTOR_3].power_level) },
#endif
#endif
#if (MOTORS >= 4)
	{ "4","4ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_4].motor_map,	M4_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_4].motor_map) },
	{ "4","4sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_4].step_angle,	M4_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_4].step_angle) },
	{ "4","4tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_4].travel_rev,	M4_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_4].travel_rev) },
	{ "4","4mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_4].microsteps,	M4_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_4].microsteps) },
	{ "4","4po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_4].polarity,	M4_POLARITY , sizeof(st_cfg.mot[MOTOR_4].polarity) },
	{ "4","4pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_4].power_mode,	M4_POWER_MODE , sizeof(st_cfg.mot[MOTOR_4].power_mode) },
#ifdef __ARM
	{ "4","4pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_4].power_level,M4_POWER_LEVEL , sizeof(st_cfg.mot[MOTOR_4].power_level) },
#endif
#endif
#if (MOTORS >= 5)
	{ "5","5ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_5].motor_map,	M5_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_5].motor_map) },
	{ "5","5sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_5].step_angle,	M5_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_5].step_angle) },
	{ "5","5tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_5].travel_rev,	M5_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_5].travel_rev) },
	{ "5","5mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_5].microsteps,	M5_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_5].microsteps) },
	{ "5","5po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_5].polarity,	M5_POLARITY , sizeof(st_cfg.mot[MOTOR_5].polarity) },
	{ "5","5pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_5].power_mode,	M5_POWER_MODE , sizeof(st_cfg.mot[MOTOR_5].power_mode) },
#ifdef __ARM
	{ "5","5pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_5].power_level,M5_POWER_LEVEL , sizeof(st_cfg.mot[MOTOR_5].power_level) },
#endif
#endif
#if (MOTORS >= 6)
	{ "6","6ma",_iip,  0, st_print_ma, get_ui8, set_ui8,   (uint32_t *)&st_cfg.mot[MOTOR_6].motor_map,	M6_MOTOR_MAP , sizeof(st_cfg.mot[MOTOR_6].motor_map) },
	{ "6","6sa",_fip,  3, st_print_sa, get_flt, st_set_sa, (uint32_t *)&st_cfg.mot[MOTOR_6].step_angle,	M6_STEP_ANGLE , sizeof(st_cfg.mot[MOTOR_6].step_angle) },
	{ "6","6tr",_fipc, 4, st_print_tr, get_flt, st_set_tr, (uint32_t *)&st_cfg.mot[MOTOR_6].travel_rev,	M6_TRAVEL_PER_REV , sizeof(st_cfg.mot[MOTOR_6].travel_rev) },
	{ "6","6mi",_iip,  0, st_print_mi, get_ui8, st_set_mi, (uint32_t *)&st_cfg.mot[MOTOR_6].microsteps,	M6_MICROSTEPS , sizeof(st_cfg.mot[MOTOR_6].microsteps) },
	{ "6","6po",_bip,  0, st_print_po, get_ui8, set_01,    (uint32_t *)&st_cfg.mot[MOTOR_6].polarity,	M6_POLARITY , sizeof(st_cfg.mot[MOTOR_6].polarity) },
	{ "6","6pm",_iip,  0, st_print_pm, get_ui8, st_set_pm, (uint32_t *)&st_cfg.mot[MOTOR_6].power_mode,	M6_POWER_MODE , sizeof(st_cfg.mot[MOTOR_6].power_mode) },
#ifdef __ARM
	{ "6","6pl",_fip,  3, st_print_pl, get_flt, st_set_pl, (uint32_t *)&st_cfg.mot[MOTOR_6].power_level,M6_POWER_LEVEL , sizeof(st_cfg.mot[MOTOR_6].power_level) },
#endif
#endif
	// Axis parameters
	{ "x","xam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_X].axis_mode,		X_AXIS_MODE , sizeof(cm.a[AXIS_X].axis_mode) },
	{ "x","xvm",_fipc, 0, cm_print_vm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].velocity_max,	X_VELOCITY_MAX , sizeof(cm.a[AXIS_X].velocity_max) },
	{ "x","xfr",_fipc, 0, cm_print_fr, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].feedrate_max,	X_FEEDRATE_MAX , sizeof(cm.a[AXIS_X].feedrate_max) },
	{ "x","xtn",_fipc, 3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].travel_min,		X_TRAVEL_MIN , sizeof(cm.a[AXIS_X].travel_min) },
	{ "x","xtm",_fipc, 3, cm_print_tm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].travel_max,		X_TRAVEL_MAX , sizeof(cm.a[AXIS_X].travel_max) },
	{ "x","xjm",_fipc, 0, cm_print_jm, get_flt,   cm_set_xjm,(uint32_t *)&cm.a[AXIS_X].jerk_max,		X_JERK_MAX , sizeof(cm.a[AXIS_X].jerk_max) },
	{ "x","xjh",_fipc, 0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_X].jerk_homing,	    X_JERK_HOMING , sizeof(cm.a[AXIS_X].jerk_homing) },
	{ "x","xjd",_fipc, 4, cm_print_jd, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].junction_dev,	X_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_X].junction_dev) },
	{ "x","xsn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[0].mode,					X_SWITCH_MODE_MIN , sizeof(sw.s[0].mode) },
	{ "x","xsx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[1].mode,					X_SWITCH_MODE_MAX , sizeof(sw.s[1].mode) },
	{ "x","xsv",_fipc, 0, cm_print_sv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].search_velocity,X_SEARCH_VELOCITY , sizeof(cm.a[AXIS_X].search_velocity) },
	{ "x","xlv",_fipc, 0, cm_print_lv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].latch_velocity,	X_LATCH_VELOCITY , sizeof(cm.a[AXIS_X].latch_velocity) },
	{ "x","xlb",_fipc, 3, cm_print_lb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].latch_backoff,	X_LATCH_BACKOFF , sizeof(cm.a[AXIS_X].latch_backoff) },
	{ "x","xzb",_fipc, 3, cm_print_zb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_X].zero_backoff,	X_ZERO_BACKOFF , sizeof(cm.a[AXIS_X].zero_backoff) },

	{ "y","yam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_Y].axis_mode,		Y_AXIS_MODE , sizeof(cm.a[AXIS_Y].axis_mode) },
	{ "y","yvm",_fipc, 0, cm_print_vm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].velocity_max,	Y_VELOCITY_MAX , sizeof(cm.a[AXIS_Y].velocity_max) },
	{ "y","yfr",_fipc, 0, cm_print_fr, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].feedrate_max,	Y_FEEDRATE_MAX , sizeof(cm.a[AXIS_Y].feedrate_max) },
	{ "y","ytn",_fipc, 3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].travel_min,		Y_TRAVEL_MIN , sizeof(cm.a[AXIS_Y].travel_min) },
	{ "y","ytm",_fipc, 3, cm_print_tm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].travel_max,		Y_TRAVEL_MAX , sizeof(cm.a[AXIS_Y].travel_max) },
	{ "y","yjm",_fipc, 0, cm_print_jm, get_flt,	  cm_set_xjm,(uint32_t *)&cm.a[AXIS_Y].jerk_max,		Y_JERK_MAX , sizeof(cm.a[AXIS_Y].jerk_max) },
	{ "y","yjh",_fipc, 0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_Y].jerk_homing,	    Y_JERK_HOMING , sizeof(cm.a[AXIS_Y].jerk_homing) },
	{ "y","yjd",_fipc, 4, cm_print_jd, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].junction_dev,	Y_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_Y].junction_dev) },
	{ "y","ysn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[2].mode,					Y_SWITCH_MODE_MIN , sizeof(sw.s[2].mode) },
	{ "y","ysx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[3].mode,					Y_SWITCH_MODE_MAX , sizeof(sw.s[3].mode) },
	{ "y","ysv",_fipc, 0, cm_print_sv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].search_velocity,Y_SEARCH_VELOCITY , sizeof(cm.a[AXIS_Y].search_velocity) },
	{ "y","ylv",_fipc, 0, cm_print_lv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].latch_velocity,	Y_LATCH_VELOCITY , sizeof(cm.a[AXIS_Y].latch_velocity) },
	{ "y","ylb",_fipc, 3, cm_print_lb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].latch_backoff,	Y_LATCH_BACKOFF , sizeof(cm.a[AXIS_Y].latch_backoff) },
	{ "y","yzb",_fipc, 3, cm_print_zb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Y].zero_backoff,	Y_ZERO_BACKOFF , sizeof(cm.a[AXIS_Y].zero_backoff) },

	{ "z","zam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_Z].axis_mode,		Z_AXIS_MODE , sizeof(cm.a[AXIS_Z].axis_mode) },
	{ "z","zvm",_fipc, 0, cm_print_vm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].velocity_max,	Z_VELOCITY_MAX , sizeof(cm.a[AXIS_Z].velocity_max) },
	{ "z","zfr",_fipc, 0, cm_print_fr, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].feedrate_max,	Z_FEEDRATE_MAX , sizeof(cm.a[AXIS_Z].feedrate_max) },
	{ "z","ztn",_fipc, 3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].travel_min,		Z_TRAVEL_MIN , sizeof(cm.a[AXIS_Z].travel_min) },
	{ "z","ztm",_fipc, 3, cm_print_tm, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].travel_max,		Z_TRAVEL_MAX , sizeof(cm.a[AXIS_Z].travel_max) },
	{ "z","zjm",_fipc, 0, cm_print_jm, get_flt,	  cm_set_xjm,(uint32_t *)&cm.a[AXIS_Z].jerk_max,		Z_JERK_MAX , sizeof(cm.a[AXIS_Z].jerk_max) },
	{ "z","zjh",_fipc, 0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_Z].jerk_homing, 	Z_JERK_HOMING , sizeof(cm.a[AXIS_Z].jerk_homing) },
	{ "z","zjd",_fipc, 4, cm_print_jd, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].junction_dev,	Z_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_Z].junction_dev) },
	{ "z","zsn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[4].mode,					Z_SWITCH_MODE_MIN , sizeof(sw.s[4].mode) },
	{ "z","zsx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[5].mode,					Z_SWITCH_MODE_MAX , sizeof(sw.s[5].mode) },
	{ "z","zsv",_fipc, 0, cm_print_sv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].search_velocity,Z_SEARCH_VELOCITY , sizeof(cm.a[AXIS_Z].search_velocity) },
	{ "z","zlv",_fipc, 0, cm_print_lv, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].latch_velocity,	Z_LATCH_VELOCITY , sizeof(cm.a[AXIS_Z].latch_velocity) },
	{ "z","zlb",_fipc, 3, cm_print_lb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].latch_backoff,	Z_LATCH_BACKOFF , sizeof(cm.a[AXIS_Z].latch_backoff) },
	{ "z","zzb",_fipc, 3, cm_print_zb, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_Z].zero_backoff,	Z_ZERO_BACKOFF , sizeof(cm.a[AXIS_Z].zero_backoff) },

	{ "a","aam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_A].axis_mode,		A_AXIS_MODE , sizeof(cm.a[AXIS_A].axis_mode) },
	{ "a","avm",_fip,  0, cm_print_vm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].velocity_max,	A_VELOCITY_MAX , sizeof(cm.a[AXIS_A].velocity_max) },
	{ "a","afr",_fip,  0, cm_print_fr, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].feedrate_max,	A_FEEDRATE_MAX , sizeof(cm.a[AXIS_A].feedrate_max) },
	{ "a","atn",_fip,  3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_A].travel_min,		A_TRAVEL_MIN , sizeof(cm.a[AXIS_A].travel_min) },
	{ "a","atm",_fip,  3, cm_print_tm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].travel_max,		A_TRAVEL_MAX , sizeof(cm.a[AXIS_A].travel_max) },
	{ "a","ajm",_fip,  0, cm_print_jm, get_flt,	  cm_set_xjm,(uint32_t *)&cm.a[AXIS_A].jerk_max,		A_JERK_MAX , sizeof(cm.a[AXIS_A].jerk_max) },
	{ "a","ajh",_fip,  0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_A].jerk_homing, 	A_JERK_HOMING , sizeof(cm.a[AXIS_A].jerk_homing) },
	{ "a","ajd",_fip,  4, cm_print_jd, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].junction_dev,	A_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_A].junction_dev) },
	{ "a","ara",_fipc, 3, cm_print_ra, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].radius,			A_RADIUS, sizeof(cm.a[AXIS_A].radius) },
	{ "a","asn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[6].mode,					A_SWITCH_MODE_MIN , sizeof(sw.s[6].mode) },
	{ "a","asx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[7].mode,					A_SWITCH_MODE_MAX , sizeof(sw.s[7].mode) },
	{ "a","asv",_fip,  0, cm_print_sv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].search_velocity,A_SEARCH_VELOCITY , sizeof(cm.a[AXIS_A].search_velocity) },
	{ "a","alv",_fip,  0, cm_print_lv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].latch_velocity,	A_LATCH_VELOCITY , sizeof(cm.a[AXIS_A].latch_velocity) },
	{ "a","alb",_fip,  3, cm_print_lb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].latch_backoff,	A_LATCH_BACKOFF , sizeof(cm.a[AXIS_A].latch_backoff) },
	{ "a","azb",_fip,  3, cm_print_zb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_A].zero_backoff,	A_ZERO_BACKOFF , sizeof(cm.a[AXIS_A].zero_backoff) },

	{ "b","bam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_B].axis_mode,		B_AXIS_MODE , sizeof(cm.a[AXIS_B].axis_mode) },
	{ "b","bvm",_fip,  0, cm_print_vm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].velocity_max,	B_VELOCITY_MAX , sizeof(cm.a[AXIS_B].velocity_max) },
	{ "b","bfr",_fip,  0, cm_print_fr, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].feedrate_max,	B_FEEDRATE_MAX , sizeof(cm.a[AXIS_B].feedrate_max) },
	{ "b","btn",_fip,  3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_B].travel_min,		B_TRAVEL_MIN , sizeof(cm.a[AXIS_B].travel_min) },
	{ "b","btm",_fip,  3, cm_print_tm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].travel_max,		B_TRAVEL_MAX , sizeof(cm.a[AXIS_B].travel_max) },
	{ "b","bjm",_fip,  0, cm_print_jm, get_flt,	  cm_set_xjm,(uint32_t *)&cm.a[AXIS_B].jerk_max,		B_JERK_MAX , sizeof(cm.a[AXIS_B].jerk_max) },
	{ "b","bjd",_fip,  0, cm_print_jd, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].junction_dev,	B_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_B].junction_dev) },
	{ "b","bra",_fipc, 3, cm_print_ra, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].radius,			B_RADIUS , sizeof(cm.a[AXIS_B].radius) },

#ifdef __ARM	// B axis extended parameters
	{ "b","asn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[AXIS_B][SW_MIN].mode,	B_SWITCH_MODE_MIN , sizeof(sw.s[AXIS_B][SW_MIN].mode) },
	{ "b","asx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[AXIS_B][SW_MAX].mode,	B_SWITCH_MODE_MAX , sizeof(sw.s[AXIS_B][SW_MAX].mode) },
	{ "b","bsv",_fip,  0, cm_print_sv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].search_velocity,B_SEARCH_VELOCITY , sizeof(cm.a[AXIS_B].search_velocity) },
	{ "b","blv",_fip,  0, cm_print_lv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].latch_velocity,	B_LATCH_VELOCITY , sizeof(cm.a[AXIS_B].latch_velocity) },
	{ "b","blb",_fip,  3, cm_print_lb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].latch_backoff,	B_LATCH_BACKOFF , sizeof(cm.a[AXIS_B].latch_backoff) },
	{ "b","bzb",_fip,  3, cm_print_zb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_B].zero_backoff,	B_ZERO_BACKOFF , sizeof(cm.a[AXIS_B].zero_backoff) },
	{ "b","bjh",_fip,  0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_B].jerk_homing,	B_JERK_HOMING , sizeof(cm.a[AXIS_B].jerk_homing) },
#endif

	{ "c","cam",_iip,  0, cm_print_am, cm_get_am, cm_set_am, (uint32_t *)&cm.a[AXIS_C].axis_mode,		C_AXIS_MODE , sizeof(cm.a[AXIS_C].axis_mode) },
	{ "c","cvm",_fip,  0, cm_print_vm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].velocity_max,	C_VELOCITY_MAX , sizeof(cm.a[AXIS_C].velocity_max) },
	{ "c","cfr",_fip,  0, cm_print_fr, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].feedrate_max,	C_FEEDRATE_MAX , sizeof(cm.a[AXIS_C].feedrate_max) },
	{ "c","ctn",_fip,  3, cm_print_tn, get_flt,   set_flu,   (uint32_t *)&cm.a[AXIS_C].travel_min,		C_TRAVEL_MIN , sizeof(cm.a[AXIS_C].travel_min) },
	{ "c","ctm",_fip,  3, cm_print_tm, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].travel_max,		C_TRAVEL_MAX , sizeof(cm.a[AXIS_C].travel_max) },
	{ "c","cjm",_fip,  0, cm_print_jm, get_flt,	  cm_set_xjm,(uint32_t *)&cm.a[AXIS_C].jerk_max,		C_JERK_MAX , sizeof(cm.a[AXIS_C].jerk_max) },
	{ "c","cjd",_fip,  0, cm_print_jd, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].junction_dev,	C_JUNCTION_DEVIATION , sizeof(cm.a[AXIS_C].junction_dev) },
	{ "c","cra",_fipc, 3, cm_print_ra, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].radius,			C_RADIUS , sizeof(cm.a[AXIS_C].radius) },
#ifdef __ARM	// C axis extended parameters
	{ "c","csn",_iip,  0, cm_print_sn, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[AXIS_C][SW_MIN].mode,	C_SWITCH_MODE_MIN , sizeof(sw.s[AXIS_C][SW_MIN].mode) },
	{ "c","csx",_iip,  0, cm_print_sx, get_ui8,   sw_set_sw, (uint32_t *)&sw.s[AXIS_C][SW_MAX].mode,	C_SWITCH_MODE_MAX , sizeof(sw.s[AXIS_C][SW_MAX].mode) },
	{ "c","csv",_fip,  0, cm_print_sv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].search_velocity,C_SEARCH_VELOCITY , sizeof(cm.a[AXIS_C].search_velocity) },
	{ "c","clv",_fip,  0, cm_print_lv, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].latch_velocity,	C_LATCH_VELOCITY , sizeof(cm.a[AXIS_C].latch_velocity) },
	{ "c","clb",_fip,  3, cm_print_lb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].latch_backoff,	C_LATCH_BACKOFF , sizeof(cm.a[AXIS_C].latch_backoff) },
	{ "c","czb",_fip,  3, cm_print_zb, get_flt,   set_flt,   (uint32_t *)&cm.a[AXIS_C].zero_backoff,	C_ZERO_BACKOFF , sizeof(cm.a[AXIS_C].zero_backoff) },
	{ "c","cjh",_fip,  0, cm_print_jh, get_flt,	  cm_set_xjh,(uint32_t *)&cm.a[AXIS_C].jerk_homing, 	C_JERK_HOMING , sizeof(cm.a[AXIS_C].jerk_homing) },
#endif

	// PWM settings
	{ "p1","p1frq",_fip, 0, pwm_print_p1frq, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].frequency,		P1_PWM_FREQUENCY , sizeof(pwm.c[PWM_1].frequency) },
	{ "p1","p1csl",_fip, 0, pwm_print_p1csl, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].cw_speed_lo,	P1_CW_SPEED_LO , sizeof(pwm.c[PWM_1].cw_speed_lo) },
	{ "p1","p1csh",_fip, 0, pwm_print_p1csh, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].cw_speed_hi,	P1_CW_SPEED_HI , sizeof(pwm.c[PWM_1].cw_speed_hi) },
	{ "p1","p1cpl",_fip, 3, pwm_print_p1cpl, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].cw_phase_lo,	P1_CW_PHASE_LO , sizeof(pwm.c[PWM_1].cw_phase_lo) },
	{ "p1","p1cph",_fip, 3, pwm_print_p1cph, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].cw_phase_hi,	P1_CW_PHASE_HI , sizeof(pwm.c[PWM_1].cw_phase_hi) },
	{ "p1","p1wsl",_fip, 0, pwm_print_p1wsl, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].ccw_speed_lo,	P1_CCW_SPEED_LO , sizeof(pwm.c[PWM_1].ccw_speed_lo) },
	{ "p1","p1wsh",_fip, 0, pwm_print_p1wsh, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].ccw_speed_hi,	P1_CCW_SPEED_HI , sizeof(pwm.c[PWM_1].ccw_speed_hi) },
	{ "p1","p1wpl",_fip, 3, pwm_print_p1wpl, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].ccw_phase_lo,	P1_CCW_PHASE_LO , sizeof(pwm.c[PWM_1].ccw_phase_lo) },
	{ "p1","p1wph",_fip, 3, pwm_print_p1wph, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].ccw_phase_hi,	P1_CCW_PHASE_HI , sizeof(pwm.c[PWM_1].ccw_phase_hi) },
	{ "p1","p1pof",_fip, 3, pwm_print_p1pof, get_flt, set_flt,(uint32_t *)&pwm.c[PWM_1].phase_off,		P1_PWM_PHASE_OFF , sizeof(pwm.c[PWM_1].phase_off) },

	// Coordinate system offsets (G54-G59 and G92)
	{ "g54","g54x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G54][AXIS_X], G54_X_OFFSET , sizeof(cm.offset[G54][AXIS_X]) },
	{ "g54","g54y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G54][AXIS_Y], G54_Y_OFFSET , sizeof(cm.offset[G54][AXIS_Y]) },
	{ "g54","g54z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G54][AXIS_Z], G54_Z_OFFSET , sizeof(cm.offset[G54][AXIS_Z]) },
	{ "g54","g54a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G54][AXIS_A], G54_A_OFFSET , sizeof(cm.offset[G54][AXIS_A]) },
	{ "g54","g54b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G54][AXIS_B], G54_B_OFFSET , sizeof(cm.offset[G54][AXIS_B]) },
	{ "g54","g54c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G54][AXIS_C], G54_C_OFFSET , sizeof(cm.offset[G54][AXIS_C]) },

	{ "g55","g55x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G55][AXIS_X], G55_X_OFFSET , sizeof(cm.offset[G55][AXIS_X]) },
	{ "g55","g55y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G55][AXIS_Y], G55_Y_OFFSET , sizeof(cm.offset[G55][AXIS_Y]) },
	{ "g55","g55z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G55][AXIS_Z], G55_Z_OFFSET , sizeof(cm.offset[G55][AXIS_Z]) },
	{ "g55","g55a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G55][AXIS_A], G55_A_OFFSET , sizeof(cm.offset[G55][AXIS_A]) },
	{ "g55","g55b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G55][AXIS_B], G55_B_OFFSET , sizeof(cm.offset[G55][AXIS_B]) },
	{ "g55","g55c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G55][AXIS_C], G55_C_OFFSET , sizeof(cm.offset[G55][AXIS_C]) },

	{ "g56","g56x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G56][AXIS_X], G56_X_OFFSET , sizeof(cm.offset[G56][AXIS_X]) },
	{ "g56","g56y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G56][AXIS_Y], G56_Y_OFFSET , sizeof(cm.offset[G56][AXIS_Y]) },
	{ "g56","g56z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G56][AXIS_Z], G56_Z_OFFSET , sizeof(cm.offset[G56][AXIS_Z]) },
	{ "g56","g56a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G56][AXIS_A], G56_A_OFFSET , sizeof(cm.offset[G56][AXIS_A]) },
	{ "g56","g56b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G56][AXIS_B], G56_B_OFFSET , sizeof(cm.offset[G56][AXIS_B]) },
	{ "g56","g56c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G56][AXIS_C], G56_C_OFFSET , sizeof(cm.offset[G56][AXIS_C]) },

	{ "g57","g57x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G57][AXIS_X], G57_X_OFFSET , sizeof(cm.offset[G57][AXIS_X]) },
	{ "g57","g57y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G57][AXIS_Y], G57_Y_OFFSET , sizeof(cm.offset[G57][AXIS_Y]) },
	{ "g57","g57z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G57][AXIS_Z], G57_Z_OFFSET , sizeof(cm.offset[G57][AXIS_Z]) },
	{ "g57","g57a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G57][AXIS_A], G57_A_OFFSET , sizeof(cm.offset[G57][AXIS_A]) },
	{ "g57","g57b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G57][AXIS_B], G57_B_OFFSET , sizeof(cm.offset[G57][AXIS_B]) },
	{ "g57","g57c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G57][AXIS_C], G57_C_OFFSET , sizeof(cm.offset[G57][AXIS_C]) },

	{ "g58","g58x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G58][AXIS_X], G58_X_OFFSET , sizeof(cm.offset[G58][AXIS_X]) },
	{ "g58","g58y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G58][AXIS_Y], G58_Y_OFFSET , sizeof(cm.offset[G58][AXIS_Y]) },
	{ "g58","g58z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G58][AXIS_Z], G58_Z_OFFSET , sizeof(cm.offset[G58][AXIS_Z]) },
	{ "g58","g58a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G58][AXIS_A], G58_A_OFFSET , sizeof(cm.offset[G58][AXIS_A]) },
	{ "g58","g58b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G58][AXIS_B], G58_B_OFFSET , sizeof(cm.offset[G58][AXIS_B]) },
	{ "g58","g58c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G58][AXIS_C], G58_C_OFFSET , sizeof(cm.offset[G58][AXIS_C]) },

	{ "g59","g59x",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G59][AXIS_X], G59_X_OFFSET , sizeof(cm.offset[G59][AXIS_X]) },
	{ "g59","g59y",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G59][AXIS_Y], G59_Y_OFFSET , sizeof(cm.offset[G59][AXIS_Y]) },
	{ "g59","g59z",_fipc, 3, cm_print_cofs, get_flt, set_flu,(uint32_t *)&cm.offset[G59][AXIS_Z], G59_Z_OFFSET , sizeof(cm.offset[G59][AXIS_Z]) },
	{ "g59","g59a",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G59][AXIS_A], G59_A_OFFSET , sizeof(cm.offset[G59][AXIS_A]) },
	{ "g59","g59b",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G59][AXIS_B], G59_B_OFFSET , sizeof(cm.offset[G59][AXIS_B]) },
	{ "g59","g59c",_fip,  3, cm_print_cofs, get_flt, set_flt,(uint32_t *)&cm.offset[G59][AXIS_C], G59_C_OFFSET , sizeof(cm.offset[G59][AXIS_C]) },

	{ "g92","g92x",_fic, 3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_X], 0 , sizeof(cm.gmx.origin_offset[AXIS_X]) },// G92 handled differently
	{ "g92","g92y",_fic, 3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_Y], 0 , sizeof(cm.gmx.origin_offset[AXIS_Y]) },
	{ "g92","g92z",_fic, 3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_Z], 0 , sizeof(cm.gmx.origin_offset[AXIS_Z]) },
	{ "g92","g92a",_fi,  3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_A], 0 , sizeof(cm.gmx.origin_offset[AXIS_A]) },
	{ "g92","g92b",_fi,  3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_B], 0 , sizeof(cm.gmx.origin_offset[AXIS_B]) },
	{ "g92","g92c",_fi,  3, cm_print_cofs, get_flt, set_nul,(uint32_t *)&cm.gmx.origin_offset[AXIS_C], 0 , sizeof(cm.gmx.origin_offset[AXIS_C]) },

	// Coordinate positions (G28, G30)
	{ "g28","g28x",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_X], 0 , sizeof(cm.gmx.g28_position[AXIS_X]) },// g28 handled differently
	{ "g28","g28y",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_Y], 0 , sizeof(cm.gmx.g28_position[AXIS_Y]) },
	{ "g28","g28z",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_Z], 0 , sizeof(cm.gmx.g28_position[AXIS_Z]) },
	{ "g28","g28a",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_A], 0 , sizeof(cm.gmx.g28_position[AXIS_A]) },
	{ "g28","g28b",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_B], 0 , sizeof(cm.gmx.g28_position[AXIS_B]) },
	{ "g28","g28c",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g28_position[AXIS_C], 0 , sizeof(cm.gmx.g28_position[AXIS_C]) },

	{ "g30","g30x",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_X], 0 , sizeof(cm.gmx.g30_position[AXIS_X]) },// g30 handled differently
	{ "g30","g30y",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_Y], 0 , sizeof(cm.gmx.g30_position[AXIS_Y]) },
	{ "g30","g30z",_fic, 3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_Z], 0 , sizeof(cm.gmx.g30_position[AXIS_Z]) },
	{ "g30","g30a",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_A], 0 , sizeof(cm.gmx.g30_position[AXIS_A]) },
	{ "g30","g30b",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_B], 0 , sizeof(cm.gmx.g30_position[AXIS_B]) },
	{ "g30","g30c",_fi,  3, cm_print_cpos, get_flt, set_nul,(uint32_t *)&cm.gmx.g30_position[AXIS_C], 0 , sizeof(cm.gmx.g30_position[AXIS_C]) },

	// this is a 128bit UUID for identifying a previously committed job state
	{ "jid","jida",_d0, 0, tx_print_nul, get_data, set_data, (uint32_t *)&cs.job_id[0], 0, sizeof(cs.job_id[0]) },
	{ "jid","jidb",_d0, 0, tx_print_nul, get_data, set_data, (uint32_t *)&cs.job_id[1], 0, sizeof(cs.job_id[1]) },
	{ "jid","jidc",_d0, 0, tx_print_nul, get_data, set_data, (uint32_t *)&cs.job_id[2], 0, sizeof(cs.job_id[2]) },
	{ "jid","jidd",_d0, 0, tx_print_nul, get_data, set_data, (uint32_t *)&cs.job_id[3], 0, sizeof(cs.job_id[3]) },

	// System parameters
	{ "sys","ja",  _fipnc,0, cm_print_ja,  get_flt,   set_flu,    (uint32_t *)&cm.junction_acceleration,   JUNCTION_ACCELERATION , sizeof(cm.junction_acceleration) },
	{ "sys","ct",  _fipnc,4, cm_print_ct,  get_flt,   set_flu,    (uint32_t *)&cm.chordal_tolerance,	   CHORDAL_TOLERANCE , sizeof(cm.chordal_tolerance) },
	{ "sys","sl",  _iipn, 0, cm_print_sl,  get_ui8,   set_ui8,    (uint32_t *)&cm.soft_limit_enable,	   SOFT_LIMIT_ENABLE , sizeof(cm.soft_limit_enable) },
	{ "sys","st",  _iipn, 0, sw_print_st,  get_ui8,   sw_set_st,  (uint32_t *)&sw.switch_type,			   SWITCH_TYPE , sizeof(sw.switch_type) },
	{ "sys","mt",  _fipn, 2, st_print_mt,  get_flt,   st_set_mt,  (uint32_t *)&st_cfg.motor_power_timeout, MOTOR_IDLE_TIMEOUT, sizeof(st_cfg.motor_power_timeout) },
	{ "",   "me",  _i0,   0, tx_print_str, st_set_me, st_set_me,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },
	{ "",   "md",  _i0,   0, tx_print_str, st_set_md, st_set_md,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },

	{ "sys","ej",  _iipn, 0, js_print_ej,  get_ui8,   set_01,     (uint32_t *)&cs.comm_mode,			   COMM_MODE , sizeof(cs.comm_mode) },
	{ "sys","jv",  _iipn, 0, js_print_jv,  get_ui8,   json_set_jv,(uint32_t *)&js.json_verbosity,		   JSON_VERBOSITY , sizeof(js.json_verbosity) },
	{ "sys","js",  _iipn, 0, js_print_js,  get_ui8,   set_01,     (uint32_t *)&js.json_syntax, 		       JSON_SYNTAX , sizeof(js.json_syntax) },
	{ "sys","tv",  _iipn, 0, tx_print_tv,  get_ui8,   set_01,     (uint32_t *)&txt.text_verbosity,		   TEXT_VERBOSITY , sizeof(txt.text_verbosity) },
	{ "sys","qv",  _iipn, 0, qr_print_qv,  get_ui8,   set_0123,   (uint32_t *)&qr.queue_report_verbosity,  QUEUE_REPORT_VERBOSITY , sizeof(qr.queue_report_verbosity) },
	{ "sys","sv",  _iipn, 0, sr_print_sv,  get_ui8,   set_012,    (uint32_t *)&sr.status_report_verbosity, STATUS_REPORT_VERBOSITY , sizeof(sr.status_report_verbosity) },
	{ "sys","si",  _iipn, 0, sr_print_si,  get_u32,   sr_set_si,  (uint32_t *)&sr.status_report_interval,  STATUS_REPORT_INTERVAL_MS , sizeof(sr.status_report_interval) },

	{ "sys","ec",  _iipn, 0, cfg_print_ec,  get_ui8,   set_ec,     (uint32_t *)&xio.enable_cr,			 XIO_EXPAND_CR , sizeof(xio.enable_cr) },
	{ "sys","ee",  _iipn, 0, cfg_print_ee,  get_ui8,   set_ee,     (uint32_t *)&xio.enable_echo,		 XIO_ENABLE_ECHO , sizeof(xio.enable_echo) },
	{ "sys","ex",  _iipn, 0, cfg_print_ex,  get_ui8,   set_ex,     (uint32_t *)&xio.enable_flow_control, XIO_ENABLE_FLOW_CONTROL , sizeof(xio.enable_flow_control) },
	{ "sys","rxm", _iipn, 0, cfg_print_rxm, get_ui8,   set_01,     (uint32_t *)&xio.rx_mode,             XIO_RX_MODE , sizeof(xio.rx_mode) },
	{ "sys","baud",_in,   0, cfg_print_baud,get_ui8,   set_baud,   (uint32_t *)&xio.usb_baud_rate,		 XIO_BAUD_115200 , sizeof(xio.usb_baud_rate) },

    // Actions and reports
	{ "", "qr",  _n0, 0, qr_print_qr,  qr_get,  set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// queue report - planner buffers available
	{ "", "qi",  _n0, 0, qr_print_qi,  qi_get,  set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// queue report - buffers added to queue
	{ "", "qo",  _n0, 0, qr_print_qo,  qo_get,  set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// queue report - buffers removed from queue
	{ "", "er",  _n0, 0, tx_print_nul, rpt_er,  set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// invoke bogus exception report for testing
	{ "", "qf",  _n0, 0, tx_print_nul, get_nul, cm_run_qf,(uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// queue flush
	{ "", "rx",  _n0, 0, tx_print_int, get_rx,  set_nul,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// bytes or lines in RX buffer
	{ "", "msg", _s0, 0, tx_print_str, get_nul, set_str,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// string for generic messages
    { "", "alarm",_n0,0, tx_print_nul, cm_alrm, cm_alrm,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // trigger alarm
    { "", "panic",_n0,0, tx_print_nul, cm_pnic, cm_pnic,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // trigger panic
    { "", "shutd",_n0,0, tx_print_nul, cm_shutd,cm_shutd, (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // trigger shutdown
    { "", "clear",_n0,0, tx_print_nul, cm_clr,  cm_clr,   (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // GET "clear" to clear alarm state
    { "", "clr",  _n0,0, tx_print_nul, cm_clr,  cm_clr,   (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // synonym for "clear"

    // status report keys
	{ "", "set", _nn0,0, tx_print_nul, get_nul, set_not,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// used only to validate key. No actions
	{ "", "srs", _n0, 0, sr_print_sr,  srs_get, srs_set,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// status report set and query SR list
	{ "", "sr",  _b0, 0, sr_print_sr,  sr_get,  sr_set,   (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// invoke status report (and old set function)

    // special functions
	{ "", "txt", _t0, 0, tx_print_nul, get_nul, set_not,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // Text wrapper (container)
	{ "", "tid", _fi, 0, tx_print_int, get_nul, set_not,  (uint32_t *)&cs.null, 0 , sizeof(cs.null) },    // Transaction id
	{ "", "test",_b0, 0, tx_print_nul, help_test, run_test, (uint32_t *)&cs.null,0 , sizeof(cs.null) },	// run tests, print test help screen
	{ "", "defa",_b0, 0, tx_print_nul, help_defa, set_defaults,(uint32_t *)&cs.null,0 , sizeof(cs.null) },// set defaults / help screen
	{ "", "boot",_b0, 0, tx_print_nul, help_boot, hw_run_boot, (uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "", "help",_n0, 0, tx_print_nul, help_config, help_config, (uint32_t *)&cs.null,0 , sizeof(cs.null) },  // prints config help screen
	{ "", "h",   _n0, 0, tx_print_nul, help_config, help_config, (uint32_t *)&cs.null,0 , sizeof(cs.null) },  // alias for "help"

	// switch readers (inX)
	{ "in","in1",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[0].state, 0 , sizeof(sw.s[0].state) },
	{ "in","in2",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[1].state, 0 , sizeof(sw.s[1].state) },
	{ "in","in3",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[2].state, 0 , sizeof(sw.s[2].state) },
	{ "in","in4",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[3].state, 0 , sizeof(sw.s[3].state) },
	{ "in","in5",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[4].state, 0 , sizeof(sw.s[4].state) },
	{ "in","in6",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[5].state, 0 , sizeof(sw.s[5].state) },
	{ "in","in7",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[6].state, 0 , sizeof(sw.s[6].state) },
	{ "in","in8",  _f0, 0, sw_print_in, get_int, set_nul, (uint32_t *)&sw.s[7].state, 0 , sizeof(sw.s[7].state) },

	// NOTE: The ordering within the gcode defaults is important for token resolution
	{ "sys","gpl", _iipn, 0, cm_print_gpl, get_ui8, set_012, (uint32_t *)&cm.default_select_plane,	GCODE_DEFAULT_PLANE , sizeof(cm.default_select_plane) },
	{ "sys","gun", _iipn, 0, cm_print_gun, get_ui8, set_01,  (uint32_t *)&cm.default_units_mode,	GCODE_DEFAULT_UNITS , sizeof(cm.default_units_mode) },
	{ "sys","gco", _iipn, 0, cm_print_gco, get_ui8, set_ui8, (uint32_t *)&cm.default_coord_system,	GCODE_DEFAULT_COORD_SYSTEM , sizeof(cm.default_coord_system) },
	{ "sys","gpa", _iipn, 0, cm_print_gpa, get_ui8, set_012, (uint32_t *)&cm.default_path_control,	GCODE_DEFAULT_PATH_CONTROL , sizeof(cm.default_path_control) },
	{ "sys","gdi", _iipn, 0, cm_print_gdi, get_ui8, set_01,  (uint32_t *)&cm.default_distance_mode, GCODE_DEFAULT_DISTANCE_MODE , sizeof(cm.default_distance_mode) },
	{ "",   "gc",  _n0,   0, tx_print_nul, get_nul, gc_run_gc,(uint32_t *)&cs.null, 0 , sizeof(cs.null) }, // gcode block - must be last in this group

	// 用户定义的数据组
	{ "uda","uda0", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_a[0], USER_DATA_A0 , sizeof(cfg.user_data_a[0]) },
	{ "uda","uda1", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_a[1], USER_DATA_A1 , sizeof(cfg.user_data_a[1]) },
	{ "uda","uda2", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_a[2], USER_DATA_A2 , sizeof(cfg.user_data_a[2]) },
	{ "uda","uda3", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_a[3], USER_DATA_A3 , sizeof(cfg.user_data_a[3]) },

	{ "udb","udb0", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_b[0], USER_DATA_B0 , sizeof(cfg.user_data_b[0]) },
	{ "udb","udb1", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_b[1], USER_DATA_B1 , sizeof(cfg.user_data_b[1]) },
	{ "udb","udb2", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_b[2], USER_DATA_B2 , sizeof(cfg.user_data_b[2]) },
	{ "udb","udb3", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_b[3], USER_DATA_B3 , sizeof(cfg.user_data_b[3]) },

	{ "udc","udc0", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_c[0], USER_DATA_C0 , sizeof(cfg.user_data_c[0]) },
	{ "udc","udc1", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_c[1], USER_DATA_C1 , sizeof(cfg.user_data_c[1]) },
	{ "udc","udc2", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_c[2], USER_DATA_C2 , sizeof(cfg.user_data_c[2]) },
	{ "udc","udc3", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_c[3], USER_DATA_C3 , sizeof(cfg.user_data_c[3]) },

	{ "udd","udd0", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_d[0], USER_DATA_D0 , sizeof(cfg.user_data_d[0]) },
	{ "udd","udd1", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_d[1], USER_DATA_D1 , sizeof(cfg.user_data_d[1]) },
	{ "udd","udd2", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_d[2], USER_DATA_D2 , sizeof(cfg.user_data_d[2]) },
	{ "udd","udd3", _dip, 0, tx_print_int, get_data, set_data,(uint32_t *)&cfg.user_data_d[3], USER_DATA_D3 , sizeof(cfg.user_data_d[3]) },

	// 诊断参数
#ifdef __DIAGNOSTIC_PARAMETERS
	{ "_te","_tex",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_X], 0 , sizeof(mr.target[AXIS_X]) },			// X target endpoint
	{ "_te","_tey",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_Y], 0 , sizeof(mr.target[AXIS_Y]) },
	{ "_te","_tez",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_Z], 0 , sizeof(mr.target[AXIS_Z]) },
	{ "_te","_tea",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_A], 0 , sizeof(mr.target[AXIS_A]) },
	{ "_te","_teb",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_B], 0 , sizeof(mr.target[AXIS_B]) },
	{ "_te","_tec",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target[AXIS_C], 0 , sizeof(mr.target[AXIS_C]) },

	{ "_tr","_trx",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_X], 0 , sizeof(mr.gm.target[AXIS_X]) },			// X target runtime
	{ "_tr","_try",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_Y], 0 , sizeof(mr.gm.target[AXIS_Y]) },
	{ "_tr","_trz",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_Z], 0 , sizeof(mr.gm.target[AXIS_Z]) },
	{ "_tr","_tra",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_A], 0 , sizeof(mr.gm.target[AXIS_A]) },
	{ "_tr","_trb",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_B], 0 , sizeof(mr.gm.target[AXIS_B]) },
	{ "_tr","_trc",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.gm.target[AXIS_C], 0 , sizeof(mr.gm.target[AXIS_C]) },

#if (MOTORS >= 1)
	{ "_ts","_ts1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_1], 0 , sizeof(mr.target_steps[MOTOR_1]) },		// Motor 1 target steps
	{ "_ps","_ps1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_1], 0 , sizeof(mr.position_steps[MOTOR_1]) },	// Motor 1 position steps
	{ "_cs","_cs1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_1], 0 , sizeof(mr.commanded_steps[MOTOR_1]) },	// Motor 1 commanded steps (delayed steps)
	{ "_es","_es1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_1], 0 , sizeof(mr.encoder_steps[MOTOR_1]) },	// Motor 1 encoder steps
	{ "_xs","_xs1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_1].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_1].corrected_steps) }, // Motor 1 correction steps applied
	{ "_fe","_fe1",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_1], 0 , sizeof(mr.following_error[MOTOR_1]) },	// Motor 1 following error in steps
#endif
#if (MOTORS >= 2)
	{ "_ts","_ts2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_2], 0 , sizeof(mr.target_steps[MOTOR_2]) },
	{ "_ps","_ps2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_2], 0 , sizeof(mr.position_steps[MOTOR_2]) },
	{ "_cs","_cs2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_2], 0 , sizeof(mr.commanded_steps[MOTOR_2]) },
	{ "_es","_es2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_2], 0 , sizeof(mr.encoder_steps[MOTOR_2]) },
	{ "_xs","_xs2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_2].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_2].corrected_steps) },
	{ "_fe","_fe2",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_2], 0 , sizeof(mr.following_error[MOTOR_2]) },
#endif
#if (MOTORS >= 3)
	{ "_ts","_ts3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_3], 0 , sizeof(mr.target_steps[MOTOR_3]) },
	{ "_ps","_ps3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_3], 0 , sizeof(mr.position_steps[MOTOR_3]) },
	{ "_cs","_cs3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_3], 0 , sizeof(mr.commanded_steps[MOTOR_3]) },
	{ "_es","_es3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_3], 0 , sizeof(mr.encoder_steps[MOTOR_3]) },
	{ "_xs","_xs3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_3].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_3].corrected_steps) },
	{ "_fe","_fe3",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_3], 0 , sizeof(mr.following_error[MOTOR_3]) },
#endif
#if (MOTORS >= 4)
	{ "_ts","_ts4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_4], 0 , sizeof(mr.target_steps[MOTOR_4]) },
	{ "_ps","_ps4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_4], 0 , sizeof(mr.position_steps[MOTOR_4]) },
	{ "_cs","_cs4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_4], 0 , sizeof(mr.commanded_steps[MOTOR_4]) },
	{ "_es","_es4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_4], 0 , sizeof(mr.encoder_steps[MOTOR_4]) },
	{ "_xs","_xs4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_4].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_4].corrected_steps) },
	{ "_fe","_fe4",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_4], 0 , sizeof(mr.following_error[MOTOR_4]) },
#endif
#if (MOTORS >= 5)
	{ "_ts","_ts5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_5], 0 , sizeof(mr.target_steps[MOTOR_5]) },
	{ "_ps","_ps5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_5], 0 , sizeof(mr.position_steps[MOTOR_5]) },
	{ "_cs","_cs5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_5], 0 , sizeof(mr.commanded_steps[MOTOR_5]) },
	{ "_es","_es5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_5], 0 , sizeof(mr.encoder_steps[MOTOR_5]) },
	{ "_xs","_xs6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_6].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_6].corrected_steps) },
	{ "_fe","_fe5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_5], 0 , sizeof(mr.following_error[MOTOR_5]) },
#endif
#if (MOTORS >= 6)
	{ "_ts","_ts6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.target_steps[MOTOR_6], 0 , sizeof(mr.target_steps[MOTOR_6]) },
	{ "_ps","_ps6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.position_steps[MOTOR_6], 0 , sizeof(mr.position_steps[MOTOR_6]) },
	{ "_cs","_cs6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.commanded_steps[MOTOR_6], 0 , sizeof(mr.commanded_steps[MOTOR_6]) },
	{ "_es","_es6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.encoder_steps[MOTOR_6], 0 , sizeof(mr.encoder_steps[MOTOR_6]) },
	{ "_xs","_xs5",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&st_pre.mot[MOTOR_5].corrected_steps, 0 , sizeof(st_pre.mot[MOTOR_5].corrected_steps) },
	{ "_fe","_fe6",_f0, 2, tx_print_flt, get_flt, set_nul,(uint32_t *)&mr.following_error[MOTOR_6], 0 , sizeof(mr.following_error[MOTOR_6]) },
#endif
//	{ "",   "_dam",_n0, 0, tx_print_nul, cm_dam,  cm_dam, (uint32_t *)&cs.null, 0 , sizeof(cs.null) },	// dump active model
#endif	//  __DIAGNOSTIC_PARAMETERS

	// Persistence for status report - must be in sequence
	// *** Count must agree with NV_STATUS_REPORT_LEN in config.h ***
	{ "","se00",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[0], 0 , sizeof(sr.status_report_list[0]) },
	{ "","se01",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[1], 0 , sizeof(sr.status_report_list[1]) },
	{ "","se02",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[2], 0 , sizeof(sr.status_report_list[2]) },
	{ "","se03",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[3], 0 , sizeof(sr.status_report_list[3]) },
	{ "","se04",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[4], 0 , sizeof(sr.status_report_list[4]) },
	{ "","se05",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[5], 0 , sizeof(sr.status_report_list[5]) },
	{ "","se06",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[6], 0 , sizeof(sr.status_report_list[6]) },
	{ "","se07",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[7], 0 , sizeof(sr.status_report_list[7]) },
	{ "","se08",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[8], 0 , sizeof(sr.status_report_list[8]) },
	{ "","se09",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[9], 0 , sizeof(sr.status_report_list[9]) },
	{ "","se10",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[10], 0 , sizeof(sr.status_report_list[10]) },
	{ "","se11",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[11], 0 , sizeof(sr.status_report_list[11]) },
	{ "","se12",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[12], 0 , sizeof(sr.status_report_list[12]) },
	{ "","se13",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[13], 0 , sizeof(sr.status_report_list[13]) },
	{ "","se14",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[14], 0 , sizeof(sr.status_report_list[14]) },
	{ "","se15",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[15], 0 , sizeof(sr.status_report_list[15]) },
	{ "","se16",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[16], 0 , sizeof(sr.status_report_list[16]) },
	{ "","se17",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[17], 0 , sizeof(sr.status_report_list[17]) },
	{ "","se18",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[18], 0 , sizeof(sr.status_report_list[18]) },
	{ "","se19",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[19], 0 , sizeof(sr.status_report_list[19]) },
	{ "","se20",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[20], 0 , sizeof(sr.status_report_list[20]) },
	{ "","se21",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[21], 0 , sizeof(sr.status_report_list[21]) },
	{ "","se22",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[22], 0 , sizeof(sr.status_report_list[22]) },
	{ "","se23",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[23], 0 , sizeof(sr.status_report_list[23]) },
	{ "","se24",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[24], 0 , sizeof(sr.status_report_list[24]) },
	{ "","se25",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[25], 0 , sizeof(sr.status_report_list[25]) },
	{ "","se26",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[26], 0 , sizeof(sr.status_report_list[26]) },
	{ "","se27",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[27], 0 , sizeof(sr.status_report_list[27]) },
	{ "","se28",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[28], 0 , sizeof(sr.status_report_list[28]) },
	{ "","se29",_fp, 0, tx_print_nul, get_u16, set_u16, (uint32_t *)&sr.status_report_list[29], 0 , sizeof(sr.status_report_list[29]) },
    // Count is 30, since se00 counts as one.

	// Group lookups - must follow the single-valued entries for proper sub-string matching
	// *** Must agree with NV_COUNT_GROUPS below ***
	// *** START COUNTING FROM HERE ***
	{ "","sys",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// system group
	{ "","p1", _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// PWM 1 group

	{ "","1",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// motor groups
	{ "","2",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","3",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","4",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
#if (MOTORS >= 5)
	{ "","5",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
#endif
#if (MOTORS >= 6)
	{ "","6",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
#endif

	{ "","x",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// axis groups
	{ "","y",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","z",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","a",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","b",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","c",  _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },

	{ "","in", _f0, 0, tx_print_nul, get_grp, set_nul,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// switch states (inputs)
	{ "","g54",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// coord offset groups
	{ "","g55",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","g56",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","g57",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","g58",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","g59",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "","g92",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// origin offsets
	{ "","g28",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// g28 home position
	{ "","g30",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// g30 home position

	{ "","mpo",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// machine position group
	{ "","pos",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// work position group
	{ "","ofs",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// work offset group
	{ "","hom",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// axis homing state group
	{ "","prb",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// probing state group
	{ "","pwr",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// motor power enagled group
	{ "","jog",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// axis jogging state group
	{ "","jid",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// job ID group

	{ "","uda", _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// user data group
	{ "","udb", _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// user data group
	{ "","udc", _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// user data group
	{ "","udd", _n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// user data group

#ifdef __DIAGNOSTIC_PARAMETERS
	{ "","_te",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// target axis endpoint group
	{ "","_tr",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// target axis runtime group
	{ "","_ts",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// target motor steps group
	{ "","_ps",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// position motor steps group
	{ "","_cs",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// commanded motor steps group
	{ "","_es",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// encoder steps group
	{ "","_xs",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// correction steps group
	{ "","_fe",_n0, 0, tx_print_nul, get_grp, set_grp,(uint32_t *)&cs.null,0 , sizeof(cs.null) },	// following error group
#endif

	// Uber-group (groups of groups, for text-mode displays only)
	// *** Must agree with NV_COUNT_UBER_GROUPS below ****
	{ "", "m", _n0, 0, tx_print_nul, _do_motors, set_nul,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "", "q", _n0, 0, tx_print_nul, _do_axes,   set_nul,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "", "o", _n0, 0, tx_print_nul, _do_offsets,set_nul,(uint32_t *)&cs.null,0 , sizeof(cs.null) },
	{ "", "$", _n0, 0, tx_print_nul, _do_all,    set_nul,(uint32_t *)&cs.null,0 , sizeof(cs.null) }
};

/***** Make sure these defines line up with any changes in the above table *****/

#define NV_COUNT_UBER_GROUPS 	4 		// count of uber-groups, above
#define STANDARD_GROUPS 		34		// count of standard groups, excluding diagnostic parameter groups

#if (MOTORS >= 5)
#define MOTOR_GROUP_5			1
#else
#define MOTOR_GROUP_5			0
#endif

#if (MOTORS >= 6)
#define MOTOR_GROUP_6			1
#else
#define MOTOR_GROUP_6			0
#endif

#ifdef __DIAGNOSTIC_PARAMETERS
#define DIAGNOSTIC_GROUPS 		8		// count of diagnostic groups only
#else
#define DIAGNOSTIC_GROUPS 		0
#endif
#define NV_COUNT_GROUPS 		(STANDARD_GROUPS + MOTOR_GROUP_5 + MOTOR_GROUP_6 + DIAGNOSTIC_GROUPS)

/* <DO NOT MESS WITH THESE DEFINES> */
#define NV_INDEX_MAX (sizeof cfgArray / sizeof(cfgItem_t))
#define NV_INDEX_END_SINGLES		(NV_INDEX_MAX - NV_COUNT_UBER_GROUPS - NV_COUNT_GROUPS - NV_STATUS_REPORT_LEN)
#define NV_INDEX_START_GROUPS		(NV_INDEX_MAX - NV_COUNT_UBER_GROUPS - NV_COUNT_GROUPS)
#define NV_INDEX_START_UBER_GROUPS (NV_INDEX_MAX - NV_COUNT_UBER_GROUPS)
/* </DO NOT MESS WITH THESE DEFINES> */

index_t	nv_index_max() { return ( NV_INDEX_MAX );}
bool nv_index_is_single(index_t index) { return ((index <= NV_INDEX_END_SINGLES) ? true : false);}
bool nv_index_lt_groups(index_t index) { return ((index <= NV_INDEX_START_GROUPS) ? true : false);}
//bool nv_index_is_group(index_t index) { return (((index >= NV_INDEX_START_GROUPS) && (index < NV_INDEX_START_UBER_GROUPS)) ? true : false);}


/***** APPLICATION SPECIFIC CONFIGS AND EXTENSIONS TO GENERIC FUNCTIONS *****/

/*
 * set_flu() - set floating point number with G20/G21 units conversion
 *
 * The number 'setted' will have been delivered in external units (inches or mm).
 * It is written to the target memory location in internal canonical units (mm).
 * The original nv->value_flt is also changed so persistence works correctly.
 * Displays should convert back from internal canonical form to external form.
 */

stat_t set_flu(nvObj_t *nv)
{
	if (cm_get_units_mode(MODEL) == INCHES) {		    // if in inches...
		nv->value_flt *= MM_PER_INCH;					// convert to canonical millimeter units
	}
	*((float *)GET_TABLE_WORD(target)) = nv->value_flt;	// write value as millimeters or degrees
	nv->precision = GET_TABLE_WORD(precision);
	nv->valuetype = TYPE_FLOAT;
	return(STAT_OK);
}

/*
 * prep_float()   - pre-process floating point number for units display
 *
 *  Also traps conditions for which float-to-ascii would fail
 */

bool prep_float(nvObj_t *nv)
{
    if (nv->valuetype != TYPE_FLOAT) {  // this test is necessary for the text parser
        return (false);                 // not required for JSON as values are already clean
    }
	if (isnan((double)nv->value_flt) || isinf((double)nv->value_flt)) { // illegal float values
        return (false);
    }
    if (cfg_has_flag(nv->index, F_CONVERT)) {       // unit conversion required?
		if (cm_get_units_mode(MODEL) == INCHES) {
			nv->value_flt *= INCHES_PER_MM;
		}
	}
    return (true);
}

/**** TinyG UberGroup Operations ****************************************************
 * Uber groups are groups of groups organized for convenience:
 *	- motors		- group of all motor groups
 *	- axes			- group of all axis groups
 *	- offsets		- group of all offsets and stored positions
 *	- all			- group of all groups
 *
 * _do_group_csv()	- get and print all groups in the list (iteration)
 * _do_motors()		- get and print motor uber group 1-N
 * _do_axes()		- get and print axis uber group XYZABC
 * _do_offsets()	- get and print offset uber group G54-G59, G28, G30, G92
 * _do_all()		- get and print all groups uber group
 */

// helper to print multiple groups in a list
#define LONGEST_CSV_STRING sizeof("g54,g55,g56,g57,g58,g59,g92,g28,g30")+1

static stat_t _do_group_csv_P(nvObj_t *nv, uint8_t items, const char *list_P)
{
    char list[LONGEST_CSV_STRING]; strcpy_P(list, list_P);
    char *rd = strtok(list, ",");               // initialize strtok & get pointer for csv item parsing
    for (uint8_t i=0; i<items; i++) {           // the list might be longer than the # of items to display
        nv_reset_nv_list(NUL);
        nv = NV_BODY;
        strncpy(nv->token, rd, TOKEN_LEN);
        nv_populate_nv_by_index(nv, nv_get_index("", nv->token));
//		nv->valuetype = TYPE_PARENT;            // left in for clarity. Not required
        nv_print_list(STAT_OK, TEXT_RESPONSE, JSON_RESPONSE);
        if ((rd = strtok(NULL, ",")) == NULL) { // the other break condition
            break;
        }
    }
	return (STAT_NO_DISPLAY);
}

static stat_t _do_motors(nvObj_t *nv)	// print parameters for all motor groups
{
    return(_do_group_csv_P(nv, MOTORS, PSTR("1,2,3,4,5,6")));
}

static stat_t _do_axes(nvObj_t *nv)	// print parameters for all axis groups
{
    return(_do_group_csv_P(nv, AXES, PSTR("x,y,z,a,b,c")));
}

static stat_t _do_offsets(nvObj_t *nv)	// print offset parameters for G54-G59,G92, G28, G30
{
    return(_do_group_csv_P(nv, 9, PSTR("g54,g55,g56,g57,g58,g59,g92,g28,g30")));
}

static stat_t _do_all(nvObj_t *nv)	    // print all parameters
{
	strcpy(nv->token,"sys");			// print system group
	get_grp(nv);
	nv_print_list(STAT_OK, TEXT_RESPONSE, JSON_RESPONSE);

	_do_motors(nv);					    // print all motor groups
	_do_axes(nv);						// print all axis groups

	strcpy(nv->token,"p1");			    // print PWM group
	get_grp(nv);
	nv_print_list(STAT_OK, TEXT_RESPONSE, JSON_RESPONSE);

	return (_do_offsets(nv));			// print all offsets
}

/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 * Most of these can be found in their respective modules.
 ***********************************************************************************/

/**** COMMUNICATIONS FUNCTIONS ******************************************************
 * set_ec() - enable CRLF on TX
 * set_ee() - enable character echo
 * set_ex() - enable XON/XOFF or RTS/CTS flow control
 * get_rx()	- get bytes or lines available in RX buffer(s)
 * set_baud() - set USB baud rate
 *
 *	The above assume USB is the std device
 */

static stat_t _set_comm_helper(nvObj_t *nv, uint32_t yes, uint32_t no)
{
	if (nv->value_int != 0) {
		(void)xio_ctrl(XIO_DEV_USB, yes);
	} else {
		(void)xio_ctrl(XIO_DEV_USB, no);
	}
	return (STAT_OK);
}

static stat_t set_ec(nvObj_t *nv) 				// expand CR to CRLF on TX
{
	if (nv->value_int > true) {
        return (STAT_INPUT_VALUE_RANGE_ERROR);
    }
	xio.enable_cr = (uint8_t)nv->value_int;     // the cast is required to prevent neighbors from being clobbered
	return(_set_comm_helper(nv, XIO_CRLF, XIO_NOCRLF));
}

static stat_t set_ee(nvObj_t *nv) 				// enable character echo
{
    if (nv->value_int > true) {
        return (STAT_INPUT_VALUE_RANGE_ERROR);
    }
	xio.enable_echo = (uint8_t)nv->value_int;   // the cast is required
	return(_set_comm_helper(nv, XIO_ECHO, XIO_NOECHO));
}

static stat_t set_ex(nvObj_t *nv)				// enable XON/XOFF or RTS/CTS flow control
{
	if (nv->value_int > FLOW_CONTROL_RTS) {
        return (STAT_INPUT_VALUE_RANGE_ERROR);
    }
	xio.enable_flow_control = (uint8_t)nv->value_int; // the cast is required
	return(_set_comm_helper(nv, XIO_XOFF, XIO_NOXOFF));
}

static stat_t get_rx(nvObj_t *nv)
{ return (STAT_OK);
#ifdef __AVR1 //xzw168
    if (xio.rx_mode == RX_MODE_CHAR) {
	    nv->value_int = xio_get_usb_rx_free();
    } else {
	    nv->value_int = xio_get_line_buffers_available();
    }
	nv->valuetype = TYPE_INTEGER;
	return (STAT_OK);
#endif
#ifdef __ARM
	nv->value_int = 254;				// ARM always says the serial buffer is available (max)
	nv->valuetype = TYPE_INTEGER;
	return (STAT_OK);
#endif
}

/* run_sx()	- send XOFF, XON --- test only
static stat_t run_sx(nvObj_t *nv)
{
	xio_putc(XIO_DEV_USB, XOFF);
	xio_putc(XIO_DEV_USB, XON);
	return (STAT_OK);
}
*/

/*
 * set_baud() - set USB baud rate
 *
 *	See xio_usart.h for valid values. Works as a callback.
 *	The initial routine changes the baud config setting and sets a flag
 *	Then it posts a user message indicating the new baud rate
 *	Then it waits for the TX buffer to empty (so the message is sent)
 *	Then it performs the callback to apply the new baud rate
 */
static const char msg_baud0[] PROGMEM = "0";
static const char msg_baud1[] PROGMEM = "9600";
static const char msg_baud2[] PROGMEM = "19200";
static const char msg_baud3[] PROGMEM = "38400";
static const char msg_baud4[] PROGMEM = "57600";
static const char msg_baud5[] PROGMEM = "115200";
static const char msg_baud6[] PROGMEM = "230400";
static const char *const msg_baud[] PROGMEM = { msg_baud0, msg_baud1, msg_baud2, msg_baud3, msg_baud4, msg_baud5, msg_baud6 };

static stat_t set_baud(nvObj_t *nv)
{
	uint8_t baud = nv->value_int;
	if ((baud < 1) || (baud > 6)) {
		nv_add_message_P(PSTR("*** WARNING *** Unsupported baud rate specified"));
		return (STAT_INPUT_VALUE_RANGE_ERROR);
	}
	xio.usb_baud_rate = baud;
	xio.usb_baud_flag = true;

	char msg[NV_MESSAGE_LEN];
	sprintf_P(msg, PSTR("*** NOTICE *** Resetting baud rate to %s"),GET_TEXT_ITEM(msg_baud, baud));
	nv_add_message(msg);

	return (STAT_OK);
}

stat_t set_baud_callback(void)
{
	if (xio.usb_baud_flag == false)
        return(STAT_NOOP);
	xio.usb_baud_flag = false;
	xio_set_baud(XIO_DEV_USB, xio.usb_baud_rate);
	return (STAT_OK);
}

/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

static const char fmt_ec[] PROGMEM = "[ec]  expand LF to CRLF on TX%6d [0=off,1=on]\n";
static const char fmt_ee[] PROGMEM = "[ee]  enable echo%18d [0=off,1=on]\n";
static const char fmt_ex[] PROGMEM = "[ex]  enable flow control%10d [0=off,1=XON/XOFF,2=RTS/CTS]\n";
static const char fmt_rxm[] PROGMEM = "[rxm] serial RX mode%15d [0=char_mode,1=line_mode]\n";
static const char fmt_baud[] PROGMEM = "[baud] USB baud rate%15d [1=9600,2=19200,3=38400,4=57600,5=115200,6=230400]\n";
static const char fmt_rx[] PROGMEM = "rx:%d\n";

void cfg_print_ec(nvObj_t *nv) { text_print(nv, fmt_ec);}
void cfg_print_ee(nvObj_t *nv) { text_print(nv, fmt_ee);}
void cfg_print_ex(nvObj_t *nv) { text_print(nv, fmt_ex);}
void cfg_print_rxm(nvObj_t *nv) { text_print(nv, fmt_rxm);}
void cfg_print_baud(nvObj_t *nv) { text_print(nv, fmt_baud);}
void cfg_print_rx(nvObj_t *nv) { text_print(nv, fmt_rx);}

#endif // __TEXT_MODE
