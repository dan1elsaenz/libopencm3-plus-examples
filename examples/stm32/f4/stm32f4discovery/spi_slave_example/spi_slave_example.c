/*
 * Copyright (C) 2023 Universidad de Costa Rica
 * Author: Alexander Rojas Brenes <alrojasbre29@gmail.com>
 * Author: Oscar Fallas Cordero <ofallasc164@gmail.com>
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

#include <stdint.h>
#include <stdlib.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

// Pin constant names to improve readability.
#define GPIO_SPI_PORT_SCK_MOSI_MISO (GPIOC)
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
    // Set the main clock of the microcontroller to a specific frequency using an external oscillator and the PLL configuration.
    rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

// SPI and GPIO peripherals clock configuration.
static void setup_peripheral_clocks(void)
{
    rcc_periph_clock_enable(RCC_SPI3);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
}

// SPI configuration.
static void setup_spi(void)
{
    // Set SPI pins to alternate function.
    gpio_mode_setup(GPIO_SPI_PORT_SCK_MOSI_MISO,
                    GPIO_MODE_AF, // Usage of alternate function to set pins as input and output.  
                    GPIO_PUPD_NONE, 
                    GPIO_SPI_SCK_PIN | GPIO_SPI_MISO_PIN | GPIO_SPI_MOSI_PIN);  
                    
    gpio_mode_setup(GPIO_SPI_PORT_CS, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SPI_CS_PIN);

    gpio_set_output_options(GPIO_SPI_PORT_SCK_MOSI_MISO,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_50MHZ, // GPIO frequency. 
                            GPIO_SPI_MISO_PIN);

    gpio_set_af(GPIO_SPI_PORT_SCK_MOSI_MISO,
                GPIO_SPI_AF,
                GPIO_SPI_SCK_PIN | GPIO_SPI_MISO_PIN | GPIO_SPI_MOSI_PIN);

    gpio_set_af(GPIO_SPI_PORT_CS,
                GPIO_SPI_AF,
                GPIO_SPI_CS_PIN);

    spi_disable(SPI3); // To ensure that no communication is in progress.

    // SPI init: this function sets the configuration parameters of the peripheral,
    // such as clock speed, CPOL and CPHA mode, data frame format, and bit order 
    // within the frame.
    spi_init_master(SPI3,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_64,   // Clock baudrate.
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, // CPOL = 0.
                    SPI_CR1_CPHA_CLK_TRANSITION_2,   // CPHA = 1.
                    SPI_CR1_DFF_16BIT,               // Data frame format.
                    SPI_CR1_MSBFIRST);               // Data frame bit order.
                    
    spi_set_slave_mode(SPI3);                        // Set to slave mode.
    spi_set_full_duplex_mode(SPI3);                  // Set bidirectional comunication.  
    
    // Set to hardware NSS management and NSS output disable.
    // The NSS pin works as a standard “chip select” input in slave mode.
    spi_disable_software_slave_management(SPI3); // SSM = 0
    spi_disable_ss_output(SPI3);                 // SSOE = 0

    spi_enable(SPI3); // Enables communications. 
}

// Wait until receive-buffer is full and returns the received value.
uint16_t receive_data_spi(void)
{
    while (!(SPI_SR(SPI3) & SPI_SR_RXNE)); // Wait for transmission to complete.
    return spi_read(SPI3); // Read data.
}

// Set the chip select to low, send the data and wait for the transmission to complete.
void send_data_spi(uint16_t data)
{
    spi_send(SPI3, data); // Send data.
    while (!(SPI_SR(SPI3) & SPI_SR_TXE)); // Wait for transmission to complete.
}

// Initialize the peripherals and enters an infinite loop where it receives data 
// from the SPI and sends it back to the SPI the way it came. 
int main(void)
{
    setup_main_clock();
    setup_peripheral_clocks();
    setup_spi();

    while (1) {
        send_data_spi(receive_data_spi());
    }
    
    return 0;
}
