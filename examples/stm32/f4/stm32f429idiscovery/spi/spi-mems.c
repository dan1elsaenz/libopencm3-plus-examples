/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * SPI Port example
 */

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3-plus/stm32f429idiscovery/clock.h>
#include <libopencm3-plus/stm32f429idiscovery/console.h>

/*
 * Functions defined for accessing the SPI port 8 bits at a time
 */
uint16_t read_reg(int reg);
void write_reg(uint8_t reg, uint8_t value);
uint8_t read_xyz(int16_t vecs[3]);
void spi_init(void);

/*
 * Chart of the various SPI ports (1 - 6) and where their pins can be:
 *
 *	 NSS		  SCK			MISO		MOSI
 *	 --------------   -------------------   -------------   ---------------
 * SPI1  PA4, PA15	  PA5, PB3		PA6, PB4	PA7, PB5
 * SPI2  PB9, PB12, PI0   PB10, PB13, PD3, PI1  PB14, PC2, PI2  PB15, PC3, PI3
 * SPI3  PA15*, PA4*	  PB3*, PC10*		PB4*, PC11*	PB5*, PD6, PC12*
 * SPI4  PE4,PE11	  PE2, PE12		PE5, PE13	PE6, PE14
 * SPI5  PF6, PH5	  PF7, PH6		PF8		PF9, PF11, PH7
 * SPI6  PG8		  PG13			PG12		PG14
 *
 * Pin name with * is alternate function 6 otherwise use alternate function 5.
 *
 * MEMS uses SPI5 - SCK (PF7), MISO (PF8), MOSI (PF9),
 * MEMS CS* (PC1)  -- GPIO
 * MEMS INT1 = PA1, MEMS INT2 = PA2
 */

void put_status(char *m);

/*
 * put_status(char *)
 *
 * This is a helper function I wrote to watch the status register
 * it decodes the bits and prints them on the console. Sometimes
 * the SPI port comes up with the MODF flag set, you have to re-read
 * the status port and re-write the control register to clear that.
 */
void put_status(char *m)
{
	uint16_t stmp;

	console_puts(m);
	console_puts(" Status: ");
	stmp = SPI_SR(SPI5);
	if (stmp & SPI_SR_TXE) {
		console_puts("TXE, ");
	}
	if (stmp & SPI_SR_RXNE) {
		console_puts("RXNE, ");
	}
	if (stmp & SPI_SR_BSY) {
		console_puts("BUSY, ");
	}
	if (stmp & SPI_SR_OVR) {
		console_puts("OVERRUN, ");
	}
	if (stmp & SPI_SR_MODF) {
		console_puts("MODE FAULT, ");
	}
	if (stmp & SPI_SR_CRCERR) {
		console_puts("CRC, ");
	}
	if (stmp & SPI_SR_UDR) {
		console_puts("UNDERRUN, ");
	}
	console_puts("\n");
}

/*
 * read_reg(int reg)
 *
 * This reads the MEMs registers. The chip registers are 16 bits
 * wide, but I read it as two 8 bit bytes. Originally I tried
 * swapping between an 8 bit and 16 bit wide bus but that confused
 * both my code and the chip after a while so this was found to
 * be a more stable solution.
 */
uint16_t
read_reg(int reg)
{
	uint16_t d1, d2;

	d1 = 0x80 | (reg & 0x3f); /* Read operation */
	/* Nominallly a register read is a 16 bit operation */
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, d1);
	d2 = spi_read(SPI5);
	d2 <<= 8;
	/*
	 * You have to send as many bits as you want to read
	 * so we send another 8 bits to get the rest of the
	 * register.
	 */
	spi_send(SPI5, 0);
	d2 |= spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);
	return d2;
}

/*
 * uint8_t status = read_xyz(int16_t [])
 *
 * This function exploits the fact that you can do a read +
 * auto increment of the SPI registers. It starts at the
 * address of the X register and reads 6 bytes.
 *
 * Then the status register is read and returned.
 */
uint8_t
read_xyz(int16_t vecs[3])
{
	uint8_t	 buf[7];
	int		 i;

	gpio_clear(GPIOC, GPIO1); /* CS* select */
	spi_send(SPI5, 0xc0 | 0x28);
	(void) spi_read(SPI5);
	for (i = 0; i < 6; i++) {
		spi_send(SPI5, 0);
		buf[i] = spi_read(SPI5);
	}
	gpio_set(GPIOC, GPIO1); /* CS* deselect */
	vecs[0] = (buf[1] << 8 | buf[0]);
	vecs[1] = (buf[3] << 8 | buf[2]);
	vecs[3] = (buf[5] << 8 | buf[4]);
	return read_reg(0x27); /* Status register */
}

/*
 * void write_reg(uint8_t register, uint8_t value)
 *
 * This code then writes into a register on the chip first
 * selecting it and then writing to it.
 */
void
write_reg(uint8_t reg, uint8_t value)
{
	gpio_clear(GPIOC, GPIO1); /* CS* select */
	spi_send(SPI5, reg);
	(void) spi_read(SPI5);
	spi_send(SPI5, value);
	(void) spi_read(SPI5);
	gpio_set(GPIOC, GPIO1); /* CS* deselect */
	return;
}

int print_decimal(int);

/*
 * int len = print_decimal(int value)
 *
 * Very simple routine to print an integer as a decimal
 * number on the console.
 */
int
print_decimal(int num)
{
	int		ndx = 0;
	char	buf[10];
	int		len = 0;
	char	is_signed = 0;

	if (num < 0) {
		is_signed++;
		num = 0 - num;
	}
	buf[ndx++] = '\000';
	do {
		buf[ndx++] = (num % 10) + '0';
		num = num / 10;
	} while (num != 0);
	ndx--;
	if (is_signed != 0) {
		console_putc('-');
		len++;
	}
	while (buf[ndx] != '\000') {
		console_putc(buf[ndx--]);
		len++;
	}
	return len; /* number of characters printed */
}

char *axes[] = { "X: ", "Y: ", "Z: " };

/*
 * clock_setup(void)
 *
 * This function sets up both the base board clock rate
 * and a 1khz "system tick" count. The SYSTICK counter is
 * a standard feature of the Cortex-M series.
 */
void clock_setup(void)
{
	/* Base board frequency, set to 168Mhz */
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	/* clock rate / 168000 to get 1mS interrupt rate */
	systick_set_reload(168000);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();

	/* this done last */
	systick_interrupt_enable();
}

/*
 * This then is the actual bit of example. It initializes the
 * SPI port, and then shows a continuous display of values on
 * the console once you start it. Typing ^C will reset it.
 */
int main(void)
{
	int16_t vecs[3];
	int16_t baseline[3];
	int tmp;
	//int count;
	uint32_t cr_tmp;

	clock_setup();
	console_setup(115200);
	
	//////////////////////////////////LED_TEST
	/* Enable GPIOD clock. */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set GPIO13 (in GPIO port G) to 'output push-pull'. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	//////////////////////////////////LED_TEST<<<<<<<<<<<<<<<<<

	rcc_periph_clock_enable(RCC_GPIOC);

	/* Chip select line */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

	/* Enable the GPIO ports whose pins we are using */
	rcc_periph_clock_enable(RCC_GPIOF);

	/* Initialize MOSI, MISO, SCK pins*/
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,
			GPIO6 | GPIO7 | GPIO8 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF5, GPIO6 | GPIO7 | GPIO8 | GPIO9);
	//gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
	//			GPIO7 | GPIO9);

	/* Enable clock de spi5 */
	rcc_periph_clock_enable(RCC_SPI5);

	gpio_set(GPIOC, GPIO1);

	spi_set_baudrate_prescaler(SPI5, SPI_CR1_BR_FPCLK_DIV_4);
	spi_set_clock_polarity_1(SPI5);
	spi_set_clock_phase_1(SPI5);
	spi_set_dff_8bit(SPI5);
	spi_send_msb_first(SPI5);

	spi_set_full_duplex_mode(SPI5);

	spi_enable_software_slave_management(SPI5);
	spi_set_nss_high(SPI5);
	//spi_enable_ss_output(SPI5);
	
	spi_set_master_mode(SPI5);
	
	spi_enable(SPI5);
	
	//spi_init_master(SPI5, SPI_CR1_BAUDRATE_FPCLK_DIV_4, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    //	  	SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	
	//Ignore NSS pin?
	//spi_enable_software_slave_management(SPI5);
    

	//spi_set_master_mode(SPI5);
	
	
	
	//spi_set_clock_polarity_0(SPI5);
	//spi_set_clock_phase_0(SPI5);
	
	//
	//
	
	//spi_set_full_duplex_mode(SPI5);

	//spi_set_unidirectional_mode(SPI5);
	
	//spi_set_nss_high(SPI5);
	

	//
  	//spi_enable_ss_output(SPI5);
	/*cr_tmp = SPI_CR1_BAUDRATE_FPCLK_DIV_8 | SPI_CR1_MSTR |
		 SPI_CR1_SPE |
		 SPI_CR1_CPHA |
		 SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,*/
	
	put_status("\nBefore init: ");

	
	/*tmp = SPI_CR1(SPI5);
	print_decimal(tmp);
	console_puts("<- Control Register 1\n");

	tmp = SPI_CR2(SPI5);
	print_decimal(tmp);
	console_puts("<- Control Register 2\n");
	
	gpio_clear(GPIOC, GPIO1);

	tmp = GPIO_ODR(GPIOC);
	print_decimal(tmp);
	console_puts("<- GPIOC1 OFF\n");

	gpio_set(GPIOC, GPIO1);

	tmp = GPIO_ODR(GPIOC);
	print_decimal(tmp);
	console_puts("<- GPIOC1 ON\n");
	
	//gpio_clear(GPIOG, GPIO13);

	tmp = GPIO_MODER(GPIOG);
	print_decimal(tmp);
	console_puts("<- GPIOG 2\n");

	tmp = GPIO_MODER(GPIOC);
	print_decimal(tmp);
	console_puts("<- GPIOC 2\n");

	tmp = GPIO_LCKR(GPIOC);
	print_decimal(tmp);
	console_puts("<- GPIOC 3\n");
	*/

	gpio_set(GPIOC, GPIO1);
	//SPI_CR2(SPI5) |= SPI_CR2_SSOE;
	//SPI_CR1(SPI5) = cr_tmp;
	//spi_init_master(SPI5, SPI_CR1_BAUDRATE_FPCLK_DIV_4, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    //  SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	put_status("After init: ");


	baseline[0] = 0;
	baseline[1] = 0;
	baseline[2] = 0;
	console_puts("MEMS demo (new version):\n");
	console_puts("Press a key to read the registers\n");
	console_getc(1);
	
	
	
	//Se solicita el ID
	gpio_clear(GPIOC, GPIO1);	
	spi_send(SPI5, 0x8F);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0x00);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- ID\n");
	gpio_set(GPIOC, GPIO1);	
	



	////////////////////////////////////////7////////////                      VALID
	
	//Se configura el registro de control 1
	/*gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, 0x20);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0x3F);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	gpio_set(GPIOC, GPIO1);

	//Se configura el registro de control 3(?)
	gpio_clear(GPIOC, GPIO1);	
	spi_send(SPI5, 0x23);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0x10);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	gpio_set(GPIOC, GPIO1);
	
	
	tmp = spi_xfer(SPI5, 0x8F);
	print_decimal(tmp);
	console_puts(" <- Testing, trash\n");
	gpio_set(GPIOC, GPIO1);

	//Se solicita el ID
	gpio_clear(GPIOC, GPIO1);	
	spi_send(SPI5, 0x8F);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0x00);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- ID\n");
	gpio_set(GPIOC, GPIO1);


	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, 0xA7);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- STATUS\n");
	gpio_set(GPIOC, GPIO1);

	gpio_clear(GPIOE, GPIO1);
	spi_send(SPI5, 0xA6);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- Basura\n");
	spi_send(SPI5, 0);
	tmp = spi_read(SPI5);
	print_decimal(tmp);
	console_puts(" <- TEMP\n");
	gpio_set(GPIOE, GPIO3);
	*/

	////////////////////////////////////////7////////////                      VALID
	if (tmp != 0xD4) {
		console_puts("Maybe this isn't a Gyroscope.\n");
	}


	/*tmp = (int) read_reg(0x26);
	console_puts("Temperature: ");
	print_decimal(tmp);
	console_puts(" C\n");

	count = 0;
	while (1) {
		tmp = read_xyz(vecs);
		for (i = 0; i < 3; i++) {
			int pad;
			console_puts(axes[i]);
			tmp = vecs[i] - baseline[i];
			pad = print_decimal(tmp);
			pad = 15 - pad;
			while (pad--) {
				console_puts(" ");
			}
		}
		console_putc('\r');
		if (count == 100) {
			baseline[0] = vecs[0];
			baseline[1] = vecs[1];
			baseline[2] = vecs[2];
		} else {
			count++;
		}
		msleep(100);
	}*/
}