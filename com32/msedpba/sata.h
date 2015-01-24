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
#pragma once
#include <stdint.h>
#include "ahci.h"

#define ATACOMMAND_IF_RECV 0x5c
#define ATACOMMAND_IF_SEND 0x5e
#define ATACOMMAND_IDENTIFY 0xec

/*  
 * These structures and constants are defined in the 
 * SerialATA specification (rev 3.2 used) for this file 
 * ****************************************************
 * ***** Block diagrams in the SATA specification are
 * ***** in BIG EIDIAN format
 * ****************************************************
 */
/* 
 * Device signatures are apparently one of those things your just
 * supposed to know.  I found these via web searches
 */
#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

/* FIS types pp 505 */
#define FIS_TYPE_REGISTER_H2D 0x27
#define FIS_TYPE_REGISTER_D2H 0x34
#define FIS_TYPE_DMA_ACTIVATE_D2H 0x39
#define FIS_TYPE_DMA_SETUP_BIDIRECTIONAL 0x41
#define FIS_TYPE_DATA_BIDIRECTIONAL 0x46
#define FIS_TYPE_BIST_ACTIVATE_BIDIRECTIONAL 0x58
#define FIS_TYPE_PIO_SETUP_D2H 0x5F

/* FIS Register Host to Device pp 506*/
typedef struct _FIS_REGISTER_H2D {
	uint8_t	fis_type;	/*<  type = 0x27 */
	uint8_t	pmport:4;	/*<  Port multiplier */
	uint8_t	reserved0:3;    /*<  Reserved */
	uint8_t	c:1;		/*<  1: Command, 0: Control */
	uint8_t	command;	/*<  Command register */
	uint8_t	feature0;	/*<  Feature register, 7:0 */
	uint8_t	lba0;		/*<  LBA low register, 7:0 */
	uint8_t	lba1;		/*<  LBA mid register, 15:8 */
	uint8_t	lba2;		/*<  LBA high register, 23:16 */
	uint8_t	device;		/*<  Device register */
	uint8_t	lba3;		/*<  LBA register, 31:24 */
	uint8_t	lba4;		/*<  LBA register, 39:32 */
	uint8_t	lba5;		/*<  LBA register, 47:40 */
	uint8_t	feature1;	/*<  Feature register, 15:8 */
	uint8_t	count0;		/*<  Count register, 7:0 */
	uint8_t	count1;		/*<  Count register, 15:8 */
	uint8_t	icc;		/*<  Isochronous command completion */
	uint8_t	control;	/*<  Control register */
 	uint8_t	Auxillary[4];	/*<  Command specific Aux info */
    
} FIS_REGISTER_H2D;

int sataIOCtl(AHCI_PORT *port, uint8_t write, void * fis, size_t fislength, void *buffer, size_t buflength);
int sataIdentify(AHCI_PORT *port, void* buffer);
int sataOpen(AHCI_PORT *port);
void sataClose(AHCI_PORT *port);
int sataIFSEND(AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength);
int sataIFRECV(AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength);
int sataIFCMD(uint8_t cmd, AHCI_PORT *port, uint8_t protocol, uint16_t comid, void* buffer, size_t bufLength);
