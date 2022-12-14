/**
 * @file sys.h
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

#if defined(PGEN_USE_PORTA)
  #define PROG_STATE_PORT PORTA
#elif defined(PGEN_USE_PORTB)
  #define PROG_STATE_PORT PORTB
#elif defined(PGEN_USE_PORTC)
  #define PROG_STATE_PORT PORTC
#elif defined(PGEN_USE_PORTD)
  #define PROG_STATE_PORT PORTD
#elif defined(PGEN_USE_PORTE)
  #define PROG_STATE_PORT PORTE
#elif defined(PGEN_USE_PORTF)
  #define PROG_STATE_PORT PORTF
#elif defined(PGEN_USE_PORTG)
  #define PROG_STATE_PORT PORTG
#else
  /* fallback */
  #define PGEN_USE_PORTA
  #define PROG_STATE_PORT PORTA
#endif

#if defined(MAKE_USE_PORTA)
  #define MAKE_SIG_PORT PORTA
#elif defined(MAKE_USE_PORTB)
  #define MAKE_SIG_PORT PORTB
#elif defined(MAKE_USE_PORTC)
  #define MAKE_SIG_PORT PORTC
#elif defined(MAKE_USE_PORTD)
  #define MAKE_SIG_PORT PORTD
#elif defined(MAKE_USE_PORTE)
  #define MAKE_SIG_PORT PORTE
#elif defined(MAKE_USE_PORTF)
  #define MAKE_SIG_PORT PORTF
#elif defined(MAKE_USE_PORTG)
  #define MAKE_SIG_PORT PORTG
#else
  /* fallback */
  #define MAKE_USE_PORTF
  #define MAKE_SIG_PORT PORTF
#endif

#if defined(HVEN_USE_PORTA)
  #define HV_STATE_PORT PORTA
#elif defined(HVEN_USE_PORTB)
  #define HV_STATE_PORT PORTB
#elif defined(HVEN_USE_PORTC)
  #define HV_STATE_PORT PORTC
#elif defined(HVEN_USE_PORTD)
  #define HV_STATE_PORT PORTD
#elif defined(HVEN_USE_PORTE)
  #define HV_STATE_PORT PORTE
#elif defined(HVEN_USE_PORTF)
  #define HV_STATE_PORT PORTF
#elif defined(HVEN_USE_PORTG)
  #define HV_STATE_PORT PORTG
#else
  /* fallback */
  #define HVEN_USE_PORTA
  #define HV_STATE_PORT PORTA
#endif

#if defined(HVP_USE_PORTA)
  #define HV_PWM_PORT PORTA
#elif defined(HVP_USE_PORTB)
  #define HV_PWM_PORT PORTB
#elif defined(HVP_USE_PORTC)
  #define HV_PWM_PORT PORTC
#elif defined(HVP_USE_PORTD)
  #define HV_PWM_PORT PORTD
#elif defined(HVP_USE_PORTE)
  #define HV_PWM_PORT PORTE
#elif defined(HVP_USE_PORTF)
  #define HV_PWM_PORT PORTF
#elif defined(HVP_USE_PORTG)
  #define HV_PWM_PORT PORTG
#else
  /* fallback */
  #define HVEN_USE_PORTA
  #define HV_STATE_PORT PORTA
#endif

#define HV_PWM_CLK (F_CPU/400000)

namespace SYS {
  void setup (void);
  void pump_timer_init (void);

  /* JTAG tri-state buffer control */
  inline void pgen_disable (void) {
    PROG_STATE_PORT.DIRSET =
    PROG_STATE_PORT.OUTCLR = _BV(PGEN_PIN);
  }
  inline void pgen_enable (void) {
    PROG_STATE_PORT.DIRCLR = _BV(PGEN_PIN);
  }

  /* JTAG tri-state buffer control */
  inline void hven_disable (void) {
    HV_STATE_PORT.OUTCLR = _BV(HVEN_PIN);
  }
  inline void hven_enable (void) {
    HV_STATE_PORT.OUTSET = _BV(HVEN_PIN);
  }

  inline void trst_disable (void) {
    #if defined (UPDI_TRST_PIN)
    UPDI_USART_PORT.DIRCLR = _BV(UPDI_TRST_PIN);
    #endif
  }
  inline void trst_enable (void) {
    #if defined (UPDI_TRST_PIN)
    UPDI_USART_PORT.DIRSET = _BV(UPDI_TRST_PIN);
    UPDI_USART_PORT.OUTCLR = _BV(UPDI_TRST_PIN);
    #endif
  }

  inline void hvp_enable (void) {
    #if defined(HVP_USE_OUTPUT)
    TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
    #endif
  }
  inline void hvp_disable (void) {
    #if defined(HVP_USE_OUTPUT)
    TCA0.SPLIT.CTRLA &= ~(TCA_SPLIT_ENABLE_bm);
    #endif
  }
}

// end of code
