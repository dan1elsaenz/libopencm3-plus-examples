/*
 * =============================================================================
 *
 * - File        : roulette.c
 * - Autores     : Daniel Alberto Sáenz Obando  (C37099)
 *                 Elsa Valeria Román Astúa     (C26910)
 * - Curso       : IE0624 - Laboratorio de Microcontroladores
 *                 Universidad de Costa Rica
 * - Fecha       : 15-06-2026
 *
 * - Descripción :
 *   Ruleta tipo casino sobre la tarjeta STM32F429i-disc1. El programa realiza
 *   lo siguiente:
 *     1. Inicializa el reloj del sistema a 168 MHz y el SysTick con base de
 *        tiempo de 1 ms (usado para temporizar la animación).
 *     2. Inicializa la SDRAM, la pantalla LCD, la pantalla táctil (STMPE811)
 *        y la biblioteca gráfica gfx.
 *     3. Configura el periférico RNG por hardware para obtener números
 *        aleatorios verdaderos de 32 bits.
 *     4. Renderiza un anillo de sectores de colores (rojo, negro y verde) con
 *        su número dibujado dentro y una flecha en la zona central.
 *     5. Espera un toque en la pantalla; al detectarlo, el RNG escoge el sector
 *        ganador y la flecha gira con desaceleración progresiva hasta detenerse
 *        en ese sector.
 *     6. Resalta el sector ganador y muestra el resultado por la pantalla.
 *
 * =============================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rng.h>

#include <libopencm3-plus/hw-accesories/cm3/clock.h>
#include <libopencm3-plus/hw-accesories/lcd/lcd-spi.h>
#include <libopencm3-plus/hw-accesories/sdram_stm32f429idiscovery.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/i2c-lcd-touch.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/lcd-serial-touch.h>
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>

/*
 * Parámetros de la ruleta
 */
#define NUM_SECTORS  13                // Sector 0 verde, rojo/negro el resto
#define CX           (LCD_WIDTH / 2)   // Centro X de la ruleta
#define CY           (LCD_HEIGHT / 2)  // Centro Y de la ruleta
#define R_OUTER      110               // Radio exterior del anillo
#define R_INNER      78                // Radio interior del anillo (límite flecha)
#define R_LABEL      93                // Radio donde se dibujan los números
#define SPIN_TURNS   5                 // Vueltas completas antes de aterrizar
#define SPIN_MS      4000              // Duración de la animación del giro (ms)
#define FRAME_MS     20                // 50 FPS

#define TWO_PI       6.28318530718f
#define HALF_PI      1.57079632679f

/* 
 * LEDs de la tarjeta
 */
#define LED_GREEN_PORT  GPIOB
#define LED_GREEN_PIN   GPIO13
#define LED_RED_PORT    GPIOC
#define LED_RED_PIN     GPIO5

// Orientación angular actual de la flecha
static float arrow_theta = -HALF_PI; // Inicia apuntando hacia arriba

/*
 * uint16_t sector_color(int sector)
 *
 * @brief  Determina el color de un sector de la ruleta.
 * @param  sector: índice del sector [0, NUM_SECTORS-1].
 * @return Color en formato RGB565
 */
static uint16_t sector_color(int sector) {
  if (sector == 0)
    // El cero siempre es verde
    return LCD_GREEN; 

  return (sector % 2) ? LCD_RED : LCD_BLACK;
}

/*
 * const char *sector_color_name(int sector)
 *
 * @brief  Devuelve el nombre textual del color de un sector.
 * @param  sector: índice del sector (0..NUM_SECTORS-1).
 * @return Cadena "VERDE", "ROJO" o "NEGRO".
 */
static const char *sector_color_name(int sector) {
  if (sector == 0)
    return "VERDE";
  return (sector % 2) ? "ROJO" : "NEGRO";
}

/*
 * void clock_setup(void)
 *
 * @brief  Configura el reloj del sistema a 168 MHz y arranca el SysTick con
 *         una interrupción cada 1 ms (la rutina sys_tick_handler y el contador
 *         mtime() se proveen en la biblioteca libopencm3-plus).
 * @return void.
 */
void clock_setup(void) {
  const uint32_t one_milisecond_rate = 168000;

  // Frecuencia 168 MHz
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
 * void draw_wheel(void)
 *
 * @brief  Renderiza una sola vez en el framebuffer el anillo de sectores de
 *         color y el número de cada sector. Recorre el bounding box del anillo
 *         y, para cada píxel dentro de la corona [R_INNER, R_OUTER], calcula el
 *         sector al que pertenece según su ángulo y lo pinta con su color.
 * @return void.
 */
void draw_wheel(void) {
  const int r_in2 = R_INNER * R_INNER;
  const int r_out2 = R_OUTER * R_OUTER;

  // Pintado de la corona de color, píxel a píxel
  for (int y = CY - R_OUTER; y <= CY + R_OUTER; ++y) {
    for (int x = CX - R_OUTER; x <= CX + R_OUTER; ++x) {
      int dx = x - CX;
      int dy = y - CY;
      int r2 = dx * dx + dy * dy;
      if (r2 < r_in2 || r2 > r_out2)
        continue; // Fuera de la corona de color

      // Ángulo del píxel normalizado a [0, 2π) y mapeo a sector
      float ang = atan2f((float)dy, (float)dx);
      if (ang < 0.0f)
        ang += TWO_PI;
      int sector = (int)(ang / TWO_PI * NUM_SECTORS);
      if (sector >= NUM_SECTORS)
        sector = NUM_SECTORS - 1;

      gfx_drawPixel(x, y, sector_color(sector));
    }
  }

  // Dibujo de los números, uno por sector, en su ángulo central
  const float step = TWO_PI / NUM_SECTORS;
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);
  for (int s = 0; s < NUM_SECTORS; ++s) {
    float mid = (s + 0.5f) * step;
    int lx = CX + (int)(R_LABEL * cosf(mid));
    int ly = CY + (int)(R_LABEL * sinf(mid));

    char buf[4];
    snprintf(buf, sizeof(buf), "%d", s);
    int len = (int)strlen(buf);

    // Centrado del texto (6 px de ancho y 8 px de alto por carácter a tamaño 1)
    gfx_setCursor(lx - len * 3, ly - 4);
    // Texto blanco sobre el color del sector
    gfx_setTextColor(LCD_WHITE, sector_color(s));
    gfx_puts(buf);
  }
}

/*
 * void draw_arrow(float theta)
 *
 * @brief  Borra la zona central y dibuja la flecha indicadora apuntando en la
 *         dirección dada. La flecha es un triángulo relleno con la punta hacia
 *         el borde interior del anillo y un cubo central como pivote.
 * @param  theta: ángulo (en radianes) hacia el que apunta la flecha.
 * @return void.
 */
void draw_arrow(float theta) {
  // Limpia la zona interior para borrar la flecha del cuadro anterior
  gfx_fillCircle(CX, CY, R_INNER - 1, LCD_BLACK);

  float c = cosf(theta);
  float s = sinf(theta);

  // Punta de la flecha cerca del borde interior del anillo
  int tx = CX + (int)((R_INNER - 6) * c);
  int ty = CY + (int)((R_INNER - 6) * s);

  // Esquinas de la base, perpendiculares a la dirección de la flecha
  const int base_r = 12;
  int b1x = CX + (int)(base_r * cosf(theta + HALF_PI));
  int b1y = CY + (int)(base_r * sinf(theta + HALF_PI));
  int b2x = CX + (int)(base_r * cosf(theta - HALF_PI));
  int b2y = CY + (int)(base_r * sinf(theta - HALF_PI));

  gfx_fillTriangle(tx, ty, b1x, b1y, b2x, b2y, LCD_YELLOW);

  // Cubo central (pivote)
  gfx_fillCircle(CX, CY, 6, LCD_WHITE);
}

/*
 * void highlight_sector(int sector)
 *
 * @brief  Resalta el sector ganador dibujando sus dos bordes radiales en
 *         blanco, desde el radio interior hasta el exterior del anillo.
 * @param  sector: índice del sector ganador.
 * @return void.
 */
void highlight_sector(int sector) {
  const float step = TWO_PI / NUM_SECTORS;
  float a0 = sector * step;
  float a1 = (sector + 1) * step;

  gfx_drawLine(CX + (int)(R_INNER * cosf(a0)), CY + (int)(R_INNER * sinf(a0)),
               CX + (int)(R_OUTER * cosf(a0)), CY + (int)(R_OUTER * sinf(a0)),
               LCD_WHITE);
  gfx_drawLine(CX + (int)(R_INNER * cosf(a1)), CY + (int)(R_INNER * sinf(a1)),
               CX + (int)(R_OUTER * cosf(a1)), CY + (int)(R_OUTER * sinf(a1)),
               LCD_WHITE);
}

/*
 * void show_result(int sector)
 *
 * @brief  Muestra el resultado del giro (color y número del sector ganador) en
 *         la parte inferior de la pantalla y resalta el sector en el anillo.
 * @param  sector: índice del sector ganador.
 * @return void.
 */
void show_result(int sector) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%s %d", sector_color_name(sector), sector);

  highlight_sector(sector);

  // Limpia la franja inferior y escribe el resultado centrado
  gfx_fillRect(0, LCD_HEIGHT - 28, LCD_WIDTH, 28, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MID_SIZE);
  gfx_setTextColor(LCD_WHITE, LCD_BLACK);
  gfx_setCursor(CX - (int)strlen(buf) * 6, LCD_HEIGHT - 22);
  gfx_puts(buf);

  lcd_show_frame();
}

/*
 * void clear_result_banner(void)
 *
 * @brief  Limpia la franja inferior donde se muestra el resultado, para
 *         preparar un nuevo giro.
 * @return void.
 */
void clear_result_banner(void) {
  gfx_fillRect(0, LCD_HEIGHT - 28, LCD_WIDTH, 28, LCD_BLACK);
}

/*
 * void spin_roulette(int win)
 *
 * @brief  Anima la flecha desde su posición actual hasta detenerse exactamente
 *         en el centro del sector ganador. Usa una función de suavizado
 *         desacelerante (ease-out) sobre la base de tiempo de SysTick, de modo
 *         que la velocidad angular llega a cero justo al final del giro.
 * @param  win: índice del sector ganador donde debe detenerse la flecha.
 * @return void.
 */
void spin_roulette(int win) {
  const float step = TWO_PI / NUM_SECTORS;

  // Ángulo objetivo: centro del sector ganador
  float target = (win + 0.5f) * step;

  // Distancia angular total: varias vueltas completas + el resto hasta el
  // objetivo (medido hacia adelante desde la posición actual).
  float delta = fmodf(target - arrow_theta, TWO_PI);
  if (delta < 0.0f)
    delta += TWO_PI;
  float total = SPIN_TURNS * TWO_PI + delta;

  float start_theta = arrow_theta;
  uint32_t t0 = mtime();
  uint32_t elapsed = 0;

  while (elapsed < SPIN_MS) {
    elapsed = mtime() - t0;
    float p = (float)elapsed / (float)SPIN_MS;
    if (p > 1.0f)
      p = 1.0f;

    // Ease-out cuadrático: la velocidad decrece hasta 0 al terminar
    float eased = 1.0f - (1.0f - p) * (1.0f - p);
    arrow_theta = start_theta + total * eased;

    draw_arrow(arrow_theta);
    lcd_show_frame();

    // Limita la tasa de cuadros para una animación uniforme
    uint32_t frame_mark = mtime();
    while (mtime() - frame_mark < FRAME_MS)
      ;
  }

  // Asegura la posición final exacta en el centro del sector
  arrow_theta = target;
  draw_arrow(arrow_theta);
  lcd_show_frame();
}

/*
 * void drain_touch_fifo(void)
 *
 * @brief  Vacía el FIFO de la pantalla táctil leyendo todas las muestras
 *         pendientes (X, Y, Z). Para esta aplicación solo interesa saber que
 *         hubo un toque, no la coordenada exacta.
 * @return void.
 */
void drain_touch_fifo(void) {
  uint16_t data_x = 0, data_y = 0, data_z = 0;
  uint8_t fifo = tft_get_fifo_size();
  while (fifo > 0) {
    tft_get_coord_data_access(X_COORD, &data_x);
    tft_get_coord_data_access(Y_COORD, &data_y);
    tft_get_coord_data_access(Z_COORD, &data_z);
    fifo = tft_get_fifo_size();
  }
}

/*
 * int main(void)
 *
 * @brief  Punto de entrada. Inicializa los periféricos, dibuja la ruleta y
 *         entra en el bucle principal: ante un toque, el RNG escoge un sector y
 *         la flecha gira hasta detenerse en él, mostrando el resultado.
 * @return No retorna (bucle infinito).
 */
int main(void) {
  system_init();
  sdram_init();
  lcd_spi_init();
  tft_setup();

  gfx_init(lcd_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
  gfx_fillScreen(LCD_BLACK);

  // Título superior
  gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
  gfx_setTextSize(LCD_TEXT_MID_SIZE);
  gfx_setCursor(CX - 36, 12);
  gfx_puts("RULETA");

  // Render inicial del anillo y la flecha en reposo
  draw_wheel();
  draw_arrow(arrow_theta);

  // Mensaje de instrucción inicial
  gfx_setTextSize(LCD_TEXT_MIN_SIZE);
  gfx_setTextColor(LCD_WHITE, LCD_BLACK);
  gfx_setCursor(CX - 48, LCD_HEIGHT - 20);
  gfx_puts("Toca para girar");
  lcd_show_frame();

  // armed = true cuando la pantalla no está siendo tocada; evita que un mismo
  // toque sostenido dispare varios giros seguidos.
  bool armed = true;

  while (true) {
    bool touched = tft_is_touch_detected();
    drain_touch_fifo();

    if (touched && armed) {
      armed = false;

      // El RNG por hardware escoge el sector ganador (32 bits verdaderos)
      uint32_t rnd = rng_get_random_blocking();
      int win = (int)(rnd % NUM_SECTORS);
      printf("RNG=%lu -> sector ganador %d (%s)\n\r", (unsigned long)rnd, win,
             sector_color_name(win));

      clear_result_banner();
      lcd_show_frame();

      spin_roulette(win);
      show_result(win);
    }

    // Re-armar cuando se suelta la pantalla
    if (!touched)
      armed = true;
  }
}
