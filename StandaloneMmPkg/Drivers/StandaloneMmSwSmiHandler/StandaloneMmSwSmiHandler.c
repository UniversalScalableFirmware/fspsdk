/** @file
  Software SMI handler implementation for bootloader.

  Copyright (c) 2011 - 2020, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/SmmCpu.h>
#include <Library/MmServicesTableLib.h>

EFI_SMM_CPU_PROTOCOL   *mSmmCpuProtocol;

UINT8                  mSmiTriggerRegister = 0xB2;
UINT8                  mSmiDataRegister    = 0xB3;

/**
  Software SMI callback for restoring SMRR base and mask in S3 path.

  @param[in]      DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      Context         Points to an optional handler context which was specified when the
                                  handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                  be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS             The interrupt was handled successfully.

**/
EFI_STATUS
EFIAPI
StandaloneMmSwSmiHandler (
  IN EFI_HANDLE                  DispatchHandle,
  IN CONST VOID                  *Context,
  IN OUT VOID                    *CommBuffer,
  IN OUT UINTN                   *CommBufferSize
  )
{
  
  EFI_STATUS                      Status;
  UINTN                           Index;
  EFI_SMM_SAVE_STATE_IO_INFO      IoInfo;

  DEBUG ((DEBUG_INFO, "Sw SMI Root Handler\n"));

  //
  // Try to find which CPU trigger SWSMI
  //
  for (Index = 0; Index < gMmst->NumberOfCpus; Index++) {
    Status = mSmmCpuProtocol->ReadSaveState (
                                mSmmCpuProtocol,
                                sizeof(IoInfo),
                                EFI_SMM_SAVE_STATE_REGISTER_IO,
                                Index,
                                &IoInfo
                                );
    if (EFI_ERROR (Status)) {
      continue;
    }
    if (IoInfo.IoPort == mSmiTriggerRegister) {
      //
      // Great! Find it.
      //
      DEBUG ((DEBUG_INFO, "CPU index = 0x%x/0x%x\n", Index, gMmst->NumberOfCpus));
      DEBUG ((DEBUG_INFO, "SW SMI Data %x\n", IoRead8 (mSmiTriggerRegister)));
      IoWrite8 (mSmiTriggerRegister, IoRead8 (mSmiTriggerRegister) + 1);
      break;
    }
  }

  return EFI_SUCCESS;
}

/**
  The driver's entry point.

  It install callbacks for bootloader sw smi

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeStandaloneMmSwSmiHandler (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     DispatchHandle;

  //
  // Locate PI SMM CPU protocol
  //
  Status = SystemTable->MmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpuProtocol
                    );
  ASSERT_EFI_ERROR (Status);


  // register a root SW SMI handler
  Status = SystemTable->MmiHandlerRegister (
                    StandaloneMmSwSmiHandler,
                    NULL,
                    &DispatchHandle
                    );

  return Status;

}

