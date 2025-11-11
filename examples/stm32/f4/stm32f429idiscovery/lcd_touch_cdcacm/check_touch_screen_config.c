/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include <libopencm3-plus/hw-accesories/cm3/clock.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/i2c-lcd-touch.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/lcd-serial-touch.h>
#include <libopencm3-plus/hw-accesories/lcd/lcd-spi.h>
#include <libopencm3-plus/hw-accesories/sdram_stm32f429idiscovery.h>
#include <libopencm3-plus/utils/misc.h>


void tft_init(void);

void clock_setup(void) {
  /* Base board frequency, set to 168Mhz */
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* clock rate / 168000 to get 1mS interrupt rate */
  const uint32_t one_milisecond_rate = 168000;

  systick_set_reload(one_milisecond_rate);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();

  /* this done last */
  systick_interrupt_enable();
}

int main(void) {
  clock_setup();
  sdram_init();
  lcd_spi_init();
  tft_setup();

  gfx_init(lcd_draw_pixel, 240, 320);
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);
  lcd_show_frame();

  // clocks
  const uint8_t clock_register_value = tft_get_clock_status();
  // controller control config
  const uint8_t controller_control_config = tft_get_controller_control_status();
  // controller config
  const uint8_t controller_configuration =
      tft_get_touchscreen_controller_configuration();
  // fifo config
  const uint8_t fifo_interrupt_level = tft_get_fifo_interrupt_level();

  char string_data[120];
  const uint8_t WAIT_TIME_TO_SHOW_CONFIG = 100;
  while (true) {
    gfx_fillScreen(LCD_BLACK);
    gfx_setCursor((int)(1), (int)(LCD_HEIGHT / 2));

    sprintf(string_data,
            "cfg(expect val) : hex val\nclock(0x00) : %X\ncontrol(0x21) : %X "
            "\nconfig(0x5A) : %X \nfifo interr(0x1): %d",
            clock_register_value, controller_control_config,
            controller_configuration, fifo_interrupt_level);

    gfx_puts(string_data);
    lcd_show_frame();
    wait(WAIT_TIME_TO_SHOW_CONFIG);
  }
}
