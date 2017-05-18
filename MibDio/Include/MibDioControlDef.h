/****************************************************************************/
/*
 * MODULE              JN-AN-1162 JenNet-IP Smart Home
 *
 * DESCRIPTION         DioControl MIB Definition
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
/***        Macro Definitions                                             ***/
/****************************************************************************/
#undef MIB_HEADER
#define MIB_HEADER "MibDioControlDef.h"

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Stack includes */
#include <jip_define_mib.h>
/* Application common includes */
#include "MibDio.h"

/*****************************************************************************
	MIB definition
/****************************************************************************/
/* DioControl MIB */
START_DEFINE_MIB(MIB_ID_DIO_CONTROL, DioControlDef)
/*         ID 					   	      Type    Name       Disp  Flags Access                 Cache Security */
DEFINE_VAR(VAR_IX_DIO_CONTROL_OUTPUT_ON,  UINT32, OutputOn,  NULL, 0,    (READ | WRITE | TRAP), NONE, NONE) /* Function: vAHI_DioSetOutput(u32On,  0) */
DEFINE_VAR(VAR_IX_DIO_CONTROL_OUTPUT_OFF, UINT32, OutputOff, NULL, 0,    (READ | WRITE | TRAP), NONE, NONE) /* Function: vAHI_DioSetOutput(u32Off, 0) */
END_DEFINE_MIB(DioControlDef)

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
