#ifndef I2C_UTIL_H
#define I2C_UTIL_H

#include <stdbool.h>

/* I2C frequency defaults to a safe value in range 10-100 kHz to be compatible with SMBus */
#ifndef I2C_FREQ_HZ
#define I2C_FREQ_HZ  50000
#endif

#define I2C_ADDR_WR(addr) ((addr) << 1)
#define I2C_ADDR_RD(addr) (((addr) << 1) | 1u)

#define I2C_PERIOD_CYCLES (CONFIG_CLOCK_FREQUENCY / I2C_FREQ_HZ)
#define I2C_DELAY(n)	  cdelay((n)*I2C_PERIOD_CYCLES/4)

static inline void cdelay(int i)
{
	while(i > 0) {
		__asm__ volatile(CONFIG_CPU_NOP);
		i--;
	}
}

static inline void i2c_oe_scl_sda(bool oe, bool scl, bool sda)
{
	i2c_w_write(
		((oe & 1)  << CSR_I2C_W_OE_OFFSET)	|
		((scl & 1) << CSR_I2C_W_SCL_OFFSET) |
		((sda & 1) << CSR_I2C_W_SDA_OFFSET)
	);
}

// START condition: 1-to-0 transition of SDA when SCL is 1
static void i2c_start(void)
{
	i2c_oe_scl_sda(1, 1, 1);
	I2C_DELAY(1);
	i2c_oe_scl_sda(1, 1, 0);
	I2C_DELAY(1);
	i2c_oe_scl_sda(1, 0, 0);
	I2C_DELAY(1);
}

// STOP condition: 0-to-1 transition of SDA when SCL is 1
static void i2c_stop(void)
{
	i2c_oe_scl_sda(1, 0, 0);
	I2C_DELAY(1);
	i2c_oe_scl_sda(1, 1, 0);
	I2C_DELAY(1);
	i2c_oe_scl_sda(1, 1, 1);
	I2C_DELAY(1);
	i2c_oe_scl_sda(0, 1, 1);
}

// Call when in the middle of SCL low, advances one clk period
static void i2c_transmit_bit(int value)
{
	i2c_oe_scl_sda(1, 0, value);
	I2C_DELAY(1);
	i2c_oe_scl_sda(1, 1, value);
	I2C_DELAY(2);
	i2c_oe_scl_sda(1, 0, value);
	I2C_DELAY(1);
	i2c_oe_scl_sda(0, 0, 0);  // release line
}

// Call when in the middle of SCL low, advances one clk period
static int i2c_receive_bit(void)
{
	int value;
	i2c_oe_scl_sda(0, 0, 0);
	I2C_DELAY(1);
	i2c_oe_scl_sda(0, 1, 0);
	I2C_DELAY(1);
	// read in the middle of SCL high
	value = i2c_r_read() & 1;
	I2C_DELAY(1);
	i2c_oe_scl_sda(0, 0, 0);
	I2C_DELAY(1);
	return value;
}

// Send data byte and return 1 if slave sends ACK
static bool i2c_transmit_byte(unsigned char data)
{
	int i;
	int ack;

	// SCL should have already been low for 1/4 cycle
	i2c_oe_scl_sda(0, 0, 0);
	for (i = 0; i < 8; ++i) {
		// MSB first
		i2c_transmit_bit((data & (1 << 7)) != 0);
		data <<= 1;
	}
	ack = i2c_receive_bit();

	// 0 from slave means ack
	return ack == 0;
}

// Read data byte and send ACK if ack=1
static unsigned char i2c_receive_byte(bool ack)
{
	int i;
	unsigned char data = 0;

	for (i = 0; i < 8; ++i) {
		data <<= 1;
		data |= i2c_receive_bit();
	}
	i2c_transmit_bit(!ack);

	return data;
}

#endif
