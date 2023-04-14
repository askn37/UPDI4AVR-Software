/**
 * @file UPDI4AVR.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "configuration.h"
#include "JTAG2.h"
#include "UPDI.h"
#include "sys.h"
#include "timer.h"
#include "abort.h"
#include "dbg.h"

// Prototypes
namespace {
  void setup (void);
  void loop (void);
  bool process_command (void);
}

namespace {
  inline void setup (void) {
    SYS::setup();
    TIMER::setup();

    /* global interrupt enable */
    sei();

    #ifdef DEBUG_USE_USART
    DBG::setup();
    DBG::print("<<< UPDI4AVR >>>");
    DBG::print("F_CPU:");
    DBG::print_dec(F_CPU);
    #endif

    ABORT::setup();
    UPDI::setup();
    JTAG2::setup();
    ABORT::stop_timer();

    // #define DEBUG_PWM
    #if defined(DEBUG_PWM)
    SYS::pgen_enable();
    SYS::hvp_enable();
    if (setjmp(ABORT::CONTEXT) == 0) {
      ABORT::set_make_interrupt(ABORT::CONTEXT);
      while (1) {
        TIMER::delay(2000);
        SYS::hven_disable();
        TIMER::delay(2000);
        SYS::hven_enable();
        #ifdef DEBUG_USE_USART
        DBG::newline();
        DBG::print_dec(TIMER::millis());
        #endif
      }
    }
    else {
      SYS::pgen_disable();
      _PROTECTED_WRITE(RSTCTRL.SWRR,
        #if defined(RSTCTRL_SWRE_bm)
        RSTCTRL_SWRE_bm
        #elif defined(RSTCTRL_SWRST_bm)
        RSTCTRL_SWRST_bm
        #else
        #assert "This RSTCTRL defined is not supported"
        #endif
      );
    }
    #endif

    SYS::trst_enable();
    for (uint8_t i = 0; i < 2; i++) {
      SYS::pgen_enable();
      TIMER::delay(100);
      SYS::pgen_disable();
      TIMER::delay(100);
    }

    UPDI::runtime(UPDI::UPDI_CMD_TARGET_RESET);
    SYS::pgen_disable();
  }

  inline void loop (void) {
    static uint8_t sig_interrupt = 0;
    volatile uint8_t abort_result;

    /* sensing JTAG2 recieve */
    if ((abort_result = setjmp(ABORT::CONTEXT)) == 0) {
      if (sig_interrupt == 1) ABORT::start_timer(ABORT::CONTEXT, MAKE_SIGNAL_DELAY_MS);
      ABORT::set_make_interrupt(ABORT::CONTEXT);

      while (!JTAG2::packet_receive());
      sig_interrupt = 2;

      /* run JTAG2 command */
      if (!process_command()) sig_interrupt = 0;
    }

    /* RTS/DTR signal abort */
    else if (abort_result == 1) {
      ABORT::stop_timer();

      /* 1st RTS Signal */
      if (sig_interrupt == 0) {
        SYS::pgen_enable();
        sig_interrupt = 1;

        UPDI::CONTROL = JTAG2::CONTROL = 0x0;
        JTAG2::sign_off();
        JTAG2::change_baudrate(false);

        #ifdef DEBUG_USE_USART
        DBG::print("!MAKE");
        #endif

        UPDI::runtime(UPDI::UPDI_CMD_ENTER);
      }

      /* 2nd RTS Signal */
      else if (sig_interrupt == 1) {
        SYS::trst_enable();
        SYS::trst_disable();
        UPDI::runtime(UPDI::UPDI_CMD_TARGET_RESET);
        SYS::pgen_disable();
        sig_interrupt = 0;

        #ifdef DEBUG_USE_USART
        DBG::print("!BOOT");
        #endif

        for (uint8_t i = 0; i < 4; i++) {
          UPDI::tdir_push();
          TIMER::delay(100);
          UPDI::tdir_pull();
          TIMER::delay(100);
        }
      }
    }

    /* timeout abort */
    /* abort_result == 2 */
    else {
      ABORT::stop_timer();

      /* interrupt ? */
      if (sig_interrupt == 1) {
        sig_interrupt = 0;
        if (bit_is_clear(MAKE_SIG_PORT.IN, MAKE_PIN)) {
          _PROTECTED_WRITE(RSTCTRL.SWRR,
            #if defined(RSTCTRL_SWRE_bm)
            RSTCTRL_SWRE_bm
            #elif defined(RSTCTRL_SWRST_bm)
            RSTCTRL_SWRST_bm
            #else
            #assert "This RSTCTRL defined is not supported"
            #endif
          );
        }
        SYS::trst_enable();
        SYS::trst_disable();
        UPDI::runtime(UPDI::UPDI_CMD_TARGET_RESET);
        SYS::pgen_disable();
      }

      /* cleanup */
      if (JTAG2::CONTROL & JTAG2::HOST_SIGN_ON) {
        #ifdef DEBUG_USE_USART
        DBG::print("!H_TO");  // HOST timeout
        #endif
        JTAG2::set_response(JTAG2::RSP_FAILED);
        JTAG2::answer_transfer();
      }
      JTAG2::sign_off();
      if (JTAG2::CONTROL & JTAG2::CHANGE_BAUD) {
        JTAG2::change_baudrate(false);
      }
      JTAG2::answer_after_change();
      JTAG2::CONTROL = UPDI::CONTROL = sig_interrupt = 0;
      UPDI::tdir_pull();
      SYS::pgen_disable();
      SYS::trst_disable();
    }
  }

  /* JTAG2 command to UPDI action convert */
  inline bool process_command (void) {
    #ifdef DEBUG_USE_USART
    DBG::print("%");
    DBG::print_dec(JTAG2::packet.number);
    #endif
    switch (JTAG2::packet.body[0]) {
      case JTAG2::CMND_SIGN_OFF : {
        ABORT::stop_timer();
        #ifdef DEBUG_USE_USART
        DBG::print(">CMND_SIGN_OFF", false);
        #endif
        JTAG2::sign_off();
        JTAG2::set_response(JTAG2::RSP_OK);
        break;
      }
      case JTAG2::CMND_GET_SIGN_ON : {
        #ifdef DEBUG_USE_USART
        DBG::print(">GET_SIGN_ON", false);
        #endif
        if (!JTAG2::sign_on()) {
          ABORT::stop_timer();
        }
        break;
      }
      case JTAG2::CMND_SET_PARAMETER : {
        #ifdef DEBUG_USE_USART
        DBG::print(">SET_P", false);
        #endif
        JTAG2::set_parameter(); break;
      }
      case JTAG2::CMND_GET_PARAMETER : {
        #ifdef DEBUG_USE_USART
        DBG::print(">GET_P", false);
        #endif
        JTAG2::get_parameter(); break;
      }
      case JTAG2::CMND_SET_DEVICE_DESCRIPTOR : {
        #ifdef DEBUG_USE_USART
        DBG::print(">SET_DEV", false);
        #endif
        JTAG2::set_device_descriptor(); break;
      }
      case JTAG2::CMND_ENTER_PROGMODE : {
        #ifdef DEBUG_USE_USART
        DBG::print(">E_PRG", false);
        #endif
        JTAG2::set_response(
          UPDI::runtime(UPDI::UPDI_CMD_ENTER_PROG)
          ? JTAG2::RSP_OK
          : JTAG2::RSP_ILLEGAL_MCU_STATE
        ); break;
      }
      case JTAG2::CMND_READ_MEMORY : {
        #ifdef DEBUG_USE_USART
        DBG::print(">R_MEM", false);
        #endif
        if (UPDI::check_sig()) break;
        if (!UPDI::runtime(UPDI::UPDI_CMD_READ_MEMORY)) {
          JTAG2::set_response(JTAG2::RSP_ILLEGAL_MCU_STATE);
        }
        break;
      }
      case JTAG2::CMND_WRITE_MEMORY : {
        #ifdef DEBUG_USE_USART
        DBG::print(">W_MEM", false);
        #endif
        JTAG2::set_response(
          UPDI::runtime(UPDI::UPDI_CMD_WRITE_MEMORY)
          ? JTAG2::RSP_OK
          : JTAG2::RSP_ILLEGAL_MEMORY_TYPE
        ); break;
      }
      case JTAG2::CMND_XMEGA_ERASE : {
        #ifdef DEBUG_USE_USART
        DBG::print(">X_ERA", false);
        DBG::print(" ET=", false);
        DBG::write_hex(JTAG2::packet.body[1]);
        DBG::print(" SA=", false);
        DBG::print_hex(*((uint32_t*)&JTAG2::packet.body[2]));
        #endif
        JTAG2::set_response(
          UPDI::runtime(UPDI::UPDI_CMD_ERASE)
          ? JTAG2::RSP_OK
          : JTAG2::RSP_ILLEGAL_MCU_STATE
        ); break;
      }

      /* no support command, dummy response, all ok */
      case JTAG2::CMND_GET_SYNC :
      case JTAG2::CMND_RESET :
      case JTAG2::CMND_GO :
      case JTAG2::CMND_LEAVE_PROGMODE : {
        #ifdef DEBUG_USE_USART
        if (JTAG2::packet.body[0] == JTAG2::CMND_RESET) {
          DBG::print(">RST=", false);
          DBG::write_hex(JTAG2::packet.body[1]);
        }
        else if (JTAG2::packet.body[0] == JTAG2::CMND_GET_SYNC)       DBG::print(">GET_SYNC", false);
        else if (JTAG2::packet.body[0] == JTAG2::CMND_GO)             DBG::print(">GO", false);
        else if (JTAG2::packet.body[0] == JTAG2::CMND_LEAVE_PROGMODE) DBG::print(">L_PRG", false);
        #endif
        JTAG2::set_response(JTAG2::RSP_OK); break;
      }

      /* undefined command ignore, not response */
      default : {
        #ifdef DEBUG_USE_USART
        DBG::print(">!?", false);
        DBG::write_hex(JTAG2::packet.body[0]);
        #endif
        return true;
      }
    }

    /* JTAG2 command response */
    JTAG2::answer_transfer();

    /* response after change action */
    return JTAG2::answer_after_change();
  }
}

int main (void) {
  setup();
  for (;;) loop();
}

// end of code
