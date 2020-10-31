/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <FsprInit.h>

/**
  This function retrieves a special reserved memory region.

  @param  HobListPtr   A HOB list pointer.
  @param  Length       A pointer to the GUID HOB data buffer length.  If the GUID HOB is
                       located, the length will be updated.
  @param  OwnerGuid    A pointer to the owner guild.
  @retval              Reserved region start address.  0 if this region does not exist.

**/
UINT64
EFIAPI
GetFspReservedMemoryFromGuid (
  CONST VOID     *HobListPtr,
  UINT64         *Length,
  EFI_GUID       *OwnerGuid
  )
{
  EFI_PEI_HOB_POINTERS    Hob;

  /*
   * Get the HOB list for processing
   */
  Hob.Raw = (VOID *)HobListPtr;

  /*
   * Collect memory ranges
   */
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_RESERVED) {
        if (CompareGuid (&Hob.ResourceDescriptor->Owner, OwnerGuid)) {
          if (Length != 0) {
            *Length = (UINT32) (Hob.ResourceDescriptor->ResourceLength);
          }
          return (UINT64) (Hob.ResourceDescriptor->PhysicalStart);
        }
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return 0;
}

/**
  Search for the saved MrcParam to initialize Memory for fastboot.

  @retval Found MrcParam or NULL
**/
VOID *
EFIAPI
FindNvsData (
  VOID
  )
{
  return NULL;
}

/**
  This FSP API is called after TempRamInit and initializes the memory.
  This FSP API accepts a pointer to a data structure that will be platform dependent
  and defined for each FSP binary. This will be documented in Integration guide with
  each FSP release.
  After FspMemInit completes its execution, it passes the pointer to the HobList and
  returns to the boot loader from where it was called. BootLoader is responsible to
  migrate it's stack and data to Memory.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to
  complete the silicon initialization and provides bootloader an opportunity to get
  control after system memory is available and before the temporary RAM is torn down.

  @param[in] FspmBase                 The base address of FSPM.
  @param[out] HobListPtr              Pointer to receive the address of the HOB list.

  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval EFI_OUT_OF_RESOURCES        Stack range requested by FSP is not met.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
EFI_STATUS
EFIAPI
CallFspMemoryInit (
  UINT32                     FspmBase,
  VOID                       **HobList
  )
{
  UINT8                       FspmUpd[sizeof(FSPM_UPD)];
  UINT8                      *DefaultMemoryInitUpd;
  FSP_INFO_HEADER            *FspHeader;
  FSP_MEMORY_INIT             FspMemoryInit;
  FSPM_UPD_COMMON            *FspmUpdCommon;
  EFI_STATUS                  Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(FspmBase + FSP_INFO_HEADER_OFF);

  ASSERT (FspHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  ASSERT (FspHeader->ImageBase == FspmBase);

  // Copy default UPD data
  DefaultMemoryInitUpd = (UINT8 *)(UINTN)(FspHeader->ImageBase + FspHeader->CfgRegionOffset);
  CopyMem (&FspmUpd, DefaultMemoryInitUpd, FspHeader->CfgRegionSize);

  /* Update architectural UPD fields */
  FspmUpdCommon = (FSPM_UPD_COMMON *)FspmUpd;
  FspmUpdCommon->FspmArchUpd.BootLoaderTolumSize = 0;
  FspmUpdCommon->FspmArchUpd.BootMode            = BOOT_WITH_FULL_CONFIGURATION;
  FspmUpdCommon->FspmArchUpd.NvsBufferPtr        = 0;

  //UpdateFspConfig (FspmUpd);

  ASSERT (FspHeader->FspMemoryInitEntryOffset != 0);
  FspMemoryInit = (FSP_MEMORY_INIT)(UINTN)(FspHeader->ImageBase + \
                                           FspHeader->FspMemoryInitEntryOffset);

  DEBUG ((DEBUG_INFO, "Call FspMemoryInit ... "));
  Status = FspMemoryInit (&FspmUpd, HobList);
  DEBUG ((DEBUG_INFO, "%r\n", Status));

  return Status;
}

/**
  This FSP API is called after FspMemoryInit API. This FSP API tears down the temporary
  memory setup by TempRamInit API. This FSP API accepts a pointer to a data structure
  that will be platform dependent and defined for each FSP binary. This will be
  documented in Integration Guide.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to
  complete the silicon initialization and provides bootloader an opportunity to get
  control after system memory is available and before the temporary RAM is torn down.

  @param[in] FspmBase            The base address of FSPM.
  @param[in] Params              Pointer to the Temp Ram Exit parameters structure.
                                 This structure is normally defined in the Integration Guide.
                                 And if it is not defined in the Integration Guide, pass NULL.

  @retval EFI_SUCCESS            FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER  Input parameters are invalid.
  @retval EFI_UNSUPPORTED        The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR       FSP initialization failed.
**/
EFI_STATUS
EFIAPI
CallFspTempRamExit (
  UINT32               FspmBase,
  VOID                *Params
  )
{
  FSP_TEMP_RAM_EXIT   TempRamExit;
  FSP_INFO_HEADER    *FspHeader;
  EFI_STATUS          Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(FspmBase + FSP_INFO_HEADER_OFF);
  if (FspHeader->TempRamExitEntryOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  TempRamExit = (FSP_TEMP_RAM_EXIT)(UINTN)(FspHeader->ImageBase + FspHeader->TempRamExitEntryOffset);

  DEBUG ((DEBUG_INFO, "Call FspTempRamExit ... "));
  Status  = TempRamExit (NULL);
  DEBUG ((DEBUG_INFO, "%r\n", Status));

  return Status;
}


/**
  This FSP API is called after TempRamExit API.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to complete the
  silicon initialization.

  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
EFI_STATUS
EFIAPI
CallFspSiliconInit (
  VOID
  )
{
  UINT8                       FspsUpd[sizeof(FSPS_UPD)];
  UINT8                      *DefaultSiliconInitUpd;
  FSP_INFO_HEADER            *FspHeader;
  FSP_SILICON_INIT            FspSiliconInit;
  EFI_STATUS                  Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(mFspsBaseAddr + FSP_INFO_HEADER_OFF);

  ASSERT (FspHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  ASSERT (FspHeader->ImageBase == mFspsBaseAddr);

  // Copy default UPD data
  DefaultSiliconInitUpd = (UINT8 *)(UINTN)(FspHeader->ImageBase + FspHeader->CfgRegionOffset);
  CopyMem (FspsUpd, DefaultSiliconInitUpd, FspHeader->CfgRegionSize);

  /* Update architectural UPD fields */
  //UpdateFspConfig (FspsUpdptr);

  ASSERT (FspHeader->FspSiliconInitEntryOffset != 0);
  FspSiliconInit = (FSP_SILICON_INIT)(UINTN)(FspHeader->ImageBase + \
                                             FspHeader->FspSiliconInitEntryOffset);

  DEBUG ((DEBUG_INFO, "Call FspSiliconInit ... \n"));
  Status = FspSiliconInit (FspsUpd);
  DEBUG ((DEBUG_INFO, "%r\n", Status));

  return Status;
}

/**
  This FSP API is used to notify the FSP about the different phases in the boot process.
  This allows the FSP to take appropriate actions as needed during different initialization
  phases. The phases will be platform dependent and will be documented with the FSP
  release. The current FSP supports following notify phases:
    Post PCI enumeration
    Ready To Boot
    End of firmware

  @param[in] Phase              Phase parameter for FspNotifyPhase

  @retval EFI_SUCCESS           The notification was handled successfully.
  @retval EFI_UNSUPPORTED       The notification was not called in the proper order.
  @retval EFI_INVALID_PARAMETER The notification code is invalid.
**/
EFI_STATUS
EFIAPI
CallFspNotifyPhase (
  FSP_INIT_PHASE  Phase
  )
{
  FSP_INFO_HEADER            *FspHeader;
  FSP_NOTIFY_PHASE            NotifyPhase;
  NOTIFY_PHASE_PARAMS         NotifyPhaseParams;
  EFI_STATUS                  Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(mFspsBaseAddr + FSP_INFO_HEADER_OFF);

  ASSERT (FspHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  ASSERT (FspHeader->ImageBase == mFspsBaseAddr);

  if (FspHeader->NotifyPhaseEntryOffset == 0) {
    return EFI_UNSUPPORTED;
  }

  NotifyPhase = (FSP_NOTIFY_PHASE)(UINTN)(FspHeader->ImageBase +
                                          FspHeader->NotifyPhaseEntryOffset);

  NotifyPhaseParams.Phase = Phase;

  DEBUG ((DEBUG_INFO, "Call FspNotifyPhase(%02X) ... ", Phase));
  Status = NotifyPhase (&NotifyPhaseParams);
  DEBUG ((DEBUG_INFO, "%r\n", Status));

  return Status;
}
