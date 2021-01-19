/*****************************************************************************
 *
 * MODULE:          JN-AN-1217 Base Device application
 *
 * COMPONENT:       app_nci_icode.c
 *
 * DESCRIPTION:     Base Device - Application layer for NCI (Installation Code encryption)
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
 * Copyright NXP B.V. 2017. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "dbg.h"
#include "ZTimer.h"
#include "pwrm.h"
#include "PDM.h"
#include "bdb_DeviceCommissioning.h"
#include "nci.h"
#include "nci_nwk.h"
#include "app_nci_icode.h"
//#include "app_icode.h"
#include "app_main.h"
#ifdef LITTLE_ENDIAN_PROCESSOR
    #include "portmacro.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_APP_NCI
    #define TRACE_APP_NCI   TRUE
#else
    #define TRACE_APP_NCI   FALSE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/* APP_NCI States */
typedef enum
{
    E_APP_NCI_STATE_NONE,           // 0
    E_APP_NCI_STATE_ABSENT,         // 1
    E_APP_NCI_STATE_PRESENT         // 2
} teAppNciState;

/* APP_NCI Modes */
typedef enum
{
    E_APP_NCI_MODE_APP,             // 0
    E_APP_NCI_MODE_NWK              // 1
} teAppNciMode;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE bool_t APP_bNciNtagCmdJoinWithCode(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint32                      u32AppNciTicks;
PRIVATE uint32                      u32AppNciMs;
PRIVATE teAppNciState               eAppNciState;
PRIVATE teAppNciMode                eAppNciMode;
PRIVATE tsNfcNwkPayload             sNfcNwkPayload;
PRIVATE uint8                      u8Endpoint;
PRIVATE uint32                    u32NfcNwkAddress;
PRIVATE bool_t                      bNciTimer;
PRIVATE uint8                      u8Sequence;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vNciStart
 *
 * DESCRIPTION:
 * Starts NCI processing
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vNciStart(uint8 u8ApplicationEndpoint)
{
    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_vNciStart(x%02x) ICODE", u32AppNciMs, u8ApplicationEndpoint);
    /* Save the endpoint for later */
    u8Endpoint = u8ApplicationEndpoint;
    /* Initialise main NTAG state machine */
    NCI_vInitialise(APP_NCI_ADDRESS,
                     APP_NCI_I2C_LOCATION,
                     APP_NCI_I2C_FREQUENCY_HZ,
                     APP_NCI_VEN_PIN,
                     APP_NCI_IRQ_PIN);
    /* Set state and mode */
    eAppNciState = E_APP_NCI_STATE_NONE;
    eAppNciMode  = E_APP_NCI_MODE_APP;
    /* Register callback */
    NCI_vRegCbEvent(APP_cbNciEvent);
    /* Flag that the timer should run */
    bNciTimer = TRUE;
    /* Start the timer */
    ZTIMER_eStart(u8TimerNci, APP_NCI_TICK_MS);
}

/****************************************************************************
 *
 * NAME: APP_vNciStop
 *
 * DESCRIPTION:
 * Stops NCI processing
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vNciStop(void)
{
    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_vNciStop()", u32AppNciMs);
    /* Are we in network mode ? */
    if (eAppNciMode == E_APP_NCI_MODE_NWK)
    {
        /* Stop network processing */
        (void) NCI_NWK_eStop();
        /* Go to application mode */
        eAppNciMode = E_APP_NCI_MODE_APP;
        /* Register callback */
        NCI_vRegCbEvent(APP_cbNciEvent);
    }
}

/****************************************************************************
 *
 * NAME: APP_cbNciTimer
 *
 * DESCRIPTION:
 * Timer callback function
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_cbNciTimer(void *pvParams)
{
    uint8 u8Byte;

    /* Restart the timer */
    if (bNciTimer) ZTIMER_eStart(u8TimerNci, APP_NCI_TICK_MS);
    /* Debug */
    /*if (u32AppNciTicks % 100 == 0) DBG_vPrintf(TRACE_APP_NCI, "."); */
    /* Increment tick counter and timer value */
    u32AppNciTicks++;
    u32AppNciMs += APP_NCI_TICK_MS;
//    #warning PermitJoin off
//    ZPS_eAplZdoPermitJoining(0);
    /* Which mode are we in ? */
    switch (eAppNciMode)
    {
        /* Network ? */
        case E_APP_NCI_MODE_NWK:
        {
            teNciNwkStatus eNciNwkStatus;

            /* Maintain network NTAG state machine */
            eNciNwkStatus = NCI_NWK_eTick(APP_NCI_TICK_MS);
            /* Finished reading ntag data ? */
            if (E_NCI_NWK_READ_FAIL == eNciNwkStatus
            ||  E_NCI_NWK_READ_OK   == eNciNwkStatus)
            {
                bool_t bWrite     = FALSE;

                /* Debug */
                DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_cbNciTimer()", u32AppNciMs);
                DBG_vPrintf(TRACE_APP_NCI, ", NCI_NWK_eTick() = %d", eNciNwkStatus);
                /* Read ok ? */
                if (E_NCI_NWK_READ_OK == eNciNwkStatus)
                {
                    /* Is the read data valid ? */
                    if (sNfcNwkPayload.sNtag.u8Version == NFC_NWK_PAYLOAD_VERSION)
                    {
                        /* Debug */
                        DBG_vPrintf(TRACE_APP_NCI, ", VALID");
                        /* Debug data */
                        DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwkPayload (Read)");
                        DBG_vPrintf(TRACE_APP_NCI, "\n        sNtag");
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Version       = %d",    sNfcNwkPayload.sNtag.u8Version);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Command       = x%02x", sNfcNwkPayload.sNtag.u8Command);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Sequence      = %d",    sNfcNwkPayload.sNtag.u8Sequence);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16DeviceId     = x%04x", sNfcNwkPayload.sNtag.u16DeviceId);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtAddress   = %08x:%08x", (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress >> 32), (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress & 0xFFFFFFFF));
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16ShortAddress = x%04x", sNfcNwkPayload.sNtag.u16ShortAddress);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Channel       = %d",    sNfcNwkPayload.sNtag.u8Channel);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16PanId        = x%04x", sNfcNwkPayload.sNtag.u16PanId);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtPanId     = %08x:%08x", (uint32)(sNfcNwkPayload.sNtag.u64ExtPanId >> 32), (uint32)(sNfcNwkPayload.sNtag.u64ExtPanId & 0xFFFFFFFF));
                        DBG_vPrintf(TRACE_APP_NCI, "\n            au8Key          =");
                        for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNtag.au8Key[u8Byte]);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16Crc          = x%04x", sNfcNwkPayload.sNtag.u16Crc);
                        DBG_vPrintf(TRACE_APP_NCI, "\n        sNci");
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Command       = x%02x", sNfcNwkPayload.sNci.u8Command);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Sequence      = %d",    sNfcNwkPayload.sNci.u8Sequence);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16DeviceId     = x%04x", sNfcNwkPayload.sNci.u16DeviceId);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtAddress   = %08x:%08x", (uint32)(sNfcNwkPayload.sNci.u64ExtAddress >> 32), (uint32)(sNfcNwkPayload.sNci.u64ExtAddress & 0xFFFFFFFF));
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16ShortAddress = x%04x", sNfcNwkPayload.sNci.u16ShortAddress);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8Channel       = %d",    sNfcNwkPayload.sNci.u8Channel);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u16PanId        = x%04x", sNfcNwkPayload.sNci.u16PanId);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtPanId     = %08x:%08x", (uint32)(sNfcNwkPayload.sNci.u64ExtPanId >> 32), (uint32)(sNfcNwkPayload.sNci.u64ExtPanId & 0xFFFFFFFF));
                        DBG_vPrintf(TRACE_APP_NCI, "\n            au8Key          =");
                        for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNci.au8Key[u8Byte]);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            au8Mic          =");
                        for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_MIC_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNci.au8Mic[u8Byte]);
                        DBG_vPrintf(TRACE_APP_NCI, "\n            u8KeySeqNum     = x%02x", sNfcNwkPayload.sNci.u8KeySeqNum);

                        /* Which command ? */
                        switch (sNfcNwkPayload.sNtag.u8Command)
                        {
                            case NFC_NWK_NTAG_CMD_JOIN_WITH_CODE: bWrite = APP_bNciNtagCmdJoinWithCode(); break;
                            default:                                                                      break;
                        }

                        /* Want to write data ? */
                        if (bWrite)
                        {
                            /* Debug data */
                            DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwkPayload (Write)");
                            DBG_vPrintf(TRACE_APP_NCI, "\n        sNtag");
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Version       = %d",    sNfcNwkPayload.sNtag.u8Version);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Command       = x%02x", sNfcNwkPayload.sNtag.u8Command);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Sequence      = %d",    sNfcNwkPayload.sNtag.u8Sequence);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16DeviceId     = x%04x", sNfcNwkPayload.sNtag.u16DeviceId);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtAddress   = %08x:%08x", (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress >> 32), (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress & 0xFFFFFFFF));
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16ShortAddress = x%04x", sNfcNwkPayload.sNtag.u16ShortAddress);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Channel       = %d",    sNfcNwkPayload.sNtag.u8Channel);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16PanId        = x%04x", sNfcNwkPayload.sNtag.u16PanId);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtPanId     = %08x:%08x", (uint32)(sNfcNwkPayload.sNtag.u64ExtPanId >> 32), (uint32)(sNfcNwkPayload.sNtag.u64ExtPanId & 0xFFFFFFFF));
                            DBG_vPrintf(TRACE_APP_NCI, "\n            au8Key          =");
                            for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNtag.au8Key[u8Byte]);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16Crc          = x%04x", sNfcNwkPayload.sNtag.u16Crc);
                            DBG_vPrintf(TRACE_APP_NCI, "\n        sNci");
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Command       = x%02x", sNfcNwkPayload.sNci.u8Command);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Sequence      = %d",    sNfcNwkPayload.sNci.u8Sequence);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16DeviceId     = x%04x", sNfcNwkPayload.sNci.u16DeviceId);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtAddress   = %08x:%08x", (uint32)(sNfcNwkPayload.sNci.u64ExtAddress >> 32), (uint32)(sNfcNwkPayload.sNci.u64ExtAddress & 0xFFFFFFFF));
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16ShortAddress = x%04x", sNfcNwkPayload.sNci.u16ShortAddress);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8Channel       = %d",    sNfcNwkPayload.sNci.u8Channel);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u16PanId        = x%04x", sNfcNwkPayload.sNci.u16PanId);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u64ExtPanId     = %08x:%08x", (uint32)(sNfcNwkPayload.sNci.u64ExtPanId >> 32), (uint32)(sNfcNwkPayload.sNci.u64ExtPanId & 0xFFFFFFFF));
                            DBG_vPrintf(TRACE_APP_NCI, "\n            au8Key          =");
                            for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNci.au8Key[u8Byte]);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            au8Mic          =");
                            for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_MIC_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNci.au8Mic[u8Byte]);
                            DBG_vPrintf(TRACE_APP_NCI, "\n            u8KeySeqNum     = x%02x", sNfcNwkPayload.sNci.u8KeySeqNum);
                            /* Start the write back to the same address */
                            eNciNwkStatus = NCI_NWK_eWrite(&u32NfcNwkAddress, &sNfcNwkPayload);
                        }
                        else
                        {
                        /* Debug */
                            DBG_vPrintf(TRACE_APP_NCI, ", !WRITE");
                        }
                    }
                    /* Invalid data ? */
                    else
                    {
                        /* Debug */
                        DBG_vPrintf(TRACE_APP_NCI, ", INVALID");
                    }
                }
                /* Failed to read ? */
                else
                {
                    /* Debug */
                    DBG_vPrintf(TRACE_APP_NCI, ", FAILED");
                }

                /* Not writing ? */
                if (bWrite == FALSE)
                {
                    /* Stop network processing */
                    eNciNwkStatus = NCI_NWK_eStop();
                    /* Go to application mode */
                    eAppNciMode = E_APP_NCI_MODE_APP;
                    /* Register callback */
                    NCI_vRegCbEvent(APP_cbNciEvent);
                }
            }
            /* Finished writing nci data or wnet idle unexpectedly ? */
            else if (E_NCI_NWK_WRITE_FAIL == eNciNwkStatus
            ||       E_NCI_NWK_WRITE_OK   == eNciNwkStatus
            ||       E_NCI_NWK_IDLE       == eNciNwkStatus)
            {
                /* Debug */
                DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_cbNtagTimer()", u32AppNciMs);
                DBG_vPrintf(TRACE_APP_NCI, ", NCI_NWK_eTick() = %d", eNciNwkStatus);
                /* Stop network processing */
                eNciNwkStatus = NCI_NWK_eStop();
                /* Go to application mode */
                eAppNciMode = E_APP_NCI_MODE_APP;
                /* Register callback */
                NCI_vRegCbEvent(APP_cbNciEvent);
            }
        }
        break;

        /* Others (application mode) ? */
        default:
        {
            /* Maintain driver NTAG state machine */
            NCI_vTick(APP_NCI_TICK_MS);
        }
        break;
    }
}

/****************************************************************************
 *
 * NAME: APP_cbNciEvent
 *
 * DESCRIPTION:
 * Called when a tag event takes place
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC  void        APP_cbNciEvent( /* Called when an event takes place */
        teNciEvent  eNciEvent,          /* Event raised */
        uint32      u32Address,
        uint32      u32Length,
        uint8       *pu8Data)           /* Event data (NULL if no data) */
{
    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_cbNciEvent(%d, %d, %d)", u32AppNciMs, eNciEvent, u32Address, u32Length);
    /* Which event ? */
    switch (eNciEvent)
    {
        /* Present ? */
        case E_NCI_EVENT_PRESENT:
        {
            /* Not already present ? */
            if (E_APP_NCI_STATE_PRESENT != eAppNciState)
            {
                /* Debug */
                DBG_vPrintf(TRACE_APP_NCI, ", eAppNciState = PRESENT");
                /* Go to present state */
                eAppNciState = E_APP_NCI_STATE_PRESENT;
                /* Not in NWK mode ? */
                if (E_APP_NCI_MODE_NWK != eAppNciMode)
                {
                    /* Debug */
                    DBG_vPrintf(TRACE_APP_NCI, ", eAppNciMode = NWK");
                    /* Go to network mode */
                    eAppNciMode = E_APP_NCI_MODE_NWK;
                    /* Start reading nci network data */
                    NCI_NWK_eRead(&u32NfcNwkAddress, &sNfcNwkPayload);
                }
            }
        }
        break;

        /* Absent ? */
        case E_NCI_EVENT_ABSENT:
        {
            /* Not already absent ? */
            if (E_APP_NCI_STATE_ABSENT != eAppNciState)
            {
                /* Debug */
                DBG_vPrintf(TRACE_APP_NCI, ", eAppNciState = ABSENT");
                /* Go to absent state */
                eAppNciState = E_APP_NCI_STATE_ABSENT;
            }
        }
        break;

        /* Others ? */
        default:
        {
            /* Do nothing */
            ;
        }
        break;
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: APP_bNciNtagCmdJoinWithCode
 *
 * DESCRIPTION:
 * Handle join with code command from ntag
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE bool_t APP_bNciNtagCmdJoinWithCode(void)
{
    bool_t  bWrite = FALSE;
    uint64 u64ExtPanId;

    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n%d: bNciNtagCmdJoinWithCode()", u32AppNciMs);
    /* Get extended PAN ID */
    u64ExtPanId = ZPS_u64AplZdoGetNetworkExtendedPanId();
    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n    u64ExtPanId    = %08x:%08x", (uint32)(u64ExtPanId >> 32), (uint32)(u64ExtPanId & 0xFFFFFFFF));

    /* Got a valid extended PAN ID (in a network) ? */
    if (u64ExtPanId != 0)
    {
        uint8                              u8Byte;
        uint16                            u16Crc;
        uint8                            *pu8NetworkKey;

        /* Get pointer to network key */
        pu8NetworkKey = (uint8*)ZPS_pvNwkSecGetNetworkKey(ZPS_pvAplZdoGetNwkHandle());
        /* Debug */
        DBG_vPrintf(TRACE_APP_NCI, "\n    pu8NetworkKey           =");
        for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", pu8NetworkKey[u8Byte]);
        DBG_vPrintf(TRACE_APP_NCI, "\n    sNtag.u64ExtAddr        = %08x:%08x", (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress >> 32), (uint32)(sNfcNwkPayload.sNtag.u64ExtAddress & 0xFFFFFFFF));
        DBG_vPrintf(TRACE_APP_NCI, "\n    sNtag.pu8InstallCode    =");
        for (u8Byte = 0; u8Byte < NFC_NWK_PAYLOAD_KEY_SIZE; u8Byte++) DBG_vPrintf(TRACE_APP_NCI, " %02x", sNfcNwkPayload.sNtag.au8Key[u8Byte]);
        DBG_vPrintf(TRACE_APP_NCI, "\n    sNtag.u16Crc            = x%04x", sNfcNwkPayload.sNtag.u16Crc);
        /* Application encryption ? */
        #ifdef APP_ICODE_H_
        {
            /* Recalculate CRC */
            u16Crc = APP_u16InstallCodeCrc(sNfcNwkPayload.sNtag.au8Key, NFC_NWK_PAYLOAD_KEY_SIZE);
        }
        /* Stack encryption ? */
        #else
        {
            /* Recalculate CRC */
            u16Crc = ZPS_u16crc(sNfcNwkPayload.sNtag.au8Key, NFC_NWK_PAYLOAD_KEY_SIZE);
        }
        #endif
        /* Debug */
        DBG_vPrintf(TRACE_APP_NCI, "\n    u16Crc                 = x%04x", u16Crc);
        /* Do the CRCs match ? */
        if (sNfcNwkPayload.sNtag.u16Crc == u16Crc)
        {
            /* Application encryption ? */
            #ifdef APP_ICODE_H_
            {
                /* Generate encrypted network key */
                bWrite = APP_bInstallCode(TRUE,                               /* bool_t      bEncrypt, */
                                          /* Inputs */
                                          sNfcNwkPayload.sNtag.au8Key,        /* uint8*    pu8InstallCode, */
                                          pu8NetworkKey,                      /* uint8*    pu8Input, */
                                          sNfcNwkPayload.sNtag.u64ExtAddress, /* uint64    u64ExtAddress, */
                                          /* Outputs */
                                          sNfcNwkPayload.sNci.au8Key,         /* uint8*    pu8Output, */
                                          sNfcNwkPayload.sNci.au8Mic);        /* uint8*    pu8Mic */
            }
            /* Stack encryption ? */
            #else
            {
                BDB_teStatus                       eBdbStatus;
                BDB_tsOobWriteDataToAuthenticate   sAuthenticate;
                uint8                            au8DataEncrypted[64];
                uint16                           u16DataSize;

                /* Populate data to authenticate */
                sAuthenticate.u64ExtAddr     = sNfcNwkPayload.sNtag.u64ExtAddress;
                sAuthenticate.pu8InstallCode = sNfcNwkPayload.sNtag.au8Key;
                /* Generate encrypted network key */
                eBdbStatus = BDB_eOutOfBandCommissionGetDataEncrypted(&sAuthenticate,
                                                                      au8DataEncrypted,
                                                                      &u16DataSize);
                /* Success ? */
                if (BDB_E_SUCCESS == eBdbStatus)
                {
                    /* Transfer data into structure */
                    memcpy(sNfcNwkPayload.sNci.au8Key, &au8DataEncrypted[ 8], NFC_NWK_PAYLOAD_KEY_SIZE);
                    memcpy(sNfcNwkPayload.sNci.au8Mic, &au8DataEncrypted[24], NFC_NWK_PAYLOAD_MIC_SIZE);
                    /* Write data back to NTAG */
                    bWrite = TRUE;
                }
            }
            #endif
            /* Success ? */
            if (TRUE == bWrite)
            {
                /* Clear ntag installation code data for writing */
                memset(sNfcNwkPayload.sNtag.au8Key, 0, sizeof(sNfcNwkPayload.sNtag.au8Key));
                sNfcNwkPayload.sNtag.u16Crc = 0;
                /* Set nci command data for writing */
                sNfcNwkPayload.sNci.u8Command       = NFC_NWK_NCI_CMD_JOIN_WITH_CODE;
            }
        }
    }

    /* Want to write back to tag ? */
    if (TRUE == bWrite)
    {
        ZPS_tsAplAfSimpleDescriptor   sDesc;

        /* Set common nci data for writing */
        sNfcNwkPayload.sNci.u8Sequence      = ++u8Sequence;
        sNfcNwkPayload.sNci.u16DeviceId     = 0xffff;
        sNfcNwkPayload.sNci.u64ExtAddress   = ZPS_u64AplZdoGetIeeeAddr();
        sNfcNwkPayload.sNci.u64ExtPanId     = u64ExtPanId;
        if (u64ExtPanId != 0)
        {
            ZPS_tsNwkNib                *psNib;

            psNib = ZPS_psAplZdoGetNib();
            sNfcNwkPayload.sNci.u16ShortAddress = ZPS_u16AplZdoGetNwkAddr();
            sNfcNwkPayload.sNci.u8Channel       = ZPS_u8AplZdoGetRadioChannel();
            sNfcNwkPayload.sNci.u16PanId        = ZPS_u16AplZdoGetNetworkPanId();
            sNfcNwkPayload.sNci.u8KeySeqNum     = psNib->sPersist.u8ActiveKeySeqNumber;
        }
        else
        {
            sNfcNwkPayload.sNci.u16ShortAddress = 0xffff;
            sNfcNwkPayload.sNci.u8Channel       = 0;
            sNfcNwkPayload.sNci.u16PanId        = 0;
            sNfcNwkPayload.sNci.u8KeySeqNum     = 0;
        }
        /* Can we get the simple descriptor for the passed in endpoint */
        if (ZPS_eAplAfGetSimpleDescriptor(u8Endpoint, &sDesc) == E_ZCL_SUCCESS)
        {
            /* Overwrite with correct id */
            sNfcNwkPayload.sNci.u16DeviceId = sDesc.u16DeviceId;
        }
    }

    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n    bWrite                  = %d", bWrite);

    return bWrite;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
