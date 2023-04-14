/**
 * @file dbg.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "dbg.h"
#include "usart.h"

#ifdef DEBUG_USE_USART

void DBG::setup (void) {
  #ifdef DEBUG_USART_PORTMUX
  PORTMUX.USARTROUTEA |= DEBUG_USART_PORTMUX;
  #endif

  #ifdef DEBUG_USART_RX_PIN
  PIN_CTRL(DEBUG_USART_PORT,DEBUG_USART_RX_PIN) = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;
  #endif
  DEBUG_USART_PORT.DIRSET = _BV(DEBUG_DBGTX_PIN);

  USART::setup(
    &DEBUG_USART_MODULE,
    USART::calc_baudrate(DEBUG_USART_BAUDRATE),
    0x00,
    (USART_TXEN_bm),
    (USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | USART_CMODE_ASYNCHRONOUS_gc | USART_SBMODE_1BIT_gc)
  );
}

void DBG::write (uint8_t data) {
  // waiting clear flag : Data Register Empty Interrupt
  loop_until_bit_is_set(DEBUG_USART_MODULE.STATUS, USART_DREIF_bp);
  DEBUG_USART_MODULE.STATUS |= USART_TXCIF_bm;
  DEBUG_USART_MODULE.TXDATAL = data;
}

void DBG::write (const char *data) {
  DBG::write((const uint8_t*)data, strlen(data), false);
}

void DBG::write (const uint8_t *data, size_t len, bool ashex) {
  while (len--) {
    if (ashex) DBG::write_hex(*data++);
    else       DBG::write(*data++);
  }
}

void DBG::write_hex (uint8_t data) {
  uint8_t nh = (data >> 4) | '0';
  uint8_t nl = (data & 15) | '0';
  if (nh > '9') nh += 7;
  if (nl > '9') nl += 7;
  DBG::write(nh);
  DBG::write(nl);
}

void DBG::newline (void) {
  DBG::write("\x0D\x0A");
}

void DBG::print (const char *str, bool newline) {
  DBG::print((const uint8_t*)str, strlen(str), newline, false);
}

void DBG::print (const uint8_t *str, size_t len, bool newline, bool ashex) {
  if (newline) DBG::newline();
  DBG::write(str, len, ashex);
}

void DBG::print_dec (const uint32_t data) {
  uint8_t c = data % 10 | '0';
  uint32_t d = data / 10;
  if (d > 0) DBG::print_dec(d);
  DBG::write((const char)c);
}

void DBG::print_hex (const uint32_t data) {
  uint8_t c = data & 0xff;
  uint32_t d = data >> 8;
  if (d > 0) DBG::print_hex(d);
  DBG::write_hex(c);
}

void DBG::dump (const uint8_t *data, size_t len) {
  size_t _count = 0;
  while (_count < len) {
    if ((_count & 15) == 0) {
      DBG::write("\x0D\x0A");
    }
    else if ((_count & 7) == 0) {
      DBG::write(' ');
    }
    DBG::write(' ');
    DBG::write_hex(*data++);
    _count++;
  }
  DBG::write("\x0D\x0A");
}

void DBG::hexlist (const uint8_t *data, size_t len) {
  size_t cnt = 0;
  while (cnt < len) {
    DBG::write_hex(*data++);
    cnt++;
    if (cnt == len) break;
    DBG::write(':');
  }
}

#endif /* DEBUG_USE_USART */

// end of code
