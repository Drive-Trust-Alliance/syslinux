/* C:B**************************************************************************
This software is Copyright 2014-2015 Michael Romeo <r0m30@r0m30.com>

    This file is part of msed.

    msed is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    msed is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with msed.  If not, see <http://www.gnu.org/licenses/>.

* C:E********************************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dprintf.h>

#include "ahci.h"
#include "sata.h"

#define SATA_IOCTL_MEM_SIZE 128+0x100
int sataIOCtl(AHCI_PORT *port, uint8_t write, void * fis, size_t fislength, void *buffer, size_t buflength) {
    if(buflength % 2) {
        printf("SATA Spec calls for even byte I/O\n");
        return -99;
    }
    if(8192 < buflength) {
        printf("Driver currently supports max 8192 byte I/O\n");
        return -98;
    }
/* allocate and populate the AHCI Command list structure */
       printf("Entered sata_I/O control \n");
/* void pointers for math */
    void *memBase;       /*< initial memory pointer before alignment */
    void *commandTable;      /*< AHCI Command table  */
    void *prdtAddr;         /*< address of PRDT */
    AHCI_PRDTE *prdte;
    void *ioBuffer;         /*< address of the I/O buffer */
    memBase = malloc(SATA_IOCTL_MEM_SIZE+buflength); 
    if((uint32_t)memBase < 1) { 
        printf("Error allocating memory for sata I/O control \n");
        return -1;  /* should reconcile these with std error numbers */
    }
    memset(memBase, 0, SATA_IOCTL_MEM_SIZE+buflength);
    commandTable = (void *) ((((uint32_t)memBase + 128) >> 7) << 7); /* go forward to a 128b boundary */
    memcpy(commandTable,fis,16*4);
    prdtAddr = commandTable +0x80;
    prdte = (AHCI_PRDTE *) prdtAddr;
    ioBuffer = commandTable + 0x100;
    prdte->DBA = (uint32_t) ioBuffer;
    prdte->DBAU = 0;
    prdte->DI.DBC = buflength - 1; /* DBC is 0 based */
    if(write) memcpy(ioBuffer,buffer,buflength);
/* update the command slot with the address of the command list */
    // assuming single threaded initialization and using slot 0 
    AHCI_COMMAND_HEADER *commandHeader = (AHCI_COMMAND_HEADER *) port->CLB;
    memset((void *)commandHeader, 0, sizeof(AHCI_COMMAND_HEADER));
    commandHeader->DI.CFL = fislength / 4;
    commandHeader->DI.W = write;
    commandHeader->DI.PRDTL = 1;
    commandHeader->CTBA = (uint32_t) commandTable;
    /* tell the HBA it has a command */
    port->CI &= 1; 
    /* wait for the results */
    do {} while (port->CI & 1);
    printf("SATAIOCTL - PRDBC %08x\n", commandHeader->PRDBC);
    if(!write) memcpy(buffer,ioBuffer,buflength);
    if(port->TFD.STSERR) return -2;
    return 0;
} 
  int sataIdentify(AHCI_PORT *port, void* buffer) {
      FIS_REGISTER_H2D fis;
      memset(&fis,0,sizeof(FIS_REGISTER_H2D));
      fis.fis_type = FIS_TYPE_REGISTER_H2D;
      fis.command = ATACOMMAND_IDENTIFY;
      fis.c = 1;
      return sataIOCtl(port, 0, (void *) &fis, sizeof(FIS_REGISTER_H2D), buffer, 512);
  }
  
