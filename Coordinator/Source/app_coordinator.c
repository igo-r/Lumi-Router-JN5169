/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_coordinator.c
 *
 * DESCRIPTION:        Base Device Demo: Coordinator application
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

#include <jendefs.h>
#include "dbg.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pwrm.h"
#include "PDM.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdo.h"
#include "zps_apl_zdp.h"
#include "app_coordinator.h"
#include "app_zcl_task.h"
#include "app_buttons.h"
#include "PDM_IDs.h"
#include "app_common.h"
#include "app_events.h"
#include "app_main.h"
#include "ZQueue.h"
#include "ZTimer.h"
#include "OnOff.h"
#ifdef JN517x
#include "AHI_ModuleConfiguration.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_APP
    #define TRACE_APP   TRUE
#else
    #define TRACE_APP   FALSE
#endif

#ifdef DEBUG_APP_EVENT
    #define TRACE_APP_EVENT   TRUE
#else
    #define TRACE_APP_EVENT   FALSE
#endif

#ifdef DEBUG_APP_BDB
    #define TRACE_APP_BDB     TRUE
#else
    #define TRACE_APP_BDB     FALSE
#endif


#ifndef USB_DONGLE
#if (JENNIC_CHIP_FAMILY == JN516x)
    #define LED_NWK_FORMED    (1<<2)    /* using DIO2 */
    #define LED_PERMIT_JOIN   (1<<3)    /* using DIO3 */
#elif (JENNIC_CHIP_FAMILY == JN517x)
    #define LED_NWK_FORMED    (1<<2)    /* using GPIO?? */
    #define LED_PERMIT_JOIN   (1<<3)    /* using GPIO?? */
#endif
#else
#if (JENNIC_CHIP_FAMILY == JN516x)
    #define LED_NWK_FORMED    (1<<16)   /* using DIO16 */
    #define LED_PERMIT_JOIN   (1<<17)   /* using DIO17 */
#elif (JENNIC_CHIP_FAMILY == JN517x)
    #define LED_NWK_FORMED    (1<<14)    /* using GPIO14 */
    #define LED_PERMIT_JOIN   (1<<15)    /* using GPIO15 */
#endif
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitDR1174LED(void);
PRIVATE void vAppHandleAfEvent( BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void vAppHandleZdoEvents( BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void vAppSendOnOff(void);
PRIVATE void vAppSendIdentifyStop( uint16 u16Address, uint8 u8Endpoint);
PRIVATE void vAppSendRemoteBindRequest(uint16 u16DstAddr, uint16 u16ClusterId, uint8 u8DstEp);
PRIVATE void APP_vBdbInit(void);
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC PDM_tsRecordDescriptor s_sDevicePDDesc;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE teNodeState eNodeState;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialiseCoordinator
 *
 * DESCRIPTION:
 * Initialises the Coordinator application
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseCoordinator(void)
{
    /* Restore any application data previously saved to flash
     * All Application records must be loaded before the call to
     * ZPS_eAplAfInit
     */
    APP_bButtonInitialise();

    eNodeState = E_STARTUP;
    uint16 u16ByteRead;
    PDM_eReadDataFromRecord(PDM_ID_APP_COORD,
                            &eNodeState,
                            sizeof(eNodeState),
                            &u16ByteRead);
#ifdef JN517x
    /* Default module configuration: change E_MODULE_DEFAULT as appropriate */
      vAHI_ModuleConfigure(E_MODULE_DEFAULT);
#endif

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();

    APP_ZCL_vInitialise();

#ifdef PDM_EEPROM
    vDisplayPDMUsage();
#endif

    /* Initialise other software modules
     * HERE
     */
    APP_vBdbInit();

    /* Delete PDM if required */
    vDeletePDMOnButtonPress(APP_BUTTONS_BUTTON_1);

    /* Always initialise any peripherals used by the application
     * HERE
     */
    vInitDR1174LED();

    DBG_vPrintf(TRACE_APP, "Recovered Application State %d On Network %d\n",
            eNodeState, sBDB.sAttrib.bbdbNodeIsOnANetwork);
}


/****************************************************************************
 *
 * NAME: APP_vBdbCallback
 *
 * DESCRIPTION:
 * Callback from the BDB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vBdbCallback(BDB_tsBdbEvent *psBdbEvent)
{
    static uint8 u8NoQueryCount;

    switch(psBdbEvent->eEventType)
    {
        case BDB_EVENT_NONE:
            break;

        case BDB_EVENT_ZPSAF:                // Use with BDB_tsZpsAfEvent
            vAppHandleAfEvent(&psBdbEvent->uEventData.sZpsAfEvent);
            break;

        case BDB_EVENT_INIT_SUCCESS:
            eNodeState = E_RUNNING;
            break;

        case BDB_EVENT_NWK_FORMATION_SUCCESS:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: NwkFormation Success \n");
            PDM_eSaveRecordData(PDM_ID_APP_COORD,
                                &eNodeState,
                                sizeof(eNodeState));
            break;

        case BDB_EVENT_NWK_STEERING_SUCCESS:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: NwkSteering Success\n");
            break;

        case BDB_EVENT_NWK_FORMATION_FAILURE:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Network Formation Failure\n");
            break;

        case BDB_EVENT_FB_HANDLE_SIMPLE_DESC_RESP_OF_TARGET:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: F&B Simple Desc response From %04x Profle %04x Device %04x Ep %d Version %d\n",
                    psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress,
                    psBdbEvent->uEventData.psFindAndBindEvent->u16ProfileId,
                    psBdbEvent->uEventData.psFindAndBindEvent->u16DeviceId,
                    psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp,
                    psBdbEvent->uEventData.psFindAndBindEvent->u8DeviceVersion);
            break;

        case BDB_EVENT_FB_CHECK_BEFORE_BINDING_CLUSTER_FOR_TARGET:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Check For Binding Cluster %04x\n",
                        psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16ClusterId);
            break;
        case BDB_EVENT_FB_CLUSTER_BIND_CREATED_FOR_TARGET:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Bind Created for cluster %04x\n",
                    psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16ClusterId);
            vAppSendRemoteBindRequest( psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress,
                                       psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16ClusterId,
                                       psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp);
            break;

        case BDB_EVENT_FB_BIND_CREATED_FOR_TARGET:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Bind Created for target EndPt %d\n",
                    psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp);

#ifndef USE_GROUPS
            vAppSendIdentifyStop( psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress,
                                  psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp);
#endif
            break;

        case BDB_EVENT_FB_GROUP_ADDED_TO_TARGET:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Group Added with ID %04x\n",
                    psBdbEvent->uEventData.psFindAndBindEvent->uEvent.u16GroupId);
            u8NoQueryCount = 0;
#ifdef USE_GROUPS
                //Example to ask to Stop identification to that group
            vAppSendIdentifyStop(psBdbEvent->uEventData.psFindAndBindEvent->u16TargetAddress,
                                 psBdbEvent->uEventData.psFindAndBindEvent->u8TargetEp);
            vAppSendRemoteBindRequest(uint16 u16DstAddr, uint16 u16ClusterId, uint8 u8DstEp)
#endif

            break;

        case BDB_EVENT_FB_ERR_BINDING_FAILED:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Binding Failed\n");
            break;

        case BDB_EVENT_FB_ERR_BINDING_TABLE_FULL:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Binding Table Full\n");
            break;

        case BDB_EVENT_FB_ERR_GROUPING_FAILED:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Grouping Failed\n");
            break;

        case BDB_EVENT_FB_NO_QUERY_RESPONSE:

            if(u8NoQueryCount >= 2)
            {
                u8NoQueryCount = 0;
                BDB_vFbExitAsInitiator();
                DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: No Identify Query Response Stopping F&B\n");
            }
            else
            {
                u8NoQueryCount++;
                DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: No Identify Query Response\n");
            }
            break;

        case BDB_EVENT_FB_TIMEOUT:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: F&B Timeout\n");
            break;


        default:
            DBG_vPrintf(TRACE_APP_BDB,"APP-BDB: Unhandled %d\n", psBdbEvent->eEventType);
            break;
    }
}


/****************************************************************************
 *
 * NAME: APP_taskCoordinator
 *
 * DESCRIPTION:
 * Main state machine
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_taskCoordinator(void)
{
    BDB_teStatus eStatus;
    APP_tsEvent sAppEvent;
    sAppEvent.eType = APP_E_EVENT_NONE;

    if (ZQ_bQueueReceive(&APP_msgAppEvents, &sAppEvent) == TRUE)
    {
        DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Event %d, NodeState=%d\n", sAppEvent.eType, eNodeState);

        if (sAppEvent.eType == APP_E_EVENT_BUTTON_DOWN)
        {
            switch(sAppEvent.uEvent.sButton.u8Button)
            {
                case APP_E_BUTTONS_BUTTON_SW1:
                    DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Send Toggle Cmd\n");
                    vAppSendOnOff();
                    break;

                case APP_E_BUTTONS_BUTTON_SW2:
                    eStatus = BDB_eNsStartNwkSteering();
                    DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Request Nwk Steering %02x\n", eStatus);
                    break;

                case APP_E_BUTTONS_BUTTON_SW4:
#ifdef USE_GROUPS
                    sBDB.sAttrib.u16bdbCommissioningGroupID = GROUP_ID;
#endif
                    eStatus = BDB_eFbTriggerAsInitiator(COORDINATOR_APPLICATION_ENDPOINT);
                    DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Find and Bind initiate %02x\n", eStatus);
                    break;

                case APP_E_BUTTONS_BUTTON_1: //DIO8
                    /* Not already on a network ? */
                    if (FALSE == sBDB.sAttrib.bbdbNodeIsOnANetwork)
                    {
                        eStatus = BDB_eNfStartNwkFormation();
                        DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Request Nwk Formation %02x\n", eStatus);
                    }
                    break;

                default:
                    break;
            }
        }
        else if (sAppEvent.eType == APP_E_EVENT_LEAVE_AND_RESET)
        {
            DBG_vPrintf(TRACE_APP_EVENT, "APP-EVT: Leave nd Reset\n");
        }

    }
}


/****************************************************************************
 *
 * NAME: vAppHandleAfEvent
 *
 * DESCRIPTION:
 * Application handler for stack events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleAfEvent( BDB_tsZpsAfEvent *psZpsAfEvent)
{
    if (psZpsAfEvent->u8EndPoint == COORDINATOR_APPLICATION_ENDPOINT)
    {
        if ((psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) ||
            (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION))
        {
            APP_ZCL_vEventHandler( &psZpsAfEvent->sStackEvent);
         }
    }
    else if (psZpsAfEvent->u8EndPoint == COORDINATOR_ZDO_ENDPOINT)
    {
        vAppHandleZdoEvents( psZpsAfEvent);
    }

    /* free up any Apdus */
    if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
    else if ( psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION )
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst);
    }


}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vAppHandleZdoEvents
 *
 * DESCRIPTION:
 * Application handler for stack events for end point 0 (ZDO)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleZdoEvents( BDB_tsZpsAfEvent *psZpsAfEvent)
{
    ZPS_tsAfEvent *psAfEvent = &(psZpsAfEvent->sStackEvent);

    switch(psAfEvent->eType)
    {
        case ZPS_EVENT_APS_DATA_INDICATION:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Data Indication Status %02x from %04x Src Ep %d Dst Ep %d Profile %04x Cluster %04x\n",
                    psAfEvent->uEvent.sApsDataIndEvent.eStatus,
                    psAfEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,
                    psAfEvent->uEvent.sApsDataIndEvent.u8SrcEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ProfileId,
                    psAfEvent->uEvent.sApsDataIndEvent.u16ClusterId);
            break;

        case ZPS_EVENT_APS_DATA_CONFIRM:
            break;

        case ZPS_EVENT_APS_DATA_ACK:
            break;
            break;

        case ZPS_EVENT_NWK_STARTED:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Network started Channel = %d\n", ZPS_u8AplZdoGetRadioChannel() );
            break;

        case ZPS_EVENT_NWK_FAILED_TO_START:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Network Failed To start\n");
            break;

        case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: New Node %04x Has Joined\n",
                    psAfEvent->uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
            break;

        case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Discovery Complete %02x\n",
                    psAfEvent->uEvent.sNwkDiscoveryEvent.eStatus);
            break;

        case ZPS_EVENT_NWK_LEAVE_INDICATION:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Leave Indication %016llx Rejoin %d\n",
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin);
            break;

        case ZPS_EVENT_NWK_LEAVE_CONFIRM:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Leave Confirm status %02x Addr %016llx\n",
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr);
            if ( (psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus == ZPS_E_SUCCESS) &&
                    (psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0ULL))
            {
                /*-we left  */
                APP_vFactoryResetRecords();
                vAHI_SwReset();
            }
            break;

        case ZPS_EVENT_NWK_STATUS_INDICATION:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Network status Indication %02x addr %04x\n",
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u8Status,
                    psAfEvent->uEvent.sNwkStatusIndicationEvent.u16NwkAddr);
            break;

        case ZPS_EVENT_NWK_ROUTE_DISCOVERY_CONFIRM:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Discovery Confirm\n");
            break;

        case ZPS_EVENT_NWK_ED_SCAN:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Energy Detect Scan %02x\n",
                    psAfEvent->uEvent.sNwkEdScanConfirmEvent.u8Status);
            break;
        case ZPS_EVENT_ZDO_BIND:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Zdo Bind event\n");
            break;

        case ZPS_EVENT_ZDO_UNBIND:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Zdo Unbiind Event\n");
            break;

        case ZPS_EVENT_ZDO_LINK_KEY:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Zdo Link Key Event Type %d Addr %016llx\n",
                        psAfEvent->uEvent.sZdoLinkKeyEvent.u8KeyType,
                        psAfEvent->uEvent.sZdoLinkKeyEvent.u64IeeeLinkAddr);
            break;

        case ZPS_EVENT_BIND_REQUEST_SERVER:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Bind Request Server Event\n");
            break;

        case ZPS_EVENT_ERROR:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: AF Error Event %d\n", psAfEvent->uEvent.sAfErrorEvent.eError);
            break;

        case ZPS_EVENT_TC_STATUS:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Trust Center Status %02x\n", psAfEvent->uEvent.sApsTcEvent.u8Status);
            break;

        default:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Unhandled Event %d\n", psAfEvent->eType);
            break;
    }
}

/****************************************************************************
 *
 * NAME: vInitDR1174LED
 *
 * DESCRIPTION:
 * Initialised DIO3 and DIO6 as output
 *
 *
 ****************************************************************************/
PRIVATE void vInitDR1174LED(void)
{

    /*Enable Pull Ups : Not really required for the outputs */
    vAHI_DioSetPullup(0,LED_NWK_FORMED);
    vAHI_DioSetPullup(0,LED_PERMIT_JOIN);

    /*Make the DIo as output*/
    vAHI_DioSetDirection(0,LED_NWK_FORMED);
    vAHI_DioSetDirection(0,LED_PERMIT_JOIN);
}

/****************************************************************************
 *
 * NAME: vAppSendOnOff
 *
 * DESCRIPTION:
 * Sends an On Of Togle Command to the bound devices
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppSendOnOff(void)
{
    tsZCL_Address   sDestinationAddress;
    uint8 u8seqNo;
    teZCL_Status eStatus;

    sDestinationAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eStatus = eCLD_OnOffCommandSend( COORDINATOR_APPLICATION_ENDPOINT,      // Src Endpoint
                             0,                                             // Dest Endpoint (bound so do not care)
                             &sDestinationAddress,
                             &u8seqNo,
                             E_CLD_ONOFF_CMD_TOGGLE);

    if (eStatus != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_APP, "Send Toggle Failed %02x\n", eStatus);
    }

}

/****************************************************************************
 *
 * NAME: vAppSendIdentifyStop
 *
 * DESCRIPTION:
 * Sends an Identify stop command to the target address
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppSendIdentifyStop( uint16 u16Address, uint8 u8Endpoint)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_Identify_IdentifyRequestPayload sPayload;

    sPayload.u16IdentifyTime = 0;
    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = u16Address;
    eCLD_IdentifyCommandIdentifyRequestSend(
                            COORDINATOR_APPLICATION_ENDPOINT,
                            u8Endpoint,
                            &sAddress,
                            &u8Seq,
                            &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppSendRemoteBindRequest
 *
 * DESCRIPTION:
 * Sends a bind request to a remote node for it to create a binding with this node
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppSendRemoteBindRequest(uint16 u16DstAddr, uint16 u16ClusterId, uint8 u8DstEp)
{
    PDUM_thAPduInstance hAPduInst;
    ZPS_tuAddress uDstAddr;
    ZPS_tsAplZdpBindUnbindReq sAplZdpBindUnbindReq;
    ZPS_teStatus eStatus;

    uDstAddr.u16Addr = u16DstAddr;

    hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);
    if (PDUM_INVALID_HANDLE != hAPduInst)
    {
        uint8 u8SeqNumber;
        sAplZdpBindUnbindReq.u64SrcAddress = ZPS_u64AplZdoLookupIeeeAddr(u16DstAddr);
        sAplZdpBindUnbindReq.u8SrcEndpoint = COORDINATOR_APPLICATION_ENDPOINT;
        sAplZdpBindUnbindReq.u16ClusterId = u16ClusterId;


        sAplZdpBindUnbindReq.u8DstAddrMode = E_ZCL_AM_IEEE;
        sAplZdpBindUnbindReq.uAddressField.sExtended.u64DstAddress = ZPS_u64NwkNibGetExtAddr(ZPS_pvAplZdoGetNwkHandle() );
        sAplZdpBindUnbindReq.uAddressField.sExtended.u8DstEndPoint = u8DstEp;

        DBG_vPrintf(TRACE_APP, "Remote Bind Dst addr %04x, Ieee Dst Addr %016llx Ieee Src %016llx\n",
                uDstAddr.u16Addr,
                sAplZdpBindUnbindReq.uAddressField.sExtended.u64DstAddress,
                sAplZdpBindUnbindReq.u64SrcAddress);

        eStatus = ZPS_eAplZdpBindUnbindRequest(hAPduInst,
                                               uDstAddr,
                                               FALSE,
                                               &u8SeqNumber,
                                               TRUE,
                                               &sAplZdpBindUnbindReq);
        DBG_vPrintf(TRACE_APP, "Sending a remote bind request Status =%x\n", eStatus);
    }
}

/****************************************************************************
 *
 * NAME: APP_vFactoryResetRecords
 *
 * DESCRIPTION:
 * Resets persisted data structures to factory new state
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vFactoryResetRecords(void)
{
    /* clear out the stack */
    DBG_vPrintf(TRACE_APP, "Call clear stack\n");
    ZPS_vDefaultStack();
    ZPS_vSaveAllZpsRecords();
    DBG_vPrintf(TRACE_APP, "Reset BDB Security\n");
    ZPS_vSetKeys();
    ZPS_eAplAibSetApsUseExtendedPanId (0);

    /* save everything */
    eNodeState = E_STARTUP;
    DBG_vPrintf(TRACE_APP, "Save App Record\n");
    PDM_eSaveRecordData(PDM_ID_APP_COORD,&eNodeState,sizeof(teNodeState));
}

/****************************************************************************
 *
 * NAME: APP_vBdbInit
 *
 * DESCRIPTION:
 * Function to initialize BDB attributes and message queue
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vBdbInit(void)
{
    BDB_tsInitArgs sInitArgs;

    sBDB.sAttrib.bbdbNodeIsOnANetwork = ((eNodeState >= E_RUNNING)?(TRUE):(FALSE));
    sInitArgs.hBdbEventsMsgQ = &APP_msgBdbEvents;

    BDB_vInit(&sInitArgs);
    sBDB.sAttrib.u32bdbPrimaryChannelSet = BDB_PRIMARY_CHANNEL_SET;
    sBDB.sAttrib.u32bdbSecondaryChannelSet = BDB_SECONDARY_CHANNEL_SET;
}

/****************************************************************************
 *
 * NAME: vDeletePDMOnButtonPress
 *
 * DESCRIPTION:
 * PDM context clearing on button press
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO)
{
    bool_t bDeleteRecords = FALSE;
    uint32 u32Buttons = u32AHI_DioReadInput() & (1 << u8ButtonDIO);
    if (u32Buttons == 0)
    {
        bDeleteRecords = TRUE;
    }
    else
    {
        bDeleteRecords = FALSE;
    }
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDeleteAllDataRecords() if context saving is not required.
     */
    if(bDeleteRecords)
    {
        DBG_vPrintf(TRACE_APP,"Deleting the PDM\n");
        APP_vFactoryResetRecords();
        while ( u32Buttons == 0 )
        {
            u32Buttons = u32AHI_DioReadInput() & (1 << u8ButtonDIO);
        }
        vAHI_SwReset();
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
