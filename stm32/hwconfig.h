#pragma once

#if defined(STM32F030x4)
#define STM32F030x6 1 // HAL requires that
#define FLASH_PAGE_SIZE 1024
#define FLASH_PAGES 16
#elif defined(STM32F031x6)
#define FLASH_PAGE_SIZE 1024
#define FLASH_PAGES 32
#else
#error "define flash size"
#endif

#define FLASH_SIZE (FLASH_PAGE_SIZE * FLASH_PAGES)
#define SETTINGS_START (FLASH_SIZE - FLASH_PAGE_SIZE)
#define SETTINGS_SIZE FLASH_PAGE_SIZE
