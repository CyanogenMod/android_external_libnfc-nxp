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
* \file  phHciNfc_CE_A.c                                             *
* \brief HCI card emulation A management routines.                              *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Aug 21 18:35:05 2009 $                                           *
* $Author: ing04880 $                                                         *
* $Revision: 1.14 $                                                           *
* $Aliases: NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $
*                                                                             *
* =========================================================================== *
*/

/*
***************************** Header File Inclusion ****************************
*/
#include <phNfcCompId.h>
#include <phNfcHalTypes.h>
#include <phHciNfc_Pipe.h>
#include <phHciNfc_Emulation.h>
#include <phOsalNfc.h>
#include <phLibNfc.h>
#include <phLibNfc_Internal.h>
#include <phHal4Nfc_Internal.h>
#include <phHciNfc_NfcIPMgmt.h>
#include <phHciNfc_Sequence.h>
/*
****************************** Macro Definitions *******************************
*/
#if defined (HOST_EMULATION)
#include <phHciNfc_CE.h>
#include <phHciNfc_CE_A.h>

#define CE_A_EVT_NFC_SEND_DATA               0x10U
#define CE_A_EVT_NFC_FIELD_ON                0x11U
#define CE_A_EVT_NFC_DEACTIVATED             0x12U
#define CE_A_EVT_NFC_ACTIVATED               0x13U
#define CE_A_EVT_NFC_FIELD_OFF               0x14U

/*
*************************** Structure and Enumeration ***************************
*/


/*
*************************** Static Function Declaration **************************
*/

static
NFCSTATUS
phHciNfc_Recv_CE_A_Event(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pEvent,
#ifdef ONE_BYTE_LEN
                             uint8_t            length
#else
                             uint16_t           length
#endif
                       );

static
NFCSTATUS
phHciNfc_Recv_CE_A_Response(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pResponse,
#ifdef ONE_BYTE_LEN
                             uint8_t            length
#else
                             uint16_t           length
#endif
                       );

/*Set CE config paramater response callback*/
static
 void phLibNfc_Mgt_SetCE_A_14443_4_ConfigParams_Cb(
                                void                             *context,
                                NFCSTATUS                        status
                                );


/* Response callback for Remote Device Receive.*/
static void phLibNfc_RemoteDev_CE_A_Receive_Cb(
                                    void            *context,
                                    phHal_sRemoteDevInformation_t *ConnectedDevice,
                                    phNfc_sData_t   *rec_rsp_data,
                                    NFCSTATUS       status
                                    );

/*Response callback for Remote Device Send.*/
static void phLibNfc_RemoteDev_CE_A_Send_Cb(
                            void        *Context,
                            NFCSTATUS   status
                            );

static NFCSTATUS
phHciNfc_CE_A_SendData (
                         phHciNfc_sContext_t    *psHciContext,
                         void                   *pHwRef,
                         phHciNfc_XchgInfo_t    *sData
                         );

/*
*************************** Function Definitions ***************************
*/
NFCSTATUS
phHciNfc_CE_A_Init_Resources(
                                phHciNfc_sContext_t     *psHciContext
                         )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phHciNfc_CE_A_Info_t     *ps_ce_a_info=NULL;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if(
            ( NULL == psHciContext->p_ce_a_info ) &&
             (phHciNfc_Allocate_Resource((void **)(&ps_ce_a_info),
            sizeof(phHciNfc_CE_A_Info_t))== NFCSTATUS_SUCCESS)
          )
        {
            psHciContext->p_ce_a_info = ps_ce_a_info;
            ps_ce_a_info->current_seq = HOST_CE_A_INVALID_SEQ;
            ps_ce_a_info->next_seq = HOST_CE_A_INVALID_SEQ;
            ps_ce_a_info->pipe_id = (uint8_t)HCI_UNKNOWN_PIPE_ID;
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }

    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_Initialise(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    /*
        1. Open Pipe,
        2. Set all parameters
    */
    NFCSTATUS           status = NFCSTATUS_SUCCESS;
    static uint8_t      sak = HOST_CE_A_SAK_DEFAULT;
    static uint8_t      atqa_info[] = { NXP_CE_A_ATQA_LOW,
                                        NXP_CE_A_ATQA_HIGH
                                        };

    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t        *ps_ce_a_info = ((phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_a_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_a_info->current_seq)
            {
                case HOST_CE_A_PIPE_OPEN:
                {
                    status = phHciNfc_Open_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_SAK_SEQ;
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case HOST_CE_A_SAK_SEQ:
                {
                    /* HOST Card Emulation A SAK Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_SAK_INDEX;
                    /* Configure the SAK of Host Card Emulation A */
                    sak = (uint8_t)HOST_CE_A_SAK_DEFAULT;
                    ps_pipe_info->param_info =(void*)&sak ;
                    ps_pipe_info->param_length = sizeof(sak) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ATQA_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_ATQA_SEQ:
                {
                    /* HOST Card Emulation A ATQA Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_ATQA_INDEX;
                    /* Configure the ATQA of Host Card Emulation A */
                    ps_pipe_info->param_info = (void*)atqa_info ;
                    ps_pipe_info->param_length = sizeof(atqa_info) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ENABLE_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_ENABLE_SEQ:
                {
                    status = phHciNfc_CE_A_Mode( psHciContext,
                        pHwRef,  HOST_CE_MODE_ENABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_DISABLE_SEQ;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                default :
                {
                  status=NFCSTATUS_FAILED;
                    break;
                }
            }
        }
    }

    return status;
}

NFCSTATUS
phHciNfc_CE_A_Release(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    /*
        1. Close pipe
    */
    NFCSTATUS       status = NFCSTATUS_SUCCESS;
    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t        *ps_ce_a_info = ((phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_a_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_NOT_ALLOWED);
        }
        else
        {
            switch(ps_ce_a_info->current_seq)
            {
                case HOST_CE_A_DISABLE_SEQ:
                {
                    status = phHciNfc_CE_A_Mode( psHciContext,
                        pHwRef,  HOST_CE_MODE_DISABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_CLOSE;
                    }
                    break;
                }
                case HOST_CE_A_PIPE_CLOSE:
                {
                    /* HOST Card Emulation A pipe close sequence */
                    status = phHciNfc_Close_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
//                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_DELETE;
//                        status = NFCSTATUS_PENDING;
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
                    }
                    break;
                }
                case HOST_CE_A_PIPE_DELETE:
                {
                    /* HOST Card Emulation A pipe delete sequence */
                    status = phHciNfc_Delete_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
#if 0
                        ps_ce_a_info->pipe_id = HCI_UNKNOWN_PIPE_ID;
                        psHciContext->p_pipe_list[ps_ce_a_info->pipe_id] = NULL;
                        phOsalNfc_FreeMemory((void *)ps_ce_a_info->p_pipe_info);
                        ps_ce_a_info->p_pipe_info = NULL;
#endif
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
                    }
                    break;
                }
                default :
                {
                    status=NFCSTATUS_FAILED;
                    break;
                }
            }
        }
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_Update_PipeInfo(
                                  phHciNfc_sContext_t     *psHciContext,
                                  uint8_t                 pipeID,
                                  phHciNfc_Pipe_Info_t    *pPipeInfo
                                  )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;

        ps_ce_a_info->current_seq = HOST_CE_A_PIPE_OPEN;
        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
        /* Update the pipe_id of the card emulation A Gate o
            btained from the HCI Response */
        ps_ce_a_info->pipe_id = pipeID;
        if (HCI_UNKNOWN_PIPE_ID != pipeID && pipeID>0)
        {
            ps_ce_a_info->p_pipe_info = pPipeInfo;
            if (NULL != pPipeInfo)
            {
                /* Update the Response Receive routine of the card
                    emulation A Gate */
                pPipeInfo->recv_resp = &phHciNfc_Recv_CE_A_Response;
                /* Update the event Receive routine of the card emulation A Gate */
                pPipeInfo->recv_event = &phHciNfc_Recv_CE_A_Event;
            }
        }
        else
        {
            ps_ce_a_info->p_pipe_info = NULL;
        }
    }

    return status;
}

NFCSTATUS
phHciNfc_CE_A_Get_PipeID(
                             phHciNfc_sContext_t        *psHciContext,
                             uint8_t                    *ppipe_id
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( (NULL != psHciContext)
        && ( NULL != ppipe_id )
        && ( NULL != psHciContext->p_ce_a_info )
        )
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info=NULL;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;
        *ppipe_id =  ps_ce_a_info->pipe_id  ;
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    return status;
}

static
NFCSTATUS
phHciNfc_Recv_CE_A_Response(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pResponse,
#ifdef ONE_BYTE_LEN
                             uint8_t            length
#else
                             uint16_t           length
#endif
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                (phHciNfc_sContext_t *)psContext ;
    if( (NULL == psHciContext) || (NULL == pHwRef) || (NULL == pResponse)
        || (length == 0))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        uint8_t                     prev_cmd = ANY_GET_PARAMETER;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                            psHciContext->p_ce_a_info ;
        if( NULL == ps_ce_a_info->p_pipe_info)
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_INVALID_HCI_SEQUENCE);
        }
        else
        {
            prev_cmd = ps_ce_a_info->p_pipe_info->prev_msg ;
            switch(prev_cmd)
            {
                case ANY_GET_PARAMETER:
                {
#if 0
                    status = phHciNfc_CE_A_InfoUpdate(psHciContext,
                                    ps_ce_a_info->p_pipe_info->reg_index,
                                    &pResponse[HCP_HEADER_LEN],
                                    (length - HCP_HEADER_LEN));
#endif /* #if 0 */
                    break;
                }
                case ANY_SET_PARAMETER:
                {
                    HCI_PRINT("CE A Parameter Set \n");
                    break;
                }
                case ANY_OPEN_PIPE:
                {
                    HCI_PRINT("CE A open pipe complete\n");
                    break;
                }
                case ANY_CLOSE_PIPE:
                {
                    HCI_PRINT("CE A close pipe complete\n");
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_RESPONSE);
                    break;
                }
            }
            if( NFCSTATUS_SUCCESS == status )
            {
                status = phHciNfc_CE_A_Update_Seq(psHciContext,
                                                    UPDATE_SEQ);
                ps_ce_a_info->p_pipe_info->prev_status = NFCSTATUS_SUCCESS;
            }
        }
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_Update_Seq(
                        phHciNfc_sContext_t     *psHciContext,
                        phHciNfc_eSeqType_t     seq_type
                    )
{
    phHciNfc_CE_A_Info_t    *ps_ce_a_info=NULL;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if( NULL == psHciContext->p_ce_a_info )
    {
        status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;
        switch(seq_type)
        {
            case RESET_SEQ:
            case INIT_SEQ:
            {
                ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
                ps_ce_a_info->current_seq = HOST_CE_A_PIPE_OPEN;
                break;
            }
            case UPDATE_SEQ:
            {
                ps_ce_a_info->current_seq = ps_ce_a_info->next_seq;
                break;
            }
            case INFO_SEQ:
            {
                break;
            }
            case REL_SEQ:
            {
                ps_ce_a_info->next_seq = HOST_CE_A_DISABLE_SEQ;
                ps_ce_a_info->current_seq = HOST_CE_A_DISABLE_SEQ;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return status;
}


NFCSTATUS
phHciNfc_CE_A_Mode(
                            void        *psHciHandle,
                            void        *pHwRef,
                            uint8_t     enable_type
                  )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    static uint8_t          param = 0 ;
    phHciNfc_sContext_t     *psHciContext = ((phHciNfc_sContext_t *)psHciHandle);

    if((NULL == psHciContext)||(NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_a_info->p_pipe_info;

        if (NULL != p_pipe_info)
        {
            p_pipe_info->reg_index = HOST_CE_A_MODE_INDEX;
            /* Enable/Disable Host Card Emulation A */
            param = (uint8_t)enable_type;
            p_pipe_info->param_info =(void*)&param ;
            p_pipe_info->param_length = sizeof(param) ;
            status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                    ps_ce_a_info->pipe_id,(uint8_t)ANY_SET_PARAMETER);
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_HCI_GATE_NOT_SUPPORTED);
        }
    }
    return status;
}



static
NFCSTATUS
phHciNfc_Recv_CE_A_Event(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pEvent,
#ifdef ONE_BYTE_LEN
                             uint8_t            length
#else
                             uint16_t           length
#endif
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                (phHciNfc_sContext_t *)psContext ;
    if( (NULL == psHciContext) || (NULL == pHwRef) || (NULL == pEvent)
        || (length == 0))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_NfcIP_Info_t       *p_nfcipinfo= (phHciNfc_NfcIP_Info_t *) psHciContext->p_nfcip_info ;
        p_nfcipinfo->rem_nfcip_tgt_info.RemDevType = phHal_eISO14443_A_PCD;
        phHciNfc_HCP_Packet_t       *p_packet = NULL;
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        phHciNfc_HCP_Message_t      *message = NULL;
        static phHal_sEventInfo_t   event_info;
        static phNfc_sTransactionInfo_t    trans_info;
        void * pInfo = &event_info;
        uint8_t evt = NFC_NOTIFY_EVENT;
        uint8_t                     instruction=0;

        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;

        /* Variable was set but never used (ARM warning) */
        PHNFC_UNUSED_VARIABLE(ps_ce_a_info);

        p_packet = (phHciNfc_HCP_Packet_t *)pEvent;
        message = &p_packet->msg.message;
        /* Get the instruction bits from the Message Header */
        instruction = (uint8_t) GET_BITS8( message->msg_header,
                                HCP_MSG_INSTRUCTION_OFFSET,
                                HCP_MSG_INSTRUCTION_LEN);
        psHciContext->host_rf_type = phHal_eISO14443_A_PICC;
        event_info.eventHost = phHal_eHostController;
        event_info.eventSource = phHal_eISO14443_A_PICC;
        event_info.eventInfo.pRemoteDevInfo = &(p_nfcipinfo->rem_nfcip_tgt_info);
        switch(instruction)
        {
            case CE_A_EVT_NFC_ACTIVATED:
            {
              psHciContext->hci_state.cur_state = hciState_Listen;
                psHciContext->hci_state.next_state = hciState_Unknown;
                event_info.eventType = NFC_EVT_ACTIVATED;
                /* Notify to the HCI Generic layer To Update the FSM */
                break;
            }
            case CE_A_EVT_NFC_DEACTIVATED:
            {
                event_info.eventType = NFC_EVT_DEACTIVATED;
                HCI_PRINT("CE A Target Deactivated\n");
                break;
            }
            case CE_A_EVT_NFC_SEND_DATA:
            {
                HCI_PRINT("CE A data is received from the PN544\n");

                if(length > (HCP_HEADER_LEN+1))
                {
                    event_info.eventType = NFC_EVT_APDU_RECEIVED;
                    trans_info.status = status;
                    trans_info.type = NFC_NOTIFY_CE_A_RECV_EVENT;
                    trans_info.length = length - HCP_HEADER_LEN - 1;
                    trans_info.buffer = &pEvent[HCP_HEADER_LEN];
                    trans_info.info = &event_info;
                    pInfo = &trans_info;
                    evt = NFC_NOTIFY_CE_A_RECV_EVENT;
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_INVALID_HCI_RESPONSE);
                }
                break;
            }
            case CE_A_EVT_NFC_FIELD_ON:
            {
                HCI_PRINT("CE A field on\n");
                event_info.eventType = NFC_EVT_FIELD_ON;
                break;
            }
            case CE_A_EVT_NFC_FIELD_OFF:
            {
                HCI_PRINT("CE A field off\n");
                event_info.eventType = NFC_EVT_FIELD_OFF;
                break;
            }
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_INVALID_HCI_INSTRUCTION);
                break;
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    evt,
                                    pInfo);
        }
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_SendData (
                         phHciNfc_sContext_t    *psHciContext,
                         void                   *pHwRef,
                         phHciNfc_XchgInfo_t    *sData
                         )
{
    NFCSTATUS       status = NFCSTATUS_SUCCESS;

    if( (NULL == psHciContext) || (NULL == pHwRef) ||
        (NULL == sData) || (0 == sData->tx_length) || (CE_MAX_SEND_BUFFER_LEN < sData->tx_length))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_a_info->p_pipe_info;

        if(NULL == p_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            if (NFCSTATUS_SUCCESS == status)
            {
                phHciNfc_HCP_Packet_t       *hcp_packet = NULL;
                phHciNfc_HCP_Message_t      *hcp_message = NULL;
                uint16_t                    length = HCP_HEADER_LEN;
                uint8_t                     pipeid = 0,
                                            i = 0;

                HCI_PRINT_BUFFER("HCI CE Send Data: ", sData->tx_buffer, sData->tx_length);

                psHciContext->tx_total = 0 ;
                pipeid = p_pipe_info->pipe.pipe_id;
                hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
                hcp_message = &(hcp_packet->msg.message);

                /* Construct the HCP Frame */
                phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                (uint8_t) pipeid, (uint8_t)HCP_MSG_TYPE_EVENT,
                                (uint8_t)CE_A_EVT_NFC_SEND_DATA);

                phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                                i, sData->tx_buffer,
                                sData->tx_length);

                length =(uint16_t)(length + i + sData->tx_length);

                p_pipe_info->sent_msg_type = (uint8_t)HCP_MSG_TYPE_EVENT;
                p_pipe_info->prev_msg = CE_A_EVT_NFC_SEND_DATA;
                psHciContext->tx_total = length;
                /* Send the Constructed HCP packet to the lower layer */
                status = phHciNfc_Send_HCP( psHciContext, pHwRef);
                p_pipe_info->prev_status = status;
            }
        }
    }
    return status;
}

/**Receive complete handler*/
void phHal4Nfc_CE_A_RecvCompleteHandler(phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,void *pInfo)
{
    pphHal4Nfc_TransceiveCallback_t pUpperRecvCb = NULL;
    NFCSTATUS RecvStatus = ((phNfc_sTransactionInfo_t *)pInfo)->status;
    /*allocate TrcvContext if not already allocated.Required since
     Receive complete can occur before any other send /receive calls.*/
    if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
    {
        Hal4Ctxt->psTrcvCtxtInfo= (pphHal4Nfc_TrcvCtxtInfo_t)
            phOsalNfc_GetMemory((uint32_t)
            (sizeof(phHal4Nfc_TrcvCtxtInfo_t)));
        if(NULL != Hal4Ctxt->psTrcvCtxtInfo)
        {
            (void)memset(Hal4Ctxt->psTrcvCtxtInfo,0,
                sizeof(phHal4Nfc_TrcvCtxtInfo_t));
            Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                = PH_OSALNFC_INVALID_TIMER_ID;
            Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus
                = NFCSTATUS_PENDING;
        }
    }
    if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
    {
        phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,0);
        RecvStatus = PHNFCSTVAL(CID_NFC_HAL ,
            NFCSTATUS_INSUFFICIENT_RESOURCES);
    }
    else
    {
        /*Allocate 4K buffer to copy the received data into*/
        if(NULL == Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer)
        {
            Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer
                = (uint8_t *)phOsalNfc_GetMemory(
                        PH_HAL4NFC_MAX_RECEIVE_BUFFER
                        );
            if(NULL == Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer)
            {
                phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,
                    0);
                RecvStatus = NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
            else /*memset*/
            {
                (void)memset(
                    Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer,
                    0,
                    PH_HAL4NFC_MAX_RECEIVE_BUFFER
                    );
            }
        }

        if(RecvStatus != NFCSTATUS_INSUFFICIENT_RESOURCES)
        {
            /*Copy the data*/
            (void)memcpy(
                (Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.buffer
                + Hal4Ctxt->psTrcvCtxtInfo->P2PRecvLength),
                ((phNfc_sTransactionInfo_t *)pInfo)->buffer,
                ((phNfc_sTransactionInfo_t *)pInfo)->length
                );
            /*Update P2PRecvLength,this also acts as the offset to append more
              received bytes*/
            Hal4Ctxt->psTrcvCtxtInfo->P2PRecvLength
                += ((phNfc_sTransactionInfo_t *)pInfo)->length;
            Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData.length
                = Hal4Ctxt->psTrcvCtxtInfo->P2PRecvLength;
        }

        if(RecvStatus != NFCSTATUS_MORE_INFORMATION)
        {
            Hal4Ctxt->psTrcvCtxtInfo->P2PRecvLength = 0;
            Hal4Ctxt->Hal4NextState = (eHal4StateTransaction
             == Hal4Ctxt->Hal4NextState?eHal4StateInvalid:Hal4Ctxt->Hal4NextState);
            if(NFCSTATUS_PENDING == Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus)
            {
                if(NULL != Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb)
                {
                    pUpperRecvCb = (pphHal4Nfc_TransceiveCallback_t)Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb;
                    Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb = NULL;
                    Hal4Ctxt->psTrcvCtxtInfo->psUpperRecvData
                        = &(Hal4Ctxt->psTrcvCtxtInfo->sLowerRecvData);
                    (*pUpperRecvCb)(
                        Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                        Hal4Ctxt->sTgtConnectInfo.psConnectedDevice,
                        Hal4Ctxt->psTrcvCtxtInfo->psUpperRecvData,
                        RecvStatus
                        );
                }
                else
                {
                    /*Receive data buffer is complete with data & P2P receive has
                      not yet been called*/
                    Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus
                        = NFCSTATUS_SUCCESS;
                }
            }
        }
    }
    return;
}

/**
* unblock any pending callback functions.
*/
NFCSTATUS
phLibNfc_Mgt_Unblock_Cb_CE_A_14443_4( )
{
  NFCSTATUS RetVal = NFCSTATUS_NOT_INITIALISED;
  if(gpphLibContext!=NULL && (gpphLibContext->psHwReference)!=NULL && (gpphLibContext->psHwReference->hal_context)!=NULL)
  {
    phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = NULL;
    Hal4Ctxt = gpphLibContext->psHwReference->hal_context;
    if(Hal4Ctxt->rem_dev_list[0]!=NULL &&
         Hal4Ctxt->rem_dev_list[0]->SessionOpened==TRUE &&
         Hal4Ctxt->rem_dev_list[0]->RemDevType == phNfc_eISO14443_A_PCD
       )
    {
      phHciNfc_sContext_t  *psHciContext = ((phHciNfc_sContext_t *)(Hal4Ctxt->psHciHandle));
      phHciNfc_NfcIP_Info_t       *p_nfcipinfo= (phHciNfc_NfcIP_Info_t *) psHciContext->p_nfcip_info ;
      p_nfcipinfo->rem_nfcip_tgt_info.RemDevType = phHal_eISO14443_A_PCD;
      phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
      static phHal_sEventInfo_t   event_info;
      void * pInfo = &event_info;

      ps_ce_a_info = (phHciNfc_CE_A_Info_t *)psHciContext->p_ce_a_info ;

      /* Variable was set but never used (ARM warning) */
      PHNFC_UNUSED_VARIABLE(ps_ce_a_info);

      psHciContext->host_rf_type = phHal_eISO14443_A_PICC;
      event_info.eventHost = phHal_eHostController;
      event_info.eventSource = phHal_eISO14443_A_PICC;
      event_info.eventInfo.pRemoteDevInfo = &(p_nfcipinfo->rem_nfcip_tgt_info);
      event_info.eventType = NFC_EVT_DEACTIVATED;
      phHciNfc_Notify_Event(psHciContext, gpphLibContext->psHwReference,
                                            NFC_NOTIFY_EVENT,
                                            pInfo);
      RetVal = NFCSTATUS_SUCCESS;
    }
  }
  return RetVal;
}


/**
* Interface to configure Card Emulation configurations.
*/
NFCSTATUS
phLibNfc_Mgt_SetCE_A_14443_4_ConfigParams(
                                uint8_t emulate,
                                pphLibNfc_RspCb_t     pConfigRspCb,
                                void*           pContext
                                )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    /* LibNfc Initialized or not */
    if((NULL == gpphLibContext)||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }/* Check for valid parameters */
    else if((NULL == pConfigRspCb)
        || (NULL == pContext))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(TRUE == gpphLibContext->status.GenCb_pending_status)
    { /*Previous callback is pending */
        RetVal = NFCSTATUS_BUSY;
    }
    else
    {
        if(eLibNfcHalStatePresenceChk !=
                gpphLibContext->LibNfcState.next_state)
        {
            phHal_uConfig_t uConfig;
            uConfig.emuConfig.emuType = NFC_HOST_CE_A_EMULATION;
            uConfig.emuConfig.config.hostEmuCfg_A.enableEmulation = emulate;
            if(emulate==FALSE)
            {
                //de-activate any pending commands
                phLibNfc_Mgt_Unblock_Cb_CE_A_14443_4();
            }
            RetVal = phHal4Nfc_ConfigParameters(
                                gpphLibContext->psHwReference,
                                NFC_EMULATION_CONFIG,
                                &uConfig,
                                phLibNfc_Mgt_SetCE_A_14443_4_ConfigParams_Cb,
                                (void *)gpphLibContext
                                );
        }
        else
        {
             gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCb= NULL;
             RetVal = NFCSTATUS_PENDING;
        }
        if(NFCSTATUS_PENDING == RetVal)
        {
            /* save the context and callback for later use */
            gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCb = pConfigRspCb;
            gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCntx = pContext;
            gpphLibContext->status.GenCb_pending_status=TRUE;
            /* Next state is configured */
            gpphLibContext->LibNfcState.next_state =eLibNfcHalStateConfigReady;
        }
        else
        {
            RetVal = NFCSTATUS_FAILED;
        }
    }
    return RetVal;
}
/**
* Response callback for CE configurations.
*/
static void phLibNfc_Mgt_SetCE_A_14443_4_ConfigParams_Cb(void     *context,
                                        NFCSTATUS status)
{
        pphLibNfc_RspCb_t       pClientCb=NULL;
    void                    *pUpperLayerContext=NULL;
     /* Check for the context returned by below layer */
    if((phLibNfc_LibContext_t *)context != gpphLibContext)
    {   /*wrong context returned*/
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if(eLibNfcHalStateShutdown == gpphLibContext->LibNfcState.next_state)
        {   /*shutdown called before completion of this api allow
            shutdown to happen */
            phLibNfc_Pending_Shutdown();
            status = NFCSTATUS_SHUTDOWN;
        }
        else
        {
            gpphLibContext->status.GenCb_pending_status = FALSE;
            if(NFCSTATUS_SUCCESS != status)
            {
                status = NFCSTATUS_FAILED;
            }
            else
            {
                status = NFCSTATUS_SUCCESS;
            }
        }
        /*update the current state */
        phLibNfc_UpdateCurState(status,gpphLibContext);

        pClientCb = gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCb;
        pUpperLayerContext = gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCntx;

        gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCb = NULL;
        gpphLibContext->sNfcIp_Context.pClientNfcIpCfgCntx = NULL;
        if (NULL != pClientCb)
        {
            /* Notify to upper layer status of configure operation */
            pClientCb(pUpperLayerContext, status);
        }
    }
    return;
}


/*  Transfer the user data to the another NfcIP device from the host.
 *  pTransferCallback is called, when all steps in the transfer sequence are
 *  completed.*/
NFCSTATUS
phHal4Nfc_CE_A_Receive(
                  phHal_sHwReference_t                  *psHwReference,
                  phHal4Nfc_TransactInfo_t              *psRecvInfo,
                  pphHal4Nfc_TransceiveCallback_t       pReceiveCallback,
                  void                                  *pContext
                 )
{
    NFCSTATUS RetStatus = NFCSTATUS_PENDING;
    phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = NULL;
     /*NULL checks*/
    if((NULL == psHwReference)
        ||( NULL == pReceiveCallback)
        ||( NULL == psRecvInfo))
    {
        phOsalNfc_RaiseException(phOsalNfc_e_PrecondFailed,1);
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_INVALID_PARAMETER);
    }
    /*Check initialised state*/
    else if((NULL == psHwReference->hal_context)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4CurrentState
                                               < eHal4StateOpenAndReady)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4NextState
                                               == eHal4StateClosed))
    {
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_NOT_INITIALISED);
    }
    else
    {
        Hal4Ctxt = (phHal4Nfc_Hal4Ctxt_t *)psHwReference->hal_context;
        if(NFC_EVT_ACTIVATED == Hal4Ctxt->sTgtConnectInfo.EmulationState)
        {
            /*Following condition gets satisfied only on target side,if receive
              is not already called*/
            if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
            {
                Hal4Ctxt->psTrcvCtxtInfo= (pphHal4Nfc_TrcvCtxtInfo_t)
                    phOsalNfc_GetMemory((uint32_t)
                    (sizeof(phHal4Nfc_TrcvCtxtInfo_t)));
                if(NULL != Hal4Ctxt->psTrcvCtxtInfo)
                {
                    (void)memset(Hal4Ctxt->psTrcvCtxtInfo,0,
                        sizeof(phHal4Nfc_TrcvCtxtInfo_t));
                    Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                        = PH_OSALNFC_INVALID_TIMER_ID;
                    Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus = NFCSTATUS_PENDING;
                }
            }
            if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
            {
                phOsalNfc_RaiseException(phOsalNfc_e_NoMemory,0);
                RetStatus= PHNFCSTVAL(CID_NFC_HAL ,
                    NFCSTATUS_INSUFFICIENT_RESOURCES);
            }
            else /*Store callback & Return status pending*/
            {
                Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt = pContext;
                /*Register upper layer callback*/
                Hal4Ctxt->psTrcvCtxtInfo->pUpperTranceiveCb = NULL;
                Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb = (pphHal4Nfc_ReceiveCallback_t)pReceiveCallback;
                if(NFCSTATUS_PENDING !=
                    Hal4Ctxt->psTrcvCtxtInfo->RecvDataBufferStatus)
                {
                    /**Create a timer to send received data in the callback*/
                    if(Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                        == PH_OSALNFC_INVALID_TIMER_ID)
                    {
                        PHDBG_INFO("HAL4: Transaction Timer Create for Receive");
                        Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                            = phOsalNfc_Timer_Create();
                    }
                    if(Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                        == PH_OSALNFC_INVALID_TIMER_ID)
                    {
                        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,
                            NFCSTATUS_INSUFFICIENT_RESOURCES);
                    }
                    else/*start the timer*/
                    {
                        phOsalNfc_Timer_Start(
                             Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId,
                             PH_HAL4NFC_CE_RECV_CB_TIMEOUT,
                             phHal4Nfc_CE_RecvTimerCb,
                             NULL
                            );
                    }
                }
            }
        }
        else/*deactivated*/
        {
            RetStatus= PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_DESELECTED);
        }
    }
    return RetStatus;
}

/**
* Interface used to transceive data from target to reader during CE communication
*/
NFCSTATUS
phLibNfc_RemoteDev_CE_A_Transceive(
                        phNfc_sData_t *      pTransferData,
                        pphLibNfc_TransceiveCallback_t    pTransceive_RspCb,
                        void                 *pContext
                        )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    /*Check Lib Nfc stack is initilized*/
    if((NULL == gpphLibContext)||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateRelease)
    {
        RetVal = NFCSTATUS_DESELECTED;
    }
    /*Check application has sent the valid parameters*/
    else if((NULL == pTransferData)
        || (NULL == pTransceive_RspCb)
        || (NULL == pTransferData->buffer)
        || (0 == pTransferData->length)
        || (NULL == pContext))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if((TRUE == gpphLibContext->status.GenCb_pending_status)
        ||(NULL!=gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb))
    {
        /*Previous callback is pending or local device is Initiator
        then don't allow */
        RetVal = NFCSTATUS_REJECTED;
    }
    else if((NULL!=gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb))
    {
        RetVal =NFCSTATUS_BUSY ;
    }
    else
    {
        if(eLibNfcHalStatePresenceChk ==
                gpphLibContext->LibNfcState.next_state)
        {
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb = NULL;
            RetVal = NFCSTATUS_PENDING;
        }
        else
        {
            if(gpphLibContext->psTransInfo!=NULL)
            {
                (void)memset(gpphLibContext->psTransInfo,
                                0,
                                sizeof(phLibNfc_sTransceiveInfo_t));

                gpphLibContext->psTransInfo->addr =UNKNOWN_BLOCK_ADDRESS;
                /*pointer to send data */
                gpphLibContext->psTransInfo->sSendData.buffer =
                                                    pTransferData->buffer;
                /*size of send data*/
                gpphLibContext->psTransInfo->sSendData.length =
                                                    pTransferData->length;

                /*Call Hal4 Send API and register callback with it*/
                PHDBG_INFO("LibNfc:CE send In Progress");
                RetVal= phHal4Nfc_CE_A_Transceive(
                                gpphLibContext->psHwReference,
                                &(gpphLibContext->sNfcIp_Context.TransactInfoRole),
                                gpphLibContext->psTransInfo->sSendData,
                                (pphHal4Nfc_TransceiveCallback_t) phLibNfc_RemoteDev_CE_A_Receive_Cb,
                                (void *)gpphLibContext
                                );
            }
        }
        if(NFCSTATUS_PENDING == RetVal)
        {
            /* Update next state to transaction */
            gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb= (pphLibNfc_Receive_RspCb_t) pTransceive_RspCb;
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb= NULL;
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCntx = pContext;
            gpphLibContext->sNfcIp_Context.pClientNfcIpRxCntx = pContext;
            gpphLibContext->status.GenCb_pending_status=TRUE;
            gpphLibContext->LibNfcState.next_state = eLibNfcHalStateTransaction;
        }
        else
        {
            RetVal = NFCSTATUS_FAILED;
        }
    }
    return RetVal;
}

/*  Transfer the user data to reader device from the host.
 *  pTransferCallback is called, when all steps in the transfer sequence are
 *  completed.*/
NFCSTATUS
phHal4Nfc_CE_A_Transceive(
                phHal_sHwReference_t                    *psHwReference,
                phHal4Nfc_TransactInfo_t                *psTransferInfo,
                phNfc_sData_t                            sTransferData,
                pphHal4Nfc_TransceiveCallback_t             pTransceiveCallback,
                void                                    *pContext
                )
{
    NFCSTATUS RetStatus = NFCSTATUS_PENDING;
    phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = NULL;
    /*NULL checks*/
    if((NULL == psHwReference)
        ||( NULL == pTransceiveCallback )
        || (NULL == psTransferInfo)
        )
    {
        phOsalNfc_RaiseException(phOsalNfc_e_PrecondFailed,1);
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_INVALID_PARAMETER);
    }
    /*Check initialised state*/
    else if((NULL == psHwReference->hal_context)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4CurrentState
                                               < eHal4StateOpenAndReady)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4NextState
                                               == eHal4StateClosed))
    {
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_NOT_INITIALISED);
    }
    else
    {
        Hal4Ctxt = (phHal4Nfc_Hal4Ctxt_t *)psHwReference->hal_context;
        if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
        {
            RetStatus= PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_FAILED);
        }
        /*Check Activated*/
        else if(NFC_EVT_ACTIVATED == Hal4Ctxt->sTgtConnectInfo.EmulationState)
        {
            Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt = pContext;
            /*Register upper layer callback*/
            Hal4Ctxt->psTrcvCtxtInfo->pUpperTranceiveCb = NULL;
            Hal4Ctxt->psTrcvCtxtInfo->pP2PRecvCb = (pphHal4Nfc_ReceiveCallback_t)pTransceiveCallback;
            PHDBG_INFO("NfcIP1 Send");
            /*allocate buffer to store senddata received from upper layer*/
            if (NULL == Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData)
            {
                Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData = (phNfc_sData_t *)
                        phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                if(NULL != Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData)
                {
                    (void)memset(Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData, 0,
                                                    sizeof(phNfc_sData_t));
                    Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                                            = PH_OSALNFC_INVALID_TIMER_ID;
                }
            }

            Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->buffer
                = sTransferData.buffer;
            Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->length
                = sTransferData.length;
            /*If data size is less than MAX_SEND_LEN ,no chaining is required*/
            Hal4Ctxt->psTrcvCtxtInfo->
                    XchangeInfo.params.nfc_info.more_info = FALSE;
            Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_length
                  = (uint8_t)sTransferData.length;
            Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_buffer
                  = sTransferData.buffer;

            PHDBG_INFO("HAL4:Calling Hci_Send_data()");

            RetStatus = phHciNfc_CE_A_SendData (
                    (phHciNfc_sContext_t *)Hal4Ctxt->psHciHandle,
                    psHwReference,
                    &(Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo)
                    );

            /*check return status*/
            if (NFCSTATUS_PENDING == RetStatus)
            {
                /*Set P2P_Send_In_Progress to defer any disconnect call until
                 Send complete occurs*/
                Hal4Ctxt->psTrcvCtxtInfo->P2P_Send_In_Progress = TRUE;
                Hal4Ctxt->Hal4NextState = eHal4StateTransaction;
                /*No of bytes remaining for next send*/
                Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->length
                    -= Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_length;
            }
        }
        else/*Deactivated*/
        {
            RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_DESELECTED);
        }
    }
    return RetStatus;
}

/**
* Interface used to receive data from initiator at target side during P2P
* communication.
*/
NFCSTATUS phLibNfc_RemoteDev_CE_A_Receive(
                                pphLibNfc_TransceiveCallback_t  pReceiveRspCb,
                                void                       *pContext
                                )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    /*Check Lib Nfc is initialized*/
    if((NULL == gpphLibContext)||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }/*Check application has sent valid parameters*/
    else if (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateRelease)
    {
        RetVal = NFCSTATUS_DESELECTED;
    }
    else if((NULL == pReceiveRspCb)
        || (NULL == pContext))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if((TRUE == gpphLibContext->status.GenCb_pending_status)
        ||(NULL!=gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb))
    {
        /*Previous callback is pending or if initiator uses this api */
        RetVal = NFCSTATUS_REJECTED;
    }
    else
    {
        if(eLibNfcHalStatePresenceChk ==
                gpphLibContext->LibNfcState.next_state)
        {
            gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb = NULL;
            RetVal = NFCSTATUS_PENDING;
        }
        else
        {
            /*Call below layer receive and register the callback with it*/
            PHDBG_INFO("LibNfc:CE Receive In Progress");
            RetVal =phHal4Nfc_CE_A_Receive(
                            gpphLibContext->psHwReference,
                            (phHal4Nfc_TransactInfo_t*)gpphLibContext->psTransInfo,
                            (pphHal4Nfc_TransceiveCallback_t)phLibNfc_RemoteDev_CE_A_Receive_Cb,
                            (void *)gpphLibContext
                            );
        }

        if(NFCSTATUS_PENDING == RetVal)
        {
            /*Update the Next state as Transaction*/
            gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb= (pphHal4Nfc_ReceiveCallback_t) pReceiveRspCb;
            gpphLibContext->sNfcIp_Context.pClientNfcIpRxCntx = pContext;
            gpphLibContext->status.GenCb_pending_status=TRUE;
            gpphLibContext->LibNfcState.next_state = eLibNfcHalStateTransaction;
        }
        else
        {
            RetVal = NFCSTATUS_FAILED;
        }
    }
    return RetVal;
}

/**
* Response callback for Remote Device Receive.
*/
static void phLibNfc_RemoteDev_CE_A_Receive_Cb(
                                    void            *context,
                                    phHal_sRemoteDevInformation_t *ConnectedDevice,
                                    phNfc_sData_t   *rec_rsp_data,
                                    NFCSTATUS       status
                                    )
{
    pphLibNfc_TransceiveCallback_t       pClientCb=NULL;

    phLibNfc_LibContext_t   *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)context;
    void                    *pUpperLayerContext=NULL;

    /* Check for the context returned by below layer */
    if(pLibNfc_Ctxt != gpphLibContext)
    {   /*wrong context returned*/
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        pClientCb = (pphLibNfc_TransceiveCallback_t) gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb;
        pUpperLayerContext = gpphLibContext->sNfcIp_Context.pClientNfcIpRxCntx;

        gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb = NULL;
        gpphLibContext->sNfcIp_Context.pClientNfcIpRxCntx = NULL;
        gpphLibContext->status.GenCb_pending_status = FALSE;
        if(eLibNfcHalStateShutdown == gpphLibContext->LibNfcState.next_state)
        {   /*shutdown called before completion of P2P receive allow
              shutdown to happen */
            phLibNfc_Pending_Shutdown();
            status = NFCSTATUS_SHUTDOWN;
        }
        else if(eLibNfcHalStateRelease == gpphLibContext->LibNfcState.next_state)
        {
            status = NFCSTATUS_ABORTED;
        }
        else
        {
            if((NFCSTATUS_SUCCESS != status) &&
                (PHNFCSTATUS(status) != NFCSTATUS_MORE_INFORMATION ) )
            {
                /*During CE receive operation initiator was removed
                from RF field of target*/
                status = NFCSTATUS_DESELECTED;
            }
            else
            {
                status = NFCSTATUS_SUCCESS;
            }
        }
        /* Update current state */
        phLibNfc_UpdateCurState(status,gpphLibContext);

        if (NULL != pClientCb)
        {
            /*Notify to upper layer status and No. of bytes
             actually received */
            pClientCb(pUpperLayerContext, (uint32_t)ConnectedDevice, rec_rsp_data, status);
        }
    }
    return;
}


/*  Transfer the user data to a reader device from the host.
 *  pTransferCallback is called, when all steps in the transfer sequence are
 *  completed.*/
NFCSTATUS
phHal4Nfc_CE_A_Send(
                phHal_sHwReference_t                    *psHwReference,
                phHal4Nfc_TransactInfo_t                *psTransferInfo,
                phNfc_sData_t                            sTransferData,
                pphHal4Nfc_SendCallback_t                pSendCallback,
                void                                    *pContext
                )
{
    NFCSTATUS RetStatus = NFCSTATUS_PENDING;
    phHal4Nfc_Hal4Ctxt_t *Hal4Ctxt = NULL;
    /*NULL checks*/
    if((NULL == psHwReference)
        ||( NULL == pSendCallback )
        || (NULL == psTransferInfo)
        )
    {
        phOsalNfc_RaiseException(phOsalNfc_e_PrecondFailed,1);
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_INVALID_PARAMETER);
    }
    /*Check initialised state*/
    else if((NULL == psHwReference->hal_context)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4CurrentState
                                               < eHal4StateOpenAndReady)
                        || (((phHal4Nfc_Hal4Ctxt_t *)
                                psHwReference->hal_context)->Hal4NextState
                                               == eHal4StateClosed))
    {
        RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_NOT_INITIALISED);
    }
    else
    {
        Hal4Ctxt = (phHal4Nfc_Hal4Ctxt_t *)psHwReference->hal_context;
        if(NULL == Hal4Ctxt->psTrcvCtxtInfo)
        {
            RetStatus= PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_FAILED);
        }
        /*Check Activated*/
        else if(NFC_EVT_ACTIVATED == Hal4Ctxt->sTgtConnectInfo.EmulationState)
        {
            Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt = pContext;
            /*Register upper layer callback*/
            Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb  = pSendCallback;
            PHDBG_INFO("NfcIP1 Send");
            /*allocate buffer to store senddata received from upper layer*/
            if (NULL == Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData)
            {
                Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData = (phNfc_sData_t *)
                        phOsalNfc_GetMemory(sizeof(phNfc_sData_t));
                if(NULL != Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData)
                {
                    (void)memset(Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData, 0,
                                                    sizeof(phNfc_sData_t));
                    Hal4Ctxt->psTrcvCtxtInfo->TransactionTimerId
                                            = PH_OSALNFC_INVALID_TIMER_ID;
                }
            }

            Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->buffer
                = sTransferData.buffer;
            Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->length
                = sTransferData.length;
            /*If data size is less than MAX_SEND_LEN ,no chaining is required*/
            Hal4Ctxt->psTrcvCtxtInfo->
                    XchangeInfo.params.nfc_info.more_info = FALSE;
            Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_length
                  = (uint8_t)sTransferData.length;
            Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_buffer
                  = sTransferData.buffer;

            PHDBG_INFO("HAL4:Calling Hci_Send_data()");

            RetStatus = phHciNfc_CE_A_SendData (
                    (phHciNfc_sContext_t *)Hal4Ctxt->psHciHandle,
                    psHwReference,
                    &(Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo)
                    );

            /*check return status*/
            if (NFCSTATUS_PENDING == RetStatus)
            {
                /*Set P2P_Send_In_Progress to defer any disconnect call until
                 Send complete occurs*/
                Hal4Ctxt->psTrcvCtxtInfo->P2P_Send_In_Progress = TRUE;
                Hal4Ctxt->Hal4NextState = eHal4StateTransaction;
                /*No of bytes remaining for next send*/
                Hal4Ctxt->psTrcvCtxtInfo->psUpperSendData->length
                    -= Hal4Ctxt->psTrcvCtxtInfo->XchangeInfo.tx_length;
            }
        }
        else/*Deactivated*/
        {
            RetStatus = PHNFCSTVAL(CID_NFC_HAL ,NFCSTATUS_DESELECTED);
        }
    }
    return RetStatus;
}

/**
* Interface used to send data from target to reader during CE communication
*/
NFCSTATUS
phLibNfc_RemoteDev_CE_A_Send(
                        phNfc_sData_t *      pTransferData,
                        pphLibNfc_RspCb_t    pSendRspCb,
                        void                 *pContext
                        )
{
    NFCSTATUS RetVal = NFCSTATUS_FAILED;
    /*Check Lib Nfc stack is initilized*/
    if((NULL == gpphLibContext)||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateRelease)
    {
        RetVal = NFCSTATUS_DESELECTED;
    }
    /*Check application has sent the valid parameters*/
    else if((NULL == pTransferData)
        || (NULL == pSendRspCb)
        || (NULL == pTransferData->buffer)
        || (0 == pTransferData->length)
        || (NULL == pContext))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if((TRUE == gpphLibContext->status.GenCb_pending_status)
        ||(NULL!=gpphLibContext->sNfcIp_Context.pClientNfcIpRxCb))
    {
        /*Previous callback is pending or local device is Initiator
        then don't allow */
        RetVal = NFCSTATUS_REJECTED;
    }/*Check for Discovered initiator handle and handle sent by application */
    else if((NULL!=gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb))
    {
        RetVal =NFCSTATUS_BUSY ;
    }
    else
    {
        if(eLibNfcHalStatePresenceChk ==
                gpphLibContext->LibNfcState.next_state)
        {
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb = NULL;
            RetVal = NFCSTATUS_PENDING;
        }
        else
        {
            if(gpphLibContext->psTransInfo!=NULL)
            {
                (void)memset(gpphLibContext->psTransInfo,
                                0,
                                sizeof(phLibNfc_sTransceiveInfo_t));

                gpphLibContext->psTransInfo->addr =UNKNOWN_BLOCK_ADDRESS;
                /*pointer to send data */
                gpphLibContext->psTransInfo->sSendData.buffer =
                                                    pTransferData->buffer;
                /*size of send data*/
                gpphLibContext->psTransInfo->sSendData.length =
                                                    pTransferData->length;

                /*Call Hal4 Send API and register callback with it*/
                PHDBG_INFO("LibNfc:CE send In Progress");
                RetVal= phHal4Nfc_CE_A_Send(
                                gpphLibContext->psHwReference,
                                &(gpphLibContext->sNfcIp_Context.TransactInfoRole),
                                gpphLibContext->psTransInfo->sSendData,
                                (pphLibNfc_RspCb_t)
                                phLibNfc_RemoteDev_CE_A_Send_Cb,
                                (void *)gpphLibContext
                                );
            }
        }
        if(NFCSTATUS_PENDING == RetVal)
        {
            /* Update next state to transaction */
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb= pSendRspCb;
            gpphLibContext->sNfcIp_Context.pClientNfcIpTxCntx = pContext;
            gpphLibContext->status.GenCb_pending_status=TRUE;
            gpphLibContext->LibNfcState.next_state = eLibNfcHalStateTransaction;
        }
        else
        {
            RetVal = NFCSTATUS_FAILED;
        }
    }
    return RetVal;
}

/*
* Response callback for Remote Device Send.
*/
static void phLibNfc_RemoteDev_CE_A_Send_Cb(
                            void        *Context,
                            NFCSTATUS   status
                            )
{
    pphLibNfc_RspCb_t       pClientCb=NULL;
    phLibNfc_LibContext_t   *pLibNfc_Ctxt = (phLibNfc_LibContext_t *)Context;
    void                    *pUpperLayerContext=NULL;

    /* Check for the context returned by below layer */
    if(pLibNfc_Ctxt != gpphLibContext)
    {   /*wrong context returned*/
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if(eLibNfcHalStateShutdown == gpphLibContext->LibNfcState.next_state)
        {   /*shutdown called before completion p2p send allow
              shutdown to happen */
            phLibNfc_Pending_Shutdown();
            status = NFCSTATUS_SHUTDOWN;
        }
        else if(eLibNfcHalStateRelease == gpphLibContext->LibNfcState.next_state)
        {
            status = NFCSTATUS_ABORTED;
        }
        else
        {
            gpphLibContext->status.GenCb_pending_status = FALSE;
            if((NFCSTATUS_SUCCESS != status) &&
                (PHNFCSTATUS(status) != NFCSTATUS_MORE_INFORMATION ) )
            {
                /*During p2p send operation initator was not present in RF
                field of target*/
                status = NFCSTATUS_DESELECTED;
            }
            else
            {
                status = NFCSTATUS_SUCCESS;
            }
        }
        /* Update current state */
        phLibNfc_UpdateCurState(status,gpphLibContext);

        pClientCb = gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb;
        pUpperLayerContext = gpphLibContext->sNfcIp_Context.pClientNfcIpTxCntx;

        gpphLibContext->sNfcIp_Context.pClientNfcIpTxCb = NULL;
        gpphLibContext->sNfcIp_Context.pClientNfcIpTxCntx = NULL;
        if (NULL != pClientCb)
        {
            /* Notify to upper layer status and No. of bytes
             actually written or send to initiator */
            pClientCb(pUpperLayerContext, status);
        }
    }
    return;
}

/**Send complete handler*/
void phHal4Nfc_CE_A_SendCompleteHandler(phHal4Nfc_Hal4Ctxt_t  *Hal4Ctxt,void *pInfo)
{
    pphHal4Nfc_SendCallback_t pUpperSendCb = NULL;
    NFCSTATUS SendStatus = ((phNfc_sCompletionInfo_t *)pInfo)->status;
    pphHal4Nfc_DiscntCallback_t pUpperDisconnectCb = NULL;
    Hal4Ctxt->psTrcvCtxtInfo->P2P_Send_In_Progress = FALSE;

    /*Send status Success or Pending disconnect in HAl4*/
    if((SendStatus != NFCSTATUS_SUCCESS)
        ||(NFC_INVALID_RELEASE_TYPE != Hal4Ctxt->sTgtConnectInfo.ReleaseType))
    {
        Hal4Ctxt->Hal4NextState = eHal4StateInvalid;
        /*Update Status*/
        SendStatus = (NFCSTATUS)(NFC_INVALID_RELEASE_TYPE !=
          Hal4Ctxt->sTgtConnectInfo.ReleaseType?NFCSTATUS_RELEASED:SendStatus);
        /*Callback For Target Send*/
        if(NULL != Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb)
        {
            pUpperSendCb = Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb;
            Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb = NULL;
            (*pUpperSendCb)(
                Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                SendStatus
                );
        }
        /*Issue Pending disconnect from HAl4*/
        if(NFC_INVALID_RELEASE_TYPE != Hal4Ctxt->sTgtConnectInfo.ReleaseType)
        {
            SendStatus = phHal4Nfc_Disconnect_Execute(gpphHal4Nfc_Hwref);
            if((NFCSTATUS_PENDING != SendStatus) &&
               (NULL != Hal4Ctxt->sTgtConnectInfo.pUpperDisconnectCb))
            {
                pUpperDisconnectCb =
                    Hal4Ctxt->sTgtConnectInfo.pUpperDisconnectCb;
                Hal4Ctxt->sTgtConnectInfo.pUpperDisconnectCb = NULL;
                (*pUpperDisconnectCb)(
                    Hal4Ctxt->sUpperLayerInfo.psUpperLayerDisconnectCtxt,
                    Hal4Ctxt->sTgtConnectInfo.psConnectedDevice,
                    SendStatus
                    );/*Notify disconnect failed to upper layer*/
            }
        }
    }
    else
    {
        Hal4Ctxt->psTrcvCtxtInfo->NumberOfBytesSent = 0;
        /*Callback For Target Send*/
        if(NULL != Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb)
        {
            pUpperSendCb = Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb;
            Hal4Ctxt->psTrcvCtxtInfo->pP2PSendCb = NULL;
            (*pUpperSendCb)(
                    Hal4Ctxt->sUpperLayerInfo.psUpperLayerCtxt,
                     SendStatus
                    );
        }
    }
    return;
}

#endif /* #if defined (HOST_EMULATION) */


