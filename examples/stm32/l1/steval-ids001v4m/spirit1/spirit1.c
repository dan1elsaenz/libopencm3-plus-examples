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

#include <libopencm3-plus/utils/misc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdio.h>

#include "spirit1.h"

#define SP1_READ 0x01
#define SP1_WRITE 0x00
#define SP1_CMD 0x80

// TODO
// Switch state function that waits until state changed really happened

char *states[] = {"STANDBY", "SLEEP", "READY", "LOCK", "RX", "TX", "ERROR"};

void write_many(SpiritSPI dev, Data_write *list, int n) {
  for (int i = 0; i < n; i++) {
    printf("Entry: Reg: 0x%02x, data: 0x%02x\n", list[i].reg, list[i].data);
    sp1_write(dev, list[i].reg, &(list[i].data), 1);
  }
}

void read_buffer(SpiritSPI dev, unsigned char *buf, int count) {
  sp1_read(dev, SP1_FIFO, buf, count, false);
  buf[count] = '\0';
}

// set minimal mandatory IC settings:
void min_init(void) {
  // DEM_ORDER = 0
  // Set TSPLIT to 3.47ns
  /* Data_write transmit_conf_data[] = { */
  /*   { 0xa3, 0x35 }, // DEM_ORDER = 0 during ratio init */
  /*   { 0x9F, 0xA0 }, // TSPLIT */
  /*   { 0xA1, 0x25 }, // Set VCO current */
  /*   { 0xBC, 0x22 }, // During radio config */
  /* }; */
}

char *get_state_str(uint8_t state) {
  switch (state) {
  case SP1_ST_STANDBY:
    return (states[0]);
  case SP1_ST_SLEEP:
    return (states[1]);
  case SP1_ST_READY:
    return (states[2]);
  case SP1_ST_LOCK:
    return (states[3]);
  case SP1_ST_RX:
    return (states[4]);
  case SP1_ST_TX:
    return (states[5]);
  default:
    return (states[6]);
  }
}

void print_sp1_status(uint16_t status) {
  printf("MC_STATE: 0x%04x, STATE: 0x%02x\n", status, SP1_STATE(status));
  printf(get_state_str(SP1_STATE(status)));
  printf("\n");
}

uint16_t my_spi_xfer(uint32_t spi, uint16_t data) {
  spi_write(spi, data);
  /* Wait for read data ready. */
  while (!(SPI_SR(spi) & SPI_SR_RXNE))
    ;
  /* Wait for transmit finished. */
  while (!(SPI_SR(spi) & SPI_SR_TXE))
    ;
  return (SPI_DR(spi));
}

uint16_t sp1_write(SpiritSPI spi_conf, uint8_t reg_addr, uint8_t *wr_data,
                   uint8_t count) {
  uint16_t status = 0x00;
  gpio_clear(spi_conf.gpioport, spi_conf.spi_cs);
  status = (my_spi_xfer(spi_conf.spiport, SP1_WRITE) << 8);
  status |= my_spi_xfer(spi_conf.spiport, reg_addr);
  for (int i = 0; i < count; i++) {
    my_spi_xfer(spi_conf.spiport, *(wr_data + i));
  }
  gpio_set(spi_conf.gpioport, spi_conf.spi_cs);
  return (status);
}

uint16_t sp1_cmd(SpiritSPI spi_conf, uint8_t cmd) {
  uint16_t status = 0x00;
  gpio_clear(spi_conf.gpioport, spi_conf.spi_cs);
  status = (my_spi_xfer(spi_conf.spiport, SP1_CMD) << 8);
  status |= my_spi_xfer(spi_conf.spiport, cmd);
  gpio_set(spi_conf.gpioport, spi_conf.spi_cs);
  return (status);
}

uint16_t sp1_read(SpiritSPI spi_conf, uint8_t reg_addr, uint8_t *rd_data,
                  uint8_t count, bool inv_dir) {
  // Returns least significant Byte first (position 0 in array)
  uint16_t status = 0x00;
  gpio_clear(spi_conf.gpioport, spi_conf.spi_cs);
  status = (my_spi_xfer(spi_conf.spiport, SP1_READ) << 8);
  status |= my_spi_xfer(spi_conf.spiport, reg_addr);
  for (int i = 0; i < count; i++) {
    if (inv_dir) {
      *(rd_data + (count - 1 - i)) = my_spi_xfer(spi_conf.spiport, 0x00);
    } else {
      *(rd_data + i) = my_spi_xfer(spi_conf.spiport, 0x00);
    }
  }
  gpio_set(spi_conf.gpioport, spi_conf.spi_cs);
  return (status);
}

void sp1_spi_setup(SpiritSPI spi_conf) {
  // Setups spi for spirit1
  // gpios are MISO, MOSI and SCLK (or-d)
  // Before calling this function you have to:
  //- Turn the clocks on for the GPIO port and SPI unit
  //- Setup CS as output (gpio_mode_setup)
  //- Configure Alternative function number for SPI (MISO, MOSI, SCLK) pins
  //(gpio_set_af)
  //- Configure MISO, MOSI, SCLK pins as Alternate Function mode
  //(gpio_mode_setup)

  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
  spi_reset(spi_conf.spiport);

  /* Set up SPI in Master mode with:
   * Clock baud rate: 1/256 of peripheral clock frequency
   (125kHz)
   * Clock polarity: Idle LOW
   * Clock phase: Data valid on 1nd clock pulse
   * Data frame format: 8-bit
   * Frame format: MSB First
   */
  spi_init_master(spi_conf.spiport, SPI_CR1_BAUDRATE_FPCLK_DIV_64,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);

  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
  spi_enable_software_slave_management(spi_conf.spiport);
  spi_set_nss_high(spi_conf.spiport);

  /* Enable SPI1 periph. */
  spi_enable(spi_conf.spiport);
}
