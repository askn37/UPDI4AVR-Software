/**
 * @file NVM.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <string.h>
#include <setjmp.h>
#include "configuration.h"

namespace NVM {
  enum nvm_control_e {
    /* register */
      NVMCTRL_REG_CTRLA    = 0x1000
    , NVMCTRL_REG_CTRLB    = 0x1001
    , NVMCTRL_REG_STATUS   = 0x1002
    , NVMCTRL_REG_INTCTRL  = 0x1003
    , NVMCTRL_REG_INTFLAGS = 0x1004
    , NVMCTRL_REG_DATA     = 0x1006
    , NVMCTRL_REG_DATAL    = 0x1006
    , NVMCTRL_REG_DATAH    = 0x1007
    , NVMCTRL_REG_ADDR     = 0x1008
    , NVMCTRL_REG_ADDRL    = 0x1009
    , NVMCTRL_REG_ADDRH    = 0x100A
    , NVMCTRL_REG_ADDRZ    = 0x100B
    /* NVMCTRL v1 */
    , NVM_CMD_WP   = 1
    , NVM_CMD_ER   = 2
    , NVM_CMD_ERWP = 3
    , NVM_CMD_PBC  = 4
    , NVM_CMD_CHER = 5
    , NVM_CMD_EEER = 6
    , NVM_CMD_WFU  = 7
    /* NVMCTRL v2 */
    , NVM_V2_CMD_NOCMD      = 0
    , NVM_V2_CMD_NOOP       = 1
    , NVM_V2_CMD_FLWR       = 2
    , NVM_V2_CMD_FLPER      = 8
    , NVM_V2_CMD_FLMPER2    = 9
    , NVM_V2_CMD_FLMPER4    = 10
    , NVM_V2_CMD_FLMPER8    = 11
    , NVM_V2_CMD_FLMPER16   = 12
    , NVM_V2_CMD_FLMPER32   = 13
    , NVM_V2_CMD_EEWR       = 18
    , NVM_V2_CMD_EEERWR     = 19
    , NVM_V2_CMD_EEBER      = 24
    , NVM_V2_CMD_EEMBER2    = 25
    , NVM_V2_CMD_EEMBER4    = 26
    , NVM_V2_CMD_EEMBER8    = 27
    , NVM_V2_CMD_EEMBER16   = 28
    , NVM_V2_CMD_EEMBER32   = 29
    , NVM_V2_CMD_CHER       = 32
    , NVM_V2_CMD_EECHER     = 48
    , NVM_V2_FLMAP_SECTION0 = 0x00
    , NVM_V2_FLMAP_SECTION1 = 0x10
    , NVM_V2_FLMAP_SECTION2 = 0x20
    , NVM_V2_FLMAP_SECTION3 = 0x30
  };
  enum avr_base_addr_e {
      BASE_NVMCTRL = 0x1000
    , BASE_FUSE    = 0x1050
    , BASE_SIGROW  = 0x1100
  };
  bool read_memory (void);
  bool write_memory (void);

  uint8_t nvm_wait (void);
  bool nvm_ctrl (uint8_t nvmcmd);
  bool nvm_ctrl_v2 (uint8_t nvmcmd);

  bool write_fuse(uint16_t addr, uint8_t data);
  bool write_fuse_v2(uint16_t addr, uint8_t data);
  bool read_flash (uint32_t start_addr, uint8_t *data, size_t byte_count);
  bool write_flash (uint32_t start_addr, uint8_t *data, size_t byte_count);
  bool read_data_memory (uint32_t start_addr, uint8_t *data, size_t byte_count);
  bool write_eeprom (uint32_t start_addr, uint8_t *data, size_t byte_count);
}

// end of code
