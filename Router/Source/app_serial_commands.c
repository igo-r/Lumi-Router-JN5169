/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_serial_commands.c
 *
 * DESCRIPTION:        Base Device Serial Commands: Coordinator application
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
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "string.h"
#include "dbg.h"
#include "pwrm.h"
#include "uart.h"
#include "PDM.h"
#include "zps_gen.h"
#include "app_router_node.h"
#include "app_zcl_task.h"
#include "app_common.h"
#include "app_serial_commands.h"
#include "app_events.h"
#include "app_main.h"
#include "ZQueue.h"
#include "ZTimer.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_SERIAL
    #define TRACE_SERIAL   FALSE
#else
    #define TRACE_SERIAL   TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

#define MAX_PACKET_SIZE         32

/** Enumerated list of states for receive state machine */
typedef enum
{
    E_STATE_RX_WAIT_START,
    E_STATE_RX_WAIT_TYPEMSB,
    E_STATE_RX_WAIT_TYPELSB,
    E_STATE_RX_WAIT_LENMSB,
    E_STATE_RX_WAIT_LENLSB,
    E_STATE_RX_WAIT_CRC,
    E_STATE_RX_WAIT_DATA,
}teSL_RxState;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vProcessRxChar(uint8 u8Char);
PRIVATE void vProcessCommand(void);
PRIVATE void vWriteTxChar(uint8 u8Char);

PRIVATE uint8 u8SL_CalculateCRC(uint16 u16Type, uint16 u16Length, uint8 *pu8Data);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

uint32    sStorage;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

uint8              au8LinkRxBuffer[32];
uint16             u16PacketType;
uint16             u16PacketLength;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/******************************************************************************
 * NAME: APP_taskAtSerial
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:      Name            Usage
 *
 * RETURNS:
 * None
 ****************************************************************************/
PUBLIC void APP_taskAtSerial(void)
{
    uint8 u8RxByte;
    if (ZQ_bQueueReceive(&APP_msgSerialRx, &u8RxByte))
    {
        DBG_vPrintf(TRACE_SERIAL, "Rx Char %02x\n", u8RxByte);
        vProcessRxChar(u8RxByte);
    }
}

/****************************************************************************
 *
 * NAME: APP_WriteMessageToSerial
 *
 * DESCRIPTION:
 * Write message to the serial link
 *
 * PARAMETERS: Name                   Usage
 * char        message                Message
 * 
 * RETURNS:
 * void
 ****************************************************************************/
PUBLIC void APP_WriteMessageToSerial(const char *message)
{
    DBG_vPrintf(TRACE_SERIAL, "APP_WriteMessageToSerial(%s)\n", message);

    for(; *message != '\0'; message++)
    {
    	vWriteTxChar(*message);
    }
}

/******************************************************************************
 * NAME: vProcessRxChar
 *
 * DESCRIPTION:
 * Processes the received character
 *
 * PARAMETERS:      Name            Usage
 * uint8            u8Char          Character
 *
 * RETURNS:
 * None
 ****************************************************************************/
PRIVATE void vProcessRxChar(uint8 u8Char)
{

    static teSL_RxState eRxState = E_STATE_RX_WAIT_START;
    static uint8 u8CRC;
    static uint16 u16Bytes;
    static bool bInEsc = FALSE;

    switch(u8Char)
    {
        case SL_START_CHAR:
             // Reset state machine
            u16Bytes = 0;
            bInEsc = FALSE;
            DBG_vPrintf(TRACE_SERIAL, "RX Start\n");
            eRxState = E_STATE_RX_WAIT_TYPEMSB;
            break;

        case SL_ESC_CHAR:
            // Escape next character
            bInEsc = TRUE;
            break;

        case SL_END_CHAR:
            // End message
            DBG_vPrintf(TRACE_SERIAL, "Got END\n");
            eRxState = E_STATE_RX_WAIT_START;
            if(u16PacketLength < MAX_PACKET_SIZE)
            {
                if(u8CRC == u8SL_CalculateCRC(u16PacketType, u16PacketLength, au8LinkRxBuffer))
                {
                    /* CRC matches - valid packet */
                    DBG_vPrintf(TRACE_SERIAL, "vProcessRxChar(%d, %d, %02x)\n", u16PacketType, u16PacketLength, u8CRC);
                    vProcessCommand();
                }
            }
            DBG_vPrintf(TRACE_SERIAL, "CRC BAD\n");
            break;

        default:
            if(bInEsc)
            {
                /* Unescape the character */
                u8Char ^= 0x10;
                bInEsc = FALSE;
            }
            DBG_vPrintf(TRACE_SERIAL, "Data 0x%x\n", u8Char & 0xFF);

            switch(eRxState)
            {

            case E_STATE_RX_WAIT_START:
                break;

            case E_STATE_RX_WAIT_TYPEMSB:
                u16PacketType = (uint16)u8Char << 8;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_TYPELSB:
                u16PacketType += (uint16)u8Char;
                DBG_vPrintf(TRACE_SERIAL, "Type 0x%x\n", u16PacketType & 0xFFFF);
                eRxState++;
                break;

            case E_STATE_RX_WAIT_LENMSB:
                u16PacketLength = (uint16)u8Char << 8;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_LENLSB:
                u16PacketLength += (uint16)u8Char;
                DBG_vPrintf(TRACE_SERIAL, "Length %d\n", u16PacketLength);
                if(u16PacketLength > MAX_PACKET_SIZE)
                {
                    DBG_vPrintf(TRACE_SERIAL, "Length > MaxLength\n");
                    eRxState = E_STATE_RX_WAIT_START;
                }
                else
                {
                    eRxState++;
                }
                break;

            case E_STATE_RX_WAIT_CRC:
                DBG_vPrintf(TRACE_SERIAL, "CRC %02x\n\n", u8Char);
                u8CRC = u8Char;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_DATA:
                if(u16Bytes < u16PacketLength)
                {
                    DBG_vPrintf(TRACE_SERIAL, "%02x ", u8Char);
                    au8LinkRxBuffer[u16Bytes++] = u8Char;
                }
                break;
            }
            break;
    }
}

/******************************************************************************
 * NAME: vProcessCommand
 *
 * DESCRIPTION:
 * Processed the received command
 *
 * PARAMETERS:      Name            Usage
 *
 * RETURNS:
 * None
 ****************************************************************************/
PRIVATE void vProcessCommand(void)
{
    switch (u16PacketType)
    {
        case E_SL_MSG_RESET:
            APP_WriteMessageToSerial("Reset...........");
            bResetIssued = TRUE;
            ZTIMER_eStart(u8TmrRestart, ZTIMER_TIME_MSEC(100));
            break;
        case E_SL_MSG_ERASE_PERSISTENT_DATA:
            APP_WriteMessageToSerial("Erase PDM.......");
            PDM_vDeleteAllDataRecords();
            APP_WriteMessageToSerial("Reset...........");
            bResetIssued = TRUE;
            ZTIMER_eStart(u8TmrRestart, ZTIMER_TIME_MSEC(100));
            break;
        default:
            break;
    }
}
/****************************************************************************
 *
 * NAME: vWriteTxByte
 *
 * DESCRIPTION:
 * Write byte to the serial link
 *
 * PARAMETERS: Name                   Usage
 * uint8       u8Char                 TX Char
 *
 * RETURNS:
 * void
 ****************************************************************************/
PRIVATE void vWriteTxChar(uint8 u8Char)
{
    ZPS_eEnterCriticalSection(NULL, &sStorage);

    if (UART_bTxReady() && ZQ_bQueueIsEmpty(&APP_msgSerialTx))
    {
        /* send byte now and enable irq */
        UART_vSetTxInterrupt(TRUE);
        UART_vTxChar(u8Char);
    }
    else
    {
        ZQ_bQueueSend(&APP_msgSerialTx, &u8Char);
    }

    ZPS_eExitCriticalSection(NULL, &sStorage);
}

/****************************************************************************
 *
 * NAME: u8SL_CalculateCRC
 *
 * DESCRIPTION:
 * Calculate CRC of packet
 *
 * PARAMETERS: Name                   RW  Usage
 *             u8Type                 R   Message type
 *             u16Length              R   Message length
 *             pu8Data                R   Message payload
 * RETURNS:
 * CRC of packet
 ****************************************************************************/
PRIVATE uint8 u8SL_CalculateCRC(uint16 u16Type, uint16 u16Length, uint8 *pu8Data)
{

    int n;
    uint8 u8CRC;

    u8CRC  = (u16Type   >> 0) & 0xff;
    u8CRC ^= (u16Type   >> 8) & 0xff;
    u8CRC ^= (u16Length >> 0) & 0xff;
    u8CRC ^= (u16Length >> 8) & 0xff;

    for(n = 0; n < u16Length; n++)
    {
        u8CRC ^= pu8Data[n];
    }

    return(u8CRC);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
