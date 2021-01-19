/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_start.c
 *
 * DESCRIPTION:        Base Device Demo: Coordinator Initialisation
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

#include <jendefs.h>
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "PDM.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "rnd_pub.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_tsv.h"
#include "AppApi.h"
#include "app_coordinator.h"
#include "app_main.h"
#include "uart.h"
#include "portmacro.h"
#ifdef APP_NCI_ICODE
#include "nci_nwk.h"
#include "app_nci_icode.h"
#endif
#ifdef APP_NCI_AES
#include "nci_nwk.h"
#include "app_nci_aes.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_APP
    #define TRACE_APP   TRUE
#else
    #define TRACE_APP   FALSE
#endif

#define HALT_ON_EXCEPTION   FALSE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void APP_vInitialise(void);
PRIVATE void vfExtendedStatusCallBack(ZPS_teExtendedStatus eExtendedStatus);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern void *_stack_low_water_mark;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{

#if JENNIC_CHIP_FAMILY == JN516x
    /* Wait until FALSE i.e. on XTAL - otherwise UART data will be at wrong speed */
    while (bAHI_GetClkSource() == TRUE);

    /* Move CPU to 32 MHz  vAHI_OptimiseWaitStates automatically called */
    bAHI_SetClockRate(3);
#endif

    /* Initialise the debug diagnostics module to use UART0 at 115K Baud;
     * Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS */
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);
    #ifdef DEBUG_921600
    {
        /* Bump baud rate up to 921600 */
        vAHI_UartSetBaudDivisor(DBG_E_UART_0, 2);
        vAHI_UartSetClocksPerBit(DBG_E_UART_0, 8);
    }
    #endif

#if (JENNIC_CHIP_FAMILY == JN516x)
    /* Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size. */
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);
#endif

    /* Catch resets due to watchdog timer expiry. Comment out to harden code. */
    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRACE_APP, "APP: Watchdog timer has reset device!\n");
#if HALT_ON_EXCEPTION
        vAHI_WatchdogStop();
        while (1);
#endif
    }

    /* idle task commences here */
    DBG_vPrintf(TRUE,"\n");
    DBG_vPrintf(TRUE, "***********************************************\n");
    DBG_vPrintf(TRUE, "* COORDINATOR RESET                           *\n");
    DBG_vPrintf(TRUE, "***********************************************\n");

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vSetUpHardware()\n");
    APP_vSetUpHardware();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitResources()\n");
    APP_vInitResources();

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vInitialise()\n");
    APP_vInitialise();

    DBG_vPrintf(TRACE_APP, "APP: Entering BDB_vStart()\n");
    BDB_vStart();

#ifdef APP_NCI_ICODE
    DBG_vPrintf(TRACE_APP, "\nAPP: Entering APP_vNciStart()");
    APP_vNciStart(COORDINATOR_APPLICATION_ENDPOINT);
#endif
#ifdef APP_NCI_AES
    DBG_vPrintf(TRACE_APP, "\nAPP: Entering APP_vNciStart()");
    APP_vNciStart(NFC_NWK_NSC_IOT_GATEWAY_DEVICE);
#endif

    DBG_vPrintf(TRACE_APP, "APP: Entering APP_vMainLoop()\n");
    APP_vMainLoop();

}

/****************************************************************************
 *
 * NAME: vAppRegisterPWRMCallbacks
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called to allow the application to register
 * sleep and wake callbacks.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppRegisterPWRMCallbacks(void)
{
    /* nothing to register as device does not sleep */
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialise
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vInitialise(void)
{
    /* Initialise Power Manager even on non-sleeping nodes as it allows the
     * device to doze when in the idle task */
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

    /* Initialise the Persistent Data Manager */
    PDM_eInitialise(63);

    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();

    UART_vInit();
    UART_vRtsStartFlow();

    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);

    /* Initialise application */
    APP_vInitialiseCoordinator();
}

/****************************************************************************
 *
 * NAME: vfExtendedStatusCallBack
 *
 * DESCRIPTION:
 * Callback from stack on extended error situations.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRUE,"ERROR: Extended status 0x%02x\n", eExtendedStatus);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
