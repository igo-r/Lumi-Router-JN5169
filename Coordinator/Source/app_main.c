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
#include "pwrm.h"
#include "portmacro.h"
#include "zps_apl_af.h"
#include "mac_vs_sap.h"
#include "AppHardwareApi.h"
#include "dbg.h"
#include "app_coordinator.h"
#include "app_serial_commands.h"
#include "app_buttons.h"
#include "app_events.h"
#include "app_main.h"
#include "uart.h"
#include "app_zcl_task.h"
#include "bdb_api.h"
#ifdef APP_NCI_ICODE
#include "app_nci_icode.h"
#endif
#ifdef APP_NCI_AES
#include "app_nci_aes.h"
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BDB_QUEUE_SIZE               3
#if (defined APP_NCI_ICODE) || (defined APP_NCI_AES)
#define APP_ZTIMER_STORAGE           4 /* NCI: Added timer */
#else
#define APP_ZTIMER_STORAGE           3
#endif
#define TIMER_QUEUE_SIZE             8
#define MLME_QUEQUE_SIZE             9
#define MCPS_QUEUE_SIZE             24
#define APP_QUEUE_SIZE               1
#define TX_QUEUE_SIZE               10
#define RX_QUEUE_SIZE               30
#define MCPS_DCFM_QUEUE_SIZE 		5

#if JENNIC_CHIP_FAMILY == JN517x
#define NVIC_INT_PRIO_LEVEL_SYSCTRL (13)
#define NVIC_INT_PRIO_LEVEL_BBC     (7)
#define NVIC_INT_PRIO_LEVEL_UART0   (5)
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

/* timers */
PUBLIC uint8 u8TimerButtonScan;
PUBLIC uint8 u8TimerZCL;
PUBLIC uint8 u8TimerId;
#if (defined APP_NCI_ICODE) || (defined APP_NCI_AES)
PUBLIC uint8 u8TimerNci;
#endif

PUBLIC tszQueue APP_msgBdbEvents;
PUBLIC tszQueue APP_msgAppEvents;
PUBLIC tszQueue APP_msgSerialTx;
PUBLIC tszQueue APP_msgSerialRx;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE ZTIMER_tsTimer asTimers[APP_ZTIMER_STORAGE + BDB_ZTIMER_STORAGE];

PRIVATE BDB_tsZpsAfEvent    asBdbEvent[BDB_QUEUE_SIZE];
PRIVATE APP_tsEvent         asAppEvent[APP_QUEUE_SIZE];
PRIVATE MAC_tsMlmeVsDcfmInd asMacMlmeVsDcfmInd[MLME_QUEQUE_SIZE];
PRIVATE MAC_tsMcpsVsDcfmInd asMacMcpsDcfmInd[MCPS_QUEUE_SIZE];
PRIVATE zps_tsTimeEvent     asTimeEvent[TIMER_QUEUE_SIZE];
PRIVATE uint8               au8TxBuffer[TX_QUEUE_SIZE];
PRIVATE uint8               au8RxBuffer[RX_QUEUE_SIZE];
PRIVATE MAC_tsMcpsVsCfmData asMacMcpsDcfm[MCPS_DCFM_QUEUE_SIZE];


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
#if (JENNIC_CHIP_FAMILY == JN517x)
extern void vISR_SystemController(uint32 u32DeviceId, uint32 u32BitMap);
#endif

extern void zps_taskZPS(void);

/*start of file*/
/****************************************************************************
 *
 * NAME: APP_vMainLoop
 *
 * DESCRIPTION:
 * Main  execution loop
 *
 * RETURNS:
 * Never
 *
 ****************************************************************************/
PUBLIC void APP_vMainLoop(void)
{

    /* idle task commences on exit from OS start call */
    while (TRUE)
    {

        zps_taskZPS();
        bdb_taskBDB();
        ZTIMER_vTask();

        APP_taskCoordinator();

        APP_taskAtSerial();

        /* Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed with in the watchdog period
         * otherwise the system will be reset.
         */
        vAHI_WatchdogRestart();

        /*
         * suspends CPU operation when the system is idle or puts the device to
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
#if (JENNIC_CHIP_FAMILY == JN517x)
    vAHI_Uart0RegisterCallback ( APP_isrUart );
    vAHI_SysCtrlRegisterCallback ( vISR_SystemController );
    u32AHI_Init();
    vAHI_InterruptSetPriority ( MICRO_ISR_MASK_BBC,        NVIC_INT_PRIO_LEVEL_BBC );
    vAHI_InterruptSetPriority ( MICRO_ISR_MASK_UART0,   NVIC_INT_PRIO_LEVEL_UART0 );
    vAHI_InterruptSetPriority ( MICRO_ISR_MASK_SYSCTRL, NVIC_INT_PRIO_LEVEL_SYSCTRL );
#endif

#if (JENNIC_CHIP_FAMILY == JN516x)
    TARGET_INITIALISE();
    /* clear interrupt priority level  */
    SET_IPL(0);
    portENABLE_INTERRUPTS();
#endif
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
    ZTIMER_eOpen(&u8TimerButtonScan,    APP_cbTimerButtonScan,  NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerZCL,           APP_cbTimerZclTick ,    NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    ZTIMER_eOpen(&u8TimerId,            APP_cbTimerId,          NULL, ZTIMER_FLAG_PREVENT_SLEEP);
#if (defined APP_NCI_ICODE) || (defined APP_NCI_AES)
    ZTIMER_eOpen(&u8TimerNci,           APP_cbNciTimer,         NULL, ZTIMER_FLAG_PREVENT_SLEEP);
#endif

    /* create all the queues*/
    ZQ_vQueueCreate(&APP_msgBdbEvents,     BDB_QUEUE_SIZE,       sizeof(BDB_tsZpsAfEvent),   (uint8*)asBdbEvent);
    ZQ_vQueueCreate(&APP_msgAppEvents,     APP_QUEUE_SIZE,       sizeof(APP_tsEvent),        (uint8*)asAppEvent);
    ZQ_vQueueCreate(&zps_msgMlmeDcfmInd,   MLME_QUEQUE_SIZE,     sizeof(MAC_tsMlmeVsDcfmInd),(uint8*)asMacMlmeVsDcfmInd);
    ZQ_vQueueCreate(&zps_msgMcpsDcfmInd,   MCPS_QUEUE_SIZE,      sizeof(MAC_tsMcpsVsDcfmInd),(uint8*)asMacMcpsDcfmInd);
    ZQ_vQueueCreate(&zps_msgMcpsDcfm,      MCPS_DCFM_QUEUE_SIZE, sizeof(MAC_tsMcpsVsCfmData),(uint8*)asMacMcpsDcfm);

    ZQ_vQueueCreate(&zps_TimeEvents,       TIMER_QUEUE_SIZE,   sizeof(zps_tsTimeEvent),    (uint8*)asTimeEvent);

    ZQ_vQueueCreate(&APP_msgSerialTx,      TX_QUEUE_SIZE,   sizeof(uint8),    (uint8*)au8TxBuffer);
    ZQ_vQueueCreate(&APP_msgSerialRx,      RX_QUEUE_SIZE,   sizeof(uint8),    (uint8*)au8RxBuffer);
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
