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
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include <errno.h>
#include <libopencm3-plus/newlib/syscall.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/steval-ids001v4m/leds.h>
#include <limits.h>
#include <stdbool.h>

#include "usart_spsgrf-868.h"
#include "steval-ids001v4m.h"
#include "spirit1.h"


#define USART_BAUDRATE 115200*4


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




static void usart_setup(void)
{
  /* Pin port A and USART2 clock */
	rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);

    /* Alternate function 7 for pins 2, 3 port A for USART2 */
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);

    gpio_set_output_options(GPIOA,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_40MHZ,
                            GPIO2 | GPIO3);


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

	rcc_periph_clock_enable(RCC_GPIOA);

	/* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO9);
    gpio_set(GPIOA, GPIO9);

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

/* void nanowait(int x) { */
/*   volatile int tmp; */
/*   for(int i=0; i<x; i++) { */
/*     __asm__("nop"); */
/*   } */
/* } */

int main(void)
{

  uint8_t rx_values[2];
  uint16_t status = 0x00;

  system_init();

  while (1) {
    status = spsgrf_read(0xF0, rx_values, 2);

    /* EEPROM test code */
    /* gpio_clear(GPIOA, GPIO9); */
    /* nanowait(10); */
    /* spi_xfer2(SPI2, 0b10000011); */
    /* status = spi_xfer2(SPI2, 0x00) << 8; */
    /* status |= spi_xfer2(SPI2, 0x00); */
    /* nanowait(10); */
    /* gpio_set(GPIOA, GPIO9); */

    usart_send_blocking(USART2, status >> 8);
    usart_send_blocking(USART2, status);
    usart_send_blocking(USART2, rx_values[1]);
    usart_send_blocking(USART2, rx_values[0]);

    //printf("Status:_0x%04x\n", status);
    //printf("RX_values:_0x%02x_0x%02x\n", rx_values[0], rx_values[1]);

    gpio_toggle(LRED);
    wait(2);
    gpio_toggle(LORANGE);
    wait(2);
  }

}
