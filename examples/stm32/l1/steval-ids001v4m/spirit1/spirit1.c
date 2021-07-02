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
#include <stdlib.h>

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
double get_fclk(SpiritSPI dev) {
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

uint32_t _get_synt_from_reg(SpiritSPI dev) {
  uint32_t synt;
  uint8_t data;

  sp1_read(dev, SP1_SYNT0, &data, 1, true);
  data &= ~SP1_SYNT0_BS;
  synt = data;
  sp1_read(dev, SP1_SYNT1, &data, 1, true);
  synt |= (((uint32_t)data) << 8);
  sp1_read(dev, SP1_SYNT2, &data, 1, true);
  synt |= (((uint32_t)data) << (8 * 2));
  sp1_read(dev, SP1_SYNT3, &data, 1, true);
  data &= ~SP1_SYNT3_WCP;
  synt |= (((uint32_t)data) << (8 * 3));
  synt >>= SP1_SYNT0_SYNT4_0;
  return (synt);
}

void _update_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield,
                      uint8_t value) {
  uint8_t data;
  uint8_t i;
  sp1_read(dev, reg, &data, 1, true);
  //  printf("%x\n", data);
  // move new value to bitfield position
  for (i = bitfield; (i & 0x01) == 0; i >>= 1) {
    value <<= 1;
  }
  //  printf("%x\n", value);
  // clean bitfield old data
  data &= ~bitfield;
  //  printf("%x\n", data);
  // set new value on bitfield
  data |= value;
  //  printf("%x\n", data);
  sp1_write(dev, reg, &data, 1);
}

uint8_t _get_bitfield(SpiritSPI dev, uint8_t reg, uint8_t bitfield) {
  uint8_t data;
  uint8_t i;
  sp1_read(dev, reg, &data, 1, true);
  data &= bitfield;
  for (i = bitfield; (i & 0x01) == 0; i >>= 1) {
    data >>= 1;
  }
  return (data);
}

uint8_t _get_D(SpiritSPI dev) {
  uint8_t D;
  switch (_get_bitfield(dev, SP1_SYNTH_CONFIG1,
                        SP1_SYNTH_CONFIG1_REFDIV)) {
  case 0:
    D = 1;
    break;
  case 1:
    D = 2;
    break;
  default:
    abort();
  }
  return (D);
}

uint8_t _get_B(SpiritSPI dev) {
  uint8_t data;
  uint8_t B;
  sp1_read(dev, SP1_SYNT0, &data, 1, true);
  switch (data & SP1_SYNT0_BS) {
  case 1: // 6
    B = 6;
    break;
  case 3: // 12
    B = 12;
    break;
  case 4: // 16
    B = 16;
    break;
  case 5: // 32
    B = 32;
    break;
  default:
    abort();
  }
  return (B);
}

double get_fbase(SpiritSPI dev) {
  uint32_t synt;
  uint8_t B;
  uint8_t D;
  double fbase;
  synt = _get_synt_from_reg(dev);
  B = _get_B(dev);
  D = _get_D(dev);
  fbase = (dev.fxo / ((B * D) / 2.0)) * synt / pow(2, 18);
  return (fbase);
}

int16_t _get_foffset(SpiritSPI dev) {
  uint8_t data;
  uint16_t fc_offset;
  sp1_read(dev, SP1_FC_OFFSET0, &data, 1, true);
  fc_offset = data;
  sp1_read(dev, SP1_FC_OFFSET1, &data, 1, true);
  // 0x0F for masking out the reserved bits
  fc_offset |= (((uint16_t)(data & 0x0F)) << 8);
  // Extending the bit sign of the 2s complement 12bit number
  if ((fc_offset & (1 << (8 + 3))) > 0) {
    fc_offset |= 0xF000;
  }
  return ((uint16_t)fc_offset);
}

uint8_t _get_channel(SpiritSPI dev) {
  uint8_t data;
  sp1_read(dev, SP1_CHNUM, &data, 1, true);
  return (data);
}

double _get_channel_spacing(SpiritSPI dev) {
  uint8_t data;
  sp1_read(dev, SP1_CHSPACE, &data, 1, true);
  return (((double)data) * dev.fxo / pow(2.0, 15));
}

double get_fchannel(SpiritSPI dev) {
  printf("%f %f %f %f\n", get_fbase(dev), ((double)_get_foffset(dev)),
         ((double)_get_channel(dev)), _get_channel_spacing(dev));
  return (get_fbase(dev) + ((double)_get_foffset(dev)) +
          ((double)_get_channel(dev)) * _get_channel_spacing(dev));
}

void set_channel(SpiritSPI dev) {
  sp1_write(dev, SP1_CHNUM, &(dev.channel), 1);
}

void set_ch_space_steps(SpiritSPI dev, uint8_t steps) {
  printf("Channel spacing: %.2fHz\n", steps * dev.fxo / pow(2, 15));
  sp1_write(dev, SP1_CHSPACE, &steps, 1);
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

double set_fbase(SpiritSPI *dev) {
  // Sets Synt value and BS (Band select)
  // Returns the real fbase set value (not exactly the same)
  int D;
  int B;
  uint8_t BS;
  double synt;
  uint32_t isynt;

  if ((dev->fbase_cmd > 779000000) && (dev->fbase_cmd < 956000000)) {
    BS = 1;
    B = 6;
  } else {
    if ((dev->fbase_cmd > 387000000) &&
        (dev->fbase_cmd < 470000000)) {
      BS = 3;
      B = 12;
    } else {
      if ((dev->fbase_cmd > 300000000) &&
          (dev->fbase_cmd < 348000000)) {
        BS = 4;
        B = 16;
      } else {
        if ((dev->fbase_cmd > 168000000) &&
            (dev->fbase_cmd < 170000000)) {
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
  sp1_write(*dev, SP1_SYNT0, &BS, 1);
  for (D = 2; D >= 1; D--) {
    synt = roundf((dev->fbase_cmd / dev->fxo) * pow(2, 18) * B * D /
                  2.0);
    isynt = synt;
    printf("For D: %d, synt: %f, %x\n", D, synt, isynt);
    if (isynt < pow(2, 25)) {
      set_synth_refdiv(*dev, D);
      set_synt_reg(*dev, isynt);
      dev->fbase_rd = get_fbase(*dev);
      return (dev->fbase_rd);
    }
  }
  abort();
}

double calc_if_ana(SpiritSPI dev) {
  // Eq 8 (if_offset_ana)
  return (roundf((SP1_FIF / dev.fxo) * 3.0 * pow(2, 12) - 64));
}

double calc_if_dig(SpiritSPI dev) {
  // Eq 9 (if_offset_dig)
  return (roundf((SP1_FIF / get_fclk(dev)) * 3.0 * pow(2, 12) - 64));
}

void set_tsplit(SpiritSPI dev) {
  _update_bitfield(dev, SP1_SYNTH_CONFIG0,
                   SP1_SYNTH_CONFIG0_SEL_TSPLIT, dev.tsplit);
}

void tx_ramp(SpiritSPI dev, bool enable) {
  if (enable) {
    _update_bitfield(dev, SP1_PA_POWER0, SP1_PA_POWER0_RAMP_ENABLE,
                     0x1);
  } else {
    _update_bitfield(dev, SP1_PA_POWER0, SP1_PA_POWER0_RAMP_ENABLE,
                     0x0);
  }
}

void set_tx_ramp_max_index(SpiritSPI dev) {
  assert(dev.tx_ramp_max_index <= 7);
  _update_bitfield(dev, SP1_PA_POWER0, SP1_PA_POWER0_LEVEL_MAX_INDEX,
                   dev.tx_ramp_max_index);
}

void set_tx_ramp_step_width(SpiritSPI dev) {
  assert(dev.tx_ramp_step <= 3);
  _update_bitfield(dev, SP1_PA_POWER0, SP1_PA_POWER0_RAMP_STEP_WIDTH,
                   dev.tx_ramp_step);
}

float get_tx_power(SpiritSPI dev, uint8_t slot) {
  assert(slot <= 7);
  uint8_t data;
  sp1_read(dev, SP1_PA_POWER0 - slot, &data, 1, true);
  return (11.0 - ((float)data - 1) / 2);
}

void set_tx_power(SpiritSPI dev, uint8_t slot) {
  // slot starts with 0 pointing to PA_POWER1 and ascending
  assert(slot <= 7);
  uint8_t data;
  data = 2 * (11.0 - dev.tx_power[slot]) + 1;
  sp1_write(dev, SP1_PA_POWER0 - slot, &data, 1);
}

void set_tx_out_capis(SpiritSPI dev) {
  assert(dev.tx_out_capis <= 3);
  _update_bitfield(dev, SP1_PA_POWER0, SP1_PA_POWER0_CWC,
                   dev.tx_out_capis);
}

void set_datarate(SpiritSPI *dev) {
  double ef;
  double mf;
  uint8_t e;
  uint8_t m;
  double tmp;
  tmp = (dev->datarate_cmd * pow(2, 28) / get_fclk(*dev));
  // Finding E
  // Using max M=255, this gives a minimal E:
  ef = ceil(log2(tmp / (256 + 255)));
  assert(ef <= 15);
  mf = round(tmp / pow(2, ef)) - 256;
  assert(mf < 256);
  e = (uint8_t)ef;
  m = (uint8_t)mf;
  printf("M: %f 0x%02X, E: %f 0x%X\n", mf, m, ef, e);
  sp1_write(*dev, SP1_MOD1, &m, 1);
  _update_bitfield(*dev, SP1_MOD0, SP1_MOD0_DATARATE_E, e);
  dev->datarate_rd = get_datarate(*dev);
}

double get_datarate(SpiritSPI dev) {
  uint8_t data;
  uint8_t datarate_m;
  uint8_t datarate_e;
  // read datarate_m
  sp1_read(dev, SP1_MOD1, &datarate_m, 1, true);
  // read datarate_e
  datarate_e = _get_bitfield(dev, SP1_MOD0, SP1_MOD0_DATARATE_E);
  // Eq 11
  return (get_fclk(dev) * (256 + ((double)datarate_m)) *
          pow(2.0, ((double)datarate_e)) / pow(2, 28));
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
