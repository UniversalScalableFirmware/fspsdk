/** @file
  FSP-M implementation related header file.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSPM_INIT_H_
#define _FSPM_INIT_H_

#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/PciCf8Lib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/FspCommonLib.h>
#include <Library/FspPlatformLib.h>
#include <Library/CacheLib.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Ppi/MasterBootMode.h>
#include <Guid/FspHeaderFile.h>
#include <FspmUpd.h>
#include "OvmfPlatforms.h"

//
// MRC Variable Attributes
//
#define MEM_TESTED_ATTR \
          (EFI_RESOURCE_ATTRIBUTE_PRESENT                 | \
           EFI_RESOURCE_ATTRIBUTE_INITIALIZED             | \
           EFI_RESOURCE_ATTRIBUTE_TESTED                  | \
           EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE             | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE       | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE)

#define MEM_NOT_TESTED_ATTR \
          (EFI_RESOURCE_ATTRIBUTE_PRESENT                 | \
           EFI_RESOURCE_ATTRIBUTE_INITIALIZED             | \
           EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE             | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE       | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
           EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE)

/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDescriptor,
  IN VOID                      *Ppi
  );

#endif
