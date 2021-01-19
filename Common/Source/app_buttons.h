/*****************************************************************************
 *
 * MODULE:             JN-AN-1217
 *
 * COMPONENT:          app_buttons.h
 *
 * DESCRIPTION:        DR1199/DR1175 Button Press detection (Interface)
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

#ifndef APP_BUTTONS_H
#define APP_BUTTONS_H

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

#if (defined BUTTON_MAP_DR1199)
    #if (defined APP_NTAG_ICODE) || (defined APP_NTAG_AES)
        typedef enum {
            APP_E_BUTTONS_BUTTON_1 = 0,
            APP_E_BUTTONS_BUTTON_SW1,
            APP_E_BUTTONS_BUTTON_SW2,
            APP_E_BUTTONS_BUTTON_SW3,
            APP_E_BUTTONS_BUTTON_SW4,
            APP_E_BUTTONS_NFC_FD
        } APP_teButtons;
        #define APP_BUTTONS_NUM             (6UL)
        #if (JENNIC_CHIP_FAMILY == JN516x)
            #define APP_BUTTONS_BUTTON_1          (8)
            #define APP_BUTTONS_BUTTON_SW1        (11)
            #define APP_BUTTONS_BUTTON_SW2        (12)
            #define APP_BUTTONS_BUTTON_SW3        (17)
            #define APP_BUTTONS_BUTTON_SW4        (1)
            #define APP_BUTTONS_NFC_FD            (0)
        #elif (JENNIC_CHIP_FAMILY == JN517x)
            #define APP_BUTTONS_BUTTON_1          (4)
            #define APP_BUTTONS_BUTTON_SW1        (12)
            #define APP_BUTTONS_BUTTON_SW2        (13)
            #define APP_BUTTONS_BUTTON_SW3        (18)
            #define APP_BUTTONS_BUTTON_SW4        (5)
            #define APP_BUTTONS_NFC_FD            (17)
        #endif
        #define APP_BUTTONS_DIO_MASK                ((1 << APP_BUTTONS_BUTTON_1) | (1 << APP_BUTTONS_BUTTON_SW4) | (1 << APP_BUTTONS_BUTTON_SW3) | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1) | (1 << APP_BUTTONS_NFC_FD))
        #define APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP (                              (1 << APP_BUTTONS_BUTTON_SW4)| (1 << APP_BUTTONS_BUTTON_SW3)  | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1) | (1 << APP_BUTTONS_NFC_FD))
    #else
        typedef enum {
            APP_E_BUTTONS_BUTTON_1 = 0,
            APP_E_BUTTONS_BUTTON_SW1,
            APP_E_BUTTONS_BUTTON_SW2,
            APP_E_BUTTONS_BUTTON_SW3,
            APP_E_BUTTONS_BUTTON_SW4,
        } APP_teButtons;

        #define APP_BUTTONS_NUM             (5UL)
        #if (JENNIC_CHIP_FAMILY == JN516x)
            #define APP_BUTTONS_BUTTON_1          (8)
            #define APP_BUTTONS_BUTTON_SW1        (11)
            #define APP_BUTTONS_BUTTON_SW2        (12)
            #define APP_BUTTONS_BUTTON_SW3        (17)
            #define APP_BUTTONS_BUTTON_SW4        (1)
        #elif (JENNIC_CHIP_FAMILY == JN517x)
            #define APP_BUTTONS_BUTTON_1          (4)
            #define APP_BUTTONS_BUTTON_SW1        (12)
            #define APP_BUTTONS_BUTTON_SW2        (13)
            #define APP_BUTTONS_BUTTON_SW3        (18)
            #define APP_BUTTONS_BUTTON_SW4        (5)
        #endif
        #define APP_BUTTONS_DIO_MASK        ((1 << APP_BUTTONS_BUTTON_1)|(1 << APP_BUTTONS_BUTTON_SW4)|(1 << APP_BUTTONS_BUTTON_SW3) | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1))
        #define APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP        ((1 << APP_BUTTONS_BUTTON_SW4)|(1 << APP_BUTTONS_BUTTON_SW3) | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1))
     #endif
#elif (defined BUTTON_MAP_DR1175)
    #if (defined APP_NTAG_ICODE) || (defined APP_NTAG_AES)
        typedef enum {
            APP_E_BUTTONS_BUTTON_1 = 0,
            APP_E_BUTTONS_NFC_FD
        } APP_teButtons;
        #define APP_BUTTONS_NUM                 (2UL)
        #if (JENNIC_CHIP_FAMILY == JN516x)
            #define APP_BUTTONS_BUTTON_1        (8)
            #define APP_BUTTONS_NFC_FD          (0)
        #elif (JENNIC_CHIP_FAMILY == JN517x)
            #define APP_BUTTONS_BUTTON_1        (4)
            #define APP_BUTTONS_NFC_FD          (17)
        #endif
        #define APP_BUTTONS_DIO_MASK            ((1 << APP_BUTTONS_BUTTON_1) | (1 << APP_BUTTONS_NFC_FD))
        #define APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP (                          (1 << APP_BUTTONS_NFC_FD))
    #else
        typedef enum {
            APP_E_BUTTONS_BUTTON_1 = 0
        } APP_teButtons;

        #define APP_BUTTONS_NUM             (1UL)
        #if (JENNIC_CHIP_FAMILY == JN516x)
            #define APP_BUTTONS_BUTTON_1        (8)
        #elif (JENNIC_CHIP_FAMILY == JN517x)
            #define APP_BUTTONS_BUTTON_1            (4)
        #endif
        #define APP_BUTTONS_DIO_MASK        (1 << APP_BUTTONS_BUTTON_1)
    #endif
#else
#error "define a button map for the hardware"
    typedef enum {
        APP_E_BUTTONS_BUTTON_1
    } APP_teButtons;

    #define APP_BUTTONS_NUM             (1UL)
    #define APP_BUTTONS_BUTTON_1        (8)
    #define APP_BUTTONS_DIO_MASK        (1 << APP_BUTTONS_BUTTON_1)
#endif

typedef enum {
    E_INTERRUPT_UNKNOWN,
    E_INTERRUPT_BUTTON,
    E_INTERRUPT_WAKE_TIMER_EXPIRY
} teInterruptType;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC bool_t APP_bButtonInitialise(void);
extern PUBLIC void vManageWakeUponSysControlISR(teInterruptType eInterruptType);

PUBLIC void APP_cbTimerButtonScan(void *pvParam);
PUBLIC void APP_cbTimerButtonDelay(void *pvParam);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_BUTTONS_H*/
