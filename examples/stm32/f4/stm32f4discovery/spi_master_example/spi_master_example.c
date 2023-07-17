/*
 * Copyright (C) 2023 Universidad de Costa Rica
 * Author: Oscar Fallas Cordero <ofallasc164@gmail.com> 
 * Author: Alexander Rojas Brenes <alrojasbre29@gmail.com>
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

#include <stdint.h>
#include <stdlib.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

// Pin constant names to improve readability.
#define GPIO_SPI_PORT_SCK_MISO_MOSI (GPIOC)
#define GPIO_SPI_PORT_CS (GPIOA)
#define GPIO_SPI_SCK_PIN (GPIO10)  
#define GPIO_SPI_MISO_PIN (GPIO11) 
#define GPIO_SPI_MOSI_PIN (GPIO12) 
#define GPIO_SPI_CS_PIN (GPIO15)  

// Configuration mode constant name to improve readability.
#define GPIO_SPI_AF (GPIO_AF6)    

// Main clock configuration.
static void setup_main_clock(void)
{
  /* Set the main clock of the microcontroller to a specific frequency using 
  an external oscillator and the PLL configuration.*/
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]); 
}

// SPI and GPIO peripherals clock configuration.
static void setup_peripheral_clocks(void)
{
  rcc_periph_clock_enable(RCC_SPI3);
  rcc_periph_clock_enable(RCC_GPIOA); 
  rcc_periph_clock_enable(RCC_GPIOC);
}

/*The function spi_select and spi_deselect allows to choice between one or more
slaves devices. Also, the CS always must be use because it allows to start the 
data trasmition*/
static void spi_deselect(void)
{
  gpio_set(GPIO_SPI_PORT_CS, GPIO_SPI_CS_PIN); //NS deselect fuction 
}

static void spi_select(void)
{
  gpio_clear(GPIO_SPI_PORT_CS, GPIO_SPI_CS_PIN); //NS select function
}

// SPI configuration.
static void spi_setup(void)
{
  /*
   * Set SPI-SCK & MISO & MOSI pin to alternate function.
   * Set SPI-CS pin to output push-pull (control CS by manual).
   */
  gpio_mode_setup(GPIO_SPI_PORT_SCK_MISO_MOSI,
                  GPIO_MODE_AF,
                  GPIO_PUPD_NONE,
                  GPIO_SPI_SCK_PIN | GPIO_SPI_MISO_PIN | GPIO_SPI_MOSI_PIN);
        

  gpio_set_output_options(GPIO_SPI_PORT_SCK_MISO_MOSI,
                          GPIO_OTYPE_PP,
                          GPIO_OSPEED_50MHZ,
                          GPIO_SPI_SCK_PIN | GPIO_SPI_MOSI_PIN);

  gpio_set_af(GPIO_SPI_PORT_SCK_MISO_MOSI,
              GPIO_SPI_AF,
              GPIO_SPI_SCK_PIN | GPIO_SPI_MISO_PIN | GPIO_SPI_MOSI_PIN);

  /* In master mode, control CS by user instead of AF. */
  gpio_mode_setup(GPIO_SPI_PORT_CS, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_SPI_CS_PIN);
  gpio_set_output_options(GPIO_SPI_PORT_CS, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_SPI_CS_PIN);

  spi_disable(SPI3);

  /* Set up in master mode. */
  spi_init_master(SPI3,
                  SPI_CR1_BAUDRATE_FPCLK_DIV_64,   /* Clock baudrate. */
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, /* CPOL = 0. */
                  SPI_CR1_CPHA_CLK_TRANSITION_2,   /* CPHA = 1. */
                  SPI_CR1_DFF_16BIT,                /* Data frame format. */
                  SPI_CR1_MSBFIRST);               /* Data frame bit order. */
  spi_set_full_duplex_mode(SPI3);

  /*
   * CS pin is not used on master side at standard multi-slave config.
   * It has to be managed internally (SSM=1, SSI=1)
   * to prevent any MODF error.
   */
  spi_enable_software_slave_management(SPI3); /* SSM = 1. */
  spi_set_nss_high(SPI3);                     /* SSI = 1. */

  spi_deselect();
  spi_enable(SPI3);
}

void send_data_spi(uint16_t data)
{
  /*This function manages the data transmition. It sends the select flag to 
  choice the slave device. Then, send a 16-bits data.*/
  spi_select();
  spi_send(SPI3,data);
  while (!(SPI_SR(SPI3) & SPI_SR_TXE)); // Wait for 'Transmit buffer empty' flag to set.
}

void receive_data_spi()
{
  /*This function read 16-bits data from MISO. Then, send the deselect flag.*/
  spi_read(SPI3);
  spi_deselect();
}

int main(void)
{
  /*Main function to initialize clocks and spi setup. It sends a data package 
  to prove the transmiton, and recieve the echo from the slave device.*/
  setup_main_clock();
  setup_peripheral_clocks();
  spi_setup();

  uint16_t data[] = {2, 5, 0xFF, 0xAF, 0xEE, 100, 0xF7, 0xB8};

  // Sends the data from data string one by on, while receiving information from slave
  while (1) {
    for (int i=0; i<=7; i++){
      send_data_spi(data[i]);
      receive_data_spi();
    }
  }

  return 0;
}