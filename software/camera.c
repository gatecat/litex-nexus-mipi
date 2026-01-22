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
	{0x0136, 0x1B}, //  EXCK_FREQ[15:8]  INCK 27MHZ
	{0x0137, 0x00}, //  EXCK_FREQ[7:0]  .0MHZ
	{0x0105, 0x00}, //  MASK_CORR_FRM 0:transmit 1:mask
	{0x0106, 0x00}, //  FAST_STANDBY_CTL 0:after frame 1:immediate
	{0x0114, 0x03}, //  CSI_LANE_MODE 4 LANE
	{0x0220, 0x00}, //  HDR_MODE[5:0] 0:disable
	{0x0221, 0x11}, //  HDR_RESO_REDU_H and V Full-res
	{0x0222, 0x01}, //  EXPO_RATIO 1
	{0x0340, 0x06}, //  FRM_LENGTH_LINES
	{0x0341, 0x38}, //  FRM_LENGTH_LINES  0x638 = 1592   
	{0x0342, 0x14}, //  LINE_LENGTH_PCK
	{0x0343, 0xE8}, //  LINE_LENGTH_PCK 0x14E8 = 5352
	{0x0344, 0x08}, //  X_ADD_START[12:8]  Cropping
	{0x0345, 0x00}, //  X_ADD_START[7:0]   0x800=2048 
	{0x0346, 0x01}, //  Y_ADD_START[11:8]
	{0x0347, 0x00}, //  Y_ADD_START[7:0]   0x100=256
	{0x0348, 0x10}, //  X_ADD_END[12:8]  Cropping
	{0x0349, 0x00}, //  X_ADD_END[7:0]    0x1000=4096
	{0x034A, 0x06}, //  Y_ADD_END[11:8]   
	{0x034B, 0x38}, //  Y_ADD_END[7:0] 0x638 = 1592 
	{0x0381, 0x01}, //  X_EVN_INC
	{0x0383, 0x01}, //  X_ODD_INC
	{0x0385, 0x01}, //  Y_EVN_INC
	{0x0387, 0x01}, //  Y_ODD_INC
	{0x0900, 0x00}, //  BINNING_MODE 0:disable
	{0x0901, 0x12}, //  BINNING_TYPE
	{0x0902, 0x00}, //  BINNING_WEIGHT 0:Average
	{0x0112, 0x0A}, //  CSI_DT_FMT_H 0A:RAW10
	{0x0113, 0x0A}, //  CSI_DT_FMT_L 0A:RAW10
	{0x034C, 0x07}, //  X_OUT_SIZE
	{0x034D, 0x80}, //  X_OUT_SIZE  0x780=1920
	{0x034E, 0x04}, //  Y_OUT_SIZE 
	{0x034F, 0x38}, //  Y_OUT_SIZE  0x438=1080
	{0x0401, 0x00}, //  SCALE_MODE 0:None
	{0x0408, 0x00}, //  DIG_CROP_X_OFFSET
	{0x0409, 0x00}, //  DIG_CROP_X_OFFSET
	{0x040A, 0x00}, //  DIG_CROP_Y_OFFSET
	{0x040B, 0x00}, //  DIG_CROP_Y_OFFSET
	{0x040C, 0x07}, //  DIG_CROP_IMAGE_WIDTH
	{0x040D, 0x80}, //  DIG_CROP_IMAGE_WIDTH 0x780=1920
	{0x040E, 0x04}, //  DIG_CROP_IMAGE_HEIGHT
	{0x040F, 0x38}, //  DIG_CROP_IMAGE_HEIGHT 0x438=1080
	{0x0301, 0x05}, //  IVTPXCK_DIV 5
	{0x0303, 0x02}, //  IVTSYCK_DIV 2
	{0x0305, 0x04}, //  PREPLLCK_VT_DIV
	{0x0306, 0x00}, //  PLL_IVT_MPY[10:8]
	{0x0307, 0x6E}, //  PLL_IVT_MPY[7:0] 0x6E=110 
	{0x0309, 0x0A}, //  IOPPXCK_DIV
	{0x030B, 0x02}, //  IOPSYCK_DIV  
	{0x030D, 0x02}, //  PREPLLCK_OP_DIV 2
	{0x030E, 0x00}, //  PLL_IOP_MPY[10:8]
	{0x030F, 0x37}, //  PLL_IOP_MPY[7:0]  0x37 = 55
	{0x0310, 0x01}, //  PLL_MULT_DRIV 0:Single PLL 1:Dual Mode 
	{0x0820, 0x05}, //  REQ_LINK_BIT_RATE_MBPS[31:24]
	{0x0821, 0xCD}, //  REQ_LINK_BIT_RATE_MBPS[23:16]
	{0x0822, 0x00}, //  REQ_LINK_BIT_RATE_MBPS[15:8]
	{0x0823, 0x00}, //  REQ_LINK_BIT_RATE_MBPS[7:0]  0x5CD = 0x0A00=2560Mbps
	{0x0B06, 0x01}, //  SING_DEF_CORR_EN 1:Enable
	{0x5A5C, 0x01}, //  MASTER_SLAVE 1:Master
	{0x0202, 0x04}, //  COARSE_INTEG_TIME
	{0x0203, 0x38}, //  COARSE_INTEG_TIME  0x438=1080
	{0x0224, 0x01}, //  ST_COARSE_INTEG_TIME
	{0x0225, 0x80}, //  ST_COARSE_INTEG_TIME  0x180=384
	{0x0204, 0x01}, //  ANA_GAIN_GLOBAL
	{0x0205, 0x00}, //  ANA_GAIN_GLOBAL  0x100=256
	{0x020E, 0x04}, //  DIG_GAIN_GR
	{0x020F, 0x00}, //  DIG_GAIN_GR  0x400=1024
	{0x0210, 0x04}, //  DIG_GAIN_R
	{0x0211, 0x00}, //  DIG_GAIN_R  0x400=1024
	{0x0212, 0x04}, //  DIG_GAIN_B
	{0x0213, 0x00}, //  DIG_GAIN_B  0x400=1024
	{0x0214, 0x04}, //  DIG_GAIN_GB
	{0x0215, 0x00}, //  DIG_GAIN_GB  0x400=1024
	{0x0216, 0x00}, //  ST_ANA_GAIN_GLOBAL
	{0x0217, 0x40}, //  ST_ANA_GAIN_GLOBAL  0x40=64
	{0x4040, 0x00}, //  CLBLANKSTOP 0:Stay in HS Mode
	{0x0350, 0x00}, //  FRM_LENGTH_CTL 0:No Automatic
	{0x0808, 0x00}, //  DPHY_CTRL 0:Automatic
	{0x3030, 0x00}, //  Disable Shield Pixels
	{0x0601, 0x00}, //  test pattern mode

	{0x0602, 0x00}, //  test pattern color -red
	{0x0603, 0x00}, //  test pattern color

	{0x0604, 0x00}, //  test pattern color -green r
	{0x0605, 0x00}, //  test pattern color

	{0x0606, 0x03}, //  test pattern color -blue
	{0x0607, 0xff}, //  test pattern color

	{0x0608, 0x00}, //  test pattern color -green b
	{0x0609, 0x00}, //  test pattern color


	{0x0100, 0x01}, //  mode select streaming on
};

void camera_init(void) {
	printf("IMX258_REG_CHIP_ID %04x\n", cam_read16(CAM_ADDR, IMX258_REG_CHIP_ID));
	run_init_sequence(lattice_rd_cfg, ARRAY_SIZE(lattice_rd_cfg));
}
