/*
 * =============================================================================
 *
 * - File        : rng_test.c
 * - Autores     : Daniel Alberto Sáenz Obando  (C37099)
 *                 Elsa Valeria Román Astúa      (C26910)
 * - Curso       : IE0624 - Laboratorio de Microcontroladores
 *                 Universidad de Costa Rica
 * - Fecha       : 16-06-2026
 *
 * - Descripción :
 *   Código base de prueba del periférico RNG (generador de números aleatorios
 *   por hardware) de la tarjeta STM32F429i-disc1. El programa realiza lo
 *   siguiente:
 *     1. Inicializa el reloj del sistema a 168 MHz y el SysTick con base de
 *        tiempo de 1 ms.
 *     2. Inicializa la consola USB CDC-ACM para imprimir por minicom.
 *     3. Configura el periférico RNG por hardware.
 *     4. Cada segundo obtiene un número aleatorio de 32 bits y lo imprime por
 *        la consola, junto con el sector de ruleta al que correspondería
 *        (valor mod NUM_SECTORS).
 *
 *   Sirve como base para verificar que el RNG genera valores impredecibles
 *   antes de integrarlo en la ruleta.
 *
 * =============================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rng.h>

#include <libopencm3-plus/hw-accesories/cm3/clock.h>
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <libopencm3-plus/newlib/syscall.h>

#define NUM_SECTORS 13  // Mismo valor que la ruleta, para ver el mapeo

// LEDs de la tarjeta
#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN GPIO13
#define LED_RED_PORT GPIOC
#define LED_RED_PIN GPIO5

/*
 * void clock_setup(void)
 *
 * @brief  Configura el reloj del sistema a 168 MHz y arranca el SysTick con
 *         una interrupción cada 1 ms (sys_tick_handler y msleep() los provee la
 *         biblioteca libopencm3-plus).
 * @return void.
 */
void clock_setup(void) {
  const uint32_t one_milisecond_rate = 168000;

  // Frecuencia base de la tarjeta: 168 MHz
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  // clock rate / 168000: tasa de interrupción de 1 ms
  systick_set_reload(one_milisecond_rate);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();
  systick_interrupt_enable();
}

/*
 * void leds_init(void)
 *
 * @brief  Configura como salidas los pines de los LEDs verde (PB13) y rojo
 *         (PC5) de la tarjeta.
 * @return void.
 */
void leds_init(void) {
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN_PIN);
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
}

/*
 * void rng_setup(void)
 *
 * @brief  Habilita el reloj del periférico RNG (bus AHB2) y activa el generador
 *         de números aleatorios por hardware.
 * @return void.
 */
void rng_setup(void) {
  rcc_periph_clock_enable(RCC_RNG);
  rng_enable();
}

/*
 * void system_init(void)
 *
 * @brief  Inicializa el reloj, los LEDs, la consola USB CDC-ACM y el RNG.
 * @return void.
 */
void system_init(void) {
  clock_setup();
  leds_init();

  // Redirige stdin/stdout/stderr hacia el dispositivo USB CDC-ACM
  devoptab_list[0] = &dotab_cdcacm;
  devoptab_list[1] = &dotab_cdcacm;
  devoptab_list[2] = &dotab_cdcacm;
  cdcacm_f429_init();

  rng_setup();
}

/*
 * void init_console(void)
 *
 * @brief  Pone stdin/stdout en modo sin buffer y vacía cualquier dato pendiente
 *         en stdin, para una comunicación limpia por minicom.
 * @return void.
 */
void init_console(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  while (lo_poll(stdin) > 0) {
    getc(stdin);
  }
  printf("Stdin cleared\n\r");
}

/*
 * int main(void)
 *
 * @brief  Punto de entrada. Inicializa los periféricos y, en bucle infinito,
 *         imprime un número aleatorio de 32 bits del RNG cada segundo.
 * @return No retorna (bucle infinito).
 */
int main(void) {
  system_init();
  init_console();

  printf("== Prueba del periferico RNG (STM32F429i-disc1) ==\n\r");

  while (true) {
    // Número aleatorio de 32 bits por hardware (bloqueante)
    uint32_t r = rng_get_random_blocking();

    printf("RNG: 0x%08lX  (%10lu)  -> sector %lu\n\r", (unsigned long)r,
           (unsigned long)r, (unsigned long)(r % NUM_SECTORS));

    msleep(1000);  // Espera 1 segundo
  }
}
