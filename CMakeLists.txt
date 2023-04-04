file(GLOB C_FILES
    stm32/*.c
)

file(GLOB USB_FILES
    stm32/usb/*.c
    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

    stm32/lora-e5/hal_compat.c

    ${STM32_CUBE_PATH}/Drivers/STM32${FAMILY}xx_HAL_Driver/Src/*.c
)

LIST(FILTER USB_FILES EXCLUDE REGEX "template")


add_library(jacdac-stm32 STATIC
    ${C_FILES}
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Device/ST/STM32${FAMILY}xx/Source/Templates/gcc/startup_stm32${MCU_NUM_LOWER}xx.s
    ${USB_FILES}
)

target_include_directories(jacdac-stm32 PUBLIC
    stm32
    ${STM32_CUBE_PATH}/Drivers/STM32${FAMILY}xx_HAL_Driver/Inc
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Device/ST/STM32${FAMILY}xx/Include
    ${STM32_CUBE_PATH}/Drivers/CMSIS/Core/Include

    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${STM32_CUBE_PATH}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    stm32/usb
)

target_link_libraries(jacdac-stm32 PUBLIC
    jacdac
)
