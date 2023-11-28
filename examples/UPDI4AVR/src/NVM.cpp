/**
 * @file NVM.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.3
 * @date 2023-11-28
 *
 * @copyright Copyright (c) 2023 askn37 at github.com
 *
 */
#include "NVM.h"
#include "JTAG2.h"
#include "UPDI.h"
#include "usart.h"
#include "sys.h"
#include "dbg.h"

namespace NVM {
  struct fuse_packet_t { uint16_t data; uint16_t addr; };
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_REG | UPDI::UPDI_DATA3
    , 0, 0, 0, 0        // $02:24bit address
  };
  static uint8_t set_repeat[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_REPEAT | UPDI::UPDI_DATA1
    , 0                 // $03:repeat count
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_INC | UPDI::UPDI_DATA1
  };
  static uint8_t set_repeat_rsd[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_STCS    | UPDI::UPDI_CS_CTRLA
    , UPDI::UPDI_SET_RSD | UPDI::UPDI_SET_GTVAL_2
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_REPEAT  | UPDI::UPDI_DATA1
    , 0                 // $05:repeat count
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_INC | UPDI::UPDI_DATA2
  };
  uint16_t flash_pagesize;
}

bool NVM::read_memory (void) {
  uint8_t  mem_type   =               JTAG2::packet.body[1];
  size_t   byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  #endif
  /* Reads from 1 to 256 bytes and even bytes 258 to 512 are allowed */
  if (byte_count == 0 || byte_count > 512 || (byte_count > 256 && byte_count & 1)) {
    JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
    return true;
  }
  JTAG2::packet.body[0] = JTAG2::RSP_MEMORY;
  JTAG2::packet.size_word[0] = byte_count + 1;
  if (!UPDI::is_control(UPDI::ENABLE_NVMPG)) {
    uint8_t *p = &JTAG2::packet.body[1];
    bool is_sign_jtag = mem_type == JTAG2::MTYPE_SIGN_JTAG;
    do {
      *p++ = is_sign_jtag ? UPDI::signature[(uint8_t)start_addr++ & 3] : 0xff;
    } while (--byte_count);
    return true;
  }
  if (byte_count >> 8)
    return NVM::read_flash(start_addr, byte_count);
  else
    return NVM::read_data(start_addr, byte_count);
}

bool NVM::write_memory (void) {
  uint8_t  mem_type   =               JTAG2::packet.body[1];
  size_t   byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  JTAG2::set_response(JTAG2::RSP_OK);

  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  DBG::print(" PS=", false); DBG::print_hex(flash_pagesize);
  #endif

  /* Address specification outside the processing range is considered an IO area operation */
  if (start_addr >> 24) {
    start_addr &= 0xFFFF;
    mem_type = JTAG2::MTYPE_SRAM;
  }

  switch (mem_type) {
    case JTAG2::MTYPE_FLASH_PAGE :
    case JTAG2::MTYPE_BOOT_FLASH :
    case JTAG2::MTYPE_XMEGA_FLASH :
    {
      if (byte_count != flash_pagesize && byte_count != 256 && byte_count != 64) {
        JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
        return true;
      }

      /* Page boundaries require special handling */
      bool is_bound = UPDI::is_control(UPDI::CHIP_ERASE)
        && ((flash_pagesize - 1) & (uint16_t)start_addr) == 0;

      if (UPDI::NVMPROGVER == '0')
        return NVM::write_flash(start_addr, byte_count, is_bound);
      else if (UPDI::NVMPROGVER == '2')
        return NVM::write_flash_v2(start_addr, byte_count, is_bound);
      else
        return NVM::write_flash_v3(start_addr, byte_count, is_bound);
    }
  }

  /* Reads from 1 to 256 bytes and even bytes 258 to 512 are allowed */
  if (byte_count == 0 || byte_count > 256) {
    JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
    return true;
  }

  if (UPDI::is_control(UPDI::UPDI_ACTIVE) && mem_type == JTAG2::MTYPE_USERSIG) {
    if ((byte_count & 0x1f) != 0) {
      JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
      return true;
    }
    return UPDI::write_userrow(start_addr, byte_count);
  }

  if (!UPDI::is_control(UPDI::ENABLE_NVMPG)) return false;
  switch (mem_type) {
    case JTAG2::MTYPE_SRAM :
    {
      write_data(start_addr, byte_count);
      return true;
    }
    case JTAG2::MTYPE_LOCK_BITS :
    case JTAG2::MTYPE_FUSE_BITS :
      if (UPDI::NVMPROGVER == '0') {
        uint8_t *p = &JTAG2::packet.body[10];
        do {
          if (!NVM::write_fuse(start_addr++, *p++)) return false;
        } while (--byte_count);
        return true;
      }
  }
  if (UPDI::NVMPROGVER == '0')
    return NVM::write_eeprom(start_addr, byte_count);
  else if (UPDI::NVMPROGVER == '2')
    return NVM::write_eeprom_v2(start_addr, byte_count);
  else
    return NVM::write_eeprom_v3(start_addr, byte_count);
}

bool NVM::read_flash (uint32_t start_addr, size_t byte_count) {
  uint8_t* p = &JTAG2::packet.body[1];
  #ifdef DEBUG_DUMP_MEMORY
  size_t count = byte_count;
  #endif
  byte_count >>= 1;
  if (byte_count == 0 || byte_count > 256) return false;
  if (!UPDI::send_repeat_header(
    (UPDI::UPDI_LD | UPDI::UPDI_DATA2),
    start_addr,
    byte_count
  )) return false;
  do {
    *p++ = UPDI::RECV();
    *p++ = UPDI::RECV();
  } while (--byte_count);
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[RD]", false);
  DBG::dump(&JTAG2::packet.body[1], count);
  #endif
  return true;
}

bool NVM::read_data (uint32_t start_addr, size_t byte_count) {
  uint8_t* p = &JTAG2::packet.body[1];
  #ifdef DEBUG_DUMP_MEMORY
  size_t count = byte_count;
  #endif
  if (byte_count == 0 || byte_count > 256) return false;
  if (!UPDI::send_repeat_header(
    (UPDI::UPDI_LD | UPDI::UPDI_DATA1),
    start_addr, byte_count)) return false;
  do { *p++ = UPDI::RECV(); } while (--byte_count);
  #ifdef DEBUG_USE_USART
  if (count <= 8) {
    DBG::write(',');
    DBG::hexlist(&JTAG2::packet.body[1], count);
  }
  else {
    DBG::print("[RD]", false);
    DBG::dump(&JTAG2::packet.body[1], count);
  }
  #endif
  return true;
}

/* NVMCTRL v0 */
/* NVMCTRL v2 */
uint8_t NVM::nvm_wait (void) {
  while (UPDI::ld8(NVM::NVMCTRL_REG_STATUS) & 3);
  return UPDI::LASTL;
}

/* NVMCTRL v3 */
uint8_t NVM::nvm_wait_v3 (void) {
  while (UPDI::ld8(NVM::NVMCTRL_V3_REG_STATUS) & 3);
  return UPDI::LASTL;
}

/* NVMCTRL v0 */
bool NVM::nvm_ctrl (uint8_t nvmcmd) {
  return UPDI::st8(NVM::NVMCTRL_REG_CTRLA, nvmcmd);
}

bool nvm_ctrl_change (uint8_t nvmcmd) {
  if (UPDI::ld8(NVM::NVMCTRL_REG_CTRLA) == nvmcmd) return true;
  if (!NVM::nvm_ctrl(NVM::NVM_V2_CMD_NOCMD)) return false;
  if (NVM::NVM_V2_CMD_NOCMD != nvmcmd) return NVM::nvm_ctrl(nvmcmd);
  return true;
}

/* NVMCTRL v2 */
bool NVM::nvm_ctrl_v2 (uint8_t nvmcmd) {
  NVM::nvm_wait();
  return nvm_ctrl_change(nvmcmd);
}

/* NVMCTRL v3 */
bool NVM::nvm_ctrl_v3 (uint8_t nvmcmd) {
  NVM::nvm_wait_v3();
  return nvm_ctrl_change(nvmcmd);
}

/* NVMCTRL v0 */
bool NVM::write_fuse (uint16_t addr, uint8_t data) {
  fuse_packet_t fuse_packet;
  fuse_packet.data = data;
  fuse_packet.addr = addr;
  #ifdef DEBUG_USE_USART
  DBG::print("[N:FU]@", false);
  DBG::write_hex(data);
  #endif
  NVM::nvm_wait();
  if (!UPDI::sts8(NVM::NVMCTRL_REG_DATA,
    (uint8_t*)&fuse_packet, sizeof(fuse_packet))) return false;
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_WFU)) return false;
  return ((NVM::nvm_wait() & 7) == 0);
}

bool NVM::write_data (uint32_t start_addr, size_t byte_count) {
  /* setting register pointer */
  *((uint32_t*)&set_ptr[2]) = start_addr;
  set_repeat[2] = (uint8_t)byte_count - 1;
  if (!UPDI::send_bytes(set_ptr, sizeof(set_ptr) - 1)) return false;
  if (UPDI::UPDI_ACK != UPDI::RECV()) return false;
  if (!UPDI::send_bytes(set_repeat, sizeof(set_repeat))) return false;
  /* page buffer stored */
  uint8_t* p = &JTAG2::packet.body[10];
  do {
    if (!UPDI::SEND(*p++)) return false;
    if (UPDI::UPDI_ACK != UPDI::RECV()) return false;
  } while (--byte_count);
  return true;
}

bool NVM::write_data_word (uint32_t start_addr, size_t byte_count) {
  byte_count >>= 1;

  /* setting register pointer and enable RSD mode */
  *((uint32_t*)&set_ptr[2]) = start_addr;
  set_repeat_rsd[5] = (uint8_t)byte_count - 1;
  if (!UPDI::send_bytes(set_ptr, sizeof(set_ptr) - 1)) return false;
  if (UPDI::UPDI_ACK != UPDI::RECV()) return false;
  if (!UPDI::send_bytes(set_repeat_rsd, sizeof(set_repeat_rsd))) return false;

  /* page buffer stored */
  uint8_t* p = &JTAG2::packet.body[10];
  do {
    UPDI::SEND(*p++);
    UPDI::SEND(*p++);
  } while (--byte_count);

  /* disable RSD mode */
  return UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2);
}

/* NVMCTRL v0 */
bool NVM::write_eeprom (uint32_t start_addr, size_t byte_count) {
  if (byte_count > 64) {
    JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
    return true;
  }
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  /* NVMCTRL page buffer clear */
  // NVM::nvm_wait();
  // if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) return false;
  NVM::nvm_wait();

  if (!write_data(start_addr, byte_count)) return false;

  /* NVMCTRL write page and complete */
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_ERWP)) return false;
  return NVM::nvm_wait() == 0;
}

/* NVMCTRL v2 */
bool NVM::write_eeprom_v2 (uint32_t start_addr, size_t byte_count) {
  if (byte_count > 2) {
    JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
    return true;
  }
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_EEERWR)) return false;

  if (!write_data(start_addr, byte_count)) return false;

  return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
}

/* NVMCTRL v3 */
bool NVM::write_eeprom_v3 (uint32_t start_addr, size_t byte_count) {
  if (byte_count > 8) {
    JTAG2::set_response(JTAG2::RSP_ILLEGAL_MEMORY_RANGE);
    return true;
  }
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_EEPBCLR)) return false;

  if (!write_data(start_addr, byte_count)) return false;

  return NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_EEPERW);
}

/* NVMCTRL v0 */
bool NVM::write_flash (uint32_t start_addr, size_t byte_count, bool is_bound) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  /* This type can use the erase write concurrent command. */
  /* NVMCTRL page buffer clear */
  NVM::nvm_wait();
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) return false;
  NVM::nvm_wait();

  if (!write_data_word(start_addr, byte_count)) return false;

  /* disable RSD mode */
  if (!UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2)) return false;

  /* NVMCTRL write page and complete */
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_ERWP)) return false;
  return NVM::nvm_wait() == 0;
}

/* NVMCTRL v2 */
bool NVM::write_flash_v2 (uint32_t start_addr, size_t byte_count, bool is_bound) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  /* Sector erase if not chip erased */
  /* However, only when the beginning of the page boundary is addressed */
  if (!UPDI::is_control(UPDI::CHIP_ERASE)
  && ((uint16_t)start_addr & (flash_pagesize - 1)) == 0) {
    if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) return false;
    if (!UPDI::st8(start_addr, 0xFF)) return false;
  }
  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLWR)) return false;

  if (!write_data_word(start_addr, byte_count)) return false;

  return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
}

/* NVMCTRL v3 */
bool NVM::write_flash_v3 (uint32_t start_addr, size_t byte_count, bool is_bound) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  /* Sector erase if not chip erased */
  /* However, only when the beginning of the page boundary is addressed */
  if (!UPDI::is_control(UPDI::CHIP_ERASE)
  && ((uint16_t)start_addr & (flash_pagesize - 1)) == 0) {
    NVM::nvm_wait_v3();
    if (!UPDI::st8(start_addr, 0xFF)) return false;
    if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPER)) return false;
  }
  if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPBCLR)) return false;

  if (!write_data_word(start_addr, byte_count)) return false;

  return NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPW);
}

// end of code
