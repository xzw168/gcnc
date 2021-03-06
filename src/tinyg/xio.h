/*
 * xio.h - Xmega IO devices - common header file
 * Part of TinyG project (g1)
 *
 * Copyright (c) 2010 - 2016 Alden S. Hart Jr.
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
/* XIO devices are compatible with avr-gcc stdio, so formatted printing
 * is supported. To use this sub-system outside of TinyG you may need
 * some defines in tinyg.h. See notes at end of this file for more details.
 */
/* Note: anything that includes xio.h first needs the following:
 * 	#include <stdio.h>				// needed for FILE def'n
 *	#include <stdbool.h>			// needed for true and false
 *	#include <avr/pgmspace.h>		// defines prog_char, PSTR
 */
/* Note: This file contains load of sub-includes near the middle
 *	#include "xio_file.h"
 *	#include "xio_usart.h"
 *	#include "xio_spi.h"
 *	#include "xio_signals.h"
 *	(possibly more)
 */
/*
 * CAVEAT EMPTOR: File under "watch your ass":
 *
 * 	  - Short story: Do not call ANYTHING that can print (i.e. send chars to the TX
 *		buffer) from a medium or hi interrupt. This obviously includes any printf()
 *		function, but also exception reports, cm_soft_alarm(), cm_hard_alarm() and a
 *		few other functions that call stdio print functions.
 *
 * 	  - Longer Story: The stdio printf() functions use character drivers provided by
 *		tinyg to access the low-level Xmega devices. Specifically xio_putc_usb() in xio_usb.c,
 *		and xio_putc_rs485() in xio_rs485.c. Since stdio does not understand non-blocking
 *		IO these functions must block if there is no space in the TX buffer. Blocking is
 *		accomplished using sleep_mode(). The IO system is the only place where sleep_mode()
 *		is used. Everything else in TinyG is non-blocking. Sleep is woken (exited) whenever
 *		any interrupt fires. So there must always be a viable interrupt source running when
 *		you enter a sleep or the system will hang (lock up). In the IO functions this is the
 *		TX interupts, which fire when space becomes available in the USART for a TX char. This
 *		Means you cannot call a print function at or above the level of the TX interrupts,
 *		which are set to medium.
 */
#ifndef XIO_H_ONCE
#define XIO_H_ONCE



#include "xio/xio_usart.h"


/*************************************************************************
 *	Device configurations
 *************************************************************************/
// Pre-allocated XIO devices (configured devices)
// Unused devices are commented out. All this needs to line up.

typedef enum {		// TYPE:	DEVICE:
	XIO_DEV_USB,		// USART	USB device
	XIO_DEV_RS485,		// USART	RS485 device
	XIO_DEV_SPI1,		// SPI		SPI channel #1
//	XIO_DEV_SPI2,		// SPI		SPI channel #2
//	XIO_DEV_SPI3,		// SPI		SPI channel #3
//	XIO_DEV_SPI4,		// SPI		SPI channel #4
	XIO_DEV_PGM,		// FILE		Program memory file  (read only)
//	XIO_DEV_SD,			// FILE		SD card (not implemented)
	XIO_DEV_COUNT		// total device count (must be last entry)
} xioDevNum_t;
// If your change these ^, check these v

#define XIO_DEV_USART_COUNT 	2       // # of USART devices
#define XIO_DEV_USART_OFFSET	0       // offset for computing indices

#define XIO_DEV_SPI_COUNT 		1       // # of SPI devices
#define XIO_DEV_SPI_OFFSET		XIO_DEV_USART_COUNT	// offset for computing indicies

#define XIO_DEV_FILE_COUNT		1       // # of FILE devices
#define XIO_DEV_FILE_OFFSET		(XIO_DEV_USART_COUNT + XIO_DEV_SPI_COUNT) // index into FILES

// Fast accessors
#define USB ds[XIO_DEV_USB]
#define USBu us[XIO_DEV_USB - XIO_DEV_USART_OFFSET]

// Aliases

#define xio_flush_read      xio_reset_usb_rx_buffers    // allows v8 code to use same call as g2

//*** Device flags ***
typedef uint16_t devflags_t;

// device capabilities flags
#define DEV_CAN_BE_CTRL		(0x0001)    // device can be a control channel
#define DEV_CAN_BE_DATA		(0x0002)    // device can be a data channel
#define DEV_CAN_READ		(0x0010)
#define DEV_CAN_WRITE		(0x0020)

// channel type
#define DEV_IS_NONE			(0x0000)    // None of the following
#define DEV_IS_CTRL			(0x0001)    // device is set as a control channel
#define DEV_IS_DATA			(0x0002)    // device is set as a data channel
#define DEV_IS_PRIMARY		(0x0004)    // device is the primary control channel
#define DEV_IS_BOTH			(DEV_IS_CTRL | DEV_IS_DATA)

// device connection state
#define DEV_IS_DISCONNECTED	(0x0010)    // device just disconnected (transient state)
#define DEV_IS_CONNECTED	(0x0020)    // device is connected (e.g. USB)
#define DEV_IS_READY		(0x0040)    // device is ready for use
#define DEV_IS_ACTIVE		(0x0080)    // device is active

// device exception flags
#define DEV_THROW_EOF		(0x0100)    // end of file encountered


/******************************************************************************
 * Device structures
 *
 * Each device has 3 structs. The generic device struct is declared below.
 * It embeds a stdio stream struct "FILE". The FILE struct uses the udata
 * field to back-reference the generic struct so getc & putc can get at it.
 * Lastly there's an 'x' struct which contains data specific to each dev type.
 *
 * The generic open() function sets up the generic struct and the FILE stream.
 * the device opens() set up the extended struct and bind it ot the generic.
 ******************************************************************************/
// NOTE" "FILE *" is another way of saying "struct __file *"
// NOTE: using the "x_" prefix fo avoid collisions with stdio defined getc, putc, etc

#define flags_t uint16_t

typedef enum {
    RX_MODE_CHAR = 0,                            // character mode input (streaming)
    RX_MODE_LINE                                 // line mode input (packet)
} xioRXMode;




/******************************************************************************
 * Readline Buffer Management
 */

#define RX_HEADERS                12        // buffer headers in the list
#define RX_BUFFER_MIN_SIZE       200        // minimum requested buffer size (they are usually larger)
#define RX_BUFFER_POOL_SIZE     1000        // total size of RX buffer memory pool
/*
#define RX_HEADERS                26        // buffer headers in the list
#define RX_BUFFER_MIN_SIZE       256        // minimum requested buffer size (they are usually larger)
#define RX_BUFFER_POOL_SIZE     1500        // total size of RX buffer memory pool
*/
typedef enum {                              // readline() buffer and slot states
    BUFFER_FREE = 0,                        // buffer (slot) is available (must be 0)
    BUFFER_FRAGMENT,                        // buffer is free but in the middle of the list
    BUFFER_FILLING,                         // buffer is partially loaded
    BUFFER_FULL,                            // buffer contains a full line
    BUFFER_PROCESSING                       // buffer is in use by the caller
} cmBufferState;






typedef struct xioSingleton {
    uint16_t magic_start;
    //FILE * stderr_shadow;				// used for stack overflow / memory integrity checking xzw168

    // communications settings
    uint8_t primary_src;				// primary input source device
    uint8_t secondary_src;				// secondary input source device
    uint8_t default_src;				// default source device

    uint8_t usb_baud_rate;				// see xio_usart.h for XIO_BAUD values
    uint8_t usb_baud_flag;				// technically this belongs in the controller singleton

    uint8_t enable_cr;					// enable CR in CRFL expansion on TX (shadow setting for XIO ctrl bits)
    uint8_t enable_echo;				// enable text-mode echo (shadow setting for XIO ctrl bits)
    uint8_t enable_flow_control;		// enable XON/XOFF or RTS/CTS flow control (shadow setting for XIO ctrl bits)
    xioRXMode rx_mode;			        // 0=RX_MODE_STREAM, 1=RX_MODE_PACKET

    // character mode reader
    uint16_t buf_size;					// persistent size variable
    uint8_t buf_state;					// holds CTRL or DATA once this is known
    char *bufp;                         // pointer to input buffer

    uint16_t magic_end;
} xioSingleton_t;
extern  xioSingleton_t xio;

struct __file {
	char	*buf;		/* buffer pointer */
	unsigned char unget;	/* ungetc() buffer */
	uint8_t	flags;		/* flags, see below */
#define __SRD	0x0001		/* OK to read */
#define __SWR	0x0002		/* OK to write */
#define __SSTR	0x0004		/* this is an sprintf/snprintf string */
#define __SPGM	0x0008		/* fmt string is in progmem */
#define __SERR	0x0010		/* found error */
#define __SEOF	0x0020		/* found EOF */
#define __SUNGET 0x040		/* ungetc() happened */
#define __SMALLOC 0x80		/* handle is malloc()ed */
#if 0
/* possible future extensions, will require uint16_t flags */
#define __SRW	0x0100		/* open for reading & writing */
#define __SLBF	0x0200		/* line buffered */
#define __SNBF	0x0400		/* unbuffered */
#define __SMBF	0x0800		/* buf is from malloc */
#endif
	int	size;		/* size of buffer */
	int	len;		/* characters read or written so far */
	int	(*put)(char, struct __file *);	/* function to write one char to device */
	int	(*get)(struct __file *);	/* function to read one char from device */
	void	*udata;		/* User defined and accessible data. */
};
#undef FILE
#define FILE struct __file 

typedef struct xioDEVICE {						// common device struct (one per dev)
	// references and self references
	uint16_t magic_start;						// memory integrity check
	uint8_t dev;								// self referential device number
	FILE file;									// stdio FILE stream structure
	void *x;									// 扩展设备结构绑定（静态）

	// function bindings
	FILE *(*x_open)(const uint8_t dev, const char *addr, const flags_t flags);
	int (*x_ctrl)(struct xioDEVICE *d, const flags_t flags);	 // set device control flags
	int (*x_gets)(struct xioDEVICE *d, char *buf, const int size);// non-blocking line reader
	int (*x_getc)(FILE *);						// read char (stdio compatible)
	int (*x_putc)(char, FILE *);				// write char (stdio compatible)
	void (*x_flow)(struct xioDEVICE *d);		// flow control callback function

	// device configuration flags
	uint8_t flag_block;
	uint8_t flag_echo;
	uint8_t flag_crlf;
	uint8_t flag_ignorecr;
	uint8_t flag_ignorelf;
	uint8_t flag_linemode;
	uint8_t flag_xoff;							// xon/xoff enabled

	// private working data and runtime flags
	int size;									// text buffer length (dynamic)
	uint8_t len;								// chars read so far (buf array index)
	uint8_t signal;								// signal value
	uint8_t flag_in_line;						// used as a state variable for line reads
	uint8_t flag_eol;							// end of line detected
	uint8_t flag_eof;							// end of file detected
	char *buf;									// text buffer binding (can be dynamic)
	uint16_t magic_end;
} xioDev_t;





/*************************************************************************
 * SUPPORTING DEFINTIONS - SHOULD NOT NEED TO CHANGE
 *************************************************************************/
/*
 * xio control flag values
 *
 * if using 32 bits must cast 1 to uint32_t for bit evaluations to work correctly
 * #define XIO_BLOCK	((uint32_t)1<<1)		// 32 bit example. Change flags_t to uint32_t
 */

#define XIO_BLOCK		((uint16_t)1<<0)		// enable blocking reads
#define XIO_NOBLOCK		((uint16_t)1<<1)		// disable blocking reads
#define XIO_XOFF 		((uint16_t)1<<2)		// enable XON/OFF flow control
#define XIO_NOXOFF 		((uint16_t)1<<3)		// disable XON/XOFF flow control
#define XIO_ECHO		((uint16_t)1<<4)		// echo reads from device to stdio
#define XIO_NOECHO		((uint16_t)1<<5)		// disable echo
#define XIO_CRLF		((uint16_t)1<<6)		// convert <LF> to <CR><LF> on writes
#define XIO_NOCRLF		((uint16_t)1<<7)		// do not convert <LF> to <CR><LF> on writes
#define XIO_IGNORECR	((uint16_t)1<<8)		// ignore <CR> on reads
#define XIO_NOIGNORECR	((uint16_t)1<<9)		// don't ignore <CR> on reads
#define XIO_IGNORELF	((uint16_t)1<<10)		// ignore <LF> on reads
#define XIO_NOIGNORELF	((uint16_t)1<<11)		// don't ignore <LF> on reads
#define XIO_LINEMODE	((uint16_t)1<<12)		// special <CR><LF> read handling
#define XIO_NOLINEMODE	((uint16_t)1<<13)		// no special <CR><LF> read handling

/*
 * Generic XIO signals and error conditions.
 * See signals.h for application specific signal defs and routines.
 */

typedef enum {
	XIO_SIG_OK,				// OK
	XIO_SIG_EAGAIN,			// would block
	XIO_SIG_EOL,			// end-of-line encountered (string has data)
	XIO_SIG_EOF,			// end-of-file encountered (string has no data)
	XIO_SIG_OVERRUN,		// buffer overrun
	XIO_SIG_RESET,			// cancel operation immediately
	XIO_SIG_FEEDHOLD,		// pause operation
	XIO_SIG_CYCLE_START,	// start or resume operation
	XIO_SIG_QUEUE_FLUSH,	// flush planner queue
	XIO_SIG_DELETE,			// backspace or delete character (BS, DEL)
	XIO_SIG_BELL,			// BELL character (BEL, ^g)
	XIO_SIG_BOOTLOADER		// ESC character - start bootloader
} xioSignals;

/* Some useful ASCII definitions */

#define NUL (char)0x00		//  ASCII NUL char (0) (not "NULL" which is a pointer)
#define STX (char)0x02		// ^b - STX
#define ETX (char)0x03		// ^c - ETX
#define ENQ (char)0x05		// ^e - ENQuire
#define BEL (char)0x07		// ^g - BEL
#define BS  (char)0x08		// ^h - backspace
#define TAB (char)0x09		// ^i - character
#define LF	(char)0x0A		// ^j - line feed
#define VT	(char)0x0B		// ^k - kill stop
#define CR	(char)0x0D		// ^m - carriage return
#define XON (char)0x11		// ^q - DC1, XON, resume
#define XOFF (char)0x13		// ^s - DC3, XOFF, pause
#define SYN (char)0x16		// ^v - SYN - Used for queue flush
#define CAN (char)0x18		// ^x - Cancel, abort
#define ESC (char)0x1B		// ^[ - ESC(ape)
#define SPC  (char)0x20		// ' '  Space character	// NB: SP is defined externally
#define DEL (char)0x7F		//  DEL(ete)

#define Q_EMPTY (char)0xFF	// signal no character

/* Signal character mappings */

#define CHAR_RESET CAN
#define CHAR_ENQUIRY ENQ
#define CHAR_FEEDHOLD (char)'!'
#define CHAR_END_HOLD (char)'~'
#define CHAR_QUEUE_FLUSH (char)'%'
//#define CHAR_BOOTLOADER ESC

/* XIO return codes
 * These codes are the "inner nest" for the STAT_ return codes.
 * The first N TG codes correspond directly to these codes.
 * This eases using XIO by itself (without tinyg) and simplifes using
 * tinyg codes with no mapping when used together. This comes at the cost
 * of making sure these lists are aligned. STAT_should be based on this list.
 */

typedef enum {
	XIO_OK = 0,				// OK - ALWAYS ZERO
	XIO_ERR,				// generic error return (errors start here)
	XIO_EAGAIN,				// function would block here (must be called again)
	XIO_NOOP,				// function had no-operation
	XIO_COMPLETE,			// operation complete
	XIO_TERMINATE,			// operation terminated (gracefully)
	XIO_RESET,				// operation reset (ungraceful)
	XIO_EOL,				// function returned end-of-line
	XIO_EOF,				// function returned end-of-file
	XIO_FILE_NOT_OPEN,		// file is not open
	XIO_FILE_SIZE_EXCEEDED, // maximum file size exceeded
	XIO_NO_SUCH_DEVICE,		// illegal or unavailable device
	XIO_BUFFER_EMPTY,		// more of a statement of fact than an error code
	XIO_BUFFER_FULL,
	XIO_BUFFER_FULL_FATAL,
	XIO_INITIALIZING,		// system initializing, not ready for use
	XIO_ERROR_16,			// reserved
	XIO_ERROR_17,			// reserved
	XIO_ERROR_18,			// reserved
	XIO_ERROR_19			// NOTE: XIO codes align to here
} xioCodes;
#define XIO_ERRNO_MAX XIO_BUFFER_FULL_NON_FATAL



/* ASCII characters used by Gcode or otherwise unavailable for special use.
    See NIST sections 3.3.2.2, 3.3.2.3 and Appendix E for Gcode uses.
    See http://www.json.org/ for JSON notation

    hex	    char    name        used by:
    ----    ----    ----------  --------------------
    0x00    NUL	    null        everything
    0x01    SOH     ctrl-A
    0x02    STX     ctrl-B      Kinen SPI protocol
    0x03    ETX     ctrl-C      Kinen SPI protocol
    0x04    EOT     ctrl-D
    0x05    ENQ     ctrl-E
    0x06    ACK     ctrl-F
    0x07    BEL     ctrl-G
    0x08    BS      ctrl-H
    0x09    HT      ctrl-I
    0x0A    LF      ctrl-J
    0x0B    VT      ctrl-K
    0x0C    FF      ctrl-L
    0x0D    CR      ctrl-M
    0x0E    SO      ctrl-N
    0x0F    SI      ctrl-O
    0x10    DLE     ctrl-P
    0x11    DC1     ctrl-Q      XOFF
    0x12    DC2     ctrl-R
    0x13    DC3     ctrl-S      XON
    0x14    DC4     ctrl-T
    0x15    NAK     ctrl-U
    0x16    SYN     ctrl-V
    0x17    ETB     ctrl-W
    0x18    CAN     ctrl-X      TinyG / grbl software reset
    0x19    EM      ctrl-Y
    0x1A    SUB     ctrl-Z
    0x1B    ESC     ctrl-[
    0x1C    FS      ctrl-\
    0x1D    GS      ctrl-]
    0x1E    RS      ctrl-^
    0x1F    US      ctrl-_

    0x20    <space>             Gcode blocks
    0x21    !       excl point  TinyG feedhold (trapped and removed from serial stream)
    0x22    "       quote       JSON notation
    0x23    #       number      Gcode parameter prefix; JSON topic prefix character
    0x24    $       dollar      TinyG / grbl out-of-cycle settings prefix
    0x25    &       ampersand   universal symbol for logical AND (not used here)
    0x26    %       percent		Queue Flush character (trapped and removed from serial stream)
								Also sometimes used as a file-start and file-end character in Gcode files
    0x27    '       single quote
    0x28    (       open paren  Gcode comments
    0x29    )       close paren Gcode comments
    0x2A    *       asterisk    Gcode expressions; JSON wildcard character
    0x2B    +       plus        Gcode numbers, parameters and expressions
    0x2C    ,       comma       JSON notation
    0x2D    -       minus       Gcode numbers, parameters and expressions
    0x2E    .       period      Gcode numbers, parameters and expressions
    0x2F    /       fwd slash   Gcode expressions & block delete char
    0x3A    :       colon       JSON notation
    0x3B    ;       semicolon	Gcode comemnt delimiter (alternate)
    0x3C    <       less than   Gcode expressions
    0x3D    =       equals      Gcode expressions
    0x3E    >       greaterthan Gcode expressions
    0x3F    ?       question mk TinyG / grbl query
    0x40    @       at symbol	JSON address prefix character

    0x5B    [       open bracketGcode expressions
    0x5C    \       backslash   JSON notation (escape)
    0x5D    ]       close brack Gcode expressions
    0x5E    ^       caret       Reserved for TinyG in-cycle command prefix
    0x5F    _       underscore

    0x60    `       grave accnt
    0x7B    {       open curly  JSON notation
    0x7C    |       pipe        universal symbol for logical OR (not used here)
    0x7D    }       close curly JSON notation
    0x7E    ~       tilde       TinyG cycle start (trapped and removed from serial stream)
    0x7F    DEL
*/

#endif	// end of include guard: XIO_H_ONCE
