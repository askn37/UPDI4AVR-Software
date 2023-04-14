/**
 * @file abort.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <util/atomic.h>
#include "sys.h"
#include "abort.h"

namespace ABORT {
  volatile uint16_t _abort_millis = 0;
  static volatile TCB_t *_abort =
  #if defined(ABORT_USE_TIMERB0)
    &TCB0;
  #elif defined(ABORT_USE_TIMERB1)
    &TCB1;
  #elif defined(ABORT_USE_TIMERB2)
    &TCB2;
  #else // fallback or defined(ABORT_USE_TIMERB3)
    &TCB3; //TCB3 fallback
  #endif
  jmp_buf* ABORT_CONTEXT = nullptr;
  jmp_buf CONTEXT;
}

/* Intrrupt handler */
#if defined(ABORT_USE_TIMERB0)
ISR(TCB0_INT_vect)
#elif defined(ABORT_USE_TIMERB1)
ISR(TCB1_INT_vect)
#elif defined(ABORT_USE_TIMERB2)
ISR(TCB2_INT_vect)
#else // fallback or defined(ABORT_USE_TIMERB3)
ISR(TCB3_INT_vect)
#endif
{
  jmp_buf *_context = ABORT::ABORT_CONTEXT;
  ABORT::_abort->INTFLAGS = TCB_CAPT_bm;
  if (_context == nullptr) return;
  if (--ABORT::_abort_millis) return;
  ABORT::_abort->CTRLA = 0;
  ABORT::ABORT_CONTEXT = nullptr;
  longjmp(*_context, 2);
}

#if defined(MAKE_USE_PORTA)
ISR(PORTA_PORT_vect)
#elif defined(MAKE_USE_PORTB)
ISR(PORTB_PORT_vect)
#elif defined(MAKE_USE_PORTC)
ISR(PORTC_PORT_vect)
#elif defined(MAKE_USE_PORTD)
ISR(PORTD_PORT_vect)
#elif defined(MAKE_USE_PORTE)
ISR(PORTE_PORT_vect)
#elif defined(MAKE_USE_PORTF)
ISR(PORTF_PORT_vect)
#else
ISR(PORTF_PORT_vect)
#endif
{
  if (bit_is_clear(MAKE_SIG_PORT.INTFLAGS, MAKE_PIN)) return;
  MAKE_SIG_PORT.INTFLAGS = _BV(MAKE_PIN);
  jmp_buf *_context = ABORT::ABORT_CONTEXT;
  PIN_CTRL(MAKE_SIG_PORT,MAKE_PIN) &= ~PORT_ISC_gm; /* change PORT_ISC_INTDISABLE_gc */
  ABORT::_abort->CTRLA = 0;
  if (_context == nullptr) return;
  ABORT::ABORT_CONTEXT = nullptr;
  longjmp(*_context, 1);
}

void ABORT::setup (void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ABORT::_abort->CTRLB = TCB_CNTMODE_INT_gc;            // periodic timer mode
    ABORT::_abort->CCMP = TIME_TRACKING_TIMER_COUNT - 1;  // overflow count : F_CPU / 1000
  }
}

void ABORT::start_timer (jmp_buf &context, uint16_t ms) {
  ABORT::_abort_millis = ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ABORT::ABORT_CONTEXT = &context;
    PIN_CTRL(MAKE_SIG_PORT,MAKE_PIN) &= ~PORT_ISC_gm; /* change PORT_ISC_INTDISABLE_gc */
    MAKE_SIG_PORT.INTFLAGS = _BV(MAKE_PIN);
    ABORT::_abort->CNT = 0;
    ABORT::_abort->INTCTRL = TCB_CAPT_bm;
    ABORT::_abort->INTFLAGS = TCB_CAPT_bm;
    ABORT::_abort->CTRLA = TCB_ENABLE_bm;
  }
}

void ABORT::stop_timer (void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ABORT::ABORT_CONTEXT = nullptr;
    ABORT::_abort->CTRLA = 0;
    PIN_CTRL(MAKE_SIG_PORT,MAKE_PIN) &= ~PORT_ISC_gm; /* change PORT_ISC_INTDISABLE_gc */
    MAKE_SIG_PORT.INTFLAGS = _BV(MAKE_PIN);
  }
  reti();   /* Restore global intrrupt flag */
}

uint16_t ABORT::timeleft (void) {
  uint16_t ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ms = ABORT::_abort_millis;
  }
  return ms;
}

jmp_buf* ABORT::abort_context (void) {
  jmp_buf* context;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    context = ABORT::ABORT_CONTEXT;
  }
  return context;
}

void ABORT::set_make_interrupt (jmp_buf &context) {
  ABORT::ABORT_CONTEXT = &context;
  PIN_CTRL(MAKE_SIG_PORT,MAKE_PIN) = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
  MAKE_SIG_PORT.INTFLAGS = _BV(MAKE_PIN);
}

// end of code
