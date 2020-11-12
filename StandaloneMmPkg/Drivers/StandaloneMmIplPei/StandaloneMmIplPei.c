/** @file
  SMM IPL that load the SMM Core into SMRAM

  Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Pi/PiDxeCis.h>
#include <PiSmm.h>

#include <Ppi/SmmAccess.h>
#include <Ppi/SmmControl.h>
#include <Ppi/SmmCommunication.h>
#include <Protocol/SmmCommunication.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <Guid/EventGroup.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/MmCoreData.h>

EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  );

//
// SMM Core Private Data structure that contains the data shared between
// the SMM IPL and the SMM Core.
//
MM_CORE_PRIVATE_DATA  mMmCorePrivateData = {
  MM_CORE_PRIVATE_DATA_SIGNATURE,     // Signature
  0,                                  // MmramRangeCount
  0,                                  // MmramRanges
  0,                                  // MmEntryPoint
  FALSE,                              // MmEntryPointRegistered
  FALSE,                              // InMm
  0,                                  // Mmst
  0,                                  // CommunicationBuffer
  0,                                  // BufferSize
  EFI_SUCCESS,                        // ReturnStatus
  0,                                  // MmCoreImageBase
  0,                                  // MmCoreImageSize
  0,                                  // MmCoreEntryPoint
  0,                                  // StandaloneBfvAddress
};

//
// Global pointer used to access mMmCorePrivateData from outside and inside SMM
//
MM_CORE_PRIVATE_DATA  *gMmCorePrivate;

//
// SMM IPL global variables
//
EFI_SMRAM_DESCRIPTOR       *mCurrentSmramRange;
BOOLEAN                    mSmmLocked = FALSE;
EFI_PHYSICAL_ADDRESS       mSmramCacheBase;
UINT64                     mSmramCacheSize;

/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI   *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  );

EFI_PEI_SMM_COMMUNICATION_PPI      mSmmCommunicationPpi = { Communicate };

EFI_PEI_PPI_DESCRIPTOR mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiSmmCommunicationPpiGuid,
  &mSmmCommunicationPpi
};

/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered UEFI service.

  @param[in] This                The EFI_PEI_SMM_COMMUNICATION_PPI instance.
  @param[in, out] CommBuffer     A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize       The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
  @retval EFI_NOT_STARTED        The service is NOT started.
**/
EFI_STATUS
EFIAPI
Communicate (
  IN CONST EFI_PEI_SMM_COMMUNICATION_PPI   *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  )
{
  EFI_STATUS                       Status;
  PEI_SMM_CONTROL_PPI              *SmmControl;
  UINT8                            SmiCommand;
  UINTN                            Size;

  DEBUG ((EFI_D_INFO, "StandaloneSmmIpl Communicate Enter\n"));

  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // If not already in SMM, then generate a Software SMI
  //
  if (!gMmCorePrivate->InMm && gMmCorePrivate->MmEntryPointRegistered) {
    //
    // Put arguments for Software SMI in gMmCorePrivate
    //
    gMmCorePrivate->CommunicationBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer;
    gMmCorePrivate->BufferSize          = (UINT64)*CommSize;
    
    //
    // Generate Software SMI
    //
    Status = PeiServicesLocatePpi (&gPeiSmmControlPpiGuid, 0, NULL, (VOID **)&SmmControl);
    ASSERT_EFI_ERROR (Status);

    SmiCommand = 0;
    Size = sizeof(SmiCommand);
    Status = SmmControl->Trigger (
                           (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                           SmmControl,
                           (INT8 *)&SmiCommand,
                           &Size,
                           FALSE,
                           0
                           );
    ASSERT_EFI_ERROR (Status); 
    
    //
    // Return status from software SMI 
    //
    *CommSize = (UINTN)gMmCorePrivate->BufferSize;
    
    Status = (EFI_STATUS)gMmCorePrivate->ReturnStatus;
    if (Status != EFI_SUCCESS) {
      Status = Status | MAX_BIT;
    }
    
    DEBUG ((EFI_D_INFO, "StandaloneSmmIpl Communicate Exit (%r)\n", Status));

    return Status;
  }

  //
  // TBD - Need Mode Switch to call gMmCorePrivate->Smst->SmiManage
  //
  DEBUG ((EFI_D_INFO, "StandaloneSmmIpl Communicate Exit (%r)\n", EFI_UNSUPPORTED));

  return EFI_UNSUPPORTED;
}

/**
  Find the maximum SMRAM cache range that covers the range specified by SmramRange.
  
  This function searches and joins all adjacent ranges of SmramRange into a range to be cached.

  @param   SmramRange       The SMRAM range to search from.
  @param   SmramCacheBase   The returned cache range base.
  @param   SmramCacheSize   The returned cache range size.

**/
VOID
GetSmramCacheRange (
  IN  EFI_SMRAM_DESCRIPTOR *SmramRange,
  OUT EFI_PHYSICAL_ADDRESS *SmramCacheBase,
  OUT UINT64               *SmramCacheSize
  )
{
  UINTN                Index;
  EFI_PHYSICAL_ADDRESS RangeCpuStart;
  UINT64               RangePhysicalSize;
  BOOLEAN              FoundAjacentRange;
  EFI_SMRAM_DESCRIPTOR *SmramRanges;

  *SmramCacheBase = SmramRange->CpuStart;
  *SmramCacheSize = SmramRange->PhysicalSize;

  SmramRanges = (EFI_SMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
  do {
    FoundAjacentRange = FALSE;
    for (Index = 0; Index < gMmCorePrivate->MmramRangeCount; Index++) {
      RangeCpuStart     = SmramRanges[Index].CpuStart;
      RangePhysicalSize = SmramRanges[Index].PhysicalSize;
      if (RangeCpuStart < *SmramCacheBase && *SmramCacheBase == (RangeCpuStart + RangePhysicalSize)) {
        *SmramCacheBase   = RangeCpuStart;
        *SmramCacheSize  += RangePhysicalSize;
        FoundAjacentRange = TRUE;
      } else if ((*SmramCacheBase + *SmramCacheSize) == RangeCpuStart && RangePhysicalSize > 0) {
        *SmramCacheSize  += RangePhysicalSize;
        FoundAjacentRange = TRUE;
      }
    }
  } while (FoundAjacentRange);
  
}

VOID
FillSmmCoreInformation (
  IN EFI_PHYSICAL_ADDRESS  SmmCoreBase,
  IN UINT64                SmmCoreSize
  )
{
  EFI_SMRAM_DESCRIPTOR        *NewSmramRanges;

  //
  // Allocate new SmramRanges
  //
  NewSmramRanges = (EFI_SMRAM_DESCRIPTOR *)AllocateZeroPool (((UINTN)gMmCorePrivate->MmramRangeCount + 1) * sizeof (EFI_SMRAM_DESCRIPTOR));
  if (NewSmramRanges == NULL) {
    return ;
  }

  CopyMem (NewSmramRanges, (VOID *)(UINTN)gMmCorePrivate->MmramRanges, (UINTN)gMmCorePrivate->MmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR)); 

  NewSmramRanges[gMmCorePrivate->MmramRangeCount].PhysicalStart   = SmmCoreBase;
  NewSmramRanges[gMmCorePrivate->MmramRangeCount].CpuStart        = SmmCoreBase;
  NewSmramRanges[gMmCorePrivate->MmramRangeCount].PhysicalSize    = SmmCoreSize;
  NewSmramRanges[gMmCorePrivate->MmramRangeCount].RegionState     = EFI_ALLOCATED;

  FreePool ((VOID *)(UINTN)gMmCorePrivate->MmramRanges);
  gMmCorePrivate->MmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)NewSmramRanges;
  gMmCorePrivate->MmramRangeCount += 1;

  return;
}

/**
  Get the fixed loadding address from image header assigned by build tool. This function only be called
  when Loading module at Fixed address feature enabled.

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.
  @retval EFI_SUCCESS               An fixed loading address is assigned to this image by build tools .
  @retval EFI_NOT_FOUND             The image has no assigned fixed loadding address.
**/
EFI_STATUS
GetPeCoffImageFixLoadingAssignedAddress(
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
   UINTN                              SectionHeaderOffset;
   EFI_STATUS                         Status;
   EFI_IMAGE_SECTION_HEADER           SectionHeader;
   EFI_IMAGE_OPTIONAL_HEADER_UNION    *ImgHdr;
   EFI_PHYSICAL_ADDRESS               FixLoaddingAddress;
   UINT16                             Index;
   UINTN                              Size;
   UINT16                             NumberOfSections;
   EFI_PHYSICAL_ADDRESS               SmramBase;
   UINT64                             SmmCodeSize;
   UINT64                             ValueInSectionHeader;
   //
   // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
   //
   SmmCodeSize = EFI_PAGES_TO_SIZE (PcdGet32(PcdLoadFixAddressSmmCodePageNumber));
 
   FixLoaddingAddress = 0;
   Status = EFI_NOT_FOUND;
   SmramBase = mCurrentSmramRange->CpuStart;
   //
   // Get PeHeader pointer
   //
   ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((CHAR8* )ImageContext->Handle + ImageContext->PeCoffHeaderOffset);
   SectionHeaderOffset = (UINTN)(
                                 ImageContext->PeCoffHeaderOffset +
                                 sizeof (UINT32) +
                                 sizeof (EFI_IMAGE_FILE_HEADER) +
                                 ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
                                 );
   NumberOfSections = ImgHdr->Pe32.FileHeader.NumberOfSections;

   //
   // Get base address from the first section header that doesn't point to code section.
   //
   for (Index = 0; Index < NumberOfSections; Index++) {
     //
     // Read section header from file
     //
     Size = sizeof (EFI_IMAGE_SECTION_HEADER);
     Status = ImageContext->ImageRead (
                              ImageContext->Handle,
                              SectionHeaderOffset,
                              &Size,
                              &SectionHeader
                              );
     if (EFI_ERROR (Status)) {
       return Status;
     }
     
     Status = EFI_NOT_FOUND;
     
     if ((SectionHeader.Characteristics & EFI_IMAGE_SCN_CNT_CODE) == 0) {
       //
       // Build tool saves the offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields in the
       // first section header that doesn't point to code section in image header. And there is an assumption that when the
       // feature is enabled, if a module is assigned a loading address by tools, PointerToRelocations & PointerToLineNumbers
       // fields should NOT be Zero, or else, these 2 fileds should be set to Zero
       //
       ValueInSectionHeader = ReadUnaligned64((UINT64*)&SectionHeader.PointerToRelocations);
       if (ValueInSectionHeader != 0) {
         //
         // Found first section header that doesn't point to code section in which uild tool saves the
         // offset to SMRAM base as image base in PointerToRelocations & PointerToLineNumbers fields
         //
         FixLoaddingAddress = (EFI_PHYSICAL_ADDRESS)(SmramBase + (INT64)ValueInSectionHeader);

         if (SmramBase + SmmCodeSize > FixLoaddingAddress && SmramBase <=  FixLoaddingAddress) {
           //
           // The assigned address is valid. Return the specified loadding address
           //
           ImageContext->ImageAddress = FixLoaddingAddress;
           Status = EFI_SUCCESS;
         }
       }
       break;
     }
     SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
   }
   DEBUG ((EFI_D_INFO|EFI_D_LOAD, "LOADING MODULE FIXED INFO: Loading module at fixed address %x, Status = %r \n", FixLoaddingAddress, Status));
   return Status;
}

/**
  Searches all the available firmware volumes and returns the first matching FFS section. 

  This function searches all the firmware volumes for FFS files with FV file type specified by FileType
  The order that the firmware volumes is searched is not deterministic. For each available FV a search 
  is made for FFS file of type FileType. If the FV contains more than one FFS file with the same FileType, 
  the FileInstance instance will be the matched FFS file. For each FFS file found a search 
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance instances 
  of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer. 
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size. 
  It is the caller's responsibility to use FreePool() to free the allocated buffer.  
  See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for details on how sections 
  are retrieved from an FFS file based on SectionType and SectionInstance.

  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  This function must be called with a TPL <= TPL_NOTIFY.

  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().

  @param  FileType             Indicates the FV file type to search for within all available FVs.
  @param  FileInstance         Indicates which file instance within all available FVs specified by FileType.
                               FileInstance starts from zero.
  @param  SectionType          Indicates the FFS section type to search for within the FFS file 
                               specified by FileType with FileInstance.
  @param  SectionInstance      Indicates which section instance within the FFS file 
                               specified by FileType with FileInstance to retrieve. SectionInstance starts from zero.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.
                               Is it the caller's responsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough resources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.
**/
EFI_STATUS
EFIAPI
GetSectionFromAnyFvByFileType  (
  IN  EFI_FV_FILETYPE               FileType,
  IN  UINTN                         FileInstance,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
{
  EFI_STATUS                 Status;
  UINTN                      FvIndex;
  EFI_PEI_FV_HANDLE          VolumeHandle;
  EFI_PEI_FILE_HANDLE        FileHandle;
  EFI_PE32_SECTION           *SectionData;
  UINT32                     SectionSize;

  //
  // Search all FV
  //
  VolumeHandle = NULL;
  for (FvIndex = 0; ; FvIndex++) {
    Status = PeiServicesFfsFindNextVolume (FvIndex, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Search PEIM FFS
    //
    FileHandle = NULL;
    Status = PeiServicesFfsFindNextFile (FileType, VolumeHandle, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Search Section
    //
    SectionData = NULL;
    Status = PeiServicesFfsFindSectionData (SectionType, FileHandle, Buffer);
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Great!
    //
    SectionData = (EFI_PE32_SECTION *)((UINT8 *)*Buffer - sizeof(EFI_PE32_SECTION));
    ASSERT (SectionData->Type == SectionType);
    SectionSize = *(UINT32 *)SectionData->Size;
    SectionSize &= 0xFFFFFF;
    *Size = SectionSize - sizeof(EFI_PE32_SECTION);

    if (FileType == EFI_FV_FILETYPE_MM_CORE_STANDALONE) {
      EFI_FV_INFO VolumeInfo;

      //
      // This is SMM BFV
      //
      Status = PeiServicesFfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
      if (!EFI_ERROR (Status)) {
        gMmCorePrivate->StandaloneBfvAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)VolumeInfo.FvStart;
      }

    }
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Load the SMM Core image into SMRAM and executes the SMM Core from SMRAM.

  @param[in] SmramRange  Descriptor for the range of SMRAM to reload the 
                         currently executing image.
  @param[in] Context     Context to pass into SMM Core

  @return  EFI_STATUS

**/
EFI_STATUS
ExecuteSmmCoreFromSmram (
  IN EFI_SMRAM_DESCRIPTOR  *SmramRange,
  IN VOID                  *Context
  )
{
  EFI_STATUS                    Status;
  VOID                          *SourceBuffer;
  UINTN                         SourceSize;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINTN                         PageCount;
  EFI_PHYSICAL_ADDRESS          DestinationBuffer;
  VOID                          *HobList;

  Status = PeiServicesGetHobList (&HobList);
  ASSERT_EFI_ERROR (Status);

  //
  // Search all Firmware Volumes for a PE/COFF image in a file of type SMM_CORE
  //  
  Status = GetSectionFromAnyFvByFileType (
             EFI_FV_FILETYPE_MM_CORE_STANDALONE,
             0,
             EFI_SECTION_PE32,
             0,
             &SourceBuffer,
             &SourceSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Initilize ImageContext
  //
  ImageContext.Handle    = SourceBuffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // if Loading module at Fixed Address feature is enabled, the SMM core driver will be loaded to 
  // the address assigned by build tool.
  //
  if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
    //
    // Get the fixed loading address assigned by Build tool
    //
    Status = GetPeCoffImageFixLoadingAssignedAddress (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Since the memory range to load SMM CORE will be cut out in SMM core, so no need to allocate and free this range
      //
      PageCount = 0;
     } else {
      DEBUG ((EFI_D_INFO, "LOADING MODULE FIXED ERROR: Loading module at fixed address at address failed\n"));
      //
      // Allocate memory for the image being loaded from the EFI_SRAM_DESCRIPTOR 
      // specified by SmramRange
      //
      PageCount = (UINTN)EFI_SIZE_TO_PAGES((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

      ASSERT ((SmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
      ASSERT (SmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

      SmramRange->PhysicalSize -= EFI_PAGES_TO_SIZE (PageCount);
      DestinationBuffer = SmramRange->CpuStart + SmramRange->PhysicalSize;
      FillSmmCoreInformation (DestinationBuffer, EFI_PAGES_TO_SIZE (PageCount));

      //
      // Align buffer on section boundry
      //
      ImageContext.ImageAddress = DestinationBuffer;
    }
  } else {
    //
    // Allocate memory for the image being loaded from the EFI_SRAM_DESCRIPTOR 
    // specified by SmramRange
    //
    PageCount = (UINTN)EFI_SIZE_TO_PAGES((UINTN)ImageContext.ImageSize + ImageContext.SectionAlignment);

    ASSERT ((SmramRange->PhysicalSize & EFI_PAGE_MASK) == 0);
    ASSERT (SmramRange->PhysicalSize > EFI_PAGES_TO_SIZE (PageCount));

    SmramRange->PhysicalSize -= EFI_PAGES_TO_SIZE (PageCount);
    DestinationBuffer = SmramRange->CpuStart + SmramRange->PhysicalSize;
    FillSmmCoreInformation (DestinationBuffer, EFI_PAGES_TO_SIZE (PageCount));

    //
    // Align buffer on section boundry
    //
    ImageContext.ImageAddress = DestinationBuffer;
  }
  
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~((EFI_PHYSICAL_ADDRESS)(ImageContext.SectionAlignment - 1));

  //
  // Print debug message showing SMM Core load address.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL loading SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.ImageAddress));

  //
  // Load the image to our new buffer
  //
  //CpuDeadLoop();
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (!EFI_ERROR (Status)) {
    //
    // Relocate the image in our new buffer
    //
    //CpuDeadLoop();
    Status = PeCoffLoaderRelocateImage (&ImageContext);
    if (!EFI_ERROR (Status)) {
      //
      // Flush the instruction cache so the image data are written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      //
      // Print debug message showing SMM Core entry point address.
      //
      DEBUG ((DEBUG_INFO, "SMM IPL calling SMM Core at SMRAM address %p\n", (VOID *)(UINTN)ImageContext.EntryPoint));

      gMmCorePrivate->MmCoreImageBase = ImageContext.ImageAddress;
      gMmCorePrivate->MmCoreImageSize = ImageContext.ImageSize;
      DEBUG ((DEBUG_INFO, "SmmCoreImageBase - 0x%016lx\n", gMmCorePrivate->MmCoreImageBase));
      DEBUG ((DEBUG_INFO, "SmmCoreImageSize - 0x%016lx\n", gMmCorePrivate->MmCoreImageSize));

      gMmCorePrivate->MmCoreEntryPoint = ImageContext.EntryPoint;
      DEBUG ((DEBUG_INFO, "SmmCoreEntryPoint - 0x%016lx\n", gMmCorePrivate->MmCoreEntryPoint));

      //
      // Execute image
      //
      LoadSmmCore (ImageContext.EntryPoint, HobList);
    }
  }

  //
  // If the load operation, relocate operation, or the image execution return an
  // error, then free memory allocated from the EFI_SRAM_DESCRIPTOR specified by 
  // SmramRange
  //
  if (EFI_ERROR (Status)) {
    SmramRange->PhysicalSize += EFI_PAGES_TO_SIZE (PageCount);
  }

  //
  // Always free memory allocted by GetFileBufferByFilePath ()
  //
  FreePool (SourceBuffer);

  return Status;
}

VOID
SmmIplDispatchDriver (
  VOID
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;
    
  while (TRUE) {
    //
    // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure 
    //
    CopyGuid (&CommunicateHeader.HeaderGuid, &gEfiEventDxeDispatchGuid);
    CommunicateHeader.MessageLength = 1;
    CommunicateHeader.Data[0] = 0;

    //
    // Generate the Software SMI and return the result
    //
    Size = sizeof (CommunicateHeader);
    Communicate (NULL, &CommunicateHeader, &Size);
      
    //
    // Return if there is no request to restart the SMM Core Dispatcher
    //
    if (CommunicateHeader.Data[0] != COMM_BUFFER_MM_DISPATCH_RESTART) {
      break;
    }
  }
}

/**
  The Entry Point for SMM IPL

  Load SMM Core into SMRAM.
  
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services. 

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmIplEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PEI_SMM_ACCESS_PPI              *SmmAccess;
  EFI_SMRAM_DESCRIPTOR            *SmramRanges;
  EFI_STATUS                      Status;
  UINTN                           Size;
  UINTN                           Index;
  UINT64                          MaxSize;
  UINT64                          SmmCodeSize;
  MM_CORE_DATA_HOB_DATA           SmmCoreDataHobData;
  VOID                            *GuidHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;

  //
  // Build Hob for SMM and DXE phase
  //
  SmmCoreDataHobData.Address = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimePages(EFI_SIZE_TO_PAGES(sizeof(mMmCorePrivateData)));
  ASSERT(SmmCoreDataHobData.Address != 0);
  gMmCorePrivate = (VOID *)(UINTN)SmmCoreDataHobData.Address;
  CopyMem ((VOID *)(UINTN)SmmCoreDataHobData.Address, &mMmCorePrivateData, sizeof(mMmCorePrivateData));
  DEBUG((EFI_D_INFO, "gMmCorePrivate - 0x%x\n", gMmCorePrivate));

  BuildGuidDataHob (
    &gMmCoreDataHobGuid,
    (VOID *)&SmmCoreDataHobData,
    sizeof(SmmCoreDataHobData)
    );

  //
  // Get SMM Access Protocol
  //
  Status = PeiServicesLocatePpi (&gPeiSmmAccessPpiGuid, 0, NULL, (VOID **)&SmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMRAM information
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (GuidHob != NULL);
  DescriptorBlock = (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK *)GET_GUID_HOB_DATA(GuidHob);

  gMmCorePrivate->MmramRangeCount = DescriptorBlock->NumberOfSmmReservedRegions;
  Size = (DescriptorBlock->NumberOfSmmReservedRegions) * sizeof(EFI_SMRAM_DESCRIPTOR);
  gMmCorePrivate->MmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (Size);
  ASSERT (gMmCorePrivate->MmramRanges != 0);
  CopyMem ((VOID *)(UINTN)gMmCorePrivate->MmramRanges, DescriptorBlock->Descriptor, Size);

  //
  // Open all SMRAM ranges
  //
  Status = SmmAccess->Open ((EFI_PEI_SERVICES **)PeiServices, SmmAccess, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Print debug message that the SMRAM window is now open.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL opened SMRAM window\n"));

  //
  // Find the largest SMRAM range between 1MB and 4GB that is at least 256KB - 4K in size
  //
  mCurrentSmramRange = NULL;
  SmramRanges = (EFI_SMRAM_DESCRIPTOR *)(UINTN)gMmCorePrivate->MmramRanges;
  for (Index = 0, MaxSize = SIZE_256KB - EFI_PAGE_SIZE; Index < gMmCorePrivate->MmramRangeCount; Index++) {
    //
    // Skip any SMRAM region that is already allocated, needs testing, or needs ECC initialization
    //
    if ((SmramRanges[Index].RegionState & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
      continue;
    }

    if (SmramRanges[Index].CpuStart >= BASE_1MB) {
      if ((SmramRanges[Index].CpuStart + SmramRanges[Index].PhysicalSize) <= BASE_4GB) {
        if (SmramRanges[Index].PhysicalSize >= MaxSize) {
          MaxSize = SmramRanges[Index].PhysicalSize;
          mCurrentSmramRange = &SmramRanges[Index];
        }
      }
    }
  }

  if (mCurrentSmramRange != NULL) {
    //
    // Print debug message showing SMRAM window that will be used by SMM IPL and SMM Core
    //
    DEBUG ((DEBUG_INFO, "SMM IPL found SMRAM window %p - %p\n", 
      (VOID *)(UINTN)mCurrentSmramRange->CpuStart, 
      (VOID *)(UINTN)(mCurrentSmramRange->CpuStart + mCurrentSmramRange->PhysicalSize - 1)
      ));

    GetSmramCacheRange (mCurrentSmramRange, &mSmramCacheBase, &mSmramCacheSize);

    //
    // if Loading module at Fixed Address feature is enabled, save the SMRAM base to Load
    // Modules At Fixed Address Configuration Table.
    //
    if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
      //
      // Build tool will calculate the smm code size and then patch the PcdLoadFixAddressSmmCodePageNumber
      //
      SmmCodeSize = LShiftU64 (PcdGet32(PcdLoadFixAddressSmmCodePageNumber), EFI_PAGE_SHIFT);
      //
      // The SMRAM available memory is assumed to be larger than SmmCodeSize
      //
      ASSERT (mCurrentSmramRange->PhysicalSize > SmmCodeSize);
    }
    //
    // Load SMM Core into SMRAM and execute it from SMRAM
    //
    Status = ExecuteSmmCoreFromSmram (mCurrentSmramRange, gMmCorePrivate);
    if (EFI_ERROR (Status)) {
      //
      // Print error message that the SMM Core failed to be loaded and executed.
      //
      DEBUG ((DEBUG_ERROR, "SMM IPL could not load and execute SMM Core from SMRAM\n"));
    }
  } else {
    //
    // Print error message that there are not enough SMRAM resources to load the SMM Core.
    //
    DEBUG ((DEBUG_ERROR, "SMM IPL could not find a large enough SMRAM region to load SMM Core\n"));
  }

  //
  // If the SMM Core could not be loaded then close SMRAM window, free allocated 
  // resources, and return an error so SMM IPL will be unloaded.
  //
  if (mCurrentSmramRange == NULL || EFI_ERROR (Status)) {
    //
    // Close all SMRAM ranges
    //
    Status = SmmAccess->Close ((EFI_PEI_SERVICES **)PeiServices, SmmAccess, 0);
    ASSERT_EFI_ERROR (Status);

    //
    // Print debug message that the SMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "SMM IPL closed SMRAM window\n"));

    //
    // Free all allocated resources
    //
    FreePool ((VOID *)(UINTN)gMmCorePrivate->MmramRanges);
    
    return EFI_UNSUPPORTED;
  }
  
  //
  // Install SmmCommunicationPpi
  //
  Status = PeiServicesInstallPpi (&mPpiList);

  //
  // Dispatch Driver again, because PiSmmCpu will cause exit
  //
  SmmIplDispatchDriver ();

  return EFI_SUCCESS;
}
