
#ifndef xio_api_h
#define xio_api_h


typedef volatile uint8_t register8_t;
typedef volatile uint16_t register16_t;
typedef volatile uint32_t register32_t;
typedef char char_t;
#define buffer_t uint_fast8_t

#define PROGMEM
#define _FDEV_ERR (-1)
#define M_PI       3.14159265358979323846   // pi
#define PSTR (const char *)

#define strncpy_P(d,s,l) (char_t *)strncpy((char *)d, (char *)s, l)
#define strpbrk_P(d,s) (char_t *)strpbrk((char *)d, (char *)s)
#define strcpy_P(d,s) (char_t *)strcpy((char *)d, (char *)s)
#define strcat_P(d,s) (char_t *)strcat((char *)d, (char *)s)
#define strstr_P(d,s) (char_t *)strstr((char *)d, (char *)s)
#define strchr_P(d,s) (char_t *)strchr((char *)d, (char)s)
#define strcmp_P(d,s) strcmp((char *)d, (char *)s)
#define strtod_P(d,p) strtod((char *)d, (char **)p)
#define strtof_P(d,p) strtof((char *)d, (char **)p)
#define strlen_P(s) strlen((char *)s)
#define isdigit_P(c) isdigit((char)c)
#define isalnum_P(c) isalnum((char)c)
#define tolower_P(c) (char_t)tolower((char)c)
#define toupper_P(c) (char_t)toupper((char)c)

#define printf_P(...)  printf(__VA_ARGS__)		// these functions want char * as inputs, not char_t *
#define fprintf_P(f,...) printf(__VA_ARGS__)//fprintf	// just sayin'
#define sprintf_P sprintf

#define GET_TABLE_WORD(a)  cfgArray[nv->index].a	// get word value from cfgArray
#define GET_TABLE_BYTE(a)  cfgArray[nv->index].a	// get byte value from cfgArray
#define GET_TABLE_FLOAT(a) cfgArray[nv->index].a	// get byte value from cfgArray

#define GET_TOKEN_BYTE(a) (char_t)cfgArray[i].a	// get token byte value from cfgArray
#define GET_TOKEN_STRING(i,a) strcpy_P(a, (char *)&cfgArray[(index_t)i].token);

#define GET_TEXT_ITEM(b,a) strncpy_P(text_item,(const char *)b[a], TEXT_ITEM_LEN-1)
#define GET_UNITS(a) strncpy_P(units_msg,(const char *)msg_units[cm_get_units_mode(a)], UNITS_MSG_LEN-1)	

typedef struct Timeout {
    uint32_t _start, _delay;
} Timeout_t;

typedef struct PORT_struct
{
    register8_t DIR;  /* I/O Port Data Direction */
    register8_t DIRSET;  /* I/O Port Data Direction Set */
    register8_t DIRCLR;  /* I/O Port Data Direction Clear */
    register8_t DIRTGL;  /* I/O Port Data Direction Toggle */
    register8_t OUT;  /* I/O Port Output */
    register8_t OUTSET;  /* I/O Port Output Set */
    register8_t OUTCLR;  /* I/O Port Output Clear */
    register8_t OUTTGL;  /* I/O Port Output Toggle */
    register8_t IN;  /* I/O port Input */
    register8_t INTCTRL;  /* Interrupt Control Register */
    register8_t INT0MASK;  /* Port Interrupt 0 Mask */
    register8_t INT1MASK;  /* Port Interrupt 1 Mask */
    register8_t INTFLAGS;  /* Interrupt Flag Register */
    register8_t reserved_0x0D;
    register8_t reserved_0x0E;
    register8_t reserved_0x0F;
    register8_t PIN0CTRL;  /* Pin 0 Control Register */
    register8_t PIN1CTRL;  /* Pin 1 Control Register */
    register8_t PIN2CTRL;  /* Pin 2 Control Register */
    register8_t PIN3CTRL;  /* Pin 3 Control Register */
    register8_t PIN4CTRL;  /* Pin 4 Control Register */
    register8_t PIN5CTRL;  /* Pin 5 Control Register */
    register8_t PIN6CTRL;  /* Pin 6 Control Register */
    register8_t PIN7CTRL;  /* Pin 7 Control Register */
} PORT_t;

/* Virtual Port */
typedef struct VPORT_struct
{
    register8_t DIR;  /* I/O Port Data Direction */
    register8_t OUT;  /* I/O Port Output */
    register8_t IN;  /* I/O Port Input */
    register8_t INTFLAGS;  /* Interrupt Flag Register */
} VPORT_t;

#define PORTA    (*(PORT_t *) 0x0600)  /* Port A */
#define VPORT0    (*(VPORT_t *) 0x0010)  /* Virtual Port 0 */


#endif
