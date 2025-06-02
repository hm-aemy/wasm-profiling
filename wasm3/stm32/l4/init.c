#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/times.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dbgmcu.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/itm.h>
#include <libopencm3/cm3/systick.h>
#include "init.h"

/*
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
int _times(struct tms *buf);
void get_buffered_line(void);

#define BUFLEN 127

static uint16_t start_ndx;
static uint16_t end_ndx;
static char buf[BUFLEN + 1];
#define buf_len ((end_ndx - start_ndx) % BUFLEN)
static inline int inc_ndx(int n)
{
	return ((n + 1) % BUFLEN);
}
static inline int dec_ndx(int n) { return (((n + BUFLEN) - 1) % BUFLEN); }

const struct rcc_clock_scale rcc_conf =
	{
		/* 120 PLL from HSI16 VR1 */
		.pllm = 2,
		.plln = 30,
		.pllp = RCC_PLLCFGR_PLLP_DIV7,
		.pllq = RCC_PLLCFGR_PLLQ_DIV4,
		.pllr = RCC_PLLCFGR_PLLR_DIV2,
		.pll_source = RCC_PLLCFGR_PLLSRC_HSI16,
		.hpre = RCC_CFGR_HPRE_NODIV,
		.ppre1 = RCC_CFGR_PPRE_NODIV,
		.ppre2 = RCC_CFGR_PPRE_NODIV,
		.voltage_scale = PWR_SCALE1,
		.flash_config = FLASH_ACR_DCEN | FLASH_ACR_ICEN |
						FLASH_ACR_LATENCY_4WS,
		.ahb_frequency = 120000000,
		.apb1_frequency = 120000000,
		.apb2_frequency = 120000000,
};

static void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hsi16_configs[RCC_CLOCK_VRANGE1_80MHZ]);

	rcc_periph_reset_pulse(RCC_AHB1RSTR);
	rcc_periph_reset_pulse(RCC_AHB2RSTR);
	rcc_periph_clock_enable(RCC_AHB1ENR);
	rcc_periph_clock_enable(RCC_AHB2ENR);

	/* Enable GPIOA clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);

	systick_set_reload(UINT32_MAX);
	// systick_set_frequency(1000, 80000000);
	systick_set_frequency(1000, 80000000);

	systick_interrupt_enable();
	systick_counter_enable();

	return;
}

// static void trace_setup(void)
// {
// 	uint32_t SWOSpeed = 4500000;
// 	SCS_DEMCR |= SCS_DEMCR_TRCENA;
// 	DBGMCU_CR = DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE_ASYNC;
// 	TPIU_SPPR = TPIU_SPPR_ASYNC_NRZ;
// 	TPIU_ACPR = (90000000 / SWOSpeed * 2) - 1;

// 	TPIU_FFCR &= ~TPIU_FFCR_ENFCONT;

// 	*((volatile uint32_t *)0xE0000FB0) = 0xC5ACCE55;
// 	ITM_TCR = (1 << 16) | ITM_TCR_ITMENA;
// 	/* Enable stimulus port 1. */
// 	ITM_TER[0] = 1;
// }

static void usart_setup(void)
{
	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO1 | GPIO2);

	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO1 | GPIO2);
}

static void trace_send_blocking(const uint8_t c)
{
	while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY))
		;

	ITM_STIM8(0) = c;
}


void trace_buffer(const uint8_t *buffer, size_t len)
{
	for (const uint8_t *c = buffer; (c - buffer) < len; c++)
	{
		trace_send_blocking(*c);
	}
}

int init(void)
{
	int i, j = 0, c = 0;

	clock_setup();
	gpio_setup();
	usart_setup();
	// trace_setup();

	return 0;
}

/* back up the cursor one space */
static inline void back_up(void)
{
	end_ndx = dec_ndx(end_ndx);
	usart_send_blocking(USART2, '\010');
	usart_send_blocking(USART2, ' ');
	usart_send_blocking(USART2, '\010');
}

/*
 * A buffered line editing function.
 */
void get_buffered_line(void)
{
	char c;

	if (start_ndx != end_ndx)
	{
		return;
	}
	while (1)
	{
		c = usart_recv_blocking(USART2);
		if (c == '\r')
		{
			buf[end_ndx] = '\n';
			end_ndx = inc_ndx(end_ndx);
			buf[end_ndx] = '\0';
			usart_send_blocking(USART2, '\r');
			usart_send_blocking(USART2, '\n');
			return;
		}
		/* ^H or DEL erase a character */
		if ((c == '\010') || (c == '\177'))
		{
			if (buf_len == 0)
			{
				usart_send_blocking(USART2, '\a');
			}
			else
			{
				back_up();
			}
			/* ^W erases a word */
		}
		else if (c == 0x17)
		{
			while ((buf_len > 0) &&
				   (!(isspace((int)buf[end_ndx]))))
			{
				back_up();
			}
			/* ^U erases the line */
		}
		else if (c == 0x15)
		{
			while (buf_len > 0)
			{
				back_up();
			}
			/* Non-editing character so insert it */
		}
		else
		{
			if (buf_len == (BUFLEN - 1))
			{
				usart_send_blocking(USART2, '\a');
			}
			else
			{
				buf[end_ndx] = c;
				end_ndx = inc_ndx(end_ndx);
				usart_send_blocking(USART2, c);
			}
		}
	}
}

/*
 * Called by libc stdio fwrite functions
 */
int _write(int fd, char *ptr, int len)
{
	int i = 0;

	/*
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 *
	 * Only work for STDOUT, STDIN, and STDERR
	 */
	if (fd > 2)
	{
		return -1;
	}
	while (*ptr && (i < len))
	{
		usart_send_blocking(USART2, *ptr);
		if (*ptr == '\n')
		{
			usart_send_blocking(USART2, '\r');
		}
		i++;
		ptr++;
	}
	return i;
}

/*
 * Called by the libc stdio fread fucntions
 *
 * Implements a buffered read with line editing.
 */
int _read(int fd, char *ptr, int len)
{
	int my_len;

	if (fd > 2)
	{
		return -1;
	}

	get_buffered_line();
	my_len = 0;
	while ((buf_len > 0) && (len > 0))
	{
		*ptr++ = buf[start_ndx];
		start_ndx = inc_ndx(start_ndx);
		my_len++;
		len--;
	}
	return my_len; /* return the length we got */
}

uint32_t ms_since_boot = 0;

void sys_tick_handler(void)
{
	ms_since_boot++;
}

int _times(struct tms *buf)
{
	buf->tms_utime = (ms_since_boot) / (1000 / CLOCKS_PER_SEC);
	buf->tms_cutime = 0;
	buf->tms_stime = 0;
	buf->tms_cstime = 0;
	return 0;
}
typedef void (*funcp_t)(void);
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;
void _fini(void)
{
	printf("_fini\n");
	for (funcp_t *fp = &__fini_array_start; fp < &__fini_array_end; fp++)
	{
		(*fp)();
	}
	while (1)
	{
	}
}