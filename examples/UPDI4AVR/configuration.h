/**
 * @file configuration.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "mcu_model.h"

/**************************************
 * PLEASE SELECT TARGET BOARAD PINOUT *
 **************************************/

#if defined(ARDUINO_AVR_UNO_WIFI_REV2)    /* ATmega4809 */
  #define BOARD_ARUDUINO_UNO_WIFI_R2
#elif defined(ARDUINO_AVR_NANO_EVERY)     /* ATmega4809 */
  #define BOARD_ARUDUINO_NANO_EVERY
#elif defined(__AVR_TINY_2X__)            /* ATtiny824 etc */
  #define BOARD_ZTINYAVR_2X
#else /* OTHER */                         /* AVR32DA32 etc */
  #define BOARD_AVRDX
#endif

/***********************
 * Using Signal Option *
 * *********************/

/* MAKE signal input PF6/RESET or other PIN */
#define ENABLE_MAKE_SIGNAL_OVER_RESET

/**************************
 * DEBUG mode using USART *
 **************************/

#ifndef DEBUG
  // #define DEBUG
#endif

/*************************
 * DEBUG mode sub-module *
 *************************/

#ifdef DEBUG
  /* enable dump memory sub-module debug */
  // #define DEBUG_DUMP_MEMORY

  /* enable dump descriptor sub-module debug */
  // #define DEBUG_DUMP_DESCRIPTOR

  /* enable UPDI tx/rx sub-module debug */
  // #define DEBUG_UPDI_LOOPBACK

  #define DEBUG_USART_BAUDRATE (230400L / 4)
#endif

/*****************
 * SELECT TIMING *
 *****************/

/* UPDI default speed: 225000L */
/* supported rabge : 235000L ~ 45000L */
#define UPDI_USART_BAUDRATE (225000L / 1)
#define UPDI_ABORT_MS 1200
#define JTAG_ABORT_MS 12000
#define HVP_ENABLE_DELAY_US 800
#define HVP_STARTUP_DELAY_MS 0
#define MAKE_SIGNAL_DELAY_MS 600

/*********************
 * HARDWARE SETTINGS *
 *********************/

/*-------------------------------------*
 *                                     *
 * ATtiny824/1624/3224 etc (tinyAVR-2) *
 *                                     *
 *-------------------------------------*/

#if defined(BOARD_ZTINYAVR_2X)

  /***
   * Using Zinnia Tinya bords pinout
   *
   * ATtiny824/1624/3224 etc (tinyAVR-2)
   *
   * Using PORTA PORTB
   *
   *  JTTX : PA1 --> Onboard CH340N RX / target MCU RX
   *  JTRX : PA2 <-- Onboard CH340N TX / target MCU TX
   *  HVP1 : PA4 --> Charge pump drive 1 (optional)
   *  HVP2 : PA5 --> Charge pump drive 2 (optional)
   *  HVEN : PA6 --> HV output enabler    : LOW:Disable, HIGH:Enable
   *  PGEN : PA7 --> Programing state     : LOW:Disable, HIGH:Enable
   *  TDIR : PB0 --> UPDI direction state : LOW:Pull, HIGH:Push
   *  TDAT : PB2 <-> UPDI to target MCU   : one-wire transfer/receiver
   *  TRST : PB3 --> Target /RESET control (optional)
   *  MAKE : PA3 <-- Reset client signal  : LOW:Active, HIGH:Deactive
   */

  /* Disable DEBUG mode */
  #undef DEBUG

  #define PGEN_USE_PORTA
  #define PGEN_PIN 7
  // #define PGEN_PIN_INVERT

  #define HVEN_USE_PORTA
  #define HVEN_PIN 6
  // #define HVPN_PIN_INVERT

  /* HV pin group using TCA0 split timer */
  #define HVP_USE_OUTPUT
  #define HVP_USE_PORTA
  #define HVP1_PIN 4
  #define HVP2_PIN 5

  #define MAKE_USE_PORTA
  #define MAKE_PIN 3

  #define UPDI_USART_MODULE USART0
  #define UPDI_USART_PORT PORTB
  #define UPDI_TDAT_PIN 2
  #define UPDI_TDAT_PIN_PULLUP
  #define UPDI_TDIR_PIN 0
  // #define UPDI_TDIR_PIN_INVERT
  #define UPDI_TRST_PIN 3

  #define JTAG_USART_MODULE USART1
  #define JTAG_USART_PORT PORTA
  #define JTAG_JTTX_PIN 1
  #define JTAG_JTRX_PIN 2

  #define ABORT_USE_TIMERB1
  #define MILLIS_USE_TIMERB0

#endif  /* defined(BOARD_ZTINYAVR_2X) */

/*-----------------------------*
 *                             *
 * ATmega4808, AVR128DB32, etc *
 *                             *
 *-----------------------------*/

#if defined(BOARD_AVRDX)

  /***
   * Using Zinnia Duino (AVRDX) bords pinout
   *
   * ATmega4808, AVR128DB32, etc
   *
   * Using PORTA PORTC PORTD PORTF
   *
   *  JTRX  : D0:PA1  <-- Onboard CH340N TX / target MCU TX
   *  JTTX  : D1:PA0  --> Onboard CH340N RX / target MCU RX
   *  DBGTX : D3:PF4  --> Outher UART RX                : use USART2 supported
   *  TDIR  : D4:PC3  --> UPDI direction state          : LOW:Pull, HIGH:Push
   *  TRST  : D6:PC1  --> Target /RESET control (optional)
   *  TDAT  : D7:PC0  <-> UPDI to target MCU            : one-wire transfer/receiver
   *  PGEN  : D10:PA7 --> Programing state              : LOW:Deactive, HIGH:Active
   *  MAKE  : RST:PF6 <-- Reset client signal (default) : LOW:Active, HIGH:Deactive (sense RTS)
   *     or : A0:PD1  <-- Reset client signal (altanate): LOW:Active, HIGH:Deactive (user switch)
   *  HVEN  : A1:PD2  --> HV output state               : LOW:Disable, HIGH:Enable
   *  HVP1  : A2:PD3  --> Charge pump drive 1 (optional)
   *  HVP2  : A3:PD4  --> Charge pump drive 2 (optional)
   */

  #define PGEN_USE_PORTA
  #define PGEN_PIN 7
  // #define PGEN_PIN_INVERT

  #define HVEN_USE_PORTD
  #define HVEN_PIN 2
  // #define HVPN_PIN_INVERT

  /* HV pin group using TCA0 split timer */
  /* using PWM pinnumber 0 ~ 5 (bad 6,7)*/
  /* and AVR DB bad D0 (not output circuit) */
  #define HVP_USE_OUTPUT
  #define HVP_USE_PORTD
  #define HVP1_PIN 3
  #define HVP2_PIN 4

  /* Sense RTS/DTR signal input intrrupt */
  #ifdef ENABLE_MAKE_SIGNAL_OVER_RESET
    #define MAKE_USE_PORTF
    #define MAKE_PIN 6
  #else
    #define MAKE_USE_PORTD
    #define MAKE_PIN 1
  #endif

  #define UPDI_USART_MODULE USART1
  // #define UPDI_USART_PORTMUX (PORTMUX_USART1_DEFAULT_gc)
  #define UPDI_USART_PORT PORTC
  #define UPDI_TDAT_PIN 0
  #define UPDI_TDIR_PIN 3
  // #define UPDI_TDAT_PIN_PULLUP
  // #define UPDI_TDIR_PIN_INVERT
  #define UPDI_TRST_PIN 1

  #define JTAG_USART_MODULE USART0
  // #define JTAG_USART_PORTMUX (PORTMUX_USART0_DEFAULT_gc)
  #define JTAG_USART_PORT PORTA
  #define JTAG_JTTX_PIN 0
  #define JTAG_JTRX_PIN 1
  #ifdef DEBUG
    #ifdef USART2_BAUD
      #define DEBUG_USE_USART
      #define DEBUG_USART_MODULE USART2
      #define DEBUG_USART_PORTMUX (PORTMUX_USART2_ALT1_gc)
      #define DEBUG_USART_PORT PORTF
      #define DEBUG_DBGTX_PIN 4
    #else
      #undef DEBUG
    #endif
  #endif /* DEBUG */

  #define ABORT_USE_TIMERB1

  #undef  MILLIS_USE_TIMERB2
  #define MILLIS_USE_TIMERB2

#endif  /* defined(BOARD_AVRDX) */

/*------------------------*
 *                        *
 * Arduino UNO WiFi Rev.2 *
 *                        *
 *------------------------*/

#if defined(BOARD_ARUDUINO_UNO_WIFI_R2)

  /***
   * Using Arduino UNO WiFi Rev.2 bords pinout
   *
   * ATmega4809
   *
   * Using PORTA PORTB PORTC PORTF PORTD(or PORTE)
   *
   *  JTRX  : --:PB5  <-- onboard mEDBG TX (ATmega32U4) : NO PINOUT
   *  JTTX  : --:PB4  --> onboard mEDBG RX (ATmega32U4) : NO PINOUT
   *  DBGTX : D1:PC4  --> outher UART RX
   *  TDIR  : --:PF3  --> UPDI direction state          : LOW:Pull, HIGH:Push
   *  TDAT  : D6:PF4  <-> UPDI to target MCU            : one-wire transfer/receiver
   *  TRST  : --:PF5  --> Target /RESET control (optional)
   *  PGEN  : --:PD6  --> Programing state LED          : LOW:Disable, HIGH:Enable
   *  MAKE  : RST:PF6 <-- Reset client signal (default) : LOW:Active, HIGH:Deactive (user switch)
   *     or : A0:PD0  <-- Reset client signal (altanate): LOW:Active, HIGH:Deactive (sense RTS)
   *  HVEN  : A1:PD1  --> HV output enabler             : LOW:Disable, HIGH:Enable
   *  HVP1  : A2:PD2  --> Charge pump drive 1 (optional)
   *  HVP2  : A3:PD3  --> Charge pump drive 2 (optional)
   */
  #undef UNO_WIFI_REV2_328MODE

  #define PGEN_USE_PORTD
  #define PGEN_PIN 6
  // #define PGEN_PIN_INVERT

  #define HVEN_USE_PORTD
  #define HVEN_PIN 1
  // #define HVPN_PIN_INVERT

  /* HV pin group using TCA0 split timer */
  /* using PWM pinnumber 0 ~ 5 (bad 6,7)*/
  #define HVP_USE_OUTPUT
  #define HVP_USE_PORTD
  #define HVP1_PIN 2
  #define HVP2_PIN 3

  /* Sense RTS/DTR signal input intrrupt */
  // #define RTS_SENSE_ENABLE
  #ifdef ENABLE_MAKE_SIGNAL_OVER_RESET
    #define MAKE_USE_PORTF
    #define MAKE_PIN 6
  #else
    #define MAKE_USE_PORTD
    #define MAKE_PIN 0
  #endif

  #define UPDI_USART_MODULE USART2
  #define UPDI_USART_PORTMUX (PORTMUX_USART2_ALT1_gc)
  #define UPDI_USART_PORT PORTF
  #define UPDI_TDAT_PIN 4
  #define UPDI_TDIR_PIN 3
  // #define UPDI_TDIR_PIN_INVERT
  #define UPDI_TRST_PIN 5

  #define JTAG_USART_MODULE USART3
  #define JTAG_USART_PORTMUX (PORTMUX_USART3_ALT1_gc)
  #define JTAG_USART_PORT PORTB
  #define JTAG_JTTX_PIN 4
  #define JTAG_JTRX_PIN 5

  #ifdef DEBUG
    #define DEBUG_USE_USART
    #define DEBUG_USART_MODULE USART1
    #define DEBUG_USART_PORTMUX (PORTMUX_USART1_ALT1_gc)
    #define DEBUG_USART_PORT PORTC
    #define DEBUG_DBGTX_PIN 4
  #endif /* DEBUG */

  #define ABORT_USE_TIMERB1
  #define MILLIS_USE_TIMERB2

#endif  /* defined(BOARD_ARUDUINO_UNO_WIFI_R2) */

/*------------------------*
 *                        *
 *  Arduino Nano Every    *
 *                        *
 *------------------------*/

#if defined(BOARD_ARUDUINO_NANO_EVERY)

  /***
   * Using Arduino Nano Every bords pinout
   *
   * ATmega4809
   *
   * Using PORTA PORTB PORTC PORTF PORTD(or PORTE)
   *
   *  JTRX  : --:PB5  <-- onboard mEDBG TX (ATmega32U4) : NO PINOUT
   *  JTTX  : --:PB4  --> onboard mEDBG RX (ATmega32U4) : NO PINOUT
   *  DBGTX : D1:PC4  --> outher UART RX
   *  TDIR  : A5:PF3  --> UPDI direction state          : LOW:Pull, HIGH:Push
   *  TDAT  : D6:PF4  <-> UPDI to target MCU            : one-wire transfer/receiver
   *  PGEN  : D13:PE2 --> Programing state LED          : LOW:Disable, HIGH:Enable
   *  MAKE  : RST:PF6 <-- Reset client signal (default) : LOW:Active, HIGH:Deactive (user switch)
   *     or : A0:PD3  <-- Reset client signal (altanate): LOW:Active, HIGH:Deactive (sense RTS)
   *  HVEN  : A1:PD2  --> HV output enabler             : LOW:Disable, HIGH:Enable
   *  HVP1  : A2:PD1  --> Charge pump drive 1 (optional)
   *  HVP2  : A3:PD0  --> Charge pump drive 2 (optional)
   */
  #undef UNO_WIFI_REV2_328MODE

  #define PGEN_USE_PORTE
  #define PGEN_PIN 2
  // #define PGEN_PIN_INVERT

  #define HVEN_USE_PORTD
  #define HVEN_PIN 2
  // #define HVPN_PIN_INVERT

  /* HV pin group using TCA0 split timer */
  /* using PWM pinnumber 0 ~ 5 (bad 6,7)*/
  #define HVP_USE_OUTPUT
  #define HVP_USE_PORTD
  #define HVP1_PIN 1
  #define HVP2_PIN 0

  /* Sense RTS/DTR signal input intrrupt */
  // #define RTS_SENSE_ENABLE
  #ifdef ENABLE_MAKE_SIGNAL_OVER_RESET
    #define MAKE_USE_PORTF
    #define MAKE_PIN 6
  #else
    #define MAKE_USE_PORTD
    #define MAKE_PIN 3
  #endif

  #define UPDI_USART_MODULE USART2
  #define UPDI_USART_PORTMUX (PORTMUX_USART2_ALT1_gc)
  #define UPDI_USART_PORT PORTF
  #define UPDI_TDAT_PIN 4
  // #define UPDI_TDAT_PIN_PULLUP
  #define UPDI_TDIR_PIN 3
  // #define UPDI_TDIR_PIN_INVERT

  #define JTAG_USART_MODULE USART3
  #define JTAG_USART_PORTMUX (PORTMUX_USART3_ALT1_gc)
  #define JTAG_USART_PORT PORTB
  #define JTAG_JTTX_PIN 4
  #define JTAG_JTRX_PIN 5

  #ifdef DEBUG
    #define DEBUG_USE_USART
    #define DEBUG_USART_MODULE USART1
    #define DEBUG_USART_PORTMUX (PORTMUX_USART1_ALT1_gc)
    #define DEBUG_USART_PORT PORTC
    #define DEBUG_DBGTX_PIN 4
  #endif /* DEBUG */

  #define ABORT_USE_TIMERB1
  #define MILLIS_USE_TIMERB2

#endif  /* defined(BOARD_ARUDUINO_NANO_EVERY) */

// end of code
