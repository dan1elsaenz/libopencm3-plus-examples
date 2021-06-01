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

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3-plus/utils/misc.h>
#include "steval-ids001v4m.h"
#include "spirit1.h"

#define SPSGRF_SDN_PORT GPIOC
#define SPSGRF_SDN GPIO13
#define SPSGRF_SPI_PORT GPIOB
#define SPSGRF_SPI SPI2
#define SPSGRF_CS_PORT GPIOB
#define SPSGRF_CS GPIO12

uint16_t spsgrf_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t count) {
  //Write to spirit1 register(s). Multiple write if count>1
  //Returns Spirit1 status word

  return(sp1_write(reg_addr, wr_data, count, SPSGRF_SPI, SPSGRF_CS_PORT, SPSGRF_CS));
}

uint16_t spsgrf_cmd(uint8_t cmd) {
  //Command spirit1
  //Returns Spirit1 status word

  return(sp1_cmd(cmd, SPSGRF_SPI, SPSGRF_CS_PORT, SPSGRF_CS));
}

uint16_t spsgrf_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t count) {
  //Read from spirit1 register(s). Multiple read if count>1
  //Returns Spirit1 status word

  return(sp1_read(reg_addr, rd_data, count, SPSGRF_SPI, SPSGRF_CS_PORT, SPSGRF_CS));
}

void spi_setup(void) {
  /* Pin port B and SPI2 clock */
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_SPI2);

  /* Alternate Function 5 for SPI on port B */
  gpio_set_af(SPSGRF_SPI_PORT, GPIO_AF5,
              GPIO13 |
              GPIO14 |
              GPIO15 );
  /* AF for pins 13(SCK), 14(MISO), 15(MOSI) on port B */
  gpio_mode_setup(SPSGRF_SPI_PORT, GPIO_MODE_AF,
                  GPIO_PUPD_NONE,
                  GPIO13 |
                  GPIO14 |
                  GPIO15 );

  /* Errata: section 2.4.6, increase pin speed or decrease APB clock speed */
  /* This is necessary for SPI and I2S */
  gpio_set_output_options(SPSGRF_SPI_PORT,
                          GPIO_OTYPE_PP,
                          GPIO_OSPEED_40MHZ,
                          GPIO13 | GPIO14 | GPIO15);

  /* Output mode for pin 12 (spsgrf-868 SPI_CS connection) */
  gpio_mode_setup(SPSGRF_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
          SPSGRF_CS);

  gpio_set(SPSGRF_CS_PORT, SPSGRF_CS);

  sp1_spi_setup(SPSGRF_SPI);
}

void spsgrf868_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOC);

  gpio_mode_setup(SPSGRF_SDN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPSGRF_SDN);
  gpio_set(SPSGRF_SDN_PORT, SPSGRF_SDN);
  wait(100);
  gpio_clear(SPSGRF_SDN_PORT, SPSGRF_SDN);

  wait(100);
}
