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
#include <syslinux/boot.h>
#include <syslinux/reboot.h>
//#include <com32.h>
//#include <stdbool.h>
//#include <ctype.h>
//#include <dprintf.h>
#include "msedpba.h"
#include "unlockOpal.h"
#include "sata.h"

int chainload(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    struct pci_domain *domain = NULL;
    struct pci_device *device = NULL;
    pciaddr_t address;
/* PCI base address registers */
    uint32_t ABAR;
    AHCI_GLOBAL *abar;
    uint8_t ioBuffer[2048];
    char consoleBuffer[100];
    OPAL_DiskInfo disk_info;
    uint32_t fail = 0;
    
    if(argc > 1) printf("argv[1] = %s \n",argv[1]);

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
        if((device->rid_class >> 8) != 0x010601) continue;
        printf("Found SATA controller in AHCI mode on the PCI buss @ %08x\n", address);
        printf("vendor/product %08x, class %08x, misc@+0c %08x \n",pci_readl(address),pci_readl(address+0x08),pci_readl(address+0x0c));
        if(0x0000 != (pci_readw(address + 0x0c) & 0x00ff)) {
          printf("Invalid PCI header type %02x\n",(pci_readw(address + 0x0c) & 0x00ff));
          return 2;
         }
        ABAR = pci_readl(address + 0x24);
        printf("PCI ABAR Registers:\n  %08x \n", ABAR);
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
        abar = (AHCI_GLOBAL *)ABAR;
        printf("Scanning for devices on adapter pi=%08x\n", abar->PI);
        for (int i = 0; i < 32; i++) {
            if (abar->PI & (1 << i)) {
                printf("SSTS %01x:%01x:%01x sig %08x\n", abar->PORT[i].SSTS.DET,
                        abar->PORT[i].SSTS.SPD, abar->PORT[i].SSTS.IPM,
                        abar->PORT[i].SIG);
                if (abar->PORT[i].SSTS.DET != AHCI_PORT_DET_READY) continue;
                if (abar->PORT[i].SSTS.IPM != AHCI_PORT_IPM_ACTIVE) continue;
                if (abar->PORT[i].SIG != SATA_SIG_ATA) continue;
                if(sataOpen(&abar->PORT[i])) return 3;
                if(sataIdentify(&abar->PORT[i], &ioBuffer)) { 
                    printf("identify failed\n");
                    return 4;
                }
                memset(&disk_info,0,sizeof(OPAL_DiskInfo));
                disk_info.devType = 1;
                IDENTIFY_RESPONSE *identifyResp = (IDENTIFY_RESPONSE *)ioBuffer;
// fix and save the drive info
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
                uint8_t *dump = (uint8_t *) &disk_info;
                printf("command:");
                for (int i = 0; i < (int) sizeof(disk_info); i++) {
                    if (!(i % 32)) printf("\n%04x ", i);
                    if (!(i % 4)) printf(" ");
                    printf("%02x", dump[i]);
                }
                fgets(consoleBuffer, sizeof consoleBuffer, stdin);
                if((disk_info.OPAL10 || disk_info.OPAL20) && disk_info.Locking_locked) {
                    fail |= unlockOpal(&abar->PORT[i],"passw0rd", &disk_info);
                    fgets(consoleBuffer, sizeof consoleBuffer, stdin);               
                }
                sataClose(&abar->PORT[i]);
                
                
            }
        }
    }	     
     
    free_pci_domain(domain);
    //syslinux_reboot(1);
    //syslinux_local_boot(1);
    char *chainargs[] = { "chain.c32","hd1,0" , };
    chainload(2, chainargs);
}

