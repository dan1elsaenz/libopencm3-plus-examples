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

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include <libopencm3-plus/hw-accesories/cm3/clock.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/i2c-lcd-touch.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/lcd-serial-touch.h>
#include <libopencm3-plus/hw-accesories/lcd/lcd-spi.h>
#include <libopencm3-plus/hw-accesories/sdram_stm32f429idiscovery.h>

void tft_init(void);

void clock_setup(void) {
  const uint32_t one_milisecond_rate = 168000;
  /* Base board frequency, set to 168Mhz */
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* clock rate / 168000 to get 1mS interrupt rate */
  systick_set_reload(one_milisecond_rate);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();

  /* this done last */
  systick_interrupt_enable();
}

void tft_init(void) {
  i2c_init();

  tft_enable_clocks(false);
  tft_touchscreen_enable_controller_control(false);

  tft_touchscreen_configure_controller_control(TSC_XYZ_AXIS, TRACK_8);
  tft_touchscreen_apply_controller_configuration(MICRO_S_500_SET, MICRO_S_500_D,
                                                 SAMPLE_2);
  tft_set_fifo_level_interrupt(0x1);

  tft_touchscreen_enable_controller_control(true);
  tft_enable_clocks(true);
}

int main(void) {
  clock_setup();
  sdram_init();
  lcd_spi_init();
  tft_init();

  gfx_init(lcd_draw_pixel, 240, 320);
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);
  lcd_show_frame();

  int x = 1;
  int y = 1;
  uint8_t current_fifo_size = 0;
  uint16_t datax = 0;
  uint16_t datay = 0;
  uint16_t dataz = 0;
  char sData[64];

  while (true) {
    gfx_fillScreen(LCD_BLACK);
    gfx_setCursor((int)(LCD_WIDTH / 4), (int)(LCD_HEIGHT / 2));

    current_fifo_size = tft_get_fifo_size();

    while (current_fifo_size > 0) {
      tft_get_coord_data_access(X_COORD, &datax);
      tft_get_coord_data_access(Y_COORD, &datay);
      tft_get_coord_data_access(Z_COORD, &dataz);
      current_fifo_size = tft_get_fifo_size();
    }
    tft_convert_touch_coord_to_lcd_coord(datax, datay, &x, &y);
    y = LCD_HEIGHT - y;
    sprintf(sData, "%u %u", x, y);
    gfx_puts(sData);
    lcd_show_frame();
  }
}
