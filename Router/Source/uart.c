/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          uart.c
 *
 * DESCRIPTION:        UART interface for Base Device application
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2016. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "AppHardwareApi.h"

#include "dbg.h"

#include "uart.h"
#include "ZQueue.h"
#include "app_main.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_UART
#define TRACE_UART  TRUE
#endif

/* default to uart 0 */
#ifndef UART
#define UART E_AHI_UART_0
#endif

#if (UART != E_AHI_UART_0 && UART != E_AHI_UART_1)
#error UART must be either 0 or 1
#endif

/* default BAUD rate 9600 */
#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE        115200
#endif

#define UART_LCR_OFFSET     0x0C
#define UART_DLM_OFFSET     0x04

#if (UART == E_AHI_UART_0)
#define UART_START_ADR      0x02003000UL
#elif (UART == E_AHI_UART_1)
#define UART_START_ADR      0x02004000UL
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

uint8 txbuf[16];
uint8 rxbuf[127];

/****************************************************************************
 *
 * NAME: UART_vInit
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
PUBLIC void UART_vInit(void)
{
    DBG_vPrintf(TRACE_UART, "Initialising UART ...");

    bAHI_UartEnable(UART, txbuf, (uint8)16, rxbuf, (uint8)127);

    vAHI_UartReset(UART, TRUE, TRUE);
    vAHI_UartReset(UART, FALSE, FALSE);

    /* Set the clock divisor register to give required buad, this has to be done
       directly as the normal routines (in ROM) do not support all baud rates */
    UART_vSetBaudRate(UART_BAUD_RATE);

    vAHI_UartSetRTSCTS(UART, TRUE);
    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE);
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1);

    DBG_vPrintf(TRACE_UART, "Done\n");
}

/****************************************************************************
 *
 * NAME: UART_vSetBaudRate
 *
 * DESCRIPTION:
 *
 * PARAMETERS: Name        RW  Usage
 *
 * RETURNS:
 *
 ****************************************************************************/

PUBLIC void UART_vSetBaudRate(uint32 u32BaudRate)
{
    uint16    u16Divisor      =  0;
    uint32    u32Remainder;
    uint8     u8ClocksPerBit  =  16;
    uint32    u32CalcBaudRate =  0;
    int32     i32BaudError    =  0x7FFFFFFF;

    while (abs(i32BaudError) > (int32)(u32BaudRate >> 4)) /* 6.25% (100/16) error */
    {
        if (--u8ClocksPerBit < 3)
        {
            return;
        }

        /* Calculate Divisor register = 16MHz / (16 x baud rate) */
        u16Divisor = (uint16)(16000000UL / ((u8ClocksPerBit+1) * u32BaudRate));

        /* Correct for rounding errors */
        u32Remainder = (uint32)(16000000UL % ((u8ClocksPerBit+1) * u32BaudRate));

        if (u32Remainder >= (((u8ClocksPerBit+1) * u32BaudRate) / 2))
        {
            u16Divisor += 1;
        }

        u32CalcBaudRate = (16000000UL / ((u8ClocksPerBit+1) * u16Divisor));

        i32BaudError = (int32)u32CalcBaudRate - (int32)u32BaudRate;
    }

    /* Set the calculated clocks per bit */
    vAHI_UartSetClocksPerBit(UART, u8ClocksPerBit);

    /* Set the calculated divisor */
    vAHI_UartSetBaudDivisor(UART, u16Divisor);
}

/****************************************************************************
 *
 * NAME: APP_isrUart
 *
 * DESCRIPTION: Handle interrupts from uart
 *
 * PARAMETERS:      Name            RW  Usage
 * None.
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
void APP_isrUart(void)
{
    uint32    u32ItemBitmap = ((*((volatile uint32 *)(UART_START_ADR + 0x08))) >> 1) & 0x0007;
    uint8     u8Byte;

    if (u32ItemBitmap & E_AHI_UART_INT_RXDATA) {
        u8Byte = u8AHI_UartReadData(UART);
        ZQ_bQueueSend(&APP_msgSerialRx, &u8Byte);
    }
    else if (u32ItemBitmap & E_AHI_UART_INT_TX) {
        if (ZQ_bQueueReceive(&APP_msgSerialTx, &u8Byte)) {
            UART_vSetTxInterrupt(TRUE);
            vAHI_UartWriteData(UART, u8Byte);
        } else {
            /* disable tx interrupt as nothing to send */
            UART_vSetTxInterrupt(FALSE);
        }
    }
}

/****************************************************************************
 *
 * NAME: UART_vRtsStopFlow
 *
 * DESCRIPTION:
 * Set UART RS-232 RTS line high to stop any further data coming in
 *
 ****************************************************************************/

PUBLIC void UART_vRtsStopFlow(void)
{
    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, E_AHI_UART_RTS_HIGH);
}

/****************************************************************************
 *
 * NAME: UART_vRtsStartFlow
 *
 * DESCRIPTION:
 * Set UART RS-232 RTS line low to allow further data
 *
 ****************************************************************************/

PUBLIC void UART_vRtsStartFlow(void)
{
    vAHI_UartSetControl(UART, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, E_AHI_UART_RTS_LOW);
}
/* [I SP001222_P1 283] end */

/****************************************************************************
 *
 * NAME: UART_vTxChar
 *
 * DESCRIPTION:
 * Set UART RS-232 RTS line low to allow further data
 *
 ****************************************************************************/
PUBLIC void UART_vTxChar(uint8 u8Char)
{
    while (!UART_bTxReady() && !(u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_TEMT));
    vAHI_UartWriteData(UART, u8Char);
    while (!UART_bTxReady() && !(u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_TEMT));
}

/****************************************************************************
 *
 * NAME: UART_bTxReady
 *
 * DESCRIPTION:
 * Set UART RS-232 RTS line low to allow further data
 *
 ****************************************************************************/
PUBLIC bool_t UART_bTxReady()
{
    return u8AHI_UartReadLineStatus(UART) & E_AHI_UART_LS_THRE;
}

/****************************************************************************
 *
 * NAME: UART_vSetTxInterrupt
 *
 * DESCRIPTION:
 * Enable / disable the tx interrupt
 *
 ****************************************************************************/
PUBLIC void UART_vSetTxInterrupt(bool_t bState)
{
    vAHI_UartSetInterrupt(UART, FALSE, FALSE, bState, TRUE, E_AHI_UART_FIFO_LEVEL_1);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
