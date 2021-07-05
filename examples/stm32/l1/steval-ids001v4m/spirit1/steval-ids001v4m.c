/*
 * Copyright (C) 2013 ARCOS-Lab Universidad de Costa Rica
 * Author: Federico Ruiz Ugalde <memeruiz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <libopencm3-plus/utils/misc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#include "spirit1.h"
#include "steval-ids001v4m.h"

#define SPSGRF_SDN_PORT GPIOC
#define SPSGRF_SDN GPIO13
#define SPSGRF_SPI_PORT GPIOB
#define SPSGRF_SPI SPI2
#define SPSGRF_CS_PORT GPIOB
#define SPSGRF_CS GPIO12

SpiritSPI spsgrf_spi = {
  .spiport = SPI2,
  .gpioport = GPIOB,
  .spi_cs = GPIO12,
  .sdnport = GPIOC,
  .sdnpin = GPIO13,
  .fxo = 50000000,
  .fbase_cmd = 868000000,
  .fbase_rd = 868000000,
  .ch_space_steps = 1, // steps of fxo/(2^15)
  .channel = 1,
  .tsplit = 1,    // 0: 1.75ns, 1: 3.47ns
  .tx_power = { // 11.0dBm -> -34dBm (in half dB decrements)
    // Use -35dBm or smaller to turn output off
    10, 10, 10, 10, 10, 10, 10, 10
  },

  .tx_out_capis = SP1_PA_POWER0_CWC_2pF4,
  .tx_ramp = false,
  .tx_ramp_max_index = 7, // 0->7
  .tx_ramp_step = 0,      // 1/8 bit period
  .datarate_cmd = 1000000,
  .datarate_rd = 1000000,
  .mod_type= SP1_MOD0_MOD_TYPE_GFSK,
  .chflt_m = 1,
  .chflt_e = 3, // 94.23kHz Table 32/33
  .afc = true,
  .afc_freeze_on_sync = true,
  .afc_mode = AFC_SLICER,
  .agc = true,
  .agc_th_high = 0x6, // 0 -> 0xF
  .agc_th_low = 0x2, // 0 -> 0xF
  .ant_sel_cs_blanking = true,
  .pckt_whitening = true,
  .pckt_crc_mode = PCKT_CRC_0x07,
  .pckt_preamble_len = 0x7,
  .pckt_len = 0x0012, // not sure if this is set automatically somewhere else
  .pckt_flt_options = RX_TIMEOUT_AND_OR_SELECT | CRC_CHECK,
  .protocol_nmax_retx = 0x0,
  .protocol_nack_tx = true,
  .protocol_auto_ack = false,
  .protocol_pers_rx = false,
  .protocol_pers_tx = false,
  .protocol_ldc_reload_on_sync = false,
  .protocol_piggybacking = false,
  .protocol_seed_reload = false,
  .protocol_csma_on = false,
  .protocol_csma_pers_on = false,
  .protocol_auto_pckt_flt = true,
  .protocol_cs_timeout_mask  = false,
  .protocol_sqi_timeout_mask = true,
  .protocol_pqi_timeout_mask = false,
  .protocol_tx_seq_num_reload = 0x0,
  .protocol_rco_calib  = false,
  .protocol_vco_calib = false,
  .protocol_ldc_mode = false,
  .pckt_frmt = PCKT_FRMT_Basic,
  .pckt_addr_len = 0x0,
  .pckt_rx_mode = PCKT_RX_MODE_Normal,
  .partnum=0x00,
  .version=0x00,
};

/* uint16_t spsgrf_write(uint8_t reg_addr, uint8_t *wr_data, uint8_t
 * count) { */
/*   //Write to spirit1 register(s). Multiple write if count>1 */
/*   //Returns Spirit1 status word */

/*   return(sp1_write(reg_addr, wr_data, count, spsgrf_spi)); */
/* } */

/* uint16_t spsgrf_cmd(uint8_t cmd) { */
/*   //Command spirit1 */
/*   //Returns Spirit1 status word */

/*   return(sp1_cmd(cmd, spsgrf_spi)); */
/* } */

/* uint16_t spsgrf_read(uint8_t reg_addr, uint8_t *rd_data, uint8_t
 * count, bool inv_dir) { */
/*   //Read from spirit1 register(s). Multiple read if count>1 */
/*   //Returns Spirit1 status word */

/*   return(sp1_read(reg_addr, rd_data, count, spsgrf_spi, inv_dir));
 */
/* } */

void spi_setup(void) {
  /* Pin port B and SPI2 clock */
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_SPI2);

  /* Alternate Function 5 for SPI on port B */
  gpio_set_af(SPSGRF_SPI_PORT, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
  /* AF for pins 13(SCK), 14(MISO), 15(MOSI) on port B */
  gpio_mode_setup(SPSGRF_SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO13 | GPIO14 | GPIO15);

  /* Errata: section 2.4.6, increase pin speed or decrease APB clock
   * speed */
  /* This is necessary for SPI and I2S */
  gpio_set_output_options(SPSGRF_SPI_PORT, GPIO_OTYPE_PP,
                          GPIO_OSPEED_40MHZ,
                          GPIO13 | GPIO14 | GPIO15);

  /* Output mode for pin 12 (spsgrf-868 SPI_CS connection) */
  gpio_mode_setup(SPSGRF_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  spsgrf_spi.spi_cs);

  gpio_set(spsgrf_spi.gpioport, spsgrf_spi.spi_cs);

  sp1_spi_setup(spsgrf_spi);
}

void spsgrf868_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOC);

  gpio_mode_setup(spsgrf_spi.sdnport, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, spsgrf_spi.sdnpin);
  gpio_set(spsgrf_spi.sdnport, spsgrf_spi.sdnpin);
  wait(100);
  gpio_clear(spsgrf_spi.sdnport, spsgrf_spi.sdnpin);

  wait(100);
}

void eeprom_init(void) {
  rcc_periph_clock_enable(RCC_GPIOA);

  /* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);
  gpio_set(GPIOA, GPIO9);
}
