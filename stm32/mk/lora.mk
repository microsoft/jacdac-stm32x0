CUBE = $(PLATFORM)/STM32CubeWL
LORAWAN = $(CUBE)/Middlewares/Third_Party/LoRaWAN

HALSRC +=  \
$(HALPREF)/stm32wlxx_hal_subghz.c \
\
$(LORAWAN)/Crypto/cmac.c \
$(LORAWAN)/Crypto/lorawan_aes.c \
$(LORAWAN)/Crypto/soft-se.c \
$(LORAWAN)/LmHandler/LmHandler.c \
$(LORAWAN)/LmHandler/NvmDataMgmt.c \
$(LORAWAN)/LmHandler/Packages/LmhpClockSync.c \
$(LORAWAN)/LmHandler/Packages/LmhpCompliance.c \
$(LORAWAN)/LmHandler/Packages/LmhpFirmwareManagement.c \
$(LORAWAN)/Mac/LoRaMac.c \
$(LORAWAN)/Mac/LoRaMacAdr.c \
$(LORAWAN)/Mac/LoRaMacClassB.c \
$(LORAWAN)/Mac/LoRaMacCommands.c \
$(LORAWAN)/Mac/LoRaMacConfirmQueue.c \
$(LORAWAN)/Mac/LoRaMacCrypto.c \
$(LORAWAN)/Mac/LoRaMacParser.c \
$(LORAWAN)/Mac/LoRaMacSerializer.c \
$(LORAWAN)/Mac/Region/Region.c \
$(LORAWAN)/Mac/Region/RegionAS923.c \
$(LORAWAN)/Mac/Region/RegionAU915.c \
$(LORAWAN)/Mac/Region/RegionBaseUS.c \
$(LORAWAN)/Mac/Region/RegionCN470.c \
$(LORAWAN)/Mac/Region/RegionCN779.c \
$(LORAWAN)/Mac/Region/RegionCommon.c \
$(LORAWAN)/Mac/Region/RegionEU433.c \
$(LORAWAN)/Mac/Region/RegionEU868.c \
$(LORAWAN)/Mac/Region/RegionIN865.c \
$(LORAWAN)/Mac/Region/RegionKR920.c \
$(LORAWAN)/Mac/Region/RegionRU864.c \
$(LORAWAN)/Mac/Region/RegionUS915.c \
$(LORAWAN)/Utilities/utilities.c \
\
$(CUBE)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio.c \
$(CUBE)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_driver.c \
$(CUBE)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_fw.c \
\
$(CUBE)/Utilities/misc/stm32_mem.c \
$(CUBE)/Utilities/timer/stm32_timer.c \
\
$(wildcard $(PLATFORM)/lora-e5/*.c)


CPPFLAGS += 	\
-I$(PLATFORM)/lora-e5 \
-I$(LORAWAN)/Utilities \
-I$(CUBE)/Utilities/misc \
-I$(CUBE)/Utilities/timer \
-I$(CUBE)/Middlewares/Third_Party/SubGHz_Phy/ \
-I$(LORAWAN)/LmHandler \
-I$(LORAWAN)/Mac \
-I$(LORAWAN)/Mac/Region \
-I$(LORAWAN)/LmHandler/Packages \
-DJD_LORA=1
