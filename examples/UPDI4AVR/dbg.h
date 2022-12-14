/**
 * @file dbg.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <string.h>
#include "configuration.h"

#ifdef DEBUG_USE_USART
namespace DBG {
  void setup (void);
  void write (uint8_t data);
  void write (const char *data);
  void write (const uint8_t *data, size_t len, bool ashex);
  void write_hex (uint8_t data);
  void newline (void);
  void print (const char *str, bool newline = true);
  void print (const uint8_t *str, size_t len, bool newline = false, bool ashex = false);
  void print_dec (const uint32_t data);
  void print_hex (const uint32_t data);
  void dump (const uint8_t *data, size_t len);
  void hexlist (const uint8_t *data, size_t len);
}
#endif  /* DEBUG_USE_USART */

// end of code
