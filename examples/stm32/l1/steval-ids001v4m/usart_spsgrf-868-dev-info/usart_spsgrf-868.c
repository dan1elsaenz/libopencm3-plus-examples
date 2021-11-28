/*
 * Copyright (C) 2013 ARCOS-Lab Universidad de Costa Rica
 * Author: Federico Ruiz Ugalde <memeruiz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <stdio.h>
#include <errno.h>
#include <libopencm3-plus/newlib/syscall.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/steval-ids001v4m/leds.h>
#include <limits.h>
#include <stdbool.h>

#include "usart_spsgrf-868.h"


#define SPSGRF_CS_PORT GPIOB
#define SPSGRF_CS GPIO12
#define SPSGRF_SDN_PORT GPIOC
#define SPSGRF_SDN GPIO13
#define USART_BAUDRATE 115200

#define SPI_READ 0x0100
#define SPI_WRITE 0x0000




const struct rcc_clock_scale rcc_clock_config_32mhz = {
  /* 32MHz PLL from HSE */
  .pll_source = RCC_CFGR_PLLSRC_HSE_CLK,
  .pll_mul = RCC_CFGR_PLLMUL_MUL12,
  .pll_div = RCC_CFGR_PLLDIV_DIV3,
  .hpre = RCC_CFGR_HPRE_NODIV,
  .ppre1 = RCC_CFGR_PPRE_NODIV,
  .ppre2 = RCC_CFGR_PPRE_NODIV,
  .voltage_scale = PWR_SCALE1,
  .flash_waitstates = 1,
  .ahb_frequency	= 32000000,
  .apb1_frequency = 32000000,
  .apb2_frequency = 32000000,
};


static void spi_setup(void) {

  /* Pin port B and SPI2 clock */
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_SPI2);

  /* Alternate Function 5 for SPI on port B */
  gpio_set_af(GPIOB, GPIO_AF5,
              GPIO13 |
              GPIO14 |
              GPIO15 );
  /* AF for pins 13(SCK), 14(MISO), 15(MOSI) on port B */
  gpio_mode_setup(GPIOB, GPIO_MODE_AF,
                  GPIO_PUPD_NONE,
                  GPIO13 |
                  GPIO14 |
                  GPIO15 );
  /* Output mode for pin 12 (spsgrf-868 SPI_CS connection) */
  gpio_mode_setup(SPSGRF_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
          SPSGRF_CS);

  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
  spi_reset(SPI2);

  /* Set up SPI in Master mode with:
   * Clock baud rate: 1/256 of peripheral clock frequency
   (125kHz)
   * Clock polarity: Idle LOW
   * Clock phase: Data valid on 1nd clock pulse
   * Data frame format: 16-bit
   * Frame format: MSB First
   */
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_64,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_16BIT,
                  SPI_CR1_MSBFIRST);

  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
  spi_enable_software_slave_management(SPI2);
  spi_set_nss_high(SPI2);

  /* Enable SPI1 periph. */
  spi_enable(SPI2);
}


static void usart_setup(void)
{
  /* Pin port A and USART2 clock */
	rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);

    /* Alternate function 7 for pins 2, 3 port A for USART2 */
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);

	/* Setup UART parameters. */
	usart_set_baudrate(USART2, USART_BAUDRATE);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}


static void leds_init(void) {
	/* GPIOB for LEDs */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
	gpio_mode_setup(LEDS_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO0 | GPIO1);

}

static void spsgrf868_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOC);

  gpio_set(SPSGRF_SDN_PORT, SPSGRF_SDN);
	gpio_mode_setup(SPSGRF_SDN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPSGRF_SDN);

  gpio_clear(SPSGRF_SDN_PORT, SPSGRF_SDN);
    wait(10);
}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART2, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

static void system_init(void) {
  /* CPU/uC general setup */
  rcc_clock_setup_pll(&rcc_clock_config_32mhz);

  /* uC + steval-ids001v4m setup */
  spi_setup();
  usart_setup();

  /* steval-ids001v4m specific setup */
  leds_init();
  spsgrf868_setup();

}

int main(void)
{

  uint16_t rx_value = 0x0;

  system_init();

  while (1) {
    /* LED on/off */
    usart_send_blocking(USART2, 'a');


    gpio_clear(SPSGRF_CS_PORT, SPSGRF_CS);
    spi_write(SPI2, (SPI_READ | 0xF1));
    rx_value = spi_read(SPI2);
    usart_send_blocking(USART2, rx_value >> 8);
    usart_send_blocking(USART2, (0x00FF & rx_value));
    spi_write(SPI2, 0x0000);
    rx_value=0;
    rx_value = spi_read(SPI2);
    usart_send_blocking(USART2, rx_value >> 8);
    usart_send_blocking(USART2, (0x00FF & rx_value));
    usart_send_blocking(USART2, 0x30);
    gpio_set(SPSGRF_CS_PORT, SPSGRF_CS);

    printf("A\n");

    gpio_toggle(LRED);
    wait(10);
    gpio_toggle(LORANGE);
    wait(10);
  }

}
