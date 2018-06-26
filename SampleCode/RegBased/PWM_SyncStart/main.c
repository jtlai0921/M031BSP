/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * $Revision: 3 $
 * $Date: 18/06/01 3:49p $
 * @brief    Demonstrate how to use PWM counter synchronous start function.
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define PLLCTL_SETTING  CLK_PLLCTL_96MHz_HXT

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief       PWM0 IRQ Handler
 *
 * @param       None
 *
 * @return      None
 *
 * @details     ISR to handle PWM0 interrupt event
 */
void PWM0_IRQHandler(void)
{

}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~32 MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    /* Waiting for 32MHz clock ready */
    while((CLK->STATUS & CLK_STATUS_HXTSTB_Msk) != CLK_STATUS_HXTSTB_Msk);

    /* Enable HIRC clock */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for HIRC clock ready */
    while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    /* Switch HCLK clock source to HIRC */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_HIRC;

    /* Switch UART0 clock source to XTAL */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART0SEL_Msk) | CLK_CLKSEL1_UART0SEL_HXT;

    /* Switch STCLK source to HCLK/2 */
    CLK->CLKSEL0 = CLK_CLKSEL0_STCLKSEL_HCLK_DIV2;

    /* Enable PWM0 module clock */
    CLK->APBCLK1 |= CLK_APBCLK1_PWM0CKEN_Msk;

    /* Enable PLL and Set PLL frequency (only rev. D & E support PLL) */
//    CLK->PLLCTL = PLLCTL_SETTING;

//    while(!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk));
//    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_PLL;

    /* Enable PWM0 module clock */
    CLK->APBCLK1 |= CLK_APBCLK1_PWM0CKEN_Msk;

    /* Enable PWM1 module clock */
    CLK->APBCLK1 |= CLK_APBCLK1_PWM1CKEN_Msk;

    /*---------------------------------------------------------------------------------------------------------*/
    /* PWM clock frequency configuration                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Select HCLK clock divider as 2 */
    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_HCLKDIV_Msk) | CLK_CLKDIV0_HCLK(2);

    /* PWM clock frequency can be set equal or double to HCLK by choosing case 1 or case 2 */
    /* case 1.PWM clock frequency is set equal to HCLK: select PWM module clock source as PCLK */
    CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_PWM0SEL_Msk) | CLK_CLKSEL2_PWM0SEL_PCLK0;
    CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_PWM1SEL_Msk) | CLK_CLKSEL2_PWM1SEL_PCLK1;

    /* case 2.PWM clock frequency is set double to HCLK: select PWM module clock source as PLL */
    //CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_PWM0SEL_Msk) | CLK_CLKSEL2_PWM0SEL_PLL;
    //CLK->CLKSEL2 = (CLK->CLKSEL2 & ~CLK_CLKSEL2_PWM1SEL_Msk) | CLK_CLKSEL2_PWM1SEL_PLL;
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable UART clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;

    /* Reset PWM0 module */
    SYS->IPRST2 |= SYS_IPRST2_PWM0RST_Msk;
    SYS->IPRST2 &= ~SYS_IPRST2_PWM0RST_Msk;

    /* Reset PWM1 module */
    SYS->IPRST2 |= SYS_IPRST2_PWM1RST_Msk;
    SYS->IPRST2 &= ~SYS_IPRST2_PWM1RST_Msk;

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Set PB multi-function pins for PWM0 Channel 0~5 */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB5MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB5MFP_PWM0_CH0;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB4MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB4MFP_PWM0_CH1;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB3MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB3MFP_PWM0_CH2;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB2MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB2MFP_PWM0_CH3;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB1MFP_PWM0_CH4;
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB0MFP_Msk));
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB0MFP_PWM0_CH5;

    /* Set PB/PC/PA multi-function pins for PWM1 Channel 0~5 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB15MFP_Msk));
    SYS->GPB_MFPH |= SYS_GPB_MFPH_PB15MFP_PWM1_CH0;
    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB14MFP_Msk));
    SYS->GPB_MFPH |= SYS_GPB_MFPH_PB14MFP_PWM1_CH1;
    SYS->GPC_MFPL = (SYS->GPC_MFPL & (~SYS_GPC_MFPL_PC7MFP_Msk));
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC7MFP_PWM1_CH2;
    SYS->GPC_MFPL = (SYS->GPC_MFPL & (~SYS_GPC_MFPL_PC6MFP_Msk ));
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC6MFP_PWM1_CH3;
    SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA7MFP_Msk));
    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA7MFP_PWM1_CH4;
    SYS->GPA_MFPL = (SYS->GPA_MFPL & (~SYS_GPA_MFPL_PA6MFP_Msk ));
    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA6MFP_PWM1_CH5;
}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART IP */
    SYS->IPRST1 |=  SYS_IPRST1_UART0RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_UART0RST_Msk;

    /* Configure UART0 and set UART0 baud rate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    /* Init System, IP clock and multi-function I/O
       In the end of SYS_Init() will issue SYS_LockReg()
       to lock protected register. If user want to write
       protected register, please issue SYS_UnlockReg()
       to unlock protected register if necessary */

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART to 115200-8n1 for print message */
    UART0_Init();

    printf("\n\nCPU @ %dHz(PLL@ %dHz)\n", SystemCoreClock, PllClock);
    printf("PWM0 clock is from %s\n", (CLK->CLKSEL2 & CLK_CLKSEL2_PWM0SEL_Msk) ? "CPU" : "PLL");
    printf("PWM1 clock is from %s\n", (CLK->CLKSEL2 & CLK_CLKSEL2_PWM1SEL_Msk) ? "CPU" : "PLL");
    printf("+------------------------------------------------------------------------+\n");
    printf("|                          PWM Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");
    printf("  This sample code will output waveform with PWM0 and PWM1 channel 0~5 at the same time.\n");
    printf("  I/O configuration:\n");
    printf("    waveform output pin: PWM0_CH0(PB.5), PWM0_CH1(PB.4), PWM0_CH2(PB.3), PWM0_CH3(PB.2), PWM0_CH4(PB.1), PWM0_CH5(PB.0)\n");
    printf("                         PWM1_CH0(PB.15), PWM1_CH1(PB.14), PWM1_CH2(PC.7), PWM1_CH3(PC.6), PWM1_CH4(PA.7), PWM1_CH5(PA.6)\n");

    /* PWM0 and PWM1 channel 0~5 frequency and duty configuration are as follows */

    /*
      Configure PWM0 channel 0 init period and duty(up counter type).
      Period is PLL / (prescaler * (CNR + 1))
      Duty ratio = (CMR) / (CNR + 1)
      Period = 48 MHz / (2 * (199 + 1)) = 120000 Hz
      Duty ratio = (100) / (199 + 1) = 50%
    */

    /* Set PWM to up counter type(edge aligned) */
    PWM0->CTL1 &= ~PWM_CTL1_CNTTYPE0_Msk;

    /* PWM0 channel 0 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM0, 0, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 0, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 0, 399);

    /* PWM0 channel 1 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 1 is share with Channel 0 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 1, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 1, 399);

    /* PWM0 channel 2 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM0, 2, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 2, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 2, 399);

    /* PWM0 channel 3 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 3 is share with Channel 2 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 3, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 3, 399);

    /* PWM0 channel 4 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM0, 4, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 4, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 4, 399);

    /* PWM0 channel 5 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 5 is share with Channel 4 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM0, 5, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM0, 5, 399);

    /* Set waveform generation */
    PWM_SET_OUTPUT_LEVEL(PWM0, 0x3F, PWM_OUTPUT_HIGH, PWM_OUTPUT_LOW, PWM_OUTPUT_NOTHING, PWM_OUTPUT_NOTHING);

    /* Enable output of PWM0 channel 0 ~ 5 */
    PWM0->POEN |= 0x3F;

    /* Set PWM to up counter type(edge aligned) */
    PWM1->CTL1 &= ~PWM_CTL1_CNTTYPE0_Msk;

    /* PWM1 channel 0 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM1, 0, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 0, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 0, 399);

    /* PWM1 channel 1 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 1 is share with Channel 0 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 1, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 1, 399);

    /* PWM1 channel 2 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM1, 2, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 2, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 2, 399);

    /* PWM1 channel 3 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 3 is share with Channel 2 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 3, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 3, 399);

    /* PWM1 channel 4 frequency and duty configuration */
    /* Set PWM Timer clock prescaler */
    PWM_SET_PRESCALER(PWM1, 4, 0); /* Divided by 1 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 4, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 4, 399);

    /* PWM1 channel 5 frequency and duty configuration */
    /* PWM Timer clock prescaler of Channel 5 is share with Channel 4 */
    /* Set PWM Timer duty */
    PWM_SET_CMR(PWM1, 5, 200);
    /* Set PWM Timer period */
    PWM_SET_CNR(PWM1, 5, 399);

    /* Set waveform generation */
    PWM_SET_OUTPUT_LEVEL(PWM1, 0x3F, PWM_OUTPUT_HIGH, PWM_OUTPUT_LOW, PWM_OUTPUT_NOTHING, PWM_OUTPUT_NOTHING);

    /* Enable output of PWM1 channel 0 ~ 5 */
    PWM1->POEN |= 0x3F;

    /* Enable counter synchronous start function for PWM0 and PWM1 channel 0~5 */
    PWM0->SSCTL = 0x3F | PWM_SSCTL_SSRC_PWM0;
    PWM1->SSCTL = 0x3F | PWM_SSCTL_SSRC_PWM0;

    printf("Press any key to start.\n");
    getchar();

    /* Trigger PWM counter synchronous start by PWM0 */
    PWM0->SSTRG = PWM_SSTRG_CNTSEN_Msk;

    printf("Done.");
    while(1);

}
