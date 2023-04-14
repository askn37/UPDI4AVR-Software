/**
 * @file timer.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <util/atomic.h>
#include "timer.h"

/* Local objects */
namespace {
  volatile uint16_t _timer_millis = 0;
  static volatile TCB_t *_timer =
  #if defined(MILLIS_USE_TIMERB0)
    &TCB0;
  #elif defined(MILLIS_USE_TIMERB1)
    &TCB1;
  #elif defined(MILLIS_USE_TIMERB2)
    &TCB2;
  #else // fallback or defined(MILLIS_USE_TIMERB3)
    &TCB3; //TCB3 fallback
  #endif
}

/* Intrrupt handler */
#if defined(MILLIS_USE_TIMERB0)
ISR(TCB0_INT_vect)
#elif defined(MILLIS_USE_TIMERB1)
ISR(TCB1_INT_vect)
#elif defined(MILLIS_USE_TIMERB2)
ISR(TCB2_INT_vect)
#else // fallback or defined(MILLIS_USE_TIMERB3)
ISR(TCB3_INT_vect)
#endif
{
  _timer_millis++;
  _timer->INTFLAGS = TCB_CAPT_bm;
}

void TIMER::setup (void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    /* MILLIS PEIOD TIMER SUPPORT */
    _timer->CTRLB = TCB_CNTMODE_INT_gc;           // periodic timer mode
    _timer->CCMP = TIME_TRACKING_TIMER_COUNT - 1; // overflow count : F_CPU / 1000
    _timer->INTCTRL |= TCB_CAPT_bm;               // interrupt capture
    #if TIME_TRACKING_TIMER_DIVIDER == 1
      _timer->CTRLA = 0;                          /* F_CPU */
    #elif TIME_TRACKING_TIMER_DIVIDER == 2
      _timer->CTRLA = CLKCTRL_PDIV0_bm;           /* F_CPU/2 */
    #else
    #assert "TIME_TRACKING_TIMER_DIVIDER not supported"
    #endif
    _timer->CTRLA |= TCB_ENABLE_bm;
  }
}

uint16_t TIMER::millis (void) {
  uint16_t ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ms = _timer_millis;
  }
  return ms;
}

uint16_t TIMER::micros (void) {
  uint32_t ms, tc;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ms = (uint32_t) _timer_millis;
    tc = (uint32_t) _timer->CNT;   // ticks (0) .. (TIME_TRACKING_TIMER_COUNT - 1)
    if (bit_is_set(_timer->INTFLAGS, TCB_CAPT_bp)) {
      ms++;
      tc = _timer->CNT;
    }
  }
#if   (F_CPU == 64000000L)
  return ms * 1000 + (tc >> 6);
#elif (F_CPU == 32000000L)
  return ms * 1000 + (tc >> 5);
#elif (F_CPU == 16000000L)
  return ms * 1000 + (tc >> 4);
#elif (F_CPU == 8000000L)
  return ms * 1000 + (tc >> 3);
#elif (F_CPU == 4000000L)
  return ms * 1000 + (tc >> 2);
#elif (F_CPU == 2000000L)
  return ms * 1000 + (tc >> 1);
#elif (F_CPU == 1000000L)
  return ms * 1000 + tc;
#elif (F_CPU > 1000000L)
  return ms * 1000 + (((uint32_t)tc * ((65536000000L + F_CPU - 1) / F_CPU)) >> 16);
#else
  return ms * 1000 + ((uint32_t)tc * 1000 / (uint16_t)TIME_TRACKING_TIMER_COUNT);
#endif
}

void TIMER::delay (uint16_t ms) {
  uint16_t start_time = micros();
  while (ms > 0) {
    /* yield(); */
    while (ms > 0 && (micros() - start_time) >= 1000) {
      ms--;
      start_time += 1000;
    }
  }
}

void TIMER::delay_us (uint16_t us) {
#if F_CPU >= 24000000L
  /* max 10923 */
  if (!us) return;
  us *= 6;
  us -= 5;
#elif F_CPU >= 20000000L
  __asm__ __volatile__(
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop");
  if (us <= 1) return;
  us = (us << 2) + us;
  us -= 7;
#elif F_CPU >= 16000000L
  /* max 16384 */
  if (us <= 1) return;
  us <<= 2;
  us -= 5;
#elif F_CPU >= 12000000L
  /* max 21846 */
  if (us <= 1) return;
  us = (us << 1) + us;
  us -= 5;
#elif F_CPU >= 10000000L
  /* max 13108 */
  if (us <= 1) return;
  us = (us << 1) + (us >> 1);
  us -= 5;
#elif F_CPU >= 8000000L
  /* max 32769 */
  if (us <= 2) return;
  us <<= 1;
  us -= 4;
#elif F_CPU >= 5000000L
  /* max 52428 */
  if (us <= 3) return;
  us += (us >> 2);
  us -= 2;
#elif F_CPU >= 4000000L
  /* max 65535 */
  if (us <= 2) return;
  us -= 2;
#elif F_CPU >= 2000000L
  if (us <= 13) return;
  us -= 11;
  us >> 1;
#else
  if (us <= 16) return;
  if (us <= 25) return;
  us -= 22;
  us >>= 2;
#endif
  __asm__ __volatile__(
      "1: sbiw %0,1" "\n\t"
      "brne 1b"
      : "=w"(us)
      : "0"(us)
  );
}

// end of code
