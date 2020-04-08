/** @file
  FSP-M component implementation.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FspmInit.h"
#include <Library/FspCommonLib.h>

extern EFI_GUID gFspSiliconFvGuid;

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mPpiBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

EFI_PEI_NOTIFY_DESCRIPTOR mMemoryDiscoveredNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  MemoryDiscoveredPpiNotifyCallback
};


/**
  Reads 8-bits of CMOS data.

  Reads the 8-bits of CMOS data at the location specified by Index.
  The 8-bit read value is returned.

  @param  Index  The CMOS location to read.

  @return The value read.

**/
UINT8
EFIAPI
CmosRead8 (
  IN      UINTN                     Index
  )
{
  IoWrite8 (0x70, (UINT8) Index);
  return IoRead8 (0x71);
}


/**
  Writes 8-bits of CMOS data.

  Writes 8-bits of CMOS data to the location specified by Index
  with the value specified by Value and returns Value.

  @param  Index  The CMOS location to write.
  @param  Value  The value to write to CMOS.

  @return The value written to CMOS.

**/
UINT8
EFIAPI
CmosWrite8 (
  IN      UINTN                     Index,
  IN      UINT8                     Value
  )
{
  IoWrite8 (0x70, (UINT8) Index);
  IoWrite8 (0x71, Value);
  return Value;
}

/**
  Migrate FSP-M UPD data before destroying CAR.

**/
VOID
EFIAPI
MigrateFspmUpdData (
  VOID
 )
{
  FSP_INFO_HEADER           *FspInfoHeaderPtr;
  VOID                      *FspmUpdPtrPostMem;
  VOID                      *FspmUpdPtrPreMem;

  FspInfoHeaderPtr = GetFspInfoHeader();
  FspmUpdPtrPostMem = (VOID *)AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)FspInfoHeaderPtr->CfgRegionSize));
  ASSERT(FspmUpdPtrPostMem != NULL);

  FspmUpdPtrPreMem = (VOID *)GetFspMemoryInitUpdDataPointer ();
  CopyMem (FspmUpdPtrPostMem, (VOID *)FspmUpdPtrPreMem, (UINTN)FspInfoHeaderPtr->CfgRegionSize);

  //
  // Update FSP-M UPD pointer in FSP Global Data
  //
  SetFspMemoryInitUpdDataPointer((VOID *)FspmUpdPtrPostMem);

  DEBUG ((DEBUG_INFO, "Migrate FSP-M UPD from %x to %x \n", FspmUpdPtrPreMem, FspmUpdPtrPostMem));

}


/**
  This function reports and installs new FV

  @retval     EFI_SUCCESS          The function completes successfully
**/
EFI_STATUS
ReportAndInstallNewFv (
  VOID
  )
{
  FSP_INFO_HEADER                *FspInfoHeader;
  EFI_FIRMWARE_VOLUME_HEADER     *FvHeader;
  UINT8                          *CurPtr;
  UINT8                          *EndPtr;

  FspInfoHeader = GetFspInfoHeaderFromApiContext();
  if (FspInfoHeader->Signature != FSP_INFO_HEADER_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "The signature of FspInfoHeader getting from API context is invalid at %p.\n", FspInfoHeader));
    FspInfoHeader = GetFspInfoHeader();
  }

  CurPtr = (UINT8 *)(UINTN)FspInfoHeader->ImageBase;
  EndPtr = CurPtr + FspInfoHeader->ImageSize - 1;

  while (CurPtr < EndPtr) {
    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)CurPtr;
    if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
      break;
    }
    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *)FvHeader,
      (UINT32)FvHeader->FvLength,
      NULL,
      NULL
      );
    CurPtr += FvHeader->FvLength;
  }

  return EFI_SUCCESS;
}

/**
  This function will be called when MRC is done.

  @param[in] PeiServices         General purpose services available to every PEIM.
  @param[in] NotifyDescriptor    Information about the notify event..
  @param[in] Ppi                 The notify context.

  @retval EFI_SUCCESS            If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  UINT64                        LowMemoryLength;
  UINT64                        HighMemoryLength;
  UINT64                        MaxLowMemoryLength;
  UINT8                         PhysicalAddressBits;
  UINT32                        RegEax;
  EFI_HOB_RESOURCE_DESCRIPTOR   *Descriptor;
  VOID                          **HobListPtr;

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "Memory Discovered Notify invoked ...\n"));

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits = 36;
  }

  //
  // Create a CPU hand-off information
  //
  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Get system memory from HOB
  //
  FspGetSystemMemorySize (&LowMemoryLength, &HighMemoryLength);

  //
  // FSP reserved memory is immediately following all available system memory regions,
  // so we should add it back to ensure this reserved region is cached.
  //
  Descriptor =  FspGetResourceDescriptorByOwner (&gFspReservedMemoryResourceHobGuid);
  ASSERT (Descriptor != NULL);
  LowMemoryLength =  Descriptor->PhysicalStart + Descriptor->ResourceLength;

  Descriptor =  FspGetResourceDescriptorByOwner (&gFspBootLoaderTolumHobGuid);
  if (Descriptor) {
    LowMemoryLength += Descriptor->ResourceLength;
  }

  DEBUG ((DEBUG_INFO, "FSP TOLM = 0x%08X\n", (UINT32)LowMemoryLength));

  //
  // Migrate FSP-M UPD data before destroying CAR
  //
  MigrateFspmUpdData ();

  HobListPtr = (VOID **)GetFspApiParameter2 ();
  if (HobListPtr != NULL) {
    *HobListPtr = (VOID *)GetHobList ();
  }
  //
  // Give control back after MemoryInitApi
  //
  FspMemoryInitDone (HobListPtr);

  if (GetFspApiCallingIndex() == TempRamExitApiIndex) {
    //
    // Disable CAR
    //
    ResetCacheAttributes ();

    //
    // Set fixed MTRR values
    //
    SetCacheAttributes (
      0x00000,
      0xA0000,
      EFI_CACHE_WRITEBACK
      );

    SetCacheAttributes (
      0xA0000,
      0x20000,
      EFI_CACHE_UNCACHEABLE
      );

    SetCacheAttributes (
      0xC0000,
      0x40000,
      EFI_CACHE_WRITEPROTECTED
      );

    //
    // Set the largest range as WB and then patch smaller ranges with UC
    // It can reduce the MTRR register usage
    //
    MaxLowMemoryLength = GetPowerOfTwo64 (LowMemoryLength);
    if (LowMemoryLength != MaxLowMemoryLength) {
      MaxLowMemoryLength = LShiftU64 (MaxLowMemoryLength, 1);
    }
    if (MaxLowMemoryLength >= 0x100000000ULL) {
      MaxLowMemoryLength = (LowMemoryLength + 0x0FFFFFFF) & 0xF0000000;
    }

    SetCacheAttributes (
      0,
      MaxLowMemoryLength,
      EFI_CACHE_WRITEBACK
      );

    if (LowMemoryLength != MaxLowMemoryLength) {
      SetCacheAttributes (
        LowMemoryLength,
        MaxLowMemoryLength - LowMemoryLength,
        EFI_CACHE_UNCACHEABLE
        );
    }

    if (HighMemoryLength) {
      SetCacheAttributes (
        0x100000000,
        HighMemoryLength,
        EFI_CACHE_WRITEBACK
        );
    }

    DEBUG ((DEBUG_INFO | DEBUG_INIT, "Memory Discovered Notify completed ...\n"));

    //
    // Give control back after TempRamExitApi
    //
    FspTempRamExitDone ();
  }

  ReportAndInstallNewFv ();

  return EFI_SUCCESS;
}

/**
  Get system low memory size.

  @retval  system memory system below 4GB.
**/
UINT32
GetSystemMemorySizeBelow4Gb (
  VOID
  )
{
  UINT8 Cmos0x34;
  UINT8 Cmos0x35;

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8) CmosRead8 (0x34);
  Cmos0x35 = (UINT8) CmosRead8 (0x35);

  return (UINT32) (((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}

/**
  Get system high memory size.

  @retval  system memory system above 4GB.
**/
UINT64
GetSystemMemorySizeAbove4Gb (
  )
{
  UINT32 Size;
  UINTN  CmosIndex;

  //
  // CMOS 0x5b-0x5d specifies the system memory above 4GB MB.
  // * CMOS(0x5d) is the most significant size byte
  // * CMOS(0x5c) is the middle size byte
  // * CMOS(0x5b) is the least significant size byte
  // * The size is specified in 64kb chunks
  //

  Size = 0;
  for (CmosIndex = 0x5d; CmosIndex >= 0x5b; CmosIndex--) {
    Size = (UINT32) (Size << 8) + (UINT32) CmosRead8 (CmosIndex);
  }

  return LShiftU64 (Size, 16);
}

/**
  Initialize PCI Express BAR

**/
VOID
PciExBarInitialization (
  VOID
  )
{
  union {
    UINT64 Uint64;
    UINT32 Uint32[2];
  } PciExBarBase;

  //
  // We only support the 256MB size for the MMCONFIG area:
  // 256 buses * 32 devices * 8 functions * 4096 bytes config space.
  //
  // The masks used below enforce the Q35 requirements that the MMCONFIG area
  // be (a) correctly aligned -- here at 256 MB --, (b) located under 64 GB.
  //
  // Note that (b) also ensures that the minimum address width we have
  // determined in AddressWidthInitialization(), i.e., 36 bits, will suffice
  // for DXE's page tables to cover the MMCONFIG area.
  //
  PciExBarBase.Uint64 = PcdGet64 (PcdPciExpressBaseAddress);
  ASSERT ((PciExBarBase.Uint32[1] & MCH_PCIEXBAR_HIGHMASK) == 0);
  ASSERT ((PciExBarBase.Uint32[0] & MCH_PCIEXBAR_LOWMASK) == 0);

  //
  // Clear the PCIEXBAREN bit first, before programming the high register.
  //
  PciCf8Write32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW), 0);

  //
  // Program the high register. Then program the low register, setting the
  // MMCONFIG area size and enabling decoding at once.
  //
  PciCf8Write32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_HIGH), PciExBarBase.Uint32[1]);
  PciCf8Write32 (
    DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW),
    PciExBarBase.Uint32[0] | MCH_PCIEXBAR_BUS_FF | MCH_PCIEXBAR_EN
    );
}

/**
  Miscellaneous chipset initialization

**/
VOID
MiscInitialization (
  VOID
  )
{
  UINTN         PmCmd;
  UINTN         Pmba;
  UINT32        PmbaAndVal;
  UINT32        PmbaOrVal;
  UINTN         AcpiCtlReg;
  UINT8         AcpiEnBit;

  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Determine platform type and save Host Bridge DID to PCD
  //
  PmCmd      = POWER_MGMT_REGISTER_Q35 (PCI_COMMAND_OFFSET);
  Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
  PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
  PmbaOrVal  = ICH9_PMBASE_VALUE;
  AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
  AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;

  //
  // If the appropriate IOspace enable bit is set, assume the ACPI PMBA
  // has been configured (e.g., by Xen) and skip the setup here.
  // This matches the logic in AcpiTimerLibConstructor ().
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // The PEI phase should be exited with fully accessibe ACPI PM IO space:
    // 1. set PMBA
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

    //
    // 2. set PCICMD/IOSE
    //
    PciOr8 (PmCmd, EFI_PCI_COMMAND_IO_SPACE);

    //
    // 3. set ACPI PM IO enable bit (PMREGMISC:PMIOSE or ACPI_CNTL:ACPI_EN)
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

}

/**
  Platform  chipset initialization

  @retval EFI_UNSUPPORTED        Unsupported chipset.
  @retval EFI_SUCCESS            Platform has been initialized successfully.

**/
EFI_STATUS
PlatformInit (
  VOID
)
{
  UINT16      HostBridgeDevId;

  //
  // Query Host Bridge DID
  //
  HostBridgeDevId = PciCf8Read16 (OVMF_HOSTBRIDGE_DID);
  if (HostBridgeDevId != INTEL_Q35_MCH_DEVICE_ID) {
    DEBUG ((DEBUG_ERROR, "Unknown Host Bridge Device ID: 0x%04x\n"));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  PciExBarInitialization ();

  MiscInitialization ();

  return EFI_SUCCESS;
}

/**
  Initialize TSEG size

  @retval     TSEG size supported by platform.

**/
UINT32
InitializeSmramTsegSize (
  VOID
  )
{
  UINT16        ExtendedTsegMbytes;
  UINT16        TsegMbytes;

  //
  // Check if QEMU offers an extended TSEG.
  //
  // This can be seen from writing MCH_EXT_TSEG_MB_QUERY to the MCH_EXT_TSEG_MB
  // register, and reading back the register.
  //
  // On a QEMU machine type that does not offer an extended TSEG, the initial
  // write overwrites whatever value a malicious guest OS may have placed in
  // the (unimplemented) register, before entering S3 or rebooting.
  // Subsequently, the read returns MCH_EXT_TSEG_MB_QUERY unchanged.
  //
  // On a QEMU machine type that offers an extended TSEG, the initial write
  // triggers an update to the register. Subsequently, the value read back
  // (which is guaranteed to differ from MCH_EXT_TSEG_MB_QUERY) tells us the
  // number of megabytes.
  //
  PciWrite16 (DRAMC_REGISTER_Q35 (MCH_EXT_TSEG_MB), MCH_EXT_TSEG_MB_QUERY);
  ExtendedTsegMbytes = PciRead16 (DRAMC_REGISTER_Q35 (MCH_EXT_TSEG_MB));
  if (ExtendedTsegMbytes == MCH_EXT_TSEG_MB_QUERY) {
    TsegMbytes = 0x10;
    PciWrite16 (DRAMC_REGISTER_Q35 (MCH_EXT_TSEG_MB), TsegMbytes);
  } else {
    TsegMbytes = ExtendedTsegMbytes;
  }
  return (UINT32)TsegMbytes << 20;
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
FspmInitEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                      Status;
  UINT64                          PeiMemSize;
  EFI_PHYSICAL_ADDRESS            PeiMemBase;
  FSPM_UPD                       *FspmUpd;
  EFI_BOOT_MODE                   BootMode;
  UINT32                          LowMemLen;
  UINT64                          HighMemLen;
  UINT32                          TsegSize;
  UINT32                          TsegBase;

  DEBUG ((DEBUG_INFO, "FspmInitPoint() - Begin\n"));

  //
  // QEMU init
  //
  Status = PlatformInit ();

  //
  // Get the Boot Mode
  //
  FspmUpd  = (FSPM_UPD *)GetFspMemoryInitUpdDataPointer ();
  BootMode = FspmUpd->FspmArchUpd.BootMode;
  DEBUG ((DEBUG_INFO, "BootMode : 0x%x\n", BootMode));
  PeiServicesSetBootMode (BootMode);


  Status = PeiServicesInstallPpi (&mPpiBootMode);

  //
  // Now that all of the pre-permanent memory activities have
  // been taken care of, post a call-back for the permanent-memory
  // resident services, such as HOB construction.
  // PEI Core will switch stack after this PEIM exit.  After that the MTRR
  // can be set.
  //
  Status = PeiServicesNotifyPpi (&mMemoryDiscoveredNotifyList);
  ASSERT_EFI_ERROR (Status);

  LowMemLen  = GetSystemMemorySizeBelow4Gb();
  HighMemLen = GetSystemMemorySizeAbove4Gb();

  TsegSize   = InitializeSmramTsegSize ();
  TsegBase   = LowMemLen - TsegSize;

  PeiMemSize = PcdGet32(PcdFspReservedMemoryLength);
  ASSERT (LowMemLen > PeiMemSize + TsegSize);
  PeiMemBase = LowMemLen - TsegSize - PeiMemSize;

  //
  // Report first 640KB of memory
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    MEM_TESTED_ATTR,
    (EFI_PHYSICAL_ADDRESS) (0),
    (UINT64) (0xA0000)
  );

  //
  // Report first 0A0000h - 0FFFFFh as RESERVED memory
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
    MEM_TESTED_ATTR,
    (EFI_PHYSICAL_ADDRESS) (0xA0000),
    (UINT64) (0x60000)
  );

  //
  // Report first 0x100000 - FSP reserved memory as system memory
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    MEM_TESTED_ATTR,
    (EFI_PHYSICAL_ADDRESS) (0x100000),
    (UINT64) (PeiMemBase - 0x100000)
  );

  BuildResourceDescriptorWithOwnerHob (
     EFI_RESOURCE_MEMORY_RESERVED,
     (
       EFI_RESOURCE_ATTRIBUTE_PRESENT |
       EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
       EFI_RESOURCE_ATTRIBUTE_TESTED |
       EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
     ),
     PeiMemBase,
     PeiMemSize,
     &gFspReservedMemoryResourceHobGuid
   );

  if (HighMemLen) {
    BuildResourceDescriptorHob (
     EFI_RESOURCE_SYSTEM_MEMORY,
     MEM_NOT_TESTED_ATTR,
     0x100000000ULL,
     HighMemLen
    );
  }

  // Report TSEG memroy
  BuildResourceDescriptorWithOwnerHob (
      EFI_RESOURCE_MEMORY_RESERVED,
      (
        EFI_RESOURCE_ATTRIBUTE_PRESENT |
        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
        EFI_RESOURCE_ATTRIBUTE_TESTED |
        EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
        EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      TsegBase,
      TsegSize,
      &gFspReservedMemoryResourceHobTsegGuid
    );

  Status = PeiServicesInstallPeiMemory (PeiMemBase, PeiMemSize);

  DEBUG ((DEBUG_INFO, "FspmInitPoint() - End\n"));

  return Status;
}
