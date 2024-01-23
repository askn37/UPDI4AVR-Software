/**
 * @file NVM.h
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.2
 * @date 2023-04-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <string.h>
#include <setjmp.h>
#include "../configuration.h"

namespace NVM {
  /* NVMCTRL v0,2 */
  enum nvm_register_v02_e {
      NVMCTRL_REG_CTRLA    = 0x1000
    , NVMCTRL_REG_CTRLB    = 0x1001
    , NVMCTRL_REG_STATUS   = 0x1002
    , NVMCTRL_REG_INTCTRL  = 0x1003
    , NVMCTRL_REG_INTFLAGS = 0x1004
    , NVMCTRL_REG_DATA     = 0x1006
    , NVMCTRL_REG_DATAL    = 0x1006
    , NVMCTRL_REG_DATAH    = 0x1007
    , NVMCTRL_REG_ADDR     = 0x1008
    , NVMCTRL_REG_ADDRL    = 0x1008
    , NVMCTRL_REG_ADDRH    = 0x1009
    , NVMCTRL_REG_ADDR2    = 0x100A
  };
  /* NVMCTRL v3,4,5 */
  enum nvm_register_v345_e {
    /* register */
      NVMCTRL_V3_REG_CTRLA    = 0x1000
    , NVMCTRL_V3_REG_CTRLB    = 0x1001
    , NVMCTRL_V3_REG_INTCTRL  = 0x1004
    , NVMCTRL_V3_REG_INTFLAGS = 0x1005
    , NVMCTRL_V3_REG_STATUS   = 0x1006
    , NVMCTRL_V3_REG_DATA     = 0x1008
    , NVMCTRL_V3_REG_DATAL    = 0x1008
    , NVMCTRL_V3_REG_DATAH    = 0x1009
    , NVMCTRL_V3_REG_DATA2    = 0x100A
    , NVMCTRL_V3_REG_DATA3    = 0x100B
    , NVMCTRL_V3_REG_ADDR     = 0x100C
    , NVMCTRL_V3_REG_ADDRL    = 0x100C
    , NVMCTRL_V3_REG_ADDRH    = 0x100D
    , NVMCTRL_V3_REG_ADDR2    = 0x100E
    , NVMCTRL_V3_REG_ADDR3    = 0x100F
  };
  /* NVMCTRL v0 */
  enum nvm_control_v0_e {
      NVM_CMD_NOOP = 0
    , NVM_CMD_WP   = 1
    , NVM_CMD_ER   = 2
    , NVM_CMD_ERWP = 3
    , NVM_CMD_PBC  = 4
    , NVM_CMD_CHER = 5
    , NVM_CMD_EEER = 6
    , NVM_CMD_WFU  = 7
  };
  /* NVMCTRL v2,4 */
  enum nvm_control_v24_e {
      NVM_V2_CMD_NOCMD      = 0x00  /* NVM_V3_CMD_NOCMD */
    , NVM_V2_CMD_NOOP       = 0x01  /* NVM_V3_CMD_NOOP */
    , NVM_V2_CMD_FLWR       = 0x02  /* v2 only */
    , NVM_V2_CMD_FLPER      = 0x08  /* NVM_V3_CMD_FLPER */
    , NVM_V2_CMD_FLMPER2    = 0x09  /* NVM_V3_CMD_FLMPER2 */
    , NVM_V2_CMD_FLMPER4    = 0x0A  /* NVM_V3_CMD_FLMPER4 */
    , NVM_V2_CMD_FLMPER8    = 0x0B  /* NVM_V3_CMD_FLMPER8 */
    , NVM_V2_CMD_FLMPER16   = 0x0C  /* NVM_V3_CMD_FLMPER16 */
    , NVM_V2_CMD_FLMPER32   = 0x0D  /* NVM_V3_CMD_FLMPER32 */
    , NVM_V2_CMD_EEWR       = 0x12  /* v2 only */
    , NVM_V2_CMD_EEERWR     = 0x13  /* v2 only */
    , NVM_V2_CMD_EEBER      = 0x18  /* v2 only */
    , NVM_V2_CMD_EEMBER2    = 0x19  /* v2 only */
    , NVM_V2_CMD_EEMBER4    = 0x1A  /* v2 only */
    , NVM_V2_CMD_EEMBER8    = 0x1B  /* v2 only */
    , NVM_V2_CMD_EEMBER16   = 0x1C  /* v2 only */
    , NVM_V2_CMD_EEMBER32   = 0x1D  /* v2 only */
    , NVM_V2_CMD_CHER       = 0x20  /* NVM_V2_CMD_CHER */
    , NVM_V2_CMD_EECHER     = 0x30  /* NVM_V3_CMD_EECHER */
  };
  enum nvm_control_v35_e {
    /* command */
      NVM_V3_CMD_NOCMD      = 0x00  /* NVM_V2_CMD_NOCMD */
    , NVM_V3_CMD_NOOP       = 0x01  /* NVM_V2_CMD_NOOP */
    , NVM_V3_CMD_FLPW       = 0x04  /* v3 only */
    , NVM_V3_CMD_FLPERW     = 0x05  /* v3 only */
    , NVM_V3_CMD_FLPER      = 0x08  /* NVM_V2_CMD_FLPER */
    , NVM_V3_CMD_FLMPER2    = 0x09  /* NVM_V2_CMD_FLMPER2 */
    , NVM_V3_CMD_FLMPER4    = 0x0A  /* NVM_V2_CMD_FLMPER4 */
    , NVM_V3_CMD_FLMPER8    = 0x0B  /* NVM_V2_CMD_FLMPER8 */
    , NVM_V3_CMD_FLMPER16   = 0x0C  /* NVM_V2_CMD_FLMPER16 */
    , NVM_V3_CMD_FLMPER32   = 0x0D  /* NVM_V2_CMD_FLMPER32 */
    , NVM_V3_CMD_FLPBCLR    = 0x0F  /* v3 only */
    , NVM_V3_CMD_EEPW       = 0x14  /* v3 only */
    , NVM_V3_CMD_EEPERW     = 0x15  /* v3 only */
    , NVM_V3_CMD_EEPER      = 0x17  /* v3 only */
    , NVM_V3_CMD_EEPBCLR    = 0x1F  /* v3 only */
    , NVM_V3_CMD_CHER       = 0x20  /* NVM_V2_CMD_CHER */
    , NVM_V3_CMD_EECHER     = 0x30  /* NVM_V2_CMD_EECHER */
  };
  enum avr_base_addr_e {
      BASE_NVMCTRL = 0x1000
    , BASE_FUSE    = 0x1050
    , BASE_USERROW = 0x1080
    , BASE_SIGROW  = 0x1100
    , BASE_EEPROM  = 0x1400

    , BASE23_FUSE    = 0x1280
    , BASE23_USERROW = 0x1300

    , BASE45_SIGROW  = 0x1080
    , BASE45_BOOTROW = 0x1100
    , BASE45_USERROW = 0x1200
  };

  extern uint16_t flash_pagesize;
  bool read_memory (void);
  bool write_memory (void);
  bool write_data (uint32_t start_addr, size_t byte_count);
  bool write_data_word (uint32_t start_addr, size_t byte_count);

  bool read_data (uint32_t start_addr, size_t byte_count);
  bool read_flash (uint32_t start_addr, size_t byte_count);

  uint8_t nvm_wait (void);
  bool nvm_ctrl (uint8_t nvmcmd);
  bool write_fuse (uint16_t addr, uint8_t data);
  bool write_eeprom (uint32_t start_addr, size_t byte_count);
  bool write_flash (uint32_t start_addr, size_t byte_count, bool is_bound);

  bool nvm_ctrl_v2 (uint8_t nvmcmd);
  bool write_eeprom_v2 (uint32_t start_addr, size_t byte_count);
  bool write_flash_v2 (uint32_t start_addr, size_t byte_count, bool is_bound);

  uint8_t nvm_wait_v3 (void);
  bool nvm_ctrl_v3 (uint8_t nvmcmd);
  bool write_eeprom_v3 (uint32_t start_addr, size_t byte_count);
  bool write_flash_v3 (uint32_t start_addr, size_t byte_count, bool is_bound);

  bool write_eeprom_v4 (uint32_t start_addr, size_t byte_count);
  bool write_flash_v4 (uint32_t start_addr, size_t byte_count, bool is_bound);

  bool chip_erase (void);
}

// end of code
