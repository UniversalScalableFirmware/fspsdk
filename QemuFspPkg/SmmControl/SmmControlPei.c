/** @file

  A PEIM providing synchronous SMI activations via the
  EFI_SMM_CONTROL_PPI.

  Copyright (C) 2013, 2015, Red Hat, Inc.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
//#include <Library/QemuFwCfgLib.h>
//#include <Library/QemuFwCfgS3Lib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/SmmControl.h>

#include "Q35MchIch9.h"

//
// The absolute IO port address of the SMI Control and Enable Register. It is
// only used to carry information from the entry point function to the
// S3SaveState protocol installation callback, strictly before the runtime
// phase.
//
STATIC UINTN mSmiEnable;

/**
  Invokes SMI activation from either the preboot or runtime environment.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  This                  The PEI_SMM_CONTROL_PPI instance.
  @param  ArgumentBuffer        The optional sized data to pass into the protocol activation.
  @param  ArgumentBufferSize    The optional size of the data.
  @param  Periodic              An optional mechanism to periodically repeat activation.
  @param  ActivationInterval    An optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The timing is unsupported.
  @retval EFI_INVALID_PARAMETER The activation period is unsupported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.
**/
STATIC
EFI_STATUS
EFIAPI
SmmControlPeiTrigger (
  IN EFI_PEI_SERVICES                                **PeiServices,
  IN PEI_SMM_CONTROL_PPI                             * This,
  IN OUT INT8                                        *ArgumentBuffer OPTIONAL,
  IN OUT UINTN                                       *ArgumentBufferSize OPTIONAL,
  IN BOOLEAN                                         Periodic OPTIONAL,
  IN UINTN                                           ActivationInterval OPTIONAL
  )
{
  UINT8       Data;

  //
  // No support for queued or periodic activation.
  //
  if (Periodic || ActivationInterval > 0) {
    return EFI_DEVICE_ERROR;
  }

  if (ArgumentBuffer == NULL) {
    Data = 0xFF;
  } else {
    if (ArgumentBufferSize == NULL || *ArgumentBufferSize != 1) {
      return EFI_INVALID_PARAMETER;
    }

    Data = *ArgumentBuffer;
  }

  //
  // The so-called "Advanced Power Management Status Port Register" is in fact
  // a generic data passing register, between the caller and the SMI
  // dispatcher. The ICH9 spec calls it "scratchpad register" --  calling it
  // "status" elsewhere seems quite the misnomer. Status registers usually
  // report about hardware status, while this register is fully governed by
  // software.
  //
  // Write to the status register first, as this won't trigger the SMI just
  // yet. Then write to the control register.
  //
  IoWrite8 (ICH9_APM_STS, 0);
  IoWrite8 (ICH9_APM_CNT, Data);
  return EFI_SUCCESS;
}

/**
  Clears any system state that was created in response to the Active call.

  @param  PeiServices           General purpose services available to every PEIM.
  @param  This                  The PEI_SMM_CONTROL_PPI instance.
  @param  Periodic              Optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.
**/
STATIC
EFI_STATUS
EFIAPI
SmmControlPeiClear (
  IN EFI_PEI_SERVICES                      **PeiServices,
  IN PEI_SMM_CONTROL_PPI                   * This,
  IN BOOLEAN                               Periodic OPTIONAL
  )
{
  if (Periodic) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The PI spec v1.4 explains that Clear() is only supposed to clear software
  // status; it is not in fact responsible for deasserting the SMI. It gives
  // two reasons for this: (a) many boards clear the SMI automatically when
  // entering SMM, (b) if Clear() actually deasserted the SMI, then it could
  // incorrectly suppress an SMI that was asynchronously asserted between the
  // last return of the SMI handler and the call made to Clear().
  //
  // In fact QEMU automatically deasserts CPU_INTERRUPT_SMI in:
  // - x86_cpu_exec_interrupt() [target-i386/seg_helper.c], and
  // - kvm_arch_pre_run() [target-i386/kvm.c].
  //
  // So, nothing to do here.
  //
  return EFI_SUCCESS;
}

STATIC PEI_SMM_CONTROL_PPI mControl = {
  &SmmControlPeiTrigger,
  &SmmControlPeiClear,
};

STATIC EFI_PEI_PPI_DESCRIPTOR mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gPeiSmmControlPpiGuid, &mControl
  }
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
SmmControlPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINT32     PmBase;
  UINT32     SmiEnableVal;
  EFI_STATUS Status;

  //
  // This module should only be included if SMRAM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  //
  // Calculate the absolute IO port address of the SMI Control and Enable
  // Register. (As noted at the top, the PEI phase has left us with a working
  // ACPI PM IO space.)
  //
  PmBase = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) &
    ICH9_PMBASE_MASK;
  mSmiEnable = PmBase + ICH9_PMBASE_OFS_SMI_EN;

  //
  // If APMC_EN is pre-set in SMI_EN, that's QEMU's way to tell us that SMI
  // support is not available. (For example due to KVM lacking it.) Otherwise,
  // this bit is clear after each reset.
  //
  SmiEnableVal = IoRead32 (mSmiEnable);
  if ((SmiEnableVal & ICH9_SMI_EN_APMC_EN) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: this Q35 implementation lacks SMI\n",
      __FUNCTION__));
    goto FatalError;
  }

  //
  // Otherwise, configure the board to inject an SMI when ICH9_APM_CNT is
  // written to. (See the Trigger() method above.)
  //
  SmiEnableVal |= ICH9_SMI_EN_APMC_EN | ICH9_SMI_EN_GBL_SMI_EN;
  IoWrite32 (mSmiEnable, SmiEnableVal);

  //
  // Prevent software from undoing the above (until platform reset).
  //
  PciOr16 (POWER_MGMT_REGISTER_Q35 (ICH9_GEN_PMCON_1),
    ICH9_GEN_PMCON_1_SMI_LOCK);

  //
  // If we can clear GBL_SMI_EN now, that means QEMU's SMI support is not
  // appropriate.
  //
  IoWrite32 (mSmiEnable, SmiEnableVal & ~(UINT32)ICH9_SMI_EN_GBL_SMI_EN);
  if (IoRead32 (mSmiEnable) != SmiEnableVal) {
    DEBUG ((EFI_D_ERROR, "%a: failed to lock down GBL_SMI_EN\n",
      __FUNCTION__));
    goto FatalError;
  }

  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;

FatalError:
  //
  // We really don't want to continue in this case.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}

