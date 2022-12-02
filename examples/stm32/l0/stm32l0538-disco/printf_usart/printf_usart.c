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
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "printf_usart.h"
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32l0538-disco/leds.h>
#include <limits.h>
#include <stdbool.h>
#include <libopencm3/stm32/syscfg.h>

#define LED_GREEN_PIN GPIO4
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOA


void leds_init(void) {
	rcc_peripheral_enable_clock(&RCC_IOPENR, RCC_IOPENR_IOPAEN);
    __asm__("nop");
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	rcc_peripheral_enable_clock(&RCC_IOPENR, RCC_IOPENR_IOPBEN);
    __asm__("nop");
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
}

void system_init(void) {
  leds_init();
  //rcc_clock_setup_hsi(&rcc_hsi_configs[RCC_CLOCK_HSI_48MHZ]);

  // This uC has a separated clock for the USB and RNG

  /* HSI48 needs the vrefint turned on */
  rcc_periph_clock_enable(RCC_SYSCFG);
  __asm__("nop");
  SYSCFG_CFGR3 |= SYSCFG_CFGR3_ENREF_HSI48 | SYSCFG_CFGR3_EN_VREFINT;
  while (!(SYSCFG_CFGR3 & SYSCFG_CFGR3_REF_HSI48_RDYF));


  rcc_osc_off(RCC_PLL);
  while (rcc_is_osc_ready(RCC_PLL)) {};


  /* PLL from HSI16, just to exercise that code */
  struct rcc_clock_scale myclock = {
    .pll_mul = RCC_CFGR_PLLMUL_MUL4,
    .pll_div = RCC_CFGR_PLLDIV_DIV2,
    .pll_source = RCC_CFGR_PLLSRC_HSI16_CLK,
    .flash_waitstates = FLASH_ACR_LATENCY_1WS,
    .voltage_scale = PWR_SCALE1,
    .hpre = RCC_CFGR_HPRE_NODIV,
    .ppre1 = RCC_CFGR_PPRE1_NODIV,
    .ppre2 = RCC_CFGR_PPRE2_NODIV,
    .ahb_frequency = 32e6,
    .apb1_frequency = 32e6,
    .apb2_frequency = 32e6,
    .msi_range = RCC_ICSCR_MSIRANGE_4MHZ
  };
  rcc_clock_setup_pll(&myclock);

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

  /* Configure USART1 to use System Clock as clock source */
  rcc_set_usart1_sel(RCC_CCIPR_USART1SEL_SYS);

  //printled2(3, 4, LRED );

  //devoptab_list[0] = &dotab_cdcacm;
  //devoptab_list[1] = &dotab_cdcacm;
  //devoptab_list[2] = &dotab_cdcacm;
  //cdcacm_init();
  devoptab_list[0] = &dotab_usart;
  devoptab_list[1] = &dotab_usart;
  devoptab_list[2] = &dotab_usart;
  //usart_port=USART1;
  usart_init();
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
