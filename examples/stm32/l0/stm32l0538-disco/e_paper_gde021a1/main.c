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
#include <libopencm3/stm32/timer.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "main.h"
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32l0538-disco/leds.h>
#include <libopencm3-plus/stm32l0538-disco/stm32l0538-disco.h>
#include <limits.h>
#include <stdbool.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3-plus/hw-accesories/gde021a1.h>


#define LED_GREEN_PIN GPIO4
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOA


void rcc_init(void) {

  rcc_osc_off(RCC_PLL);
  while (rcc_is_osc_ready(RCC_PLL)) {};


  /* PLL from HSI16, just to exercise that code */
  struct rcc_clock_scale myclock = {
    .ahb_frequency = 32e6,
    .apb1_frequency = 32e6,
    .apb2_frequency = 32e6,
    .flash_waitstates = 1,
    .pll_source = RCC_CFGR_PLLSRC_HSI16_CLK, /* not even sure there's hse on l053 disco */
    /* .msi_range  doesn't matter */
    .pll_mul = RCC_CFGR_PLLMUL_MUL4,
    .pll_div = RCC_CFGR_PLLDIV_DIV2,
    .hpre = RCC_CFGR_HPRE_NODIV,
    .ppre1 = RCC_CFGR_PPRE1_NODIV,
    .ppre2 = RCC_CFGR_PPRE2_NODIV,
  };
  rcc_clock_setup_pll(&myclock);

  /* HSI48 needs the vrefint turned on */
  rcc_periph_clock_enable(RCC_SYSCFG);
  SYSCFG_CFGR3 |= SYSCFG_CFGR3_ENREF_HSI48 | SYSCFG_CFGR3_EN_VREFINT;
  while (!(SYSCFG_CFGR3 & SYSCFG_CFGR3_REF_HSI48_RDYF));

  rcc_osc_off(RCC_HSI16);
  while (rcc_is_osc_ready(RCC_HSI48)) {};

  rcc_osc_on(RCC_HSI16);
  rcc_wait_for_osc_ready(RCC_HSI16);

  rcc_osc_on(RCC_PLL);
  rcc_wait_for_osc_ready(RCC_PLL);

  rcc_set_sysclk_source(RCC_HSI16);

  rcc_osc_off(RCC_HSI48);
  while (rcc_is_osc_ready(RCC_HSI48)) {};

  /* For USB, but can't use HSI48 as a sysclock on L0 */
  crs_autotrim_usb_enable();
  rcc_set_hsi48_source_rc48();

  rcc_osc_on(RCC_HSI48);
  rcc_wait_for_osc_ready(RCC_HSI48);

}

void system_init(void) {
  leds_init();
  rcc_init();

  printled2(3, 4, LRED );

  devoptab_list[0] = &dotab_cdcacm;
  devoptab_list[1] = &dotab_cdcacm;
  devoptab_list[2] = &dotab_cdcacm;
  cdcacm_init();
}

void init_console(void) {
  setvbuf(stdin, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)

  while (poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }
  printf("Stdin cleared\n");
}

const unsigned char init_data[]={
  0x82, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x00, 0x00,
  0xAA, 0xAA, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0x00,
  0x55, 0xAA, 0xAA, 0x00, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x15, 0x15, 0x15, 0x15,
  0x05, 0x05, 0x05, 0x05, 0x01, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x41, 0x45, 0xF1, 0xFF, 0x5F, 0x55, 0x01, 0x00,
  0x00, 0x00};

static uint8_t rx_values[3096/6];

const unsigned char image[]={
  0xFF, 0xFF, 0xFF, 0xff, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
  0xFF, 0x3F, 0xFF, 0x00, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0x00, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0x00, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0x00, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0x00, 0xFF, 0xFF, 0xFC, 0xFF,
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

int main(void)
{
  system_init();
  init_console();
  char cmd_s[50]="";
  char cmd[10]="";
  char confirm[10]="";
  int i, j;
  int c=0;
  int n_char=0;

  for (int i=0; i<512; i++){
     rx_values[i]=0;
  }

  printled2(5, 4, LRED );

  printf("Reading busy\n\r");
  c=getc(stdin);

  printf("Busy: %d\n", gde021a1_busy(gde021a1_spi));

  gde021a1_setup();
  spi1_epaper_setup();
  printf("Busy: %d\n", gde021a1_busy(gde021a1_spi));

  rx_values[0]=0x00+0x03;
  gde021a1_cmd_data(gde021a1_spi, 0x11, rx_values, 1);

  //X start/end
  rx_values[0]=0x00;
  rx_values[1]=0x11;
  gde021a1_cmd_data(gde021a1_spi, 0x44, rx_values, 2);

  //Y start/end
  rx_values[0]=0x00;
  rx_values[1]=0xAB;
  gde021a1_cmd_data(gde021a1_spi, 0x45, rx_values, 2);

  //x counter
  rx_values[0]=0x00;
  gde021a1_cmd_data(gde021a1_spi, 0x4E, rx_values, 1);

  //y counter
  rx_values[0]=0x00;
  gde021a1_cmd_data(gde021a1_spi, 0x4F, rx_values, 1);

  rx_values[0]=0x1F;
  gde021a1_cmd_data(gde021a1_spi, 0xF0, rx_values, 1);

  rx_values[0]=0xA0;
  gde021a1_cmd_data(gde021a1_spi, 0x2C, rx_values, 1);

  rx_values[0]=0x63;
  gde021a1_cmd_data(gde021a1_spi, 0x3C, rx_values, 1);

  //gde021a1_cmd(gde021a1_spi, 0x32);
  rx_values[0]=0x63;
  gde021a1_cmd_data(gde021a1_spi, 0x32, init_data, 90);
  printf("Busy: %d\n", gde021a1_busy(gde021a1_spi));

  gde021a1_cmd_data(gde021a1_spi, 0x24, rx_values, 0);

  gpio_clear(gde021a1_spi.spicsport, gde021a1_spi.spi_cs);
  gpio_set(gde021a1_spi.dc_port, gde021a1_spi.dc_pin);
  j=0;
  int u=0;
  int v=0;
  int k=0;
  for (int i=0; i<3096; i++){
    //rx_values[j]=i*255/3096;
    //rx_values[j]=255;
    my_spi_xfer(gde021a1_spi.spi, image[j+k*8]);
    //gde021a1_data(gde021a1_spi, rx_values, 512);
    if (k==15) {
      k=0;
    }
    if (j==7){
      j=0; //restart symbol col
    } else {
      j++; //increment symbol col
    }
    if (u==((18)-1)) {
      u=0; //start img col
      v++; //increment img row
      k++; //increment symbol row
      j=0; //restart symbol col
    }
    else {
      u++; //increment im col

    }
  }
  gpio_set(gde021a1_spi.dc_port, gde021a1_spi.dc_pin);
  gpio_set(gde021a1_spi.spicsport, gde021a1_spi.spi_cs);

  rx_values[0]=0x03;
  gde021a1_cmd_data(gde021a1_spi, 0x21, rx_values, 1);

  rx_values[0]=0xC4;
  gde021a1_cmd_data(gde021a1_spi, 0x22, rx_values, 1);

  rx_values[0]=0x63;
  gde021a1_cmd_data(gde021a1_spi, 0x20, rx_values, 0);

  printf("Busy: %d\n", gde021a1_busy(gde021a1_spi));



  printled2(7, 4, LRED );
  while (1){
    printf("Test\n\r");
    if ((poll(stdin) > 0)) {
      i=0;
      if (poll(stdin) > 0) {
    	c=0;
    	while (c!='\r') {
    	  c=getc(stdin);
    	  i++;
    	  putc(c, stdout);
    	}
      }
    }
  }
}
