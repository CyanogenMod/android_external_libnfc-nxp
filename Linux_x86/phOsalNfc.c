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
 * \file phOsalNfc.c
 * \brief OSAL Implementation for linux
 *
 * Project: Trusted NFC Linux Light
 *
 * $Date: 03 aug 2009
 * $Author: Jérémie Corbier
 * $Revision: 1.0
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <phOsalNfc.h>

#ifdef ANDROID
#define LOG_TAG "NFC-HCI"

#include <utils/Log.h>

phOsalNfc_Exception_t phOsalNfc_Exception;
#endif

#ifdef DEBUG
#define MAX_PRINT_BUFSIZE (0x450U)
char phOsalNfc_DbgTraceBuffer[MAX_PRINT_BUFSIZE];
#endif

/*!
 * \brief Allocates memory.
 *        This function attempts to allocate \a size bytes on the heap and
 *        returns a pointer to the allocated block.
 *
 * \param size size of the memory block to be allocated on the heap.
 *
 * \return pointer to allocated memory block or NULL in case of error.
 */
void *phOsalNfc_GetMemory(uint32_t size)
{
   void *pMem = (void *)malloc(size);
   return pMem;
}

/*!
 * \brief Frees allocated memory block.
 *        This function deallocates memory region pointed to by \a pMem.
 *
 * \param pMem pointer to memory block to be freed.
 */
void phOsalNfc_FreeMemory(void *pMem)
{
   if(NULL !=  pMem)
      free(pMem);
}

void phOsalNfc_DbgString(const char *pString)
{
#ifdef DEBUG
   if(pString != NULL)
#ifndef ANDROID
      printf(pString);
#else
      LOGD("%s", pString);
#endif
#endif
}

void phOsalNfc_DbgTrace(uint8_t data[], uint32_t size)
{
#ifdef DEBUG
   uint32_t i;
#ifdef ANDROID
   char buf[10];
#endif

   if(size == 0)
      return;

#ifndef ANDROID
   for(i = 0; i < size; i++)
   {
      if((i % 10) == 0)
         printf("\n\t\t\t");
      printf("%02X ", data[i]);
   }
   printf("\n\tBlock size is: %d\n", size);
#else
   phOsalNfc_DbgTraceBuffer[0] = '\0';
   for(i = 0; i < size; i++)
   {
      if((i % 10) == 0)
      {
         LOGD("%s", phOsalNfc_DbgTraceBuffer);
         phOsalNfc_DbgTraceBuffer[0] = '\0';
      }

      snprintf(buf, 10, "%02X ", data[i]);
      strncat(phOsalNfc_DbgTraceBuffer, buf, 10);
   }
   LOGD("%s", phOsalNfc_DbgTraceBuffer);
   LOGD("Block size is: %d", size);
#endif
#endif
}

/*!
 * \brief Raises exception.
 *        This function raises an exception of type \a eExceptionType with
 *        reason \a reason to stack clients.
 *
 * \param eExceptionType exception type.
 * \param reason reason for this exception.
 *
 * \note Clients willing to catch exceptions are to handle the SIGABRT signal.
 *       On Linux, exception type and reason are passed to the signal handler as
 *       a pointer to a phOsalNfc_Exception_t structure.
 *       As sigqueue is not available in Android, exception information are
 *       stored in the phOsalNfc_Exception global.
 */
void phOsalNfc_RaiseException(phOsalNfc_ExceptionType_t eExceptionType, uint16_t reason)
{
    LOGD("phOsalNfc_RaiseException() called");

    if(eExceptionType == phOsalNfc_e_UnrecovFirmwareErr)
    {
        LOGE("HCI Timeout - Exception raised");
        LOGE("Force restart of NFC Service");
        abort();
    }
}

/*!
 * \brief display data bytes.
 *        This function displays data bytes for debug purpose
 * \param[in] pString pointer to string to be displayed.
 * \param[in] length number of bytes to be displayed.
 * \param[in] pBuffer pointer to data bytes to be displayed.
 *
 */
void phOsalNfc_PrintData(const char *pString, uint32_t length, uint8_t *pBuffer)
{
    char print_buffer[512]; // Max length 512 for the download mode
    int i;

    if(NULL!=pString && length > 1 && length < 34)
    {
        print_buffer[0] = '\0';
        for (i = 0; i < length; i++) {
            snprintf(&print_buffer[i*5], 6, " 0x%02X", pBuffer[i]);
        }
        LOGD("> NFC I2C %s: %s", pString,print_buffer);
    }
}

