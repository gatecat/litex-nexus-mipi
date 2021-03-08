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

void camera_init(void) {
	printf("IMX258_REG_CHIP_ID %04x\n", cam_read16(CAM_ADDR, IMX258_REG_CHIP_ID));
	run_init_sequence(mipi_data_rate_640mbps, ARRAY_SIZE(mipi_data_rate_640mbps));
	run_init_sequence(mode_1048_780_regs, ARRAY_SIZE(mode_1048_780_regs));
	cam_write8(CAM_ADDR, IMX258_REG_MODE_SELECT, IMX258_MODE_STREAMING);
}
