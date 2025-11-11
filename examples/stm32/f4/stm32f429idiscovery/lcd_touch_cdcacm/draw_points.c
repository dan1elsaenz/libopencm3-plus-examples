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
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>

#define LED_GREEN_PIN GPIO13
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOC


void leds_init(void) {
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

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

void system_init(void) {
  clock_setup();

  leds_init();

  devoptab_list[0] = &dotab_cdcacm;
  devoptab_list[1] = &dotab_cdcacm;
  devoptab_list[2] = &dotab_cdcacm;
  cdcacm_f429_init();
  /* devoptab_list[0] = &dotab_usart; */
  /* devoptab_list[1] = &dotab_usart; */
  /* devoptab_list[2] = &dotab_usart; */
  /* usart_init(); */
}

void init_console(void) {
  setvbuf(stdin, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)

  while (lo_poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }
  printf("Stdin cleared\n");
}

int main(void) {
  system_init();
  init_console();
  sdram_init();
  lcd_spi_init();
  tft_setup();

  int i, j;
  int c=0;

  gfx_init(lcd_draw_pixel, 240, 320);
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);
  gfx_setCursor(50, 50);
  gfx_puts("test");
  lcd_show_frame();

  const uint8_t CIRCLE_RADIUS = 5;
  const uint8_t ARRAY_SIZE = 50;

  int x_pos = 1;
  int y_pos = 1;
  int x_positions[ARRAY_SIZE];
  int y_positions[ARRAY_SIZE];
  uint8_t current_fifo_size = 0;
  uint16_t data_x = 0;
  uint16_t data_y = 0;
  uint16_t data_z = 0;

  for (int i = 0; i < ARRAY_SIZE; ++i) {
    x_positions[i] = 0;
    y_positions[i] = 0;
  }

  uint8_t array_index = 0;
  while (true) {
    gfx_fillScreen(LCD_BLACK);
    current_fifo_size = tft_get_fifo_size();

    while (current_fifo_size > 0) {
      tft_get_coord_data_access(X_COORD, &data_x);
      tft_get_coord_data_access(Y_COORD, &data_y);
      tft_get_coord_data_access(Z_COORD, &data_z);
      current_fifo_size = tft_get_fifo_size();
    }
    if (tft_is_touch_detected()) {
      tft_convert_touch_coord_to_lcd_coord(data_x, data_y, &x_pos, &y_pos);
      x_positions[array_index] = x_pos;
      y_positions[array_index] = y_pos;
      array_index = array_index > ARRAY_SIZE ? 0 : array_index + 1;
    }
    for (int i = 0; i < ARRAY_SIZE; ++i) {
      gfx_fillCircle(x_positions[i], y_positions[i], CIRCLE_RADIUS, LCD_YELLOW);
    }
    gfx_setCursor(50, 50);
    gfx_puts("test");
    lcd_show_frame();

    /* USB terminal */
    printf("Test\n\r");
    if ((lo_poll(stdin) > 0)) {
      i=0;
      if (lo_poll(stdin) > 0) {
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
