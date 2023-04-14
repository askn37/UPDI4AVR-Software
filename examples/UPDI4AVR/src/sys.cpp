/**
 * @file sys.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "sys.h"

namespace {
  static PORT_t *port_list[] = {
    #if defined(PORTA)
    &PORTA,
    #endif
    #if defined(PORTB)
    &PORTB,
    #endif
    #if defined(PORTC)
    &PORTC,
    #endif
    #if defined(PORTD)
    &PORTD,
    #endif
    #if defined(PORTE)
    &PORTE,
    #endif
    #if defined(PORTF)
    &PORTF,
    #endif
  };
}

void SYS::setup (void) {
  cli();
  /* ATmegaXX08 */
  #if defined(__AVR_MEGA_0X__) || defined(__AVR_TINY_2X__)
  _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB,
    #if (F_CPU == 20000000L) || (F_CPU == 16000000L)
    (0)                                         /* No division on clock */
    #elif (F_CPU == 10000000L) || (F_CPU == 8000000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc)       /* Clock DIV2 */
    #elif (F_CPU == 5000000L) || (F_CPU == 4000000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_4X_gc)       /* Clock DIV4 */
    #elif (F_CPU == 2500000L) || (F_CPU == 2000000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_8X_gc)       /* Clock DIV8 */
    #elif (F_CPU == 1250000L) || (F_CPU == 1000000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_16X_gc)      /* Clock DIV16 */
    #elif (F_CPU == 625000L) || (F_CPU == 500000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_32X_gc)      /* Clock DIV32 */
    #elif (F_CPU == 312500L) || (F_CPU == 250000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_64X_gc)      /* Clock DIV64 */
    #elif (F_CPU == 3333333L) || (F_CPU == 2666666L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_6X_gc)       /* Clock DIV6 */
    #elif (F_CPU == 2000000L) || (F_CPU == 1600000L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_10X_gc)      /* Clock DIV10 (ex) */
    #elif (F_CPU == 1666666L) || (F_CPU == 1333333L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_12X_gc)      /* Clock DIV12 */
    #elif (F_CPU == 833333L) || (F_CPU == 666666L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_24X_gc)      /* Clock DIV24 */
    #elif (F_CPU == 416666L) || (F_CPU == 333333L)
    (CLKCTRL_PEN_bm | CLKCTRL_PDIV_48X_gc)      /* Clock DIV48 */
    #else
    #assert "This internal CPU clock is not supported"
    #endif
  );
  #elif defined(__AVR_DX__)
  _PROTECTED_WRITE(CLKCTRL_OSCHFCTRLA,
    #if (F_CPU == 32000000L)
    (0x0B<<2)
    #elif (F_CPU == 28000000L)
    (0x0A<<2)
    #elif (F_CPU == 24000000L)
    (0x09<<2)
    #elif (F_CPU == 20000000L)
    (0x08<<2)
    #elif (F_CPU == 16000000L)
    (0x07<<2)
    #elif (F_CPU == 12000000L)
    (0x06<<2)
    #elif (F_CPU == 8000000L)
    (0x05<<2)
    #elif (F_CPU == 4000000L)
    (0x03<<2)
    #elif (F_CPU == 3000000L)
    (0x02<<2)
    #elif (F_CPU == 2000000L)
    (0x01<<2)
    #elif (F_CPU == 1000000L)
    (0x00<<2)
    #else
    #assert "This internal CPU clock is not supported"
    #endif
  );
  #else
  #assert "This MCU family is not supported"
  #endif

  // Overwrite all pin INPUT_DISABLE
  // See datasheet: Microchip DS40002173C 86P
  #if defined(__AVR_DX__)
  PORTA.PINCONFIG = PORT_ISC_INPUT_DISABLE_gc;
  for (size_t i = 0; i < (sizeof(port_list) / sizeof(PORT_t*)); i++) {
    PORT_t *port = port_list[i];
    port->PINCTRLUPD = 0xFF;
  }
  #else
  for (size_t i = 0; i < (sizeof(port_list) / sizeof(PORT_t*)); i++) {
    PORT_t *port = port_list[i];
    for (uint8_t bit_pos = 0; bit_pos < 8; bit_pos++) {
      volatile uint8_t *pin_ctrl_reg = (volatile uint8_t *)&(port->PIN0CTRL) + bit_pos;
      *pin_ctrl_reg = PORT_ISC_INPUT_DISABLE_gc;
    }
  }
  #endif

  #if defined(HVPN_PIN_INVERT)
  PIN_CTRL(HV_STATE_PORT,HVEN_PIN) = PORT_INVEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  #endif
  HV_STATE_PORT.DIRSET = _BV(HVEN_PIN);
  SYS::hven_disable();

  PIN_CTRL(PROG_STATE_PORT,PGEN_PIN) =
  #if defined(PGEN_PIN_INVERT)
    PORT_INVEN_bm |
  #endif
    PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  PROG_STATE_PORT.DIRSET =
  PROG_STATE_PORT.OUTCLR = _BV(PGEN_PIN);
  SYS::pgen_disable();

  #if defined (UPDI_TRST_PIN)
  PIN_CTRL(UPDI_USART_PORT,UPDI_TRST_PIN) = PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  UPDI_USART_PORT.DIRCLR =
  UPDI_USART_PORT.OUTCLR = _BV(UPDI_TRST_PIN);
  SYS::trst_disable();
  #endif

  /* RTS or MAKE signal input */
  PIN_CTRL(MAKE_SIG_PORT,MAKE_PIN) = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;
  MAKE_SIG_PORT.DIRCLR = _BV(MAKE_PIN);

  #if defined(HVP_USE_OUTPUT)
    SYS::pump_timer_init();
  #endif
}

void SYS::pump_timer_init (void) {
  PIN_CTRL(HV_PWM_PORT,HVP2_PIN) = PORT_INVEN_bm | PORT_ISC_INPUT_DISABLE_gc;
  HV_PWM_PORT.DIRSET = _BV(HVP1_PIN) | _BV(HVP2_PIN);

#if defined(__AVR_TINY_2X__)
  PORTMUX.TCAROUTEA =
  #if defined(HVP_USE_PORTA)
    #if HVP1_PIN == 3 || HVP2_PIN == 3
    PORTMUX_TCA03_DEFAULT_gc |
    #endif
    #if HVP1_PIN == 4 || HVP2_PIN == 4
    PORTMUX_TCA04_DEFAULT_gc |
    #endif
    #if HVP1_PIN == 5 || HVP2_PIN == 5
    PORTMUX_TCA05_DEFAULT_gc |
    #endif
  #endif
  #if defined(HVP_USE_PORTB)
    #if HVP1_PIN == 0 || HVP2_PIN == 0
    PORTMUX_TCA00_DEFAULT_gc |
    #endif
    #if HVP1_PIN == 1 || HVP2_PIN == 1
    PORTMUX_TCA01_DEFAULT_gc |
    #endif
    #if HVP1_PIN == 2 || HVP2_PIN == 2
    PORTMUX_TCA02_DEFAULT_gc |
    #endif
    #if HVP1_PIN == 3 || HVP2_PIN == 3
    PORTMUX_TCA00_ALTERNATE_gc |
    #endif
    #if HVP1_PIN == 4 || HVP2_PIN == 4
    PORTMUX_TCA01_ALTERNATE_gc |
    #endif
    #if HVP1_PIN == 5 || HVP2_PIN == 5
    PORTMUX_TCA02_ALTERNATE_gc |
    #endif
  #endif
  #if defined(HVP_USE_PORTC)
    #if HVP1_PIN == 3 || HVP2_PIN == 3
    PORTMUX_TCA03_ALTERNATE_gc |
    #endif
    #if HVP1_PIN == 4 || HVP2_PIN == 4
    PORTMUX_TCA04_ALTERNATE_gc |
    #endif
    #if HVP1_PIN == 5 || HVP2_PIN == 5
    PORTMUX_TCA05_ALTERNATE_gc |
    #endif
  #endif
  0;
#else
  PORTMUX.TCAROUTEA =
  #if defined(HVP_USE_PORTA)
    PORTMUX_TCA0_PORTA_gc;
  #elif defined(HVP_USE_PORTB)
    PORTMUX_TCA0_PORTB_gc;
  #elif defined(HVP_USE_PORTC)
    PORTMUX_TCA0_PORTC_gc;
  #elif defined(HVP_USE_PORTD)
    PORTMUX_TCA0_PORTD_gc;
  #elif defined(HVP_USE_PORTE)
    PORTMUX_TCA0_PORTE_gc;
  #elif defined(HVP_USE_PORTF)
    PORTMUX_TCA0_PORTF_gc;
  #elif defined(HVP_USE_PORTG)
    PORTMUX_TCA0_PORTG_gc;
  #endif
#endif

  TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

  TCA0.SPLIT.CTRLB =
  #if HVP1_PIN == 0 || HVP2_PIN == 0
    TCA_SPLIT_LCMP0EN_bm |
  #endif
  #if HVP1_PIN == 1 || HVP2_PIN == 1
    TCA_SPLIT_LCMP1EN_bm |
  #endif
  #if HVP1_PIN == 2 || HVP2_PIN == 2
    TCA_SPLIT_LCMP2EN_bm |
  #endif
  #if HVP1_PIN == 3 || HVP2_PIN == 3
    TCA_SPLIT_HCMP0EN_bm |
  #endif
  #if HVP1_PIN == 4 || HVP2_PIN == 4
    TCA_SPLIT_HCMP1EN_bm |
  #endif
  #if HVP1_PIN == 5 || HVP2_PIN == 5
    TCA_SPLIT_HCMP2EN_bm |
  #endif
    0;

  #if HVP1_PIN < 3 || HVP2_PIN < 3
  TCA0.SPLIT.LPER =
  #endif
  #if HVP1_PIN >= 3 || HVP2_PIN >= 3
  TCA0.SPLIT.HPER =
  #endif
    HV_PWM_CLK - 1;

  #if HVP1_PIN == 0 || HVP2_PIN == 0
  TCA0.SPLIT.LCMP0 =
  #endif
  #if HVP1_PIN == 1 || HVP2_PIN == 1
  TCA0.SPLIT.LCMP1 =
  #endif
  #if HVP1_PIN == 2 || HVP2_PIN == 2
  TCA0.SPLIT.LCMP2 =
  #endif
  #if HVP1_PIN == 3 || HVP2_PIN == 3
  TCA0.SPLIT.HCMP0 =
  #endif
  #if HVP1_PIN == 4 || HVP2_PIN == 4
  TCA0.SPLIT.HCMP1 =
  #endif
  #if HVP1_PIN == 5 || HVP2_PIN == 5
  TCA0.SPLIT.HCMP2 =
  #endif
    HV_PWM_CLK / 2 - 1;

  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV4_gc;
}

// end of code
