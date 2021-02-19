/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_reporting.c
 *
 * DESCRIPTION:        Base Device application - reporting functionality
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
#include <string.h>
#include "dbg.h"
#include "zps_gen.h"
#include "PDM.h"
#include "PDM_IDs.h"
#include "app_common.h"
#include "zcl_options.h"
#include "zcl_common.h"
#include "app_reporting.h"
#ifdef CLD_GROUPS
#include "Groups.h"
#include "Groups_internal.h"
#endif
#include "app_device_temperature.h"
#include "DeviceTemperatureConfiguration.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_REPORT
    #define TRACE_REPORT   FALSE
#else
    #define TRACE_REPORT   TRUE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE uint8 u8GetRecordIndex(uint16 u16ClusterID, uint16 u16AttributeEnum);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/*Just Two reports for time being*/
PRIVATE tsReports asSavedReports[ZCL_NUMBER_OF_REPORTS];

/* define the default reports */
tsReports asDefaultReports[ZCL_NUMBER_OF_REPORTS] = \
{\
    {GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION, {0, E_ZCL_INT16, E_CLD_DEVTEMPCFG_ATTR_ID_CURRENT_TEMPERATURE, MIN_REPORT_INTERVAL, MAX_REPORT_INTERVAL, 0, {DEVICE_TEMPERATURE_MINIMUM_REPORTABLE_CHANGE}}}
};


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: eRestoreReports
 *
 * DESCRIPTION:
 * Loads the reporting information from the EEPROM/PDM
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC PDM_teStatus eRestoreReports( void )
{
    /* Restore any report data that is previously saved to flash */
    uint16 u16ByteRead;
    PDM_teStatus eStatusReportReload = PDM_eReadDataFromRecord(PDM_ID_APP_REPORTS,
                                                              asSavedReports,
                                                              sizeof(asSavedReports),
                                                              &u16ByteRead);

    DBG_vPrintf(TRACE_REPORT,"eStatusReportReload = %d\n", eStatusReportReload);
    /* Restore any application data previously saved to flash */

    return  (eStatusReportReload);
}

/****************************************************************************
 *
 * NAME: vMakeSupportedAttributesReportable
 *
 * DESCRIPTION:
 * Makes the attributes reportable for On Off and Level control
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vMakeSupportedAttributesReportable(void)
{
    uint16 u16AttributeEnum;
    uint16 u16ClusterId;
    int i;


    tsZCL_AttributeReportingConfigurationRecord*    psAttributeReportingConfigurationRecord;

    DBG_vPrintf(TRACE_REPORT, "MAKE Reportable ep %d\n", ROUTER_APPLICATION_ENDPOINT);

    for (i = 0; i < ZCL_NUMBER_OF_REPORTS; i++)
    {
        u16AttributeEnum = asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum;
        u16ClusterId = asSavedReports[i].u16ClusterID;
        psAttributeReportingConfigurationRecord = &(asSavedReports[i].sAttributeReportingConfigurationRecord);
        DBG_vPrintf(TRACE_REPORT, "Cluster %04x Attribute %04x Min %d Max %d IntV %d Direct %d Change %d\n",
                u16ClusterId,
                u16AttributeEnum,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zint8ReportableChange);
        eZCL_SetReportableFlag( ROUTER_APPLICATION_ENDPOINT, u16ClusterId, TRUE, FALSE, u16AttributeEnum);
        eZCL_CreateLocalReport( ROUTER_APPLICATION_ENDPOINT, u16ClusterId, 0, TRUE, psAttributeReportingConfigurationRecord);
    }
}

/****************************************************************************
 *
 * NAME: vLoadDefaultConfigForReportable
 *
 * DESCRIPTION:
 * Loads a default configuration
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vLoadDefaultConfigForReportable(void)
{
    memset(asSavedReports, 0 ,sizeof(asSavedReports));
    int i;
    for (i = 0; i < ZCL_NUMBER_OF_REPORTS; i++)
    {
        asSavedReports[i] = asDefaultReports[i];
    }

#if TRACE_REPORT
    DBG_vPrintf(TRACE_REPORT,"Loaded Defaults Records\n");
    for (i = 0; i < ZCL_NUMBER_OF_REPORTS; i++)
    {
        DBG_vPrintf(TRACE_REPORT,"Cluster %04x Type %d Attr %04x Min %d Max %d IntV %d Direct %d Change %d\n",
                asSavedReports[i].u16ClusterID,
                asSavedReports[i].sAttributeReportingConfigurationRecord.eAttributeDataType,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16AttributeEnum,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
                asSavedReports[i].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
                asSavedReports[i].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);
    }
#endif

    /*Save this Records*/
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS,
                        asSavedReports,
                        sizeof(asSavedReports));
}


/****************************************************************************
 *
 * NAME: vSaveReportableRecord
 *
 * DESCRIPTION:
 * Loads a default configuration
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSaveReportableRecord(  uint16 u16ClusterID,
                                    tsZCL_AttributeReportingConfigurationRecord* psAttributeReportingConfigurationRecord)
{
    uint8 u8Index = u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);

    if (u8Index == 0xFF)
        return;

    DBG_vPrintf(TRACE_REPORT, "Save to report %d\n", u8Index);

    /*For CurrentLevel attribute in LevelControl Cluster*/
    asSavedReports[u8Index].u16ClusterID=u16ClusterID;
    memcpy( &(asSavedReports[u8Index].sAttributeReportingConfigurationRecord),
            psAttributeReportingConfigurationRecord,
            sizeof(tsZCL_AttributeReportingConfigurationRecord) );

    DBG_vPrintf(TRACE_REPORT,"Cluster %04x Type %d Attrib %04x Min %d Max %d IntV %d Direction %d Change %d\n",
            asSavedReports[u8Index].u16ClusterID,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.eAttributeDataType,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16AttributeEnum,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange );

    /*Save this Records*/
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS,
                        asSavedReports,
                        sizeof(asSavedReports));
}

PRIVATE uint8 u8GetRecordIndex(uint16 u16ClusterID,
		                       uint16 u16AttributeEnum)
{
	uint8 u8Index = 0xFF;

    if (u16ClusterID == GENERAL_CLUSTER_ID_DEVICE_TEMPERATURE_CONFIGURATION)
        u8Index = REPORT_DEVICE_TEMPERATURE_CONFIGURATION_SLOT;

	return u8Index;
}

PUBLIC void vRestoreDefaultRecord(  uint8                                        u8EndPointID,
									uint16                                       u16ClusterID,
									tsZCL_AttributeReportingConfigurationRecord* psAttributeReportingConfigurationRecord)
{
    uint8 u8Index = u8GetRecordIndex(u16ClusterID, psAttributeReportingConfigurationRecord->u16AttributeEnum);

    if (u8Index == 0xFF)
        return;

    eZCL_CreateLocalReport( u8EndPointID, u16ClusterID, 0, TRUE, &(asDefaultReports[u8Index].sAttributeReportingConfigurationRecord));

    DBG_vPrintf(TRACE_REPORT, "Save to report %d\n", u8Index);

    memcpy( &(asSavedReports[u8Index].sAttributeReportingConfigurationRecord),
    		&(asDefaultReports[u8Index].sAttributeReportingConfigurationRecord),
            sizeof(tsZCL_AttributeReportingConfigurationRecord) );

    DBG_vPrintf(TRACE_REPORT,"Cluster %04x Type %d Attrib %04x Min %d Max %d IntV %d Direction %d Change %d\n",
            asSavedReports[u8Index].u16ClusterID,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.eAttributeDataType,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16AttributeEnum,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MinimumReportingInterval,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16MaximumReportingInterval,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u16TimeoutPeriodField,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.u8DirectionIsReceived,
            asSavedReports[u8Index].sAttributeReportingConfigurationRecord.uAttributeReportableChange.zuint8ReportableChange);

    /*Save this Records*/
    PDM_eSaveRecordData(PDM_ID_APP_REPORTS,
                        asSavedReports,
                        sizeof(asSavedReports));

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
