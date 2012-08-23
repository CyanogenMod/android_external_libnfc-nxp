/*
 * Copyright (C) 2010 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
* =========================================================================== *
*                                                                             *
*                                                                             *
* \file  phHciNfc_CE_A.h                                             *
* \brief HCI card emulation management routines.                              *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Aug 14 17:01:27 2009 $                                           *
* $Author: ing04880 $                                                         *
* $Revision: 1.5 $                                                            *
* $Aliases: NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $                                                                *
*                                                                             *
* =========================================================================== *
*/


#ifndef PHHCINFC_CE_H
#define PHHCINFC_CE_H

/*@}*/


/**
*  \name HCI
*
* File: \ref phHciNfc_CE_A.h
*
*/
/*@{*/
#define PHHCINFC_CE_FILEREVISION "$Revision: 1.5 $" /**< \ingroup grp_file_attributes */
#define PHHCINFC_CE_FILEALIASES  "$Aliases: NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $"     /**< \ingroup grp_file_attributes */
/*@}*/

/*
***************************** Header File Inclusion ****************************
*/

#include <phHal4Nfc_Internal.h>

/*
****************************** Macro Definitions *******************************
*/

#ifdef _WIN32
/*Timeout value for recv data timer for CE.This timer is used for creating
  Asynchronous behavior in the scenario where the data is received even before
  the upper layer calls the phHal4Nfc_receive().*/
#define     PH_HAL4NFC_CE_RECV_CB_TIMEOUT       100U
#else
#define     PH_HAL4NFC_CE_RECV_CB_TIMEOUT      0x00U
#endif/*#ifdef _WIN32*/
#define CE_MAX_SEND_BUFFER_LEN             257

/*
******************** Enumeration and Structure Definition **********************
*/

/* Context for secured element */
typedef struct phLibNfc_CeCtxt
{
    /* Store SE discovery notification callback and its context */
    pphLibNfc_CE_NotificationCb_t       pCeListenerNtfCb;
    void                                *pCeListenerCtxt;

}phLibNfc_CeCtxt_t;


/*
*********************** Function Prototype Declaration *************************
*/
/*timer callback to send already buffered receive data to upper layer*/
void phHal4Nfc_CE_RecvTimerCb(uint32_t CERecvTimerId, void *pContext);

/* SE register listner response notification */
void phLibNfc_CeNotification(void                     *context,
                        phHal_eNotificationType_t     type,
                        phHal4Nfc_NotificationInfo_t  info,
                        NFCSTATUS                     status
                        );


/*Activation complete handler*/
void phHal4Nfc_CEActivateComplete(
                             phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,
                             void *pInfo
                            );

/*Deactivation complete handler*/
void phHal4Nfc_HandleCEDeActivate(
                               phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,
                               void *pInfo
                               );
#endif /* PHHCINFC_CE_H */


