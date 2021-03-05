#ifndef IMX258_REGS
#define IMX258_REGS

// From https://github.com/torvalds/linux/blob/master/drivers/media/i2c/imx258.c

#define IMX258_REG_MODE_SELECT		0x0100
#define IMX258_MODE_STANDBY		0x00
#define IMX258_MODE_STREAMING		0x01

/* Chip ID */
#define IMX258_REG_CHIP_ID		0x0016
#define IMX258_CHIP_ID			0x0258

/* V_TIMING internal */
#define IMX258_VTS_30FPS		0x0c98
#define IMX258_VTS_30FPS_2K		0x0638
#define IMX258_VTS_30FPS_VGA		0x034c
#define IMX258_VTS_MAX			0xffff

/*Frame Length Line*/
#define IMX258_FLL_MIN			0x08a6
#define IMX258_FLL_MAX			0xffff
#define IMX258_FLL_STEP			1
#define IMX258_FLL_DEFAULT		0x0c98

/* HBLANK control - read only */
#define IMX258_PPL_DEFAULT		5352

/* Exposure control */
#define IMX258_REG_EXPOSURE		0x0202
#define IMX258_EXPOSURE_MIN		4
#define IMX258_EXPOSURE_STEP		1
#define IMX258_EXPOSURE_DEFAULT		0x640
#define IMX258_EXPOSURE_MAX		65535

/* Analog gain control */
#define IMX258_REG_ANALOG_GAIN		0x0204
#define IMX258_ANA_GAIN_MIN		0
#define IMX258_ANA_GAIN_MAX		0x1fff
#define IMX258_ANA_GAIN_STEP		1
#define IMX258_ANA_GAIN_DEFAULT		0x0

/* Digital gain control */
#define IMX258_REG_GR_DIGITAL_GAIN	0x020e
#define IMX258_REG_R_DIGITAL_GAIN	0x0210
#define IMX258_REG_B_DIGITAL_GAIN	0x0212
#define IMX258_REG_GB_DIGITAL_GAIN	0x0214
#define IMX258_DGTL_GAIN_MIN		0
#define IMX258_DGTL_GAIN_MAX		4096	/* Max = 0xFFF */
#define IMX258_DGTL_GAIN_DEFAULT	1024
#define IMX258_DGTL_GAIN_STEP		1

/* Test Pattern Control */
#define IMX258_REG_TEST_PATTERN		0x0600

/* Orientation */
#define REG_MIRROR_FLIP_CONTROL		0x0101
#define REG_CONFIG_MIRROR_FLIP		0x03
#define REG_CONFIG_FLIP_TEST_PATTERN	0x02

#endif
