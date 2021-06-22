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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "main.h"
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <libopencm3-plus/newlib/devices/null.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/steval-ids001v4m/leds.h>
#include <stdio.h>
#include <stdlib.h>
//#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#include "steval-ids001v4m.h"
#include "spirit1.h"

typedef struct {
  uint8_t reg;
  uint8_t data;
} Data_write;

Data_write transmit_conf_data[] = {
  { 0xa3, 0x35 },
  { 0x07, 0x36 },
  { 0x0D, 0xAC },
  { 0x08, 0x06 },
  { 0x09, 0x82 },
  { 0x0A, 0x8F },
  { 0x0B, 0x99 },
  { 0x0C, 0x01 },
  { 0x6C, 0x00 },
  { 0x9F, 0xA0 },
  { 0x10, 0x01 },
  { 0x18, 0x87 },
  { 0x1A, 0x93 },
  { 0x1B, 0x1A },
  { 0x1D, 0x13 },
  { 0x1E, 0xC8 },
  { 0x25, 0x62 },
  { 0x27, 0x15 },
  { 0x32, 0x3F },
  { 0x33, 0x30 },
  { 0x35, 0x12 },
  { 0x4f, 0x41 },
  { 0x50, 0x40 },
  { 0x51, 0x01 },
  { 0xA1, 0x25 },
  { 0xBC, 0x22 },
  { 0xA4, 0x0C },
  { 0x50, 0x46 }, // Start calibration
};

const struct rcc_clock_scale rcc_clock_config_32mhz = {
  /* 32MHz PLL from HSE */
  .pll_source = RCC_CFGR_PLLSRC_HSE_CLK,
  .pll_mul = RCC_CFGR_PLLMUL_MUL12,
  .pll_div = RCC_CFGR_PLLDIV_DIV3,
  .hpre = RCC_CFGR_HPRE_NODIV,
  .ppre1 = RCC_CFGR_PPRE_NODIV,
  .ppre2 = RCC_CFGR_PPRE_NODIV,
  .voltage_scale = PWR_SCALE1,
  .flash_waitstates = 1,
  .ahb_frequency	= 32000000,
  .apb1_frequency = 32000000,
  .apb2_frequency = 32000000,
};

static void leds_init(void) {
	/* GPIOB for LEDs */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
	gpio_mode_setup(LEDS_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO0 | GPIO1);


}

static void eeprom_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);

	/* GPIO0 y 1 for LEDs (RED and ORANGE respectively) */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO9);
    gpio_set(GPIOA, GPIO9);
}

void system_init(void) {
  /* CPU/uC general setup */
  rcc_clock_setup_pll(&rcc_clock_config_32mhz);

  // For USB_DU pull-up activation
  rcc_periph_clock_enable(RCC_SYSCFG);
  SYSCFG_PMC |= 0x0001;

  leds_init();
  //printled2(2, 10, LRED);
  //printled2(2, 10, LORANGE);

  /* uC + steval-ids001v4m setup */
  /* devoptab_list[0] = &dotab_usart; */
  /* devoptab_list[1] = &dotab_usart; */
  /* devoptab_list[2] = &dotab_usart; */
  devoptab_list[0] = &dotab_cdcacm;
  devoptab_list[1] = &dotab_cdcacm;
  devoptab_list[2] = &dotab_cdcacm;
  cdcacm_init();
  //usart_port=USART2;
  //usart_init();

  spi_setup();
  /* steval-ids001v4m specific setup */
  spsgrf868_setup();
}

/* void nanowait(int x) { */
/*   volatile int tmp; */
/*   for(int i=0; i<x; i++) { */
/*     __asm__("nop"); */
/*   } */
/* } */

void init_console(void) {
  setvbuf(stdin,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout,NULL,_IONBF,0); // Sets stdin in unbuffered mode (normal for usart com)

  while (poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }
  printf("Stdin cleared\n");
}

#define MAX_LINE 60

void end_waiting(void) {
  static int clr_counter=0;
    if (clr_counter == MAX_LINE) {
      printf("\r");
      for (int i=0; i<MAX_LINE; i++) {
        printf(" ");
      }
      printf("\r");
      clr_counter=0;
    } else {
      printf("-");
      clr_counter+=1;
    }
}


#define MSG_SIZE 50
#define CMD_SIZE 10
#define WRITE_SIZE 50
#define RETURN_CHAR '\r'
#define NULL_CHAR '\0'


void read_serial(char *buffer) {
  int input_position = 0;
  int input_char_value = 0;

  memset(buffer, 0, MSG_SIZE);
  while (input_char_value != RETURN_CHAR) {
    input_char_value = getc(stdin);
    putc(input_char_value, stdout);
    buffer[input_position] = input_char_value;
    input_position++;
  }
  putc('\n', stdout);
  buffer[input_position] = NULL_CHAR;
}

void write_many(Data_write *list, int n) {
  for (int i=0; i<n; i++) {
    printf("Entry: Reg: 0x%02x, data: 0x%02x\n", list[i].reg, list[i].data);
    spsgrf_write(list[i].reg, &(list[i].data), 1);
  }
}

void read_buffer(char *buf, int count) {
  spsgrf_read(SP1_FIFO, buf, count, false);
  buf[count]='\0';
}

int main(void)
{


  system_init();

  uint8_t rx_values[20];
  uint16_t status = 0x00;
  uint16_t status_new = 0x00;
  uint16_t temp = 0x00;
  char rx_buf[97];
  int rxfifo_count=0;

  init_console();

  printf("\n----------\n");

  status = spsgrf_read(0xF0, rx_values, 2, true);
  printf("\n");
  printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status));
  printf("Device Info, PARTNUM: 0x%02X, VERSION: 0x%02X\n", rx_values[1], rx_values[0]);

  //  wait(10);
  printf("\n");
  printf("Sleep!\n");
  status = spsgrf_cmd(SP1_CMD_SLEEP);
  status = spsgrf_read(SP1_MC_STATE, rx_values, 2, true);
  printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status));
  printf(get_state_str(SP1_STATE(status)));
  printf("\n");

  //  wait(10);
  printf("\n");
  printf("Ready!\n");
  status = spsgrf_cmd(SP1_CMD_READY);
  while (SP1_STATE(status) != SP1_ST_READY) {
    status = spsgrf_read(SP1_MC_STATE, rx_values, 2, true);
    printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status));
    printf(get_state_str(SP1_STATE(status)));
    printf("\n");
  }

  /* wait(10); */
  /* printf("\n"); */
  /* printf("Standby!\n"); */
  /* status = spsgrf_cmd(SP1_CMD_STANDBY); */
  /* status = spsgrf_read(SP1_MC_STATE, rx_values, 2); */
  /* printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status)); */
  /* printf(get_state_str(SP1_STATE(status))); */
  /* printf("\n"); */


  //  wait(10);
  printf("\n");
  status = spsgrf_read(SP1_ANA_FUNC_CONF, rx_values, 2, true);
  printf("Read, HIGH: 0x%02X, LOW: 0x%02X\n", rx_values[1], rx_values[0]);
  printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status));
  printf(get_state_str(SP1_STATE(status)));
  printf("\n");

  char my_input[MSG_SIZE];
  char my_cmd[CMD_SIZE];
  uint8_t write_data[WRITE_SIZE];
  uint8_t sp1_reg;
  uint8_t sp1_cmd;
  int read_counts;
  int write_counts;
  int len=0;

  printf("\nExecuting writing sequence!!!\n");
  write_many(transmit_conf_data, sizeof(transmit_conf_data)/sizeof(Data_write));

  printf("\nType your command: r/w/c reg_num readings\n");
  printf("(r) read, (w) write, (c) cmd, (s) get status, (b) read&print buffer\n");
  while (true) {
    if (poll(stdin) > 0) {
      read_serial(my_input);
      sscanf(my_input, "%s", my_cmd);
      printf("my cmd: %s\n", my_cmd);
      switch(my_cmd[0]) {
      case 'r':
        printf("Enter register address to read and number of readings: reg_addr count \n");
        read_serial(my_input);
        sscanf(my_input, "0x%x %d", &sp1_reg, &read_counts);
        if (read_counts==0)
          read_counts=1;
        printf("Reading: %x register, counts: %d\n", sp1_reg, read_counts);
        status = spsgrf_read(sp1_reg, rx_values, read_counts, true);
        printf("--------\n");
        for (int i=0; i<read_counts; i++) {
          printf("Address: 0x%02X, Value: 0x%02X\n", sp1_reg+read_counts-1-i, rx_values[i]);
        }
        printf("--------\n");
        break;
      case 'w':
        printf("Enter write info: reg_addr wr_count \n");
        read_serial(my_input);
        write_counts=1;
        sscanf(my_input, "0x%x %d", &sp1_reg, &write_counts);
        if (write_counts == 0)
          write_counts=1;
        for (int i=0; i<write_counts; i++) {
          printf("Enter data[%d]:\n", i);
          read_serial(my_input);
          sscanf(my_input, "0x%x", &write_data[i]);
          printf("Writing: 0x%02X\n", write_data[i]);
        }
        printf("--------\n");
        printf("Writing: ");
        for (int i=0; i<write_counts; i++) {
          printf(" 0x%02X", write_data[i]);
        }
        printf("\n");
        printf("--------\n");
        status = spsgrf_write(sp1_reg, write_data, write_counts);
        break;
      case 'c':
        printf("Enter command to send: cmd (in hex)\n");
        read_serial(my_input);
        sscanf(my_input, "0x%x", &sp1_cmd);
        printf("--------\n");
        printf("Sending cmd: 0x%x\n", sp1_cmd);
        printf("--------\n");
        status = spsgrf_cmd(sp1_cmd);
        break;
      case 's':
        printf("Retrieving state ...\n");
        status = spsgrf_read(SP1_MC_STATE, rx_values, 2, true);
        status_new = (rx_values[1] << 8) | rx_values[0];
        printf("Current state:\n");
        printf("--------\n");
        print_sp1_status(status_new);
        printf("--------\n");
        break;
      case 'b':
        printf("Reading all buffer ...\n");
        printf("Reading RX buffer count\n");
        status = spsgrf_read(SP1_LINEAR_FIFO_STATUS, rx_values, 2, true);
        temp = ( rx_values[1] << 8 ) | rx_values[0];
        rxfifo_count= SP1_LINEAR_FIFO_STATUS_RXCOUNT(temp);
        printf("RX buffer count: %d\n", rxfifo_count);
        read_buffer(rx_buf, rxfifo_count);
        printf("Received data:\n");
        printf("%s\n", rx_buf);
        printf("--------\n");

      default:
        goto endcmd;
      }

      printf("Previous state:\n");
      printf("MC_STATE: 0x%04X, STATE: 0x%02X\n", status, SP1_STATE(status));
      printf(get_state_str(SP1_STATE(status)));
      printf("\n");
    endcmd:
      printf("\n");
      printf("(r) read, (w) write, (c) cmd, (s) get status\n");
    }
    /* EEPROM test code */
    /* gpio_clear(GPIOA, GPIO9); */
    /* nanowait(10); */
    /* spi_xfer2(SPI2, 0b10000011); */
    /* status = spi_xfer2(SPI2, 0x00) << 8; */
    /* status |= spi_xfer2(SPI2, 0x00); */
    /* nanowait(10); */
    /* gpio_set(GPIOA, GPIO9); */

    //usart_send_blocking(USART2, status >> 8);
    //usart_send_blocking(USART2, status);
    //usart_send_blocking(USART2, rx_values[1]);
    //usart_send_blocking(USART2, rx_values[0]);

    gpio_toggle(LRED);
    wait(2);
    //end_waiting();
    gpio_toggle(LORANGE);
    wait(2);
  }

}
