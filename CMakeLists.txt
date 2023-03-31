file(GLOB C_FILES
    stm32/*.c
)

add_library(jacdac-stm32 STATIC
    ${C_FILES}
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Device/ST/STM32${FAMILY}xx/Source/Templates/gcc/startup_stm32${MCU_NUM_LOWER}xx.s
)

target_include_directories(jacdac-stm32 PUBLIC
    stm32
    ${STM32_CUBE_PATH}/Drivers/STM32${FAMILY}xx_HAL_Driver/Inc
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Device/ST/STM32${FAMILY}xx/Include
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Core/Include
)

target_link_libraries(jacdac-stm32 PUBLIC
    jacdac
)
