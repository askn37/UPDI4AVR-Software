/**
 * @file timer.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <setjmp.h>
#include "configuration.h"

/* Ex) ATmegaXX08 : implimented TCB0, TCB1, TCB2, not impliment TCB3 */
#if !defined(MILLIS_USE_TIMERB0) \
 && !defined(MILLIS_USE_TIMERB1) \
 && !defined(MILLIS_USE_TIMERB2) \
 && !defined(MILLIS_USE_TIMERB3)
#define MILLIS_USE_TIMERB2
#endif

/* Timer F_CPU Clock divider (can be 1 or 2) */
#define TIME_TRACKING_TIMER_DIVIDER 1

/* Should correspond to exactly 1 ms, i.e. millis() */
/* max limit of UINT16_MAX */
#define TIME_TRACKING_TIMER_COUNT (F_CPU/(1000*TIME_TRACKING_TIMER_DIVIDER))

namespace TIMER {
  void setup (void);
  uint16_t millis (void);
  uint16_t micros (void);
  void delay(uint16_t ms);
  void delay_us(uint16_t us);
}

// end of code
