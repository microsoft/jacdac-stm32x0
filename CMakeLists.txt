file(GLOB DS_FILES
    stm32/*.c
)

add_library(jacdac-stm32 STATIC
    ${DS_FILES}
)

target_include_directories(jacdac-stm32 PUBLIC
    stm32
    ${STM32_CUBE_${FAMILY}_PATH}/Drivers/STM32${FAMILY}xx_HAL_Driver/Inc
    ${STM32_CUBE_${FAMILY}_PATH}/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ${STM32_CUBE_${FAMILY}_PATH}/Drivers/CMSIS/Core/Include
)

target_link_libraries(jacdac-stm32 PUBLIC
    jacdac
)
