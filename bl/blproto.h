#pragma once

#define BL_PAGE_SIZE JD_FLASH_PAGE_SIZE
#define BL_SUBPAGE_SIZE 208
#define BL_NUM_SUBPAGES ((BL_PAGE_SIZE + SUBPAGE_SIZE - 1) / SUBPAGE_SIZE)

#include "jacdac/dist/c/bootloader.h"
