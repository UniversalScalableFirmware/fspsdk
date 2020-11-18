/** @file
  FSP-I component implementation.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FspSwitchStackLib.h>

#include <Library/FspCommonLib.h>

CONST EFI_PEI_PPI_DESCRIPTOR gFspSiliconInitDonePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gFspSiliconInitDoneGuid,
  NULL
};


/**
  FSP-I Init 

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
FspiInitEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                   Status;
  
  //
  // Give control back after SMM Init
  //
  DEBUG ((DEBUG_INFO, "FSP-SMM Init Done\n"));
  Pei2LoaderSwitchStack ();

  // 
  // Install gFspSiliconInitDonePpi to dispatch FspNotifyPhasePeim
  //  
  DEBUG ((DEBUG_INFO, "Installing gFspSiliconInitDonePpi\n"));
  Status = PeiServicesInstallPpi (&gFspSiliconInitDonePpi);
  ASSERT_EFI_ERROR (Status);
  

  return EFI_SUCCESS;
}
