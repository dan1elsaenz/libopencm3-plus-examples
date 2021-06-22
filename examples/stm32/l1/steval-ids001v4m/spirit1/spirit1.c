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
#include <libopencm3/stm32/spi.h>
#include <libopencm3-plus/utils/misc.h>
#include "spirit1.h"

#define SP1_READ 0x01
#define SP1_WRITE 0x00
#define SP1_CMD 0x80

//TODO

// Switch state function that waits until state changed really happened

const char* states[] = {
  "STANDBY",
  "SLEEP",
  "READY",
  "LOCK",
  "RX",
  "TX",
  "ERROR"
};

char* get_state_str(uint8_t state) {
  switch (state) {
  case SP1_ST_STANDBY :
    return(states[0]);
  case SP1_ST_SLEEP :
    return(states[1]);
  case SP1_ST_READY :
    return(states[2]);
  case SP1_ST_LOCK :
    return(states[3]);
  case SP1_ST_RX :
    return(states[4]);
  case SP1_ST_TX :
    return(states[5]);
  default:
    return(states[6]);
  }
}

void print_sp1_status(uint16_t status) {
  printf("MC_STATE: 0x%04x, STATE: 0x%02x\n", status, SP1_STATE(status));
  printf(get_state_str(SP1_STATE(status)));
  printf("\n");
}

uint16_t my_spi_xfer(uint32_t spi, uint16_t data)
{
  spi_write(spi, data);
  /* Wait for read data ready. */
  while (!(SPI_SR(spi) & SPI_SR_RXNE));
  /* Wait for transmit finished. */
  while (!(SPI_SR(spi) & SPI_SR_TXE));
  return(SPI_DR(spi));
}


uint16_t sp1_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t count,
                   uint32_t spiport, uint32_t gpioport, uint16_t gpios) {
  uint16_t status = 0x00;
  gpio_clear(gpioport, gpios);
  status = (my_spi_xfer(spiport, SP1_WRITE) << 8);
  status |= my_spi_xfer(spiport, reg_addr);
  for (int i=0; i<count; i++) {
     my_spi_xfer(spiport, *(wr_data+i));
  }
  gpio_set(gpioport, gpios);
  return(status);
}

uint16_t sp1_cmd(uint8_t cmd, uint32_t spiport,
                        uint32_t gpioport, uint16_t gpios) {
  uint16_t status = 0x00;
  gpio_clear(gpioport, gpios);
  status = (my_spi_xfer(spiport, SP1_CMD) << 8);
  status |= my_spi_xfer(spiport, cmd);
  gpio_set(gpioport, gpios);
  return(status);
}

uint16_t sp1_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t count,
                  uint32_t spiport, uint32_t gpioport, uint16_t gpios, bool inv_dir) {
  //Returns least significant Byte first (position 0 in array)
  uint16_t status = 0x00;
  gpio_clear(gpioport, gpios);
  status = (my_spi_xfer(spiport, SP1_READ) << 8);
  status |= my_spi_xfer(spiport, reg_addr);
  for (int i=0; i<count; i++) {
    if (inv_dir) {
      *(rd_data+(count-1-i)) = my_spi_xfer(spiport, 0x00);
    } else {
      *(rd_data+i) = my_spi_xfer(spiport, 0x00);
    }
  }
  gpio_set(gpioport, gpios);
  return(status);
}

void sp1_spi_setup(uint32_t spiport) {
  //Setups spi for spirit1
  //gpios are MISO, MOSI and SCLK (or-d)
  //Before calling this function you have to:
  //- Turn the clocks on for the GPIO port and SPI unit
  //- Setup CS as output (gpio_mode_setup)
  //- Configure Alternative function number for SPI (MISO, MOSI, SCLK) pins (gpio_set_af)
  //- Configure MISO, MOSI, SCLK pins as Alternate Function mode (gpio_mode_setup)

  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
  spi_reset(spiport);

  /* Set up SPI in Master mode with:
   * Clock baud rate: 1/256 of peripheral clock frequency
   (125kHz)
   * Clock polarity: Idle LOW
   * Clock phase: Data valid on 1nd clock pulse
   * Data frame format: 8-bit
   * Frame format: MSB First
   */
  spi_init_master(spiport, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
  spi_enable_software_slave_management(spiport);
  spi_set_nss_high(spiport);

  /* Enable SPI1 periph. */
  spi_enable(spiport);
}
