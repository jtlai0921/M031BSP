/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 9 $
 * $Date: 18/05/31 2:01p $
 * @brief    Demonstrate how to implement a composite device.(VCOM and HID keyboard)
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"
#include "VCOM_And_hid_keyboard.h"

/*--------------------------------------------------------------------------*/
STR_VCOM_LINE_CODING gLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
uint16_t gCtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */

/*--------------------------------------------------------------------------*/
#define RX_BUFSIZE           512 /* RX buffer size */
#define TX_BUFSIZE           512 /* RX buffer size */
#define TX_FIFO_SIZE        64  /* TX Hardware FIFO size */

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/* UART0 */
volatile uint8_t comRbuf[RX_BUFSIZE];
volatile uint16_t comRbytes = 0;
volatile uint16_t comRhead = 0;
volatile uint16_t comRtail = 0;

volatile uint8_t comTbuf[TX_BUFSIZE];
volatile uint16_t comTbytes = 0;
volatile uint16_t comThead = 0;
volatile uint16_t comTtail = 0;

uint8_t gRxBuf[64] = {0};
uint8_t *gpu8RxBuf = 0;
uint32_t gu32RxSize = 0;
uint32_t gu32TxSize = 0;

volatile int8_t gi8BulkOutReady = 0;

void SYS_Init(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~32 MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    /* Enable Internal High speed RC oscillator (HIRC) */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for 32MHz clock ready */
    while((CLK->STATUS & CLK_STATUS_HXTSTB_Msk) != CLK_STATUS_HXTSTB_Msk);

    /* Waiting for Internal High speed RC clock ready */
    while((CLK->STATUS & CLK_STATUS_HIRCSTB_Msk) != CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to HIRC */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk ) | CLK_CLKSEL0_HCLKSEL_HIRC ;

    /* Switch UART0 clock source to XTAL */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART0SEL_Msk) | CLK_CLKSEL1_UART0SEL_HXT;

    /* Switch USB clock source to HIRC */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_USBDSEL_Msk) | CLK_CLKSEL0_HCLKSEL_HIRC;

    /* USB Clock = HIRC / 1 */
    CLK->CLKDIV0 = CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk;

    /* Enable UART0 clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk ;

    /* Enable USB clock */
    CLK->APBCLK0 |= CLK_APBCLK0_USBDCKEN_Msk ;

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Lock protected registers */
    SYS_LockReg();
}

/*---------------------------------------------------------------------------------------------------------*/
/* UART Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void UART02_IRQHandler(void)
{
    uint8_t bInChar;
    int32_t size;
    uint32_t u32IntStatus;

    u32IntStatus = UART0->INTSTS;

    if((u32IntStatus & UART_INTSTS_RDAIF_Msk) || (u32IntStatus & UART_INTSTS_RXTOIF_Msk))
    {
        /* Receiver FIFO threshold level is reached or Rx time out */

        /* Get all the input characters */
        while ((UART0->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0)
        {
            /* Get the character from UART Buffer */
            bInChar = UART0->DAT;

            /* Check if buffer full */
            if(comRbytes < RX_BUFSIZE)
            {
                /* Enqueue the character */
                comRbuf[comRtail++] = bInChar;
                if(comRtail >= RX_BUFSIZE)
                    comRtail = 0;
                comRbytes++;
            }
            else
            {
                /* FIFO over run */
            }
        }
    }

    if(u32IntStatus & UART_INTSTS_THREIF_Msk)
    {

        if(comTbytes)
        {
            /* Fill the Tx FIFO */
            size = comTbytes;
            if(size >= TX_FIFO_SIZE)
            {
                size = TX_FIFO_SIZE;
            }

            while(size)
            {
                bInChar = comTbuf[comThead++];
                UART0->DAT = bInChar;
                if(comThead >= TX_BUFSIZE)
                    comThead = 0;
                comTbytes--;
                size--;
            }
        }
        else
        {
            /* No more data, just stop Tx (Stop work) */
            UART0->INTEN &= ~UART_INTEN_THREIEN_Msk;
        }
    }
}

void VCOM_TransferData(void)
{
    int32_t i, i32Len;

    /* Check whether USB is ready for next packet or not*/
    if(gu32TxSize == 0)
    {
        /* Check whether we have new COM Rx data to send to USB or not */
        if(comRbytes)
        {
            i32Len = comRbytes;
            if(i32Len > EP2_MAX_PKT_SIZE)
                i32Len = EP2_MAX_PKT_SIZE;

            for(i = 0; i < i32Len; i++)
            {
                gRxBuf[i] = comRbuf[comRhead++];
                if(comRhead >= RX_BUFSIZE)
                    comRhead = 0;
            }

            __set_PRIMASK(1);
            comRbytes -= i32Len;
            __set_PRIMASK(0);

            gu32TxSize = i32Len;
            USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2)), (uint8_t *)gRxBuf, i32Len);
            USBD_SET_PAYLOAD_LEN(EP2, i32Len);
        }
        else
        {
            /* Prepare a zero packet if previous packet size is EP2_MAX_PKT_SIZE and
               no more data to send at this moment to note Host the transfer has been done */
            i32Len = USBD_GET_PAYLOAD_LEN(EP2);
            if(i32Len == EP2_MAX_PKT_SIZE)
                USBD_SET_PAYLOAD_LEN(EP2, 0);
        }
    }

    /* Process the Bulk out data when bulk out data is ready. */
    if(gi8BulkOutReady && (gu32RxSize <= TX_BUFSIZE - comTbytes))
    {
        for(i = 0; i < gu32RxSize; i++)
        {
            comTbuf[comTtail++] = gpu8RxBuf[i];
            if(comTtail >= TX_BUFSIZE)
                comTtail = 0;
        }

        __set_PRIMASK(1);
        comTbytes += gu32RxSize;
        __set_PRIMASK(0);

        gu32RxSize = 0;
        gi8BulkOutReady = 0; /* Clear bulk out ready flag */

        /* Ready to get next BULK out */
        USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
    }

    /* Process the software Tx FIFO */
    if(comTbytes)
    {
        /* Check if Tx is working */
        if((UART0->INTEN & UART_INTEN_THREIEN_Msk) == 0)
        {
            /* Send one bytes out */
            UART0->DAT = comTbuf[comThead++];
            if(comThead >= TX_BUFSIZE)
                comThead = 0;

            __set_PRIMASK(1);
            comTbytes--;
            __set_PRIMASK(0);

            /* Enable Tx Empty Interrupt. (Trigger first one) */
            UART0->INTEN |= UART_INTEN_THREIEN_Msk;
        }
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Init UART0 to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    printf("\n\n");
    printf("+--------------------------------------------------------------+\n");
    printf("|     NuMicro USB Virtual COM and HID Keyboard Sample Code     |\n");
    printf("+--------------------------------------------------------------+\n");
    printf("If PB.15 = 0, just report it is key 'a'.\n");

    /* Set PB.15 as Quasi-bidirectional mode */
    PB->MODE = (PB->MODE & ~GPIO_MODE_MODE15_Msk) | (GPIO_MODE_QUASI << GPIO_MODE_MODE15_Pos);

    /* Open USB controller */
    USBD_Open(&gsInfo, HID_ClassRequest, NULL);

    /* Endpoint configuration */
    HID_Init();

    /* Start USB device */
    USBD_Start();

    NVIC_EnableIRQ(USBD_IRQn);

    NVIC_EnableIRQ(UART02_IRQn);

    while(1)
    {
        VCOM_TransferData();
        HID_UpdateKbData();
    }
}



/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

