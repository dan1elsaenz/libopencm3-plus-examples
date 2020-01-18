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
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "cdcacm_example.h"

#include <libopencm3-plus/cdcacm_one_serial/cdcacm_common.h>
#include <libopencm3-plus/stm32f429idiscovery/console.h>
#include <libopencm3-plus/cdcacm_one_serial/cdcacm.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32f4discovery/leds.h>
#include <limits.h>
#include <stdbool.h>


/*
 * int len = print_decimal(int value)
 *
 * Very simple routine to print an integer as a decimal
 * number on the console.
 */
int print_decimal(int num) {
  int ndx = 0;
  char buf[10];
  int len = 0;
  char is_signed = 0;

  if (num < 0) {
    is_signed++;
    num = 0 - num;
  }
  buf[ndx++] = '\000';
  do {
    buf[ndx++] = (num % 10) + '0';
    num = num / 10;
  } while (num != 0);
  ndx--;
  if (is_signed != 0) {
    console_putc('-');
    len++;
  }
  while (buf[ndx] != '\000') {
    console_putc(buf[ndx--]);
    len++;
  }
  return len; /* number of characters printed */
}


void cdcacm_init(void) {

  //System setup
  //NOTE: This is the current setup, this time we dont consider the VBUS for reasons later explained.

  rcc_periph_clock_enable(RCC_GPIOB);                                     //Current pins
	rcc_periph_clock_enable(RCC_OTGHS);                                     //In the datasheet, the pins PB14 and PB15 are conected to the system OTG_HS                    
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO14 | GPIO15);  //And that same should work for OTG_FS (FS == Full speed, HS == High speed)
	gpio_set_af(GPIOB, GPIO_AF12, GPIO14 | GPIO15);                         //We use this initialization because thats the one used in the libopencm3 examples. 
                                                                          //(which isnt working either)

  //NOTE: The following is the older setup, this one considers 3 pins, but that changed for the STM32F4, as the VBUS no longer is manually enabled
  //(in the datasheet is considered as a special function (not alternative), and those have to be activated manually
  //in the register, but even then, for the oficial examples of libopencm3, they dont consider this activation in the setup). 
  
  #if 0
  rcc_peripheral_enable_clock(&RCC_AHB2ENR, RCC_AHB2ENR_OTGFSEN); //Older code
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);  //This one considers 3 pins
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO11 | GPIO12); 
  gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);       
  #endif

  cdcacm_usb_init();    
  nvic_set_priority(NVIC_OTG_FS_IRQ, IRQ_PRI_USB);
  nvic_enable_irq(NVIC_OTG_FS_IRQ);


  //WARNING: Having problems here, code doesnt advance, as cdcacm_get_config never turns to 1 (state of succesful configuration). 
  while (cdcacm_get_config() != 1) {
    wait(1);
  }; // wait until usb is configured

    
}

void otg_fs_isr(void) { usbd_poll(usbdev); }

//Leds for debugging
void leds_init(void) { 
	rcc_periph_clock_enable(RCC_GPIOG);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13 | GPIO14);
}

void system_init(void) {
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
  leds_init();            //Leds for debugging reasons
  console_setup(115200);  //Console for debugging reasons
  cdcacm_init();

}

int main(void)
{
  system_init();
  char cmd_s[50]="";
  char cmd[10]="";
  char confirm[10]="";
  int i, j;
  int c=0;
  int n_char=0;

  setvbuf(stdin, NULL, _IONBF, 0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout, NULL, _IONBF, 0); // Sets stdin in unbuffered mode (normal for usart com)

  while (poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }

  while (1){
    printf("Test\n");
    if ((poll(stdin) > 0)) {
      i=0;
      if (poll(stdin) > 0) {
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
