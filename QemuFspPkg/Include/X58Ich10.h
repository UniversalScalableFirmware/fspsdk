/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 316966-002
  - Intel(R) datasheet 316972-004

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.   The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __X58_ICH10_H__
#define __X58_ICH10_H__

//
// Host Bridge Device ID (DID) value for Q35/MCH
//
#define INTEL_X58_MCH_DEVICE_ID 0x3400

#define REGISTER_X58_PCIEXBAR  PCI_CF8_LIB_ADDRESS (0xFF, 0, 1, 0x50)

//
// B/D/F/Type: 0/0x1f/0/PCI
//
#define POWER_MGMT_REGISTER_ICH10(Offset) \
  PCI_LIB_ADDRESS (0, 0x1f, 0, (Offset))

#define ICH10_PMBASE               0x40
#define ICH10_PMBASE_MASK            (BIT15 | BIT14 | BIT13 | BIT12 | BIT11 | \
                                     BIT10 | BIT9  | BIT8  | BIT7)

#define ICH10_ACPI_CNTL            0x44
#define ICH10_ACPI_CNTL_ACPI_EN      BIT7

#define ICH10_GEN_PMCON_1          0xA0
#define ICH10_GEN_PMCON_1_SMI_LOCK   BIT4

#define ICH10_RCBA                 0xF0
#define ICH10_RCBA_EN                BIT0

#define ICH10_PMBASE_IO            0x400

#define ICH10_ROOT_COMPLEX_BASE    0xFED1C000

#endif
