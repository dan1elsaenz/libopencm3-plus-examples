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

#include <libopencm3-plus/stm32l476discovery/lcd.h>
#include <libopencm3-plus/utils/misc.h>

#define LED_GREEN_PORT GPIOE
#define LED_GREEN_PIN GPIO8
#define LED_RED_PORT GPIOB
#define LED_RED_PIN GPIO2

void leds_setup(void);

void leds_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOE);
  gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  LED_GREEN_PIN);
  gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
}

int main(void) {
  lcd_init();
  leds_setup();

  // leds
  gpio_set(LED_RED_PORT, LED_RED_PIN);
  gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);

  while (true) {
    gpio_toggle(LED_RED_PORT, LED_RED_PIN);
    gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
    for (int i = 0; i < 400000; i++) {
      __asm__("NOP");
    }
    write_char_to_lcd_ram(POS_6, 'A');
  }
  return 0;
}
