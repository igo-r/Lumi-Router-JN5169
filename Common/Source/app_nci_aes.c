/*****************************************************************************
 *
 * MODULE:          JN-AN-1217 Base Device application
 *
 * COMPONENT:       app_nci_aes.c
 *
 * DESCRIPTION:     Base Device - Application layer for NCI (AES encryption)
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
#include "nci.h"
#include "nci_nwk.h"
#include "app_nci_aes.h"
#include "app_main.h"

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
PRIVATE uint8                       u8AppNciNwkNscType;
PRIVATE tsNfcNwk                    sNfcNwk;
PRIVATE bool_t                      bNciTimer;

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
PUBLIC void APP_vNciStart(uint8 u8NscType)
{
    /* Debug */
    DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_vNciStart(x%02x) AES", u32AppNciMs, u8NscType);
    /* Store the NSC device type */
    u8AppNciNwkNscType = u8NscType;
    /* Initialise main NTAG state machine */
    NCI_vInitialise(APP_NCI_ADDRESS,
                     APP_NCI_I2C_LOCATION,
                     APP_NCI_VEN_PIN,
                     17); /* Use SW3/DIO17 for NCI-IRQ pin */
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
    /* De-register callback */
    NCI_vRegCbEvent(NULL);
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
    /* Restart the timer */
    if (bNciTimer) ZTIMER_eStart(u8TimerNci, APP_NCI_TICK_MS);
    /* Debug */
    //DBG_vPrintf(TRACE_APP_NCI, ".");
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
            /* Network NTAG state machine finished ? */
            if (E_NCI_NWK_ACTIVE != eNciNwkStatus)
            {
                /* Debug */
                DBG_vPrintf(TRACE_APP_NCI, "\n%d: APP_cbNciTimer()", u32AppNciMs);
                DBG_vPrintf(TRACE_APP_NCI, ", NCI_NWK_eTick() = %d", eNciNwkStatus);
                DBG_vPrintf(TRACE_APP_NCI, ", eAppNciMode = APP");
                /* Stop network processing */
                (void) NCI_NWK_eStop();
                /* Go to application mode */
                eAppNciMode = E_APP_NCI_MODE_APP;
                /* Reclaim event callbacks */
                NCI_vRegCbEvent(APP_cbNciEvent);
                /* Success ? */
                if (E_NCI_NWK_OK == eNciNwkStatus)
                {
                    #if TRACE_APP_NCI
                    {
                        /* Debug */
                        DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sDev.u8NscType       = x%02x",     sNfcNwk.sDev.u8NscType);
                        DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sDev.u64ExtAddress   = x%08x%08x", (uint32)(sNfcNwk.sDev.u64ExtAddress >> 32), (uint32)(sNfcNwk.sDev.u64ExtAddress & 0xffffffff));
                    }
                    #endif

                    /* Which command was passed in ? */
                    switch (sNfcNwk.sNwk.u8Command)
                    {
                        /* Return device to factory new */
                        case NFC_NWK_CMD_FACTORY_NEW:
                        /* Join network ? */
                        case NFC_NWK_CMD_JOIN_NETWORK:
                        /* Secure join network with ECC encryption (not supported) */
                        case NFC_NWK_CMD_SECURE_JOIN_NETWORK:
                        /* Leave network ? */
                        case NFC_NWK_CMD_LEAVE_NETWORK:
                        {
                            /* Need to purge joining node in application layer */
                            ZPS_vNMPurgeEntry(sNfcNwk.sDev.u64ExtAddress);
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
                /* Finish activity to allow sleeping */
                (void) PWRM_eFinishActivity();
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
                    uint8 *pu8NetworkKey;
                    ZPS_tsNwkNib *psNib;
                    uint32 u32Idx;

                    /* Debug */
                    DBG_vPrintf(TRACE_APP_NCI, ", eAppNciMode = NWK");
                    /* Zero the NtagNwk data structure */
                    memset(&sNfcNwk, 0, sizeof(sNfcNwk));
                    /* Get pointer to network key */
                    pu8NetworkKey = (uint8*)ZPS_pvNwkSecGetNetworkKey(ZPS_pvAplZdoGetNwkHandle());
                    /* Get pointer to NIB */
                    psNib = ZPS_psAplZdoGetNib();
                    /* Populate the network data in the NtagNwk structure */
                    sNfcNwk.sNwk.u8NscType             = u8AppNciNwkNscType;
                    sNfcNwk.sNwk.u8Command             = NFC_NWK_CMD_JOIN_NETWORK;
                    sNfcNwk.sNwk.u8Channel             = ZPS_u8AplZdoGetRadioChannel();
                    sNfcNwk.sNwk.u16PanId              = ZPS_u16AplZdoGetNetworkPanId();
                    sNfcNwk.sNwk.u64ExtPanId           = ZPS_u64AplZdoGetNetworkExtendedPanId();
                    sNfcNwk.sNwk.u64TrustCenterAddress = ZPS_u64AplZdoGetIeeeAddr();
                    memcpy(sNfcNwk.sNwk.au8NetworkKey, pu8NetworkKey, NFC_NWK_KEY_LENGTH);
                    sNfcNwk.sNwk.u8KeySeqNum           = psNib->sPersist.u8ActiveKeySeqNumber;
                    /* Debug */
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u8NscType             = x%02x",     sNfcNwk.sNwk.u8NscType);
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u8Command             = %d",        sNfcNwk.sNwk.u8Command);
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u8Channel             = %d",        sNfcNwk.sNwk.u8Channel);
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u16PanId              = x%04x",     sNfcNwk.sNwk.u16PanId);
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u64ExtPanId           = x%08x%08x", (uint32)(sNfcNwk.sNwk.u64ExtPanId           >> 32), (uint32)(sNfcNwk.sNwk.u64ExtPanId           & 0xffffffff));
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u64TrustCenterAddress = x%08x%08x", (uint32)(sNfcNwk.sNwk.u64TrustCenterAddress >> 32), (uint32)(sNfcNwk.sNwk.u64TrustCenterAddress & 0xffffffff));
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.au8NetworkKey         = ");
                    for (u32Idx = 0; u32Idx < NFC_NWK_KEY_LENGTH; u32Idx++) DBG_vPrintf(TRACE_APP_NCI, "%02x", sNfcNwk.sNwk.au8NetworkKey[u32Idx]);
                    DBG_vPrintf(TRACE_APP_NCI, "\n    sNfcNwk.sNwk.u8KeySeqNum           = %d",        sNfcNwk.sNwk.u8KeySeqNum);
                    /* Start activity to prevent sleeping */
                    (void) PWRM_eStartActivity();
                    /* Go to network mode */
                    eAppNciMode = E_APP_NCI_MODE_NWK;
                    /* Start ntag network processing */
                    NCI_NWK_eStart(&sNfcNwk);
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

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
