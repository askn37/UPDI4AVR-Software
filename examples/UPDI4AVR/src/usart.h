/**
 * @file usart.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <util/atomic.h>
#include "../configuration.h"

namespace USART {
  uint16_t calc_baudrate (uint32_t baudrate);
  uint16_t calc_baudrate_synchronous (uint32_t baudrate);
  void change_baudrate (volatile USART_t *hwserial_module, uint32_t baudrate);
  void setup (volatile USART_t *hwserial_module, uint16_t boud, uint8_t ctrl_a, uint8_t ctrl_b, uint8_t ctrl_c);
}

// end of code
