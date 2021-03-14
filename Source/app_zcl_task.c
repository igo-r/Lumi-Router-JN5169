/****************************************************************************
 *
 * MODULE:              Lumi Router
 *
 * COMPONENT:           app_zcl_task.c
 *
 * DESCRIPTION:         ZCL Interface
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
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <string.h>

/* Generated */
#include "pdum_gen.h"
#include "zps_gen.h"

/* Application */
#include "app_main.h"
#include "app_reporting.h"
#include "app_zcl_task.h"
#include "zcl_options.h"

/* SDK JN-SW-4170 */
#include "Basic.h"
#include "DeviceTemperatureConfiguration.h"
#include "Groups.h"
#include "Identify.h"
#include "ZTimer.h"
#include "bdb_api.h"
#include "dbg.h"
#include "zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_ZCL
#define TRACE_ZCL TRUE
#else
#define TRACE_ZCL FALSE
#endif

#define ZCL_TICK_TIME ZTIMER_TIME_MSEC(100)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_vHandleIdentify(uint16 u16Time);
PRIVATE void APP_ZCL_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_vHandleClusterUpdate(tsZCL_CallBackEvent *psEvent);
PRIVATE teZCL_Status APP_ZCL_eRegisterLumiRouterEndPoint(uint8 u8EndPointIdentifier,
                                                         tfpZCL_ZCLCallBackFunction cbCallBack,
                                                         APP_tsLumiRouter *psDeviceInfo);
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC APP_tsLumiRouter sLumiRouter;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE uint8 u8IdentifyCount = 0;
PRIVATE bool_t bIdentifyState = FALSE;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZLL */
    eZCL_Status = eZCL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
        DBG_vPrintf(TRACE_ZCL, "Err: eZLO_Initialise:%d\n", eZCL_Status);

    /* Start the tick timer */
    if (ZTIMER_eStart(u8TimerTick, ZCL_TICK_TIME) != E_ZTIMER_OK)
        DBG_vPrintf(TRACE_ZCL, "APP: Failed to Start Tick Timer\n");

    /* Register Light EndPoint */
    eZCL_Status =
        APP_ZCL_eRegisterLumiRouterEndPoint(LUMIROUTER_APPLICATION_ENDPOINT, &APP_ZCL_cbEndpointCallback, &sLumiRouter);
    if (eZCL_Status != E_ZCL_SUCCESS)
        DBG_vPrintf(TRACE_ZCL, "Error: APP_ZCL_eRegisterLumiRouterEndPoint: %02x\r\n", eZCL_Status);

    APP_ZCL_vDeviceSpecific_Init();
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSetIdentifyTime
 *
 * DESCRIPTION:
 * Sets the remaining time in the identify cluster
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSetIdentifyTime(uint16 u16Time)
{
    sLumiRouter.sIdentifyServerCluster.u16IdentifyTime = u16Time;
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vEventHandler
 *
 * DESCRIPTION:
 * Main ZCL processing task
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vEventHandler(ZPS_tsAfEvent *psStackEvent)
{
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = psStackEvent;

    DBG_vPrintf(TRACE_ZCL, "ZCL_Task endpoint event:%d \n", psStackEvent->eType);
    sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
    vZCL_EventHandler(&sCallBackEvent);
}

/****************************************************************************
 *
 * NAME: APP_cbTimerZclTick
 *
 * DESCRIPTION:
 * Task kicked by the tick timer
 *
 ****************************************************************************/
PUBLIC void APP_cbTimerZclTick(void *pvParam)
{
    static uint32 u32Tick10ms = 9;
    static uint32 u32Tick1Sec = 99;

    tsZCL_CallBackEvent sCallBackEvent;

    if (ZTIMER_eStart(u8TimerTick, ZTIMER_TIME_MSEC(10)) != E_ZTIMER_OK)
        DBG_vPrintf(TRACE_ZCL, "APP: Failed to Start Tick Timer\n");

    u32Tick10ms++;
    u32Tick1Sec++;

    /* Wrap the Tick10ms counter and provide 100ms ticks to cluster */
    if (u32Tick10ms > 9) {
        eZCL_Update100mS();
        u32Tick10ms = 0;
    }

    /* Wrap the 1 second  counter and provide 1Hz ticks to cluster */
    if (u32Tick1Sec > 99) {
        u32Tick1Sec = 0;
        sCallBackEvent.pZPSevent = NULL;
        sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
        vZCL_EventHandler(&sCallBackEvent);
    }
}

#ifdef CLD_IDENTIFY_10HZ_TICK
/****************************************************************************
 *
 * NAME: vIdEffectTick
 *
 * DESCRIPTION:
 * ZLO Device Specific identify tick
 *
 ****************************************************************************/
PUBLIC void vIdEffectTick(uint8 u8Endpoint)
{

    if ((u8Endpoint == LUMIROUTER_APPLICATION_ENDPOINT) && (sLumiRouter.sIdentifyServerCluster.u16IdentifyTime > 0)) {
        u8IdentifyCount--;
        if (u8IdentifyCount == 0) {
            u8IdentifyCount = 5;
            bIdentifyState = (bIdentifyState) ? FALSE : TRUE;
        }
    }
}
#endif

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
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType) {

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
}

/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{

    switch (psEvent->eEventType) {
    case E_ZCL_CBET_LOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Unhandled event");
        break;

    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        DBG_vPrintf(TRACE_ZCL,
                    "\nEP EVT: Rd Attr Rsp %04x AS %d",
                    psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum,
                    psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus);
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read attributes response");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read request");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Default response");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Error");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Timer");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: ZigBee");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom Cl %04x\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
        APP_ZCL_vHandleClusterCustomCommands(psEvent);
        break;

    case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Write Individual Attribute Status %02x\n", psEvent->eZCL_Status);
        break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE: {
        tsZCL_IndividualAttributesResponse *psIndividualAttributeResponse =
            &psEvent->uMessage.sIndividualAttributeResponse;
        DBG_vPrintf(TRACE_ZCL,
                    "Individual Report attribute for Cluster = %d\n",
                    psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        DBG_vPrintf(TRACE_ZCL, "eAttributeDataType = %d\n", psIndividualAttributeResponse->eAttributeDataType);
        DBG_vPrintf(TRACE_ZCL, "u16AttributeEnum = %d\n", psIndividualAttributeResponse->u16AttributeEnum);
        DBG_vPrintf(TRACE_ZCL, "eAttributeStatus = %d\n", psIndividualAttributeResponse->eAttributeStatus);
    } break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE: {
        tsZCL_AttributeReportingConfigurationRecord *psAttributeReportingRecord =
            &psEvent->uMessage.sAttributeReportingConfigurationRecord;
        DBG_vPrintf(
            TRACE_ZCL,
            "Individual Configure Report Cluster %d Attrib %d Type %d Min %d Max %d IntV %d Direcct %d Change %d\n",
            psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
            psAttributeReportingRecord->u16AttributeEnum,
            psAttributeReportingRecord->eAttributeDataType,
            psAttributeReportingRecord->u16MinimumReportingInterval,
            psAttributeReportingRecord->u16MaximumReportingInterval,
            psAttributeReportingRecord->u16TimeoutPeriodField,
            psAttributeReportingRecord->u8DirectionIsReceived,
            psAttributeReportingRecord->uAttributeReportableChange);

        if (E_ZCL_SUCCESS == psEvent->eZCL_Status) {
            APP_vSaveReportableRecord(psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
                                      psAttributeReportingRecord);
        }
        else if (E_ZCL_RESTORE_DEFAULT_REPORT_CONFIGURATION == psEvent->eZCL_Status) {
            APP_vRestoreDefaultRecord(LUMIROUTER_APPLICATION_ENDPOINT,
                                      psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum,
                                      psAttributeReportingRecord);
        }
    } break;

    case E_ZCL_CBET_CLUSTER_UPDATE:
        DBG_vPrintf(TRACE_ZCL, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        APP_ZCL_vHandleClusterUpdate(psEvent);
        break;

    case E_ZCL_CBET_REPORT_REQUEST:
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Invalid evt type 0x%x", (uint8)psEvent->eEventType);
        break;
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vHandleIdentify
 *
 * DESCRIPTION:
 * Application identify handler
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_vHandleIdentify(uint16 u16Time)
{
    static bool bActive = FALSE;
    if (u16Time == 0) {
        /* Restore to off/off state */
        bActive = FALSE;
    }
    else {
        /* Set the Identify levels */
        if (!bActive) {
            bActive = TRUE;
            u8IdentifyCount = 5;
            bIdentifyState = TRUE;
        }
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vHandleClusterCustomCommands
 *
 * DESCRIPTION:
 * callback for ZCL cluster custom command events
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_vHandleClusterCustomCommands(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId) {
    case GENERAL_CLUSTER_ID_IDENTIFY: {
        tsCLD_IdentifyCallBackMessage *psCallBackMessage =
            (tsCLD_IdentifyCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
        if (psCallBackMessage->u8CommandId == E_CLD_IDENTIFY_CMD_IDENTIFY) {
            APP_ZCL_vHandleIdentify(sLumiRouter.sIdentifyServerCluster.u16IdentifyTime);
        }
    } break;

    case GENERAL_CLUSTER_ID_BASIC: {
        tsCLD_BasicCallBackMessage *psCallBackMessage =
            (tsCLD_BasicCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
        if (psCallBackMessage->u8CommandId == E_CLD_BASIC_CMD_RESET_TO_FACTORY_DEFAULTS) {
            DBG_vPrintf(TRACE_ZCL, "Basic Factory Reset Received\n");
            memset(&sLumiRouter, 0, sizeof(APP_tsLumiRouter));
            APP_ZCL_vDeviceSpecific_Init();
            APP_ZCL_eRegisterLumiRouterEndPoint(LUMIROUTER_APPLICATION_ENDPOINT,
                                                &APP_ZCL_cbEndpointCallback,
                                                &sLumiRouter);
        }
    } break;
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vHandleClusterUpdate
 *
 * DESCRIPTION:
 * callback for ZCL cluster update events
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_vHandleClusterUpdate(tsZCL_CallBackEvent *psEvent)
{
    if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == GENERAL_CLUSTER_ID_IDENTIFY) {
        APP_ZCL_vHandleIdentify(sLumiRouter.sIdentifyServerCluster.u16IdentifyTime);
        if (sLumiRouter.sIdentifyServerCluster.u16IdentifyTime == 0) {
            tsBDB_ZCLEvent sBDBZCLEvent;
            /* provide callback to BDB handler for identify on Target */
            sBDBZCLEvent.eType = BDB_E_ZCL_EVENT_IDENTIFY;
            sBDBZCLEvent.psCallBackEvent = psEvent;
            BDB_vZclEventHandler(&sBDBZCLEvent);
        }
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_eRegisterLumiRouterEndPoint
 *
 * DESCRIPTION:
 * Registers an lumi router with the ZCL layer
 *
 * PARAMETERS:  Name                            Usage
 *              u8EndPointIdentifier            Endpoint being registered
 *              cbCallBack                      Pointer to endpoint callback
 *              psDeviceInfo                    Pointer to struct containing
 *                                              data for endpoint
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PRIVATE teZCL_Status APP_ZCL_eRegisterLumiRouterEndPoint(uint8 u8EndPointIdentifier,
                                                         tfpZCL_ZCLCallBackFunction cbCallBack,
                                                         APP_tsLumiRouter *psDeviceInfo)
{
    /* Fill in end point details */
    psDeviceInfo->sEndPoint.u8EndPointNumber = u8EndPointIdentifier;
    psDeviceInfo->sEndPoint.u16ManufacturerCode = ZCL_MANUFACTURER_CODE;
    psDeviceInfo->sEndPoint.u16ProfileEnum = HA_PROFILE_ID;
    psDeviceInfo->sEndPoint.bIsManufacturerSpecificProfile = FALSE;
    psDeviceInfo->sEndPoint.u16NumberOfClusters =
        sizeof(APP_tsLumiRouterClusterInstances) / sizeof(tsZCL_ClusterInstance);
    psDeviceInfo->sEndPoint.psClusterInstance = (tsZCL_ClusterInstance *)&psDeviceInfo->sClusterInstance;
    psDeviceInfo->sEndPoint.bDisableDefaultResponse = ZCL_DISABLE_DEFAULT_RESPONSES;
    psDeviceInfo->sEndPoint.pCallBackFunctions = cbCallBack;

    if (eCLD_BasicCreateBasic(&psDeviceInfo->sClusterInstance.sBasicServer,
                              TRUE,
                              &sCLD_Basic,
                              &psDeviceInfo->sBasicServerCluster,
                              &au8BasicClusterAttributeControlBits[0]) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    if (eCLD_IdentifyCreateIdentify(&psDeviceInfo->sClusterInstance.sIdentifyServer,
                                    TRUE,
                                    &sCLD_Identify,
                                    &psDeviceInfo->sIdentifyServerCluster,
                                    &au8IdentifyAttributeControlBits[0],
                                    &psDeviceInfo->sIdentifyServerCustomDataStructure) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    if (eCLD_GroupsCreateGroups(&psDeviceInfo->sClusterInstance.sGroupsServer,
                                TRUE,
                                &sCLD_Groups,
                                &psDeviceInfo->sGroupsServerCluster,
                                &au8GroupsAttributeControlBits[0],
                                &psDeviceInfo->sGroupsServerCustomDataStructure,
                                &psDeviceInfo->sEndPoint) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    if (eCLD_DeviceTemperatureConfigurationCreateDeviceTemperatureConfiguration(
            &psDeviceInfo->sClusterInstance.sDeviceTemperatureConfigurationServer,
            TRUE,
            &sCLD_DeviceTemperatureConfiguration,
            &psDeviceInfo->sDeviceTemperatureConfigurationServerCluster,
            &au8DeviceTempConfigClusterAttributeControlBits[0]) != E_ZCL_SUCCESS) {
        return E_ZCL_FAIL;
    }

    return eZCL_Register(&psDeviceInfo->sEndPoint);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vDeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZCL specific initialization
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_vDeviceSpecific_Init(void)
{
    memcpy(sLumiRouter.sBasicServerCluster.au8ManufacturerName, BAS_MANUF_NAME_STRING, CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8ModelIdentifier, BAS_MODEL_ID_STRING, CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8DateCode, BAS_DATE_STRING, CLD_BAS_DATE_SIZE);
    memcpy(sLumiRouter.sBasicServerCluster.au8SWBuildID, BAS_SW_BUILD_STRING, CLD_BAS_SW_BUILD_SIZE);

    sLumiRouter.sDeviceTemperatureConfigurationServerCluster.i16CurrentTemperature = 0;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
