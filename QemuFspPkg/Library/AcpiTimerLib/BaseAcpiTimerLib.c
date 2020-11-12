/** @file
  Provide constructor and GetTick for Base instance of ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#include "Q35MchIch9.h"

#define ICH9_PMBASE_VALUE 0x1800

//
// Common bits in same-purpose registers
//
#define PMBA_RTE BIT0

//
// Common IO ports relative to the Power Management Base Address
//
#define ACPI_TIMER_OFFSET 0x8

//
// Cached ACPI Timer IO Address
//
STATIC UINT32 mAcpiTimerIoAddr;

/**
  The constructor function caches the ACPI tick counter address, and,
  if necessary, enables ACPI IO space.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
AcpiTimerLibConstructor (
  VOID
  )
{
  UINTN Pmba;
  UINT32 PmbaAndVal;
  UINT32 PmbaOrVal;
  UINTN AcpiCtlReg;
  UINT8 AcpiEnBit;

  Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
  PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
  PmbaOrVal  = ICH9_PMBASE_VALUE;
  AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
  AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;

  //
  // Check to see if the Power Management Base Address is already enabled
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // If the Power Management Base Address is not programmed,
    // then program it now.
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

    //
    // Enable PMBA I/O port decodes
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

  mAcpiTimerIoAddr = (PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET;
  return RETURN_SUCCESS;
}

/**
  Internal function to read the current tick counter of ACPI.

  Read the current ACPI tick counter using the counter address cached
  by this instance's constructor.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  //
  //   Return the current ACPI timer value.
  //
  return IoRead32 (mAcpiTimerIoAddr);
}
