/****************************************************************************/
/*
 * MODULE              JN-AN-1162 JenNet-IP Smart Home
 *
 * DESCRIPTION         DioControl MIB Implementation
 */
/****************************************************************************/
/*
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
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
 * Copyright NXP B.V. 2013. All rights reserved
 */
/****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Standard includes */
#include <string.h>
/* SDK includes */
#include <jendefs.h>
/* Hardware includes */
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>
/* Stack includes */
#include <Api.h>
#include <AppApi.h>
#include <JIP.h>
#include <6LP.h>
#include <AccessFunctions.h>
/* JenOS includes */
#include <dbg.h>
#include <dbg_uart.h>
#include <os.h>
#include <pdm.h>
/* Application device includes */
#include "MibDio.h"
#include "MibDioControl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Debug */
#define MIB_DIO_CONTROL_DBG TRUE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE tsMibDioControl *psMibDioControl;  /* MIB data */

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: MibDioControl_vInit
 *
 * DESCRIPTION:
 * Initialises data
 *
 ****************************************************************************/
PUBLIC void MibDioControl_vInit(thJIP_Mib          hMibDioControlInit,
                                tsMibDioControl *psMibDioControlInit)
{
    /* Debug */
    DBG_vPrintf(MIB_DIO_CONTROL_DBG, "\nMibDioControl_vInit() {%d}", sizeof(tsMibDioControl));

    /* Valid data pointer ? */
    if (psMibDioControlInit != (tsMibDioControl *) NULL)
    {

        /* Take copy of pointer to data */
        psMibDioControl = psMibDioControlInit;
        /* Take a copy of the MIB handle */
        psMibDioControl->hMib = hMibDioControlInit;
    }
}

/****************************************************************************
 *
 * NAME: MibDioControl_vRegister
 *
 * DESCRIPTION:
 * Registers MIB
 *
 ****************************************************************************/
PUBLIC void MibDioControl_vRegister(void)
{
    teJIP_Status eStatus;

    /* Register MIB */
    eStatus = eJIP_RegisterMib(psMibDioControl->hMib);
    /* Debug */
    DBG_vPrintf(MIB_DIO_CONTROL_DBG, "\nMibDioControl_vRegister()");
    DBG_vPrintf(MIB_DIO_CONTROL_DBG, "\n\teJIP_RegisterMib(DioControl)=%d", eStatus);

}

/****************************************************************************
 *
 * NAME: MibDioControl_eSetOutputOn
 *
 * DESCRIPTION:
 * Sets OutputOn variable
 *
 ****************************************************************************/
PUBLIC teJIP_Status MibDioControl_eSetOutputOn(uint32 u32Val, void *pvCbData)
{
    teJIP_Status   eReturn = E_JIP_OK;

	/* Write new value */
	vAHI_DioSetOutput(0, u32Val);

	vJIP_NotifyChanged(psMibDioControl->hMib, VAR_IX_DIO_CONTROL_OUTPUT_ON);


    return eReturn;
}

/****************************************************************************
 *
 * NAME: MibDioControl_eSetOutputOff
 *
 * DESCRIPTION:
 * Sets OutputOff variable
 *
 ****************************************************************************/
PUBLIC teJIP_Status MibDioControl_eSetOutputOff(uint32 u32Val, void *pvCbData)
{
    teJIP_Status   eReturn = E_JIP_OK;

	/* Write new value */
	vAHI_DioSetOutput(u32Val, 0);

	vJIP_NotifyChanged(psMibDioControl->hMib, VAR_IX_DIO_CONTROL_OUTPUT_OFF);



    return eReturn;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
