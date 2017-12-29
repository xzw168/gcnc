

#include <string.h>					// for memset()
#include <stdio.h>					// precursor for xio.h
//#include <avr/pgmspace.h>			// precursor for xio.h
#include <stdint.h>
#include "xio.h"					// all device includes are nested here
#include "tinyg.h"					// needed by init() for default source
#include "config.h"					// needed by init() for default source
#include "controller.h"				// needed by init() for default source
#include "report.h"
#include "util.h"
#include "canonical_machine.h"      // for assertions cm_panic


xioSingleton_t xio;
xioDev_t 		ds[XIO_DEV_COUNT];
static char buf[200];

uint8_t xio_isbusy()
{

	return (true);
}

char *readline(devflags_t *flags, uint16_t *size)
{

	if(xio_usart_gets(buf,200)==XIO_OK)
		return buf;
    return NULL;
}

buffer_t xio_get_usb_rx_free(void)
{
	return 10;//(RX_BUFFER_SIZE - xio_get_rx_bufcount_usart(&USBu));
}

int xio_set_baud(const uint8_t dev, const uint8_t baud)
{
	return (XIO_OK);
}

void xio_reset_usb_rx_buffers(void)
{

}

uint8_t xio_get_line_buffers_available()
{
   
    return (0);
}

uint8_t xio_test_assertions()
{

    return (STAT_OK);
}

#define SETFLAG(t,f) if ((flags & t) != 0) { d->f = true; }
#define CLRFLAG(t,f) if ((flags & t) != 0) { d->f = false; }

int xio_ctrl_generic(xioDev_t *d, const flags_t flags)
{
	SETFLAG(XIO_BLOCK,		flag_block);
	CLRFLAG(XIO_NOBLOCK,	flag_block);
	SETFLAG(XIO_XOFF,		flag_xoff);
	CLRFLAG(XIO_NOXOFF,		flag_xoff);
	SETFLAG(XIO_ECHO,		flag_echo);
	CLRFLAG(XIO_NOECHO,		flag_echo);
	SETFLAG(XIO_CRLF,		flag_crlf);
	CLRFLAG(XIO_NOCRLF,		flag_crlf);
	SETFLAG(XIO_IGNORECR,	flag_ignorecr);
	CLRFLAG(XIO_NOIGNORECR,	flag_ignorecr);
	SETFLAG(XIO_IGNORELF,	flag_ignorelf);
	CLRFLAG(XIO_NOIGNORELF,	flag_ignorelf);
	SETFLAG(XIO_LINEMODE,	flag_linemode);
	CLRFLAG(XIO_NOLINEMODE,	flag_linemode);
	return (XIO_OK);
}

int xio_ctrl(const uint8_t dev, const flags_t flags)
{
	return (xio_ctrl_generic(&ds[dev], flags));
}

uint16_t EEPROM_WriteBytes(const uint16_t address, const int8_t *buf, const uint16_t size)
{
	return 0;
}

/*
 * EEPROM_ReadBytes() - read N bytes to EEPROM; may span multiple pages
 *
 *	This function reads a character string to IO mapped EEPROM.
 *	If memory mapped EEPROM is enabled this function will not work.
 *	A string may span multiple EEPROM pages.
 */

uint16_t EEPROM_ReadBytes(const uint16_t address, int8_t *buf, const uint16_t size)
{
	return 0;
}


void xio_set_stdin(const uint8_t dev) {  }
void xio_set_stdout(const uint8_t dev) {  }
void xio_set_stderr(const uint8_t dev){ }
