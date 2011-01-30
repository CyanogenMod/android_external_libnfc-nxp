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
 * \file phDalNfc_i2c.c
 * \brief DAL I2C port implementation for linux
 *
 * Project: Trusted NFC Linux
 *
 */

#define LOG_TAG "NFC_i2c"
#include <utils/Log.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <phDal4Nfc_debug.h>
#include <phDal4Nfc_i2c.h>
#include <phOsalNfc.h>
#include <phNfcStatus.h>
#if defined(ANDROID)
#include <string.h>
#endif

#include <linux/pn544.h>

typedef struct
{
   int  nHandle;
   char nOpened;

} phDal4Nfc_I2cPortContext_t;


/*-----------------------------------------------------------------------------------
                                      VARIABLES
------------------------------------------------------------------------------------*/
static phDal4Nfc_I2cPortContext_t gI2cPortContext;



/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_set_open_from_handle

PURPOSE:  Initialize internal variables

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_initialize(void)
{
   memset(&gI2cPortContext, 0, sizeof(phDal4Nfc_I2cPortContext_t));
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_set_open_from_handle

PURPOSE:  The application could have opened the link itself. So we just need
          to get the handle and consider that the open operation has already
          been done.

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_set_open_from_handle(phHal_sHwReference_t * pDalHwContext)
{
   gI2cPortContext.nHandle = (int) pDalHwContext->p_board_driver;
   DAL_ASSERT_STR(gI2cPortContext.nHandle >= 0, "Bad passed com port handle");
   gI2cPortContext.nOpened = 1;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_is_opened

PURPOSE:  Returns if the link is opened or not. (0 = not opened; 1 = opened)

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_is_opened(void)
{
   return gI2cPortContext.nOpened;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_flush

PURPOSE:  Flushes the link ; clears the link buffers

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_flush(void)
{
   /* Nothing to do (driver has no internal buffers) */
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_close

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

void phDal4Nfc_i2c_close(void)
{
   DAL_PRINT("Closing port\n");
   if (gI2cPortContext.nOpened == 1)
   {
      close(gI2cPortContext.nHandle);
      gI2cPortContext.nHandle = 0;
      gI2cPortContext.nOpened = 0;
   }
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_open_and_configure

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

NFCSTATUS phDal4Nfc_i2c_open_and_configure(pphDal4Nfc_sConfig_t pConfig, void ** pLinkHandle)
{
   char *       pComPort;

   DAL_ASSERT_STR(gI2cPortContext.nOpened==0, "Trying to open but already done!");

   switch(pConfig->nLinkType)
   {
       case ENUM_DAL_LINK_TYPE_I2C:
          pComPort = "/dev/pn544";
          break;
       default:
          DAL_DEBUG("Open failed: unknown type %d\n", pConfig->nLinkType);
          return NFCSTATUS_INVALID_PARAMETER;
   }

   DAL_DEBUG("Opening port=%s\n", pComPort);

   /* open port */
   gI2cPortContext.nHandle = open(pComPort, O_RDWR | O_NOCTTY);
   if (gI2cPortContext.nHandle < 0)
   {
       DAL_DEBUG("Open failed: open() returned %d\n", gI2cPortContext.nHandle);
      *pLinkHandle = NULL;
      return PHNFCSTVAL(CID_NFC_DAL, NFCSTATUS_INVALID_DEVICE);
   }

   gI2cPortContext.nOpened = 1;
   *pLinkHandle = (void*)gI2cPortContext.nHandle;

   DAL_PRINT("Open succeed\n");

   return NFCSTATUS_SUCCESS;
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_read

PURPOSE:  Reads nNbBytesToRead bytes and writes them in pBuffer.
          Returns the number of bytes really read or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_read(uint8_t * pBuffer, int nNbBytesToRead)
{
   int ret;
   DAL_ASSERT_STR(gI2cPortContext.nOpened == 1, "read called but not opened!");

   DAL_DEBUG("Reading %d bytes\n", nNbBytesToRead);
   ret = read(gI2cPortContext.nHandle, pBuffer, nNbBytesToRead);
   if (ret < 0)
   {
      DAL_DEBUG("Read failed: read() returned %d\n", ret);
   }
   else
   {
      DAL_DEBUG("Read succeed (%d bytes)\n", ret);
   }
   return ret;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_write

PURPOSE:  Writes nNbBytesToWrite bytes from pBuffer to the link
          Returns the number of bytes that have been wrote to the interface or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_i2c_write(uint8_t * pBuffer, int nNbBytesToWrite)
{
   int ret;
   DAL_ASSERT_STR(gI2cPortContext.nOpened == 1, "write called but not opened!");

   DAL_DEBUG("Writing %d bytes\n", nNbBytesToWrite);
   ret = write(gI2cPortContext.nHandle, pBuffer, nNbBytesToWrite);
   if (ret < 0)
   {
      DAL_DEBUG("Write failed: write() returned %d \n", ret);
   }
   else
   {
      DAL_DEBUG("Write succeed (%d bytes)\n", ret);
   }
   return ret;
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_i2c_reset

PURPOSE:  Reset the PN544, using the VEN pin

-----------------------------------------------------------------------------*/
int phDal4Nfc_i2c_reset(long level)
{
   int ret = NFCSTATUS_SUCCESS;   

   DAL_DEBUG("phDal4Nfc_i2c_reset, VEN level = %ld",level);

   ret = ioctl(gI2cPortContext.nHandle, PN544_SET_PWR, level);

   /* HACK to increase reset time
    * TODO: move this to kernel
    */
   if (level == 0) {
       LOGW("sleeping a little longer...");
       usleep(50000);
   } else {
       usleep(10000);
   }

   return ret;
}





