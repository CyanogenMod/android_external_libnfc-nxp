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

/**
 * \file  phFriNfc_LlcpTransport_Connectionless.c
 * \brief 
 *
 * Project: NFC-FRI
 *
 */
/*include files*/
#include <phOsalNfc.h>
#include <phLibNfcStatus.h>
#include <phLibNfc.h>
#include <phNfcLlcpTypes.h>
#include <phFriNfc_LlcpTransport.h>
#include <phFriNfc_Llcp.h>

/* TODO: comment function Handle_Connectionless_IncommingFrame */
void Handle_Connectionless_IncommingFrame(phFriNfc_LlcpTransport_t      *pLlcpTransport,
                                          phNfc_sData_t                 *psData,
                                          uint8_t                       dsap,
                                          uint8_t                       ssap)
{
   uint8_t i=0;

   for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
   {
      /* Test if a socket is registered to get this packet */
      if(pLlcpTransport->pSocketTable[i].socket_sSap == dsap && pLlcpTransport->pSocketTable[i].bSocketRecvPending == TRUE)
      {
         /* Reset the RecvPending variable */
         pLlcpTransport->pSocketTable[i].bSocketRecvPending = FALSE;

         /* Copy the received buffer into the receive buffer */   
         memcpy(pLlcpTransport->pSocketTable[i].sSocketRecvBuffer->buffer,psData->buffer,psData->length);

         /* Update the received length */
         *pLlcpTransport->pSocketTable[i].receivedLength = psData->length;

         /* call the Recv callback */
         pLlcpTransport->pSocketTable[i].pfSocketRecvFrom_Cb(pLlcpTransport->pSocketTable[i].pRecvContext,ssap,NFCSTATUS_SUCCESS);
         break;
      }
   }
}

/* TODO: comment function phFriNfc_LlcpTransport_Connectionless_SendTo_CB */
static void phFriNfc_LlcpTransport_Connectionless_SendTo_CB(void*        pContext,
                                                            NFCSTATUS    status)
{
   phFriNfc_LlcpTransport_Socket_t   *pLlcpSocket = (phFriNfc_LlcpTransport_Socket_t*)pContext;

   /* Reset the SendPending variable */
   pLlcpSocket->bSocketSendPending = FALSE;

   /* Call the send callback */
   pLlcpSocket->pfSocketSend_Cb(pLlcpSocket->pSendContext,status);
   
}

static void phFriNfc_LlcpTransport_Connectionless_Abort(phFriNfc_LlcpTransport_Socket_t* pLlcpSocket)
{
   if (pLlcpSocket->pfSocketSend_Cb != NULL)
   {
      pLlcpSocket->pfSocketSend_Cb(pLlcpSocket->pSendContext, NFCSTATUS_ABORTED);
      pLlcpSocket->pSendContext = NULL;
      pLlcpSocket->pfSocketSend_Cb = NULL;
   }
   if (pLlcpSocket->pfSocketRecvFrom_Cb != NULL)
   {
      pLlcpSocket->pfSocketRecvFrom_Cb(pLlcpSocket->pRecvContext, 0, NFCSTATUS_ABORTED);
      pLlcpSocket->pRecvContext = NULL;
      pLlcpSocket->pfSocketRecvFrom_Cb = NULL;
      pLlcpSocket->pfSocketRecv_Cb = NULL;
   }
   pLlcpSocket->pAcceptContext = NULL;
   pLlcpSocket->pfSocketAccept_Cb = NULL;
   pLlcpSocket->pListenContext = NULL;
   pLlcpSocket->pfSocketListen_Cb = NULL;
   pLlcpSocket->pConnectContext = NULL;
   pLlcpSocket->pfSocketConnect_Cb = NULL;
   pLlcpSocket->pDisonnectContext = NULL;
   pLlcpSocket->pfSocketDisconnect_Cb = NULL;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Close a socket on a LLCP-connectionless device</b>.
*
* This function closes a LLCP socket previously created using phFriNfc_LlcpTransport_Socket.
*
* \param[in]  pLlcpSocket                    A pointer to a phFriNfc_LlcpTransport_Socket_t.

* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Connectionless_Close(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket)
{
   /* Reset the pointer to the socket closed */
   pLlcpSocket->eSocket_State                      = phFriNfc_LlcpTransportSocket_eSocketDefault;
   pLlcpSocket->eSocket_Type                       = phFriNfc_LlcpTransport_eDefaultType;
   pLlcpSocket->pContext                           = NULL;
   pLlcpSocket->pSocketErrCb                       = NULL;
   pLlcpSocket->socket_sSap                        = PHFRINFC_LLCP_SAP_DEFAULT;
   pLlcpSocket->socket_dSap                        = PHFRINFC_LLCP_SAP_DEFAULT;
   pLlcpSocket->bSocketRecvPending                 = FALSE;
   pLlcpSocket->bSocketSendPending                 = FALSE;
   pLlcpSocket->bSocketListenPending               = FALSE;
   pLlcpSocket->bSocketDiscPending                 = FALSE;
   pLlcpSocket->RemoteBusyConditionInfo            = FALSE;
   pLlcpSocket->ReceiverBusyCondition              = FALSE;
   pLlcpSocket->socket_VS                          = 0;
   pLlcpSocket->socket_VSA                         = 0;
   pLlcpSocket->socket_VR                          = 0;
   pLlcpSocket->socket_VRA                         = 0;

   phFriNfc_LlcpTransport_Connectionless_Abort(pLlcpSocket);

   memset(&pLlcpSocket->sSocketOption, 0x00, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

   if (pLlcpSocket->sServiceName.buffer != NULL) {
       phOsalNfc_FreeMemory(pLlcpSocket->sServiceName.buffer);
   }
   pLlcpSocket->sServiceName.buffer = NULL;
   pLlcpSocket->sServiceName.length = 0;

   return NFCSTATUS_SUCCESS;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Send data on a socket to a given destination SAP</b>.
*
* This function is used to write data on a socket to a given destination SAP.
* This function can only be called on a connectionless socket.
* 
*
* \param[in]  pLlcpSocket        A pointer to a LlcpSocket created.
* \param[in]  nSap               The destination SAP.
* \param[in]  psBuffer           The buffer containing the data to send.
* \param[in]  pSend_RspCb        The callback to be called when the 
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_PENDING                  Reception operation is in progress,
*                                            pSend_RspCb will be called upon completion.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Connectionless_SendTo(phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
                                                       uint8_t                                     nSap,
                                                       phNfc_sData_t*                              psBuffer,
                                                       pphFriNfc_LlcpTransportSocketSendCb_t       pSend_RspCb,
                                                       void*                                       pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Store send callback  and context*/
   pLlcpSocket->pfSocketSend_Cb = pSend_RspCb;
   pLlcpSocket->pSendContext    = pContext;

   /* Test if a send is pending with this socket */
   if(pLlcpSocket->bSocketSendPending == TRUE)
   {
      status = NFCSTATUS_FAILED;
      pLlcpSocket->pfSocketSend_Cb(pLlcpSocket->pSendContext,status);
   }
   else
   {
      /* Fill the psLlcpHeader stuture with the DSAP,PTYPE and the SSAP */
      pLlcpSocket->sLlcpHeader.dsap  = nSap;
      pLlcpSocket->sLlcpHeader.ptype = PHFRINFC_LLCP_PTYPE_UI;
      pLlcpSocket->sLlcpHeader.ssap  = pLlcpSocket->socket_sSap;

      pLlcpSocket->bSocketSendPending = TRUE;

      /* Send to data to the approiate socket */
      status =  phFriNfc_Llcp_Send(pLlcpSocket->psTransport->pLlcp,
                                   &pLlcpSocket->sLlcpHeader,
                                   NULL,
                                   psBuffer,
                                   phFriNfc_LlcpTransport_Connectionless_SendTo_CB,
                                   pLlcpSocket);
   }

   return status;
}


 /**
* \ingroup grp_lib_nfc
* \brief <b>Read data on a socket and get the source SAP</b>.
*
* This function is the same as phLibNfc_Llcp_Recv, except that the callback includes
* the source SAP. This functions can only be called on a connectionless socket.
* 
*
* \param[in]  pLlcpSocket        A pointer to a LlcpSocket created.
* \param[in]  psBuffer           The buffer receiving the data.
* \param[in]  pRecv_RspCb        The callback to be called when the 
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_PENDING                  Reception operation is in progress,
*                                            pRecv_RspCb will be called upon completion.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phLibNfc_LlcpTransport_Connectionless_RecvFrom(phFriNfc_LlcpTransport_Socket_t                   *pLlcpSocket,
                                                         phNfc_sData_t*                                    psBuffer,
                                                         pphFriNfc_LlcpTransportSocketRecvFromCb_t         pRecv_Cb,
                                                         void                                              *pContext)
{
   NFCSTATUS status = NFCSTATUS_PENDING;

   if(pLlcpSocket->bSocketRecvPending)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      /* Store the callback and context*/
      pLlcpSocket->pfSocketRecvFrom_Cb  = pRecv_Cb;
      pLlcpSocket->pRecvContext         = pContext;

      /* Store the pointer to the receive buffer */
      pLlcpSocket->sSocketRecvBuffer   =  psBuffer;
      pLlcpSocket->receivedLength      =  &psBuffer->length;

      /* Set RecvPending to TRUE */
      pLlcpSocket->bSocketRecvPending = TRUE;
   }
   return status;
}
