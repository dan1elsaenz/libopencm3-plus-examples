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

#include <math.h>
#include <stdint.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

#include <libopencm3-plus/utils/misc.h>

#include <libopencm3-plus/stm32f429idiscovery/clock.h>
#include <libopencm3-plus/stm32f429idiscovery/console.h>
#include <libopencm3-plus/stm32f429idiscovery/gfx.h>
#include <libopencm3-plus/stm32f429idiscovery/lcd-spi.h>
#include <libopencm3-plus/stm32f429idiscovery/sdram.h>

#define SLEEP_TIME 2000
#define CONSOLE_BAUD_RATE 115200

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

/*
 * This is our example, the heavy lifing is actually in lcd-spi.c but
 * this drives that code.
 */
int main(void) {
  int planet_1, planet_2, planet_3;

  clock_setup();
  console_setup(CONSOLE_BAUD_RATE);
  sdram_init();
  lcd_spi_init();
  console_puts("LCD Initialized\n");
  console_puts("Should have a checker pattern, press any key to proceed\n");
  msleep(SLEEP_TIME);
  /*	(void) console_getc(1); */
  gfx_init(lcd_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
  gfx_fillScreen(LCD_GREY);
  gfx_fillRoundRect(10, 10, 220, 220, 5, LCD_WHITE);
  gfx_drawRoundRect(10, 10, 220, 220, 5, LCD_RED);
  gfx_fillCircle(20, 250, 10, LCD_RED);
  gfx_fillCircle(120, 250, 10, LCD_GREEN);
  gfx_fillCircle(220, 250, 10, LCD_BLUE);
  gfx_setTextSize(2);
  gfx_setCursor(15, 25);
  gfx_puts("STM32F4-DISCO");
  gfx_setTextSize(1);
  gfx_setCursor(15, 49);
  gfx_puts("Simple example to put some");
  gfx_setCursor(15, 60);
  gfx_puts("stuff on the LCD screen.");
  lcd_show_frame();
  console_puts("Now it has a bit of structured graphics.\n");
  console_puts("Press a key for some simple animation.\n");
  msleep(SLEEP_TIME);
  /*	(void) console_getc(1); */
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(3);
  planet_1 = 0;
  planet_2 = 45;
  planet_3 = 90;
  while (1) {
    gfx_fillScreen(LCD_BLACK);
    gfx_setCursor(15, 36);
    gfx_puts("PLANETS!");
    gfx_fillCircle(120, 160, 40, LCD_YELLOW);
    gfx_drawCircle(120, 160, 55, LCD_GREY);
    gfx_drawCircle(120, 160, 75, LCD_GREY);
    gfx_drawCircle(120, 160, 100, LCD_GREY);

    gfx_fillCircle(120 + (sin(degrees_to_radians(planet_1)) * 55),
                   160 + (cos(degrees_to_radians(planet_1)) * 55), 5, LCD_RED);
    gfx_fillCircle(120 + (sin(degrees_to_radians(planet_2)) * 75),
                   160 + (cos(degrees_to_radians(planet_2)) * 75), 10,
                   LCD_WHITE);
    gfx_fillCircle(120 + (sin(degrees_to_radians(planet_3)) * 100),
                   160 + (cos(degrees_to_radians(planet_3)) * 100), 8,
                   LCD_BLUE);
    planet_1 = (planet_1 + 3) % 360;
    planet_2 = (planet_2 + 2) % 360;
    planet_3 = (planet_3 + 1) % 360;
    lcd_show_frame();
  }
}
