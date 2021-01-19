/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_main.c
 *
 * DESCRIPTION:
 *
 *****************************************************************************
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
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include "jendefs.h"
#include "ZQueue.h"
#include "ZTimer.h"
#include "portmacro.h"
#include "zps_apl_af.h"
#include "mac_vs_sap.h"
#include "AppHardwareApi.h"
#include "dbg.h"
#include "app_main.h"
#include "app_events.h"
#include "app_zcl_task.h"
#include "PDM.h"
#include "app_router_node.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_APP
#define TRACE_APP   FALSE
#else
#define TRACE_APP   TRUE
#endif

#define APP_QUEUE_SIZE               8
#define BDB_QUEUE_SIZE               2
#define APP_ZTIMER_STORAGE           2
#define TIMER_QUEUE_SIZE             8
#define MLME_QUEQUE_SIZE             8
#define MCPS_QUEUE_SIZE             20
#define MCPS_DCFM_QUEUE_SIZE 		5

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC uint8 u8TimerZCL;

PUBLIC tszQueue APP_msgBdbEvents;
PUBLIC tszQueue APP_msgAppEvents;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE ZTIMER_tsTimer asTimers[APP_ZTIMER_STORAGE + BDB_ZTIMER_STORAGE];

PRIVATE APP_tsEvent         asAppEvent[APP_QUEUE_SIZE];
PRIVATE BDB_tsZpsAfEvent    asBdbEvent[BDB_QUEUE_SIZE];
PRIVATE MAC_tsMcpsVsDcfmInd asMacMcpsDcfmInd[MCPS_QUEUE_SIZE];
PRIVATE MAC_tsMlmeVsDcfmInd asMacMlmeVsDcfmInd[MLME_QUEQUE_SIZE];
PRIVATE zps_tsTimeEvent     asTimeEvent[TIMER_QUEUE_SIZE];
PRIVATE MAC_tsMcpsVsCfmData asMacMcpsDcfm[MCPS_DCFM_QUEUE_SIZE];



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
extern void zps_taskZPS(void);
extern void PWRM_vManagePower(void);

/****************************************************************************
 *
 * NAME: APP_vMainLoop
 *
 * DESCRIPTION:
 * Main application loop
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vMainLoop(void)
{

    while (TRUE)
    {
        DBG_vPrintf(FALSE, "APP: Entering zps_taskZPS\n");
        zps_taskZPS();

        DBG_vPrintf(FALSE, "APP: Entering bdb_taskBDB\n");
        bdb_taskBDB();

        DBG_vPrintf(FALSE, "APP: Entering ZTIMER_vTask\n");
        ZTIMER_vTask();

        DBG_vPrintf(FALSE, "APP: Entering APP_taskRouter\n");
        APP_taskRouter();

        /* Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed within the watchdog period
         * otherwise the system will be reset.
         */
        vAHI_WatchdogRestart();

        /* suspends CPU operation when the system is idle or puts the device to
         * sleep if there are no activities in progress
         */
        PWRM_vManagePower();
    }
}

/****************************************************************************
 *
 * NAME: APP_vSetUpHardware
 *
 * DESCRIPTION:
 * Set up interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vSetUpHardware(void)
{
    TARGET_INITIALISE();
    /* clear interrupt priority level  */
    SET_IPL(0);
    portENABLE_INTERRUPTS();
}

/****************************************************************************
 *
 * NAME: APP_vInitResources
 *
 * DESCRIPTION:
 * Initialise resources (timers, queue's etc)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitResources(void)
{
    /* Initialise the Z timer module */
    ZTIMER_eInit(asTimers, sizeof(asTimers) / sizeof(ZTIMER_tsTimer));

    /* Create Z timers */
    ZTIMER_eOpen(&u8TimerZCL,          APP_cbTimerZclTick,      NULL, ZTIMER_FLAG_PREVENT_SLEEP);

    /* Create all the queues */
    ZQ_vQueueCreate(&APP_msgAppEvents,     APP_QUEUE_SIZE,     sizeof(APP_tsEvent),        (uint8*)asAppEvent);
    ZQ_vQueueCreate(&APP_msgBdbEvents,     BDB_QUEUE_SIZE,     sizeof(BDB_tsZpsAfEvent),   (uint8*)asBdbEvent);
    ZQ_vQueueCreate(&zps_msgMlmeDcfmInd,   MLME_QUEQUE_SIZE,   sizeof(MAC_tsMlmeVsDcfmInd),(uint8*)asMacMlmeVsDcfmInd);
    ZQ_vQueueCreate(&zps_msgMcpsDcfmInd,   MCPS_QUEUE_SIZE,    sizeof(MAC_tsMcpsVsDcfmInd),(uint8*)asMacMcpsDcfmInd);
    ZQ_vQueueCreate(&zps_msgMcpsDcfm,      MCPS_DCFM_QUEUE_SIZE, sizeof(MAC_tsMcpsVsCfmData),(uint8*)asMacMcpsDcfm);
    ZQ_vQueueCreate(&zps_TimeEvents,       TIMER_QUEUE_SIZE,   sizeof(zps_tsTimeEvent),    (uint8*)asTimeEvent);

}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
