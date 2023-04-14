/**
 * @file NVM.cpp
 * @author askn (K.Sato) multix.jp
 * @brief
 * @version 0.1
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
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
    , 0, 0, 0, 0        // $02:long address
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
}

bool NVM::read_memory (void) {
  uint8_t  mem_type   =         JTAG2::packet.body[1];
  size_t   byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  #endif
  JTAG2::packet.body[0] = JTAG2::RSP_MEMORY;
  JTAG2::packet.size_word[0] = byte_count + 1;
  if (byte_count >= 2) {
    /* flash area is always even */
    switch (mem_type) {
      case JTAG2::MTYPE_XMEGA_FLASH :     // 0xC0
      case JTAG2::MTYPE_BOOT_FLASH : {    // 0xC1
        return NVM::read_flash(start_addr, byte_count);
      }
      /* EEPROM area */
      case JTAG2::MTYPE_EEPROM :          // 0x22
      case JTAG2::MTYPE_EEPROM_XMEGA :    // 0xC4
      case JTAG2::MTYPE_USERSIG :         // 0xC5
      case JTAG2::MTYPE_FUSE_BITS :       // 0xB2
      case JTAG2::MTYPE_LOCK_BITS :       // 0xB3
      case JTAG2::MTYPE_SIGN_JTAG :       // 0xB4
      case JTAG2::MTYPE_OSCCAL_BYTE :     // 0xB5
      case JTAG2::MTYPE_PRODSIG : {       // 0xC6
        break;
      }
      default : return false;
    }
  }
  return NVM::read_data(start_addr, byte_count);
}

bool NVM::write_memory (void) {
  uint8_t  mem_type   =               JTAG2::packet.body[1];
  size_t   byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  #endif
  /* flash area is always even */
  if (byte_count >= 2) {
    switch (mem_type) {
      case JTAG2::MTYPE_XMEGA_FLASH :   // 0xC0
      case JTAG2::MTYPE_BOOT_FLASH : {  // 0xC1
        if (UPDI::NVMPROGVER == '3')
          return NVM::write_flash_v3(start_addr, byte_count);
        if (UPDI::NVMPROGVER == '2')
          return NVM::write_flash_v2(start_addr, byte_count);
        return NVM::write_flash(start_addr, byte_count);
      }
      case JTAG2::MTYPE_USERSIG : {     // 0xC5
        /* AVR_DA/DB/DD/EA is Flash */
        /* This system is implemented as Flash */
        /* NVMCTRL v3 */
        if (UPDI::NVMPROGVER == '3') {
          /* Erase page before writing */
            NVM::nvm_wait_v3();
          if (!UPDI::st8(start_addr, 0xFF)) return false;
          if (!NVM::nvm_ctrl_v3(NVM::NVM_V2_CMD_FLPER)) return false;
          UPDI::set_control(UPDI::CHIP_ERASE);
          return NVM::write_flash_v3(start_addr, byte_count);
        }
        /* NVMCTRL v2 */
        if (UPDI::NVMPROGVER == '2') {
          /* Erase page before writing */
          if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) return false;
          if (!UPDI::st8(start_addr, 0xFF)) return false;
          UPDI::set_control(UPDI::CHIP_ERASE);
          return NVM::write_flash_v2(start_addr, byte_count);
        }
        /* megaAVR/tinyAVR is EEPROM */
        /* This system is implemented as an EEPROM */
        break;
      }
      case JTAG2::MTYPE_EEPROM :        // 0x22
      case JTAG2::MTYPE_EEPROM_XMEGA :  // 0xC4
      case JTAG2::MTYPE_LOCK_BITS : {   // 0xB3
        break;
      }
      default : return false;
    }
  }
  else {
    /* 1-byte unit writing passes here */
    uint8_t data = JTAG2::packet.body[10];
    switch (mem_type) {
      case JTAG2::MTYPE_LOCK_BITS :     // 0xB3
      case JTAG2::MTYPE_FUSE_BITS : {   // 0xB2
        if (UPDI::ld8(start_addr) == data && UPDI::LASTH == 0) return true;
        if (UPDI::NVMPROGVER == '3')
          return NVM::write_fuse_v3(start_addr, data);
        if (UPDI::NVMPROGVER == '2')
          return NVM::write_fuse_v2(start_addr, data);
        return NVM::write_fuse(start_addr, data);
      }
      case JTAG2::MTYPE_USERSIG : {     // 0xC5
        /* AVR_DA/DB/DD/EA is Flash */
        /* This system is implemented as Flash */
        /* NVMCTRL v3 */
        if (UPDI::NVMPROGVER == '3') {
          /* If the address is the beginning and 0xFF is written, the page is erased. */
          if (((uint8_t)start_addr & 63) == 0 && data == 0xFF) {
            NVM::nvm_wait_v3();
            if (!UPDI::st8(start_addr, 0xFF)) return false;
            if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPER)) return false;
            return true;
          }
          /* Since Flash can only be written in units of even numbers, make it an even number. */
          /* Supplement 0xFF to the other byte */
          if ((uint8_t)start_addr & 1) {
            JTAG2::packet.body[11] = data;
            JTAG2::packet.body[10] = 0xFF;
            (uint8_t)start_addr--;
          }
          else {
            JTAG2::packet.body[11] = 0xFF;
          }
          byte_count = 2;
          UPDI::set_control(UPDI::CHIP_ERASE);
          return NVM::write_flash_v3(start_addr, byte_count);
        }
        /* NVMCTRL v2 */
        if (UPDI::NVMPROGVER == '2') {
          /* If the address is the beginning and 0xFF is written, the page is erased. */
          if (((uint8_t)start_addr & 63) == 0 && data == 0xFF) {
            if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) return false;
            if (!UPDI::st8(start_addr, 0xFF)) return false;
            return true;
          }
          /* Since Flash can only be written in units of even numbers, make it an even number. */
          /* Supplement 0xFF to the other byte */
          if ((uint8_t)start_addr & 1) {
            JTAG2::packet.body[11] = data;
            JTAG2::packet.body[10] = 0xFF;
            (uint8_t)start_addr--;
          }
          else {
            JTAG2::packet.body[11] = 0xFF;
          }
          byte_count = 2;
          UPDI::set_control(UPDI::CHIP_ERASE);
          return NVM::write_flash_v2(start_addr, byte_count);
        }
        /* megaAVR/tinyAVR is EEPROM */
        /* This system is implemented as an EEPROM */
      }
    }
  }
  if (UPDI::NVMPROGVER == '3')
    return NVM::write_eeprom_v3(start_addr, byte_count);
  if (UPDI::NVMPROGVER == '2')
    return NVM::write_eeprom_v2(start_addr, byte_count);
  return NVM::write_eeprom(start_addr, byte_count);
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

/* NVMCTRL v2 */
bool NVM::write_fuse_v2 (uint16_t addr, uint8_t data) {
  #ifdef DEBUG_USE_USART
  DBG::print("[N:FU]@", false);
  DBG::write_hex(data);
  #endif
  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_EEERWR)) return false;
  if (!UPDI::st8(addr, data)) return false;
  bool _r = (NVM::nvm_wait() & 0x70) == 0;
  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD)) return false;
  return _r;
}

/* NVMCTRL v3 */
bool NVM::write_fuse_v3 (uint16_t addr, uint8_t data) {
  #ifdef DEBUG_USE_USART
  DBG::print("[N:FU]@", false);
  DBG::write_hex(data);
  #endif
  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD)) return false;
  if (!UPDI::st8(addr, data)) return false;
  bool _r = (NVM::nvm_wait_v3() & 0x70) == 0;
  if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_EEPERW)) return false;
  return _r;
}

/* NVMCTRL v0 */
bool NVM::write_eeprom (uint32_t start_addr, size_t byte_count) {
  if (byte_count == 0 || byte_count > 256) return false;
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  /* NVMCTRL page buffer clear */
  NVM::nvm_wait();
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) return false;
  NVM::nvm_wait();

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

  /* NVMCTRL write page and complete */
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_ERWP)) return false;
  return NVM::nvm_wait() == 0;
}

/* NVMCTRL v2 */
bool NVM::write_eeprom_v2 (uint32_t start_addr, size_t byte_count) {
  if (byte_count == 0 || byte_count > 256) return false;
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_EEERWR)) return false;

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

  return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
}

/* NVMCTRL v3 */
bool NVM::write_eeprom_v3 (uint32_t start_addr, size_t byte_count) {
  if (byte_count == 0 || byte_count > 256) return false;
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif

  if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_NOCMD)) return false;

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

  return NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_EEPERW);
}

/* NVMCTRL v0 */
bool NVM::write_flash (uint32_t start_addr, size_t byte_count) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif
  byte_count >>= 1;
  if (byte_count == 0 || byte_count > 256) return false;

  /* この系統ではページ消去を書込と同時に行える */
  /* NVMCTRL page buffer clear */
  NVM::nvm_wait();
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) return false;
  NVM::nvm_wait();

  /* setting register pointer and enable RSD mode */
  *((uint32_t*)&set_ptr[2]) = start_addr;
  set_repeat_rsd[5] = byte_count - 1;
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
  if (!UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2)) return false;

  /* NVMCTRL write page and complete */
  if (!NVM::nvm_ctrl(NVM::NVM_CMD_ERWP)) return false;
  return NVM::nvm_wait() == 0;
}

/* NVMCTRL v2 */
bool NVM::write_flash_v2 (uint32_t start_addr, size_t byte_count) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif
  byte_count >>= 1;
  if (byte_count == 0 || byte_count > 256) return false;

  /* チップ消去していない場合はセクター消去 */
  /* ただしページ境界先頭がアドレスされた場合に限る */
  if (!UPDI::is_control(UPDI::CHIP_ERASE)
  && ((uint16_t)start_addr & (JTAG2::flash_pagesize - 1)) == 0) {
    if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) return false;
    if (!UPDI::st8(start_addr, 0xFF)) return false;
  }
  if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLWR)) return false;

  /* setting register pointer and enable RSD mode */
  *((uint32_t*)&set_ptr[2]) = start_addr;
  set_repeat_rsd[5] = byte_count - 1;
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
  if (!UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2)) return false;

  return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
}

/* NVMCTRL v3 */
bool NVM::write_flash_v3 (uint32_t start_addr, size_t byte_count) {
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(start_addr, byte_count);
  #endif
  byte_count >>= 1;
  if (byte_count == 0 || byte_count > 256) return false;

  /* チップ消去していない場合はセクター消去 */
  /* ただしページ境界先頭がアドレスされた場合に限る */
  if (!UPDI::is_control(UPDI::CHIP_ERASE)
  && ((uint16_t)start_addr & (JTAG2::flash_pagesize - 1)) == 0) {
    NVM::nvm_wait_v3();
    if (!UPDI::st8(start_addr, 0xFF)) return false;
    if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPER)) return false;
  }
  if (!NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPBCLR)) return false;

  /* setting register pointer and enable RSD mode */
  *((uint32_t*)&set_ptr[2]) = start_addr;
  set_repeat_rsd[5] = byte_count - 1;
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
  if (!UPDI::set_cs_ctra(UPDI::UPDI_SET_GTVAL_2)) return false;

  return NVM::nvm_ctrl_v3(NVM::NVM_V3_CMD_FLPW);
}

// end of code
