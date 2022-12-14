/**
 * @file abort.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <setjmp.h>
#include "configuration.h"
#include "timer.h"

/* Ex) ATmegaXX08 : implimented TCB0, TCB1, TCB2, not impliment TCB3 */
#if !defined(ABORT_USE_TIMERB0) \
 && !defined(ABORT_USE_TIMERB1) \
 && !defined(ABORT_USE_TIMERB2) \
 && !defined(ABORT_USE_TIMERB3)
#define ABORT_USE_TIMERB1
#endif

namespace ABORT {
  extern jmp_buf* ABORT_CONTEXT;
  extern jmp_buf CONTEXT;
  void setup (void);
  void start_timer (jmp_buf &context, uint16_t ms);
  void stop_timer (void);
  uint16_t timeleft (void);
  jmp_buf* abort_context (void);
  void set_make_interrupt (jmp_buf &context);
}

// end of code
