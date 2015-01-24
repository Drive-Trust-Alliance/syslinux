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
#include <consoles.h>
#include <sys/pci.h>
//#include <com32.h>
//#include <stdbool.h>
//#include <ctype.h>
//#include <dprintf.h>
#include "msedpba.h"
#include "unlockOpal.h"
#include "sata.h"

int main(void)
{
    struct pci_domain *domain = NULL;
    struct pci_device *device = NULL;
    pciaddr_t address;
/* PCI base address registers */
    uint32_t BAR0, BAR1, BAR2, BAR3, BAR4, BAR5, ABAR;
    AHCI_GLOBAL *abar;
    uint8_t ioBuffer[2048];
    char consoleBuffer[100];
    OPAL_DiskInfo disk_info;
    uint32_t fail = 0;
    

     console_ansi_std();
     
    domain = pci_scan();
    if(!domain) {
        printf("No supported devices found\n");
        return 1;
    }
    

    for_each_pci_func3(device, domain, address) {
	printf("DID %08x SDID %08x CLASS %08x\n",
                    device->vid_did,
                    device->svid_sdid,
                    device->rid_class);
        if((device->rid_class >> 8) == 0x010601) {
            printf("Found SATA controller in AHCI mode on the PCI buss @ %08x\n", address);
            printf("vendor/product %08x, class %08x, misc@+0c %08x ",pci_readl(address),pci_readl(address+0x08),pci_readl(address+0x0c));
            if(0x0000 == (pci_readw(address + 0x0c) & 0x00ff)) {
                BAR0 = pci_readl(address + 0x10);
                BAR1 = pci_readl(address + 0x14);
                BAR2 = pci_readl(address + 0x18);
                BAR3 = pci_readl(address + 0x1c);
                BAR4 = pci_readl(address + 0x20);
                BAR5 = ABAR = pci_readl(address + 0x24);
                printf("PCI Base Registers:\n  %08x %08x %08x \n  %08x %08x %08x \n",
                        BAR0,BAR1,BAR2,BAR3,BAR4,BAR5);
          } else {
              printf("Invalid PCI header type %02x\n",(pci_readw(address + 0x0c) & 0x00ff));
              return 2;
             }
            abar = (AHCI_GLOBAL *)ABAR;
            printf("Scanning for devices on adapter pi=%08x\n", abar->PI);
            for (int i = 0; i < 32; i++) {
                if (abar->PI & (1 << i)) {
                    printf("SSTS %01x:%01x:%01x sig %08x\n", abar->PORT[i].SSTS.DET,
                            abar->PORT[i].SSTS.SPD, abar->PORT[i].SSTS.IPM,
                            abar->PORT[i].SIG);
                    if ((abar->PORT[i].SSTS.DET == AHCI_PORT_DET_READY) &&
                            (abar->PORT[i].SSTS.IPM == AHCI_PORT_IPM_ACTIVE) &&
                            (abar->PORT[i].SIG == SATA_SIG_ATA)
                            ) {
                        printf("Now do some IO\n");
                        if(sataOpen(&abar->PORT[i])) return 3;
                        if(sataIdentify(&abar->PORT[i], &ioBuffer)) {
                            printf("identify failed\n");
                        }
                        memset(&disk_info,0,sizeof(OPAL_DiskInfo));
                        disk_info.devType = 1;
                        
                        IDENTIFY_RESPONSE *identifyResp = (IDENTIFY_RESPONSE *)ioBuffer;
                        for (uint16_t i = 0; i < sizeof (disk_info.serialNum); i += 2) {
                            disk_info.serialNum[i] = identifyResp->serialNum[i + 1];
                            disk_info.serialNum[i + 1] = identifyResp->serialNum[i];
                        }
                        for (uint16_t i = 0; i < sizeof (disk_info.firmwareRev); i += 2) {
                            disk_info.firmwareRev[i] = identifyResp->firmwareRev[i + 1];
                            disk_info.firmwareRev[i + 1] = identifyResp->firmwareRev[i];
                        }
                        for (uint16_t i = 0; i < sizeof (disk_info.modelNum); i += 2) {
                            disk_info.modelNum[i] = identifyResp->modelNum[i + 1];
                            disk_info.modelNum[i + 1] = identifyResp->modelNum[i];
                        }
                        printf("Identify %s %s %s\n", disk_info.modelNum,disk_info.firmwareRev,disk_info.serialNum);
                        discovery0(&abar->PORT[i], &disk_info);
                        uint32_t *dump = (uint32_t *) &disk_info;
                        for(int i = 0; i < (int) sizeof(OPAL_DiskInfo) / 4; i+=4) {
                            printf("%08x %08x %08x %08x \n",dump[0+i],dump[1+i],dump[2+i],dump[3+i]);
                        }
                        if((disk_info.OPAL10 || disk_info.OPAL20) && disk_info.Locking_locked) 
                            fail |= unlockOpal(&abar->PORT[i],"passw0rd",disk_info.Locking_MBREnabled);
                        sataClose(&abar->PORT[i]);
                       fgets(consoleBuffer, sizeof consoleBuffer, stdin);
                    }
                }
            }
        }
    }	     
     
    free_pci_domain(domain);
}
