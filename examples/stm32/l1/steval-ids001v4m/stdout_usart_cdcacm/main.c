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
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "main.h"
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <libopencm3-plus/newlib/devices/null.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/steval-ids001v4m/leds.h>
#include <limits.h>
#include <stdbool.h>



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


void leds_init(void) {
	/* GPIOB for LEDs */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
	gpio_mode_setup(LEDS_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO0 | GPIO1);

}

void system_init(void) {
  /* CPU/uC general setup */
  rcc_clock_setup_pll(&rcc_clock_config_32mhz);
  rcc_periph_clock_enable(RCC_SYSCFG);
  SYSCFG_PMC |= 0x0001;
  leds_init();
  cdcacm_init();
  devoptab_list[0] = &dotab_usart;
  devoptab_list[1] = &dotab_usart;
  devoptab_list[2] = &dotab_usart;
  usart_port=USART2;
  usart_init();
}

int main(void)
{
  system_init();
  //gpio_set(LRED);
  printled2(3, 2, LRED);
  char cmd_s[50]="";
  char cmd[10]="";
  char confirm[10]="";
  int i, j;
  int c=0;
  int n_char=0;

  setvbuf(stdin,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)

  while (poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }

  while (1){
    printled2(1, 1, LORANGE);
    printf("Test\n");
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
