/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * $Revision: 4 $
 * $Date: 18/05/31 5:44p $
 * @brief
 *           Transmit and receive data with auto flow control.
 *           This sample code needs to work with USCI_UART_Autoflow_Master.
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

#define RXBUFSIZE  1024

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint8_t g_u8RecData[RXBUFSIZE]  = {0};
volatile int32_t g_i32pointer = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void USCI_AutoFlow_FunctionRxTest(void);

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~32 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for 32MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to HIRC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART0 clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Switch UART0 clock source to XTAL */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

    /* Enable UUART0 clock */
    CLK_EnableModuleClock(USCI0_MODULE);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Set UUART0 multi-function pins */
    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA8MFP_Msk)) | SYS_GPA_MFPH_PA8MFP_USCI0_CTL1;
    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA9MFP_Msk)) | SYS_GPA_MFPH_PA9MFP_USCI0_DAT1;
    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA10MFP_Msk)) | SYS_GPA_MFPH_PA10MFP_USCI0_DAT0;
    SYS->GPC_MFPH = (SYS->GPC_MFPH & ~(SYS_GPC_MFPH_PC14MFP_Msk)) | SYS_GPC_MFPH_PC14MFP_USCI0_CTL0;

    /* Lock protected registers */
    SYS_LockReg();
}

void USCI0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init USCI                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Configure USCI0 as UART mode */
    UUART_Open(UUART0, 115200);
}

int main()
{
    SYS_Init();

    /* Init UART0 to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    /* Init USCI0 for test */
    USCI0_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\nUSCI UART Sample Program\n");

    /* USCI UART auto flow sample slave function */
    USCI_AutoFlow_FunctionRxTest();

    while (1);
}

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle USCI interrupt event                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void USCI_IRQHandler(void)
{
    uint32_t u32Temp;
    volatile uint32_t u32ProtSts = UUART_GET_PROT_STATUS(UUART0);
    volatile uint32_t u32BufSts = UUART_GET_BUF_STATUS(UUART0);

    if (u32ProtSts & UUART_PROTSTS_RXENDIF_Msk)     /* Receive end interrupt */
    {
        /* Handle received data */
        UUART_CLR_PROT_INT_FLAG(UUART0, UUART_PROTSTS_RXENDIF_Msk);
        u32Temp = UUART_READ(UUART0);
        g_u8RecData[g_i32pointer] = u32Temp;
        g_i32pointer++;
    }
    else if (u32BufSts & UUART_BUFSTS_RXOVIF_Msk)      /* Receive buffer over-run error interrupt */
    {
        UUART_CLR_BUF_INT_FLAG(UUART0, UUART_BUFSTS_RXOVIF_Msk);
        printf("\nRx buffer is over-run.");
    }
}
/*---------------------------------------------------------------------------------------------------------*/
/*  AutoFlow Function Test (Slave)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void USCI_AutoFlow_FunctionRxTest()
{
    uint32_t u32i;

    printf("\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|     Pin Configure                                         |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|  ______                                            _____  |\n");
    printf("| |      |                                          |     | |\n");
    printf("| |Master|                                          |Slave| |\n");
    printf("| |    TX|--USCI0_DAT1   <======> USCI0_DAT0      --|RX   | |\n");
    printf("| |  nCTS|--USCI0_CTL0   <======> USCI0_CTL1      --|nRTS | |\n");
    printf("| |______|                                          |_____| |\n");
    printf("|                                                           |\n");
    printf("+-----------------------------------------------------------+\n");

    printf("\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|       AutoFlow Function Test (Slave)                      |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|  Description :                                            |\n");
    printf("|    The sample code needs two boards. One is Master and    |\n");
    printf("|    the other is slave. Master will send 1k bytes data     |\n");
    printf("|    to slave. Slave will check if received data is correct |\n");
    printf("|    after getting 1k bytes data.                           |\n");
    printf("|    Press any key to start...                              |\n");
    printf("+-----------------------------------------------------------+\n");
    getchar();

    /* Enable CTS and RTS auto flow control */
    UUART_EnableFlowCtrl(UUART0);

    /* Enable USCI receive end and receive buffer over-run error Interrupt */
    UUART_EnableInt(UUART0, UUART_RXEND_INT_MASK | UUART_BUF_RXOV_INT_MASK);
    NVIC_EnableIRQ(USCI_IRQn);

    printf("\n Starting to receive data...\n");

    /* Wait for receive 1k bytes data */
    while (g_i32pointer < RXBUFSIZE);

    /* Compare Data */
    for (u32i = 0; u32i < RXBUFSIZE; u32i++)
    {
        if (g_u8RecData[u32i] != (u32i & 0xFF))
        {
            printf("\n Compare Data Failed  \n");
            printf("u32i = %d\n", u32i);
            printf("g_u8RecData[%d] = %d\n", u32i, g_u8RecData[u32i]);
            while (1);
        }
    }

    printf("\n Receive OK & Check OK\n");

    /* Disable USCI interrupt */
    UUART_DisableInt(UUART0, UUART_RXEND_INT_MASK | UUART_BUF_RXOV_INT_MASK);
    NVIC_DisableIRQ(USCI_IRQn);

}

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
