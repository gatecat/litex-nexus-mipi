// This file is Copyright (c) 2020 Florent Kermarrec <florent@enjoy-digital.fr>
// License: BSD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>
#include <generated/mem.h>

#include "camera.h"

/*-----------------------------------------------------------------------*/
/* Uart                                                                  */
/*-----------------------------------------------------------------------*/

static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if(readchar_nonblock()) {
		c[0] = readchar();
		c[1] = 0;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					putsnonl("\x08 \x08");
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				putsnonl("\n");
				ptr = 0;
				return s;
			default:
				if(ptr >= (sizeof(s) - 1))
					break;
				putsnonl(c);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}

	return NULL;
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}

static void prompt(void)
{
	printf("\e[92;1mlitex-mipi-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/

static void help(void)
{
	puts("\nLiteX MIPI app built "__DATE__" "__TIME__"\n");
	puts("Available commands:");
	puts("help               - Show this command");
	puts("reboot             - Reboot CPU");
	puts("cam_init           - Run camera initialisation");
	puts("freq               - Print frequency counter output");
	puts("data               - Print 32 words of received MIPI data");
	puts("packet             - Print 128 words of last received packet");
}

static void reboot_cmd(void)
{
	ctrl_reset_write(1);
}

static void read_freq_cmd(void)
{
	printf("Byte clk freq: %dHz\n", clk_byte_freq_value_read());
}

static void read_data_cmd(void)
{
	for (int i = 0; i < 32; i++)
		printf("%08x %08x %01x\n", dphy_header_in_read(), hs_rx_data_in_read(), hs_rx_sync_in_read());
}

static void read_packet_cmd(void)
{
	volatile unsigned *buf = (volatile unsigned *)PACKET_IO_BASE;
	for (int i = 0; i < 128; i++)
		printf("%08x\n", buf[i]);
}

static void console_service(void)
{
	char *str;
	char *token;

	str = readstr();
	if(str == NULL) return;
	token = get_token(&str);
	if(strcmp(token, "help") == 0)
		help();
	else if(strcmp(token, "reboot") == 0)
		reboot_cmd();
	else if(strcmp(token, "cam_init") == 0)
		camera_init();
	else if(strcmp(token, "freq") == 0)
		read_freq_cmd();
	else if(strcmp(token, "data") == 0)
		read_data_cmd();
	else if(strcmp(token, "packet") == 0)
		read_packet_cmd();
	prompt();
}

int main(void)
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
#endif
	uart_init();

	camera_init();

	help();
	prompt();


	while(1) {
		console_service();
	}

	return 0;
}
