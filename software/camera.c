#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>

#include "i2c_util.h"
#include "imx258_regs.h"

#include "camera.h"

#define CAM_ADDR 0x1a

// I2C IO functions with 16 bit addressing and 8/16 bit data
static bool cam_addr(unsigned char slave_addr, unsigned short addr) {
	i2c_start();
	if(!i2c_transmit_byte(I2C_ADDR_WR(slave_addr))) {
		i2c_stop();
		return false;
	}
	if(!i2c_transmit_byte((addr >> 8) & 0xFF)) {
		i2c_stop();
		return false;
	}
	if(!i2c_transmit_byte(addr & 0xFF)) {
		i2c_stop();
		return false;
	}
	return true;
}
static unsigned char cam_read8(unsigned char slave_addr, unsigned short addr)
{
	if (!cam_addr(slave_addr, addr)) {
		return 0xFF;
	}
	i2c_start();

	if(!i2c_transmit_byte(I2C_ADDR_RD(slave_addr))) {
		i2c_stop();
		return 0xFF;
	}
	unsigned char data = i2c_receive_byte(1);

	i2c_stop();

	return data;
}

static unsigned short cam_read16(unsigned char slave_addr, unsigned short addr)
{
	if (!cam_addr(slave_addr, addr)) {
		return 0xFF;
	}
	i2c_start();
	if(!i2c_transmit_byte(I2C_ADDR_RD(slave_addr))) {
		i2c_stop();
		return 0xFF;
	}
	unsigned char data_h = i2c_receive_byte(1);
	unsigned char data_l = i2c_receive_byte(1);
	i2c_stop();
	return (data_h << 8U) | data_l;
}

static bool cam_write8(unsigned char slave_addr, unsigned short addr, unsigned char data)
{
	if (!cam_addr(slave_addr, addr)) {
		return 0xFF;
	}
	if(!i2c_transmit_byte(data)) {
		i2c_stop();
		return false;
	}
	i2c_stop();

	return true;
}

static bool run_init_sequence(const struct imx258_reg *regs, int count)
{
	int i;
	for (i = 0; i < count; i++) {
		if (!cam_write8(CAM_ADDR, regs[i].address, regs[i].val)) {
			printf("write %d failed (addr=%04x, data=%02x)!\n", i, regs[i].address, regs[i].val);
			return false;
		} 
	}
	return true;
}

// From the Lattice reference design
static const struct imx258_reg lattice_rd_cfg[] = {
	{0x0100, 0x00}, // standby
	{0x0136, 0x1B}, // external clock 27MHz
	{0x0137, 0x00}, //
	{0x0105, 0x00}, // mask corrupted frames - on
	{0x0106, 0x00}, // fast standby - on
	{0x0114, 0x03}, // CSI lane mode - 4 lines
	{0x0220, 0x00}, // HDR mode - normal
	{0x0221, 0x00}, // resolution reduction
	{0x0222, 0x01}, // exposure ratio,

	// random stuff
	{ 0x3051, 0x00 },
	{ 0x6B11, 0xCF },
	{ 0x7FF0, 0x08 },
	{ 0x7FF1, 0x0F },
	{ 0x7FF2, 0x08 },
	{ 0x7FF3, 0x1B },
	{ 0x7FF4, 0x23 },
	{ 0x7FF5, 0x60 },
	{ 0x7FF6, 0x00 },
	{ 0x7FF7, 0x01 },
	{ 0x7FF8, 0x00 },
	{ 0x7FF9, 0x78 },
	{ 0x7FFA, 0x00 },
	{ 0x7FFB, 0x00 },
	{ 0x7FFC, 0x00 },
	{ 0x7FFD, 0x00 },
	{ 0x7FFE, 0x00 },
	{ 0x7FFF, 0x03 },
	{ 0x7F76, 0x03 },
	{ 0x7F77, 0xFE },
	{ 0x7FA8, 0x03 },
	{ 0x7FA9, 0xFE },
	{ 0x7B24, 0x81 },
	{ 0x6564, 0x07 },
	{ 0x6B0D, 0x41 },
	{ 0x653D, 0x04 },
	{ 0x6B05, 0x8C },
	{ 0x6B06, 0xF9 },
	{ 0x6B08, 0x65 },
	{ 0x6B09, 0xFC },
	{ 0x6B0A, 0xCF },
	{ 0x6B0B, 0xD2 },
	{ 0x6700, 0x0E },
	{ 0x6707, 0x0E },
	{ 0x9104, 0x00 },
	{ 0x4648, 0x7F },
	{ 0x7420, 0x00 },
	{ 0x7421, 0x1C },
	{ 0x7422, 0x00 },
	{ 0x7423, 0xD7 },
	{ 0x5F04, 0x00 },
	{ 0x5F05, 0xED },

	{ 0x94DC, 0x20 },
	{ 0x94DD, 0x20 },
	{ 0x94DE, 0x20 },
	{ 0x95DC, 0x20 },
	{ 0x95DD, 0x20 },
	{ 0x95DE, 0x20 },
	{ 0x7FB0, 0x00 },
	{ 0x9010, 0x3E },
	{ 0x9419, 0x50 },
	{ 0x941B, 0x50 },
	{ 0x9519, 0x50 },
	{ 0x951B, 0x50 },

	{ 0x3052, 0x00 },
	{ 0x4e21, 0x14 },
	{ 0x7b25, 0x00 },


	{0x0340, 0x04}, // lines per frame MSB
	{0x0341, 0x5C}, // lines per frame LSB
	{0x0342, 0x14}, // pixels per line MSB
	{0x0343, 0xE8}, // pixels per line LSB
	{0x0344, 0x00}, // X address start MSB
	{0x0345, 0x00}, // X address start LSB
	{0x0346, 0x00}, // Y address start MSB
	{0x0347, 0x00}, // Y address start LSB
	{0x0348, 0x10}, // X address end MSB
	{0x0349, 0x6F}, // X address end LSB
	{0x034A, 0x0C}, // Y address end MSB
	{0x034B, 0x2F}, // Y address end LSB
	{0x0381, 0x01}, // 01 X increment for even pixels
	{0x0383, 0x01}, // 01 X increment for odd pixels
	{0x0385, 0x01}, // 01 Y increment for even pixels
	{0x0387, 0x01}, // 01 Y increment for odd pixels
	{0x0900, 0x01}, // binning mode - enable
	{0x0901, 0x14}, // binning type - V:1/4
	{0x0902, 0x00}, // binning weight - addition
	{0x0112, 0x0A}, // data format - RAW10
	{0x0113, 0x0A},
	{0x034C, 0x04}, // output X crop MSB
	{0x034D, 0x18}, // output X crop LSB
	{0x034E, 0x03}, // output Y crop MSB
	{0x034F, 0x0c}, // output Y crop LSB
	{0x0401, 0x01}, // scaling mode - on
	{0x0404, 0x00}, // scaling mode MSB
	{0x0405, 0x40}, // scaling mode LSB
	{0x3038, 0x00}, // IMX258_REG_SCALE_MODE_EXT
	{0x303a, 0x00}, // scale M ext MSB
	{0x303b, 0x10}, // scale M ext LSB
	{0x0408, 0x00}, // digital crop X offset MSB
	{0x0409, 0x00}, // digital crop X offset LSB
	{0x040A, 0x00}, // digital crop Y offset MSB
	{0x040B, 0x00}, // digital crop Y offset LSB
	{0x040C, 0x10}, // digital crop width MSB
	{0x040D, 0x70}, // digital crop width LSB
	{0x040E, 0x03}, // digital crop height MSB
	{0x040F, 0x0C}, // digital crop height LSB
	{0x0301, 0x05}, // pixel clock divider
	{0x0303, 0x02}, // system clock divider
	{0x0305, 0x04}, // pre PLL clock divider
	{0x0306, 0x00}, // PLL multiplier MSB
	{0x0307, 0x6E}, // PLL multiplier LSB
	{0x0309, 0x0A}, // output pixel clock divider
	{0x030B, 0x02}, // output system clock divider
	{0x0310, 0x01}, // PLL mode - dual
	{0x030D, 0x02}, // pre PLL clock op divider
	{0x030E, 0x00}, // PLL op multiplier MSB
	{0x030F, 0x37}, // PLL op multiplier LSB
	{0x0820, 0x05}, // link bit rate - 32 bits MSB
	{0x0821, 0xCD}, // link bit rate
	{0x0822, 0x00}, // link bit rate
	{0x0823, 0x00}, // link bit rate - 32 bits LSB
	{0x0B06, 0x01}, // dynamic defect compensation - enable
	{0x5A5C, 0x01}, // master mode
	{0x0202, 0x03}, // coarse integration time MSB
	{0x0203, 0x20}, // coarse integration time LSB
	{0x0224, 0x01}, // short integration time MSB
	{0x0225, 0x80}, // short integration time LSB
	{0x0204, 0x01}, // analogue gain
	{0x0205, 0x00},
	{0x020E, 0x04}, // digital gain - Gr MSB
	{0x020F, 0x00}, // digital gain - Gr LSB
	{0x0210, 0x04}, // digital gain - R MSB
	{0x0211, 0x00}, // digital gain - R LSB
	{0x0212, 0x04}, // digital gain - B MSB
	{0x0213, 0x00}, // digital gain - B LSB
	{0x0214, 0x04}, // digital gain - Gb MSB
	{0x0215, 0x00}, // digital gain - Gb LSB
	{0x0216, 0x00}, // short analogue gain
	{0x0217, 0x00}, // short analogue gain
	{0x4040, 0x00}, // clock mode during blanking - continuous
	{0x0350, 0x00}, // frame length control - auto
	{0x0808, 0x00}, // DPHY control - use UI control

	{0x0600, 0x00}, // test pattern mode MSB
	{0x0601, 0x03}, // test pattern mode LSB

	{0x0602, 0x00}, // test pattern red MSB
	{0x0603, 0x00}, // test pattern red LSB


	{0x0604, 0x03}, // test pattern gr MSB
	{0x0605, 0xFF}, // test pattern gr LSB

	{0x0606, 0x00}, // test pattern blue MSB
	{0x0607, 0x00}, // test pattern blue LSB

	{0x0608, 0x03}, // test pattern gb MSB
	{0x0609, 0xFF}, // test pattern gb LSB


	{0x0100, 0x01}, // start streaming
};

void camera_init(void) {
	printf("IMX258_REG_CHIP_ID %04x\n", cam_read16(CAM_ADDR, IMX258_REG_CHIP_ID));
	run_init_sequence(lattice_rd_cfg, ARRAY_SIZE(lattice_rd_cfg));
}
