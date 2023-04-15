/**
 * @file JTAG2.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include <string.h>
#include <setjmp.h>
#include "../configuration.h"

#define BAUD_REG_VAL(baud) ((((F_CPU*8)/baud)+1)>> 1)
#define PARAM_VTARGET_VAL 5000

namespace JTAG2 {
  /* See documentation AVR067: JTAGICE mkII Communication Protocol Manual */

  /* Parameters IDs */
  enum jtag_parameter_e {
      PARAM_HW_VER    = 0x01
    , PARAM_FW_VER    = 0x02
    , PARAM_EMU_MODE  = 0x03
    , PARAM_BAUD_RATE = 0x05
    , PARAM_VTARGET   = 0x06
  };

  /* valid values for PARAM_BAUD_RATE_VAL */
  /* Changing CMT_SET_PARAMETER command */
  enum jtag_baud_rate_e {
      BAUD_2400 = 0x01  // $01 no supported
    , BAUD_4800         // $02
    , BAUD_9600
    , BAUD_19200        // $04 POR default
    , BAUD_38400
    , BAUD_57600
    , BAUD_115200       // $07 normal
    , BAUD_14400
    , BAUD_153600
    , BAUD_230400
    , BAUD_460800
    , BAUD_921600
    , BAUD_128000
    , BAUD_256000
    , BAUD_512000
    , BAUD_1024000
    , BAUD_150000
    , BAUD_200000
    , BAUD_250000
    , BAUD_300000
    , BAUD_400000
    , BAUD_500000
    , BAUD_600000
    , BAUD_666666
    , BAUD_1000000
    , BAUD_1500000
    , BAUD_2000000
    , BAUD_3000000

#if (F_CPU / 2400 < 4096)
    , BAUD_LOWER = BAUD_2400
#elif (F_CPU / 4800 < 4096)
    , BAUD_LOWER = BAUD_4800
#elif (F_CPU / 9600 < 4096)
    , BAUD_LOWER = BAUD_9600
#else
    , BAUD_LOWER = BAUD_19200
#endif

#if (F_CPU >= 24000000UL)
    , BAUD_UPPER = BAUD_3000000
#elif (F_CPU >= 16000000UL)
    , BAUD_UPPER = BAUD_2000000
#elif (F_CPU >= 12000000UL)
    , BAUD_UPPER = BAUD_1500000
#elif (F_CPU >= 8000000UL)
    , BAUD_UPPER = BAUD_1000000
#else
    , BAUD_UPPER = BAUD_115200
#endif
  };

  /* JTAG::CONTROL flags */
  enum jtag_control_e {
      HOST_SIGN_ON  = 0x01
    , USART_TX_EN   = 0x02
    , CHANGE_BAUD   = 0x04
    , ANS_FAILED    = 0x80
  };

  /* Master Commands IDs */
  enum jtag_cmnd_e {
      CMND_SIGN_OFF               = 0x00
    , CMND_GET_SIGN_ON            = 0x01
    , CMND_SET_PARAMETER          = 0x02
    , CMND_GET_PARAMETER          = 0x03
    , CMND_WRITE_MEMORY           = 0x04
    , CMND_READ_MEMORY            = 0x05
    , CMND_GO                     = 0x08
    , CMND_RESET                  = 0x0B
    , CMND_SET_DEVICE_DESCRIPTOR  = 0x0C
    , CMND_GET_SYNC               = 0x0F
    , CMND_ENTER_PROGMODE         = 0x14
    , CMND_LEAVE_PROGMODE         = 0x15
    , CMND_XMEGA_ERASE            = 0x34
  };

  /* Single byte response IDs */
  enum jtag_response_e {
    // Success
      RSP_OK                    = 0x80
    , RSP_PARAMETER             = 0x81
    , RSP_MEMORY                = 0x82
    , RSP_SIGN_ON               = 0x86
    // Error
    , RSP_FAILED                = 0xA0
    , RSP_ILLEGAL_PARAMETER     = 0xA1
    , RSP_ILLEGAL_MEMORY_TYPE   = 0xA2
    , RSP_ILLEGAL_MEMORY_RANGE  = 0xA3
    , RSP_ILLEGAL_MCU_STATE     = 0xA5
    , RSP_ILLEGAL_VALUE         = 0xA6
    , RSP_ILLEGAL_BREAKPOINT    = 0xA8
    , RSP_ILLEGAL_JTAG_ID       = 0xA9
    , RSP_ILLEGAL_COMMAND       = 0xAA
    , RSP_NO_TARGET_POWER       = 0xAB
    , RSP_DEBUGWIRE_SYNC_FAILED = 0xAC
    , RSP_ILLEGAL_POWER_STATE   = 0xAD
  };

  /* Memory type message sub-command */
  enum jtag_mem_type_e {
      MTYPE_IO_SHADOW     = 0x30   // cached IO registers?
    , MTYPE_SRAM          = 0x20   // target's SRAM or [ext.] IO registers
    , MTYPE_EEPROM        = 0x22   // EEPROM, what way?
    , MTYPE_EVENT         = 0x60   // ICE event memory
    , MTYPE_SPM           = 0xA0   // flash through LPM/SPM
    , MTYPE_FLASH_PAGE    = 0xB0   // flash in programming mode
    , MTYPE_EEPROM_PAGE   = 0xB1   // EEPROM in programming mode
    , MTYPE_FUSE_BITS     = 0xB2   // fuse bits in programming mode
    , MTYPE_LOCK_BITS     = 0xB3   // lock bits in programming mode
    , MTYPE_SIGN_JTAG     = 0xB4   // signature in programming mode
    , MTYPE_OSCCAL_BYTE   = 0xB5   // osccal cells in programming mode
    , MTYPE_CAN           = 0xB6   // CAN mailbox
    , MTYPE_XMEGA_FLASH   = 0xC0   // xmega (app.) flash
    , MTYPE_BOOT_FLASH    = 0xC1   // xmega boot flash
    , MTYPE_EEPROM_XMEGA  = 0xC4   // xmega EEPROM in debug mode
    , MTYPE_USERSIG       = 0xC5   // xmega user signature
    , MTYPE_PRODSIG       = 0xC6   // xmega production signature
  };

  /* CMND_XMEGA_ERASE sub-command */
  enum jtag_erase_mode_e {
      XMEGA_ERASE_CHIP
    , XMEGA_ERASE_APP
    , XMEGA_ERASE_BOOT
    , XMEGA_ERASE_EEPROM
    , XMEGA_ERASE_APP_PAGE
    , XMEGA_ERASE_BOOT_PAGE
    , XMEGA_ERASE_EEPROM_PAGE
    , XMEGA_ERASE_USERSIG
  };

  /* Global valiables */
  extern uint8_t CONTROL;
  extern uint8_t PARAM_EMU_MODE_VAL;
  extern jtag_baud_rate_e PARAM_BAUD_RATE_VAL;
  extern uint16_t flash_pagesize;

  /* JTAG2 packet */
  constexpr uint8_t MESSAGE_START = 0x1B; /* SOH */
  constexpr uint8_t TOKEN = 0x0E;         /* STX */
  constexpr int MAX_BODY_SIZE = 512 + 4 + 4 + 1;
  union jtag_packet_t {
    uint8_t _pad;                         // alignment padding
    uint8_t raw[MAX_BODY_SIZE + 1 + 4 + 4 + 1 + 2];
    struct {
      uint8_t soh;                        // $00:1 $1B
      union {                             // $01:2
        uint16_t number;
        uint8_t number_byte[2];
      };
      union {                             // $03:4=N
        uint32_t size;
        int16_t size_word[2];
        uint8_t size_byte[4];
      };
      uint8_t stx;                        // $07:1 $0E
      uint8_t body[MAX_BODY_SIZE];        // $08:N
      uint8_t _crc[2];                    // $08+N:2
    };
  } extern packet;

  /* methods */
  void setup (void);
  bool transfer_enable (void);
  void transfer_disable (void);
  void change_baudrate (bool wait = false);
  uint8_t get (void);
  uint8_t put (uint8_t data);
  uint16_t crc16_update(uint16_t crc, uint8_t data);
  bool packet_receive (void);
  void answer_transfer (void);
  bool answer_after_change (void);
  void set_response (jtag_response_e response_code);

  inline uint8_t is_control (uint8_t value) {
    return JTAG2::CONTROL & value;
  }
  inline void set_control (uint8_t value) {
    JTAG2::CONTROL |= value;
  }
  inline void clear_control (uint8_t value) {
    JTAG2::CONTROL &= ~(value);
  }

  void sign_off (void);
  bool sign_on (void);
  void set_parameter (void);
  void get_parameter (void);
  void set_device_descriptor (void);
}

// end of code
