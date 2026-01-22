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
	{0x0105, 0x00}, // mask corrupted frames - on
	{0x0106, 0x00}, // fast standby - on
	{0x0114, 0x03}, // CSI lane mode - 4 lines
	{0x0220, 0x00}, // HDR mode - normal
	{0x0221, 0x11}, // resolution reduction
	{0x0222, 0x01}, // exposure ratio,
	{0x0340, 0x04}, // lines per frame MSB
	{0x0341, 0x5C}, // lines per frame LSB
	{0x0342, 0x15}, // pixels per line MSB
	{0x0343, 0x18}, // pixels per line LSB
	{0x0344, 0x00}, // X address start MSB
	{0x0345, 0x00}, // X address start LSB
	{0x0346, 0x00}, // Y address start MSB
	{0x0347, 0x00}, // Y address start LSB
	{0x0348, 0x0E}, // X address end MSB
	{0x0349, 0xFF}, // X address end LSB
	{0x034A, 0x08}, // Y address end MSB
	{0x034B, 0x6F}, // Y address end LSB
	{0x0381, 0x01}, // 01 X increment for even pixels
	{0x0383, 0x01}, // 01 X increment for odd pixels
	{0x0385, 0x01}, // 01 Y increment for even pixels
	{0x0387, 0x01}, // 01 Y increment for odd pixels
	{0x0900, 0x00}, // binning mode - enable
	{0x0901, 0x12}, // binning type - V:1/2
	{0x0902, 0x00}, // binning weight - addition
	{0x0112, 0x0A}, // data format - RAW10
	{0x0113, 0x0A},
	{0x034C, 0x07}, // output X crop MSB
	{0x034D, 0x80}, // output X crop LSB
	{0x034E, 0x04}, // output Y crop MSB
	{0x034F, 0x38}, // output Y crop LSB
	{0x0401, 0x00}, // scaling mode - no scaling
	{0x0408, 0x00}, // digital crop X offset MSB
	{0x0409, 0x00}, // digital crop X offset LSB
	{0x040A, 0x00}, // digital crop Y offset MSB
	{0x040B, 0x00}, // digital crop Y offset LSB
	{0x040C, 0x07}, // digital crop width MSB
	{0x040D, 0x80}, // digital crop width LSB
	{0x040E, 0x04}, // digital crop height MSB
	{0x040F, 0x38}, // digital crop height LSB
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
	{0x0205, 0x80},
	{0x020E, 0x03}, // digital gain - Gr MSB
	{0x020F, 0x00}, // digital gain - Gr LSB
	{0x0210, 0x04}, // digital gain - R MSB
	{0x0211, 0x80}, // digital gain - R LSB
	{0x0212, 0x05}, // digital gain - B MSB
	{0x0213, 0x00}, // digital gain - B LSB
	{0x0214, 0x03}, // digital gain - Gb MSB
	{0x0215, 0x00}, // digital gain - Gb LSB
	{0x0216, 0x00}, // short analogue gain
	{0x0217, 0x00}, // short analogue gain
	{0x4040, 0x00}, // clock mode during blanking - continuous
	{0x0350, 0x00}, // frame length control - auto
	{0x0808, 0x00}, // DPHY control - use UI control

	{0x0600, 0x00}, // test pattern mode MSB
	{0x0601, 0x00}, // test pattern mode LSB

	{0x0100, 0x01}, // start streaming
};

void camera_init(void) {
	printf("IMX258_REG_CHIP_ID %04x\n", cam_read16(CAM_ADDR, IMX258_REG_CHIP_ID));
	run_init_sequence(lattice_rd_cfg, ARRAY_SIZE(lattice_rd_cfg));
}
