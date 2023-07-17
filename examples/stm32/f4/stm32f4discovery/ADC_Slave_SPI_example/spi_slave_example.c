/*
 * Copyright (C) 2023 Universidad de Costa Rica
 * Author: Alexander Rojas Brenes <alrojasbre29@gmail.com>
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
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>

#include <libopencm3-plus/newlib/syscall.h>
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32f4discovery/leds.h>

#include "cdcacm_example.h"
#include "adc.h"


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
    // Setthe main clock of the microcontroller to a specific frequency using an external oscillator and the PLL configuration.
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
                    SPI_CR1_DFF_16BIT,                // Data frame format.
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

// Función de configuración de parámetros de adc
static void adc_setup(void)
{
	// Activación de clocks
	rcc_periph_clock_enable(RCC_ADC1);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

	// Pines 0-7 del puerto A como entradas analógicas
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, 
					GPIO_ADC_IN__0 | GPIO_ADC_IN__1 | GPIO_ADC_IN__2 |
					GPIO_ADC_IN__3 | GPIO_ADC_IN__4 | GPIO_ADC_IN__5 |
					GPIO_ADC_IN__6 | GPIO_ADC_IN__7);

	// Pines 0 y 1 del puerto B como entradas analógicas
	gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, 
					GPIO_ADC_IN__8 | GPIO_ADC_IN__9);
	
	// Pines 0-5 del puerto C como entradas analógicas
	gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
					GPIO_ADC_IN_10 | GPIO_ADC_IN_11 | GPIO_ADC_IN_12);

	adc_power_off(ADC1);
	adc_set_clk_prescale(ADC_CCR_ADCPRE_BY2); // divide entre 2 la frecuencia del ADC. 

	// Conversión continua
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);

	// Tiempo de muestro
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_56CYC); // Muestrea las señales por 56 ciclos antes de convertirlas.   
	adc_set_resolution(ADC1, ADC_CR1_RES_12BIT); // Resolución de 12 bits en los valores convertidos. 
	adc_power_on(ADC1);

	// Espera de la inicialización del ADC
	int i;
	for (i = 0; i < 800000; i++)
		__asm__("nop");
}

void leds_init(void) 
{
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
	
	// Puertos de salida en caso de que se necesite
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12| GPIO13| GPIO14| GPIO15);
}

void system_init(void) 
{
	// Frecuencia de reloj
	//rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
	leds_init();

	// Elementos que apuntan a la estructura dotab_cdcacm. 
	devoptab_list[0] = &dotab_cdcacm;
	devoptab_list[1] = &dotab_cdcacm;
	devoptab_list[2] = &dotab_cdcacm;
	devoptab_list[3] = &dotab_cdcacm;
	devoptab_list[4] = &dotab_cdcacm;
	devoptab_list[5] = &dotab_cdcacm;
	devoptab_list[6] = &dotab_cdcacm;
	devoptab_list[7] = &dotab_cdcacm;
	devoptab_list[8] = &dotab_cdcacm;
	devoptab_list[9] = &dotab_cdcacm;
	devoptab_list[10] = &dotab_cdcacm;
	devoptab_list[11] = &dotab_cdcacm;
	devoptab_list[12] = &dotab_cdcacm;

	cdcacm_init();
}

// Función que lee el ADC
static uint16_t read_adc(uint8_t channel)
{
	// Definición del canal
	uint8_t channel_array[16];

	// Definición del pin a usar
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);

	// Lectura del ADC
	while (!adc_eoc(ADC1));
	uint16_t signal = adc_read_regular(ADC1);
	
	return signal;
}

// Initialize the peripherals and enters an infinite loop where it receives data 
// from the SPI and sends it back to the SPI after subtracting 0x000F.
int main(void)
{
    setup_main_clock();
    setup_peripheral_clocks();
    setup_spi();

    // Inicialización de las variables en las que se almacenarán los resultados de la conversión. 
	uint16_t signal__0 = 0, signal__1 = 0, signal__2 = 0, signal__3 = 0;
	uint16_t signal__4 = 0, signal__5 = 0, signal__6 = 0, signal__7 = 0;
	uint16_t signal__8 = 0, signal__9 = 0, signal_10 = 0, signal_11 = 0;
	uint16_t signal_12 = 0; 

	adc_setup();
	system_init();

	// Sets stdin in unbuffered mode (normal for usart com)
	setvbuf(stdin,NULL,_IONBF,0);
	// Sets stdout in unbuffered mode (normal for usart com)
	setvbuf(stdout,NULL,_IONBF,0); 

	while (lo_poll(stdin) > 0) {
		printf("Cleaning stdin\n");
		getc(stdin);
	}

	while (1) {
		// Toma de datos de cada ADC usado
		signal__0 = read_adc(ADC_SIGNAL__0); // PA0
		signal__1 = read_adc(ADC_SIGNAL__1); // PA1
		signal__2 = read_adc(ADC_SIGNAL__2); // PA2
		signal__3 = read_adc(ADC_SIGNAL__3); // PA3
		signal__4 = read_adc(ADC_SIGNAL__4); // PA4
		signal__5 = read_adc(ADC_SIGNAL__5); // PA5
		signal__6 = read_adc(ADC_SIGNAL__6); // PA6
		signal__7 = read_adc(ADC_SIGNAL__7); // PA7
		signal__8 = read_adc(ADC_SIGNAL__8); // PB0
		signal__9 = read_adc(ADC_SIGNAL__9); // PB1
		signal_10 = read_adc(ADC_SIGNAL_10); // PC0
		signal_11 = read_adc(ADC_SIGNAL_11); // PC1
		signal_12 = read_adc(ADC_SIGNAL_12); // PC2

		// Impresión de los datos de cada ADC 
		printf("%d,%d,%d,%d\n", signal__0, signal__1, signal__2, signal__3);
		printf("%d,%d,%d,%d\n", signal__4, signal__5, signal__6, signal__7);
		printf("%d,%d,%d,%d\n", signal__8, signal__9, signal_10, signal_11);
		printf("%d\n\n\n", signal_12); 

		send_data_spi(receive_data_spi());
	}

    return 0;
}
