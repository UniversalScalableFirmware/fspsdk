## @file
# FSP DSC build file for QEMU platform
#
# Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = QemuFspPkg
  PLATFORM_GUID                  = 1BEDB57A-7904-406e-8486-C89FC7FB39EE
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/QemuFspPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = QemuFspPkg/QemuFspPkg.fdf

  #
  # UPD tool definition
  #
  FSP_R_UPD_FFS_GUID             = 658FF4B0-DD33-4295-AC27-13E5A268D991
  FSP_T_UPD_FFS_GUID             = 70BCF6A5-FFB1-47D8-B1AE-EFE5508E23EA
  FSP_M_UPD_FFS_GUID             = D5B86AEA-6AF7-40D4-8014-982301BC3D89
  FSP_S_UPD_FFS_GUID             = E3CD9B18-998C-4F76-B65E-98B154E5446F

  #
  # Set platform specific package/folder name, same as passed from PREBUILD script.
  # PLATFORM_PACKAGE would be the same as PLATFORM_NAME as well as package build folder
  # DEFINE only takes effect at R9 DSC and FDF.
  #
  DEFINE FSP_PACKAGE                     = QemuFspPkg
  DEFINE FSP_IMAGE_ID                    = 0x245053464D455124  # $QEMFSP$
  DEFINE FSP_IMAGE_REV                   = 0x00001010

  DEFINE CAR_BASE_ADDRESS                = 0x00000000
  DEFINE CAR_REGION_SIZE                 = 0x00080000
  DEFINE CAR_BLD_REGION_SIZE             = 0x00070000
  DEFINE CAR_FSP_REGION_SIZE             = 0x00010000

  DEFINE FSP_ARCH                        = X64

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses]
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  CacheLib|IntelFsp2Pkg/Library/BaseCacheLib/BaseCacheLib.inf
  CacheAsRamLib|IntelFsp2Pkg/Library/BaseCacheAsRamLibNull/BaseCacheAsRamLibNull.inf
  FspSwitchStackLib|IntelFsp2Pkg/Library/BaseFspSwitchStackLib/BaseFspSwitchStackLib.inf
  FspCommonLib|IntelFsp2Pkg/Library/BaseFspCommonLib/BaseFspCommonLib.inf
  FspPlatformLib|IntelFsp2Pkg/Library/BaseFspPlatformLib/BaseFspPlatformLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
!if $(TARGET) == DEBUG
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
!endif


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnS3Boot    | TRUE
  gQemuFspPkgTokenSpaceGuid.PcdFspHeaderRevision          | 0x03
  gQemuFspPkgTokenSpaceGuid.PcdFspImageIdString           | $(FSP_IMAGE_ID)
  gQemuFspPkgTokenSpaceGuid.PcdFspImageRevision           | $(FSP_IMAGE_REV)
  #
  # FSP CAR Usages  (BL RAM | FSP RAM | FSP CODE)
  #
  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamBase         | $(CAR_BASE_ADDRESS)
  gIntelFsp2PkgTokenSpaceGuid.PcdTemporaryRamSize         | $(CAR_REGION_SIZE)
  gIntelFsp2PkgTokenSpaceGuid.PcdFspTemporaryRamSize      | $(CAR_FSP_REGION_SIZE)
  gIntelFsp2PkgTokenSpaceGuid.PcdFspReservedBufferSize    | 0x0100

  # This defines how much space will be used for heap in FSP temporary memory
  # x % of FSP temporary memory will be used for heap
  # (100 - x) % of FSP temporary memory will be used for stack
  gIntelFsp2PkgTokenSpaceGuid.PcdFspHeapSizePercentage    | 65

  # This is a platform specific global pointer used by FSP
  gIntelFsp2PkgTokenSpaceGuid.PcdGlobalDataPointerAddress | 0xFED00148
  gIntelFsp2PkgTokenSpaceGuid.PcdFspReservedMemoryLength  | 0x00100000

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel   | 0x00000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask           | 0
!else
  gEfiMdePkgTokenSpaceGuid.PcdFixedDebugPrintErrorLevel   | 0x80000047
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask           | 0x27
!endif

[PcdsPatchableInModule]
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress       | 0xE0000000
  #
  # This entry will be patched during the build process
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress        | 0x12345678

!if $(TARGET) == RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel        | 0
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel        | 0x80000047
!endif


###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################
[Components.IA32]
  #
  # FSP Binary Components
  #
  $(FSP_PACKAGE)/FspHeader/FspHeader.inf

  #
  # SEC
  #
  IntelFsp2Pkg/FspSecCore/FspSecCoreT.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecTLib.inf
  }

  QemuFspPkg/FsprInit/FsprInit.inf

  QemuFspPkg/FsprInit/Ia32/Vtf0/Bin/ResetVector.inf

[Components.$(FSP_ARCH)]
  IntelFsp2Pkg/FspSecCore/FspSecCoreM.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecMLib.inf
  }

  IntelFsp2Pkg/FspSecCore/FspSecCoreS.inf {
    <LibraryClasses>
      FspSecPlatformLib|$(FSP_PACKAGE)/Library/PlatformSecLib/Vtf0PlatformSecSLib.inf
  }

  #
  # PEI Core
  #
  MdeModulePkg/Core/Pei/PeiMain.inf

  #
  # PCD
  #
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  $(FSP_PACKAGE)/FspmInit/FspmInit.inf
  $(FSP_PACKAGE)/FspsInit/FspsInit.inf
  $(FSP_PACKAGE)/QemuVideo/QemuVideo.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
      ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  }
  IntelFsp2Pkg/FspNotifyPhase/FspNotifyPhasePeim.inf

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any
#                        standard flags that are defined by the build process. They can be
#                        applied for any modules or only those modules with the specific
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################
[BuildOptions]
# Append build options for EDK and EDKII drivers (= is Append, == is Replace)
  # Enable link-time optimization when building with GCC49
  *_GCC49_IA32_CC_FLAGS = -flto
  *_GCC49_IA32_DLINK_FLAGS = -flto
  *_GCC5_IA32_CC_FLAGS = -fno-pic
  *_GCC5_IA32_DLINK_FLAGS = -no-pie
  *_GCC5_IA32_ASLCC_FLAGS = -fno-pic
  *_GCC5_IA32_ASLDLINK_FLAGS = -no-pie
