/**
 * @file JTAG2.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <util/atomic.h>
#include "sys.h"
#include "JTAG2.h"
#include "UPDI.h"
#include "NVM.h"
#include "usart.h"
#include "timer.h"
#include "abort.h"
#include "dbg.h"

/* Global valiables */
uint8_t JTAG2::CONTROL;
uint8_t JTAG2::PARAM_EMU_MODE_VAL;
JTAG2::jtag_baud_rate_e JTAG2::PARAM_BAUD_RATE_VAL;
JTAG2::jtag_packet_t JTAG2::packet;

/* Local values */
namespace JTAG2 {
  const uint16_t BAUD_TABLE[] = {
      BAUD_REG_VAL(2400)    // 0: not used dummy
    , BAUD_REG_VAL(2400)    // 1: under limit low speed
    , BAUD_REG_VAL(4800)
    , BAUD_REG_VAL(9600)
    , BAUD_REG_VAL(19200)   // 4: JTAG2 startup default speed
    , BAUD_REG_VAL(38400)
    , BAUD_REG_VAL(57600)
    , BAUD_REG_VAL(115200)  // 7: avrdude 6.x max speed
    , BAUD_REG_VAL(14400)
    , BAUD_REG_VAL(153600)  // 9: using avrdude 7.x
    , BAUD_REG_VAL(230400)
    , BAUD_REG_VAL(460800)
    , BAUD_REG_VAL(921600)
    , BAUD_REG_VAL(128000)
    , BAUD_REG_VAL(256000)
    , BAUD_REG_VAL(512000)
    , BAUD_REG_VAL(1024000)
    , BAUD_REG_VAL(150000)
    , BAUD_REG_VAL(200000)
    , BAUD_REG_VAL(250000)
    , BAUD_REG_VAL(300000)
    , BAUD_REG_VAL(400000)
    , BAUD_REG_VAL(500000)
    , BAUD_REG_VAL(600000)
    , BAUD_REG_VAL(666666)
    , BAUD_REG_VAL(1000000)
    , BAUD_REG_VAL(1500000)
    , BAUD_REG_VAL(2000000) // F_CPU 16Mhz max limit
    , BAUD_REG_VAL(3000000) // F_CPU 24Mhz over
  };
  const uint8_t sign_on_resp[] = {
      JTAG2::RSP_SIGN_ON  // $00: MESSAGE_ID   : $86
    , 0x01                // $01: COMM_ID      : Communications protocol version
    , 0x01                // $02: M_MCU_BLDR   : boot-loader FW version
    , 0x06                // $03: M_MCU_FW_MIN : firmware version (minor)
    , 0x06                // $04: M_MCU_FW_MAJ : firmware version (major)
    , 0x01                // $05: M_MCU_HW     : hardware version
    , 0x01                // $06: S_MCU_BLDR   : boot-loader FW version
    , 0x00                // $07: S_MCU_FW_MIN : firmware version (minor)
    , 0x06                // $08: S_MCU_FW_MAJ : firmware version (major)
    , 0x01                // $09: S_MCU_HW     : hardware version
    , 0x00                // $0A: SERIAL_NUMBER0
    , 0x00                // $0B: SERIAL_NUMBER1
    , 0x00                // $0C: SERIAL_NUMBER2
    , 0x00                // $0D: SERIAL_NUMBER3
    , 0x00                // $0E: SERIAL_NUMBER4
    , 0x00                // $0F: SERIAL_NUMBER5
                          // $10-$1C: DEVICE_ID_STR : terminate \0
    , 'J', 'T', 'A', 'G', 'I', 'C', 'E', ' ', 'm', 'k', 'I', 'I', 0
  };
}

void JTAG2::setup (void) {
  JTAG2::CONTROL = 0;
  JTAG2::PARAM_BAUD_RATE_VAL = JTAG2::BAUD_19200;

  #ifdef JTAG_USART_PORTMUX
  PORTMUX.USARTROUTEA |= JTAG_USART_PORTMUX;
  #endif

  PIN_CTRL(JTAG_USART_PORT,JTAG_JTRX_PIN) = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;
  PIN_CTRL(JTAG_USART_PORT,JTAG_JTTX_PIN) = PORT_ISC_INPUT_DISABLE_gc;

  USART::setup(
    &JTAG_USART_MODULE,
    JTAG2::BAUD_TABLE[JTAG2::PARAM_BAUD_RATE_VAL],
    (0x00),
    (USART_RXEN_bm),
    (USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | USART_CMODE_ASYNCHRONOUS_gc | USART_SBMODE_1BIT_gc)
  );
}

bool JTAG2::transfer_enable (void) {
  if (!JTAG2::is_control(JTAG2::USART_TX_EN)) {
    SYS::pgen_enable();
    UPDI::runtime(UPDI::UPDI_CMD_ENTER);
    PIN_CTRL(JTAG_USART_PORT,JTAG_JTTX_PIN) = PORT_ISC_INTDISABLE_gc;
    JTAG_USART_PORT.DIRSET = _BV(JTAG_JTTX_PIN);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      JTAG_USART_MODULE.CTRLB |= USART_TXEN_bm;
    }
    JTAG2::set_control(JTAG2::USART_TX_EN);
    #ifdef DEBUG_USE_USART
    DBG::print("(TX_ON)", false);
    #endif
    TIMER::delay(200);
  }
  return true;
}

void JTAG2::transfer_disable (void) {
  if (JTAG2::is_control(JTAG2::USART_TX_EN)) {
    loop_until_bit_is_set(JTAG_USART_MODULE.STATUS, USART_TXCIF_bp);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      JTAG_USART_MODULE.CTRLB &= ~USART_TXEN_bm;
    }
    PIN_CTRL(JTAG_USART_PORT,JTAG_JTTX_PIN) = PORT_ISC_INPUT_DISABLE_gc;
    JTAG_USART_PORT.DIRCLR = _BV(JTAG_JTTX_PIN);
    if (UPDI::CONTROL & UPDI::UPDI_ACTIVE) {
      UPDI::runtime(UPDI::UPDI_CMD_LEAVE);
    }
    SYS::pgen_disable();
    SYS::trst_enable();
    SYS::trst_disable();
    #ifdef DEBUG_USE_USART
    DBG::print("(TX_OF)", false);
    #endif
  }
}

void JTAG2::change_baudrate (bool wait) {
  if (wait) {
    loop_until_bit_is_set(JTAG_USART_MODULE.STATUS, USART_TXCIF_bp);
  }
  USART::change_baudrate(&JTAG_USART_MODULE, JTAG2::BAUD_TABLE[JTAG2::PARAM_BAUD_RATE_VAL]);
  JTAG2::clear_control(JTAG2::CHANGE_BAUD);
}

void JTAG2::answer_transfer (void) {
  #ifdef DEBUG_USE_USART
  DBG::print("$", false);
  DBG::write_hex(packet.body[0]);
  #endif
  uint16_t crc = ~0;
  int16_t len = packet.size_word[0] + 8;
  uint8_t *p = &packet.soh;
  uint8_t *q = &packet.soh;
  if (packet.body[0] >= JTAG2::RSP_FAILED) {
    JTAG2::set_control(JTAG2::ANS_FAILED);
  }
  while (len--) crc = JTAG2::crc16_update(crc, *q++);
  (*q++) = crc;
  (*q++) = crc >> 8;
  while (p != q) JTAG2::put(*p++);
}

bool JTAG2::answer_after_change (void) {
  bool continuous = true;
  if (JTAG2::is_control(JTAG2::CHANGE_BAUD)) {
    JTAG2::change_baudrate(true);
    JTAG2::clear_control(JTAG2::CHANGE_BAUD);
  }
  /* finalize JTAG2 sequence */
  if (JTAG2::is_control(JTAG2::HOST_SIGN_ON | JTAG2::USART_TX_EN) == JTAG2::USART_TX_EN) {
    JTAG2::transfer_disable();
    /* final status indicator */
    if (!JTAG2::is_control(JTAG2::ANS_FAILED)) {
      UPDI::tdir_push();
      ABORT::start_timer(ABORT::CONTEXT, 1000);
    }
    continuous = false;
    JTAG2::CONTROL = UPDI::CONTROL = 0;
  }
  return continuous;
}

/* blocked character get */
uint8_t JTAG2::get (void) {
  loop_until_bit_is_set(JTAG_USART_MODULE.STATUS, USART_RXCIF_bp);
  return JTAG_USART_MODULE.RXDATAL;
}

uint8_t JTAG2::put (uint8_t data) {
  loop_until_bit_is_set(JTAG_USART_MODULE.STATUS, USART_DREIF_bp);
  JTAG_USART_MODULE.STATUS |= USART_TXCIF_bm;
  return JTAG_USART_MODULE.TXDATAL = data;
}

uint16_t JTAG2::crc16_update(uint16_t crc, uint8_t data) {
  return _crc_ccitt_update(crc, data);
}

bool JTAG2::packet_receive (void) {
  uint16_t crc = ~0;
  uint8_t *p = &packet.soh;
  uint8_t *q = &packet.soh;
  while (JTAG2::get() != MESSAGE_START);
  ABORT::start_timer(ABORT::CONTEXT, JTAG_ABORT_MS);
  ABORT::set_make_interrupt(ABORT::CONTEXT);
  (*p++) = MESSAGE_START;
  for (int16_t i = 0; i < 7; i++) (*p++) = JTAG2::get();
  if (packet.stx != TOKEN) {
    #ifdef DEBUG_USE_USART
    DBG::print("!token");
    #endif
    return false;
  }
  if (packet.size > sizeof(packet.body)) {
    #ifdef DEBUG_USE_USART
    DBG::print("!size");
    #endif
    return false;
  }
  for (int16_t i = -2; i < packet.size_word[0]; i++) (*p++) = JTAG2::get();
  while (p != q) crc = JTAG2::crc16_update(crc, *q++);
  if (crc != 0) {
    #ifdef DEBUG_USE_USART
    DBG::print("!crc");
    #endif
    return false;
  }
  return true;
}

void JTAG2::set_response (jtag_response_e response_code) {
  packet.size = 3;
  packet.body[0] = response_code;
  packet.body[1] = JTAG2::CONTROL;
  packet.body[2] = UPDI::CONTROL;
}

void JTAG2::sign_off (void) {
  JTAG2::PARAM_BAUD_RATE_VAL = JTAG2::BAUD_19200;
  JTAG2::clear_control(JTAG2::HOST_SIGN_ON);
  JTAG2::set_control(JTAG2::CHANGE_BAUD);
}

bool JTAG2::sign_on (void) {
  if (JTAG2::transfer_enable()) {
    JTAG2::set_control(JTAG2::HOST_SIGN_ON);
    packet.size = sizeof(sign_on_resp);
    for (uint8_t i = 0; i < sizeof(sign_on_resp); i++) {
      packet.body[i] = sign_on_resp[i];
    }
    JTAG2::clear_control(JTAG2::ANS_FAILED);
    return true;
  }
  else {
    JTAG2::set_response(JTAG2::RSP_FAILED);
    return false;
  }
}

void JTAG2::set_parameter (void) {
  uint8_t param_type = packet.body[1];
  uint8_t param_val = packet.body[2];
  switch (param_type) {
    case JTAG2::PARAM_EMU_MODE : {
      JTAG2::PARAM_EMU_MODE_VAL = param_val;
      #ifdef DEBUG_USE_USART
      DBG::print(" EMU=", false);
      DBG::write_hex(JTAG2::PARAM_EMU_MODE_VAL);
      #endif
      break;
    }
    case JTAG2::PARAM_BAUD_RATE : {
      if ((param_val >= JTAG2::BAUD_LOWER) && (param_val <= JTAG2::BAUD_UPPER)) {
        JTAG2::PARAM_BAUD_RATE_VAL = (jtag_baud_rate_e) param_val;
        JTAG2::set_control(JTAG2::CHANGE_BAUD);
        #ifdef DEBUG_USE_USART
        DBG::print(" BAUD=", false);
        DBG::write_hex(JTAG2::PARAM_BAUD_RATE_VAL);
        #endif
        break;
      }
      JTAG2::set_response(JTAG2::RSP_ILLEGAL_PARAMETER);
      return;
    }
    default : {
      #ifdef DEBUG_USE_USART
      DBG::write('?');
      DBG::write_hex(param_type);
      DBG::write(':');
      DBG::print_hex(*((uint32_t*)&packet.body[2]));
      #endif
    }
  }
  JTAG2::set_response(JTAG2::RSP_OK);
}

void JTAG2::get_parameter (void) {
  uint8_t &param_type = packet.body[1];
  switch (param_type) {
    case JTAG2::PARAM_HW_VER : {
      #ifdef DEBUG_USE_USART
      DBG::print(" HW_VER=", false);
      #endif
      packet.size_word[0] = 3;
      packet.body[1] = JTAG2::sign_on_resp[5];
      packet.body[2] = JTAG2::sign_on_resp[9];
      break;
    }
    case JTAG2::PARAM_FW_VER : {
      #ifdef DEBUG_USE_USART
      DBG::print(" SW_VER=", false);
      #endif
      packet.size_word[0] = 5;
      packet.body[1] = JTAG2::sign_on_resp[3];
      packet.body[2] = JTAG2::sign_on_resp[4];
      packet.body[3] = JTAG2::sign_on_resp[7];
      packet.body[4] = JTAG2::sign_on_resp[8];
      break;
    }
    case JTAG2::PARAM_EMU_MODE : {
      #ifdef DEBUG_USE_USART
      DBG::print(" EMU=", false);
      #endif
      packet.size_word[0] = 2;
      packet.body[1] = JTAG2::PARAM_EMU_MODE_VAL;
      break;
    }
    case JTAG2::PARAM_BAUD_RATE : {
      #ifdef DEBUG_USE_USART
      DBG::print(" BAUD=", false);
      DBG::print_dec(JTAG2::PARAM_BAUD_RATE_VAL);
      #endif
      packet.size_word[0] = 2;
      packet.body[1] = JTAG2::PARAM_BAUD_RATE_VAL;
      break;
    }
    case JTAG2::PARAM_VTARGET : {
      #ifdef DEBUG_USE_USART
      DBG::print(" VTG=", false);
      DBG::print_dec(PARAM_VTARGET_VAL);
      #endif
      packet.size_word[0] = 3;
      packet.body[1] = PARAM_VTARGET_VAL & 0xFF;
      packet.body[2] = PARAM_VTARGET_VAL >> 8;
      break;
    }
    default : {
      JTAG2::set_response(JTAG2::RSP_ILLEGAL_PARAMETER);
      return;
    }
  }
  packet.body[0] = JTAG2::RSP_PARAMETER;
}

void JTAG2::set_device_descriptor (void) {
  #ifdef DEBUG_USE_USART
  // uiFlashPageSize
  uint16_t flash_pagesize = *((uint16_t*)&packet.body[0x0f4]);
  // ucEepromPageSize
  uint16_t eeprom_pagesize = packet.body[0x0f6];
  // ulFlashSize
  uint32_t flash_areasize = *((uint32_t*)&packet.body[0x0fd]);
  // uiFlashpages
  uint16_t flash_pageunit = *((uint16_t*)&packet.body[0x11a]);
  DBG::print(" FPS=", false);
  DBG::print_dec(flash_pagesize);
  DBG::print(" EPS=", false);
  DBG::print_dec(eeprom_pagesize);
  DBG::print(" FAS=", false);
  DBG::print_dec(flash_areasize);
  DBG::print(" FPU=", false);
  DBG::print_dec(flash_pageunit);
  #ifdef DEBUG_DUMP_DESCRIPTOR
  DBG::print(" DUMP=", false);
  DBG::print_dec(packet.size_word[0]);
  DBG::dump(&packet.body[0], packet.size_word[0]);
  #endif
  #endif
  JTAG2::set_response(JTAG2::RSP_OK);
}

// end of code
