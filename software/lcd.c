#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>


#include "lcd.h"


#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1 0xDA
#define ST77XX_RDID2 0xDB
#define ST77XX_RDID3 0xDC
#define ST77XX_RDID4 0xDD

// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00


// Some register settings
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH 0x04

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Some ready-made 16-bit ('565') color settings:
#define ST7735_BLACK ST77XX_BLACK
#define ST7735_WHITE ST77XX_WHITE
#define ST7735_RED ST77XX_RED
#define ST7735_GREEN ST77XX_GREEN
#define ST7735_BLUE ST77XX_BLUE
#define ST7735_CYAN ST77XX_CYAN
#define ST7735_MAGENTA ST77XX_MAGENTA
#define ST7735_YELLOW ST77XX_YELLOW
#define ST7735_ORANGE ST77XX_ORANGE

static void reset_lcd() {
	lcd_gpio_out_write(0x00);
	cdelay(1000);
	lcd_gpio_out_write(0x02);
	cdelay(1000);
}

static void lcd_write_cmd(uint8_t data) {
	lcd_gpio_out_write(0x02);
	lcd_spi_cs_write(0x01);
	lcd_spi_mosi_write(data);
	lcd_spi_control_write(0x0801); // start 8 bit write
	while ((lcd_spi_status_read() & 0x1) == 0x0)
		;
}

static void lcd_write_param(uint8_t data) {
	lcd_gpio_out_write(0x03);
	lcd_spi_cs_write(0x01);
	lcd_spi_mosi_write(data);
	lcd_spi_control_write(0x0801); // start 8 bit write
	while ((lcd_spi_status_read() & 0x1) == 0x0)
		;
}


static void lcd_write_data(uint16_t data) {
	lcd_gpio_out_write(0x03);
	lcd_spi_cs_write(0x01);
	lcd_spi_mosi_write(data);
	lcd_spi_control_write(0x1001); // start 8 bit write
	while ((lcd_spi_status_read() & 0x1) == 0x0)
		;
}

void lcd_init(void) {
	reset_lcd();

    lcd_write_cmd(ST77XX_SWRESET); //  1: Software reset, 0 args, w/delay
    cdelay(1500);                          //     150 ms delay
    lcd_write_cmd(ST77XX_SLPOUT); //  2: Out of sleep mode, 0 args, w/delay
    cdelay(1500);                          //     150 ms delay
    lcd_write_cmd(ST7735_FRMCTR1);              //  3: Framerate ctrl - normal mode, 3 arg:
    lcd_write_param(0x01);
    lcd_write_param(0x2C);
    lcd_write_param(0x2D);             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    lcd_write_cmd(ST7735_FRMCTR2);              //  4: Framerate ctrl - idle mode, 3 args:
    lcd_write_param(0x01);
    lcd_write_param(0x2C);
    lcd_write_param(0x2D);             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    lcd_write_cmd(ST7735_FRMCTR3);              //  5: Framerate - partial mode, 6 args:
    lcd_write_param(0x01);
    lcd_write_param(0x2C);
    lcd_write_param(0x2D);             //     Dot inversion mode
    lcd_write_param(0x01); 
    lcd_write_param(0x2C);
    lcd_write_param(0x2D);             //     Line inversion mode
    lcd_write_cmd(ST7735_INVCTR);              //  6: Display inversion ctrl, 1 arg:
    lcd_write_param(0x07);                         //     No inversion
    lcd_write_cmd(ST7735_PWCTR1);              //  7: Power control, 3 args, no delay:
    lcd_write_param(0xA2);
    lcd_write_param(0x02);                         //     -4.6V
    lcd_write_param(0x84);                         //     AUTO mode
    lcd_write_cmd(ST7735_PWCTR2);              //  8: Power control, 1 arg, no delay:
    lcd_write_param(0xC5);                         //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
    lcd_write_cmd(ST7735_PWCTR3);              //  9: Power control, 2 args, no delay:
    lcd_write_param(0x0A);                         //     Opamp current small
    lcd_write_param(0x00);                         //     Boost frequency
    lcd_write_cmd(ST7735_PWCTR4);              // 10: Power control, 2 args, no delay:
    lcd_write_param(0x8A);                         //     BCLK/2,
    lcd_write_param(0x2A);                         //     opamp current small & medium low
    lcd_write_cmd(ST7735_PWCTR5);              // 11: Power control, 2 args, no delay:
    lcd_write_param(0x8A);
    lcd_write_param(0xEE);
    lcd_write_cmd(ST7735_VMCTR1);              // 12: Power control, 1 arg, no delay:
    lcd_write_param(0x0E);
    lcd_write_cmd(ST77XX_INVOFF);              // 13: Don't invert display, no args
    lcd_write_cmd(ST77XX_MADCTL);              // 14: Mem access ctl (directions), 1 arg:
    lcd_write_param(0xC8);                         //     row/col addr, bottom-top refresh
    lcd_write_cmd(ST77XX_COLMOD);             // 15: set color mode, 1 arg, no delay:
    lcd_write_param(0x05);                      //     16-bit color

    lcd_write_cmd(ST77XX_CASET);             //  1: Column addr set, 4 args, no delay:
    lcd_write_param(0x00);
    lcd_write_param(0x00);                   //     XSTART = 0
    lcd_write_param(0x00);
    lcd_write_param(0x7F);                   //     XEND = 127
    lcd_write_cmd(ST77XX_RASET);              //  2: Row addr set, 4 args, no delay:
    lcd_write_param(0x00);
    lcd_write_param(0x00);                   //     XSTART = 0
    lcd_write_param(0x00);
    lcd_write_param(0x7F);                 //     XEND = 127

    cdelay(1500);                          //     150 ms delay

   	lcd_write_cmd(ST77XX_NORON);

    cdelay(1500);                          //     150 ms delay

   	lcd_write_cmd(ST77XX_DISPON);


   	lcd_write_cmd(ST77XX_RAMWR);
   	for (int y = 0; y < 128; y++) {
   		for (int x = 0; x < 128; x++) {
   			lcd_write_data(ST7735_RED);
   		}
   	}
}

void lcd_write(void) {
   	for (int y = 0; y < 128; y++) {
   		for (int x = 0; x < 128; x++) {
   			lcd_write_data(ST7735_RED);
   		}
   	}
}