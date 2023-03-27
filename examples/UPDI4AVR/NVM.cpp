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

bool NVM::read_memory (void) {
  uint8_t  mem_type   = JTAG2::packet.body[1];
  uint32_t byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  #endif
  JTAG2::packet.body[0] = JTAG2::RSP_MEMORY;
  JTAG2::packet.size = byte_count + 1;
  switch (mem_type) {
    case JTAG2::MTYPE_BOOT_FLASH :
    case JTAG2::MTYPE_XMEGA_FLASH : {
      if (byte_count & 1)
        return NVM::read_data_memory(start_addr, &JTAG2::packet.body[1], byte_count);
      else
        return NVM::read_flash(start_addr, &JTAG2::packet.body[1], byte_count);
    }
    case JTAG2::MTYPE_EEPROM :
    case JTAG2::MTYPE_FUSE_BITS :
    case JTAG2::MTYPE_LOCK_BITS :
    case JTAG2::MTYPE_SIGN_JTAG :
    case JTAG2::MTYPE_OSCCAL_BYTE :
    case JTAG2::MTYPE_EEPROM_XMEGA :
    case JTAG2::MTYPE_USERSIG :
    case JTAG2::MTYPE_PRODSIG : {
      return NVM::read_data_memory(start_addr, &JTAG2::packet.body[1], byte_count);
    }
  }
  return false;
}

bool NVM::write_memory (void) {
  uint8_t  mem_type   = JTAG2::packet.body[1];
  uint32_t byte_count = *((uint32_t*)&JTAG2::packet.body[2]);
  uint32_t start_addr = *((uint32_t*)&JTAG2::packet.body[6]);
  #ifdef DEBUG_USE_USART
  DBG::print(" MT=", false); DBG::write_hex(mem_type);
  DBG::print(" BC=", false); DBG::print_dec(byte_count);
  DBG::print(" SA=", false); DBG::print_hex(start_addr);
  #endif
  switch (mem_type) {
    case JTAG2::MTYPE_BOOT_FLASH :
    case JTAG2::MTYPE_XMEGA_FLASH : {
      return NVM::write_flash(start_addr, &JTAG2::packet.body[10], byte_count);
    }
    case JTAG2::MTYPE_LOCK_BITS : {
      if ((uint8_t)byte_count == 1) {
        return (UPDI::NVMPROGVER == 2
          ? NVM::write_fuse_v2(start_addr, JTAG2::packet.body[10])
          : NVM::write_fuse(start_addr, JTAG2::packet.body[10])
        );
      }
      return NVM::write_eeprom(start_addr, &JTAG2::packet.body[10], byte_count);
    }
    case JTAG2::MTYPE_FUSE_BITS : {
      if ((uint8_t)byte_count != 1) break;
      return (UPDI::NVMPROGVER == 2
        ? NVM::write_fuse_v2(start_addr, JTAG2::packet.body[10])
        : NVM::write_fuse(start_addr, JTAG2::packet.body[10])
      );
    }
    case JTAG2::MTYPE_USERSIG : {
      // AVR_DA/DB/DD/EA is Flash Type
      if (UPDI::NVMPROGVER == 2) {
        if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) return false;
        if (!UPDI::st8(start_addr, 0xFF)) return false;
        return NVM::write_flash(start_addr, &JTAG2::packet.body[10], byte_count);
      }
      // megaAVR/tinyAVR is EEPROM Type
    }
    case JTAG2::MTYPE_EEPROM :
    case JTAG2::MTYPE_EEPROM_XMEGA : {
      return NVM::write_eeprom(start_addr, &JTAG2::packet.body[10], byte_count);
    }
  }
  return false;
}

uint8_t NVM::nvm_wait (void) {
  while (UPDI::ld8(NVM::NVMCTRL_REG_STATUS) & 3);
  return UPDI::LASTL;
}

bool NVM::nvm_ctrl (uint8_t nvmcmd) {
  return UPDI::st8(NVM::NVMCTRL_REG_CTRLA, nvmcmd);
}

bool NVM::nvm_ctrl_v2 (uint8_t nvmcmd) {
  NVM::nvm_wait();
  if (UPDI::ld8(NVM::NVMCTRL_REG_CTRLA) == nvmcmd) return true;
  if (!NVM::nvm_ctrl(NVM::NVM_V2_CMD_NOCMD)) return false;
  if (NVM::NVM_V2_CMD_NOCMD != nvmcmd) return NVM::nvm_ctrl(nvmcmd);
  return true;
}

struct fuse_packet_t { uint16_t data; uint16_t addr; };
bool NVM::write_fuse (uint16_t addr, uint8_t data) {
  #ifdef DEBUG_USE_USART
  DBG::print("[N:FU]@", false);
  DBG::write_hex(data);
  #endif
  for (;;) {
    fuse_packet_t fuse_packet;
    fuse_packet.data = data;
    fuse_packet.addr = addr;
    NVM::nvm_wait();
    if (UPDI::sts8(NVM::NVMCTRL_REG_DATA, (uint8_t*)&fuse_packet, sizeof(fuse_packet)) != sizeof(fuse_packet)) break;
    if (!NVM::nvm_ctrl(NVM::NVM_CMD_WFU)) break;
    return NVM::nvm_wait() == 0;
  }
  return false;
}

bool NVM::write_fuse_v2 (uint16_t addr, uint8_t data) {
  #ifdef DEBUG_USE_USART
  DBG::print("[N2:FU]@", false);
  DBG::write_hex(data);
  #endif
  for (;;) {
    if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_EEERWR)) break;
    if (!UPDI::st8(addr, data)) break;
    bool _r = NVM::nvm_wait() == 0;
    if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD)) break;
    return _r;
  }
  return false;
}

bool NVM::read_flash (uint32_t start_addr, uint8_t *data, size_t byte_count) {
  uint8_t *p = data;
  size_t cnt = 0;
  for (;;) {
    if (byte_count == 0 || byte_count > 512) break;
    if (!UPDI::send_repeat_header(
      (UPDI::UPDI_LD | UPDI::UPDI_DATA2),
      start_addr,
      (byte_count >> 1)
    )) break;

    while (cnt < byte_count) {
      *p++ = UPDI::RECV();
      cnt++;
    }

    #ifdef DEBUG_DUMP_MEMORY
    DBG::print("[RD]", false);
    DBG::dump(data, byte_count);
    #endif
    break;
  }
  return cnt == byte_count;
}

bool NVM::write_flash (uint32_t start_addr, uint8_t *data, size_t byte_count) {
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_REG | UPDI::UPDI_DATA3
    , 0, 0, 0, 0  // $02:long address
  };
  static uint8_t set_repeat[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_STCS | UPDI::UPDI_CS_CTRLA
    , UPDI::UPDI_SET_RSD
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_REPEAT | UPDI::UPDI_DATA1
    , 0           // $05:repeat count
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_INC | UPDI::UPDI_DATA2
  };
  static uint8_t clr_rsd[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_STCS | UPDI::UPDI_CS_CTRLA
    , 0x0
  };

  /* fallback chek and re-try */
  if (((UPDI::CHIP_ERASE | UPDI::UPDI_LOWBAUD) & UPDI::CONTROL)
    == (UPDI::CHIP_ERASE | UPDI::UPDI_LOWBAUD)) {
    UPDI::CONTROL &= ~(UPDI::CHIP_ERASE | UPDI::UPDI_LOWBAUD);
    #ifdef DEBUG_USE_USART
    DBG::print("{RP}", false);
    #endif
    if (UPDI::enter_updi()) return false;
  }

  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[RD]", false);
  DBG::dump(data, byte_count);
  #endif
  for (;;) {
    byte_count >>= 1;
    if (!byte_count || byte_count > 256) break;

    /* NVMCTRL v2 */
    if (UPDI::NVMPROGVER == 2) {
      if (((uint16_t)start_addr & (JTAG2::flash_pagesize - 1)) == 0) {
        if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLPER)) break;
        if (!UPDI::st8(start_addr, 0xFF)) break;
      }
      if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_FLWR)) break;
    }
    /* NVMCTRL v1 */
    else {
      /* NVMCTRL page buffer clear */
      NVM::nvm_wait();
      if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) break;
      NVM::nvm_wait();
    }

    /* setting register pointer and enable RSD mode */
    *((uint32_t*)&set_ptr[2]) = start_addr;
    set_repeat[5] = byte_count - 1;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)-1) != sizeof(set_ptr)-1) break;
    if (UPDI::UPDI_ACK != UPDI::RECV()) break;
    if (UPDI::send_bytes(set_repeat, sizeof(set_repeat)) != sizeof(set_repeat)) break;

    /* page buffer stored */
    uint8_t *p = data;
    size_t cnt = 0;
    while (cnt < byte_count) {
      if (!UPDI::SEND(*p++)) break;
      if (!UPDI::SEND(*p++)) break;
      cnt++;
    }

    /* disable RSD mode */
    if (UPDI::send_bytes(clr_rsd, sizeof(clr_rsd)) != sizeof(clr_rsd)) break;

    /* NVMCTRL v2 */
    if (UPDI::NVMPROGVER == 2) {
      return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
    }
    /* NVMCTRL v1 */
    else {
      /* NVMCTRL write page and complete */
      if (!NVM::nvm_ctrl(NVM::NVM_CMD_WP)) break;
      return (NVM::nvm_wait() == 0 && cnt == byte_count);
    }
  }
  return false;
}

bool NVM::read_data_memory (uint32_t start_addr, uint8_t *data, size_t byte_count) {
  uint8_t *q = data;
  size_t cnt = 0;
  for (;;) {
    if (byte_count == 0 || byte_count > 256) break;
    if (!UPDI::send_repeat_header((UPDI::UPDI_LD | UPDI::UPDI_DATA1), start_addr, byte_count)) break;
    while (cnt < byte_count) {
      *q++ = UPDI::RECV();
      cnt++;
    }
    #ifdef DEBUG_USE_USART
    if (byte_count <= 8) {
      DBG::write(',');
      DBG::hexlist(data, cnt);
    }
    else {
      DBG::print("[RD]", false);
      DBG::dump(data, byte_count);
    }
    #endif
    break;
  }
  return cnt == byte_count;
}

bool NVM::write_eeprom (uint32_t start_addr, uint8_t *data, size_t byte_count) {
  static uint8_t set_ptr[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_REG | UPDI::UPDI_DATA3
    , 0, 0, 0, 0  // $02:long address
  };
  static uint8_t set_repeat[] = {
      UPDI::UPDI_SYNCH
    , UPDI::UPDI_REPEAT | UPDI::UPDI_DATA1
    , 0           // $03:repeat count
    , UPDI::UPDI_SYNCH
    , UPDI::UPDI_ST | UPDI::UPDI_PTR_INC | UPDI::UPDI_DATA1
  };
  #ifdef DEBUG_DUMP_MEMORY
  DBG::print("[WR]", false);
  DBG::dump(data, byte_count);
  #endif
  for (;;) {
    if (!byte_count || byte_count > 256) break;

    /* NVMCTRL v2 */
    if (UPDI::NVMPROGVER == 2) {
      if (!NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_EEERWR)) break;
    }
    /* NVMCTRL v1 */
    else {
      /* NVMCTRL page buffer clear */
      NVM::nvm_wait();
      if (!NVM::nvm_ctrl(NVM::NVM_CMD_PBC)) break;
      NVM::nvm_wait();
    }

    /* setting register pointer */
    *((uint32_t*)&set_ptr[2]) = start_addr;
    set_repeat[2] = byte_count - 1;
    if (UPDI::send_bytes(set_ptr, sizeof(set_ptr)-1) != sizeof(set_ptr)-1) break;
    if (UPDI::UPDI_ACK != UPDI::RECV()) break;
    if (UPDI::send_bytes(set_repeat, sizeof(set_repeat)) != sizeof(set_repeat)) break;

    /* page buffer stored */
    uint8_t *p = data;
    size_t cnt = 0;
    while (cnt < byte_count) {
      if (!UPDI::SEND(*p++)) break;
      if (UPDI::UPDI_ACK != UPDI::RECV()) break;
      cnt++;
    }

    /* NVMCTRL v2 */
    if (UPDI::NVMPROGVER == 2) {
      return NVM::nvm_ctrl_v2(NVM::NVM_V2_CMD_NOCMD);
    }
    /* NVMCTRL v1 */
    else {
      /* NVMCTRL write page and complete */
      if (!NVM::nvm_ctrl(NVM::NVM_CMD_ERWP)) break;
      return (NVM::nvm_wait() == 0 && cnt == byte_count);
    }
  }
  return false;
}

// end of code
