/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_zcl_task.c
 *
 * DESCRIPTION:        Base Device application: ZCL Interface
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
#include <AppApi.h>
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "PDM.h"
#include "dbg.h"
#include "zps_gen.h"
#include "ZTimer.h"
#include "zcl.h"
#include "zcl_options.h"
#include "app_zcl_task.h"
#include "base_device.h"
#include "app_common.h"
#include "app_main.h"
#include "zps_gen.h"
#include "app_events.h"
#include "GenericBoard.h"
#include <string.h>


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_ZCL
#define TRACE_ZCL   TRUE
#else
#define TRACE_ZCL   FALSE
#endif



/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

#define ZCL_TICK_TIME           ZTIMER_TIME_MSEC(100)


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_vHandleClusterUpdate(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_vZCL_DeviceSpecific_Init(void);
PRIVATE void vStartEffect(uint8 u8Effect);
PRIVATE void vHandleIdentifyRequest(uint16 u16Duration);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
tsZHA_BaseDevice sBaseDevice;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZLL */
    eZCL_Status = eZCL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "\nErr: eZLO_Initialise:%d", eZCL_Status);
    }

    /* Start the tick timer */
    if(ZTIMER_eStart(u8TimerZCL, ZCL_TICK_TIME) != E_ZTIMER_OK)
    {
        DBG_vPrintf(TRACE_ZCL, "APP: Failed to Start Tick Timer\n");
    }

    //eZCL_Status = eApp_ZLO_RegisterEndpoint(&APP_ZCL_cbEndpointCallback);
    eZCL_Status =  eZHA_RegisterBaseDeviceEndPoint(ENDDEVICE_APPLICATION_ENDPOINT,
                                                         &APP_ZCL_cbEndpointCallback,
                                                         &sBaseDevice);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ZCL, "Error: eZHA_RegisterBaseDeviceEndPoint:%d\r\n", eZCL_Status);
    }

    APP_vZCL_DeviceSpecific_Init();
}


/****************************************************************************
 *
 * NAME: APP_ZCL_vSetIdentifyTime
 *
 * DESCRIPTION:
 * Sets the remaining time in the identify cluster
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSetIdentifyTime(uint16 u16Time)
{
    sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = u16Time;
}


/****************************************************************************
 *
 * NAME: APP_cbTimerZclTick
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerZclTick(void *pvParam)
{

    static uint32 u32Tick1Sec = 9;

    tsZCL_CallBackEvent sCallBackEvent;

    ZTIMER_eStart(u8TimerZCL, ZCL_TICK_TIME);

    u32Tick1Sec++;

    eZCL_Update100mS();

    /* Wrap the 1 second  counter and provide 1Hz ticks to cluster */
    if(u32Tick1Sec > 10)
    {
        u32Tick1Sec = 0;
        sCallBackEvent.pZPSevent = NULL;
        sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
        vZCL_EventHandler(&sCallBackEvent);
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vEventHandler
 *
 * DESCRIPTION:
 * Main ZCL processing task
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent)
{
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = psStackEvent;

    DBG_vPrintf(TRACE_ZCL, "\nZCL_Task endpoint event:%d", psStackEvent->eType);
    sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
    vZCL_EventHandler(&sCallBackEvent);

}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: APP_ZCL_cbGeneralCallback
 *
 * DESCRIPTION:
 * General callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{
#if TRUE == TRACE_ZCL
    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_LOCK_MUTEX:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Lock Mutex\r\n");
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Unlock Mutex\r\n");
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Unhandled Event\r\n");
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read attributes response");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read request");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Default response");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Error");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Timer");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: ZigBee");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nInvalid event type");
        break;

    }
#endif

}


/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType)
    {
        case E_ZCL_CBET_UNHANDLED_EVENT:
        case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        case E_ZCL_CBET_READ_REQUEST:
        case E_ZCL_CBET_DEFAULT_RESPONSE:
        case E_ZCL_CBET_ERROR:
        case E_ZCL_CBET_TIMER:
        case E_ZCL_CBET_ZIGBEE_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EP EVT:No action\r\n");
            break;

        case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
            DBG_vPrintf(TRACE_ZCL, " Read Attrib Rsp %d %02x\n", psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus,
                *((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
            break;

        case E_ZCL_CBET_CLUSTER_CUSTOM:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom %04x\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
            APP_vHandleClusterCustomCommands(psEvent);
            break;

        case E_ZCL_CBET_CLUSTER_UPDATE:
            DBG_vPrintf(TRACE_ZCL, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
            APP_vHandleClusterUpdate(psEvent);
            break;

        default:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Invalid event type (%d) in APP_ZCL_cbEndpointCallback\r\n", psEvent->eEventType);
            break;
    }
}
/****************************************************************************
 *
 * NAME: APP_vHandleClusterCustomCommands
 *
 * DESCRIPTION:
 * callback for ZCL cluster custom command events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
    {

        case GENERAL_CLUSTER_ID_BASIC:
        {
            tsCLD_BasicCallBackMessage *psCallBackMessage = (tsCLD_BasicCallBackMessage*)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
            if (psCallBackMessage->u8CommandId == E_CLD_BASIC_CMD_RESET_TO_FACTORY_DEFAULTS )
            {
                DBG_vPrintf(TRACE_ZCL, "Basic Factory Reset Received\n");
                memset(&sBaseDevice,0,sizeof(tsZHA_BaseDevice));
                APP_vZCL_DeviceSpecific_Init();
                eZHA_RegisterBaseDeviceEndPoint(ENDDEVICE_APPLICATION_ENDPOINT,
                                                &APP_ZCL_cbEndpointCallback,
                                                &sBaseDevice);
            }
        }
        break;

        case GENERAL_CLUSTER_ID_IDENTIFY:
        {
            tsCLD_IdentifyCallBackMessage *psCallBackMessage = (tsCLD_IdentifyCallBackMessage*)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
            DBG_vPrintf(TRACE_ZCL, "- for identify cluster\r\n");
            /* provide callback to BDB handler for identify query response on initiator*/
            if(psEvent->psClusterInstance->bIsServer == FALSE)
            {
                tsBDB_ZCLEvent  sBDBZCLEvent;
                DBG_vPrintf(TRACE_ZCL, "\nCallBackBDB");
                sBDBZCLEvent.eType = BDB_E_ZCL_EVENT_IDENTIFY_QUERY;
                sBDBZCLEvent.psCallBackEvent = psEvent;
                BDB_vZclEventHandler(&sBDBZCLEvent);
            }
            else
            {
                // Server Side
                if (psCallBackMessage->u8CommandId == E_CLD_IDENTIFY_CMD_IDENTIFY)
                {
                    vHandleIdentifyRequest(sBaseDevice.sIdentifyServerCluster.u16IdentifyTime);
                }
                else if (psCallBackMessage->u8CommandId == E_CLD_IDENTIFY_CMD_TRIGGER_EFFECT)
                {
                    vStartEffect( psCallBackMessage->uMessage.psTriggerEffectRequestPayload->eEffectId);
                }
            }
        }
        break;

        case GENERAL_CLUSTER_ID_GROUPS:
            DBG_vPrintf(TRACE_ZCL, "- for groups cluster\r\n");
            break;

        case 0x1000:
            DBG_vPrintf(TRACE_ZCL, "\n    - for 0x1000");
            break;

        default:
            DBG_vPrintf(TRACE_ZCL, "- for unknown cluster %d\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
            break;
    }
}

/****************************************************************************
 *
 * NAME: APP_vHandleClusterUpdate
 *
 * DESCRIPTION:
 * callback for ZCL cluster update events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vHandleClusterUpdate(tsZCL_CallBackEvent *psEvent)
{
    if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == GENERAL_CLUSTER_ID_IDENTIFY)
    {
        //vAPP_ZCL_DeviceSpecific_UpdateIdentify();
        #ifdef SLEEP_ENABLE
            vReloadSleepTimers();
        #endif
    }
}

/****************************************************************************
 *
 * NAME: APP_vZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZCL specific initialization
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vZCL_DeviceSpecific_Init(void)
{
    memcpy(sBaseDevice.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sBaseDevice.sBasicServerCluster.au8ModelIdentifier, "BDB-EndDevice", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sBaseDevice.sBasicServerCluster.au8DateCode, "20150212", CLD_BAS_DATE_SIZE);
    memcpy(sBaseDevice.sBasicServerCluster.au8SWBuildID, "1000-0001", CLD_BAS_SW_BUILD_SIZE);
}

/****************************************************************************
 *
 * NAME: vStartEffect
 *
 * DESCRIPTION:
 * ZLO Device Specific identify effect set up
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PRIVATE void vStartEffect(uint8 u8Effect)
{
    switch (u8Effect) {
        case E_CLD_IDENTIFY_EFFECT_BLINK:
            sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = 2;
            break;

        case E_CLD_IDENTIFY_EFFECT_BREATHE:
            sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = 17;
            break;

        case E_CLD_IDENTIFY_EFFECT_OKAY:
            sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = 3;
            break;

        case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
            sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = 9;
            break;

        case E_CLD_IDENTIFY_EFFECT_FINISH_EFFECT:
        case E_CLD_IDENTIFY_EFFECT_STOP_EFFECT:
            sBaseDevice.sIdentifyServerCluster.u16IdentifyTime = 1;
            break;
    }
    vHandleIdentifyRequest( sBaseDevice.sIdentifyServerCluster.u16IdentifyTime);
}

/****************************************************************************
 *
 * NAME: vHandleIdentifyRequest
 *
 * DESCRIPTION: handle identify request command received by the remote
 * causes the identify blink for the required time
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleIdentifyRequest(uint16 u16Duration)
{
    ZTIMER_eStop(u8TimerId);
    if (u16Duration == 0)
    {
        vGenericLEDSetOutput(1, 0);
    }
    else
    {
        ZTIMER_eStart(u8TimerId, ZTIMER_TIME_MSEC(500));
        vGenericLEDSetOutput(1, 1);
    }
}

/****************************************************************************
 *
 * NAME: APP_cbTimerId
 *
 * DESCRIPTION: Tasks that handles the flashing leds during identfy operation
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerId(void *pvParam)
{
    if (sBaseDevice.sIdentifyServerCluster.u16IdentifyTime == 0)
    {
        ZTIMER_eStop(u8TimerId);
        vGenericLEDSetOutput(1, 0);
    }
    else
    {
        vGenericLEDSetOutput(1, sBaseDevice.sIdentifyServerCluster.u16IdentifyTime%2);
        ZTIMER_eStop(u8TimerId);
        ZTIMER_eStart(u8TimerId, ZTIMER_TIME_MSEC(500));
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
