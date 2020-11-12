/** @file
  SMM IPL that produces SMM related runtime protocols and load the SMM Core into SMRAM

  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/SmmBase2.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmControl2.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/Cpu.h>

#include <Guid/EventGroup.h>
#include <Guid/EventLegacyBios.h>
#include <Guid/MmUefiInfo.h>
#include <Guid/MmFvDispatch.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include <Guid/MmCoreData.h>

//
// Function prototypes from produced protocols
//

/**
  Indicate whether the driver is currently executing in the SMM Initialization phase.

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   InSmram                 Pointer to a Boolean which, on return, indicates that the driver is currently executing
                                   inside of SMRAM (TRUE) or outside of SMRAM (FALSE).

  @retval  EFI_INVALID_PARAMETER   InSmram was NULL.
  @retval  EFI_SUCCESS             The call returned successfully.

**/
EFI_STATUS
EFIAPI
SmmBase2InSmram (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      BOOLEAN                 *InSmram
  );

/**
  Retrieves the location of the System Management System Table (SMST).

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   Smst                    On return, points to a pointer to the System Management Service Table (SMST).

  @retval  EFI_INVALID_PARAMETER   Smst or This was invalid.
  @retval  EFI_SUCCESS             The memory was returned to the system.
  @retval  EFI_UNSUPPORTED         Not in SMM.

**/
EFI_STATUS
EFIAPI
SmmBase2GetSmstLocation (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      EFI_SMM_SYSTEM_TABLE2   **Smst
  );

/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered 
  UEFI service.  This function is part of the SMM Communication Protocol that may 
  be called in physical mode prior to SetVirtualAddressMap() and in virtual mode 
  after SetVirtualAddressMap().

  @param[in]     This                The EFI_SMM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                     being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS                The message was successfully posted.
  @retval EFI_INVALID_PARAMETER      The CommBuffer was NULL.
**/
EFI_STATUS
EFIAPI
SmmCommunicationCommunicate (
  IN CONST EFI_SMM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  );

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired when DxeDispatch Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplDxeDispatchEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Event notification that is fired when a GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmIplSetVirtualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// Data structure used to declare a table of protocol notifications and event 
// notifications required by the SMM IPL
//
typedef struct {
  BOOLEAN           Protocol;
  BOOLEAN           CloseOnLock;
  EFI_GUID          *Guid;
  EFI_EVENT_NOTIFY  NotifyFunction;
  VOID              *NotifyContext;
  EFI_TPL           NotifyTpl;
  EFI_EVENT         Event;
} SMM_IPL_EVENT_NOTIFICATION;

//
// Handle to install the SMM Base2 Protocol and the SMM Communication Protocol
//
EFI_HANDLE  mSmmIplHandle = NULL;

//
// SMM Base 2 Protocol instance
//
EFI_SMM_BASE2_PROTOCOL  mSmmBase2 = {
  SmmBase2InSmram,
  SmmBase2GetSmstLocation
};

//
// SMM Communication Protocol instance
//
EFI_SMM_COMMUNICATION_PROTOCOL  mSmmCommunication = {
  SmmCommunicationCommunicate
};

//
// Global pointer used to access mSmmCorePrivateData from outside and inside SMM
//
MM_CORE_PRIVATE_DATA  *gSmmCorePrivate;

//
// SMM IPL global variables
//
EFI_SMM_CONTROL2_PROTOCOL  *mSmmControl2;
EFI_SMM_ACCESS2_PROTOCOL   *mSmmAccess;
BOOLEAN                    mSmmLocked = FALSE;

//
// Table of Protocol notification and GUIDed Event notifications that the SMM IPL requires
//
SMM_IPL_EVENT_NOTIFICATION  mSmmIplEvents[] = {
  //
  // Declare protocl notification on DxeSmmReadyToLock protocols.  When this notification is etablished, 
  // the associated event is immediately signalled, so the notification function will be executed and the 
  // DXE SMM Ready To Lock Protocol will be found if it is already in the handle database.
  //
  { TRUE,  TRUE,  &gEfiDxeSmmReadyToLockProtocolGuid, SmmIplReadyToLockEventNotify,      &gEfiDxeSmmReadyToLockProtocolGuid, TPL_CALLBACK, NULL },
  //
  // Declare event notification on EndOfDxe event.  When this notification is etablished, 
  // the associated event is immediately signalled, so the notification function will be executed and the 
  // SMM End Of Dxe Protocol will be found if it is already in the handle database.
  //
  { FALSE, FALSE,  &gEfiEndOfDxeEventGroupGuid,        SmmIplGuidedEventNotify,           &gEfiEndOfDxeEventGroupGuid,        TPL_CALLBACK, NULL },
  //
  // Declare event notification on the DXE Dispatch Event Group.  This event is signaled by the DXE Core
  // each time the DXE Core dispatcher has completed its work.  When this event is signalled, the SMM Core
  // if notified, so the SMM Core can dispatch SMM drivers.
  //
  { FALSE, TRUE,  &gEfiEventDxeDispatchGuid,          SmmIplDxeDispatchEventNotify,      &gEfiEventDxeDispatchGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Ready To Boot Event Group.  This is an extra event notification that is
  // used to make sure SMRAM is locked before any boot options are processed.
  //
  { FALSE, TRUE,  &gEfiEventReadyToBootGuid,          SmmIplReadyToLockEventNotify,      &gEfiEventReadyToBootGuid,          TPL_CALLBACK, NULL },
  //
  // Declare event notification on Legacy Boot Event Group.  This is used to inform the SMM Core that the platform 
  // is performing a legacy boot operation, and that the UEFI environment is no longer available and the SMM Core 
  // must guarantee that it does not access any UEFI related structures outside of SMRAM.
  //
  { FALSE, FALSE, &gEfiEventLegacyBootGuid,           SmmIplGuidedEventNotify,           &gEfiEventLegacyBootGuid,           TPL_CALLBACK, NULL },
  //
  // Declare event notification on SetVirtualAddressMap() Event Group.  This is used to convert gSmmCorePrivate 
  // and mSmmControl2 from physical addresses to virtual addresses.
  //
  { FALSE, FALSE, &gEfiEventVirtualAddressChangeGuid, SmmIplSetVirtualAddressNotify,     NULL,                               TPL_CALLBACK, NULL },
  //
  // Terminate the table of event notifications
  //
  { FALSE, FALSE, NULL,                               NULL,                              NULL,                               TPL_CALLBACK, NULL }
};

/**
  Indicate whether the driver is currently executing in the SMM Initialization phase.

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   InSmram                 Pointer to a Boolean which, on return, indicates that the driver is currently executing
                                   inside of SMRAM (TRUE) or outside of SMRAM (FALSE).

  @retval  EFI_INVALID_PARAMETER   InSmram was NULL.
  @retval  EFI_SUCCESS             The call returned successfully.

**/
EFI_STATUS
EFIAPI
SmmBase2InSmram (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      BOOLEAN                 *InSmram
  )
{
  if (InSmram == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *InSmram = gSmmCorePrivate->InMm;

  return EFI_SUCCESS;
}

/**
  Retrieves the location of the System Management System Table (SMST).

  @param   This                    The EFI_SMM_BASE2_PROTOCOL instance.
  @param   Smst                    On return, points to a pointer to the System Management Service Table (SMST).

  @retval  EFI_INVALID_PARAMETER   Smst or This was invalid.
  @retval  EFI_SUCCESS             The memory was returned to the system.
  @retval  EFI_UNSUPPORTED         Not in SMM.

**/
EFI_STATUS
EFIAPI
SmmBase2GetSmstLocation (
  IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
  OUT      EFI_SMM_SYSTEM_TABLE2   **Smst
  )
{
  if ((This == NULL) ||(Smst == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (!gSmmCorePrivate->InMm) {
    return EFI_UNSUPPORTED;
  }
  
  *Smst = (EFI_SMM_SYSTEM_TABLE2 *)(UINTN)gSmmCorePrivate->Mmst;

  return EFI_SUCCESS;
}

/**
  Communicates with a registered handler.
  
  This function provides a service to send and receive messages from a registered 
  UEFI service.  This function is part of the SMM Communication Protocol that may 
  be called in physical mode prior to SetVirtualAddressMap() and in virtual mode 
  after SetVirtualAddressMap().

  @param[in] This                The EFI_SMM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                 being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS            The message was successfully posted.
  @retval EFI_INVALID_PARAMETER  The CommBuffer was NULL.
**/
EFI_STATUS
EFIAPI
SmmCommunicationCommunicate (
  IN CONST EFI_SMM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize
  )
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *CommunicateHeader;
  BOOLEAN                     OldInSmm;
  EFI_SMM_SYSTEM_TABLE2       *Smst;
  
  //DEBUG ((EFI_D_INFO, "SmmCommunicationCommunicate Communicate Enter\n"));

  //
  // Check parameters
  //
  if ((CommBuffer == NULL) || (CommSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // CommSize must hold HeaderGuid and MessageLength
  //
  if (*CommSize < OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If not already in SMM, then generate a Software SMI
  //
  if (!gSmmCorePrivate->InMm && gSmmCorePrivate->MmEntryPointRegistered) {
    //
    // Put arguments for Software SMI in gSmmCorePrivate
    //
    gSmmCorePrivate->CommunicationBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer;
    gSmmCorePrivate->BufferSize          = (UINT64)*CommSize;

    //
    // Generate Software SMI
    //
    Status = mSmmControl2->Trigger (mSmmControl2, NULL, NULL, FALSE, 0);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    //
    // Return status from software SMI 
    //
    *CommSize = (UINTN)gSmmCorePrivate->BufferSize;

    Status = (EFI_STATUS)gSmmCorePrivate->ReturnStatus;
    if (Status != EFI_SUCCESS) {
      Status = Status | MAX_BIT;
    }
    
    //DEBUG ((EFI_D_INFO, "SmmCommunicationCommunicate Communicate Exit (%r)\n", Status));

    return Status;
  }

  //
  // If we are in SMM, then the execution mode must be physical, which means that
  // OS established virtual addresses can not be used.  If SetVirtualAddressMap()
  // has been called, then a direct invocation of the Software SMI is not 
  // not allowed so return EFI_INVALID_PARAMETER.
  //
  if (EfiGoneVirtual()) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If we are not in SMM, don't allow call SmiManage() directly when SMRAM is closed or locked.
  //
  if ((!gSmmCorePrivate->InMm) && (!mSmmAccess->OpenState || mSmmAccess->LockState)) {
    return EFI_INVALID_PARAMETER;
  }
 
  //
  // Save current InSmm state and set InSmm state to TRUE
  //
  OldInSmm = gSmmCorePrivate->InMm;
  gSmmCorePrivate->InMm = TRUE;

  //
  // Already in SMM and before SetVirtualAddressMap(), so call SmiManage() directly.
  //
  CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)CommBuffer;
  *CommSize -= OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
  Smst = (EFI_SMM_SYSTEM_TABLE2 *)(UINTN)gSmmCorePrivate->Mmst;
  Status = Smst->SmiManage (
                   &CommunicateHeader->HeaderGuid, 
                   NULL, 
                   CommunicateHeader->Data, 
                   CommSize
                   );

  //
  // Update CommunicationBuffer, BufferSize and ReturnStatus
  // Communicate service finished, reset the pointer to CommBuffer to NULL
  //
  *CommSize += OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);

  //
  // Restore original InSmm state
  //
  gSmmCorePrivate->InMm = OldInSmm;

  return (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Event notification that is fired when GUIDed Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplGuidedEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;

  //
  // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure 
  //
  CopyGuid (&CommunicateHeader.HeaderGuid, (EFI_GUID *)Context);
  CommunicateHeader.MessageLength = 1;
  CommunicateHeader.Data[0] = 0;

  //
  // Generate the Software SMI and return the result
  //
  Size = sizeof (CommunicateHeader);
  SmmCommunicationCommunicate (&mSmmCommunication, &CommunicateHeader, &Size);
}

/**
  Event notification that is fired when DxeDispatch Event Group is signaled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplDxeDispatchEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;
  EFI_STATUS                  Status;

  //
  // Keep calling the SMM Core Dispatcher until there is no request to restart it.
  //
  while (TRUE) {
    //
    // Use Guid to initialize EFI_SMM_COMMUNICATE_HEADER structure
    // Clear the buffer passed into the Software SMI.  This buffer will return
    // the status of the SMM Core Dispatcher.
    //
    CopyGuid (&CommunicateHeader.HeaderGuid, (EFI_GUID *)Context);
    CommunicateHeader.MessageLength = 1;
    CommunicateHeader.Data[0] = 0;

    //
    // Generate the Software SMI and return the result
    //
    Size = sizeof (CommunicateHeader);
    SmmCommunicationCommunicate (&mSmmCommunication, &CommunicateHeader, &Size);
    
    //
    // Return if there is no request to restart the SMM Core Dispatcher
    //
    if (CommunicateHeader.Data[0] != COMM_BUFFER_MM_DISPATCH_RESTART) {
      return;
    }

    //
    // Close all SMRAM ranges to protect SMRAM
    //
    Status = mSmmAccess->Close (mSmmAccess);
    ASSERT_EFI_ERROR (Status);

    //
    // Print debug message that the SMRAM window is now closed.
    //
    DEBUG ((DEBUG_INFO, "SMM IPL closed SMRAM window\n"));
  }
}

/**
  Event notification that is fired every time a DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmIplReadyToLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;
  UINTN       Index;

  //
  // See if we are already locked
  //
  if (mSmmLocked) {
    return;
  }
  
  //
  // Make sure this notification is for this handler
  //
  if (CompareGuid ((EFI_GUID *)Context, &gEfiDxeSmmReadyToLockProtocolGuid)) {
    Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, &Interface);
    if (EFI_ERROR (Status)) {
      return;
    }
  } else {
    //
    // If SMM is not locked yet and we got here from gEfiEventReadyToBootGuid being 
    // signalled, then gEfiDxeSmmReadyToLockProtocolGuid was not installed as expected.
    // Print a warning on debug builds.
    //
    DEBUG ((DEBUG_WARN, "SMM IPL!  DXE SMM Ready To Lock Protocol not installed before Ready To Boot signal\n"));
  }

  //
  // Lock the SMRAM (Note: Locking SMRAM may not be supported on all platforms)
  //
  mSmmAccess->Lock (mSmmAccess);
  
  //
  // Close protocol and event notification events that do not apply after the 
  // DXE SMM Ready To Lock Protocol has been installed or the Ready To Boot 
  // event has been signalled.
  //
  for (Index = 0; mSmmIplEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmIplEvents[Index].CloseOnLock) {
      gBS->CloseEvent (mSmmIplEvents[Index].Event);
    }
  }

  //
  // Inform SMM Core that the DxeSmmReadyToLock protocol was installed
  //
  SmmIplGuidedEventNotify (Event, (VOID *)&gEfiDxeSmmReadyToLockProtocolGuid);

  //
  // Print debug message that the SMRAM window is now locked.
  //
  DEBUG ((DEBUG_INFO, "SMM IPL locked SMRAM window\n"));
  
  //
  // Set flag so this operation will not be performed again
  //
  mSmmLocked = TRUE;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmIplSetVirtualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mSmmControl2);
  EfiConvertPointer (0x0, (VOID **)&gSmmCorePrivate);
}

VOID
SmmIplNotifyUefiInfo (
  VOID
  )
{
  EFI_MM_COMMUNICATE_UEFI_INFO     CommunicationUefiInfo;
  UINTN                            Size;

  DEBUG((EFI_D_INFO, "SmmIplNotifyUefiInfo\n"));

  CopyGuid (&CommunicationUefiInfo.HeaderGuid, &gMmUefiInfoGuid);
  CommunicationUefiInfo.MessageLength = sizeof(EFI_MM_COMMUNICATE_UEFI_INFO);
  CommunicationUefiInfo.Data.EfiSystemTable = (EFI_PHYSICAL_ADDRESS)(UINTN)gST;

  //
  // Generate the Software SMI and return the result
  //
  Size = sizeof (CommunicationUefiInfo);
  SmmCommunicationCommunicate (NULL, &CommunicationUefiInfo, &Size);
}

VOID
SmmIplDispatchFv (
  VOID
  )
{
  EFI_MM_COMMUNICATE_FV_DISPATCH   CommunicationFvDispatch;
  UINTN                            Size;
  EFI_PEI_HOB_POINTERS             Hob;

  DEBUG((EFI_D_INFO, "SmmIplDispatchFv\n"));

  for (Hob.Raw = GetHobList(); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV2) {
      CopyGuid (&CommunicationFvDispatch.HeaderGuid, &gMmFvDispatchGuid);
      CommunicationFvDispatch.MessageLength = sizeof(EFI_MM_COMMUNICATE_FV_DISPATCH_DATA);
      CommunicationFvDispatch.Data.Address = Hob.FirmwareVolume2->BaseAddress;
      CommunicationFvDispatch.Data.Size = Hob.FirmwareVolume2->Length;
      DEBUG((EFI_D_INFO, "Fv Base - 0x%lx, Length - 0x%lx\n",
        Hob.FirmwareVolume2->BaseAddress,
        Hob.FirmwareVolume2->Length
        ));

      //
      // Generate the Software SMI and return the result
      //
      Size = sizeof (CommunicationFvDispatch);
      SmmCommunicationCommunicate (NULL, &CommunicationFvDispatch, &Size);
    }
  }
}

VOID
SmmIplDispatchDriver (
  VOID
  )
{
  EFI_SMM_COMMUNICATE_HEADER  CommunicateHeader;
  UINTN                       Size;

  DEBUG((EFI_D_INFO, "SmmIplDispatchDriver\n"));

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
    SmmCommunicationCommunicate (NULL, &CommunicateHeader, &Size);
      
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

  Load SMM Core into SMRAM, register SMM Core entry point for SMIs, install 
  SMM Base 2 Protocol and SMM Communication Protocol, and register for the 
  critical events required to coordinate between DXE and SMM environments.
  
  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmIplEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  VOID                            *Registration;
  EFI_HOB_GUID_TYPE               *GuidHob;
  MM_CORE_DATA_HOB_DATA           *DataInHob;
  
  GuidHob = GetFirstGuidHob (&gMmCoreDataHobGuid);
  if (GuidHob == NULL) {
    return EFI_UNSUPPORTED;
  }
  DataInHob       = GET_GUID_HOB_DATA (GuidHob);
  gSmmCorePrivate = (MM_CORE_PRIVATE_DATA *)(UINTN)DataInHob->Address;

  DEBUG((EFI_D_INFO, "gSmmCorePrivate - 0x%x\n", gSmmCorePrivate));

  //
  // Get SMM Access Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&mSmmAccess);
  ASSERT_EFI_ERROR (Status);

  //
  // Get SMM Control2 Protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmControl2ProtocolGuid, NULL, (VOID **)&mSmmControl2);
  ASSERT_EFI_ERROR (Status);

  //
  // Install SMM Base2 Protocol and SMM Communication Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmmIplHandle,
                  &gEfiSmmBase2ProtocolGuid,         &mSmmBase2,
                  &gEfiSmmCommunicationProtocolGuid, &mSmmCommunication,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create the set of protocol and event notififcations that the SMM IPL requires
  //
  for (Index = 0; mSmmIplEvents[Index].NotifyFunction != NULL; Index++) {
    if (mSmmIplEvents[Index].Protocol) {
      mSmmIplEvents[Index].Event = EfiCreateProtocolNotifyEvent (
                                     mSmmIplEvents[Index].Guid,
                                     mSmmIplEvents[Index].NotifyTpl,
                                     mSmmIplEvents[Index].NotifyFunction,
                                     mSmmIplEvents[Index].NotifyContext,
                                    &Registration
                                    );
    } else {
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      mSmmIplEvents[Index].NotifyTpl,
                      mSmmIplEvents[Index].NotifyFunction,
                      mSmmIplEvents[Index].NotifyContext,
                      mSmmIplEvents[Index].Guid,
                      &mSmmIplEvents[Index].Event
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Notify UEFI information
  //
  SmmIplNotifyUefiInfo ();

  //
  // Dispatch FV
  //
  SmmIplDispatchFv ();
  
  //
  // Dispatch Driver
  //
  SmmIplDispatchDriver ();

  return EFI_SUCCESS;
}
