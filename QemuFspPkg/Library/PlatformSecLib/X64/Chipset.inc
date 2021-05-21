;; @file
; Chipset constants and macros
;
; Copyright (c) 2017-2018 Intel Corporation.
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------
;;

;
; APIC register
;
APICID                        EQU 0FEE00020h

;
; base addresses and register definitions.
;

B_ACPI_BASE_EN      EQU BIT1
  R_ACPI_PM1_CNT          EQU 004h
  V_ACPI_PM1_CNT_S5       EQU 01C00h
  R_ACPI_PM1_TMR          EQU 008h
  R_TCO_STS               EQU 064h
  B_TCO_STS_SECOND_TO     EQU BIT17
  R_TCO_CNT               EQU 068h
  B_TCO_CNT_TMR_HLT       EQU BIT11

  B_SMBUS_PCICMD_IOSE     EQU BIT0
  R_SMBUS_AUXC            EQU 00Dh

ACPI_BASE_ADDRESS         EQU 0400h
SPI_BASE_ADDRESS          EQU 0FED01000h
;  B_LPC_SPI_BASE_EN       EQU BIT1
  R_SPI_BCR               EQU 0DCh
  R_PMC_GEN_PMCON_1       EQU 01020h
  R_PMC_PMIR              EQU 01048h
  B_PMC_PMIR_CF9GR        EQU BIT20

  B_LPC_PUNIT_BASE_EN     EQU BIT1

  B_LPC_ILB_BASE_EN       EQU BIT1

  B_LPC_RCBA_EN           EQU BIT0

  B_LPC_IO_BASE_EN        EQU BIT1

  B_LPC_MPHY_BASE_EN      EQU BIT1

PCH_RCBA_BASE_ADDRESS     EQU 0FED1C000h

MCH_BASE_ADDRESS              EQU 0FED10000h
  B_MCH_BASE_ADDRESS_EN       EQU BIT0

;
;B_Unit Registers
;
BUNIT_BMISC                   EQU 6800h
  B_BMISC_RESDRAM             EQU 01h   ;Bit 0 - When this bit is set, reads targeting E-segment are routed to DRAM.
  B_BMISC_RFSDRAM             EQU 02h   ;Bit 1 - When this bit is set, reads targeting F-segment are routed to DRAM.

;
; HPET compare register
;
HPET_COMP_1                   EQU 0FED00108h
HPET_COMP_2                   EQU 0FED0010Ch
HPET_COMP_3                   EQU 0FED00128h
HPET_COMP_4                   EQU 0FED00148h
HPTC_AE                       EQU BIT7
;
; MCH PCIe base address
;
CPU_HEC_SIZE                  EQU 000000000h      ; 256M
CPU_HEC_EN                    EQU 000000001h      ; Enable
CPU0_HEC_PCI_ADDR             EQU 080FF0150h
CPU1_HEC_PCI_ADDR             EQU 080FE0150h
CPU_HEC_BASE                  EQU 0E0000000h

;
; PCI registers
;
R_MCH_BASE                EQU (( 0h * 8 + 00h) * 1000h + 0048h)
R_LPC_ACPI_BASE           EQU ((1Fh * 8 + 00h) * 1000h + 0040h)
R_LPC_PMC_BASE            EQU ((1Fh * 8 + 00h) * 1000h + 0044h)
R_LPC_IO_BASE             EQU ((1Fh * 8 + 00h) * 1000h + 004Ch)
R_LPC_ILB_BASE            EQU ((1Fh * 8 + 00h) * 1000h + 0050h)
R_LPC_SPI_BASE            EQU ((1Fh * 8 + 00h) * 1000h + 0054h)
R_LPC_MPHY_BASE           EQU ((1Fh * 8 + 00h) * 1000h + 0058h)
R_LPC_PUNIT_BASE          EQU ((1Fh * 8 + 00h) * 1000h + 005Ch)
R_LPC_RCBA                EQU ((1Fh * 8 + 00h) * 1000h + 00F0h)
R_SMBUS_CMD               EQU ((1Fh * 8 + 03h) * 1000h + 0004h)
R_SMBUS_BASE              EQU ((1Fh * 8 + 03h) * 1000h + 0020h)
R_P2SB_PCI_CMD            EQU ((0Dh * 8 + 00h) * 1000h + 0004h)
R_P2SB_MMIO_BAR0          EQU ((0Dh * 8 + 00h) * 1000h + 0010h)
R_P2SB_HPTC               EQU ((0Dh * 8 + 00h) * 1000h + 0060h)  ;HPTC config register. B0:D13:F0 0x60x
R_PMC_PCI_CMD             EQU ((0Dh * 8 + 01h) * 1000h + 0004h)
R_PMC_ACPI_BASE           EQU ((0Dh * 8 + 01h) * 1000h + 0020h)  ;BAR2 IO config register. B0:D13:F1 0x20
R_PMC_MMIO_BAR0           EQU ((0Dh * 8 + 01h) * 1000h + 0010h)  ;BAR0 IO config register. B0:D13:F1 0x10
R_PMC_MMIO_BAR1           EQU ((0Dh * 8 + 01h) * 1000h + 0018h)  ;BAR1 IO config register. B0:D13:F1 0x18

SYRE_CPURST                   EQU 14

;
; PCIEXBAR constants for enable in bit [0]
;
ENABLE                        EQU 1

;
; PCIEXBAR constants for size in bit [2:1]
;
PCIEXBAR_64MB                 EQU 010b
PCIEXBAR_128MB                EQU 001b
PCIEXBAR_256MB                EQU 000b

MMCFG_BASE                    EQU CPU_HEC_BASE    ; 4GB-128MB
