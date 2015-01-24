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

#include "MsedEndianFixup.h"
#include "MsedStructures.h"
#include "sata.h"

// buffers aligned in sataIOCtl 
#define IO_BUFFER_LENGTH 2048
#define  ALIGNED_ALLOC(align, length) malloc(length);
#define  ALIGNED_FREE(x) free(x);
void discovery0(AHCI_PORT *port, OPAL_DiskInfo *disk_info);
uint8_t sendCmd(AHCI_PORT *port, uint8_t ataCommand, uint8_t protocol, 
        uint16_t comid, void *buffer , size_t bufSize);
uint8_t exec(AHCI_PORT *port, void * cmd, void * response, uint8_t protocol, uint16_t comid);
