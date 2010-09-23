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

/*
 * \file  phFriNfc_MifULFormat.c
 * \brief NFC Ndef Formatting For Mifare ultralight card.
 *
 * Project: NFC-FRI
 *
 * $Date: Fri Oct 23 12:00:05 2009 $
 * $Author: ing02260 $
 * $Revision: 1.8 $
 * $Aliases: NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $
 *
 */

#include <phFriNfc_MifULFormat.h>
#include <phFriNfc_OvrHal.h>

/*! \ingroup grp_file_attributes
 *  \name NDEF Mapping
 *
 * File: \ref phFriNfc_MifULFormat.c
 *
 */
/*@{*/
#define PHFRINFCMIFULFORMAT_FILEREVISION "$Revision: 1.8 $"
#define PHFRINFCMIFULFORMAT_FILEALIASES  "$Aliases: NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $"
/*@}*/
/*!
* \brief \copydoc page_ovr Helper function for Mifare UL. This function calls the   
* transceive function
*/
static NFCSTATUS phFriNfc_MfUL_H_Transceive(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/*!
* \brief \copydoc page_ovr Helper function for Mifare UL. This function calls the   
* read or write operation
*/
static NFCSTATUS phFriNfc_MfUL_H_WrRd(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt);

/*!
* \brief \copydoc page_ovr Helper function for Mifare UL. This function fills the  
* send buffer for transceive function
*/
static void phFriNfc_MfUL_H_fillSendBuf(phFriNfc_sNdefSmtCrdFmt_t   *NdefSmtCrdFmt,
                                        uint8_t                     BlockNo);

/*!
* \brief \copydoc page_ovr Helper function for Mifare UL. This function shall process
* the read bytes
*/
static NFCSTATUS phFriNfc_MfUL_H_ProRd16Bytes(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt);

/*!
* \brief \copydoc page_ovr Helper function for Mifare UL. This function shall process the
* OTP bytes written
*/
static NFCSTATUS phFriNfc_MfUL_H_ProWrOTPBytes(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

static int MemCompare1 ( void *s1, void *s2, unsigned int n );
/*The function does a comparision of two strings and returns a non zero value 
if two strings are unequal*/
static int MemCompare1 ( void *s1, void *s2, unsigned int n )
{
    int8_t   diff = 0;
    int8_t *char_1  =(int8_t *)s1;
    int8_t *char_2  =(int8_t *)s2;
    if(NULL == s1 || NULL == s2)
    {
        PHDBG_CRITICAL_ERROR("NULL pointer passed to memcompare");
    }
    else
    {
        for(;((n>0)&&(diff==0));n--,char_1++,char_2++)
        {
            diff = *char_1 - *char_2;
        }
    }
    return (int)diff;
}

void phFriNfc_MfUL_Reset(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    uint8_t OTPByte[] = PH_FRINFC_MFUL_FMT_OTP_BYTES; 

    NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = PH_FRINFC_MFUL_FMT_VAL_0;
    (void)memcpy(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                OTPByte,
                sizeof(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes));
}

NFCSTATUS phFriNfc_MfUL_Format(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    NFCSTATUS               Result = NFCSTATUS_SUCCESS;
    uint8_t                 OTPByte[] = PH_FRINFC_MFUL_FMT_OTP_BYTES;
    
    NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = PH_FRINFC_MFUL_FMT_VAL_0;
    (void)memcpy(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                OTPByte,
                sizeof(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes));

    /* Set the state */
    NdefSmtCrdFmt->State = PH_FRINFC_MFUL_FMT_RD_16BYTES;
    /* Initialise current block to the lock bits block */
    NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = PH_FRINFC_MFUL_FMT_VAL_2;
    
    /* Start authentication */
    Result = phFriNfc_MfUL_H_WrRd(NdefSmtCrdFmt);
    return Result;
}

void phFriNfc_MfUL_Process(void             *Context,
                           NFCSTATUS        Status)
{
    phFriNfc_sNdefSmtCrdFmt_t  *NdefSmtCrdFmt = (phFriNfc_sNdefSmtCrdFmt_t *)Context;
    
    if(Status == NFCSTATUS_SUCCESS)
    {
        switch(NdefSmtCrdFmt->State)
        {
        case PH_FRINFC_MFUL_FMT_RD_16BYTES:
            Status = phFriNfc_MfUL_H_ProRd16Bytes(NdefSmtCrdFmt);
            break;

        case PH_FRINFC_MFUL_FMT_WR_OTPBYTES:
            Status = phFriNfc_MfUL_H_ProWrOTPBytes(NdefSmtCrdFmt);
            break;

        case PH_FRINFC_MFUL_FMT_WR_TLV:
#ifdef PH_NDEF_MIFARE_ULC               
            if (NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_ULC_CARD)
            {
                /* Write NDEF TLV in block number 5 */
                NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = 
                                                        PH_FRINFC_MFUL_FMT_VAL_5;
                /* Card already have the OTP bytes so write TLV */
                NdefSmtCrdFmt->State = PH_FRINFC_MFUL_FMT_WR_TLV1;
                
                Status = phFriNfc_MfUL_H_WrRd(NdefSmtCrdFmt);
            }
#endif /* #ifdef PH_NDEF_MIFARE_ULC */

            break;

#ifdef PH_NDEF_MIFARE_ULC   
        case PH_FRINFC_MFUL_FMT_WR_TLV1:
        
        break;
#endif /* #ifdef PH_NDEF_MIFARE_ULC */
        
        default:
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                NFCSTATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }
    /* Status is not success then call completion routine */
    if(Status != NFCSTATUS_PENDING)
    {
        phFriNfc_SmtCrdFmt_HCrHandler(NdefSmtCrdFmt, Status);
    }
}

static NFCSTATUS phFriNfc_MfUL_H_WrRd( phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt )
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;

    /* Fill the send buffer */
    phFriNfc_MfUL_H_fillSendBuf(NdefSmtCrdFmt, 
                            NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock);

    /* Call transceive */
    Result = phFriNfc_MfUL_H_Transceive(NdefSmtCrdFmt);
    
    return Result;
}

static NFCSTATUS phFriNfc_MfUL_H_Transceive(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;

    /* set the data for additional data exchange*/
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.NADPresent = 0;
    NdefSmtCrdFmt->psDepAdditionalInfo.NAD = 0;

    /*set the completion routines for the card operations*/
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.CompletionRoutine = phFriNfc_NdefSmtCrd_Process;
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.Context = NdefSmtCrdFmt;

    *NdefSmtCrdFmt->SendRecvLength = PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE;

    /* Call the Overlapped HAL Transceive function */ 
    Result = phFriNfc_OvrHal_Transceive(    NdefSmtCrdFmt->LowerDevice,
                                            &NdefSmtCrdFmt->SmtCrdFmtCompletionInfo,
                                            NdefSmtCrdFmt->psRemoteDevInfo,
                                            NdefSmtCrdFmt->Cmd,
                                            &NdefSmtCrdFmt->psDepAdditionalInfo,
                                            NdefSmtCrdFmt->SendRecvBuf,
                                            NdefSmtCrdFmt->SendLength,
                                            NdefSmtCrdFmt->SendRecvBuf,
                                            NdefSmtCrdFmt->SendRecvLength);
    return Result;
}

static void phFriNfc_MfUL_H_fillSendBuf( phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt,
                                 uint8_t                    BlockNo)
{
#ifdef PH_NDEF_MIFARE_ULC
    uint8_t     NDEFTLV1[4] = {0x01, 0x03, 0xA0, 0x10}; 
    uint8_t     NDEFTLV2[4] = {0x44, 0x03, 0x00, 0xFE}; 
#endif /* #ifdef PH_NDEF_MIFARE_ULC */
    uint8_t     NDEFTLV[4] = {0x03, 0x00, 0xFE, 0x00};


    
    
    /* First byte for send buffer is always the block number */
    NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_0] = (uint8_t)BlockNo;
    switch(NdefSmtCrdFmt->State)
    {
    case PH_FRINFC_MFUL_FMT_RD_16BYTES:
#ifdef PH_HAL4_ENABLE
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareRead;
#else
        /* Read command */
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareCmdListMifareRead;
#endif /* #ifdef PH_HAL4_ENABLE */
        /* Send length for read command is always one */
        NdefSmtCrdFmt->SendLength = PH_FRINFC_MFUL_FMT_VAL_1;
        break;

    case PH_FRINFC_MFUL_FMT_WR_OTPBYTES:
        /* Send length for read command is always Five */
        NdefSmtCrdFmt->SendLength = PH_FRINFC_MFUL_FMT_VAL_5;
        /* Write command */
#ifdef PH_HAL4_ENABLE
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareWrite4;
#else
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareCmdListMifareWrite4;
#endif /* #ifdef PH_HAL4_ENABLE */
        /* Copy the OTP bytes */
        (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_1], 
                    NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                    PH_FRINFC_MFUL_FMT_VAL_4);
        break;

    case PH_FRINFC_MFUL_FMT_WR_TLV:
#ifndef PH_NDEF_MIFARE_ULC      
    default:
#endif /* #ifndef PH_NDEF_MIFARE_ULC */    
        /* Send length for read command is always Five */
        NdefSmtCrdFmt->SendLength = PH_FRINFC_MFUL_FMT_VAL_5;
        /* Write command */
#ifdef PH_HAL4_ENABLE
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareWrite4;
#else
        NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareCmdListMifareWrite4;
#endif /* #ifdef PH_HAL4_ENABLE */        
        /* Copy the NDEF TLV */
#ifdef PH_NDEF_MIFARE_ULC

        if (NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_ULC_CARD)
        {
            (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_1], 
                    NDEFTLV1,
                    PH_FRINFC_MFUL_FMT_VAL_4);
        }
        else if (NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_UL_CARD)
        {
            (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_1], 
                NDEFTLV,
                PH_FRINFC_MFUL_FMT_VAL_4);
        }
        else
        {
        }
#else
        (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_1], 
                    NDEFTLV,
                    PH_FRINFC_MFUL_FMT_VAL_4);

#endif /* #ifdef PH_NDEF_MIFARE_ULC */

        break;

#ifdef PH_NDEF_MIFARE_ULC
    case PH_FRINFC_MFUL_FMT_WR_TLV1:
        if (NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_ULC_CARD)
        {
            /* Send length for write command is always Five */
            NdefSmtCrdFmt->SendLength = PH_FRINFC_MFUL_FMT_VAL_5;
            /* Write command */
            NdefSmtCrdFmt->Cmd.MfCmd = phHal_eMifareWrite4;
            (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_1], 
                        NDEFTLV2,
                        PH_FRINFC_MFUL_FMT_VAL_4);
        }
        break;
    default:
        break;
#endif /* #ifdef PH_NDEF_MIFARE_ULC */

    
    }
}

static NFCSTATUS phFriNfc_MfUL_H_ProRd16Bytes( phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt )
{
    NFCSTATUS   Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                                    NFCSTATUS_FORMAT_ERROR);
    uint32_t    memcompare = PH_FRINFC_MFUL_FMT_VAL_0;
    uint8_t     ZeroBuf[] = {0x00, 0x00, 0x00, 0x00};

#ifdef PH_NDEF_MIFARE_ULC
    uint8_t                 OTPByteUL[] = PH_FRINFC_MFUL_FMT_OTP_BYTES;
    uint8_t                 OTPByteULC[] = PH_FRINFC_MFULC_FMT_OTP_BYTES;
#endif /* #ifdef PH_NDEF_MIFARE_ULC */

    /* Check the lock bits (byte number 2 and 3 of block number 2) */
    if ((NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_2] == 
        PH_FRINFC_MFUL_FMT_LOCK_BITS_VAL) && 
        (NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_3] == 
        PH_FRINFC_MFUL_FMT_LOCK_BITS_VAL))
    {

#ifdef PH_NDEF_MIFARE_ULC

        if (NdefSmtCrdFmt->SendRecvBuf[8] == 0x02 && 
            NdefSmtCrdFmt->SendRecvBuf[9] == 0x00) 
        {
            NdefSmtCrdFmt->CardType = PH_FRINFC_NDEFMAP_MIFARE_ULC_CARD;
            
            (void)memcpy(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                        OTPByteULC,
                        sizeof(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes));
        }
        else if (NdefSmtCrdFmt->SendRecvBuf[8] == 0xFF && 
                NdefSmtCrdFmt->SendRecvBuf[9] == 0xFF)
        {
            NdefSmtCrdFmt->CardType = PH_FRINFC_NDEFMAP_MIFARE_UL_CARD;
            
            (void)memcpy(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                        OTPByteUL,
                        sizeof(NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes));
        }
        else
        {
            NdefSmtCrdFmt->CardType = PH_FRINFC_NDEFMAP_MIFARE_UL_CARD;
        }

#endif /* #ifdef PH_NDEF_MIFARE_ULC */

        memcompare = (uint32_t) 
                    MemCompare1(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_4],
                            NdefSmtCrdFmt->AddInfo.Type2Info.OTPBytes,
                            PH_FRINFC_MFUL_FMT_VAL_4);

        if (memcompare == PH_FRINFC_MFUL_FMT_VAL_0)
        {
            /* Write NDEF TLV in block number 4 */
            NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = 
                                                PH_FRINFC_MFUL_FMT_VAL_4;
            /* Card already have the OTP bytes so write TLV */
            NdefSmtCrdFmt->State = PH_FRINFC_MFUL_FMT_WR_TLV;
        }
        else
        {
            /* IS the card new, OTP bytes = {0x00, 0x00, 0x00, 0x00} */
            memcompare = (uint32_t)MemCompare1(&NdefSmtCrdFmt->SendRecvBuf[PH_FRINFC_MFUL_FMT_VAL_4],
                                ZeroBuf,
                                PH_FRINFC_MFUL_FMT_VAL_4);
            /* If OTP bytes are Zero then the card is Zero */
            if (memcompare == PH_FRINFC_MFUL_FMT_VAL_0)
            {
                /* Write OTP bytes in block number 3 */
                NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = 
                                                PH_FRINFC_MFUL_FMT_VAL_3;
                /* Card already have the OTP bytes so write TLV */
                NdefSmtCrdFmt->State = PH_FRINFC_MFUL_FMT_WR_OTPBYTES;
            }
        }
    }

    

#ifdef PH_NDEF_MIFARE_ULC
    if(
        ((NdefSmtCrdFmt->State == PH_FRINFC_MFUL_FMT_WR_TLV) || 
        (NdefSmtCrdFmt->State == PH_FRINFC_MFUL_FMT_WR_OTPBYTES)) &&
        ((NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_ULC_CARD) ||
        (NdefSmtCrdFmt->CardType == PH_FRINFC_NDEFMAP_MIFARE_UL_CARD))
        )
#else
    if((NdefSmtCrdFmt->State == PH_FRINFC_MFUL_FMT_WR_TLV) || 
        (NdefSmtCrdFmt->State == PH_FRINFC_MFUL_FMT_WR_OTPBYTES))
#endif /* #ifdef PH_NDEF_MIFARE_ULC */        
    {
        Result = phFriNfc_MfUL_H_WrRd(NdefSmtCrdFmt);
    }
    return Result;
}

static NFCSTATUS phFriNfc_MfUL_H_ProWrOTPBytes( phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt )
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    /* Card already have the OTP bytes so write TLV */
    NdefSmtCrdFmt->State = PH_FRINFC_MFUL_FMT_WR_TLV;

    /* Write NDEF TLV in block number 4 */
    NdefSmtCrdFmt->AddInfo.Type2Info.CurrentBlock = 
                                PH_FRINFC_MFUL_FMT_VAL_4;

    Result = phFriNfc_MfUL_H_WrRd(NdefSmtCrdFmt);
    return Result;
}

