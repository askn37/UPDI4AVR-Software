/**
 * @file mcu_model.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#undef AVR_NANO_4809_328MODE

/* GENERIC MACRO */

#define CONCAT(X,Y) X##Y
#define CONCAT3(X,Y,Z) X##Y##Z
#define PIN_CTRL(X,Y) X.CONCAT3(PIN,Y,CTRL)

#include <avr/io.h>

/***
 * Supporeted specs model
 *
 * UART  : 2+   (JTAG + UPDI half-duplex single wired)
 *       : (+1) (DEBUG output console)
 * FLASH : 16KB+
 * SRAM  : 2KB+
 * F_CPU : 8Mhz+
 *
 * unsupported : ATmega808/809 (FLASH 8KB / SRAM 1KB)
 */

#if ( \
     defined(__AVR_ATmega4808__) \
  || defined(__AVR_ATmega3208__) \
  || defined(__AVR_ATmega1608__) \
  || defined(__AVR_ATmega808__) \
  )
  #define __AVR_MEGA_TINY__
  #define __AVR_MEGA_0X__
  #define __AVR_MEGA_08__
#elif ( \
     defined(__AVR_ATmega4809__) \
  || defined(__AVR_ATmega3209__) \
  || defined(__AVR_ATmega1609__) \
  || defined(__AVR_ATmega809__) \
  )
  #define __AVR_MEGA_TINY__
  #define __AVR_MEGA_0X__
  #define __AVR_MEGA_09__
#elif ( \
     defined(__AVR_AVR32DA28__) \
  || defined(__AVR_AVR32DA32__) \
  || defined(__AVR_AVR32DA48__) \
  || defined(__AVR_AVR32DA64__) \
  || defined(__AVR_AVR64DA28__) \
  || defined(__AVR_AVR64DA32__) \
  || defined(__AVR_AVR64DA48__) \
  || defined(__AVR_AVR64DA64__) \
  || defined(__AVR_AVR128DA28__) \
  || defined(__AVR_AVR128DA32__) \
  || defined(__AVR_AVR128DA48__) \
  || defined(__AVR_AVR128DA64__) \
  )
  #define __AVR_DX__
  #define __AVR_DA__
#elif ( \
     defined(__AVR_AVR32DB28__) \
  || defined(__AVR_AVR32DB32__) \
  || defined(__AVR_AVR32DB48__) \
  || defined(__AVR_AVR32DB64__) \
  || defined(__AVR_AVR64DB28__) \
  || defined(__AVR_AVR64DB32__) \
  || defined(__AVR_AVR64DB48__) \
  || defined(__AVR_AVR64DB64__) \
  || defined(__AVR_AVR128DB28__) \
  || defined(__AVR_AVR128DB32__) \
  || defined(__AVR_AVR128DB48__) \
  || defined(__AVR_AVR128DB64__) \
  || defined(__AVR_AVR16DD28__) \
  || defined(__AVR_AVR16DD32__) \
  || defined(__AVR_AVR32DD28__) \
  || defined(__AVR_AVR32DD32__) \
  || defined(__AVR_AVR64DD28__) \
  || defined(__AVR_AVR64DD32__) \
  )
  #define __AVR_DX__
  #define __AVR_DB__
  #define __AVR_DD__
#elif ( \
     defined(__AVR_ATtiny824__) \
  || defined(__AVR_ATtiny826__) \
  || defined(__AVR_ATtiny827__) \
  || defined(__AVR_ATtiny1624__) \
  || defined(__AVR_ATtiny1626__) \
  || defined(__AVR_ATtiny1627__) \
  || defined(__AVR_ATtiny3224__) \
  || defined(__AVR_ATtiny3226__) \
  || defined(__AVR_ATtiny3227__) \
  )
  #define __AVR_MEGA_TINY__
  #define __AVR_TINY_2X__
#else
  #assert This MCU family is not supported
  #include BUILD_STOP
#endif

// end of code
