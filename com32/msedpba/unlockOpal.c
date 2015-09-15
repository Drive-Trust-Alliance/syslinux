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
#include "gc.h"
#include "unlockOpal.h"
#include "MsedStructures.h"
#include "trace.h"

uint8_t unlockOpal(AHCI_PORT *port, char * pass, OPAL_DiskInfo *disk_info) {
    uint8_t startSession[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58,
        0xf8,
        0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x02,
        0xf0,
        0x81, 0x69,
        0xa8, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x00, 0x02,
        0x01,
        0xf2, 0x00, 0xd0, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf3,
        0xf2, 0x03, 0xa8,
        0x00, 0x00, 0x00, 0x09, 0x00, 0x01, 0x00, 0x01,
        0xf3,
        0xf1,
        0xf9,
        0xf0, 0x00, 0x00, 0x00, 0xf1,
    };
    uint8_t unlock[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x4c, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28,
        0xf8,
        0xa8, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00, 0x01,
        0xa8, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x17,
        0xf0, 0xf2, 0x01, 0xf0,
        0xf2, 0x07, 0x00, 0xf3,
        0xf2, 0x08, 0x00, 0xf3,
        0xf1, 0xf3, 0xf1,
        0xf9,
        0xf0, 0x00, 0x00, 0x00, 0xf1,
    };
    uint8_t mbrDone[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x48, 0xff, 0xff, 0xfc, 0x52, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24,
        0xf8,
        0xa8, 0x00, 0x00, 0x08, 0x03, 0x00, 0x00, 0x00, 0x01,
        0xa8, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x17,
        0xf0, 0xf2, 0x01, 0xf0,
        0xf2, 0x02, 0x01, 0xf3,
        0xf1, 0xf3, 0xf1,
        0xf9,
        0xf0, 0x00, 0x00, 0x00, 0xf1,
    };
    uint8_t endSession[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x28, 0xff, 0xff, 0xfc, 0x01, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfa, 0x00, 0x00, 0x00,
    };
#pragma pack(push)
#pragma pack(1)

    typedef union _TCGINTDATA {
        uint8_t TCGUINT8;
        uint16_t TCGUINT16;
        uint32_t TCGUINT32;
    } TCGINTDATA;

    typedef struct _TCGUINT {
        uint8_t header;
        TCGINTDATA d;
    } TCGUINT;

    typedef struct _TCGSHORTCHAR {
        uint8_t header;
        char bytes[];
    } TCGSHORTCHAR;

    typedef struct _TCGSMEDCHAR {
        uint16_t header;
        char bytes[];
    } TCGMEDCHAR;

    typedef struct _TCGTOKEN {
        uint8_t header;
        char bytes[];
    } TCGTOKEN;

    typedef struct _RESPONSE {

        union {
            TCGUINT i;
            TCGSHORTCHAR sc;
            TCGMEDCHAR mc;
            TCGTOKEN token;
        } data;
    } RESPONSE;
#pragma pack(pop)
    uint8_t cmd[IO_BUFFER_LENGTH];
    uint8_t resp[IO_BUFFER_LENGTH];
    uint16_t comid;
    uint32_t tsn;
    void *passpos;
    OPALHeader *pCmd;
    void *resppos;
    RESPONSE *pResp;
    char consoleBuffer[100];

    pCmd = (OPALHeader *) cmd;

    comid = disk_info->OPAL20 ? 
        disk_info->OPAL20_basecomID : disk_info->OPAL10_basecomID;

    // start a session
    memset(cmd, 0, IO_BUFFER_LENGTH);
    memcpy(cmd, startSession, sizeof (startSession));

    pCmd->cp.extendedComID[0] = ((comid & 0xff00) >> 8);
    pCmd->cp.extendedComID[1] = (comid & 0x00ff);
    /* hash the password and place it in the command */
    passpos = (void *) cmd;
    passpos += 0x5c;
    gc_pbkdf2_sha1(pass, strnlen(pass, 256), (char *) disk_info->serialNum, 
            20, 75000, passpos, 32);
    uint8_t *dump = (uint8_t *) cmd;
    TRACE{
        printf("command:");
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n%04x ", i);
            if (!(i % 4)) printf(" ");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    if (exec(port, (void *) cmd, (void *) resp, 0x01, comid)) {
        printf("start session failed \n");
        return 10;
    }
    TRACE{
        printf("response:");
        dump = (uint8_t *) resp;
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n%04x ", i);
            if (!(i % 4)) printf(" ");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    // get the TSN from the response
    resppos = (void *) resp + sizeof (OPALHeader);
    pResp = (RESPONSE *) resppos;
    if (pResp->data.token.header != 0xf8) {
        TRACE printf("invalid start session response (1) \n");
        return 11;
    }
    resppos += (1 + 9 + 9);
    pResp = (RESPONSE *) resppos;
    if (pResp->data.token.header != 0xf0) {
        TRACE printf("invalid start session response (2) \n");
        return 12;
    }
    resppos += 1;
    pResp = (RESPONSE *) resppos;
    // first list member is HSN 
    if ((pResp->data.token.header & 0xf0) != 0x80) {
        TRACE printf("invalid start session response (3) \n");
        return 13;
    }
    resppos += ((pResp->data.token.header & 0x0f) + 1);
    pResp = (RESPONSE *) resppos;
    // Second list member is TSN 
    if ((pResp->data.token.header & 0xf0) != 0x80) {
        TRACE printf("invalid start session response (4) \n");
        return 14;
    }
    if ((pResp->data.token.header & 0x0f) == 1)
        tsn = (uint32_t)pResp->data.i.d.TCGUINT8;
    else
        if ((pResp->data.token.header & 0x0f) == 2)
        tsn = (uint32_t)SWAP16(pResp->data.i.d.TCGUINT16);
    else
        if ((pResp->data.token.header & 0x0f) == 4)
        tsn = SWAP32(pResp->data.i.d.TCGUINT32);
    else {
        TRACE printf("invalid start session response (5) \n");
        return 15;
    }
    // unlock the drive 
    memset(cmd, 0, IO_BUFFER_LENGTH);
    memcpy(cmd, unlock, sizeof (unlock));
    pCmd->cp.extendedComID[0] = ((comid & 0xff00) >> 8);
    pCmd->cp.extendedComID[1] = (comid & 0x00ff);
    pCmd->pkt.TSN = SWAP32(tsn);
    TRACE{
        dump = (uint8_t *) cmd;
        printf("command:");
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n%04x ", i);
            if (!(i % 4)) printf(" ");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    if (exec(port, (void *) cmd, (void *) resp, 0x01, comid)) {
        TRACE printf("unlock failed \n");
        return 20;
    }
    TRACE{
        printf("response:");
        dump = (uint8_t *) resp;
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n%04x ", i);
            if (!(i % 4)) printf(" ");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    resppos = (void *) resp + sizeof (OPALHeader);
    pResp = (RESPONSE *) resppos;
    if (pResp->data.token.header != 0xf0) {
        TRACE printf("invalid unlock response (1) \n");
        return 21;
    }
    if (pResp->data.token.bytes[3] != 0x00) {
        TRACE printf("invalid unlock response method status %i \n", 
                (uint16_t) pResp->data.token.bytes[3]);
        return 22;
    }
    if (disk_info->Locking_MBREnabled) {
        // set mbrDone  
        memset(cmd, 0, IO_BUFFER_LENGTH);
        memcpy(cmd, mbrDone, sizeof (mbrDone));
        pCmd->cp.extendedComID[0] = ((comid & 0xff00) >> 8);
        pCmd->cp.extendedComID[1] = (comid & 0x00ff);
        pCmd->pkt.TSN = SWAP32(tsn);
        TRACE{
            dump = (uint8_t *) cmd;
            printf("command:");
            for (int i = 0; i < 0x100; i++) {
                if (!(i % 32)) printf("\n%04x ", i);
                if (!(i % 4)) printf(" ");
                printf("%02x", dump[i]);
            }
            fgets(consoleBuffer, sizeof consoleBuffer, stdin);
        }
        if (exec(port, (void *) cmd, (void *) resp, 0x01, comid)) {
            TRACE printf("mbrDone failed \n");
            return 30;
        }
        TRACE{
            printf("response:");
            dump = (uint8_t *) resp;
            for (int i = 0; i < 0x100; i++) {
                if (!(i % 32)) printf("\n%04x ", i);
                if (!(i % 4)) printf(" ");
                printf("%02x", dump[i]);
            }
            fgets(consoleBuffer, sizeof consoleBuffer, stdin);
        }
        resppos = (void *) resp + sizeof (OPALHeader);
        pResp = (RESPONSE *) resppos;
        if (pResp->data.token.header != 0xf0) {
            TRACE printf("invalid mbrDone response (1) \n");
            return 32;
        }
        if (pResp->data.token.bytes[3] != 0x00) {
            TRACE printf("invalid mbrDone response method status %i \n", 
                    (uint16_t) pResp->data.token.bytes[3]);
            return 32;
        }
    }
    // end the session
    memset(cmd, 0, IO_BUFFER_LENGTH);
    memcpy(cmd, endSession, sizeof (endSession));
    pCmd->cp.extendedComID[0] = ((comid & 0xff00) >> 8);
    pCmd->cp.extendedComID[1] = (comid & 0x00ff);
    pCmd->pkt.TSN = SWAP32(tsn);
    TRACE{
        dump = (uint8_t *) cmd;
        printf("command:");
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    if (exec(port, (void *) cmd, (void *) resp, 0x01, comid)) {
        printf("endSession failed \n");
        return 40;
    }
    TRACE{
        printf("response:");
        dump = (uint8_t *) resp;
        for (int i = 0; i < 0x100; i++) {
            if (!(i % 32)) printf("\n");
            printf("%02x", dump[i]);
        }
        fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    }
    resppos = (void *) resp + sizeof (OPALHeader);
    pResp = (RESPONSE *) resppos;
    if (pResp->data.token.header != 0xfa) {
        TRACE printf("invalid endSession response (1) \n");
        return 41;
    }
    TRACE fgets(consoleBuffer, sizeof consoleBuffer, stdin);
    return 0;
}
