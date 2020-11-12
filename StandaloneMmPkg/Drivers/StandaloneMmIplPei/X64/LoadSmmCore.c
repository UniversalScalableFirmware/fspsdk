/** @file
  SMM IPL that load the SMM Core into SMRAM

  Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Pi/PiDxeCis.h>
#include <PiSmm.h>

typedef
EFI_STATUS
(EFIAPI *STANDALONE_SMM_FOUNDATION_ENTRY_POINT) (
  IN VOID  *HobStart
  );

EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  )
{
  STANDALONE_SMM_FOUNDATION_ENTRY_POINT         EntryPoint;

  EntryPoint = (STANDALONE_SMM_FOUNDATION_ENTRY_POINT)(UINTN)Entry;
  return EntryPoint (Context1);
}
