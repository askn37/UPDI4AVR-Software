/**
 * @file UPDI.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.3
 * @date 2023-11-28
 *
 * @copyright Copyright (c) 2023 askn37 at github.com
 *
 */
#include <setjmp.h>
#include "sys.h"
#include "UPDI.h"
#include "JTAG2.h"
#include "NVM.h"
#include "usart.h"
#include "timer.h"
#include "abort.h"
#include "dbg.h"

namespace UPDI {
  const static uint8_t nvmprog_key[10]   = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x20, 0x67, 0x6F, 0x72, 0x50, 0x4D, 0x56, 0x4E };
  const static uint8_t erase_key[10]     = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x65, 0x73, 0x61, 0x72, 0x45, 0x4D, 0x56, 0x4E };
  const static uint8_t urowwrite_key[10] = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x65, 0x74, 0x26, 0x73, 0x55, 0x4D, 0x56, 0x4E };

  volatile uint8_t LASTL; // Last read byte
  volatile uint8_t LASTH; // Last read status
  uint8_t NVMPROGVER;
  uint8_t CONTROL;
  uint8_t signature[4];
}

void UPDI::setup (void) {
  UPDI::CONTROL = 0x00;

  #ifdef UPDI_USART_PORTMUX
  PORTMUX.USARTROUTEA |= UPDI_USART_PORTMUX;
  #endif

  PIN_CTRL(UPDI_USART_PORT,UPDI_TDAT_PIN) =
    #ifdef UPDI_TDAT_PIN_PULLUP
    PORT_PULLUPEN_bm |
    #endif
    PORT_ISC_INTDISABLE_gc;

  #if defined(UPDI_TDIR_PIN)
  #if defined(UPDI_TDIR_PIN_INVERT)
  PIN_CTRL(UPDI_USART_PORT,UPDI_TDIR_PIN) |= PORT_INVEN_bm;
  #endif
  UPDI_USART_PORT.DIRSET = _BV(UPDI_TDIR_PIN);
  #endif

  USART::setup(
    &UPDI_USART_MODULE,
    USART::calc_baudrate(UPDI_USART_BAUDRATE),
    (USART_LBME_bm | 0x03),
    (USART_TXEN_bm | USART_RXEN_bm | USART_ODME_bm | USART_RXMODE_NORMAL_gc),
    (USART_CHSIZE_8BIT_gc | USART_PMODE_EVEN_gc | USART_CMODE_ASYNCHRONOUS_gc | USART_SBMODE_2BIT_gc)
  );
}

void UPDI::fallback_speed (uint32_t baudrate) {
  USART::change_baudrate(&UPDI_USART_MODULE, USART::calc_baudrate(baudrate));
  #ifdef DEBUG_USE_USART
  DBG::print("UBAUD=");
  DBG::print_dec(UPDI_USART_MODULE.BAUD);
  DBG::write(':');
  DBG::print_dec((F_CPU * 4) / UPDI_USART_MODULE.BAUD);
  #endif
}

void UPDI::drain (void) {
  uint8_t j = 0;
  do {
    if (bit_is_set(UPDI_USART_MODULE.STATUS, USART_RXCIF_bp)) {
      UPDI::LASTH = UPDI_USART_MODULE.RXDATAH;
      UPDI::LASTL = UPDI_USART_MODULE.RXDATAL;
      j = 0;
    }
  } while (--j);
}

void UPDI::BREAK (bool longbreak, bool use_hv) {
  uint16_t baud_reg = UPDI_USART_MODULE.BAUD;
  UPDI_USART_MODULE.BAUD = longbreak ? ~1 : (baud_reg << 2);
  UPDI::SEND(0x00);
  UPDI_USART_MODULE.BAUD = baud_reg;

  if (use_hv) {
    /*********************
     * Using HV sequence *
     *********************/
    #ifdef DEBUG_USE_USART
    DBG::print("[HV]", false);
    #endif
    SYS::trst_enable();
    SYS::hvp_enable();
    #if defined(HVP_STARTUP_DELAY_MS) && HVP_STARTUP_DELAY_MS > 0
    TIMER::delay(HVP_STARTUP_DELAY_MS);
    #endif
    SYS::trst_disable();
    UPDI::BREAK();
    TIMER::delay_us(5);               // UPDI:LOW  HV:Hi-Z
    SYS::hven_enable();
    TIMER::delay_us(HVP_ENABLE_DELAY_US);    // UPDI:Hi-Z HV:HIGH
    SYS::hven_disable();
    SYS::hvp_disable();
    TIMER::delay_us(5);               // UPDI:HI-Z HV:Hi-Z
    UPDI::SEND(0xFE);                 // UPDI:LOW  HV:Hi-Z
    UPDI::set_control(UPDI::HV_ENABLE);
    TIMER::delay(4);                  // UPDI:Hi-Z HV:Hi-Z
  }

  loop_until_bit_is_set(UPDI_USART_PORT.IN, UPDI_TDAT_PIN);
  UPDI::LASTH = UPDI_USART_MODULE.RXDATAH;
  UPDI::LASTL = UPDI_USART_MODULE.RXDATAL;
  UPDI_USART_MODULE.STATUS =
  UPDI_USART_MODULE.RXDATAH = 0;
}

uint8_t UPDI::RECV (void) {
  /* receive symbol */
  loop_until_bit_is_set(UPDI_USART_MODULE.STATUS, USART_RXCIF_bp);
  UPDI::LASTH = UPDI_USART_MODULE.RXDATAH ^ 0x80;

  #ifdef DEBUG_UPDI_LOOPBACK

  UPDI::LASTL = UPDI_USART_MODULE.RXDATAL;
  DBG::write('<'); DBG::write_hex(UPDI::LASTL);
  return UPDI::LASTL;

  #else

  return UPDI::LASTL = UPDI_USART_MODULE.RXDATAL;

  #endif
}

bool UPDI::SEND (const uint8_t data) {
  loop_until_bit_is_set(UPDI_USART_MODULE.STATUS, USART_DREIF_bp);

  #ifdef DEBUG_UPDI_LOOPBACK
  DBG::write('>');
  DBG::write_hex(data);
  #endif

  /* sending symbol */
  UPDI_USART_MODULE.STATUS |= USART_TXCIF_bm;
  UPDI_USART_MODULE.TXDATAL = data;
  loop_until_bit_is_set(UPDI_USART_MODULE.STATUS, USART_TXCIF_bp);

  /* loopback symbol verify */
  bool _r = data == UPDI::RECV();
  if (!_r) UPDI::LASTH |= 0x20;
  return _r;
}

size_t UPDI::send_bytes (const uint8_t *data, size_t len) {
  uint8_t *p = (uint8_t*)(void*)data;
  size_t _count = 0;
  while (len--) {
    if (!UPDI::SEND(*p++)) return false;
    _count++;
  }
  return _count;
}

bool UPDI::send_repeat_header (uint8_t cmd, uint32_t addr, size_t len) {
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_REG | UPDI::UPDI_DATA3
    , 0, 0, 0, 0  // qword address
  };
  static uint8_t set_repeat[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_REPEAT | UPDI::UPDI_DATA1
    , 0                   // repeat count
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_PTR_INC  // +cmd
  };
  for (;;) {
    *((uint32_t*)&set_ptr[2]) = addr;
    set_repeat[2] = len - 1;
    set_repeat[4] = UPDI::UPDI_PTR_INC | cmd;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)-1) != sizeof(set_ptr)-1) break;
    if (UPDI::UPDI_ACK != UPDI::RECV()) break;
    return UPDI::send_bytes(set_repeat, sizeof(set_repeat)) == sizeof(set_repeat);
  }
  return false;
}

bool UPDI::st8 (uint32_t addr, uint8_t data) {
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_STS | UPDI::UPDI_ADDR3 | UPDI::UPDI_DATA1
    , 0, 0, 0, 0  // qword address
  };
  for (;;) {
    *((uint32_t*)&set_ptr[2]) = addr;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)-1) != sizeof(set_ptr)-1) break;
    if (UPDI::UPDI_ACK != UPDI::RECV()) break;
    if (!UPDI::SEND(data)) break;
    return UPDI::UPDI_ACK == UPDI::RECV();
  }
  return false;
}

size_t UPDI::sts8 (uint32_t addr, uint8_t *data, size_t len) {
  size_t cnt = 0;
  if (UPDI::send_repeat_header((UPDI::UPDI_ST | UPDI::UPDI_DATA1), addr, len)) {
    while (cnt < len) {
      if (!UPDI::SEND(*data++)) break;
      if (UPDI::UPDI_ACK != RECV()) break;
      cnt++;
    }
  }
  return cnt;
}

uint8_t UPDI::ld8 (uint32_t addr) {
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_LDS | UPDI::UPDI_ADDR3 | UPDI::UPDI_DATA1
    , 0, 0, 0, 0  // qword address
  };
  for (;;) {
    *((uint32_t*)&set_ptr[2]) = addr;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)-1) == sizeof(set_ptr)-1) {
      return RECV();
    }
    /* fallback */
    UPDI::BREAK();
  }
}

bool UPDI::is_cs_stat (const uint8_t code, const uint8_t check) {
  static uint8_t set_ptr[] = { UPDI::UPDI_SYNCH, 0 };
  for (;;) {
    set_ptr[1] = UPDI::UPDI_LDCS | code;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)) != sizeof(set_ptr)) break;
    return check == (UPDI::RECV() & check);
  }
  return false;
}
/* inline bool is_sys_stat (const uint8_t check); // UPDI_CS_ASI_SYS_STATUS */
/* inline bool is_key_stat (const uint8_t check); // UPDI_CS_ASI_KEY_STATUS */
/* inline bool is_rst_stat (void);                // UPDI_CS_ASI_RESET_REQ */

bool UPDI::set_cs_stat (const uint8_t code, const uint8_t data) {
  static uint8_t set_ptr[] = { UPDI::UPDI_SYNCH, 0, 0 };
  set_ptr[1] = UPDI::UPDI_STCS | code;
  set_ptr[2] = data;
  return UPDI::send_bytes(set_ptr, sizeof(set_ptr)) == sizeof(set_ptr);
}
/* inline bool reset (bool logic);  // UPDI_CS_ASI_RESET_REQ, UPDI_RSTREQ */

bool UPDI::loop_until_sys_stat_is_clear (uint8_t bitmap, uint16_t limit) {
  #ifdef ENABLE_DEBUG_UPDI_SENDER
  uint16_t _back = _send_ptr;
  #endif
  do {
    if (!is_sys_stat(bitmap)) return true;
    #ifdef ENABLE_DEBUG_UPDI_SENDER
    _send_ptr = _back;
    #endif
    TIMER::delay_us(50);
  } while (--limit);
  return false;
}

bool UPDI::loop_until_sys_stat_is_set (uint8_t bitmap, uint16_t limit) {
  #ifdef ENABLE_DEBUG_UPDI_SENDER
  uint16_t _back = _send_ptr;
  #endif
  do {
    if (is_sys_stat(bitmap)) return true;
    #ifdef ENABLE_DEBUG_UPDI_SENDER
    _send_ptr = _back;
    #endif
    TIMER::delay_us(50);
  } while (--limit);
  return false;
}

bool UPDI::loop_until_key_stat_is_clear (uint8_t bitmap, uint16_t limit) {
  #ifdef ENABLE_DEBUG_UPDI_SENDER
  uint16_t _back = _send_ptr;
  #endif
  do {
    if (!is_key_stat(bitmap)) return true;
    #ifdef ENABLE_DEBUG_UPDI_SENDER
    _send_ptr = _back;
    #endif
    TIMER::delay_us(50);
  } while (--limit);
  return false;
}

bool UPDI::loop_until_key_stat_is_set (uint8_t bitmap, uint16_t limit) {
  #ifdef ENABLE_DEBUG_UPDI_SENDER
  uint16_t _back = _send_ptr;
  #endif
  do {
    if (is_key_stat(bitmap)) return true;
    #ifdef ENABLE_DEBUG_UPDI_SENDER
    _send_ptr = _back;
    #endif
    TIMER::delay_us(50);
  } while (--limit);
  return false;
}

bool UPDI::read_parameter (void) {
  uint8_t updi_sib[32];
  uint8_t *p;
  size_t len;
  for (;;) {
    #ifdef DEBUG_USE_USART
    DBG::print("(SIB)", false);
    #endif
    *(uint32_t*)&signature[0] = 0;
    if (!UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2)) break;
    if (!UPDI::SEND(UPDI::UPDI_SYNCH)) break;
    if (!UPDI::SEND(UPDI::UPDI_SIB_256)) break;
    *(uint32_t*)&signature[0] = -1;
    p = &updi_sib[0];
    len = sizeof(updi_sib);
    while (len--) *p++ = UPDI::RECV();
    if (updi_sib[9] == ':') {
      /* UPDI connection success */
      signature[0] = 0x1E;
      signature[1] = updi_sib[0] == ' ' ? updi_sib[4] : updi_sib[0];
      signature[2] = updi_sib[10];
      UPDI::NVMPROGVER = updi_sib[10];
      UPDI::set_control(UPDI::UPDI_ACTIVE);
      #ifdef DEBUG_USE_USART
      DBG::print("[SIB]"); DBG::write(&updi_sib[0], 32, false);
      DBG::print(" PV=", false); DBG::write(UPDI::NVMPROGVER);
      #endif
    }
    return true;
  }
  #ifdef DEBUG_USE_USART
  DBG::print("(ER)", false);
  #endif
  return false;
}

bool UPDI::chip_erase (void) {
  for (;;) {
    /* Using HV mode before */
    if (!UPDI::is_control(UPDI::UPDI_ACTIVE | UPDI::ENABLE_NVMPG)) {
      UPDI::BREAK(false, true);
    }

    drain();
    if (!loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS, 500)) return false;

    /* send erase_key */
    if (UPDI::send_bytes(UPDI::erase_key, sizeof(UPDI::erase_key)) != sizeof(UPDI::erase_key)) break;

    #ifdef DEBUG_USE_USART
    DBG::print("(ERKEY)", false);
    #endif

    /* restart target : change mode */
    if (!UPDI::reset(true) || !UPDI::reset(false)) break;

    /* If RSTSYS is true, it is still not accessible */
    loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS);

    /* If LOCKSTATUS is clear, the chip is unlocked */
    loop_until_sys_stat_is_clear(UPDI_SYS_LOCKSTATUS);

    /* Make sure the CHER bit is cleared before next reset */
    loop_until_key_stat_is_clear(UPDI_KEY_CHIPERASE);

    UPDI::set_control(UPDI::CHIP_ERASE | UPDI::UPDI_ACTIVE);
    JTAG2::clear_control(JTAG2::ANS_FAILED);

    #ifdef DEBUG_USE_USART
    DBG::print("[ERASED]", false);
    #endif

    /* send nvmprog_key */
    if (UPDI::send_bytes(UPDI::nvmprog_key, sizeof(UPDI::nvmprog_key)) != sizeof(UPDI::nvmprog_key)) break;
    #ifdef DEBUG_USE_USART
    DBG::print("(NVMKEY)", false);
    #endif

    /* restart target : change mode */
    if (!UPDI::reset(true) || !UPDI::reset(false)) break;

    loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS);

    loop_until_sys_stat_is_set(UPDI_SYS_NVMPROG);

    if (!UPDI::is_control(UPDI::ENABLE_NVMPG)) {
      if (!UPDI::read_parameter()) break;
    }

    UPDI::set_control(UPDI::ENABLE_NVMPG);
    return true;
  }
  return false;
}

bool UPDI::enter_nvmprog (void) {
  for (;;) {
    /* is Enable NVMPROG ? */
    if (UPDI::is_sys_stat(UPDI::UPDI_SYS_NVMPROG)) {
      UPDI::set_control(UPDI::ENABLE_NVMPG);
      #ifdef DEBUG_USE_USART
      DBG::print("[NVMPRG]", false);  // NVMPROG enable
      #endif
      return true;
    }

    if (UPDI::is_sys_stat(UPDI::UPDI_SYS_LOCKSTATUS)) {
      #ifdef DEBUG_USE_USART
      DBG::print("[LOCK]", false);    // Device locked
      #endif
      return false;
    }

    /* send nvmprog_key */
    if (UPDI::send_bytes(UPDI::nvmprog_key, sizeof(UPDI::nvmprog_key)) != sizeof(UPDI::nvmprog_key)) break;

    #ifdef DEBUG_USE_USART
    DBG::print("(NVMKEY)");
    #endif

    /* restart target : change mode */
    if (!(UPDI::reset(true) && UPDI::reset(false))) break;

    #ifdef DEBUG_USE_USART
    DBG::print("(RST)", false);
    #endif

    /* wait enable : change mode success */
    do {
      // TIMER::delay_us(1800);
      UPDI::BREAK(true);
    }
    while (!UPDI::is_sys_stat(UPDI::UPDI_SYS_NVMPROG));
    #ifdef DEBUG_USE_USART
    DBG::print("[NVMPRG]", false);  // NVMPROG enable
    #endif
    UPDI::set_control(UPDI::ENABLE_NVMPG);

    return true;
  }
  return false;
}

bool UPDI::enter_updi (void) {
  volatile bool result = false;
  ABORT::stop_timer();
  UPDI::tdir_pull();
  UPDI::NVMPROGVER = 0;
  UPDI::clear_control(UPDI::UPDI_ACTIVE | /* UPDI::UPDI_LOWBAUD | */ UPDI::ENABLE_NVMPG);
  SYS::pgen_enable();
  // SYS::trst_disable();
  for (uint8_t i = 0; i < 3; i++) {
    if (setjmp(ABORT::CONTEXT) == 0) {
      ABORT::start_timer(ABORT::CONTEXT, 100);
      // UPDI::fallback_speed(UPDI_USART_BAUDRATE >> i);
      SYS::trst_enable();
      TIMER::delay_us(250);
      SYS::trst_disable();
      TIMER::delay_us(800);
      UPDI::BREAK(true);
      if (UPDI::read_parameter()) {
        ABORT::stop_timer();
        result = true;
        break;
      }
    }
    else {
      ABORT::stop_timer();
      #ifdef DEBUG_USE_USART
      DBG::print("(U_TO)", false);
      #endif
    }
    // UPDI::set_control(UPDI::UPDI_LOWBAUD);
  }
  return result;
}

bool UPDI::leave_updi (void) {
  for (;;) {
    if (!(UPDI::reset(true) && UPDI::reset(false))) break;

    /* leave updi */
    if (!UPDI::set_cs_stat(UPDI::UPDI_CS_CTRLB, UPDI::UPDI_SET_UPDIDIS)) break;
    #ifdef DEBUG_USE_USART
    DBG::print("[U_OF]"); // UPDI disabled
    #endif
    return true;
  }
  return false;
}

bool UPDI::write_userrow(uint32_t start_addr, size_t byte_count) {
  for (;;) {
    drain();
    if (!loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS, 500)) break;

    /* send urowwrite_key */
    if (UPDI::send_bytes(UPDI::urowwrite_key, sizeof(UPDI::urowwrite_key)) != sizeof(UPDI::urowwrite_key)) break;
    #ifdef DEBUG_USE_USART
    DBG::print("(NVMKEY)", false);
    #endif

    /* restart target : change mode */
    if (!UPDI::reset(true) || !UPDI::reset(false)) break;

    /* Wait for system reset to finish */
    if (!loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS, 500)) break;

    /* Make sure you are in USERROW mode */
    loop_until_sys_stat_is_set(UPDI_SYS_UROWPROG);

    NVM::write_data(start_addr, byte_count);

    #ifdef DEBUG_USE_USART
    DBG::print("(STORE)", false);
    #endif

    set_cs_stat(UPDI_CS_ASI_SYS_CTRLA, UPDI_SET_UROWWRITE_FINAL | UPDI_SET_CLKREQ);

    #ifdef DEBUG_USE_USART
    DBG::print("(FINAL)", false);
    #endif

    loop_until_sys_stat_is_clear(UPDI_SYS_UROWPROG, 200);

    set_cs_stat(UPDI_CS_ASI_KEY_STATUS, UPDI_KEY_UROWWRITE);

    #ifdef DEBUG_USE_USART
    DBG::print("(COMPLETE)", false);
    #endif

    /* send nvmprog_key */
    if (UPDI::send_bytes(UPDI::nvmprog_key, sizeof(UPDI::nvmprog_key)) != sizeof(UPDI::nvmprog_key)) break;
    #ifdef DEBUG_USE_USART
    DBG::print("(NVMKEY)", false);
    #endif

    /* restart target : change mode */
    if (!UPDI::reset(true) || !UPDI::reset(false)) break;
    loop_until_sys_stat_is_clear(UPDI_SYS_RSTSYS);

    loop_until_sys_stat_is_set(UPDI_SYS_NVMPROG);

    return true;
  }
  return false;
}

/* UPDI action */
bool UPDI::runtime (uint8_t updi_cmd) {
  volatile bool _result = false;
  ABORT::stop_timer();
  UPDI::clear_control(UPDI::UPDI_FALT | UPDI::UPDI_TIMEOUT);
  if (setjmp(ABORT::CONTEXT) == 0) {
    ABORT::start_timer(ABORT::CONTEXT, UPDI_ABORT_MS);
    switch (updi_cmd) {
      case UPDI::UPDI_CMD_ENTER : {
        if (UPDI::enter_updi()) {
          ABORT::start_timer(ABORT::CONTEXT, UPDI_ABORT_MS);
          _result = UPDI::enter_nvmprog();
        }
        break;
      }
      case UPDI::UPDI_CMD_LEAVE : {
        _result = UPDI::leave_updi(); break;
      }
      case UPDI::UPDI_CMD_ENTER_PROG : {
        if (UPDI::NVMPROGVER == 0) break;
        _result = UPDI::enter_nvmprog(); break;
      }
      case UPDI::UPDI_CMD_READ_MEMORY : {
        _result = NVM::read_memory(); break;
      }
      case UPDI::UPDI_CMD_WRITE_MEMORY : {
        if (UPDI::NVMPROGVER == 0) break;
        #ifdef DEBUG_USE_USART
        if (UPDI::is_sys_stat(UPDI::UPDI_SYS_LOCKSTATUS)) {
          DBG::print("[LOCKED]", false);
        }
        #endif
        _result = NVM::write_memory();
        #ifdef WRITE_RETRY
        if (!_result) {     /* write error retry */
          #ifdef DEBUG_USE_USART
          DBG::print("(RTY)");
          #endif
          _result = NVM::write_memory();
        }
        #endif 
        break;
      }
      case UPDI::UPDI_CMD_ERASE : {
        if (JTAG2::packet.body[1] == JTAG2::XMEGA_ERASE_CHIP
          && *((uint32_t*)&JTAG2::packet.body[2]) == 0) {
          _result = UPDI::is_control(UPDI::ENABLE_NVMPG)
                  ? NVM::chip_erase()
                  : UPDI::chip_erase();
        }
        break;
      }
      case UPDI::UPDI_CMD_TARGET_RESET : {
        #ifdef DEBUG_USE_USART
        DBG::print("[RST]");
        #endif
        if (UPDI::enter_updi()) {
          ABORT::start_timer(ABORT::CONTEXT, UPDI_ABORT_MS);
          _result = UPDI::reset(true) && UPDI::reset(false) && UPDI::leave_updi();
        }
        break;
      }
    }
    ABORT::stop_timer();
  }
  else {
    ABORT::stop_timer();
    #ifdef DEBUG_USE_USART
    DBG::print("(U_TO)"); // UPDI TIMEOUT
    #endif
    UPDI::BREAK();
    UPDI::set_control(UPDI::UPDI_TIMEOUT);
  }
  if (!_result) UPDI::set_control(UPDI::UPDI_FALT);
  #ifdef DEBUG_USE_USART
  if (!_result) {
    DBG::write('#');
    DBG::write_hex(UPDI::LASTH);
    DBG::write_hex(UPDI::LASTL);
    DBG::print("(U_NG)", false);
  }
  else {
    DBG::print("(U_OK)", false);
  }
  #endif
  return _result;
}

// end of code
