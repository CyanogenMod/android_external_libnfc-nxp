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
 * \file  phFriNfc_LlcpTransport.c
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
#include <phFriNfc_Llcp.h>
#include <phFriNfc_LlcpTransport.h>
#include <phFriNfc_LlcpTransport_Connectionless.h>
#include <phFriNfc_LlcpTransport_Connection.h>


/* TODO: comment function Transport recv CB */
static void phFriNfc_LlcpTransport__Recv_CB(void            *pContext,
                                            phNfc_sData_t   *psData,
                                            NFCSTATUS        status)
{
   phFriNfc_Llcp_sPacketHeader_t   sLlcpLocalHeader;
   uint8_t   dsap;
   uint8_t   ptype;
   uint8_t   ssap;

   phFriNfc_LlcpTransport_t* pLlcpTransport = (phFriNfc_LlcpTransport_t*)pContext;

   if(status != NFCSTATUS_SUCCESS)
   {
      pLlcpTransport->LinkStatusError = TRUE;
   }
   else
   {
      phFriNfc_Llcp_Buffer2Header( psData->buffer,0x00, &sLlcpLocalHeader);

      dsap  = (uint8_t)sLlcpLocalHeader.dsap;
      ptype = (uint8_t)sLlcpLocalHeader.ptype;
      ssap  = (uint8_t)sLlcpLocalHeader.ssap;

      /* Update the length value (without the header length) */
      psData->length = psData->length - PHFRINFC_LLCP_PACKET_HEADER_SIZE;

      /* Update the buffer pointer */
      psData->buffer = psData->buffer + PHFRINFC_LLCP_PACKET_HEADER_SIZE;

      switch(ptype)
      {
      /* Connectionless */
      case PHFRINFC_LLCP_PTYPE_UI:
         {
            Handle_Connectionless_IncommingFrame(pLlcpTransport,
                                                 psData,
                                                 dsap,
                                                 ssap);
         }break;

      /* Connection oriented */
      /* NOTE: forward reserved PTYPE to enable FRMR sending */
      case PHFRINFC_LLCP_PTYPE_CONNECT:
      case PHFRINFC_LLCP_PTYPE_CC:
      case PHFRINFC_LLCP_PTYPE_DISC:
      case PHFRINFC_LLCP_PTYPE_DM:
      case PHFRINFC_LLCP_PTYPE_I:
      case PHFRINFC_LLCP_PTYPE_RR:
      case PHFRINFC_LLCP_PTYPE_RNR:
      case PHFRINFC_LLCP_PTYPE_FRMR:
      case PHFRINFC_LLCP_PTYPE_RESERVED1:
      case PHFRINFC_LLCP_PTYPE_RESERVED2:
      case PHFRINFC_LLCP_PTYPE_RESERVED3:
      case PHFRINFC_LLCP_PTYPE_RESERVED4:
         {
            Handle_ConnectionOriented_IncommingFrame(pLlcpTransport,
                                                     psData,
                                                     dsap,
                                                     ptype,
                                                     ssap);
         }break;
      default:
         {

         }break;
      }

      /*Restart the Receive Loop */
      status  = phFriNfc_Llcp_Recv(pLlcpTransport->pLlcp,
                                   phFriNfc_LlcpTransport__Recv_CB,
                                   pLlcpTransport);
   }
}


/* TODO: comment function Transport reset */
NFCSTATUS phFriNfc_LlcpTransport_Reset (phFriNfc_LlcpTransport_t      *pLlcpTransport,
                                        phFriNfc_Llcp_t               *pLlcp)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   /* Check for NULL pointers */
   if(pLlcpTransport == NULL || pLlcp == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Reset Transport structure */ 
      pLlcpTransport->pLlcp            = pLlcp;
      pLlcpTransport->LinkStatusError  = FALSE;
      pLlcpTransport->bSendPending     = FALSE;
      pLlcpTransport->bRecvPending     = FALSE;
      pLlcpTransport->bDmPending       = FALSE;
      pLlcpTransport->bFrmrPending     = FALSE;
      pLlcpTransport->socketIndex      = FALSE;
      pLlcpTransport->LinkStatusError  = 0;


      /* Reset all the socket info in the table */
      for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
      {
         pLlcpTransport->pSocketTable[i].eSocket_State                  = phFriNfc_LlcpTransportSocket_eSocketDefault;
         pLlcpTransport->pSocketTable[i].eSocket_Type                   = phFriNfc_LlcpTransport_eDefaultType;
         pLlcpTransport->pSocketTable[i].index                          = i;
         pLlcpTransport->pSocketTable[i].pContext                       = NULL;
         pLlcpTransport->pSocketTable[i].pListenContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pAcceptContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pRejectContext                 = NULL;
         pLlcpTransport->pSocketTable[i].pConnectContext                = NULL;
         pLlcpTransport->pSocketTable[i].pDisonnectContext              = NULL;
         pLlcpTransport->pSocketTable[i].pSendContext                   = NULL;
         pLlcpTransport->pSocketTable[i].pRecvContext                   = NULL;
         pLlcpTransport->pSocketTable[i].pSocketErrCb                   = NULL;
         pLlcpTransport->pSocketTable[i].bufferLinearLength             = 0;
         pLlcpTransport->pSocketTable[i].bufferSendMaxLength            = 0;
         pLlcpTransport->pSocketTable[i].bufferRwMaxLength              = 0;
         pLlcpTransport->pSocketTable[i].ReceiverBusyCondition          = FALSE;
         pLlcpTransport->pSocketTable[i].RemoteBusyConditionInfo        = FALSE;
         pLlcpTransport->pSocketTable[i].socket_sSap                    = PHFRINFC_LLCP_SAP_DEFAULT;
         pLlcpTransport->pSocketTable[i].socket_dSap                    = PHFRINFC_LLCP_SAP_DEFAULT;
         pLlcpTransport->pSocketTable[i].bSocketRecvPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketSendPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketListenPending           = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketDiscPending             = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketConnectPending          = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketAcceptPending           = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketRRPending               = FALSE;
         pLlcpTransport->pSocketTable[i].bSocketRNRPending              = FALSE;
         pLlcpTransport->pSocketTable[i].psTransport                    = pLlcpTransport;
         pLlcpTransport->pSocketTable[i].pfSocketSend_Cb                = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketRecv_Cb                = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketRecvFrom_Cb            = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketListen_Cb              = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketConnect_Cb             = NULL;
         pLlcpTransport->pSocketTable[i].pfSocketDisconnect_Cb          = NULL;
         pLlcpTransport->pSocketTable[i].socket_VS                      = 0;
         pLlcpTransport->pSocketTable[i].socket_VSA                     = 0;
         pLlcpTransport->pSocketTable[i].socket_VR                      = 0;
         pLlcpTransport->pSocketTable[i].socket_VRA                     = 0;
         pLlcpTransport->pSocketTable[i].remoteRW                       = 0;
         pLlcpTransport->pSocketTable[i].localRW                        = 0;
         pLlcpTransport->pSocketTable[i].remoteMIU                      = 0;
         pLlcpTransport->pSocketTable[i].localMIUX                      = 0;
         pLlcpTransport->pSocketTable[i].index                          = 0;
         pLlcpTransport->pSocketTable[i].indexRwRead                    = 0;
         pLlcpTransport->pSocketTable[i].indexRwWrite                   = 0;

         memset(&pLlcpTransport->pSocketTable[i].sSocketOption, 0x00, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

         if (pLlcpTransport->pSocketTable[i].sServiceName.buffer != NULL) {
            phOsalNfc_FreeMemory(pLlcpTransport->pSocketTable[i].sServiceName.buffer);
         }
         pLlcpTransport->pSocketTable[i].sServiceName.buffer = NULL;
         pLlcpTransport->pSocketTable[i].sServiceName.length = 0;
      }

      /* Start The Receive Loop */
      status  = phFriNfc_Llcp_Recv(pLlcpTransport->pLlcp,
                                   phFriNfc_LlcpTransport__Recv_CB,
                                   pLlcpTransport);
   }
   return status;
}

/* TODO: comment function Transport CloseAll */
NFCSTATUS phFriNfc_LlcpTransport_CloseAll (phFriNfc_LlcpTransport_t *pLlcpTransport)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   /* Check for NULL pointers */
   if(pLlcpTransport == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Close all sockets */
   for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
   {
      if(pLlcpTransport->pSocketTable[i].eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
      {
         switch(pLlcpTransport->pSocketTable[i].eSocket_State)
         {
         case phFriNfc_LlcpTransportSocket_eSocketConnected:
         case phFriNfc_LlcpTransportSocket_eSocketConnecting:
         case phFriNfc_LlcpTransportSocket_eSocketAccepted:
         case phFriNfc_LlcpTransportSocket_eSocketDisconnected:
         case phFriNfc_LlcpTransportSocket_eSocketDisconnecting:
         case phFriNfc_LlcpTransportSocket_eSocketRejected:
            phFriNfc_LlcpTransport_Close(&pLlcpTransport->pSocketTable[i]);
            break;
         }
      }
      else
      {
         phFriNfc_LlcpTransport_Close(&pLlcpTransport->pSocketTable[i]);
      }
   }

   return status;
}


/**
* \ingroup grp_lib_nfc
* \brief <b>Get the local options of a socket</b>.
*
* This function returns the local options (maximum packet size and receive window size) used
* for a given connection-oriented socket. This function shall not be used with connectionless
* sockets.
*
* \param[out] pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  psLocalOptions        A pointer to be filled with the local options of the socket.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_SocketGetLocalOptions(phFriNfc_LlcpTransport_Socket_t  *pLlcpSocket,
                                                       phLibNfc_Llcp_sSocketOptions_t   *psLocalOptions)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if (pLlcpSocket == NULL || psLocalOptions == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /*  Test the socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /*  Test the socket state */
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
     status = phFriNfc_LlcpTransport_ConnectionOriented_SocketGetLocalOptions(pLlcpSocket,
                                                                              psLocalOptions);
   }

   return status;
}


/**
* \ingroup grp_lib_nfc
* \brief <b>Get the local options of a socket</b>.
*
* This function returns the remote options (maximum packet size and receive window size) used
* for a given connection-oriented socket. This function shall not be used with connectionless
* sockets.
*
* \param[out] pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  psRemoteOptions       A pointer to be filled with the remote options of the socket.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_SocketGetRemoteOptions(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket,
                                                        phLibNfc_Llcp_sSocketOptions_t*    psRemoteOptions)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if (pLlcpSocket == NULL || psRemoteOptions == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /*  Test the socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /*  Test the socket state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_SocketGetRemoteOptions(pLlcpSocket,
                                                                                psRemoteOptions);
   }

   return status;
}

 /**
* \ingroup grp_fri_nfc
* \brief <b>Create a socket on a LLCP-connected device</b>.
*
* This function creates a socket for a given LLCP link. Sockets can be of two types : 
* connection-oriented and connectionless. If the socket is connection-oriented, the caller
* must provide a working buffer to the socket in order to handle incoming data. This buffer
* must be large enough to fit the receive window (RW * MIU), the remaining space being
* used as a linear buffer to store incoming data as a stream. Data will be readable later
* using the phLibNfc_LlcpTransport_Recv function.
* The options and working buffer are not required if the socket is used as a listening socket,
* since it cannot be directly used for communication.
*
* \param[in]  pLlcpSocketTable      A pointer to a table of PHFRINFC_LLCP_NB_SOCKET_DEFAULT sockets.
* \param[in]  eType                 The socket type.
* \param[in]  psOptions             The options to be used with the socket.
* \param[in]  psWorkingBuffer       A working buffer to be used by the library.
* \param[out] pLlcpSocket           A pointer on the socket to be filled with a
                                    socket found on the socket table.
* \param[in]  pErr_Cb               The callback to be called each time the socket
*                                   is in error.
* \param[in]  pContext              Upper layer context to be returned in the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                            declared in the options.
* \retval NFCSTATUS_INSUFFICIENT_RESOURCES   No more socket handle available.
* \retval NFCSTATUS_FAILED                   Operation failed.  
* */
NFCSTATUS phFriNfc_LlcpTransport_Socket(phFriNfc_LlcpTransport_t                  *pLlcpTransport,
                                        phFriNfc_LlcpTransport_eSocketType_t      eType,
                                        phFriNfc_LlcpTransport_sSocketOptions_t   *psOptions,
                                        phNfc_sData_t                             *psWorkingBuffer,
                                        phFriNfc_LlcpTransport_Socket_t           **pLlcpSocket,
                                        pphFriNfc_LlcpTransportSocketErrCb_t      pErr_Cb,
                                        void                                      *pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_sLinkParameters_t  LlcpLinkParamInfo;
   uint8_t index=0;
   uint8_t cpt;

   /* Check for NULL pointers */
   if (((NULL == psOptions) && (eType != phFriNfc_LlcpTransport_eConnectionLess)) || ((psWorkingBuffer == NULL) && (eType != phFriNfc_LlcpTransport_eConnectionLess)) || pLlcpSocket == NULL || pErr_Cb == NULL || pContext == NULL || pLlcpTransport == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      return status;
   }
   /*  Test the socket type*/
   else if(eType != phFriNfc_LlcpTransport_eConnectionOriented && eType != phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      return status;
   }

   /* Get the local parameters of the LLCP Link */
   status = phFriNfc_Llcp_GetLocalInfo(pLlcpTransport->pLlcp,&LlcpLinkParamInfo);
   if(status != NFCSTATUS_SUCCESS)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_FAILED);
   }
   else
   {
      /* Search a socket free in the Socket Table*/
      do
      {
         if(pLlcpTransport->pSocketTable[index].eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
         {
            /* Set the socket pointer to socket of the table */
            *pLlcpSocket = &pLlcpTransport->pSocketTable[index];

            /* Store the socket info in the socket pointer */
            pLlcpTransport->pSocketTable[index].eSocket_Type     = eType;
            pLlcpTransport->pSocketTable[index].pSocketErrCb     = pErr_Cb;

            /* Store the context of the upper layer */
            pLlcpTransport->pSocketTable[index].pContext   = pContext;

            /* Set the pointers to the different working buffers */
            if(pLlcpTransport->pSocketTable[index].eSocket_Type != phFriNfc_LlcpTransport_eConnectionLess)
            {
               /* Test the socket options */
               if((psOptions->rw > PHFRINFC_LLCP_RW_MAX) && (eType == phFriNfc_LlcpTransport_eConnectionOriented))
               {
                  status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
               }
               /* Set socket options */
               memcpy(&pLlcpTransport->pSocketTable[index].sSocketOption, psOptions, sizeof(phFriNfc_LlcpTransport_sSocketOptions_t));

               /* Set socket local params (MIUX & RW) */
               pLlcpTransport->pSocketTable[index].localMIUX = (pLlcpTransport->pSocketTable[index].sSocketOption.miu - PHFRINFC_LLCP_MIU_DEFAULT) & PHFRINFC_LLCP_TLV_MIUX_MASK;
               pLlcpTransport->pSocketTable[index].localRW   = pLlcpTransport->pSocketTable[index].sSocketOption.rw & PHFRINFC_LLCP_TLV_RW_MASK;

               /* Set the Max length for the Send and Receive Window Buffer */
               pLlcpTransport->pSocketTable[index].bufferSendMaxLength   = pLlcpTransport->pSocketTable[index].sSocketOption.miu;
               pLlcpTransport->pSocketTable[index].bufferRwMaxLength     = pLlcpTransport->pSocketTable[index].sSocketOption.miu * ((pLlcpTransport->pSocketTable[index].sSocketOption.rw & PHFRINFC_LLCP_TLV_RW_MASK));
               pLlcpTransport->pSocketTable[index].bufferLinearLength    = psWorkingBuffer->length - pLlcpTransport->pSocketTable[index].bufferSendMaxLength - pLlcpTransport->pSocketTable[index].bufferRwMaxLength;

               /* Test the connection oriented buffers length */
               if((pLlcpTransport->pSocketTable[index].bufferSendMaxLength + pLlcpTransport->pSocketTable[index].bufferRwMaxLength) > psWorkingBuffer->length  
                   || ((pLlcpTransport->pSocketTable[index].bufferLinearLength < PHFRINFC_LLCP_MIU_DEFAULT) && (pLlcpTransport->pSocketTable[index].bufferLinearLength != 0)))
               {
                  status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_BUFFER_TOO_SMALL);
                  return status;
               }

               /* Set the pointer and the length for the Receive Window Buffer */
               for(cpt=0;cpt<pLlcpTransport->pSocketTable[index].localRW;cpt++)
               {
                  pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].buffer = psWorkingBuffer->buffer + (cpt*pLlcpTransport->pSocketTable[index].sSocketOption.miu);
                  pLlcpTransport->pSocketTable[index].sSocketRwBufferTable[cpt].length = 0;
               }

               /* Set the pointer and the length for the Send Buffer */
               pLlcpTransport->pSocketTable[index].sSocketSendBuffer.buffer     = psWorkingBuffer->buffer + pLlcpTransport->pSocketTable[index].bufferRwMaxLength;
               pLlcpTransport->pSocketTable[index].sSocketSendBuffer.length     = pLlcpTransport->pSocketTable[index].bufferSendMaxLength;

               /** Set the pointer and the length for the Linear Buffer */
               pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.buffer   = psWorkingBuffer->buffer + pLlcpTransport->pSocketTable[index].bufferRwMaxLength + pLlcpTransport->pSocketTable[index].bufferSendMaxLength;
               pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length   = pLlcpTransport->pSocketTable[index].bufferLinearLength;

               if(pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length != 0)
               {
                  /* Init Cyclic Fifo */
                  phFriNfc_Llcp_CyclicFifoInit(&pLlcpTransport->pSocketTable[index].sCyclicFifoBuffer,
                                               pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.buffer,
                                               pLlcpTransport->pSocketTable[index].sSocketLinearBuffer.length);
               }
            }
            /* Store index of the socket */
            pLlcpTransport->pSocketTable[index].index = index;

            /* Set the socket into created state */
            pLlcpTransport->pSocketTable[index].eSocket_State = phFriNfc_LlcpTransportSocket_eSocketCreated;
            return status;
         }
         else
         {
            index++;
         }
      }while(index<PHFRINFC_LLCP_NB_SOCKET_MAX);
      
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INSUFFICIENT_RESOURCES);
   }
   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Close a socket on a LLCP-connected device</b>.
*
* This function closes a LLCP socket previously created using phFriNfc_LlcpTransport_Socket.
* If the socket was connected, it is first disconnected, and then closed.
*
* \param[in]  pLlcpSocket                    A pointer to a phFriNfc_LlcpTransport_Socket_t.

* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Close(phFriNfc_LlcpTransport_Socket_t*   pLlcpSocket)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if( pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_Type == phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Close(pLlcpSocket);
   }
   else if(pLlcpSocket->eSocket_Type ==  phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = phFriNfc_LlcpTransport_Connectionless_Close(pLlcpSocket);
   }
   else
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }

   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Bind a socket to a local SAP</b>.
*
* This function binds the socket to a local Service Access Point.
*
* \param[out] pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  pConfigInfo           A port number for a specific socket
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_ALREADY_REGISTERED       The selected SAP is already bound to another
                                             socket.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/

NFCSTATUS phFriNfc_LlcpTransport_Bind(phFriNfc_LlcpTransport_Socket_t    *pLlcpSocket,
                                      uint8_t                            nSap)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketCreated)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else if(nSap<2 || nSap>63)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Test if the nSap it is useb by another socket */
      for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
      {
         if((pLlcpSocket->psTransport->pSocketTable[i].socket_sSap == nSap) 
            && (pLlcpSocket->psTransport->pSocketTable[i].eSocket_Type == pLlcpSocket->eSocket_Type))
         {
            return status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_ALREADY_REGISTERED);
         }
      }
      /* Set the nSap value of the socket */
      pLlcpSocket->socket_sSap = nSap;
      /* Set the socket state */
      pLlcpSocket->eSocket_State = phFriNfc_LlcpTransportSocket_eSocketBound;
   }
   return status;
}

/*********************************************/
/*           ConnectionOriented              */
/*********************************************/

/**
* \ingroup grp_fri_nfc
* \brief <b>Listen for incoming connection requests on a socket</b>.
*
* This function switches a socket into a listening state and registers a callback on
* incoming connection requests. In this state, the socket is not able to communicate
* directly. The listening state is only available for connection-oriented sockets
* which are still not connected. The socket keeps listening until it is closed, and
* thus can trigger several times the pListen_Cb callback.
*
*
* \param[in]  pLlcpSocket        A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  psServiceName      A pointer to Service Name 
* \param[in]  pListen_Cb         The callback to be called each time the
*                                socket receive a connection request.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state to switch
*                                            to listening state.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Listen(phFriNfc_LlcpTransport_Socket_t*          pLlcpSocket,
                                        phNfc_sData_t                             *psServiceName,
                                        pphFriNfc_LlcpTransportSocketListenCb_t   pListen_Cb,
                                        void*                                     pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || pListen_Cb == NULL|| pContext == NULL || psServiceName == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Check for socket state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   /* Check for socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if a listen is not pending with this socket */
   else if(pLlcpSocket->bSocketListenPending)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test the length of the SN */
   else if(psServiceName->length > PHFRINFC_LLCP_SN_MAX_LENGTH)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Listen(pLlcpSocket,
                                                                psServiceName,
                                                                pListen_Cb,
                                                                pContext);
   }
   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Accept an incoming connection request for a socket</b>.
*
* This functions allows the client to accept an incoming connection request.
* It must be used with the socket provided within the listen callback. The socket
* is implicitly switched to the connected state when the function is called.
*
* \param[in]  pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  psOptions             The options to be used with the socket.
* \param[in]  psWorkingBuffer       A working buffer to be used by the library.
* \param[in]  pErr_Cb               The callback to be called each time the accepted socket
*                                   is in error.
* \param[in]  pContext              Upper layer context to be returned in the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_BUFFER_TOO_SMALL         The working buffer is too small for the MIU and RW
*                                            declared in the options.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Accept(phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                        phFriNfc_LlcpTransport_sSocketOptions_t*     psOptions,
                                        phNfc_sData_t*                               psWorkingBuffer,
                                        pphFriNfc_LlcpTransportSocketErrCb_t         pErr_Cb,
                                        pphFriNfc_LlcpTransportSocketAcceptCb_t      pAccept_RspCb,
                                        void*                                        pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || psOptions == NULL || psWorkingBuffer == NULL || pErr_Cb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Check for socket state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   /* Check for socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test the socket options */
   else if(psOptions->rw > PHFRINFC_LLCP_RW_MAX)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      /* Set the Max length for the Send and Receive Window Buffer */
      pLlcpSocket->bufferSendMaxLength   = psOptions->miu;
      pLlcpSocket->bufferRwMaxLength     = psOptions->miu * ((psOptions->rw & PHFRINFC_LLCP_TLV_RW_MASK));
      pLlcpSocket->bufferLinearLength    = psWorkingBuffer->length - pLlcpSocket->bufferSendMaxLength - pLlcpSocket->bufferRwMaxLength;

      /* Test the buffers length */
      if((pLlcpSocket->bufferSendMaxLength + pLlcpSocket->bufferRwMaxLength) > psWorkingBuffer->length
          || ((pLlcpSocket->bufferLinearLength < PHFRINFC_LLCP_MIU_DEFAULT)  && (pLlcpSocket->bufferLinearLength != 0)))
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_BUFFER_TOO_SMALL);
      }
      else
      {
         pLlcpSocket->psTransport->socketIndex = pLlcpSocket->index;

         status  = phFriNfc_LlcpTransport_ConnectionOriented_Accept(pLlcpSocket,
                                                                    psOptions,
                                                                    psWorkingBuffer,
                                                                    pErr_Cb,
                                                                    pAccept_RspCb,
                                                                    pContext);
      }
   }
   return status;
}

 /**
* \ingroup grp_fri_nfc
* \brief <b>Reject an incoming connection request for a socket</b>.
*
* This functions allows the client to reject an incoming connection request.
* It must be used with the socket provided within the listen callback. The socket
* is implicitly closed when the function is called.
*
* \param[in]  pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  pReject_RspCb         The callback to be call when the Reject operation is completed
* \param[in]  pContext              Upper layer context to be returned in the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Reject( phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                          pphFriNfc_LlcpTransportSocketRejectCb_t   pReject_RspCb,
                                          void                                      *pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Check for socket state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   /* Check for socket type */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phLibNfc_LlcpTransport_ConnectionOriented_Reject(pLlcpSocket,
                                                                pReject_RspCb,
                                                                pContext);
   }

   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Try to establish connection with a socket on a remote SAP</b>.
*
* This function tries to connect to a given SAP on the remote peer. If the
* socket is not bound to a local SAP, it is implicitly bound to a free SAP.
*
* \param[in]  pLlcpSocket        A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  nSap               The destination SAP to connect to.
* \param[in]  pConnect_RspCb     The callback to be called when the connection
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_PENDING                  Connection operation is in progress,
*                                            pConnect_RspCb will be called upon completion.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Connect( phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                          uint8_t                                    nSap,
                                          pphFriNfc_LlcpTransportSocketConnectCb_t   pConnect_RspCb,
                                          void*                                      pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || pConnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test the port number value */
   else if(nSap<02 || nSap>63)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionOriented socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }

   /* Test if the socket is not in connecting or connected state*/
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketCreated && pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else 
   {
      if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
      {
         /* Bind with a sSap Free */
         pLlcpSocket->socket_sSap = 32;

         for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
         {
            if(pLlcpSocket->socket_sSap == pLlcpSocket->psTransport->pSocketTable[i].socket_sSap)
            {
               pLlcpSocket->socket_sSap++;
            }
         }
      }
      pLlcpSocket->eSocket_State = phFriNfc_LlcpTransportSocket_eSocketBound;

      status = phFriNfc_LlcpTransport_ConnectionOriented_Connect(pLlcpSocket,
                                                                 nSap,
                                                                 NULL,
                                                                 pConnect_RspCb,
                                                                 pContext);
   }
   
   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Try to establish connection with a socket on a remote service, given its URI</b>.
*
* This function tries to connect to a SAP designated by an URI. If the
* socket is not bound to a local SAP, it is implicitly bound to a free SAP.
*
* \param[in]  pLlcpSocket           A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  psUri              The URI corresponding to the destination SAP to connect to.
* \param[in]  pConnect_RspCb     The callback to be called when the connection
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_PENDING                  Connection operation is in progress,
*                                            pConnect_RspCb will be called upon completion.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_ConnectByUri(phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                              phNfc_sData_t*                             psUri,
                                              pphFriNfc_LlcpTransportSocketConnectCb_t   pConnect_RspCb,
                                              void*                                      pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   uint8_t i;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || pConnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionOriented socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is not in connect pending or connected state*/
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketConnecting || pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test the length of the SN */
   else if(psUri->length > PHFRINFC_LLCP_SN_MAX_LENGTH)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else 
   {
      if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
      {
         /* Bind with a sSap Free */
         pLlcpSocket->socket_sSap = 32;

         for(i=0;i<PHFRINFC_LLCP_NB_SOCKET_MAX;i++)
         {
            if(pLlcpSocket->socket_sSap == pLlcpSocket->psTransport->pSocketTable[i].socket_sSap)
            {
               pLlcpSocket->socket_sSap++;
            }
         }
      }
      status = phFriNfc_LlcpTransport_ConnectionOriented_Connect(pLlcpSocket,
                                                                 PHFRINFC_LLCP_SAP_DEFAULT,
                                                                 psUri,
                                                                 pConnect_RspCb,
                                                                 pContext);
   }

   return status;
}

/**
* \ingroup grp_lib_nfc
* \brief <b>Disconnect a currently connected socket</b>.
*
* This function initiates the disconnection of a previously connected socket.
*
* \param[in]  pLlcpSocket        A pointer to a phFriNfc_LlcpTransport_Socket_t.
* \param[in]  pDisconnect_RspCb  The callback to be called when the 
*                                operation is completed.
* \param[in]  pContext           Upper layer context to be returned in
*                                the callback.
*
* \retval NFCSTATUS_SUCCESS                  Operation successful.
* \retval NFCSTATUS_INVALID_PARAMETER        One or more of the supplied parameters
*                                            could not be properly interpreted.
* \retval NFCSTATUS_PENDING                  Disconnection operation is in progress,
*                                            pDisconnect_RspCb will be called upon completion.
* \retval NFCSTATUS_INVALID_STATE            The socket is not in a valid state, or not of 
*                                            a valid type to perform the requsted operation.
* \retval NFCSTATUS_NOT_INITIALISED          Indicates stack is not yet initialized.
* \retval NFCSTATUS_SHUTDOWN                 Shutdown in progress.
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Disconnect(phFriNfc_LlcpTransport_Socket_t*           pLlcpSocket,
                                            pphLibNfc_LlcpSocketDisconnectCb_t         pDisconnect_RspCb,
                                            void*                                      pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || pDisconnect_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionOriented socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is connected  state*/
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
       status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   else
   {
      status = phLibNfc_LlcpTransport_ConnectionOriented_Disconnect(pLlcpSocket,
                                                                    pDisconnect_RspCb,
                                                                    pContext);
   }

   return status;
}

/**
* \ingroup grp_fri_nfc
* \brief <b>Send data on a socket</b>.
*
* This function is used to write data on a socket. This function
* can only be called on a connection-oriented socket which is already
* in a connected state.
* 
*
* \param[in]  pLlcpSocket        A pointer to a phFriNfc_LlcpTransport_Socket_t.
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
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Send(phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                      phNfc_sData_t*                               psBuffer,
                                      pphFriNfc_LlcpTransportSocketSendCb_t        pSend_RspCb,
                                      void*                                        pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || psBuffer == NULL || pSend_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionOriented socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is in connected state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketConnected)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   /* Test the length of the buffer */
   else if(psBuffer->length > pLlcpSocket->remoteMIU )
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if a send is pending */
   else if(pLlcpSocket->pfSocketSend_Cb != NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Send(pLlcpSocket,
                                                              psBuffer,
                                                              pSend_RspCb,
                                                              pContext);
   }

   return status;
}

 /**
* \ingroup grp_fri_nfc
* \brief <b>Read data on a socket</b>.
*
* This function is used to read data from a socket. It reads at most the
* size of the reception buffer, but can also return less bytes if less bytes
* are available. If no data is available, the function will be pending until
* more data comes, and the response will be sent by the callback. This function
* can only be called on a connection-oriented socket.
* 
*
* \param[in]  pLlcpSocket        A pointer to a phFriNfc_LlcpTransport_Socket_t.
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
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_Recv( phFriNfc_LlcpTransport_Socket_t*             pLlcpSocket,
                                       phNfc_sData_t*                               psBuffer,
                                       pphFriNfc_LlcpTransportSocketRecvCb_t        pRecv_RspCb,
                                       void*                                        pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;

   /* Check for NULL pointers */
   if(pLlcpSocket == NULL || psBuffer == NULL || pRecv_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionOriented socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionOriented)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is in connected state */
   else if(pLlcpSocket->eSocket_State == phFriNfc_LlcpTransportSocket_eSocketDefault)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if a receive is pending */
   else if(pLlcpSocket->bSocketRecvPending == TRUE)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
   }
   else
   {
      status = phFriNfc_LlcpTransport_ConnectionOriented_Recv(pLlcpSocket,
                                                              psBuffer,
                                                              pRecv_RspCb,
                                                              pContext);
   }

   return status;
}

/*****************************************/
/*           ConnectionLess              */
/*****************************************/

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
* \retval NFCSTATUS_FAILED                   Operation failed.
*/
NFCSTATUS phFriNfc_LlcpTransport_SendTo( phFriNfc_LlcpTransport_Socket_t             *pLlcpSocket,
                                         uint8_t                                     nSap,
                                         phNfc_sData_t                               *psBuffer,
                                         pphFriNfc_LlcpTransportSocketSendCb_t       pSend_RspCb,
                                         void*                                       pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   phFriNfc_Llcp_sLinkParameters_t  LlcpRemoteLinkParamInfo;

   if(pLlcpSocket == NULL || psBuffer == NULL || pSend_RspCb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test the port number value */
   else if(nSap<2 || nSap>63)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionless socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionLess)
   {
       status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is in an updated state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      /* Get the local parameters of the LLCP Link */
      status = phFriNfc_Llcp_GetRemoteInfo(pLlcpSocket->psTransport->pLlcp,&LlcpRemoteLinkParamInfo);
      if(status != NFCSTATUS_SUCCESS)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_FAILED);
      }
      /* Test the length of the socket buffer for ConnectionLess mode*/
      else if(psBuffer->length > LlcpRemoteLinkParamInfo.miu)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
      }
      /* Test if the link is in error state */
      else if(pLlcpSocket->psTransport->LinkStatusError)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
      }
      else
      {
         status = phFriNfc_LlcpTransport_Connectionless_SendTo(pLlcpSocket,
                                                               nSap,
                                                               psBuffer,
                                                               pSend_RspCb,
                                                               pContext);
      }
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
NFCSTATUS phFriNfc_LlcpTransport_RecvFrom( phFriNfc_LlcpTransport_Socket_t                   *pLlcpSocket,
                                           phNfc_sData_t*                                    psBuffer,
                                           pphFriNfc_LlcpTransportSocketRecvFromCb_t         pRecv_Cb,
                                           void*                                             pContext)
{
   NFCSTATUS status = NFCSTATUS_SUCCESS;
   if(pLlcpSocket == NULL || psBuffer == NULL || pRecv_Cb == NULL || pContext == NULL)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is a connectionless socket */
   else if(pLlcpSocket->eSocket_Type != phFriNfc_LlcpTransport_eConnectionLess)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_PARAMETER);
   }
   /* Test if the socket is in an updated state */
   else if(pLlcpSocket->eSocket_State != phFriNfc_LlcpTransportSocket_eSocketBound)
   {
      status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_INVALID_STATE);
   }
   else
   {
      if(pLlcpSocket->bSocketRecvPending)
      {
         status = PHNFCSTVAL(CID_FRI_NFC_LLCP_TRANSPORT, NFCSTATUS_REJECTED);
      }
      else
      {
         status = phLibNfc_LlcpTransport_Connectionless_RecvFrom(pLlcpSocket,
                                                                 psBuffer,
                                                                 pRecv_Cb,
                                                                 pContext);
      }
   }

   return status;
}
