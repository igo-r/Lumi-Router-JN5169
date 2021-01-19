/*****************************************************************************
 *
 * MODULE: NCI (NFC Controller Interface)
 *
 * COMPONENT: nci.h
 *
 * $AUTHOR: Martin Looker$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Projects/Components/NCI/Trunk/Include/nci.h $
 *
 * $Revision: 17111 $
 *
 * $LastChangedBy: nxp29761 $
 *
 * $LastChangedDate: 2016-04-19 15:34:21 +0100 (Tue, 19 Apr 2016) $
 *
 * $Id: nci.h 17111 2016-04-19 14:34:21Z nxp29761 $
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
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
 * Copyright NXP B.V. 2015. All rights reserved
 ***************************************************************************/
#ifndef NCI_H_
#define NCI_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Ntag defines */
#define NCI_NTAG_UNKNOWN             0
#define NCI_NTAG_NT3H1101        31101
#define NCI_NTAG_NT3H1201        31201
#define NCI_NTAG_NT3H2111        32111
#define NCI_NTAG_NT3H2211        32211

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/* NCI Events - Passed to application by library calling the registered callback function */
typedef enum
{

    E_NCI_EVENT_ABSENT,             /* NFC A Passive Poll Mode Type 2 tag has been removed */
    E_NCI_EVENT_PRESENT,            /* NFC A Passive Poll Mode Type 2 tag has been presented */
    E_NCI_EVENT_READ_FAIL,
    E_NCI_EVENT_READ_OK,
    E_NCI_EVENT_WRITE_FAIL,
    E_NCI_EVENT_WRITE_OK,
} teNciEvent;

/* NCI Event Callback function */
typedef void (*tprNciCbEvent)(          /* Called when an event takes place */
        teNciEvent  eNciEvent,          /* Event raised */
        uint32      u32Address,
        uint32      u32Length,
        uint8       *pu8Data);          /* Event data (NULL if no data) */

/****************************************************************************/
/***        Exported Functions (called by application)                    ***/
/****************************************************************************/
PUBLIC  void        NCI_vInitialise(    /* Initialise NCI library */
        uint8       u8Address,          /* Reader I2C address (0xFF for automatic detection of NPC100 or PN7120) */
        bool_t      bLocation,          /* Use alternative JN516x I2C pins */
        uint8       u8InputVen,         /* Output DIO for VEN */
        uint8       u8InputIrq);        /* Input DIO for IRQ */

PUBLIC  void        NCI_vRegCbEvent(    /* Register event callback function */
        tprNciCbEvent prRegCbEvent);    /* Pointer to event callback function */

PUBLIC  void        NCI_vTick(          /* Call regularly (5ms) to allow NCI library processing */
        uint32      u32TickMs);         /* Time in ms since previous tick */

PUBLIC  bool_t      NCI_bRead(          /* Call to read data */
        uint32      u32ReadAddress,     /* Byte address of read */
        uint32      u32ReadLength,      /* Number of bytes to read */
        uint8       *pu8ReadData);      /* Buffer to read data into */

PUBLIC  bool_t      NCI_bReadVersion(   /* Call to read version data */
        uint32      u32ReadLength,      /* Number of bytes to read */
        uint8       *pu8ReadData);      /* Buffer to read data into */

PUBLIC  bool_t      NCI_bWrite(         /* Call to write data */
        uint32      u32WriteAddress,    /* Byte address of write */
        uint32      u32WriteLength,     /* Number of bytes to write */
        uint8       *pu8WriteData);     /* Buffer to write data from */

PUBLIC  bool_t      NCI_bEnd(void);     /* Call at end of tag access */

PUBLIC  uint32      NCI_u32Ntag(void);    /* Returns NTAG type */
PUBLIC  uint8      *NCI_pu8Header(void);  /* Returns NTAG header */
PUBLIC  uint8      *NCI_pu8Version(void); /* Returns NTAG version */
PUBLIC  uint32      NCI_u32Config(void);  /* Returns NTAG config registers address */
PUBLIC  uint32      NCI_u32Session(void); /* Returns NTAG session registers address */
PUBLIC  uint32      NCI_u32Sram(void);    /* Returns NTAG SRAM address */

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif /* NCI_H_ */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
