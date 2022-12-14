/**
 * @file UPDI.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
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
  const static uint8_t nvmprog_key[10] = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x20, 0x67, 0x6F, 0x72, 0x50, 0x4D, 0x56, 0x4E };
  const static uint8_t erase_key[10]   = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x65, 0x73, 0x61, 0x72, 0x45, 0x4D, 0x56, 0x4E };

  /* unused keys */
  // const static uint8_t urowwrite_key[10] = { UPDI::UPDI_SYNCH, UPDI::UPDI_KEY_64, 0x65, 0x74, 0x26, 0x73, 0x55, 0x4D, 0x56, 0x4E };

  volatile uint8_t LASTL; // Last read byte
  volatile uint8_t LASTH; // Last read status
  uint8_t NVMPROGVER;
  uint8_t CONTROL;
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
    , UPDI::UPDI_STS | UPDI::UPDI_ADDR2 | UPDI::UPDI_DATA1
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

bool UPDI::read_parameter (void) {
  uint8_t updi_sib[16];
  uint8_t *p;
  size_t len;
  for (;;) {
    #ifdef DEBUG_USE_USART
    DBG::print("(SIB)", false);
    #endif

    if (!UPDI::SEND(UPDI::UPDI_SYNCH)) break;
    if (!UPDI::SEND(UPDI::UPDI_SIB_128)) break;
    p = &updi_sib[0];
    len = sizeof(updi_sib);
    while (len--) *p++ = UPDI::RECV();

    /* UPDI connection success */

    UPDI::set_control(UPDI::UPDI_ACTIVE);
    UPDI::NVMPROGVER = (updi_sib[10] == '2') ? 2 : 1;
    #ifdef DEBUG_USE_USART
    DBG::print("[SIB]"); DBG::write(&updi_sib[0], 8, false);
    DBG::print("[NVM]"); DBG::write(&updi_sib[8], 8, false);
    DBG::print(" PV=", false); DBG::write('0' + UPDI::NVMPROGVER);
    #endif
    return true;
  }
  #ifdef DEBUG_USE_USART
  DBG::print("(ER)", false);
  #endif
  return false;
}

bool UPDI::check_sig (void) {
  uint8_t  mem_type   = JTAG2::packet.body[1];
  uint32_t byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  if (!UPDI::is_control(UPDI::ENABLE_NVMPG)
    && mem_type == JTAG2::MTYPE_SIGN_JTAG
    && byte_count == 1
    && start_addr >= NVM::BASE_SIGROW && start_addr <= (NVM::BASE_SIGROW + 2)) {
    JTAG2::packet.body[0] = JTAG2::RSP_MEMORY;
    JTAG2::packet.body[1] = UPDI::NVMPROGVER == 0 ? (UPDI::LASTH ? 0x01 : 0xFF) : 0x00;
    JTAG2::packet.size = 2;
    #ifdef DEBUG_USE_USART
    DBG::print(" MT=", false); DBG::write_hex(mem_type);
    DBG::print(" BC=", false); DBG::print_dec(byte_count);
    DBG::print(" SA=", false); DBG::print_hex(start_addr);
    DBG::write(',');
    DBG::hexlist(&JTAG2::packet.body[1], 1);
    #endif
    return true;
  }
  return false;
}

bool UPDI::chip_erase (void) {
  for (;;) {
    /* Using HV mode before */
    if (!UPDI::is_control(UPDI::UPDI_ACTIVE | UPDI::ENABLE_NVMPG)) {
      UPDI::BREAK(false, true);
    }

    /* send nvmprog_key */
    if (UPDI::send_bytes(UPDI::nvmprog_key, sizeof(UPDI::nvmprog_key)) != sizeof(UPDI::nvmprog_key)) break;
    #ifdef DEBUG_USE_USART
    DBG::print("(NVMKEY)", false);
    #endif

    /* send erase_key */
    if (UPDI::send_bytes(UPDI::erase_key, sizeof(UPDI::erase_key)) != sizeof(UPDI::erase_key)) break;

    #ifdef DEBUG_USE_USART
    DBG::print("(ERKEY)", false);
    #endif

    /* restart target : change mode */
    if (!(UPDI::reset(true) && UPDI::reset(false))) break;

    #ifdef DEBUG_USE_USART
    DBG::print("(RST)", false);
    #endif

    /* wait enable : chip erase mode success */
    TIMER::delay(50);

    if (!UPDI::is_control(UPDI::UPDI_ACTIVE | UPDI::ENABLE_NVMPG)) {
      if (!UPDI::enter_updi()) break;
    }

    while (UPDI::is_sys_stat(UPDI::UPDI_SYS_LOCKSTATUS));
    #ifdef DEBUG_USE_USART
    DBG::print("[ERASED]", false);
    #endif

    UPDI::set_control(UPDI::CHIP_ERASE);
    JTAG2::clear_control(JTAG2::ANS_FAILED);

    return UPDI::enter_nvmprog();
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
  UPDI::clear_control(UPDI::UPDI_ACTIVE | UPDI::UPDI_LOWBAUD | UPDI::ENABLE_NVMPG);
  SYS::pgen_enable();
  // SYS::trst_disable();
  for (uint8_t i = 0; i <= 2; i++) {
    if (setjmp(ABORT::CONTEXT) == 0) {
      ABORT::start_timer(ABORT::CONTEXT, 100);
      UPDI::fallback_speed(UPDI_USART_BAUDRATE >> i);
      SYS::trst_enable();
      SYS::trst_disable();
      TIMER::delay_us(200);
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
    UPDI::set_control(UPDI::UPDI_LOWBAUD);
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
        if (UPDI::is_sys_stat(UPDI::UPDI_SYS_LOCKSTATUS)) {
          #ifdef DEBUG_USE_USART
          DBG::print("[LOCKED]", false);
          #endif
          break;
        }
        _result = NVM::write_memory();
        if (!_result) {     /* write error retry */
          #ifdef DEBUG_USE_USART
          DBG::print("(RTY)");
          #endif
          _result = NVM::write_memory();
        }
        break;
      }
      case UPDI::UPDI_CMD_ERASE : {
        if (JTAG2::packet.body[1] == JTAG2::XMEGA_ERASE_CHIP
          && *((uint32_t*)&JTAG2::packet.body[2]) == 0) {
          _result = UPDI::chip_erase();
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
