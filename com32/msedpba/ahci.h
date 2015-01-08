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
#pragma pack(push)
#pragma pack(1)

/** Structures for accessing AHCI devices
 * See the AHCI specification v1.3 
 * 
 * LITTLE ENDIAN ONLY
 */
/* 
 * AHCI port control memory 
 * Defined in the AHCI 1.3 specification on pp 21
 * 
 *  *************** LITTLE ENDIAN LAYOUT ***********
 */
/* 
 * structures to define bits of a AHCI PORT registers
 */
typedef struct _PxIS {
    uint32_t DHRS :1; /*< Device to Host Register FIS Interrupt */
    uint32_t PSS :1; /*< PIO Setup FIS Interrupt */
    uint32_t DSS :1; /*< DMA Setup FIS Interrupt */
    uint32_t SDBSS :1; /*< Set Device Bits Interrupt */
    uint32_t UFS :1; /*< Unknown FIS Interrupt */
    uint32_t DPS :1; /*< Descriptor Processed */
    uint32_t PCS :1; /*< Port Connect Change Status */
    uint32_t DMPS :1; /*< Device Mechanical Presence Status */
    uint32_t reserved_08_21 :14; /*<  21:08 Reserved */
    uint32_t PRCS :1; /*< PhyRdy Change Status */
    uint32_t IPMS :1; /*< Incorrect Port Multiplier Status */
    uint32_t OFS :1; /*< Overflow Status */
    uint32_t reserved_25 :1; /*< Reserved */
    uint32_t INFS :1; /*< Interface Non-fatal Error Status */
    uint32_t IFS :1; /*<  Interface Fatal Error Status */
    uint32_t HBDS :1; /*< Host Bus Data Error Status */
    uint32_t HBFS :1; /*< Host Bus Fatal Error Status */
    uint32_t TFES :1; /*< Task File Error Status */
    uint32_t CPDS :1; /*<  Cold Port Detect Status */
} PxIS;
typedef struct _PxIE {
    uint32_t DHRE :1; /*< Device to Host Register FIS Interrupt Enable */
    uint32_t PSE :1; /*< PIO Setup FIS Interrupt Enable */
    uint32_t DSE :1; /*< DMA Setup FIS Interrupt Enable */
    uint32_t SDBE :1; /*< Set Device Bits FIS Interrupt Enable */
    uint32_t UFE :1; /*< Unknown FIS Interrupt Enable */
    uint32_t DPE :1; /*< Descriptor Processed Interrupt Enable */
    uint32_t PCE :1; /*< Port Change Interrupt Enable */
    uint32_t DMPE :1; /*< Device Mechanical Presence Enable */
    uint32_t reserved_08_21 :14; /*< Reserved */
    uint32_t PRCE :1; /*< PhyRdy Change Interrupt Enable */
    uint32_t IPME :1; /*< Incorrect Port Multiplier Enable */
    uint32_t OFE :1; /*< Overflow Enable */
    uint32_t  reserves_25 :1; /*< Reserved */
    uint32_t INFE :1; /*< Interface Non-fatal Error Enable */
    uint32_t IFE :1; /*< Interface Fatal Error Enable */
    uint32_t HBDE :1; /*< Host Bus Data Error Enable */
    uint32_t HBFE :1; /*< Host Bus Fatal Error Enable */
    uint32_t TFEE :1; /*< Task File Error Enable  */
    uint32_t CPDE :1; /*< Cold Presence Detect Enable */
} PxIE;
typedef struct _PxCMD {
    uint32_t ST  :1; /*< Start */
    uint32_t SUD :1; /*< Spin-Up Device */
    uint32_t POD :1; /*< Power On Device */
    uint32_t CLO :1; /*< Command List Override */
    uint32_t FRE :1; /*< FIS Receive Enable */
    uint32_t reserved_05_07 :3; /*< Reserved */
    uint32_t CCS :5; /*< Current Command Slot */
    uint32_t MPSS :1; /*< Mechanical Presence Switch State */
    uint32_t FR :1; /*< FIS Receive Running */
    uint32_t CR :1; /*< Command List Running */
    uint32_t CPS :1; /*< Cold Presence State */
    uint32_t PMA :1; /*< Port Multiplier Attached */
    uint32_t HPCP :1; /*< Hot Plug Capable Port */
    uint32_t MPSP :1; /*< Mechanical Presence Switch Attached to Port */
    uint32_t CPD :1; /*< Cold Presence Detection */
    uint32_t ESP :1; /*< External SATA Port */
    uint32_t FBSCP :1; /*< FIS-based Switching Capable Port */
    uint32_t APSTE :1; /*< Automatic Partial to Slumber Transitions Enabled */
    uint32_t ATAPI :1; /*< Device is ATAPI */
    uint32_t DLAE :1; /*< Drive LED on ATAPI Enable */
    uint32_t ALPE :1; /*< Aggressive Link Power Management Enable */
    uint32_t ASP :1; /*< Aggressive Slumber / Partial */
    uint32_t ICC :4; /*< Interface Communication Control */

} PxCMD;
typedef struct _PxTFD { 
/* Status (STS) */
    uint32_t  STSERR :1; /*< error during the transfer */
    uint32_t STSDRQcs :2; /*< Command specific */
    uint32_t STSDRQ :1; /*< data transfer is requested */
    uint32_t STSBSYcs :3; /*< Command specific */
    uint32_t STSBSY :1; /*< interface is busy */
     uint32_t ERR :8; /*< latest copy of the task file error register */
    uint32_t reserved_16_31 :16; /*< Reserved */
} PxTFD;
#define AHCI_PORT_DET_NOTPRESENT 0
#define AHCI_PORT_DET_PRESENT 1
#define AHCI_PORT_DET_PHYCOM 2
#define AHCI_PORT_DET_READY 3
#define AHCI_PORT_DET_OFFLINE 4
#define AHCI_PORT_SPD_NOTPRESENT 0
#define AHCI_PORT_SPD_GEN1 1
#define AHCI_PORT_SPD_GEN2 2
#define AHCI_PORT_SPD_GEN3 3
#define AHCI_PORT_IPM_NOTPRESENT 0
#define AHCI_PORT_IPM_ACTIVE 1
#define AHCI_PORT_IPM_PARTIALPM 2
#define AHCI_PORT_IPM_SLUMBER 6
typedef struct _PxSSTS {
    uint32_t DET :4; /*< Device Detection */
    uint32_t SPD :4; /*< Current Interface Speed */
    uint32_t IPM :4; /*< Interface Power Management */
    uint32_t reserved_12_31 :20; /*< Reserved */
} PxSSTS; 
/** 
 * AHCI Port register definition 
 */
typedef volatile struct _AHCI_PORT
{
    uint32_t CBL;     /*< Port x Command List Base Address */
    uint32_t CBLU;    /*< Port x Command List Base Address Upper 32-Bits*/
    uint32_t FB;       /*< Port x FIS Base Address */
    uint32_t FBU;      /*< Port x FIS Base Address Upper 32-Bits */
    PxIS     IS;      /*< Port x Interrupt Status */
    PxIE     IE;      /*< Port x Interrupt Enable */
    PxCMD    CMD;     /*< Port x Command and Status */
    uint32_t reserved_1c_1f;   /*< Reserved */
    PxTFD    TFD;     /*< Port x Task File Data */
    uint32_t SIG;     /*< Port x Signature */
    PxSSTS   SSTS;    /*< Port x Serial ATA Status (SCR0: SStatus) */
    uint32_t SCTL;    /*< Port x Serial ATA Control (SCR2: SControl) */
    uint32_t SERR;    /*< Port x Serial ATA Error (SCR1: SError) */
    uint32_t SACT;    /*< Port x Serial ATA Active (SCR3: SActive) */
    uint32_t CI;      /*< Port x Command Issue */
    uint32_t SNTF;    /*< Port x Serial ATA Notification (SCR4: SNotification) */
    uint32_t FBS;     /*< Port x FIS-based Switching Control */
    uint8_t  resserved_44_6f[0x70-0x44];    /*< Reserved */
    uint32_t VS[4];       /*< Port x Vendor Specific */
    
} AHCI_PORT;
/** AHCI global memory.  This structure is pointed to by BAR5 AKA ABAR
 * Defined in the AHCI 1.3.Specification pp 14
 *
 *  *****LITTLE ENDIAN LAYOUT********* 
 */
/*
 * Structures to access bit fields in AHCI Global memory 
 */
/** 
 * AHCI global memory register definition 
 */
typedef struct _AHCICAP {
    uint32_t NP         :5;  /*< Number of ports */
    uint32_t SXS        :1; /*< Supports External Sata */
    uint32_t EMS        :1; /*< Supports Enclisure Management */
    uint32_t CCCS       :1; /*<Command Completion Coalescing Supported */
    uint32_t NCS       :5; /*< Number of Command Slots*/
    uint32_t PSC       :1; /*< Partial State Capable*/
    uint32_t SSC       :1; /*< Slumber State Capable*/
    uint32_t PMD       :1; /*< PIO Multiple DRQ Block*/
    uint32_t FBSS       :1; /*< FIS-based Switching Supported*/
    uint32_t SPM       :1; /*< Supports Port Multiplier*/
    uint32_t SAM       :1; /*<Supports AHCI mode only */
    uint32_t res01     :1; /*< reserved */
    uint32_t ISS       :4; /*< Interface Speed Support*/
    uint32_t SCLO      :1; /*< Supports Command List Override*/
    uint32_t SAL       :1; /*< Supports Activity LED*/
    uint32_t SALP      :1; /*< Supports Aggressive Link Power Managemen*/
    uint32_t SSS       :1; /*< Supports Staggered Spin-up*/
    uint32_t SMPS      :1; /*< Supports Mechanical Presence Switch*/
    uint32_t SSNTF     :1; /*< Supports SNotification Register*/
    uint32_t SNCQ      :1; /*< Supports Native Command Queuing*/
    uint32_t S64a      :1; /*< Supports 64-bit Addressing*/
} AHCICAP;
typedef struct _AHCIGHC {
    uint32_t HR       :1; /*< HBA Reset */
    uint32_t IE       :1; /*< Interrupt Enable */
    uint32_t MRSM     :1; /*< MSI Revert to Single Message */
    uint32_t res      :28; /*< reserved */
    uint32_t AE       :1; /*< AHCI Enable */
} AHCIGHC;
typedef struct _AHCICCCCTL {
     uint16_t EN       :1; /*< Enable */
    uint16_t res03     :2; /*< reserved */
    uint16_t INT       :5; /*< Interrupt */
    uint16_t CC        :8; /*< Command Completion */
    uint16_t TV;            /*< Timeout Value */  
} AHCICCCCTL;
typedef struct _AHCICAP2 {
  uint32_t BOH       :1; /*< BIOS/OS Handoff */
    uint32_t NVMP       :1; /*< NVMHCI Present */
    uint32_t APST       :1; /*< Automatic Partial to Slumber Transitions */
    uint32_t res04       :29 ; /*< Reserved */  
} AHCICAP2;
typedef volatile struct _AHCI_GLOBAL
{
    AHCICAP CAP;  /*< AHCI Host Capabilities */
    AHCIGHC GHC;  /*< AHCO Global Host Control */
    uint32_t IS;         /*< Interupt Status */
    uint32_t PI;         /*< Ports implememted */
    uint32_t VS;         /*< AHCI Version */
    AHCICCCCTL CCC_CTL;  /*< Command Completion Coalescing Control */
    uint32_t CCC_PORTS; /*< Command Completion Coalescing Ports */
    uint16_t OFST;  /*< Enclosure Management Location offset */
    uint16_t SZ; /*< Enclosure Management Location buffer size*/
    uint32_t EM_CTL; /*< Enclosure Management Control */
    AHCICAP2 CAP2;   /*< Capabilities Extended */
    uint32_t BOHC;      /*< BIOS/OS Handoff Control and Status */
    uint8_t reserved_2c_5f[0x60-0x2c];  /* reserved */
    uint8_t reserved_60_9f[0xa0-0x60]; /*< reserved for NVMHCI */
    uint8_t reserved_a0_ff[0x100-0xa0]; /*< Vendor specific registers */
    AHCI_PORT PORT[];  /* AHCI port registers */
    
} AHCI_GLOBAL;