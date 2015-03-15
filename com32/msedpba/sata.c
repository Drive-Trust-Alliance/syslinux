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
#include <dprintf.h>

#include "ahci.h"
#include "sata.h"

#define SATA_IOCTL_MEM_SIZE 4096*2  //ensure buffer 4K aligned 
#define PRT dprintf
int sataIOCtl(AHCI_PORT *port, uint8_t write, void * fis, size_t fislength, void *buffer, size_t buflength) {
    if(buflength % 2) {
        PRT("SATA Spec calls for even byte I/O\n");
        return -99;
    }
    if(8192 < buflength) {
        PRT("Driver currently supports max 8192 byte I/O\n");
        return -98;
    }
/* allocate and populate the AHCI Command list structure */
       PRT("Entered sata_I/O control \n");
/* void pointers for math */
    void *memBase;       /*< initial memory pointer before alignment */
    void *commandTable;      /*< AHCI Command table  */
    void *prdtAddr;         /*< address of PRDT */
    AHCI_PRDTE *prdte;
    void *ioBuffer;         /*< address of the I/O buffer */
    memBase = malloc(SATA_IOCTL_MEM_SIZE+buflength); 
    if(NULL == memBase) { 
        PRT("Error allocating memory for sata I/O control \n");
        return -1;  /* should reconcile these with std error numbers */
    }
    memset(memBase, 0, SATA_IOCTL_MEM_SIZE+buflength);
    commandTable = memBase + 4096;
 #ifdef __FIRMWARE_EFI64__
    commandTable = (void*) ((uint64_t) commandTable & ~(4096-1)); /* back up to a 4k boundary */
#else
    commandTable = (void *) ((uint32_t) commandTable & ~(4096-1)); /* back up to a 4k boundary */
#endif
    memcpy(commandTable,fis,fislength);
    prdtAddr = commandTable + 0x80;
    prdte = (AHCI_PRDTE *) prdtAddr;
    ioBuffer = commandTable + 4096;
 #ifdef __FIRMWARE_EFI64__
    prdte->DBA = (uint32_t) ((uint64_t) ioBuffer & 0xffffffff) ;
     prdte->DBAU = (uint32_t) ((uint64_t) ioBuffer >> 32) & 0xffffffff;
#else
    prdte->DBA = (uint32_t) ioBuffer;
    prdte->DBAU = 0;
#endif
    prdte->DI.DBC = buflength - 1; /* DBC is 0 based */
    prdte->DI.I = 1;
    memcpy(ioBuffer,buffer,buflength);
    PRT("commandTable %p PRDT %p ioBuffer %p \n", commandTable, prdtAddr, ioBuffer);
/* update the command slot with the address of the command list */
// assuming single threaded initialization and using slot 0 
#ifdef __FIRMWARE_EFI64__
   AHCI_COMMAND_HEADER *commandHeader = (AHCI_COMMAND_HEADER *) (((uint64_t) port->CLB) & (((uint64_t) port->CLBU) << 32));
#else
    AHCI_COMMAND_HEADER *commandHeader = (AHCI_COMMAND_HEADER *) port->CLB;
#endif
    memset((void *)commandHeader, 0, sizeof(AHCI_COMMAND_HEADER));
    commandHeader->DI.CFL = fislength / 4;
    commandHeader->DI.W = write;
    commandHeader->DI.PRDTL = 1;
 #ifdef __FIRMWARE_EFI64__
     commandHeader->CTBA = (uint32_t) ((uint64_t)commandTable & 0xffffffff) ;
     commandHeader->CTBAU = (uint32_t) ((uint64_t)commandTable >> 32) & 0xffffffff;
#else   
    commandHeader->CTBA = (uint32_t) commandTable;
#endif
    /* tell the HBA it has a command */
    port->IS.REG = 0xffff;
    port->CI |= 1; 
    PRT("issued command - bsy %01x drq %01x CI %08x\n", port->TFD.STSBSY, port->TFD.STSDRQ, port->CI);
    /* wait for the results */
    do {if(port->TFD.STSERR) break;} while (port->CI & 1);
    PRT("Ended - bsy %01x drq %01x STSERR %01x CI %08x\n", port->TFD.STSBSY, port->TFD.STSDRQ, 
            port->TFD.STSERR,port->CI);
    PRT("SATAIOCTL - PRDBC %08x\n", commandHeader->PRDBC);

    if (port->TFD.STSERR) {
        int saveError = port->TFD.ERR;
        // stop and restart the port to clear stserr
        port->CMD.ST = 0; /* stop the port */
        do {} while (port->CI != 0);
        port->CMD.FRE = 0;
        do {} while (port->CMD.FR != 0);
        port->CMD.FRE = 1;
        do {} while (port->CMD.FR != 1);
        port->CMD.ST = 1;
        do {} while (port->CMD.CR != 1);
        free(memBase);
        return saveError;
    }
/* this hangs in vbox but seems to be needed in real hardware so I added the counter */
    PRT("Spinning on IS.DPS\n");
    int spinondps = 0xffff;
    do {if(1 < --spinondps) break;} while (port->IS.BIT.DPS != 1);
    memcpy(buffer, ioBuffer, buflength);
    port->IS.REG = 0xffff;
    free(memBase);
    return 0;
}

int sataOpen(AHCI_PORT *port) {
    return ahci_initialize(port);
}

void sataClose(AHCI_PORT *port) {
    ahci_restore(port);
}

int sataIdentify(AHCI_PORT *port, void* buffer) {
    FIS_REGISTER_H2D fis;
    memset(&fis, 0, sizeof (FIS_REGISTER_H2D));
    fis.fis_type = FIS_TYPE_REGISTER_H2D;
    fis.command = ATACOMMAND_IDENTIFY;
    fis.c = 1;
    return sataIOCtl(port, 0, (void *) &fis, sizeof (FIS_REGISTER_H2D), buffer, 512);
}

int sataIFSEND(AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength) {
    return sataIFCMD(ATACOMMAND_IF_SEND, port, protocol, comid, buffer, bufLength);
}

int sataIFRECV(AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength) {
    return sataIFCMD(ATACOMMAND_IF_RECV, port, protocol, comid, buffer, bufLength);
}

int sataIFCMD(uint8_t cmd, AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength) {
    FIS_REGISTER_H2D fis;
    uint8_t readwrite;
    memset(&fis, 0, sizeof (FIS_REGISTER_H2D));
    fis.fis_type = FIS_TYPE_REGISTER_H2D;
    fis.command = cmd;
    fis.c = 1;
    if (ATACOMMAND_IF_RECV == cmd)
        readwrite = 0;
    else if (ATACOMMAND_IF_SEND == cmd)
        readwrite = 1;
    else return 0xff;
    /* FIS fields per ACS-3 pp 273 281  
     * and TCG SIIS pp 17 */
    fis.feature0 = protocol;
    /* count and lba0 have the length ACS-3 */
    fis.count0 = bufLength / 512;
    fis.lba0 = 0; // upper 8 bits of transfer length, always 0 for 8192 max 
    /* lba1 & lba2 are SP specific ACS3 */
    /* SP specific = comid SIIS */
    fis.lba1 = comid & 0x00ff;
    fis.lba2 = (comid >> 8) & 0x00ff;
    return sataIOCtl(port, readwrite, (void *) &fis, sizeof (FIS_REGISTER_H2D), buffer, bufLength);
}
  