/**
 ******************************************************************************
 * @file    usbd_cdc_if.c
 * @author  MCD Application Team
 * @brief   Generic media access Layer.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                      www.st.com/SLA0044
 *
 ******************************************************************************
 */

#include "usbd_cdc_if.h"
#include "interfaces/jd_usb.h"

static int8_t JDUSB_Init(void);
static int8_t JDUSB_DeInit(void);
static int8_t JDUSB_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t JDUSB_Receive(uint8_t *pbuf, uint32_t *Len);
static int8_t JDUSB_TransmitCplt(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);

USBD_CDC_ItfTypeDef USBD_CDC_JDUSB_fops = {JDUSB_Init, JDUSB_DeInit, JDUSB_Control, JDUSB_Receive,
                                           JDUSB_TransmitCplt};

USBD_CDC_LineCodingTypeDef linecoding = {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
};

extern USBD_HandleTypeDef USBD_Device;
uint8_t UserRxBuffer[64];
uint8_t UserTxBuffer[64];
volatile uint8_t usb_in_tx;

static void maybe_fill_buffer(int force) {
    int len = 0;

    target_disable_irq();
    if (force || usb_in_tx == 0) {
        len = jd_usb_pull(UserTxBuffer);
        usb_in_tx = len > 0;
    }
    target_enable_irq();

    if (len > 0) {
        if (USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBuffer, len) != 0 ||
            USBD_CDC_TransmitPacket(&USBD_Device) != 0)
            usb_in_tx = 0;
    }
}

void jd_usb_pull_ready(void) {
    maybe_fill_buffer(0);
}

/**
 * @brief  JDUSB_Init
 *         Initializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t JDUSB_Init(void) {
    USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBuffer, 0);
    USBD_CDC_SetRxBuffer(&USBD_Device, UserRxBuffer);

    USBD_CDC_ReceivePacket(&USBD_Device);

    return (0);
}

/**
 * @brief  JDUSB_DeInit
 *         DeInitializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t JDUSB_DeInit(void) {
    /*
       Add your deinitialization code here
    */
    return (0);
}

/**
 * @brief  JDUSB_Control
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t JDUSB_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
    switch (cmd) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
        /* Add your code here */
        break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
        /* Add your code here */
        break;

    case CDC_SET_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_GET_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_CLEAR_COMM_FEATURE:
        /* Add your code here */
        break;

    case CDC_SET_LINE_CODING:
        linecoding.bitrate =
            (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24));
        linecoding.format = pbuf[4];
        linecoding.paritytype = pbuf[5];
        linecoding.datatype = pbuf[6];

        /* Add your code here */
        break;

    case CDC_GET_LINE_CODING:
        pbuf[0] = (uint8_t)(linecoding.bitrate);
        pbuf[1] = (uint8_t)(linecoding.bitrate >> 8);
        pbuf[2] = (uint8_t)(linecoding.bitrate >> 16);
        pbuf[3] = (uint8_t)(linecoding.bitrate >> 24);
        pbuf[4] = linecoding.format;
        pbuf[5] = linecoding.paritytype;
        pbuf[6] = linecoding.datatype;

        /* Add your code here */
        break;

    case CDC_SET_CONTROL_LINE_STATE:
        /* Add your code here */
        break;

    case CDC_SEND_BREAK:
        /* Add your code here */
        break;

    default:
        break;
    }

    return (0);
}

/**
 * @brief  JDUSB_Receive
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will issue a NAK packet on any OUT packet received on
 *         USB endpoint until exiting this function. If you exit this function
 *         before transfer is complete on CDC interface (ie. using DMA controller)
 *         it will result in receiving more data while previous ones are still
 *         not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t JDUSB_Receive(uint8_t *Buf, uint32_t *Len) {
    jd_usb_push(Buf, *Len);
    USBD_CDC_ReceivePacket(&USBD_Device);
    return (0);
}

/**
 * @brief  JDUSB_TransmitCplt
 *         Data transmitted callback
 *
 *         @note
 *         This function is IN transfer complete callback used to inform user that
 *         the submitted Data is successfully sent over USB.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t JDUSB_TransmitCplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
    UNUSED(Buf);
    UNUSED(Len);
    UNUSED(epnum);

    maybe_fill_buffer(1);

    return (0);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
