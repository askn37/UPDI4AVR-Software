/**
 * @file usart.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "usart.h"

uint16_t USART::calc_baudrate (uint32_t baudrate) {
  return ((((F_CPU * 8) / baudrate) + 1) >> 1);
}

uint16_t USART::calc_baudrate_synchronous (uint32_t baudrate) {
  return ((((F_CPU * 32) / (baudrate >> 2)) + 1) >> 1);
}

void USART::change_baudrate (volatile USART_t *hwserial_module, uint32_t baud_reg) {
  if (((*hwserial_module).CTRLC & USART_CMODE_MSPI_gc) == 0) {
    if (baud_reg < 128) {
      baud_reg <<= 1;
      (*hwserial_module).CTRLB |= USART_RXMODE_CLK2X_gc;
    }
    else {
      (*hwserial_module).CTRLB &= ~(USART_RXMODE_CLK2X_gc);
    }
    #if defined(__AVR_MEGA_0X__)
    int8_t sigrow_val = (FUSE.OSCCFG & FUSE_FREQSEL_gm) == 2 ? SIGROW.OSC20ERR5V : SIGROW.OSC16ERR5V;
    baud_reg += (((uint32_t)baud_reg * sigrow_val) >> 10);
    #endif
  }
  else {
    baud_reg &= ~63;
  }
  (*hwserial_module).BAUD = baud_reg;
}

void USART::setup (volatile USART_t *hwserial_module, uint16_t baud_reg, uint8_t ctrl_a, uint8_t ctrl_b, uint8_t ctrl_c) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    (*hwserial_module).CTRLA = ctrl_a;
    (*hwserial_module).CTRLC = ctrl_c;
    (*hwserial_module).CTRLB = ctrl_b;
  }
  USART::change_baudrate(hwserial_module, baud_reg);
}

// end of code
