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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include <libopencm3-plus/stm32f429idiscovery/clock.h>
#include <libopencm3-plus/stm32f429idiscovery/console.h>
#include <libopencm3-plus/stm32f429idiscovery/gfx.h>
#include <libopencm3-plus/stm32f429idiscovery/i2c-lcd-touch.h>
#include <libopencm3-plus/stm32f429idiscovery/lcd-spi.h>
#include <libopencm3-plus/stm32f429idiscovery/sdram.h>
#include <libopencm3-plus/utils/misc.h>

#define CHIP_ID 0x00
#define ID_VER 0x02
#define SYS_CTRL1 0x03
#define SYS_CTRL2 0x04
#define TSC_CTRL 0x40
#define TSC_CFG 0x41
#define FIFO_TH 0x4A
#define FIFO_STA 0x4B
#define FIFO_SIZE 0x4C
#define TSC_DATA_X 0x4D
#define TSC_DATA_Y 0x4F
#define TSC_DATA_Z 0x51

// For touch screen (origin lower left)
#define MIN_X 350
#define MIN_Y 280
#define MAX_X 3650
#define MAX_Y 3800

// For screen (origin upper left)
#define X_RES 239
#define Y_RES 319

void clock_setup(void) {
  const uint32_t one_milisecond_rate = 168000;
  /* Base board frequency, set to 168Mhz */
  rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* clock rate / 168000 to get 1mS interrupt rate */
  systick_set_reload(one_milisecond_rate);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();

  /* this done last */
  systick_interrupt_enable();
}

void touch2lcd(int touch_x, int touch_y, int *lcd_x, int *lcd_y) {
  *lcd_x = (touch_x - MIN_X) * X_RES / (MAX_X - MIN_X);
  if (*lcd_x > X_RES) {
    *lcd_x = X_RES;
  }
  if (*lcd_x < 0) {
    *lcd_x = 0;
  }

  *lcd_y = Y_RES - (touch_y - MIN_Y) * Y_RES / (MAX_Y - MIN_Y);
  if (*lcd_y > Y_RES) {
    *lcd_y = Y_RES;
  }
  if (*lcd_y < 0) {
    *lcd_y = 0;
  }
}

int main(void) {
  clock_setup();
  console_setup(115200);
  sdram_init();
  lcd_spi_init();
  i2c_init();

  gfx_init(lcd_draw_pixel, 240, 320);
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);

  uint8_t data2[2];
  uint8_t cmd2[2];
  uint8_t data;
  uint16_t datax, datay, dataz;
  uint8_t cmd;
  uint8_t fifo_state = 0;

  cmd = CHIP_ID;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, data2, 2);

  cmd = ID_VER;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = SYS_CTRL2;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = SYS_CTRL2;
  cmd2[1] = 0x00; // Turn off TSC and ADC clocks
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = SYS_CTRL2;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = TSC_CTRL;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = TSC_CTRL;
  cmd2[1] = 0x11; // Turn on TSC, 4 tracking index, TSC disable
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = TSC_CTRL;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = TSC_CFG;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = TSC_CFG;
  cmd2[1] = 0x9A;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = TSC_CFG;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = FIFO_TH;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = FIFO_TH;
  cmd2[1] = 0x1; // Not zero!
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = FIFO_TH;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = SYS_CTRL2;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = SYS_CTRL2;
  cmd2[1] = 0x00; // Turn on all clocks
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = SYS_CTRL2;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = TSC_CTRL;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd2[0] = TSC_CTRL;
  cmd2[1] = 0x11; // Turn on TSC, 4 tracking index, TSC disable
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, cmd2, 2, &data, 0);

  cmd = TSC_CTRL;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  cmd = TSC_CFG;
  i2c_transfer7(STMPE811_I2C, STMPE811_ADDR, &cmd, 1, &data, 1);

  lcd_show_frame();

  int x = 1;
  int y = 1;

  // uint8_t u8Counter = 0;
  while (true) {
    char sData[64];
    gfx_fillScreen(LCD_BLACK);
    gfx_setCursor((int)(LCD_WIDTH / 4), (int)(LCD_HEIGHT / 2));

    cmd = FIFO_STA;
    i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, &data, 1);

    cmd = FIFO_SIZE;
    i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, &fifo_state, 1);

    while (fifo_state > 0) {

      cmd = TSC_DATA_X;
      i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, data2, 2);
      datax = data2[1] | (data2[0] << 8);

      cmd = TSC_DATA_Y;
      i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, data2, 2);
      datay = data2[1] | (data2[0] << 8);

      cmd = TSC_DATA_Z;
      i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, data2, 2);
      dataz = data2[1];

      cmd = FIFO_SIZE;
      i2c_transfer7(I2C3, STMPE811_ADDR, &cmd, 1, &fifo_state, 1);
    }
    touch2lcd(datax, datay, &x, &y);
    sprintf(sData, "%u %u", x, y);
    gfx_puts(sData);
    lcd_show_frame();
  }
}
