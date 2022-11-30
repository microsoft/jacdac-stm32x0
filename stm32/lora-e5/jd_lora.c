
#include "jdstm.h"

#if JD_LORA
#include "systime.h"
#include "LmHandler.h"
#include "lora_app.h"
#include "lora_info.h"
#include "interfaces/jd_lora.h"

// arbitrary; do not use 224
#define LORAWAN_USER_APP_PORT 2
#define LORAWAN_SWITCH_CLASS_PORT 3

static bool needs_process;

static uint8_t get_battery_level(void) {
    // normally 0-254
    return BAT_LEVEL_NO_MEASURE;
}

static void process_notify(void) {
    needs_process = true;
}

static void get_device_id(uint8_t *id) {
    uint64_t devid = jd_device_id();
    memcpy(id, &devid, 8);
}

static uint32_t get_device_addr(void) {
    uint64_t devid = jd_device_id();
    return jd_hash_fnv1a(&devid, 8);
}

static void on_tx_data(LmHandlerTxParams_t *params) {
    if (!params)
        return;

    if (params->IsMcpsConfirm) {
        DMESG("MCPS-Confirm: U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", (int)params->UplinkCounter,
              params->AppData.Port, params->Datarate, params->TxPower);
        if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
            DMESG("CONFIRMED [%s]", (params->AckReceived != 0) ? "ACK" : "NACK");
        else
            DMESG("UNCONFIRMED");
    }
}

static void on_join_req(LmHandlerJoinParams_t *params) {
    if (!params)
        return;

    if (params->Status == LORAMAC_HANDLER_SUCCESS) {
        DMESG("JOINED %s", params->Mode == ACTIVATION_TYPE_ABP ? "ABP" : "OTAA");
    } else {
        DMESG("JOIN FAILED");
    }
}

// this is completely random
static void on_rx_data(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params) {
    if (!appData && !params)
        return;

    static const char *slotStrings[] = {"1",           "2",           "C",
                                        "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot"};

    DMESG("MCPS-Indication: D/L FRAME:%04d | SLOT:%s | PORT:%d | DR:%d | RSSI:%d | SNR:%d",
          (int)params->DownlinkCounter, slotStrings[params->RxSlot], appData->Port,
          params->Datarate, params->Rssi, params->Snr);

    switch (appData->Port) {
    case LORAWAN_SWITCH_CLASS_PORT:
        if (appData->BufferSize == 1) {
            switch (appData->Buffer[0]) {
            case 0:
                LmHandlerRequestClass(CLASS_A);
                break;
            case 1:
                LmHandlerRequestClass(CLASS_B);
                break;
            case 2:
                LmHandlerRequestClass(CLASS_C);
                break;
            default:
                break;
            }
        }
        break;
    case LORAWAN_USER_APP_PORT:
        if (appData->BufferSize == 1) {
            if (appData->Buffer[0] & 0x01)
                DMESG("LED OFF");
            else
                DMESG("LED ON");
        }
        break;
    }
}

static LmHandlerCallbacks_t LmHandlerCallbacks = {
    //
    .GetBatteryLevel = get_battery_level,
    .GetTemperature = adc_read_temp,
    .GetUniqueId = get_device_id,
    .GetDevAddr = get_device_addr,
    .OnMacProcess = process_notify,
    .OnJoinRequest = on_join_req,
    .OnTxData = on_tx_data,
    .OnRxData = on_rx_data,
};

static LmHandlerParams_t LmHandlerParams = {
    //
    .ActiveRegion = LORAMAC_REGION_US915,
    .DefaultClass = CLASS_A,
    .AdrEnable = LORAMAC_HANDLER_ADR_ON,
    .TxDatarate = DR_0,   // not used when ADR enabled
    .PingPeriodicity = 4, // 1<<4 seconds
};

void jd_lora_init(void) {
    UTIL_TIMER_Init();

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

    LmHandlerJoin(ACTIVATION_TYPE_OTAA);
}

void jd_lora_process(void) {
    if (needs_process) {
        needs_process = false;
        LmHandlerProcess();
    }

    // SysTimeGetMcuTime(); // refresh timer - not needed
}

int jd_lora_send(const void *data, uint32_t datalen) {
    if (datalen > LORAWAN_APP_DATA_BUFFER_MAX_SIZE)
        JD_PANIC();

    if (target_in_irq() && !jd_lora_in_timer())
        JD_PANIC();

    UTIL_TIMER_Time_t next_tx_ms = 0;
    LmHandlerAppData_t app_data = {
        .Port = LORAWAN_USER_APP_PORT,
        .BufferSize = datalen,
        .Buffer = (uint8_t *)data,
    };
    bool is_confirmed = false;

    // this copies data into internal buffers
    int r = LmHandlerSend(&app_data, is_confirmed, &next_tx_ms, false);
    DMESG("send -> %d; next tx %d ms", r, (int)next_tx_ms);
    return r;
}

#endif