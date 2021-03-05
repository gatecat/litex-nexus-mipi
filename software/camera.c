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

void camera_init(void) {
	printf("IMX258_REG_CHIP_ID %04x\n", cam_read16(CAM_ADDR, IMX258_REG_CHIP_ID));
}
