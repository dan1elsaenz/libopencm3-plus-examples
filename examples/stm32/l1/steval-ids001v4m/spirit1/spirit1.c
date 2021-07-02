/*
 * Copyright (C) 2013 ARCOS-Lab Universidad de Costa Rica
 * Author: Federico Ruiz Ugalde <memeruiz@gmail.com>
 *
 * This program is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <math.h>
#include <stdio.h>

#include "spirit1.h"

#define SP1_READ 0x01
#define SP1_WRITE 0x00
#define SP1_CMD 0x80
#define SP1_FIF 480000

// TODO
// Switch state function that waits until state changed
// really happened

/* clang-format off */
char *states[] = {
  "STANDBY",
  "SLEEP",
  "READY",
  "LOCK",
  "RX",
  "TX",
  "ERROR"
};
/* clang-format on */

void write_many(SpiritSPI dev, Data_write *list, int n) {
  for (int i = 0; i < n; i++) {
    printf("Entry: Reg: 0x%02x, data: 0x%02x\n", list[i].reg,
           list[i].data);
    sp1_write(dev, list[i].reg, &(list[i].data), 1);
  }
}

void read_buffer(SpiritSPI dev, unsigned char *buf, int count) {
  sp1_read(dev, SP1_FIFO, buf, count, false);
  buf[count] = '\0';
}

Data_write min_init_data[] = {
  { 0xBC, 0x22 }, // During radio config
  { 0xa3, 0x35 }, // DEM_ORDER = 0 during ratio init
  { 0xA1, 0x25 }, // Set VCO current
};

// set minimal mandatory IC settings:
void min_init(SpiritSPI dev) {
  write_many(dev, min_init_data,
             sizeof(min_init_data) / sizeof(Data_write));
}

void change_to_state(SpiritSPI dev, int state_cmd, int state_result) {
  printf("Changing to %s\n", get_state_str(state_result));
  sp1_cmd(dev, state_cmd);
  while (SP1_STATE(get_mc_state(dev)) != state_result) {
  }
  printf("Now in %s\n", get_state_str(state_result));
}

// Get Digital clock freq
float get_fclk(SpiritSPI dev) {
  uint8_t data;
  sp1_read(dev, SP1_XO_RCO_TEST, &data, 1, true);
  if ((data & SP1_XO_RCO_TEST_PD_CLKDIV) == 0) {
    return (dev.fxo / 2.);
  } else {
    return (dev.fxo);
  }
}

#define FXO_50M_H 52000000
#define FXO_50M_L 48000000
#define FXO_25M_H 26000000
#define FXO_25M_L 24000000

void set_clkdiv(SpiritSPI dev) {
  // If fxo 48-52MHz, enable divider
  // If fxo 24-26MHz, disable divider
  // Must go to standby mode before changing
  uint8_t data;
  change_to_state(dev, SP1_CMD_STANDBY, SP1_ST_STANDBY);
  sp1_read(dev, SP1_XO_RCO_TEST, &data, 1, true);
  if ((dev.fxo > FXO_50M_L) && (dev.fxo < FXO_50M_H)) {
    // divide by 2
    printf("Freq around 50MHz: dividing by 2\n");
    data &= ~SP1_XO_RCO_TEST_PD_CLKDIV; // enable divider
    sp1_write(dev, SP1_XO_RCO_TEST, &data, 1);
  } else {
    if ((dev.fxo > FXO_25M_L) && (dev.fxo < FXO_25M_H)) {
      // divide by 1
      printf("Freq around 25MHz: dividing by 1\n");
      data |= SP1_XO_RCO_TEST_PD_CLKDIV; // disable divider
      sp1_write(dev, SP1_XO_RCO_TEST, &data, 1);
    } else {
      printf("Invalid FXO frequency: %f\n", dev.fxo);
    }
  }
  change_to_state(dev, SP1_CMD_READY, SP1_ST_READY);
}

void set_synth_refdiv(SpiritSPI dev, int D) {
  uint8_t data;
  sp1_read(dev, SP1_SYNTH_CONFIG1, &data, 1, true);
  switch (D) {
  case 2:
    data = SP1_SYNTH_CONFIG1_REFDIV |
           (data & ~(SP1_SYNTH_CONFIG1_REFDIV));
    break;
  case 1:
    data = (data & ~(SP1_SYNTH_CONFIG1_REFDIV));
    break;
  default:
    goto set_synth_refdiv_fail;
  }
  sp1_write(dev, SP1_SYNTH_CONFIG1, &data, 1);
  return;
set_synth_refdiv_fail:
  assert((D != 1) & (D != 2));
}

void set_synt_reg(SpiritSPI dev, uint32_t synt) {
  uint8_t data;
  uint8_t tmp;
  // aligning synt with SYNT0 start bit
  synt = (synt << SP1_SYNT0_SYNT4_0);
  // synt0
  tmp = (uint8_t)(synt & ((uint8_t)(0xFF << SP1_SYNT0_SYNT4_0)));
  sp1_read(dev, SP1_SYNT0, &data, 1, true);
  data = tmp | ((~((uint8_t)(0xFF << SP1_SYNT0_SYNT4_0))) & data);
  sp1_write(dev, SP1_SYNT0, &data, 1);
  // synt1
  data = (uint8_t)((synt >> 8) & 0xFF);
  sp1_write(dev, SP1_SYNT1, &data, 1);
  // synt2
  data = (uint8_t)((synt >> 8 * 2) & 0xFF);
  sp1_write(dev, SP1_SYNT2, &data, 1);
  // synt3
  sp1_read(dev, SP1_SYNT0, &data, 1, true);
  data = (SP1_SYNT3_WCP & data) |
         ((uint8_t)((synt >> 8 * 3) & ~SP1_SYNT3_WCP));
  sp1_write(dev, SP1_SYNT3, &data, 1);
}

float set_synt(SpiritSPI dev) {
  // Sets Synt value and BS (Band select)
  int D;
  int B;
  uint8_t BS;
  float synt;
  uint32_t isynt;

  if ((dev.fbase > 779000000) && (dev.fbase < 956000000)) {
    BS = 1;
    B = 6;
  } else {
    if ((dev.fbase > 387000000) && (dev.fbase < 470000000)) {
      BS = 3;
      B = 12;
    } else {
      if ((dev.fbase > 300000000) && (dev.fbase < 348000000)) {
        BS = 4;
        B = 16;
      } else {
        if ((dev.fbase > 168000000) && (dev.fbase < 170000000)) {
          BS = 5;
          B = 32;
        } else {
          printf("Invalid base frequency\n");
          return (0);
        }
      }
    }
  }
  printf("B: %d\n", B);
  sp1_write(dev, SP1_SYNT0, &BS, 1);
  // TODO: D is selected by Synth_config[1].REFDIV
  for (D = 2; D >= 1; D--) {
    synt = roundf((dev.fbase / dev.fxo) * pow(2, 18) * B * D / 2.0);
    isynt = synt;
    printf("For D: %d, synt: %f, %x\n", D, synt, isynt);
    if (isynt < pow(2, 26)) {
      set_synth_refdiv(dev, D);
      set_synt_reg(dev, isynt);
      return (isynt);
    }
  }
  // If synt is bigger than 26bit, then use D=2
  return (synt);
}

float calc_if_ana(SpiritSPI dev) {
  // Eq 8 (if_offset_ana)
  return (roundf((SP1_FIF / dev.fxo) * 3.0 * pow(2, 12) - 64));
}

float calc_if_dig(SpiritSPI dev) {
  // Eq 9 (if_offset_dig)
  return (roundf((SP1_FIF / get_fclk(dev)) * 3.0 * pow(2, 12) - 64));
}

uint16_t get_mc_state(SpiritSPI dev) {
  uint8_t rx_value[2];
  uint16_t state;
  sp1_read(dev, SP1_MC_STATE, rx_value, 2, true);
  state = rx_value[0] | (rx_value[1] << 8);
  print_sp1_status(state);
  return (state);
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
  printf("MC_STATE: 0x%04x, STATE: 0x%02x\n", status,
         SP1_STATE(status));
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

uint16_t sp1_write(SpiritSPI spi_conf, uint8_t reg_addr,
                   uint8_t *wr_data, uint8_t count) {
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

uint16_t sp1_read(SpiritSPI spi_conf, uint8_t reg_addr,
                  uint8_t *rd_data, uint8_t count, bool inv_dir) {
  // Returns least significant Byte first (position 0 in array)
  uint16_t status = 0x00;
  gpio_clear(spi_conf.gpioport, spi_conf.spi_cs);
  status = (my_spi_xfer(spi_conf.spiport, SP1_READ) << 8);
  status |= my_spi_xfer(spi_conf.spiport, reg_addr);
  for (int i = 0; i < count; i++) {
    if (inv_dir) {
      *(rd_data + (count - 1 - i)) =
          my_spi_xfer(spi_conf.spiport, 0x00);
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
  //- Configure Alternative function number for SPI (MISO, MOSI,
  // SCLK)
  // pins (gpio_set_af)
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
   * Setting nss high is very important, even if we are controlling
   * the GPIO ourselves this bit needs to be at least set to 1,
   * otherwise the spi peripheral will not send any data out.
   */
  spi_enable_software_slave_management(spi_conf.spiport);
  spi_set_nss_high(spi_conf.spiport);

  /* Enable SPI1 periph. */
  spi_enable(spi_conf.spiport);
}
