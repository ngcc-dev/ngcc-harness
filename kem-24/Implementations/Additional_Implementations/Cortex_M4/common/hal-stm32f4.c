#include "hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rng.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/dwt.h>


/* 24 MHz */
const struct rcc_clock_scale benchmarkclock = {
  .pllm = 8, //VCOin = HSE / PLLM = 1 MHz
  .plln = 192, //VCOout = VCOin * PLLN = 192 MHz
  .pllp = 8, //PLLCLK = VCOout / PLLP = 24 MHz (low to have 0WS)
  .pllq = 4, //PLL48CLK = VCOout / PLLQ = 48 MHz (required for USB, RNG)
  .hpre = RCC_CFGR_HPRE_DIV_NONE,
  .ppre1 = RCC_CFGR_PPRE_DIV_2,
  .ppre2 = RCC_CFGR_PPRE_DIV_NONE,
  .flash_config = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_LATENCY_0WS,
  .apb1_frequency = 12000000,
  .apb2_frequency = 24000000,
};

static void clock_setup(const enum clock_mode clock)
{
  switch(clock)
  {
    case CLOCK_BENCHMARK:
      rcc_clock_setup_hse_3v3(&benchmarkclock);
      break;
    case CLOCK_FAST:
    default:
      rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
      break;
  }

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_USART2);
  rcc_periph_clock_enable(RCC_DMA1);
  rcc_periph_clock_enable(RCC_RNG);

  flash_prefetch_enable();
}

static void gpio_setup(void)
{
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
  gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
}
static void usart_setup(int baud)
{
  usart_set_baudrate(USART2, baud);
  usart_set_databits(USART2, 8);
  usart_set_stopbits(USART2, USART_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX_RX);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  usart_enable(USART2);
}

static void dwt_setup(void)
{
  dwt_enable_cycle_counter();
}

static void send_USART_str(const char* in)
{
  int i;
  for(i = 0; in[i] != 0; i++) {
    usart_send_blocking(USART2, *(unsigned char *)(in+i));
  }
  usart_send_blocking(USART2, '\r');
  usart_send_blocking(USART2, '\n');
}
void hal_setup(const enum clock_mode clock)
{
  clock_setup(clock);
  gpio_setup();
  usart_setup(115200);
  dwt_setup();
  rng_enable();
}
void hal_send_str(const char* in)
{
  send_USART_str(in);
}

uint64_t hal_get_time()
{
  static uint32_t last;
  static uint64_t high;
  uint32_t now = dwt_read_cycle_counter();

  if (now < last) {
    high += 1ULL << 32;
  }
  last = now;

  return high | now;
}
