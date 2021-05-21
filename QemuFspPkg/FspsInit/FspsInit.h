/** @file
  FSP-S implementation related header file.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_INIT_PEI_H_
#define _FSP_INIT_PEI_H_

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FspCommonLib.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/Graphics.h>
#include <FspmUpd.h>
#include <FspsUpd.h>

/**
  FSP initialization after PCI enumeration

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully

**/
EFI_STATUS
EFIAPI
FspInitAfterPciEnumerationCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

/**
  FSP initialization at the end of PEI

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully

**/
EFI_STATUS
EFIAPI
FspInitEndOfPeiCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

/**
  FSP initialization at ready to boot event

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully

**/
EFI_STATUS
EFIAPI
FspReadyToBootCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

/**
  FSP initialization at the end of firmware

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully

**/
EFI_STATUS
EFIAPI
FspEndOfFirmwareCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

/**
  FSP graphics initialization after PCI enumeration

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully

**/
EFI_STATUS
EFIAPI
FspGfxInitCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

#endif
