/** @file
  FSP-S component implementation.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FspsInit.h"

static EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiEndOfPeiSignalPpiGuid,
    FspInitEndOfPeiCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gFspEventEndOfFirmwareGuid,
    FspEndOfFirmwareCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiEventReadyToBootGuid,
    FspReadyToBootCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiGraphicsPpiGuid,
    FspGfxInitCallback
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPciEnumerationCompleteProtocolGuid,
    FspInitAfterPciEnumerationCallback
  }
};


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
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_GRAPHICS_PPI        *GfxInitPpi;
  FSPS_UPD                    *FspsUpd;

  GfxInitPpi = (EFI_PEI_GRAPHICS_PPI *)Ppi;

  FspsUpd = GetFspSiliconInitUpdDataPointer ();

  ///
  /// Call PeiGraphicsPpi.GraphicsPpiInit to initilize the display
  ///
  DEBUG ((DEBUG_INFO, "GraphicsPpiInit Start\n"));
  Status = GfxInitPpi->GraphicsPpiInit ((VOID *)FspsUpd->FspsConfig.GraphicsConfigPtr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GraphicsPpiInit failed. \n"));
  }

  return Status;
}

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
  )
{
  // Place holder
  DEBUG ((DEBUG_INFO, "FspInitEndOfPeiCallback++\n"));

  DEBUG ((DEBUG_INFO, "FspInitEndOfPeiCallback--\n"));

  return EFI_SUCCESS;
}

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
  )
{
  // Place holder
  DEBUG ((DEBUG_INFO, "FspInitAfterPciEnumerationCallback++\n"));

  DEBUG ((DEBUG_INFO, "FspInitAfterPciEnumerationCallback--\n"));

  return EFI_SUCCESS;
}

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
  )
{
  // Place holder
  DEBUG ((DEBUG_INFO, "FspReadyToBootCallback++\n"));

  DEBUG ((DEBUG_INFO, "FspReadyToBootCallback--\n"));

  return EFI_SUCCESS;
}

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
  )
{
  // Place holder
  DEBUG ((DEBUG_INFO, "FspEndOfFirmwareCallback++\n"));

  DEBUG ((DEBUG_INFO, "FspEndOfFirmwareCallback--\n"));

  return EFI_SUCCESS;
}

/**
  FSP Init before memory PEI module entry point

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
FspsInitEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                   Status;
  FSPS_UPD                    *FspsUpd;
  FSP_INFO_HEADER             *FspInfoHeader;

  Status = EFI_SUCCESS;
  FspsUpd = NULL;

  DEBUG ((DEBUG_INFO, "FspInitEntryPoint() - start\n"));
  FspInfoHeader = GetFspInfoHeaderFromApiContext ();
  SetFspInfoHeader (FspInfoHeader);

  FspsUpd = (FSPS_UPD *)GetFspApiParameter();
  if (FspsUpd == NULL) {
    //
    // Use the UpdRegion as default
    //
    FspsUpd = (FSPS_UPD *) (FspInfoHeader->ImageBase + FspInfoHeader->CfgRegionOffset);
  }
  SetFspSiliconInitUpdDataPointer (FspsUpd);

  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "FspInitEntryPoint() - end\n"));
  return Status;
}
