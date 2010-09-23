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
 * \file  phFriNfc_MapTools.c
 * \brief NFC Ndef Internal Mapping File.
 *
 * Project: NFC-FRI
 *
 * $Date: Fri Sep 26 11:13:22 2008 $
 * $Author: ing08205 $
 * $Revision: 1.5 $
 * $Aliases: NFC_FRI1.1_WK840_R10_PREP1,NFC_FRI1.1_WK840_R10_1,NFC_FRI1.1_WK842_R11_PREP1,NFC_FRI1.1_WK842_R11_PREP2,NFC_FRI1.1_WK842_R11_1,NFC_FRI1.1_WK844_PREP1,NFC_FRI1.1_WK844_R12_1,NFC_FRI1.1_WK846_PREP1,NFC_FRI1.1_WK846_R13_1,NFC_FRI1.1_WK848_PREP1,NFC_FRI1.1_WK848_R14_1,NFC_FRI1.1_WK850_PACK1,NFC_FRI1.1_WK851_PREP1,NFC_FRI1.1_WK850_R15_1,NFC_FRI1.1_WK902_PREP1,NFC_FRI1.1_WK902_R16_1,NFC_FRI1.1_WK904_PREP1,NFC_FRI1.1_WK904_R17_1,NFC_FRI1.1_WK906_R18_1,NFC_FRI1.1_WK908_PREP1,NFC_FRI1.1_WK908_R19_1,NFC_FRI1.1_WK910_PREP1,NFC_FRI1.1_WK910_R20_1,NFC_FRI1.1_WK912_PREP1,NFC_FRI1.1_WK912_R21_1,NFC_FRI1.1_WK914_PREP1,NFC_FRI1.1_WK914_R22_1,NFC_FRI1.1_WK914_R22_2,NFC_FRI1.1_WK916_R23_1,NFC_FRI1.1_WK918_R24_1,NFC_FRI1.1_WK920_PREP1,NFC_FRI1.1_WK920_R25_1,NFC_FRI1.1_WK922_PREP1,NFC_FRI1.1_WK922_R26_1,NFC_FRI1.1_WK924_PREP1,NFC_FRI1.1_WK924_R27_1,NFC_FRI1.1_WK926_R28_1,NFC_FRI1.1_WK928_R29_1,NFC_FRI1.1_WK930_R30_1,NFC_FRI1.1_WK934_PREP_1,NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $
 *
 */

#include <phFriNfc_NdefMap.h>
#include <phFriNfc_MapTools.h>

#ifndef PH_FRINFC_MAP_MIFAREUL_DISABLED
#include <phFriNfc_MifareULMap.h>
#endif  /* PH_FRINFC_MAP_MIFAREUL_DISABLED*/

#ifndef PH_FRINFC_MAP_MIFARESTD_DISABLED
#include <phFriNfc_MifareStdMap.h>
#endif  /* PH_FRINFC_MAP_MIFARESTD_DISABLED */

#ifndef PH_FRINFC_MAP_DESFIRE_DISABLED
#include <phFriNfc_DesfireMap.h>
#endif  /* PH_FRINFC_MAP_DESFIRE_DISABLED */

#ifndef PH_FRINFC_MAP_FELICA_DISABLED
#include <phFriNfc_FelicaMap.h>
#endif  /* PH_FRINFC_MAP_FELICA_DISABLED */

#include <phFriNfc_OvrHal.h>

/*! \ingroup grp_file_attributes
 *  \name NDEF Mapping
 *
 * File: \ref phFriNfc_MapTools.c
 *       This file has functions which are used common across all the
 *       typ1/type2/type3/type4 tags.
 *
 */
/*@{*/
#define PHFRINFCNDEFMAP_FILEREVISION "$Revision: 1.5 $"
#define PHFRINFCNDEFMAP_FILEALIASES  "$Aliases: NFC_FRI1.1_WK840_R10_PREP1,NFC_FRI1.1_WK840_R10_1,NFC_FRI1.1_WK842_R11_PREP1,NFC_FRI1.1_WK842_R11_PREP2,NFC_FRI1.1_WK842_R11_1,NFC_FRI1.1_WK844_PREP1,NFC_FRI1.1_WK844_R12_1,NFC_FRI1.1_WK846_PREP1,NFC_FRI1.1_WK846_R13_1,NFC_FRI1.1_WK848_PREP1,NFC_FRI1.1_WK848_R14_1,NFC_FRI1.1_WK850_PACK1,NFC_FRI1.1_WK851_PREP1,NFC_FRI1.1_WK850_R15_1,NFC_FRI1.1_WK902_PREP1,NFC_FRI1.1_WK902_R16_1,NFC_FRI1.1_WK904_PREP1,NFC_FRI1.1_WK904_R17_1,NFC_FRI1.1_WK906_R18_1,NFC_FRI1.1_WK908_PREP1,NFC_FRI1.1_WK908_R19_1,NFC_FRI1.1_WK910_PREP1,NFC_FRI1.1_WK910_R20_1,NFC_FRI1.1_WK912_PREP1,NFC_FRI1.1_WK912_R21_1,NFC_FRI1.1_WK914_PREP1,NFC_FRI1.1_WK914_R22_1,NFC_FRI1.1_WK914_R22_2,NFC_FRI1.1_WK916_R23_1,NFC_FRI1.1_WK918_R24_1,NFC_FRI1.1_WK920_PREP1,NFC_FRI1.1_WK920_R25_1,NFC_FRI1.1_WK922_PREP1,NFC_FRI1.1_WK922_R26_1,NFC_FRI1.1_WK924_PREP1,NFC_FRI1.1_WK924_R27_1,NFC_FRI1.1_WK926_R28_1,NFC_FRI1.1_WK928_R29_1,NFC_FRI1.1_WK930_R30_1,NFC_FRI1.1_WK934_PREP_1,NFC_FRI1.1_WK934_R31_1,NFC_FRI1.1_WK941_PREP1,NFC_FRI1.1_WK941_PREP2,NFC_FRI1.1_WK941_1,NFC_FRI1.1_WK943_R32_1,NFC_FRI1.1_WK949_PREP1,NFC_FRI1.1_WK943_R32_10,NFC_FRI1.1_WK943_R32_13,NFC_FRI1.1_WK943_R32_14,NFC_FRI1.1_WK1007_R33_1,NFC_FRI1.1_WK1007_R33_4,NFC_FRI1.1_WK1017_PREP1,NFC_FRI1.1_WK1017_R34_1,NFC_FRI1.1_WK1017_R34_2,NFC_FRI1.1_WK1023_R35_1 $"
/*@}*/

NFCSTATUS phFriNfc_MapTool_SetCardState(phFriNfc_NdefMap_t  *NdefMap,
                                        uint32_t            Length)
{
    NFCSTATUS   Result = NFCSTATUS_SUCCESS;
    if(Length == PH_FRINFC_NDEFMAP_MFUL_VAL0)
    {
        /* As the NDEF LEN / TLV Len is Zero, irrespective of any state the card 
           shall be set to INITIALIZED STATE*/
        NdefMap->CardState =(uint8_t) (((NdefMap->CardState == 
                                PH_NDEFMAP_CARD_STATE_READ_ONLY) || 
                                (NdefMap->CardState == 
                                PH_NDEFMAP_CARD_STATE_INVALID))?
                                PH_NDEFMAP_CARD_STATE_INVALID:
                                PH_NDEFMAP_CARD_STATE_INITIALIZED);
    }
    else
    {
        switch(NdefMap->CardState)
        {
            case PH_NDEFMAP_CARD_STATE_INITIALIZED:
                NdefMap->CardState =(uint8_t) ((NdefMap->CardState == 
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_WRITE);
            break;

            case PH_NDEFMAP_CARD_STATE_READ_ONLY:
                NdefMap->CardState = (uint8_t)((NdefMap->CardState == 
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_ONLY);
            break;

            case PH_NDEFMAP_CARD_STATE_READ_WRITE:
                NdefMap->CardState = (uint8_t)((NdefMap->CardState == 
                    PH_NDEFMAP_CARD_STATE_INVALID)?
                    NdefMap->CardState:
                    PH_NDEFMAP_CARD_STATE_READ_WRITE);
            break;

            default:
                NdefMap->CardState = PH_NDEFMAP_CARD_STATE_INVALID;
                Result = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, 
                            NFCSTATUS_NO_NDEF_SUPPORT);
            break;
        }
    }
    Result = ((NdefMap->CardState == 
                PH_NDEFMAP_CARD_STATE_INVALID)?
                PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP, 
                NFCSTATUS_NO_NDEF_SUPPORT):
                Result);
    return Result;
}

/*  To check mapping spec version */

NFCSTATUS   phFriNfc_MapTool_ChkSpcVer( const phFriNfc_NdefMap_t  *NdefMap,
                                        uint8_t             VersionIndex)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    uint8_t TagVerNo = NdefMap->SendRecvBuf[VersionIndex];

    if ( TagVerNo == 0 )
    {
        /*Return Status Error “ Invalid Format”*/
        status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
    }
    else if((NdefMap->CardType == PH_FRINFC_NDEFMAP_MIFARE_STD_1K_CARD) || 
        (NdefMap->CardType == PH_FRINFC_NDEFMAP_MIFARE_STD_4K_CARD))
    {
        /* calculate the major and minor version number of Mifare std version number */
        status = (( (( PH_NFCFRI_MFSTDMAP_NFCDEV_MAJOR_VER_NUM == 
                        PH_NFCFRI_MFSTDMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_MFSTDMAP_NFCDEV_MINOR_VER_NUM == 
                        PH_NFCFRI_MFSTDMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
                    (( PH_NFCFRI_MFSTDMAP_NFCDEV_MAJOR_VER_NUM == 
                        PH_NFCFRI_MFSTDMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
                    ( PH_NFCFRI_MFSTDMAP_NFCDEV_MINOR_VER_NUM < 
                        PH_NFCFRI_MFSTDMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))?
                NFCSTATUS_SUCCESS:
                PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,
                            NFCSTATUS_INVALID_FORMAT));
    }
    else
    {
        /* calculate the major and minor version number of T3VerNo */
        if( (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM == 
                PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
            ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM == 
                PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo))) ||
            (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM == 
                PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo ) )&&
            ( PH_NFCFRI_NDEFMAP_NFCDEV_MINOR_VER_NUM < 
                PH_NFCFRI_NDEFMAP_GET_MINOR_TAG_VERNO(TagVerNo) )))
        {
            status = PHNFCSTVAL(CID_NFC_NONE,NFCSTATUS_SUCCESS);
        }
        else 
        {
            if (( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM <
                    PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo) ) ||
               ( PH_NFCFRI_NDEFMAP_NFCDEV_MAJOR_VER_NUM > 
                    PH_NFCFRI_NDEFMAP_GET_MAJOR_TAG_VERNO(TagVerNo)))
            {
                status = PHNFCSTVAL(CID_FRI_NFC_NDEF_MAP,NFCSTATUS_INVALID_FORMAT);
            }
        }
    }
    return (status);
}
