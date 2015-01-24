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

#define AHCI_INIT_MEM_SIZE (1024 + 1024+256+sizeof(AHCI_INIT_SAVE))

int ahci_initialize(AHCI_PORT *port) {
   printf("Entered ahci_initialize\n");
/* void pointers for math */
    void *memBase;       /*< initial memory pointer before alignment */
    void *commandList;   /*< pointer to command list */
    void *receivedFIS;   /*< received FIS stucture */
    AHCI_INIT_SAVE *ahciSave;    /*< Save area for status restore and free */

    memBase = malloc(AHCI_INIT_MEM_SIZE); 
    if(NULL == memBase) { 
        printf("Error allocating memory for AHCI initialization \n");
        return -1;  /* should reconcile these with std error numbers */
    }
    
    memset(memBase, 0, AHCI_INIT_MEM_SIZE);
    commandList = (void *) ((((uint32_t)memBase + 1024) >> 10) << 10);           /* go forward to a 1K boundary */
    receivedFIS = commandList + 1024;
    ahciSave = receivedFIS + 256;
    
    ahciSave->originalCR = port->CMD.CR;
    ahciSave->originalFR = port->CMD.FR;
    
//    printf("Stopping the PORT CMD.ST = %01x \n",port->CMD.ST);
/* Stop activity on the port so we can reconfigure the memory*/
    port->CMD.ST = 0;   /* stop the port */
//    printf("Waiting on CI\n");
    do {} while(port->CI !=0 );
    port->CMD.FRE = 0;
    do {} while (port->CMD.FR != 0);
    printf("Port stopped ST = %01x FRE = %01x CR = %01x FR = %01x \n",port->CMD.ST,port->CMD.FRE
            ,port->CMD.CR,port->CMD.FR);
//    printf("Port stopped assigning memory\n");
/*  Assign the new memory structures to the port */
    ahciSave->baseMemoryAddress = (uint32_t) memBase;
    ahciSave->originalCLB = port->CLB;
    ahciSave->originalCLBU = port->CLBU;
    ahciSave->originalFB = port->FB;
    ahciSave->originalFBU = port->FBU;
    port->CLB = (uint32_t) commandList;
    port->CLBU = 0;
    port->FB = (uint32_t) receivedFIS;
    port->FBU = 0;
//    printf("Restarting Port\n");
    port->CMD.FRE = 1;
    do {} while (port->CMD.FR != 1);
    port->CMD.ST = 1;
    do {} while (port->CMD.CR != 1);
    printf("Port started ST = %01x FRE = %01x CR = %01x FR = %01x \n",port->CMD.ST,port->CMD.FRE
            ,port->CMD.CR,port->CMD.FR);
    printf("CLB = %08x FB = %08x \n",port->CLB,port->FB);
    return 0;
} 
void ahci_restore(AHCI_PORT *port) {
    void *memBase;              /*< initial memory pointer before alignment */
    void *cmdList;              /*< CommandList  */
    AHCI_INIT_SAVE *ahciSave;    /*< Save area for status restore and free */
    
    /* Stop activity on the port so we can reconfigure the memory*/
    port->CMD.ST = 0;   /* stop the port */
//    printf("Waiting on CI\n");
    do {} while(port->CI !=0 );
    port->CMD.FRE = 0;
    do {} while (port->CMD.FR != 0);
/* restore port to original state */
    cmdList = (void *) port->CLB;
    ahciSave = cmdList + 1024 + 256;
    memBase = (void *) ahciSave->baseMemoryAddress; 
    port->CLB= ahciSave->originalCLB;
    port->CLBU = ahciSave->originalCLBU;
    port->FB = ahciSave->originalFB;
    port->FBU = ahciSave->originalFBU;
    
/* Restart port if required */
     if(ahciSave->originalFR) {
        port->CMD.FRE = 1;
        do {} while (port->CMD.FR != 1);
    }
    if(ahciSave->originalCR) {
        port->CMD.ST = 1;
        do {} while (port->CMD.CR != 1);
    }
    free(memBase);
}
