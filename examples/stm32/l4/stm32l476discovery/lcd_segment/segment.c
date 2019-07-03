/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018 Karl Palsson <karlp@tweak.net.au>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/l4/exti.h>
#include <libopencm3/stm32/l4/flash.h>
#include <libopencm3/stm32/l4/gpio.h>
#include <libopencm3/stm32/l4/rcc.h>

#define LED_GREEN_PORT GPIOE
#define LED_GREEN_PIN GPIO8
#define LED_RED_PORT GPIOB
#define LED_RED_PIN GPIO2

void gpio_setup(void);
void lcd_setup(void);

static void clock_setup(void) {
  /* FIXME - this should eventually become a clock struct helper setup */
  rcc_osc_on(RCC_HSI16);

  flash_prefetch_enable();
  flash_set_ws(4);
  flash_dcache_enable();
  flash_icache_enable();
  /* 16MHz / 4 = > 4 * 40 = 160MHz VCO => 80MHz main pll  */
  rcc_set_main_pll(RCC_PLLCFGR_PLLSRC_HSI16, 4, 40, 0, 0,
                   RCC_PLLCFGR_PLLR_DIV2);
  rcc_osc_on(RCC_PLL);
  rcc_set_sysclk_source(RCC_CFGR_SW_PLL); /* careful with the param here! */
  rcc_wait_for_sysclk_status(RCC_PLL);
  /* FIXME - eventually handled internally */
  rcc_ahb_frequency = 80e6;
  rcc_apb1_frequency = 80e6;
  rcc_apb2_frequency = 80e6;
}

void lcd_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO7 | GPIO8 | GPIO9 | GPIO10);
}

void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOE);
  gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  LED_GREEN_PIN);
  /* red led for buttons */
  gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
}

int main(void) {
  clock_setup();
  gpio_setup();
  gpio_set(LED_RED_PORT, LED_RED_PIN);
  gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
  lcd_setup();

  gpio_set(GPIOA, GPIO8); // com1
  while (true) {
    gpio_toggle(LED_RED_PORT, LED_RED_PIN);
    gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
    for (int i = 0; i < 4000; i++) {
      __asm__("NOP");
    }
    gpio_toggle(GPIOA, GPIO7); // seg0
  }
  return 0;
}
