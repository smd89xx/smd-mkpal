#include "genesis.h"

__attribute__((externally_visible))
const ROMHeader rom_header = {
#if (ENABLE_BANK_SWITCH != 0)
    "SEGA SSF        ",
#elif (MODULE_MEGAWIFI != 0)
    "SEGA MEGAWIFI   ",
#else
    "SEGA MEGA DRIVE ",
#endif
    "(C)S.SILVA 2026 ",
    "SEGA MEGA DRIVE PALETTE GENERATOR               ",
    "SEGA GENESIS PALETTE GENERATOR                  ",
    "GM 00000000-00",
    0x000,
    "J6              ",
    0x00000000,
#if (ENABLE_BANK_SWITCH != 0)
    0x003FFFFF,
#else
    0x000FFFFF,
#endif
    0xE0FF0000,
    0xE0FFFFFF,
    "RA",
    0xF820,
    0x00200001,
    0x0020FFFF,
    "            ",
    "dealing with Windows was too hard       ",
    "JUE             "
};
