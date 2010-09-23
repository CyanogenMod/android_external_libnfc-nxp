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
* \file  phFriNfc_DesfireFormat.c
* \brief This component encapsulates different format functinalities ,
*        for the Type4/Desfire card. 
*
* Project: NFC-FRI
*
* $Date: Fri Feb 20 14:38:17 2009 $
* $Author: ing07385 $
* $Revision: 1.4 $
* $Aliases: NFC_FRI1.1_WK908_R19_1,NFC_FRI1.1_WK910_PREP1,NFC_FRI1.1_WK910_R20_1,NFC_FRI1.1_WK912_PREP1,NFC_FRI1.1_WK912_R21_1,NFC_FRI1.1_WK914_PREP1,NFC_FRI1.1_WK914_R22_1,NFC_FRI1.1_WK914_R22_2,NFC_FRI1.1_WK916_R23_1,NFC_FRI1.1_WK918_R24_1,NFC_FRI1.1_WK920_PREP1,NFC_FRI1.1_WK920_R25_1,NFC_FRI1.1_WK922_PREP1,NFC_FRI1.1_WK922_R26_1,NFC_FRI1.1_WK924_PREP1,NFC_FRI1.1_WK924_R27_1,NFC_FRI1.1_WK926_R28_1,NFC_FRI1.1_WK928_R29_1,NFC_FRI1.1_WK930_R30_1,NFC_FRI1.1_WK934_PREP_1,NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $
*
*/
#include <phNfcTypes.h>
#include <phFriNfc_OvrHal.h>
#include <phFriNfc_SmtCrdFmt.h>
#include <phFriNfc_DesfireFormat.h>


/* Following section details, how to wrap the native DESFire commands in to ISO commands
Following are different native commands are wrapped under the ISO commands :
1. Crate Application
2. Select File
3. Get version
4. Create CC/NDEF File
5. Write data to STD File
In this File above commands are sent using the ISO Wrapper.

Wrapping the native DESFire APDU's procedure
--------------------------------------------------------------------------------
CLA         INS     P1      P2      Lc          Data            Le
0x90        Cmd     0x00    0x00    Data Len    Cmd. Par's      0x00
-----------------------------------------------------------------------------------*/        

/* Helper functions to create app/select app/create data file/read /write from the
CC file and NDEF files */
static void phFriNfc_Desf_HWrapISONativeCmds(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt,uint8_t CmdType);

/* Gets H/W version details*/
static NFCSTATUS phFriNfc_Desf_HGetHWVersion(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Gets S/W version details*/
static NFCSTATUS phFriNfc_Desf_HGetSWVersion(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt);

/* Updates the version details to context structure*/
static NFCSTATUS phFriNfc_Desf_HUpdateVersionDetails(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt);

/*Gets UID details*/
static NFCSTATUS phFriNfc_Desf_HGetUIDDetails(phFriNfc_sNdefSmtCrdFmt_t * NdefSmtCrdFmt);

/*Creates Application*/
static NFCSTATUS phFriNfc_Desf_HCreateApp(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Selects Application*/
static NFCSTATUS phFriNfc_Desf_HSelectApp(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/*Creates Capability Container File*/
static NFCSTATUS phFriNfc_Desf_HCreatCCFile(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Create NDEF File*/
static NFCSTATUS phFriNfc_Desf_HCreatNDEFFile(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Writes CC Bytes to CC File*/
static NFCSTATUS phFriNfc_Desf_HWrCCBytes(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Writes NDEF data into NDEF File*/
static NFCSTATUS phFriNfc_Desf_HWrNDEFData(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);

/* Transceive Cmd initiation*/
static NFCSTATUS phFriNfc_Desf_HSendTransCmd(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt);


void phFriNfc_Desfire_Reset( phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    /* This piece of code may not be useful for the current supported DESFire formatting feature*/
    /* Currently, s/w doesn't support authenticating either PICC Master key nor NFC Forum
    Application Master key*/

    /*NdefSmtCrdFmt->AddInfo.Type4Info.PICCMasterKey[] = PH_FRINFC_SMTCRDFMT_DESF_PICC_MASTER_KEY;
    NdefSmtCrdFmt->AddInfo.Type4Info.NFCForumMasterkey[]  = PH_FRINFC_SMTCRDFMT_DESF_NFCFORUM_APP_KEY;*/

    /* reset to zero PICC and NFC FORUM master keys*/
    (void)memset((void *)NdefSmtCrdFmt->AddInfo.Type4Info.PICCMasterKey, 
        0x00, 
        16);

    (void)memset((void *)NdefSmtCrdFmt->AddInfo.Type4Info.NFCForumMasterkey,
        0x00, 
        16);
    NdefSmtCrdFmt->AddInfo.Type4Info.PrevState = 0;

}


static void phFriNfc_Desf_HWrapISONativeCmds(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt,uint8_t CmdType)
{

    uint16_t i=0, CmdByte=1;
    uint8_t NdefFileBytes[] = PH_FRINFC_DESF_NDEFFILE_BYTES;
    uint8_t CCFileBytes[]  = PH_FRINFC_DESF_CCFILE_BYTES;
    

    /* common elements for all the native commands*/

    /* Class Byte */
    NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_CLASS_BYTE;

    /* let the place to store the cmd byte type, point to next index*/
    i += 2;

    
    /* P1/P2 offsets always set to zero */
    NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_OFFSET_P1;
    i++;
    NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_OFFSET_P2;
    i++;

    switch(CmdType)
    {

    case PH_FRINFC_DESF_GET_HW_VERSION_CMD :
    case PH_FRINFC_DESF_GET_SW_VERSION_CMD :
    case PH_FRINFC_DESF_GET_UID_CMD :

        if (CmdType == PH_FRINFC_DESF_GET_HW_VERSION_CMD )
        {
            /* Instruction Cmd code */
            NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_GET_VER_CMD;
        }
        else
        {
            /* Instruction Cmd code */
            NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_PICC_ADDI_FRAME_RESP;
        }
        
        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /* NO Data to send in this cmd*/
        /* we are not suppose to set Le*/
        /* set the length of the buffer*/
        NdefSmtCrdFmt->SendLength = i;

        break;

    case PH_FRINFC_DESF_CREATEAPP_CMD:

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_CREATE_AID_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_CRAPP_WRDT_LEN;
        i++;

        /* NFC FORUM APPLICATION ID*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_FIRST_AID_BYTE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_SEC_AID_BYTE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_THIRD_AID_BYTE;
        i++;

        /* set key settings and number of keys*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_PICC_NFC_KEY_SETTING;
        i++;

        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NFCFORUM_APP_NO_OF_KEYS;
        i++;

        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        /* set the length of the buffer*/
        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_CREATEAPP_CMD_SNLEN;

        break;

    case PH_FRINFC_DESF_SELECTAPP_CMD :

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_SLECT_APP_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_SLAPP_WRDT_LEN;
        i++;

        /* Data*/
        /* set the send buffer to create the application identifier*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_FIRST_AID_BYTE;
        i++;

        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_SEC_AID_BYTE;
        i++;

        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_THIRD_AID_BYTE;
        i++;

        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        /* set the length of the buffer*/
        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_SELECTAPP_CMD_SNLEN;
    
        break;

    case PH_FRINFC_DESF_CREATECC_CMD :

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_CREATE_DATA_FILE_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_CRCCNDEF_WRDT_LEN;
        i++;

        /* set cc file id*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_CC_FILE_ID;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_COMM_SETTINGS;
        i++;

        /* set the Access Rights are set to full read/write, full cntrl*/
        NdefSmtCrdFmt->SendRecvBuf[i] = 0xEE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0xEE;
        i++;

        /* set the file size*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_CC_FILE_SIZE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        /* set the length of the buffer*/
        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_CREATECCNDEF_CMD_SNLEN ;
        break;

    case PH_FRINFC_DESF_CREATENDEF_CMD :

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_CREATE_DATA_FILE_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_CRCCNDEF_WRDT_LEN;
        i++;

        /* set cc file id*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NDEF_FILE_ID;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_COMM_SETTINGS;
        i++;

        /* set the r/w access rights.Once Authentication part is fixed,
           we will use the constants*/
        NdefSmtCrdFmt->SendRecvBuf[i] = 0xEE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0xEE;
        i++;

        NdefSmtCrdFmt->SendRecvBuf[i]= (uint8_t)NdefSmtCrdFmt->AddInfo.Type4Info.CardSize ;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i]= (uint8_t)(NdefSmtCrdFmt->AddInfo.Type4Info.CardSize >> 8) ;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i]= (uint8_t)(NdefSmtCrdFmt->AddInfo.Type4Info.CardSize >> 16); 
        i++;
        
        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        /* set the length of the buffer*/
        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_CREATECCNDEF_CMD_SNLEN ;

        break;

    case  PH_FRINFC_DESF_WRITECC_CMD:

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_WRITE_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_WRCC_WRDT_LEN;
        i++;

        /* set the file id*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_CC_FILE_ID;
        i++;

        /* set the offset to zero*/
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /* Set the length of data available to write*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_CC_FILE_SIZE;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /*set the data to be written to the CC file*/
        (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[i],
            CCFileBytes,
            PH_FRINFC_DESF_CC_FILE_SIZE);
        i++;

        i += PH_FRINFC_DESF_CC_FILE_SIZE;

        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_WRITECC_CMD_SNLEN;
        break;

    case  PH_FRINFC_DESF_WRITENDEF_CMD:

        /* Instruction Cmd code */
        NdefSmtCrdFmt->SendRecvBuf[CmdByte] = PH_FRINFC_DESF_WRITE_CMD;

        /*  Lc: Length of wrapped data */
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_WRNDEF_WRDT_LEN;
        i++;

        /* set the file id*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NDEF_FILE_ID;
        i++;

        /* set the offset to zero*/
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /* Set the length of data available to write*/
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x02;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;
        NdefSmtCrdFmt->SendRecvBuf[i] = 0x00;
        i++;

        /*set the data to be written to the CC file*/

        (void)memcpy(&NdefSmtCrdFmt->SendRecvBuf[i],
            NdefFileBytes,
            2);
        i += 2;
        
        /* Le bytes*/
        NdefSmtCrdFmt->SendRecvBuf[i] = PH_FRINFC_DESF_NATIVE_LE_BYTE;
        i++;

        NdefSmtCrdFmt->SendLength = PH_FRINFC_DESF_WRITENDEF_CMD_SNLEN;
        break;

    default :
        break;
    }
}

static NFCSTATUS phFriNfc_Desf_HGetHWVersion(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
#ifdef PH_HAL4_ENABLE 
    /* Removed uint8_t i=0; */
#else
    uint8_t i=0;
#endif /* #ifdef PH_HAL4_ENABLE */

    /*set the state*/
    NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_GET_HW_VERSION;

    /* Helper routine to wrap the native DESFire cmds*/
    phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_GET_HW_VERSION_CMD);

    status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);

    return ( status);
}

static NFCSTATUS phFriNfc_Desf_HGetSWVersion(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);

    if( ( NdefSmtCrdFmt->SendRecvBuf[*(NdefSmtCrdFmt->SendRecvLength)- 1] == 
                PH_FRINFC_DESF_PICC_ADDI_FRAME_RESP) )
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_GET_SW_VERSION;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_GET_SW_VERSION_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return status;
}

static NFCSTATUS phFriNfc_Desf_HUpdateVersionDetails(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{
    NFCSTATUS status = PHNFCSTVAL(CID_NFC_NONE,
        NFCSTATUS_SUCCESS);

    if( ( NdefSmtCrdFmt->SendRecvBuf[*(NdefSmtCrdFmt->SendRecvLength)- 
        PH_SMTCRDFMT_DESF_VAL1] ==  PH_FRINFC_DESF_PICC_ADDI_FRAME_RESP ) )
    {
        NdefSmtCrdFmt->AddInfo.Type4Info.MajorVersion = NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL3];
        NdefSmtCrdFmt->AddInfo.Type4Info.MinorVersion = NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL4];
        if ( ( NdefSmtCrdFmt->AddInfo.Type4Info.MajorVersion == PH_FRINFC_DESF4_MAJOR_VERSION )&&
             ( NdefSmtCrdFmt->AddInfo.Type4Info.MinorVersion == PH_FRINFC_DESF4_MINOR_VERSION ))
        {
            /* card size of DESFire4 type */
            NdefSmtCrdFmt->AddInfo.Type4Info.CardSize = PH_FRINFC_DESF4_MEMORY_SIZE;

        }
        else
        {
            // need to handle the Desfire8 type cards
            // need to use get free memory
            status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
                NFCSTATUS_INVALID_REMOTE_DEVICE);
            
        }
    }
    return status;

}

static NFCSTATUS phFriNfc_Desf_HGetUIDDetails(phFriNfc_sNdefSmtCrdFmt_t * NdefSmtCrdFmt)
{

    NFCSTATUS status = NFCSTATUS_PENDING;
    if( ( NdefSmtCrdFmt->SendRecvBuf[*(NdefSmtCrdFmt->SendRecvLength)- 
        PH_SMTCRDFMT_DESF_VAL1] ==  PH_FRINFC_DESF_PICC_ADDI_FRAME_RESP) )
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_GET_UID;

        /* Helper routine to wrap the native desfire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_GET_UID_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }

    return status;

}


static NFCSTATUS phFriNfc_Desf_HCreateApp(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{
    NFCSTATUS status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);

    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL14] == PH_FRINFC_DESF_NAT_WRAP_FIRST_RESP_BYTE)
         && (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL15] == PH_FRINFC_DESF_NAT_WRAP_SEC_RESP_BYTE ))
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_CREATE_AID;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_CREATEAPP_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return ( status);
}


static NFCSTATUS phFriNfc_Desf_HSelectApp(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);

    /* check for the response of previous operation, before
    issuing the next command*/

    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NAT_WRAP_FIRST_RESP_BYTE) && 
        (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NAT_WRAP_SEC_RESP_BYTE ))
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_SELECT_APP;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_SELECTAPP_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return ( status);

}

static NFCSTATUS phFriNfc_Desf_HCreatCCFile(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{
    NFCSTATUS status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);

    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NATIVE_RESP_BYTE1) &&
            (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NATIVE_RESP_BYTE2 ))
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_CREATE_CCFILE;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_CREATECC_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return ( status);
}

static NFCSTATUS phFriNfc_Desf_HCreatNDEFFile(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);

    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NATIVE_RESP_BYTE1) &&
            (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NATIVE_RESP_BYTE2 ))
    {   
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_CREATE_NDEFFILE;

        /* Helper routine to wrap the native desfire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_CREATENDEF_CMD);

        status = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);

    }

    return ( status);

}

static NFCSTATUS phFriNfc_Desf_HWrCCBytes(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);
    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NATIVE_RESP_BYTE1) &&
            (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NATIVE_RESP_BYTE2 ))
    {

        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_WRITE_CC_FILE;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_WRITECC_CMD);

        result = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return (result);
}

static NFCSTATUS phFriNfc_Desf_HWrNDEFData(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT,
        NFCSTATUS_FORMAT_ERROR);


    if ( (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NATIVE_RESP_BYTE1) && 
            (NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NATIVE_RESP_BYTE2 ))
    {
        /*set the state*/
        NdefSmtCrdFmt->State = PH_FRINFC_DESF_STATE_WRITE_NDEF_FILE;

        /* Helper routine to wrap the native DESFire cmds*/
        phFriNfc_Desf_HWrapISONativeCmds(NdefSmtCrdFmt,PH_FRINFC_DESF_WRITENDEF_CMD);

        Result = phFriNfc_Desf_HSendTransCmd(NdefSmtCrdFmt);
    }
    return (Result);
}

static NFCSTATUS phFriNfc_Desf_HSendTransCmd(phFriNfc_sNdefSmtCrdFmt_t *NdefSmtCrdFmt)
{

    NFCSTATUS status =  NFCSTATUS_SUCCESS;

    /* set the command type*/
#ifdef PH_HAL4_ENABLE
    NdefSmtCrdFmt->Cmd.Iso144434Cmd = phHal_eIso14443_4_Raw;
#else
    NdefSmtCrdFmt->Cmd.Iso144434Cmd = phHal_eIso14443_4_CmdListTClCmd;
#endif /* #ifdef PH_HAL4_ENABLE */

    /* set the Additional Info*/
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.MetaChaining = 0;
    NdefSmtCrdFmt->psDepAdditionalInfo.DepFlags.NADPresent = 0;

    /*set the completion routines for the desfire card operations*/
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.CompletionRoutine = phFriNfc_NdefSmtCrd_Process;
    NdefSmtCrdFmt->SmtCrdFmtCompletionInfo.Context = NdefSmtCrdFmt;

    /* set the receive length */
    *NdefSmtCrdFmt->SendRecvLength = PH_FRINFC_SMTCRDFMT_MAX_SEND_RECV_BUF_SIZE;
    

    /*Call the Overlapped HAL Transceive function */ 
    status = phFriNfc_OvrHal_Transceive(NdefSmtCrdFmt->LowerDevice,
        &NdefSmtCrdFmt->SmtCrdFmtCompletionInfo,
        NdefSmtCrdFmt->psRemoteDevInfo,
        NdefSmtCrdFmt->Cmd,
        &NdefSmtCrdFmt->psDepAdditionalInfo,
        NdefSmtCrdFmt->SendRecvBuf,
        NdefSmtCrdFmt->SendLength,
        NdefSmtCrdFmt->SendRecvBuf,
        NdefSmtCrdFmt->SendRecvLength);

    return (status);


}

NFCSTATUS phFriNfc_Desfire_Format(phFriNfc_sNdefSmtCrdFmt_t    *NdefSmtCrdFmt)
{

    NFCSTATUS status =  NFCSTATUS_SUCCESS;
    status =  phFriNfc_Desf_HGetHWVersion(NdefSmtCrdFmt);
    return (status);
}

void phFriNfc_Desf_Process( void       *Context,
                           NFCSTATUS   Status)
{

    phFriNfc_sNdefSmtCrdFmt_t      *NdefSmtCrdFmt; 

    NdefSmtCrdFmt = (phFriNfc_sNdefSmtCrdFmt_t *)Context;


    if((Status & PHNFCSTBLOWER) == (NFCSTATUS_SUCCESS & PHNFCSTBLOWER))
    {
        switch(NdefSmtCrdFmt->State)
        {

        case PH_FRINFC_DESF_STATE_GET_HW_VERSION :

            /* Check and store the h/w and s/w specific details.
            Ex: Major/Minor version, memory storage info. */

            Status = phFriNfc_Desf_HGetSWVersion(NdefSmtCrdFmt);
        
            break;

        case PH_FRINFC_DESF_STATE_GET_SW_VERSION :

            /* Check and store the h/w and s/w specific details.
            Ex: Major/Minor version, memory storage info. */

            Status = phFriNfc_Desf_HUpdateVersionDetails(NdefSmtCrdFmt);
            if ( Status == NFCSTATUS_SUCCESS )
            {
                Status = phFriNfc_Desf_HGetUIDDetails(NdefSmtCrdFmt);
            }
            break;
        case PH_FRINFC_DESF_STATE_GET_UID : 

            Status = phFriNfc_Desf_HCreateApp(NdefSmtCrdFmt);
            break;

        case PH_FRINFC_DESF_STATE_CREATE_AID :  

            Status = phFriNfc_Desf_HSelectApp(NdefSmtCrdFmt);
            break;  

        case PH_FRINFC_DESF_STATE_SELECT_APP :  

            Status = phFriNfc_Desf_HCreatCCFile(NdefSmtCrdFmt);
            break;  

        case PH_FRINFC_DESF_STATE_CREATE_CCFILE :

            Status = phFriNfc_Desf_HCreatNDEFFile(NdefSmtCrdFmt);
            break;  

        case PH_FRINFC_DESF_STATE_CREATE_NDEFFILE :

            Status = phFriNfc_Desf_HWrCCBytes(NdefSmtCrdFmt);
            break;  

        case  PH_FRINFC_DESF_STATE_WRITE_CC_FILE :

            Status = phFriNfc_Desf_HWrNDEFData(NdefSmtCrdFmt);
            break;  

        case  PH_FRINFC_DESF_STATE_WRITE_NDEF_FILE :
            
            if (( NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL0] == PH_FRINFC_DESF_NATIVE_RESP_BYTE1 )&&
                    ( NdefSmtCrdFmt->SendRecvBuf[PH_SMTCRDFMT_DESF_VAL1] == PH_FRINFC_DESF_NATIVE_RESP_BYTE2 ))
            {
                Status = PHNFCSTVAL(CID_NFC_NONE,
                    NFCSTATUS_SUCCESS);   
                NdefSmtCrdFmt->CardState = 0;
            }
            break;  
    
        default :
            /*set the invalid state*/
            Status = PHNFCSTVAL(CID_FRI_NFC_NDEF_SMTCRDFMT, 
                NFCSTATUS_INVALID_DEVICE_REQUEST);
            break;
        }
    }
    /* Handle the all the error cases*/
    if ( (Status & PHNFCSTBLOWER) != (NFCSTATUS_PENDING & PHNFCSTBLOWER))
    {
        /* call respective CR */
        phFriNfc_SmtCrdFmt_HCrHandler(NdefSmtCrdFmt,Status);
    }

}

