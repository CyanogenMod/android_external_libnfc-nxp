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
* \file  phHciNfc_CE.c                                             *
* \brief HCI card emulation management routines.                              *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Thu Dec 29 2011 $                                           *
* $Author: doug yeager (doug@simplytapp.com) $                        *
* $Revision: 0.01 $                                                           *
*                                                                             *
* =========================================================================== *
*/

/*
***************************** Header File Inclusion ****************************
*/
#include <phLibNfc.h>
#include <phLibNfc_Internal.h>
#include <phHal4Nfc_Internal.h>
/*
****************************** Macro Definitions *******************************
*/
#if defined (HOST_EMULATION)
#include <phHciNfc_CE.h>

/*
*************************** Structure and Enumeration ***************************
*/


/*
*************************** Static Function Declaration **************************
*/


/*
*************************** Function Definitions ***************************
*/

/**
* Registers notification handler to handle CE specific events
*/
NFCSTATUS phLibNfc_CE_NtfRegister   (
                            pphLibNfc_CE_NotificationCb_t  pCE_NotificationCb,
                            void                            *pContext
                            )
{
     NFCSTATUS         Status = NFCSTATUS_SUCCESS;
     pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)gpphLibContext;

     if((NULL == gpphLibContext) ||
         (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
     {
         Status = NFCSTATUS_NOT_INITIALISED;
     }
     else if((pCE_NotificationCb == NULL)
         ||(NULL == pContext))
     {
         /*parameters sent by upper layer are not valid*/
         Status = NFCSTATUS_INVALID_PARAMETER;
     }
     else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
     {
         Status = NFCSTATUS_SHUTDOWN;
     }
     else
     {
         /*Register CE notification with lower layer.
         Any activity on Smx or UICC will be notified */
         Status = phHal4Nfc_RegisterNotification(
                                            pLibContext->psHwReference,
                                            eRegisterHostCardEmulation,
                                            phLibNfc_CeNotification,
                                            (void*)pLibContext);
        if(Status == NFCSTATUS_SUCCESS)
        {
            pLibContext->sCeContext.pCeListenerNtfCb = pCE_NotificationCb;
            pLibContext->sCeContext.pCeListenerCtxt=pContext;
        }
        else
        {
            /* Registration failed */
            Status = NFCSTATUS_FAILED;
        }
     }
     return Status;
}
/**
* CE Notification events are notified with this callback
*/
void phLibNfc_CeNotification(void  *context,
                                    phHal_eNotificationType_t    type,
                                    phHal4Nfc_NotificationInfo_t  info,
                                    NFCSTATUS                   status)
{
    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)context;
    phHal_sEventInfo_t  *pEvtInfo = NULL;

    if(pLibContext != gpphLibContext)
    {
        /*wrong context returned*/
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if((status == NFCSTATUS_SUCCESS) && (type == NFC_EVENT_NOTIFICATION))
        {
            pEvtInfo = info.psEventInfo;
            status = NFCSTATUS_SUCCESS;
            if(pEvtInfo->eventHost == phHal_eHostController &&
               (pEvtInfo->eventSource==phHal_eISO14443_A_PICC ||
                pEvtInfo->eventSource==phHal_eISO14443_B_PICC)
              )
            {
                phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = NULL;
                Hal4Ctxt = (phHal4Nfc_Hal4Ctxt_t *)pLibContext->psHwReference->hal_context;
                switch(pEvtInfo->eventType)
                {
                    case NFC_EVT_ACTIVATED:
                    {
                        phHal4Nfc_CEActivateComplete(Hal4Ctxt,pEvtInfo);
                        break;
                    }
                    case NFC_EVT_DEACTIVATED:
                    {
                        phHal4Nfc_HandleCEDeActivate(Hal4Ctxt,pEvtInfo);
                        break;
                    }
                    case NFC_EVT_FIELD_ON:
                    {
                        (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_EvtFieldOn,
                            NULL,
                            status);
                        break;
                    }
                    case NFC_EVT_FIELD_OFF:
                    {
                        (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_EvtFieldOff,
                            NULL,
                            status);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else
            {

            }
         }
    }
  return;
}

/*Activation complete handler*/
void phHal4Nfc_CEActivateComplete(
                             phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,
                             void *pInfo
                            )
{
    phHal_sEventInfo_t *psEventInfo = (phHal_sEventInfo_t *)pInfo;

    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)Hal4Ctxt->sUpperLayerInfo.HCEEventNotificationCtxt;

    NFCSTATUS Status = NFCSTATUS_SUCCESS;
    Hal4Ctxt->sTgtConnectInfo.EmulationState = NFC_EVT_ACTIVATED;
    /*if P2p notification is registered*/
    if( NULL != pLibContext->sCeContext.pCeListenerNtfCb)
    {
        /*Allocate remote device Info for CE target*/
        Hal4Ctxt->rem_dev_list[0]
                = (phHal_sRemoteDevInformation_t *)
                    phOsalNfc_GetMemory(
                    sizeof(phHal_sRemoteDevInformation_t)
                    );
        if(NULL == Hal4Ctxt->rem_dev_list[0])
        {
            phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,0);
            Status = PHNFCSTVAL(CID_NFC_HAL ,
                NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            (void)memset((void *)Hal4Ctxt->rem_dev_list[0],
                                0,sizeof(phHal_sRemoteDevInformation_t));
            /*Copy device info*/
            (void)memcpy(Hal4Ctxt->rem_dev_list[0],
                                psEventInfo->eventInfo.pRemoteDevInfo,
                                sizeof(phHal_sRemoteDevInformation_t)
                                );
            Hal4Ctxt->psADDCtxtInfo->nbr_of_devices = 1;
            /*Allocate Trcv context info*/
            if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
            {
                Hal4Ctxt->psTrcvCtxtInfo= (pphHal4Nfc_TrcvCtxtInfo_t)
                    phOsalNfc_GetMemory((uint32_t)
                    (sizeof(phHal4Nfc_TrcvCtxtInfo_t)));
                if(NULL != Hal4Ctxt->psTrcvCtxtInfo)
                {
                    (void)memset(Hal4Ctxt->psTrcvCtxtInfo,0,
                        sizeof(phHal4Nfc_TrcvCtxtInfo_t));
                    Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus
                        = NFCSTATUS_PENDING;
                    Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                                        = PH_OSALNFC_INVALID_TIMER_ID;
                }
            }
            if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
            {
                phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,0);
                Status= PHNFCSTVAL(CID_NFC_HAL ,
                    NFCSTATUS_INSUFFICIENT_RESOURCES);
            }
            else
            {
                /*Update state*/
                Hal4Ctxt->Hal4CurrentState = eHal4StateEmulation;
                Hal4Ctxt->Hal4NextState = eHal4StateInvalid;
                /*set session Opened ,this will keep track of whether the session
                 is alive.will be reset if a Event DEACTIVATED is received*/
                Hal4Ctxt->rem_dev_list[0]->SessionOpened = TRUE;
                Hal4Ctxt->sTgtConnectInfo.psConnectedDevice = Hal4Ctxt->rem_dev_list[0];
                gpphLibContext->Connected_handle = Hal4Ctxt->sTgtConnectInfo.psConnectedDevice;
                if(psEventInfo->eventSource==phHal_eISO14443_A_PICC)
                {
                   (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_A_EvtActivated,
                            (uint32_t)Hal4Ctxt->rem_dev_list[0],
                            Status);
                }
                else
                {
                   (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_B_EvtActivated,
                            (uint32_t)Hal4Ctxt->rem_dev_list[0],
                            Status);
                }
            }
        }
    }
    return;
}

/*Deactivation complete handler*/
void phHal4Nfc_HandleCEDeActivate(
                               phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,
                               void *pInfo
                               )
{
    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)Hal4Ctxt->sUpperLayerInfo.HCEEventNotificationCtxt;

    pphHal4Nfc_TransceiveCallback_t pUpperRecvCb = NULL;
    pphHal4Nfc_SendCallback_t pUpperSendCb = NULL;
    phHal4Nfc_NotificationInfo_t uNotificationInfo;
    uNotificationInfo.psEventInfo = (phHal_sEventInfo_t *)pInfo;
    uint32_t handle=0;
    /*session is closed*/
    if(NULL != Hal4Ctxt->rem_dev_list[0])
    {
        if(Hal4Ctxt->rem_dev_list[0]->RemoteDevInfo.Iso14443_4_PCD_Info.buffer != NULL)
        {
          phOsalNfc_FreeMemory((void *)(Hal4Ctxt->rem_dev_list[0]->RemoteDevInfo.Iso14443_4_PCD_Info.buffer));
          Hal4Ctxt->rem_dev_list[0]->RemoteDevInfo.Iso14443_4_PCD_Info.buffer = NULL;
        }
        Hal4Ctxt->rem_dev_list[0]->RemoteDevInfo.Iso14443_4_PCD_Info.length = 0;
        Hal4Ctxt->rem_dev_list[0]->SessionOpened = FALSE;
        handle = (uint32_t)Hal4Ctxt->rem_dev_list[0];
        Hal4Ctxt->rem_dev_list[0] = NULL;
        Hal4Ctxt->psADDCtxtInfo->nbr_of_devices = 0;
    }
    Hal4Ctxt->sTgtConnectInfo.psConnectedDevice = NULL;
    gpphLibContext->Connected_handle = 0x0000;
    /*Update state*/
    Hal4Ctxt->Hal4CurrentState = eHal4StateOpenAndReady;
    Hal4Ctxt->Hal4NextState  = eHal4StateInvalid;
    Hal4Ctxt->sTgtConnectInfo.EmulationState = NFC_EVT_DEACTIVATED;
    /*If Trcv ctxt info is allocated ,free it here*/
    if(NULL != Hal4Ctxt->psTrcvCtxtInfo)
    {
        if(PH_OSALNFC_INVALID_TIMER_ID !=
            Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId)
        {
            phOsalNfc_Timer_Stop(Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId);
            phOsalNfc_Timer_Delete(Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId);
        }
        pUpperRecvCb = (pphHal4Nfc_TransceiveCallback_t)Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb;
        pUpperSendCb = Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb;
        /*Free Hal4 resources used by Target*/
        if (NULL != Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer)
        {
            phOsalNfc_FreeMemory(Hal4Ctxt->psTrcvCtxtInfo->
                sLowerRecvData.buffer);
        }
        if((NULL == Hal4Ctxt->sTgtConnectInfo.psConnectedDevice)
            && (NULL != Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData))
        {
            phOsalNfc_FreeMemory(Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData);
        }
        phOsalNfc_FreeMemory(Hal4Ctxt->psTrcvCtxtInfo);
        Hal4Ctxt->psTrcvCtxtInfo = NULL;
    }
    /*if recv callback is pending*/
    if(NULL != pUpperRecvCb)
    {
        (*pUpperRecvCb)(
                        Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                        Hal4Ctxt->sTgtConnectInfo.psConnectedDevice,
                        NULL,
                        NFCSTATUS_DESELECTED
                        );
    }
    /*if send callback is pending*/
    else if(NULL != pUpperSendCb)
    {
        (*pUpperSendCb)(
                        Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                        NFCSTATUS_DESELECTED
                        );
    }
    /*if CENotification is registered*/
    else if(NULL != pLibContext->sCeContext.pCeListenerNtfCb)
    {
      if(uNotificationInfo.psEventInfo->eventSource==phHal_eISO14443_A_PICC)
      {
        (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_A_EvtDeActivated,
                            handle,
                            NFCSTATUS_SUCCESS);
      }
      else
      {
        (*pLibContext->sCeContext.pCeListenerNtfCb)(
                            pLibContext->sCeContext.pCeListenerCtxt,
                            phLibNfc_eCE_B_EvtDeActivated,
                            handle,
                            NFCSTATUS_SUCCESS);
      }
    }
}

/**
 * Unregister the CE Notification.
 */
NFCSTATUS phLibNfc_CE_NtfUnregister(void)
{
    NFCSTATUS Status = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)gpphLibContext;

    if((NULL == gpphLibContext) ||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        /*Lib Nfc is not initialized*/
        Status = NFCSTATUS_NOT_INITIALISED;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        Status = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        /*Unregister CE event notification with lower layer.
        even some transaction happens on UICC or Smx will not
        be notified afterworlds */
        Status = phHal4Nfc_UnregisterNotification(
                                                pLibContext->psHwReference,
                                                eRegisterHostCardEmulation,
                                                pLibContext);
        if(Status != NFCSTATUS_SUCCESS)
        {
            /*Unregister failed*/
            Status=NFCSTATUS_FAILED;
        }
        pLibContext->sCeContext.pCeListenerNtfCb=NULL;
        pLibContext->sCeContext.pCeListenerCtxt=NULL;
    }
    return Status;
}


/*Timer callback for recv data timer for CE.This timer is used for creating
  Asynchronous behavior in the scenario where the data is received even before
  the upper layer calls the phHal4Nfc_receive().*/
void phHal4Nfc_CE_RecvTimerCb(uint32_t CERecvTimerId, void *pContext)
{
    phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = (phHal4Nfc_Hal4Ctxt_t *)(
                                            gpphHal4Nfc_Hwref->hal_context);
    pphLibNfc_TransceiveCallback_t pUpperRecvCb = NULL;
    NFCSTATUS RecvDataBufferStatus = NFCSTATUS_PENDING;
    PHNFC_UNUSED_VARIABLE(pContext);

    phOsalNfc_Timer_Stop(CERecvTimerId);
    phOsalNfc_Timer_Delete(CERecvTimerId);
    if(NULL != Hal4Ctxt->psTrcvCtxtInfo)
    {
        RecvDataBufferStatus = Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus;
        Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus = NFCSTATUS_PENDING;

        Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
            = PH_OSALNFC_INVALID_TIMER_ID;
        /*Update state*/
        Hal4Ctxt->Hal4NextState = (eHal4StateTransaction
             == Hal4Ctxt->Hal4NextState?eHal4StateInvalid:Hal4Ctxt->Hal4NextState);
        /*Provide address of received data to upper layer data pointer*/
        Hal4Ctxt->psTrcvCtxtInfo->psUpperRecvData
            = &(Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData);
        /*Chk NULL and call recv callback*/
        if(Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb != NULL)
        {
            pUpperRecvCb = (pphLibNfc_TransceiveCallback_t)Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb;
            Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb = NULL;
            (*pUpperRecvCb)(
                Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                Hal4Ctxt->sTgtConnectInfo.psConnectedDevice,
                Hal4Ctxt->psTrcvCtxtInfo->psUpperRecvData,
                RecvDataBufferStatus
                );
        }
    }
    return;
}


#endif /* #if defined (HOST_EMULATION) */


