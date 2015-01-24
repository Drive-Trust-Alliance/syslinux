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

/* code extracted from Msed  */
#include <stdlib.h>
#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include "msedpba.h"
#include "sata.h"

uint8_t sendCmd(AHCI_PORT *port, uint8_t ataCommand, uint8_t protocol, 
        uint16_t comid, void *buffer , size_t bufSize) {
    if(IF_RECV == ataCommand) 
        return sataIFRECV(port, protocol, comid, buffer, bufSize);
    else
        if(IF_SEND == ataCommand) 
            return sataIFSEND(port, protocol, comid, buffer, bufSize);
    return 0xff;
}
uint8_t exec(AHCI_PORT *port, void * cmd, void * response, uint8_t protocol, uint16_t comid)
{
    uint8_t rc = 0;
    rc = sendCmd(port,IF_SEND, protocol, comid, cmd, IO_BUFFER_LENGTH);
    if (0 != rc) {
        printf("Command failed on send %i",rc);
        return rc;
    }
   OPALHeader *hdr = (OPALHeader *) response;
    do {
        msleep(25);
        memset(response, 0, IO_BUFFER_LENGTH);
        rc = sendCmd(port, IF_RECV, protocol, comid, response, IO_BUFFER_LENGTH);
    }
    while ((0 != hdr->cp.outstandingData) && (0 == hdr->cp.minTransfer));
    if (0 != rc) {
        printf("Command failed on recv %i",rc);
        return rc;
    }
     return 0;
}
//void MsedBaseDev::discovery0()
void discovery0(AHCI_PORT *port, OPAL_DiskInfo *disk_info)
{
//    LOG(D1) << "Entering MsedBaseDev::discovery0()";
    printf("Entering Discovery 0 \n");
    void * d0Response = NULL;
    uint8_t * epos, *cpos;
    Discovery0Header * hdr;
    Discovery0Features * body;
    d0Response = ALIGNED_ALLOC(4096, IO_BUFFER_LENGTH);
    if (NULL == d0Response) return;
    memset(d0Response, 0, IO_BUFFER_LENGTH);
    if (sendCmd(port, IF_RECV, 0x01, 0x0001, d0Response, IO_BUFFER_LENGTH)) {
        ALIGNED_FREE(d0Response);
        return;
    }
    uint32_t *dump = (uint32_t *) d0Response;
    for (int i = 0; i < (int) sizeof (OPAL_DiskInfo) / 4; i += 4) {
        printf("%08x %08x %08x %08x \n", dump[0 + i], dump[1 + i], dump[2 + i], dump[3 + i]);
    }
    epos = cpos = (uint8_t *) d0Response;
    hdr = (Discovery0Header *) d0Response;
 //   LOG(D3) << "Dumping D0Response";
 //   IFLOG(D3) MsedHexDump(hdr, SWAP32(hdr->length));
    epos = epos + SWAP32(hdr->length);
    cpos = cpos + 48; // TODO: check header version

    do {
        body = (Discovery0Features *) cpos;
        switch (SWAP16(body->TPer.featureCode)) { /* could use of the structures here is a common field */
        case FC_TPER: /* TPer */
            disk_info->TPer = 1;
            disk_info->TPer_ACKNACK = body->TPer.acknack;
            disk_info->TPer_async = body->TPer.async;
            disk_info->TPer_bufferMgt = body->TPer.bufferManagement;
            disk_info->TPer_comIDMgt = body->TPer.comIDManagement;
            disk_info->TPer_streaming = body->TPer.streaming;
            disk_info->TPer_sync = body->TPer.sync;
            break;
        case FC_LOCKING: /* Locking*/
            disk_info->Locking = 1;
            disk_info->Locking_locked = body->locking.locked;
            disk_info->Locking_lockingEnabled = body->locking.lockingEnabled;
            disk_info->Locking_lockingSupported = body->locking.lockingSupported;
            disk_info->Locking_MBRDone = body->locking.MBRDone;
            disk_info->Locking_MBREnabled = body->locking.MBREnabled;
            disk_info->Locking_mediaEncrypt = body->locking.mediaEncryption;
            break;
        case FC_GEOMETRY: /* Geometry Features */
            disk_info->Geometry = 1;
            disk_info->Geometry_align = body->geometry.align;
            disk_info->Geometry_alignmentGranularity = SWAP64(body->geometry.alignmentGranularity);
            disk_info->Geometry_logicalBlockSize = SWAP32(body->geometry.logicalBlockSize);
            disk_info->Geometry_lowestAlignedLBA = SWAP64(body->geometry.lowestAlighedLBA);
            break;
        case FC_ENTERPRISE: /* Enterprise SSC */
            disk_info->Enterprise = 1;
			disk_info->ANY_OPAL_SSC = 1;
            disk_info->Enterprise_rangeCrossing = body->enterpriseSSC.rangeCrossing;
            disk_info->Enterprise_basecomID = SWAP16(body->enterpriseSSC.baseComID);
            disk_info->Enterprise_numcomID = SWAP16(body->enterpriseSSC.numberComIDs);
            break;
        case FC_OPALV100: /* Opal V1 */
            disk_info->OPAL10 = 1;
			disk_info->ANY_OPAL_SSC = 1;
            disk_info->OPAL10_basecomID = SWAP16(body->opalv100.baseComID);
            disk_info->OPAL10_numcomIDs = SWAP16(body->opalv100.numberComIDs);
            break;
        case FC_SINGLEUSER: /* Single User Mode */
            disk_info->SingleUser = 1;
            disk_info->SingleUser_all = body->singleUserMode.all;
            disk_info->SingleUser_any = body->singleUserMode.any;
            disk_info->SingleUser_policy = body->singleUserMode.policy;
            disk_info->SingleUser_lockingObjects = SWAP32(body->singleUserMode.numberLockingObjects);
            break;
        case FC_DATASTORE: /* Datastore Tables */
            disk_info->DataStore = 1;
            disk_info->DataStore_maxTables = SWAP16(body->datastore.maxTables);
            disk_info->DataStore_maxTableSize = SWAP32(body->datastore.maxSizeTables);
            disk_info->DataStore_alignment = SWAP32(body->datastore.tableSizeAlignment);
            break;
        case FC_OPALV200: /* OPAL V200 */
            disk_info->OPAL20 = 1;
			disk_info->ANY_OPAL_SSC = 1;
            disk_info->OPAL20_basecomID = SWAP16(body->opalv200.baseCommID);
            disk_info->OPAL20_initialPIN = body->opalv200.initialPIN;
            disk_info->OPAL20_revertedPIN = body->opalv200.revertedPIN;
            disk_info->OPAL20_numcomIDs = SWAP16(body->opalv200.numCommIDs);
            disk_info->OPAL20_numAdmins = SWAP16(body->opalv200.numlockingAdminAuth);
            disk_info->OPAL20_numUsers = SWAP16(body->opalv200.numlockingUserAuth);
            disk_info->OPAL20_rangeCrossing = body->opalv200.rangeCrossing;
            break;
        default:
            disk_info->Unknown += 1;
   //         LOG(D) << "Unknown Feature in Discovery 0 response " << std::hex << SWAP16(body->TPer.featureCode) << std::dec;
            /* should do something here */
            break;
        }
        cpos = cpos + (body->TPer.length + 4);
    }
    while (cpos < epos);
    ALIGNED_FREE(d0Response);
}
