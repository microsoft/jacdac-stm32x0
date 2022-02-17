
#include "jdstm.h"
#include "systime.h"
#include "LmHandler.h"
#include "lora_app.h"
#include "lora_info.h"

static LmHandlerCallbacks_t LmHandlerCallbacks;
#if 0
 = {.GetBatteryLevel = GetBatteryLevel,
                                                  .GetTemperature = GetTemperatureLevel,
                                                  .GetUniqueId = GetUniqueId,
                                                  .GetDevAddr = GetDevAddr,
                                                  .OnMacProcess = OnMacProcessNotify,
                                                  .OnJoinRequest = OnJoinRequest,
                                                  .OnTxData = OnTxData,
                                                  .OnRxData = OnRxData};
#endif

static LmHandlerParams_t LmHandlerParams = {.ActiveRegion = ACTIVE_REGION,
                                            .DefaultClass = LORAWAN_DEFAULT_CLASS,
                                            .AdrEnable = LORAWAN_ADR_STATE,
                                            .TxDatarate = LORAWAN_DEFAULT_DATA_RATE,
                                            .PingPeriodicity =
                                                LORAWAN_DEFAULT_PING_SLOT_PERIODICITY};

void jd_lora_init(void) {
    LoraInfo_Init();
    LmHandlerInit(&LmHandlerCallbacks);

    LmHandlerConfigure(&LmHandlerParams);

    // gateway configured for channels 8-15, 65
    static uint16_t gatewayMask[6] = {0xFF00, 0x0000, 0x0000, 0x0000, 0x0002, 0x0000};

    MibRequestConfirm_t mibReq;
    memset(&mibReq, 0, sizeof(mibReq));
    mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
    mibReq.Param.ChannelsDefaultMask = gatewayMask;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_CHANNELS_MASK;
    mibReq.Param.ChannelsMask = gatewayMask;
    LoRaMacMibSetRequestConfirm(&mibReq);

    LmHandlerJoin(LORAWAN_DEFAULT_ACTIVATION_TYPE);
}

void jd_lora_process(void) {
    SysTimeGetMcuTime(); // refresh timer
}
