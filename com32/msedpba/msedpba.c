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
#include "msedpba.h"
#include "unlockOpal.h"
#include "sata.h"
#include "trace.h"

// define stuff stolen from syslinux codebase
int chainload(int argc, char *argv[]);
int ask_passwd(char *user_passwd);

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
    uint8_t rc;
    char user_password[260];
    char *chainargs[] = { "chain.c32","hd0,0" ,};
    
    msedTraceLevel = 0;
    printf("Msed PBA for BIOS machines \n\n");
    if(argc > 1) {
        if(!(memcmp("TRACE",argv[1],5))) {
             msedTraceLevel = 1;
        }
    }
    if(argc > 2) {
        chainargs[1] = argv[2];
        printf("ChainLoading changed to %s\n",chainargs[1]);
    }
     
again:
    console_ansi_raw();
    printf("Enter Pass Phrase to unlock OPAL SSC drives>");
    if(ask_passwd(user_password)) {
        printf("\n");
        goto again;
    }
    printf("\n");
    console_ansi_std();
    domain = pci_scan();
    if(!domain) {
        printf("No PCI bus found??\n");
        goto chainload;
    }
    

    for_each_pci_func3(device, domain, address) {
	TRACE printf("DID %08x SDID %08x CLASS %08x\n",
                    device->vid_did,
                    device->svid_sdid,
                    device->rid_class);
        if((device->rid_class >> 8) != 0x010601) continue;
        TRACE {
            printf("Found SATA controller in AHCI mode on the PCI buss @ %08x\n", address);
            printf("vendor/product %08x, class %08x, misc@+0c %08x \n",pci_readl(address),pci_readl(address+0x08),pci_readl(address+0x0c));
        }
        if(0x0000 != (pci_readw(address + 0x0c) & 0x00ff)) {
          printf("Invalid PCI header type %02x\n",(pci_readw(address + 0x0c) & 0x00ff));
          continue;
         }
        ABAR = pci_readl(address + 0x24);
        TRACE printf("PCI ABAR Registers:\n  %08x \n", ABAR);
        TRACE fgets(consoleBuffer, sizeof consoleBuffer, stdin);
        abar = (AHCI_GLOBAL *)ABAR;
        TRACE printf("Scanning for devices on adapter pi=%08x\n", abar->PI);
        for (int i = 0; i < 32; i++) {
            if (abar->PI & (1 << i)) {
                TRACE printf("SSTS %01x:%01x:%01x sig %08x\n", abar->PORT[i].SSTS.DET,
                        abar->PORT[i].SSTS.SPD, abar->PORT[i].SSTS.IPM,
                        abar->PORT[i].SIG);
                if (abar->PORT[i].SSTS.DET != AHCI_PORT_DET_READY) continue;
                if (abar->PORT[i].SSTS.IPM != AHCI_PORT_IPM_ACTIVE) continue;
                if (abar->PORT[i].SIG != SATA_SIG_ATA) continue;
                printf("Found device ");
                if(sataOpen(&abar->PORT[i])) {
                    printf("Failed to open SATA port\n");
                    continue;
                }
                if(sataIdentify(&abar->PORT[i], &ioBuffer)) { 
                    printf("identify failed\n");
                    continue;
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
                TRACE printf("Identify %s %s %s\n", disk_info.modelNum,disk_info.firmwareRev,disk_info.serialNum);
                printf(" %s ", disk_info.modelNum);
                discovery0(&abar->PORT[i], &disk_info);
                uint8_t *dump = (uint8_t *) &disk_info;
                TRACE {
                    printf("disk_info:");
                    for (int i = 0; i < (int) sizeof(disk_info); i++) {
                        if (!(i % 32)) printf("\n%04x ", i);
                        if (!(i % 4)) printf(" ");
                        printf("%02x", dump[i]);
                    }
                    fgets(consoleBuffer, sizeof consoleBuffer, stdin);
                }
                if((disk_info.OPAL10 || disk_info.OPAL20) && disk_info.Locking_locked) {
                    printf(" is OPAL ");
                    rc = unlockOpal(&abar->PORT[i],user_password, &disk_info);
                    TRACE fgets(consoleBuffer, sizeof consoleBuffer, stdin);
                    if(rc == 0) {
                        printf("unlocked \n");
                    } else {
                        printf("Failed \n");
                        fail += 1;    
                    }
                } else 
                    printf(" NOT OPAL \n");
                sataClose(&abar->PORT[i]);
            }
        }
    }	     
    free_pci_domain(domain);
chainload:
    printf("About to Chainload %s ",chainargs[1]);
    fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    chainload(2, chainargs);
}
