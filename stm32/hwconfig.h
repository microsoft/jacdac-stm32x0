#pragma once

#if defined(STM32F030x4)
#define STM32F030x6 1 // HAL requires that
#define FLASH_PAGE_SIZE 1024
#elif defined(STM32F031x6)
#define FLASH_PAGE_SIZE 1024
#else
#error "define flash size"
#endif
