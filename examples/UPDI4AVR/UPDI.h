/**
 * @file UPDI.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <string.h>
#include "configuration.h"

namespace UPDI {
  extern volatile uint8_t LASTL;
  extern volatile uint8_t LASTH;
  extern uint8_t CONTROL;
  extern uint8_t NVMPROGVER;

  /* UPDI::CONTROL flags */
  enum updi_control_e {
      UPDI_ACTIVE   = 0x01
    , ENABLE_NVMPG  = 0x02
    , CHIP_ERASE    = 0x04
    , HV_ENABLE     = 0x08
    , UPDI_LOWBAUD  = 0x20
    , UPDI_TIMEOUT  = 0x40
    , UPDI_FALT     = 0x80
  };
  enum updi_command_e {
      UPDI_CMD_ENTER = 1
    , UPDI_CMD_LEAVE
    , UPDI_CMD_ENTER_PROG
    , UPDI_CMD_READ_MEMORY
    , UPDI_CMD_WRITE_MEMORY
    , UPDI_CMD_TARGET_RESET
    , UPDI_CMD_ERASE
  };
  enum updi_operate_e {
    /* UPDI command */
      UPDI_SYNCH   = 0x55
    , UPDI_ACK     = 0x40
    , UPDI_LDS     = 0x00
    , UPDI_STS     = 0x40
    , UPDI_LD      = 0x20
    , UPDI_ST      = 0x60
    , UPDI_LDCS    = 0x80
    , UPDI_STCS    = 0xC0
    , UPDI_REPEAT  = 0xA0
    , UPDI_RSTREQ  = 0x59
    /* KEY length */
    , UPDI_KEY_64  = 0xE0
    , UPDI_KEY_128 = 0xE1
    , UPDI_SIB_64  = 0xE4
    , UPDI_SIB_128 = 0xE5
    /* ADDR size */
    , UPDI_ADDR1   = 0x00
    , UPDI_ADDR2   = 0x04
    , UPDI_ADDR3   = 0x08
    /* DATA size */
    , UPDI_DATA1   = 0x00
    , UPDI_DATA2   = 0x01
    , UPDI_DATA3   = 0x02
    /* PoinTeR type  */
    , UPDI_PTR_IMD = 0x00
    , UPDI_PTR_INC = 0x04
    , UPDI_PTR_REG = 0x08
    /* control system register */
    , UPDI_CS_STATUSA        = 0x00
    , UPDI_CS_STATUSB        = 0x01
    , UPDI_CS_CTRLA          = 0x02
    , UPDI_CS_CTRLB          = 0x03
    , UPDI_CS_ASI_KEY_STATUS = 0x07
    , UPDI_CS_ASI_RESET_REQ  = 0x08
    , UPDI_CS_ASI_CTRLA      = 0x09
    , UPDI_CS_ASI_SYS_CTRLA  = 0x0A
    , UPDI_CS_ASI_SYS_STATUS = 0x0B
    , UPDI_CS_ASI_CRC_STATUS = 0x0C
  };
  enum updi_status_e {
      UPDI_KEY_UROWWRITE  = 0x20  // ASI_KEY_STATUS
    , UPDI_KEY_NVMPROG    = 0x10
    , UPSI_KEY_CHIPERASE  = 0x08
    , UPDI_SYS_RSTSYS     = 0x20
    , UPDI_SYS_INSLEEP    = 0x10
    , UPDI_SYS_NVMPROG    = 0x08
    , UPDI_SYS_UROWPROG   = 0x04
    , UPDI_SYS_LOCKSTATUS = 0x01
    , UPDI_CRC_STATUS_bm  = 0x07  // ASI_CRC_STATUS
  };
  enum updi_bitset_e {
      UPDI_ERR_PESIG_bm  = 0x07 // STATUSB
    , UPDI_SET_IBDLEY    = 0x80 // CTRLA
    , UPDI_SET_PARD      = 0x20
    , UPDI_SET_DTD       = 0x10
    , UPDI_SET_RSD       = 0x08
    , UPDI_SET_GTVAL_bm  = 0x07
    , UPDI_SET_GTVAL_128 = 0x00
    , UPDI_SET_GTVAL_64  = 0x01
    , UPDI_SET_GTVAL_32  = 0x02
    , UPDI_SET_GTVAL_16  = 0x03
    , UPDI_SET_GTVAL_8   = 0x04
    , UPDI_SET_GTVAL_4   = 0x05
    , UPDI_SET_GTVAL_2   = 0x06
    , UPDI_SET_NACKDIS   = 0x10 // CTRLB
    , UPDI_SET_CCDETDIS  = 0x08
    , UPDI_SET_UPDIDIS   = 0x04
    , UPDI_SET_UPDICLKSEL_bm   = 0x03 // ASI_CTRLA
    , UPDI_SET_UPDICLKSEL_32M  = 0x00 // 1800kbps
    , UPDI_SET_UPDICLKSEL_16M  = 0x01 // 900kbps
    , UPDI_SET_UPDICLKSEL_8M   = 0x02 // 450kbps
    , UPDI_SET_UPDICLKSEL_4M   = 0x03 // 225kbps
    , UPDI_SET_UROWWRITE_FINAL = 0x02 // ASI_SYS_CTRLA
    , UPDI_SET_CLKREQ          = 0x01
  };

  inline void tdir_push (void) {
    #if defined(UPDI_TDIR_PIN_INVERT)
    PIN_CTRL(UPDI_USART_PORT,UPDI_TDIR_PIN) &= ~(PORT_INVEN_bm);
    #else
    PIN_CTRL(UPDI_USART_PORT,UPDI_TDIR_PIN) |= PORT_INVEN_bm;
    #endif
  }
  inline void tdir_pull (void) {
    #if defined(UPDI_TDIR_PIN_INVERT)
    PIN_CTRL(UPDI_USART_PORT,UPDI_TDIR_PIN) |= PORT_INVEN_bm;
    #else
    PIN_CTRL(UPDI_USART_PORT,UPDI_TDIR_PIN) &= ~(PORT_INVEN_bm);
    #endif
  }

  void setup (void);
  void fallback_speed (uint32_t baudrate);

  inline uint8_t is_control (uint8_t value) {
    return UPDI::CONTROL & value;
  }
  inline void set_control (uint8_t value) {
    UPDI::CONTROL |= value;
  }
  inline void clear_control (uint8_t value) {
    UPDI::CONTROL &= ~(value);
  }

  void BREAK (bool longbreak = false, bool use_hv = false);
  bool SEND (const uint8_t data);
  uint8_t RECV (void);

  size_t send_bytes (const uint8_t *data, size_t len);
  bool send_repeat_header (uint8_t cmd, uint32_t addr, size_t len);

  bool st8 (uint32_t addr, uint8_t data);
  size_t sts8 (uint32_t addr, uint8_t *data, size_t len);

  uint8_t ld8 (uint32_t addr);

  bool is_cs_stat (const uint8_t code, const uint8_t check);
  inline bool is_sys_stat (const uint8_t check) {
    return is_cs_stat(UPDI_CS_ASI_SYS_STATUS, check);
  }
  inline bool is_key_stat (const uint8_t check) {
    return is_cs_stat(UPDI_CS_ASI_KEY_STATUS, check);
  }
  inline bool is_rst_stat (void) {
    return is_cs_stat(UPDI_CS_ASI_RESET_REQ, 1);
  }
  bool set_cs_stat (const uint8_t code, const uint8_t data);
  inline bool reset (bool logic) {
    return set_cs_stat(UPDI_CS_ASI_RESET_REQ, (logic ? UPDI_RSTREQ : 0));
  }
  bool read_parameter (void);
  bool check_sig (void);
  void hv_pulse (void);
  bool chip_erase (void);
  bool enter_nvmprog (void);
  bool enter_updi (void);
  bool leave_updi (void);

  bool runtime (uint8_t updi_cmnd);
}

// end of code
