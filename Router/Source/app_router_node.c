/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_router_node.c
 *
 * DESCRIPTION:        Base Device Demo: Router application
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
#include <string.h>
#include "dbg.h"
#include "pdum_apl.h"
#include "pdum_nwk.h"
#include "pdum_gen.h"
#include "pwrm.h"
#include "PDM.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "zps_apl_aps.h"
#include "app_common.h"
#include "app_main.h"
#include "ZTimer.h"
#include "app_events.h"
#include <rnd_pub.h>
#include "app_zcl_task.h"
#include "app_router_node.h"
#include "zps_nwk_nib.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "zcl.h"
#include "app_reporting.h"
#include "app_serial_commands.h"
#include "app_device_temperature.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


#ifndef DEBUG_APP
    #define TRACE_APP   FALSE
#else
    #define TRACE_APP   TRUE
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

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vAppHandleAfEvent( BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void vAppHandleZdoEvents( BDB_tsZpsAfEvent *psZpsAfEvent);
PRIVATE void APP_vBdbInit(void);
#if BDB_JOIN_USES_INSTALL_CODE_KEY == TRUE
PRIVATE void APP_vSetICDerivedLinkKey(void);
#endif
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE teNodeState eNodeState;
uint32 u32OldFrameCtr;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

#ifdef PDM_EEPROM
    extern uint8 u8PDM_CalculateFileSystemCapacity();
    extern uint8 u8PDM_GetFileSystemOccupancy();
#endif

extern void zps_vNwkSecClearMatSet(void *psNwk);

/****************************************************************************
 *
 * NAME: vPrintAPSTable
 *
 * DESCRIPTION:
 * Prints the content of APS table
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vPrintAPSTable(void)
{
    uint8 i;
    uint8 j;

    ZPS_tsAplAib * tsAplAib;

    tsAplAib = ZPS_psAplAibGetAib();

    for ( i = 0 ; i < (tsAplAib->psAplDeviceKeyPairTable->u16SizeOfKeyDescriptorTable + 1) ; i++ )
    {
        DBG_vPrintf(TRACE_APP, "MAC: %016llx Key: ", ZPS_u64NwkNibGetMappedIeeeAddr(ZPS_pvAplZdoGetNwkHandle(),tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u16ExtAddrLkup));
        for(j=0; j<16;j++)
        {
            DBG_vPrintf(TRACE_APP, "%02x ", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].au8LinkKey[j]);
        }
        DBG_vPrintf(TRACE_APP, "\n");
        DBG_vPrintf(TRACE_APP, "Incoming FC: %d\n", tsAplAib->pu32IncomingFrameCounter[i]);
        DBG_vPrintf(TRACE_APP, "Outgoing FC: %d\n", tsAplAib->psAplDeviceKeyPairTable->psAplApsKeyDescriptorEntry[i].u32OutgoingFrameCounter);
    }
}
/****************************************************************************
 *
 * NAME: APP_vInitialiseRouter
 *
 * DESCRIPTION:
 * Initialises the application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseRouter(void)
{
    uint16 u16ByteRead;
    PDM_teStatus eStatusReportReload;

    /* Stay awake */
    PWRM_eStartActivity();

    eNodeState = E_STARTUP;
    PDM_eReadDataFromRecord(PDM_ID_APP_ROUTER,
                            &eNodeState,
                            sizeof(teNodeState),
                            &u16ByteRead);

   /* Restore any report data that is previously saved to flash */
       eStatusReportReload = eRestoreReports();

       ZPS_u32MacSetTxBuffers  (4);

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();

    /* Initialise ZCL */
    APP_ZCL_vInitialise();

    /* Initialise other software modules
     * HERE
     */
    APP_vBdbInit();

    /* Always initialise any peripherals used by the application
     * HERE
     */

    APP_vDeviceTemperatureInit();

    /* The functions u8PDM_CalculateFileSystemCapacity and u8PDM_GetFileSystemOccupancy
     * may be called at any time to monitor space available in  the eeprom  */
    DBG_vPrintf(TRACE_APP, "PDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity() );
    DBG_vPrintf(TRACE_APP, "PDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy() );

    DBG_vPrintf(TRACE_APP, "Start Up StaTe %d On Network %d\n",
            eNodeState,
            sBDB.sAttrib.bbdbNodeIsOnANetwork);

    /*Load the reports from the PDM or the default ones depending on the PDM load record status*/
    if(eStatusReportReload !=PDM_E_STATUS_OK )
    {
        /*Load Defaults if the data was not correct*/
        vLoadDefaultConfigForReportable();
    }
    /*Make the reportable attributes */
    vMakeSupportedAttributesReportable();

#if BDB_JOIN_USES_INSTALL_CODE_KEY == TRUE
    APP_vSetICDerivedLinkKey();
    vPrintAPSTable();
#endif

    APP_WriteMessageToSerial("Router started..");
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
    BDB_teStatus eStatus;

    switch(psBdbEvent->eEventType)
    {
        case BDB_EVENT_NONE:
            break;

        case BDB_EVENT_ZPSAF:                // Use with BDB_tsZpsAfEvent
            vAppHandleAfEvent(&psBdbEvent->uEventData.sZpsAfEvent);
            break;

        case BDB_EVENT_INIT_SUCCESS:

            DBG_vPrintf(TRACE_APP,"APP: BDB_EVENT_INIT_SUCCESS\n");
            if (eNodeState == E_STARTUP)
            {
                eStatus = BDB_eNsStartNwkSteering();
                DBG_vPrintf(TRACE_APP, "BDB Try Steering status %d\n",eStatus);
            }
            else
            {
                DBG_vPrintf(TRACE_APP, "BDB Init go Running");
                eNodeState = E_RUNNING;
                PDM_eSaveRecordData(PDM_ID_APP_ROUTER,&eNodeState,sizeof(teNodeState));
            }

            break;


        case BDB_EVENT_NWK_FORMATION_SUCCESS:
            DBG_vPrintf(TRACE_APP,"APP: NwkFormation Success \n");
            eNodeState = E_RUNNING;
            PDM_eSaveRecordData(PDM_ID_APP_ROUTER,
                                &eNodeState,
                                sizeof(eNodeState));
            break;


        case BDB_EVENT_NWK_STEERING_SUCCESS:
            DBG_vPrintf(TRACE_APP,"APP: NwkSteering Success \n");
            eNodeState = E_RUNNING;
            PDM_eSaveRecordData(PDM_ID_APP_ROUTER, &eNodeState, sizeof(teNodeState) );
            break;

        default:
            break;
    }
}

/****************************************************************************
 *
 * NAME: APP_taskRouter
 *
 * DESCRIPTION:
 * Task that handles application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_taskRouter(void)
{
    APP_tsEvent sAppEvent;
    sAppEvent.eType = APP_E_EVENT_NONE;

    if (ZQ_bQueueReceive(&APP_msgAppEvents, &sAppEvent) == TRUE)
    {
        DBG_vPrintf(TRACE_APP, "ZPR: App event %d, NodeState=%d\n", sAppEvent.eType, eNodeState);

        if (sAppEvent.eType == APP_E_EVENT_LEAVE_AND_RESET)
        {
            if (eNodeState == E_RUNNING)
            {
                if (ZPS_eAplZdoLeaveNetwork( 0UL, FALSE, FALSE) != ZPS_E_SUCCESS )
                  {
                    APP_vFactoryResetRecords();
                    vAHI_SwReset();
                }
            }
            else
            {
                APP_vFactoryResetRecords();
                vAHI_SwReset();
            }
        }
    }
}

/****************************************************************************
 *
 * NAME: APP_vOobcSetRunning
 *
 * DESCRIPTION:
 * Set running state for OOBC
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vOobcSetRunning(void)
{
    eNodeState = E_RUNNING;
    PDM_eSaveRecordData(PDM_ID_APP_ROUTER,&eNodeState,sizeof(teNodeState));
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
    if (psZpsAfEvent->u8EndPoint == ROUTER_APPLICATION_ENDPOINT)
    {
        DBG_vPrintf(TRACE_APP, "Pass to ZCL\n");
        if ((psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) ||
            (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION))
        {
            APP_ZCL_vEventHandler( &psZpsAfEvent->sStackEvent);
         }
    }
    else if (psZpsAfEvent->u8EndPoint == 0)
    {
        vAppHandleZdoEvents( psZpsAfEvent);
    }

    /* Ensure Freeing of Apdus */
    if (psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
    else if ( psZpsAfEvent->sStackEvent.eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION )
    {
        PDUM_eAPduFreeAPduInstance(psZpsAfEvent->sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst);
    }

}

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
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Data Indication Status %02x from %04x Src Ep Dst %d Ep %d Profile %04x Cluster %04x\n",
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
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Network started\n");
            break;

        case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Joined Network Addr %04x Rejoin %d\n",
                    psAfEvent->uEvent.sNwkJoinedEvent.u16Addr,
                    psAfEvent->uEvent.sNwkJoinedEvent.bRejoin);
            break;
        case ZPS_EVENT_NWK_FAILED_TO_START:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Network Failed To start\n");
            break;

        case ZPS_EVENT_NWK_FAILED_TO_JOIN:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Failed To Join %02x Rejoin %d\n",
                    psAfEvent->uEvent.sNwkJoinFailedEvent.u8Status,
                    psAfEvent->uEvent.sNwkJoinFailedEvent.bRejoin);
            break;

        case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: New Node %04x Has Joined\n",
                    psAfEvent->uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
            break;

        case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Discovery Complete %02x\n",
                    psAfEvent->uEvent.sNwkDiscoveryEvent.eStatus);
            vPrintAPSTable();
            break;

        case ZPS_EVENT_NWK_LEAVE_INDICATION:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Leave Indication %016llx Rejoin %d\n",
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr,
                    psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin);
            if ( (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0UL) &&
                 (psAfEvent->uEvent.sNwkLeaveIndicationEvent.u8Rejoin == 0) )
            {
                /* We sare asked to Leave without rejoin */
                DBG_vPrintf(TRACE_APP, "LEAVE IND -> For Us No Rejoin\n");
                APP_vFactoryResetRecords();
                vAHI_SwReset();
            }
            break;

        case ZPS_EVENT_NWK_LEAVE_CONFIRM:
            DBG_vPrintf(TRACE_APP, "APP-ZDO: Leave Confirm status %02x Addr %016llx\n",
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus,
                    psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr);
            if ((psAfEvent->uEvent.sNwkLeaveConfirmEvent.eStatus == ZPS_E_SUCCESS) &&
                (psAfEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0UL))
            {
                DBG_vPrintf(TRACE_APP, "Leave -> Reset Data Structures\n");
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


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


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
    ZPS_vDefaultStack();
    ZPS_vSetKeys();
    ZPS_eAplAibSetApsUseExtendedPanId (0);
#if BDB_JOIN_USES_INSTALL_CODE_KEY == TRUE
    APP_vSetICDerivedLinkKey();
#endif
    /* save everything */
    eNodeState = E_STARTUP;
    PDM_eSaveRecordData(PDM_ID_APP_ROUTER,&eNodeState,sizeof(teNodeState));
    ZPS_vSaveAllZpsRecords();
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
}

#if BDB_JOIN_USES_INSTALL_CODE_KEY == TRUE
/****************************************************************************
 *
 * NAME: APP_vSetICDerivedLinkKey
 *
 * DESCRIPTION:
 * Setting up an install code derived link key
 * Install code is 8-byte MAC address repeated once.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vSetICDerivedLinkKey()
{
    int i;
    uint8 au8ICode[16];
    uint64 u64MyMacAddr = ZPS_u64NwkNibGetExtAddr(ZPS_pvAplZdoGetNwkHandle());
    /* Test Vector  = {0x83,0xFE,0xD3,0x40,0x7A,0x93,0x97,0x23,0xA5,0xC6,0x39,0xB2,0x69,0x16,0xD5,0x05};   */

    /* Copy uint64 MAC into array twice */
    for (i=0; i<=15; i++)
    {
        au8ICode[15-i]= ((u64MyMacAddr >> (i%8)*8) & (0xFF));
    }

    for (i=0; i<16; i++)
    {
        DBG_vPrintf(TRACE_APP, "%02x", au8ICode[i]);
    }
    DBG_vPrintf(TRACE_APP, "\n");

    ZPS_vAplSecSetInitialSecurityState( ZPS_ZDO_PRCONFIGURED_INSTALLATION_CODE,
                                        au8ICode,
                                        16,
                                        ZPS_APS_GLOBAL_LINK_KEY);
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
