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
 * \file phDalNfc_uart.c
 * \brief DAL com port implementation for linux
 *
 * Project: Trusted NFC Linux Lignt
 *
 * $Date: 07 aug 2009
 * $Author: Jonathan roux
 * $Revision: 1.0 $
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <phDal4Nfc_debug.h>
#include <phDal4Nfc_uart.h>
#include <phOsalNfc.h>
#include <phNfcStatus.h>
#if defined(ANDROID)
#include <string.h>
#endif

typedef struct
{
   int  nHandle;
   char nOpened;
   struct termios nIoConfigBackup;
   struct termios nIoConfig;

} phDal4Nfc_ComPortContext_t;

/*-----------------------------------------------------------------------------------
                                COM PORT CONFIGURATION
------------------------------------------------------------------------------------*/
#define DAL_BAUD_RATE  B115200



/*-----------------------------------------------------------------------------------
                                      VARIABLES
------------------------------------------------------------------------------------*/
static phDal4Nfc_ComPortContext_t gComPortContext;



/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_set_open_from_handle

PURPOSE:  Initialize internal variables

-----------------------------------------------------------------------------*/

void phDal4Nfc_uart_initialize(void)
{
   memset(&gComPortContext, 0, sizeof(phDal4Nfc_ComPortContext_t));
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_set_open_from_handle

PURPOSE:  The application could have opened the link itself. So we just need
          to get the handle and consider that the open operation has already
          been done.

-----------------------------------------------------------------------------*/

void phDal4Nfc_uart_set_open_from_handle(phHal_sHwReference_t * pDalHwContext)
{
   gComPortContext.nHandle = (int) pDalHwContext->p_board_driver;
   DAL_ASSERT_STR(gComPortContext.nHandle >= 0, "Bad passed com port handle");
   gComPortContext.nOpened = 1;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_is_opened

PURPOSE:  Returns if the link is opened or not. (0 = not opened; 1 = opened)

-----------------------------------------------------------------------------*/

int phDal4Nfc_uart_is_opened(void)
{
   return gComPortContext.nOpened;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_flush

PURPOSE:  Flushes the link ; clears the link buffers

-----------------------------------------------------------------------------*/

void phDal4Nfc_uart_flush(void)
{
   int ret;
   /* flushes the com port */
   ret = tcflush(gComPortContext.nHandle, TCIFLUSH);
   DAL_ASSERT_STR(ret!=-1, "tcflush failed");
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_close

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

void phDal4Nfc_uart_close(void)
{
   if (gComPortContext.nOpened == 1)
   {
      close(gComPortContext.nHandle);
      gComPortContext.nHandle = 0;
      gComPortContext.nOpened = 0;
   }
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_close

PURPOSE:  Closes the link

-----------------------------------------------------------------------------*/

NFCSTATUS phDal4Nfc_uart_open_and_configure(pphDal4Nfc_sConfig_t pConfig, void ** pLinkHandle)
{
   char *       pComPort;
   int          nComStatus;
   NFCSTATUS    nfcret = NFCSTATUS_SUCCESS;
   int          ret;

   DAL_ASSERT_STR(gComPortContext.nOpened==0, "Trying to open but already done!");

   switch(pConfig->nLinkType)
   {
     case ENUM_DAL_LINK_TYPE_COM1:
      pComPort = "/dev/ttyS0";
      break;
     case ENUM_DAL_LINK_TYPE_COM2:
      pComPort = "/dev/ttyS1";
      break;
     case ENUM_DAL_LINK_TYPE_COM3:
      pComPort = "/dev/ttyS2";
      break;
     case ENUM_DAL_LINK_TYPE_COM4:
      pComPort = "/dev/ttyS3";
      break;
     case ENUM_DAL_LINK_TYPE_COM5:
      pComPort = "/dev/ttyS4";
      break;
     case ENUM_DAL_LINK_TYPE_COM6:
      pComPort = "/dev/ttyS5";
      break;
     case ENUM_DAL_LINK_TYPE_COM7:
      pComPort = "/dev/ttyS6";
      break;
     case ENUM_DAL_LINK_TYPE_COM8:
      pComPort = "/dev/ttyS7";
      break;
     case ENUM_DAL_LINK_TYPE_USB:
      pComPort = "/dev/ttyUSB0";
      break;
     default:
      return NFCSTATUS_INVALID_PARAMETER;
   }

   /* open communication port handle */
   gComPortContext.nHandle = open(pComPort, O_RDWR | O_NOCTTY);
   if (gComPortContext.nHandle < 0)
   {
      *pLinkHandle = NULL;
      return PHNFCSTVAL(CID_NFC_DAL, NFCSTATUS_INVALID_DEVICE);
   }

   gComPortContext.nOpened = 1;
   *pLinkHandle = (void*)gComPortContext.nHandle;

   /*
    *  Now configure the com port
    */
   ret = tcgetattr(gComPortContext.nHandle, &gComPortContext.nIoConfigBackup); /* save the old io config */
   if (ret == -1)
   {
      /* tcgetattr failed -- it is likely that the provided port is invalid */
      *pLinkHandle = NULL;
      return PHNFCSTVAL(CID_NFC_DAL, NFCSTATUS_INVALID_DEVICE);
   }
   ret = fcntl(gComPortContext.nHandle, F_SETFL, 0); /* Makes the read blocking (default).  */
   DAL_ASSERT_STR(ret != -1, "fcntl failed");
   /* Configures the io */
   memset((void *)&gComPortContext.nIoConfig, (int)0, (size_t)sizeof(struct termios));
   /*
    BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
    CRTSCTS : output hardware flow control (only used if the cable has
              all necessary lines. See sect. 7 of Serial-HOWTO)
    CS8     : 8n1 (8bit,no parity,1 stopbit)
    CLOCAL  : local connection, no modem contol
    CREAD   : enable receiving characters
   */
   gComPortContext.nIoConfig.c_cflag = DAL_BAUD_RATE | CS8 | CLOCAL | CREAD;  /* Control mode flags */
   gComPortContext.nIoConfig.c_iflag = IGNPAR;                                          /* Input   mode flags : IGNPAR  Ignore parity errors */
   gComPortContext.nIoConfig.c_oflag = 0;                                               /* Output  mode flags */
   gComPortContext.nIoConfig.c_lflag = 0;                                               /* Local   mode flags. Read mode : non canonical, no echo */
   gComPortContext.nIoConfig.c_cc[VTIME] = 0;                                           /* Control characters. No inter-character timer */
   gComPortContext.nIoConfig.c_cc[VMIN]  = 1;                                           /* Control characters. Read is blocking until X characters are read */

   /*
      TCSANOW  Make changes now without waiting for data to complete
      TCSADRAIN   Wait until everything has been transmitted
      TCSAFLUSH   Flush input and output buffers and make the change
   */
   ret = tcsetattr(gComPortContext.nHandle, TCSANOW, &gComPortContext.nIoConfig);
   DAL_ASSERT_STR(ret != -1, "tcsetattr failed");

   /*
      On linux the DTR signal is set by default. That causes a problem for pn544 chip
      because this signal is connected to "reset". So we clear it. (on windows it is cleared by default).
   */
   ret = ioctl(gComPortContext.nHandle, TIOCMGET, &nComStatus);
   DAL_ASSERT_STR(ret != -1, "ioctl TIOCMGET failed");
   nComStatus &= ~TIOCM_DTR;
   ret = ioctl(gComPortContext.nHandle, TIOCMSET, &nComStatus);
   DAL_ASSERT_STR(ret != -1, "ioctl TIOCMSET failed");
   DAL_DEBUG("Com port status=%d\n", nComStatus);
   usleep(10000); /* Mandatory sleep so that the DTR line is ready before continuing */

   return nfcret;
}


/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_read

PURPOSE:  Reads nNbBytesToRead bytes and writes them in pBuffer.
          Returns the number of bytes really read or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_uart_read(uint8_t * pBuffer, int nNbBytesToRead)
{
   fd_set rfds;
   struct timeval tv;
   int ret;

   DAL_ASSERT_STR(gComPortContext.nOpened == 1, "read called but not opened!");

   FD_ZERO(&rfds);
   FD_SET(gComPortContext.nHandle, &rfds);

   /* select will block for 10 sec */
   tv.tv_sec = 2;
   tv.tv_usec = 0;

   ret = select(gComPortContext.nHandle + 1, &rfds, NULL, NULL, &tv);

   if (ret == -1)
      return -1;

   if (ret)
      return read(gComPortContext.nHandle, pBuffer, nNbBytesToRead);

   return 0;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_link_write

PURPOSE:  Writes nNbBytesToWrite bytes from pBuffer to the link
          Returns the number of bytes that have been wrote to the interface or -1 in case of error.

-----------------------------------------------------------------------------*/

int phDal4Nfc_uart_write(uint8_t * pBuffer, int nNbBytesToWrite)
{
   fd_set wfds;
   struct timeval tv;
   int ret;

   DAL_ASSERT_STR(gComPortContext.nOpened == 1, "write called but not opened!");

   FD_ZERO(&wfds);
   FD_SET(gComPortContext.nHandle, &wfds);

   /* select will block for 10 sec */
   tv.tv_sec = 2;
   tv.tv_usec = 0;

   ret = select(gComPortContext.nHandle + 1, NULL, &wfds, NULL, &tv);

   if (ret == -1)
      return -1;

   if (ret)
      return write(gComPortContext.nHandle, pBuffer, nNbBytesToWrite);

   return 0;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_reset

PURPOSE:  Reset the PN544, using the VEN pin

-----------------------------------------------------------------------------*/
int phDal4Nfc_uart_reset()
{
   DAL_PRINT("phDal4Nfc_uart_reset");

   return NFCSTATUS_FEATURE_NOT_SUPPORTED;
}

/*-----------------------------------------------------------------------------

FUNCTION: phDal4Nfc_uart_write

PURPOSE:  Put the PN544 in download mode, using the GPIO4 pin

-----------------------------------------------------------------------------*/
int phDal4Nfc_uart_download()
{
   DAL_PRINT("phDal4Nfc_uart_download");

   return NFCSTATUS_FEATURE_NOT_SUPPORTED;
}

