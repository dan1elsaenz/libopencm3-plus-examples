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
#include <libopencm3/stm32/spi.h>
#include <libopencm3-plus/newlib/syscall.h>
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32l0538-disco/leds.h>
#include <limits.h>
#include <stdbool.h>
#include <libopencm3/stm32/syscfg.h>

#define LED_GREEN_PIN GPIO13
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOC

#define I3G4250D_RDWR ( 1 << 7 )
#define I3G4250D_CS GPIOC, GPIO1

#define I3G4250D_WHO_AM_I    0x0F
#define I3G4250D_OUT_TEMP    0x26
#define I3G4250D_STATUS_REG  0x27
#define I3G4250D_CTRL_REG1   0x20
#define I3G4250D_CTRL_REG1_XEN  (1 << 0) // 0.5 s
#define I3G4250D_CTRL_REG1_YEN  (1 << 1)
#define I3G4250D_CTRL_REG1_ZEN  (1 << 2)
#define I3G4250D_CTRL_REG1_PD  (1 << 3)
#define I3G4250D_CTRL_REG1_BW_SHIFT  4
#define I3G4250D_CTRL_REG4    0x23
#define I3G4250D_CTRL_REG4_FS_SHIFT  4

#define I3G4250D_OUT_X_L    0x28
#define I3G4250D_OUT_X_H    0x29


uint8_t read_reg(uint8_t reg){
  uint8_t tmp1;
  uint8_t tmp2;
  tmp1 = I3G4250D_RDWR | (reg & 0x3f); // Registers go up to 11 1000

  // Sending register
  gpio_clear(I3G4250D_CS);
  spi_send(SPI5, tmp1);
  tmp2 = spi_read(SPI5); //reading garbage

  // Sending 8 bits to push result
  spi_send(SPI5, 0);
  tmp2 = spi_read(SPI5);
  gpio_set(I3G4250D_CS);
  return(tmp2);
}

uint16_t write_reg(uint8_t reg, uint8_t data){
  uint8_t tmp1;
  uint8_t tmp2;
  tmp1 = ~I3G4250D_RDWR & (reg & 0x3f); // Registers go up to 11 1000

  // Sending register
  gpio_clear(I3G4250D_CS);
  spi_send(SPI5, tmp1);
  tmp2 = spi_read(SPI5); //reading garbage

  // Sending 8 bits of data
  spi_send(SPI5, data);
  tmp2 = spi_read(SPI5);
  gpio_set(I3G4250D_CS);
  return(tmp2);
}

void spi_init(void) {
  // For SPI5 port
  rcc_periph_clock_enable(RCC_SPI5);
  rcc_periph_clock_enable(RCC_GPIOF);
  gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7 | GPIO8 | GPIO9);
  gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO8 | GPIO9);

  // I3G 4250D SPI chip select
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set(I3G4250D_CS);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

  // SPI5
  //rcc_periph_clock_enable(RCC_SPI5);
  spi_set_master_mode(SPI5);
  spi_set_baudrate_prescaler(SPI5, SPI_CR1_BR_FPCLK_DIV_64);
  spi_enable_software_slave_management(SPI5);

  // CPHA = 1, CPOL = 1 
  spi_set_clock_phase_1(SPI5);
  spi_set_clock_polarity_1(SPI5);
  //spi_set_clock_phase_0(SPI5);
  //spi_set_clock_polarity_0(SPI5);
  spi_set_dff_8bit(SPI5);
  spi_set_nss_high(SPI5);
  spi_enable(SPI5);
}

void leds_init(void) {
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

void system_init(void) {
  leds_init();

  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

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


int main(void)
{
  system_init();
  init_console();
  spi_init();
  char cmd_s[50]="";
  char cmd[10]="";
  char confirm[10]="";
  int i, j;
  int c=0;
  int n_char=0;
  uint8_t data;
  int16_t data2;

  uint8_t temp;
  int8_t temp_entero;

  data = read_reg(I3G4250D_WHO_AM_I);
  write_reg(I3G4250D_CTRL_REG1, I3G4250D_CTRL_REG1_PD
              | I3G4250D_CTRL_REG1_XEN | I3G4250D_CTRL_REG1_YEN
              | I3G4250D_CTRL_REG1_ZEN | (3 << I3G4250D_CTRL_REG1_BW_SHIFT));
  write_reg(I3G4250D_CTRL_REG4, (1 << I3G4250D_CTRL_REG4_FS_SHIFT));
  printf("Test %x \n\r", data);

  while (1){
    // data = read_reg(I3G4250D_WHO_AM_I);
    // //printf("Data: %x \n\r", data);

    // // Esperar 0.5 s
    for (int i = 0; i < 168000000 / 25; i++) {
        __asm__("nop");
    };

    // temp = read_reg(I3G4250D_OUT_TEMP);

    // temp_entero = (int8_t)temp;
    // int celsius = 25 - temp_entero;
    // printf("Temp: 0x%02x, %d, %d\n\r", temp, temp_entero, celsius);
    
    uint8_t xl = read_reg(I3G4250D_OUT_X_L);
    uint8_t xh = read_reg(I3G4250D_OUT_X_H);

    int16_t raw_x = (int16_t)(((uint16_t)xh << 8) | (uint16_t)xl);

    float vel_dps = raw_x * 0.0175f;

    printf("x_raw: %+07d\n\r", raw_x, vel_dps);
    // printf("x_raw: %+07d\r", raw_x);
  }

}