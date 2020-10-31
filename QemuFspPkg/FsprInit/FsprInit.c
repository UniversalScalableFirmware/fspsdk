/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <FsprInit.h>

const
FSPT_UPD TempRamInitParams = {
  .FspUpdHeader = {
    .Signature = FSPT_UPD_SIGNATURE,
    .Revision  = 1,
    .Reserved  = {0},
  },
  .FsptCommonUpd = {
    .Revision              = 1,
    .MicrocodeRegionBase   = 0,
    .MicrocodeRegionLength = 0,
    .CodeRegionBase        = 0xFF000000,
    .CodeRegionLength      = 0x00000000,
  },
  .UpdTerminator = 0x55AA,
};

const FSP_INIT_PHASE mNotify[] = {
  EnumInitPhaseAfterPciEnumeration,
  EnumInitPhaseReadyToBoot,
  EnumInitPhaseEndOfFirmware
};

// SplitFspBin.py rebase  -f  FspRel.bin -c t m s -b 0xFFFC7000 0xFFFA5000 0xFFF90000
const UINT32 mFsptBaseAddr = FixedPcdGet32(PcdFlashFvFsptBase) + FixedPcdGet32(PcdFspAreaBaseAddress);
const UINT32 mFspmBaseAddr = FixedPcdGet32(PcdFlashFvFspmBase) + FixedPcdGet32(PcdFspAreaBaseAddress);
const UINT32 mFspsBaseAddr = FixedPcdGet32(PcdFlashFvFspsBase) + FixedPcdGet32(PcdFspAreaBaseAddress);


//
// Global Descriptor Table (GDT)
//
STATIC
CONST IA32_SEGMENT_DESCRIPTOR
mGdtEntries[STAGE_GDT_ENTRY_COUNT] = {
  /* selector { Global Segment Descriptor                              } */
  /* 0x00 */  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, //null descriptor
  /* 0x08 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear data segment descriptor
  /* 0x10 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //linear code segment descriptor
  /* 0x18 */  {{0xffff, 0,  0,  0x3,  1,  0,  1,  0xf,  0,  0, 1,  1,  0}}, //system data segment descriptor
  /* 0x20 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0xf,  0,  1, 0,  1,  0}}, //linear code (64-bit) segment descriptor
  /* 0x28 */  {{0xffff, 0,  0,  0xb,  1,  0,  1,  0x0,  0,  0, 0,  0,  0}}, //16-bit code segment descriptor
  /* 0x30 */  {{0xffff, 0,  0,  0x2,  1,  0,  1,  0x0,  0,  0, 0,  0,  0}}, //16-bit data segment descriptor
};

//
// IA32 Gdt register
//
STATIC
CONST IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };

CONST UINT32 mErrorCodeFlag = 0x00027d00;

/**
  This function sets the Loader global data pointer.

  @param[in] LoaderData       Loader data pointer.

**/
VOID
EFIAPI
SetLoaderGlobalDataPointer (
  IN LOADER_GLOBAL_DATA   *LoaderData
  )
{
  IA32_DESCRIPTOR        Idtr;

  AsmReadIdtr (&Idtr);
  (* (UINTN *) (Idtr.Base - sizeof (UINT64))) = (UINTN)LoaderData;
}

/**
  This function gets the Loader global data pointer.

**/
LOADER_GLOBAL_DATA *
EFIAPI
GetLoaderGlobalDataPointer (
  VOID
  )
{
  IA32_DESCRIPTOR     Idtr;

  AsmReadIdtr (&Idtr);
  return (LOADER_GLOBAL_DATA *) (* (UINTN *) (Idtr.Base - sizeof (UINT64)));
}

/**
  Common exception handler.

  It will print out the location where exception occured and then halt the system.
  This function will never return.

  @param[in] Stack          Current stack address pointer.
  @param[in] ExceptionType  Exception type code.
**/
VOID
EFIAPI
CommonExceptionHandler (
  IN UINT32        *Stack,
  IN UINT8          ExceptionType
  )
{
  UINT32  *Ptr;

  // Skip the ExceptionType on the stack
  Ptr = Stack + 1;

  // Skip the ErrorCode on the stack
  if (ExceptionType < CPU_EXCEPTION_NUM) {
    if (mErrorCodeFlag & (1 << ExceptionType)) {
      Ptr++;
    }
  }

  DEBUG ((DEBUG_ERROR, "Exception #%d from 0x%04X:0x%08X !!!\n", ExceptionType, Ptr[1], Ptr[0]));
  CpuDeadLoop ();
}

/**
  Update exception handler in IDT table .

  This function is used to update the IDT exception handler with current stage.

  @param[in]  IdtDescritor   If non-zero, it is new IDT descriptor to be updated.
                             if it is 0,  the IDT descriptor will be retrieved from IDT base register.

**/
STATIC
VOID
UpdateExceptionHandler (
  IN VOID                    *IdtDescriptor
)
{
  UINT64                     *IdtTable;
  IA32_IDT_GATE_DESCRIPTOR    IdtGateDescriptor;
  UINT32                      Index;
  UINT32                      Address;
  IA32_DESCRIPTOR             Idtr;
  EXCEPTION_HANDLER_TEMPLATE_MAP  TemplateMap;

  if (IdtDescriptor == NULL) {
    AsmReadIdtr (&Idtr);
  } else {
    Idtr = *(IA32_DESCRIPTOR *) IdtDescriptor;
  }
  IdtTable                          = (UINT64 *)Idtr.Base;
  IdtGateDescriptor.Uint64          = 0;
  IdtGateDescriptor.Bits.Selector   = AsmReadCs ();
  IdtGateDescriptor.Bits.GateType   = IA32_IDT_GATE_TYPE_INTERRUPT_32;

  AsmGetTemplateAddressMap (&TemplateMap);
  for (Index = 0; Index < CPU_EXCEPTION_NUM; Index ++) {
    Address = TemplateMap.ExceptionStart + TemplateMap.ExceptionStubHeaderSize * Index;
    IdtGateDescriptor.Bits.OffsetHigh = (UINT16)(Address >> 16);
    IdtGateDescriptor.Bits.OffsetLow  = (UINT16)Address;
    IdtTable[Index] = IdtGateDescriptor.Uint64;
  }

  if (IdtDescriptor != NULL) {
    AsmWriteIdtr (&Idtr);
  }
}

/**
  Load IDT table for current processor.

  It function initializes the exception handlers in IDT table and
  loads it into current processor.  It also try to store a specific DWORD
  data just before actual IDT base. It is used to set/get bootloader
  global data structure pointer.

  @param  IdtTable  Contain the IDT table pointer.
                    It must point to a STAGE_IDT_TABLE buffer preallocated.

  @param  Data      It contains a DWORD data that will be stored just before the actual IDT table.
                    It can be used to set/get global data using IDT pointer.

**/
VOID
LoadIdt (
  IN STAGE_IDT_TABLE   *IdtTable,
  IN UINTN              Data
  )
{
  IA32_DESCRIPTOR             Idtr;

  IdtTable->LdrGlobal  = Data;

  Idtr.Base  = (UINTN) &IdtTable->IdtTable;
  Idtr.Limit = (UINT16) (sizeof (IdtTable->IdtTable) - 1);
  UpdateExceptionHandler (&Idtr);
}

/**
  Copy GDT to memory and Load GDT table for current processor.

  It function initializes GDT table and loads it into current processor.

  @param[in]  GdtTable  Pointer to STAGE_GDT_TABLE structure to fill the GDT.
  @param[in]  GdtrPtr   Pointer to the source IA32_DESCRIPTOR structure.

**/
VOID
LoadGdt (
  IN STAGE_GDT_TABLE   *GdtTable,
  IN IA32_DESCRIPTOR   *GdtrPtr
  )
{
  IA32_DESCRIPTOR     Gdtr;
  UINT32              GdtLen;

  if (GdtrPtr == NULL) {
    AsmReadGdtr (&Gdtr);
    GdtrPtr = &Gdtr;
  }

  GdtLen = sizeof(GdtTable->GdtTable);
  if (GdtLen > (UINTN)GdtrPtr->Limit + 1) {
    GdtLen = GdtrPtr->Limit + 1;
  }
  CopyMem (GdtTable->GdtTable, (VOID *)GdtrPtr->Base, GdtLen);

  Gdtr.Base  = (UINTN) GdtTable->GdtTable;
  Gdtr.Limit = (UINT16) (GdtLen - 1);
  AsmWriteGdtr (&Gdtr);
}

/**

  Entry point to the C language phase of Stage1A.

  After the Stage1A assembly code has initialized some temporary memory and set
  up the stack, control is transferred to this function.
  - Initialize the global data
  - Do post TempRaminit board initialization.
  - Relocate by itself stage1A code to temp memory and execute.
  - CPU halted if relocation fails.

  @param[in] Params            Pointer to stage specific parameters.

**/
VOID
EFIAPI
SecStartup (
  IN VOID  *Params
  )
{
  LOADER_GLOBAL_DATA        LdrGlobalData;
  STAGE_IDT_TABLE           IdtTable;
  STAGE_GDT_TABLE           GdtTable;
  LOADER_GLOBAL_DATA       *LdrGlobal;
  LOADER_GLOBAL_DATA       *OldLdrGlobal;
  STAGE1A_ASM_PARAM        *Stage1aAsmParam;
  UINT32                    StackTop;
  VOID                     *HobList;
  UINT32                    FspReservedMemBase;
  UINT64                    FspReservedMemSize;
  UINT32                    MemPoolStart;
  UINT32                    MemPoolEnd;
  UINT32                    MemPoolCurrTop;
  UINT32                    LoaderReservedMemSize;
  UINT32                    LoaderHobStackSize;
  EFI_STATUS                Status;
  STAGE_IDT_TABLE          *IdtTablePtr;
  STAGE_GDT_TABLE          *GdtTablePtr;

  Stage1aAsmParam = (STAGE1A_ASM_PARAM *)Params;

  // Init global data
  LdrGlobal = &LdrGlobalData;
  ZeroMem (LdrGlobal, sizeof (LOADER_GLOBAL_DATA));
  StackTop = (UINT32)(UINTN)Params + sizeof (STAGE1A_ASM_PARAM);
  LdrGlobal->Signature             = LDR_GDATA_SIGNATURE;
  LdrGlobal->StackTop              = StackTop;
  LdrGlobal->MemPoolEnd            = StackTop + 0xE000;
  LdrGlobal->MemPoolStart          = StackTop;
  LdrGlobal->MemPoolCurrTop        = LdrGlobal->MemPoolEnd;
  LdrGlobal->MemPoolCurrBottom     = LdrGlobal->MemPoolStart;
  LdrGlobal->DebugPrintErrorLevel  = 0x8000004F;

  LoadGdt (&GdtTable, (IA32_DESCRIPTOR *)&mGdt);
  LoadIdt (&IdtTable, (UINT32)(UINTN)LdrGlobal);
  SetLoaderGlobalDataPointer (LdrGlobal);

  DEBUG ((DEBUG_INFO, "\n============= FSP-R Entry =============\n\n"));

  Status = CallFspMemoryInit (mFspmBaseAddr, &HobList);
  ASSERT_EFI_ERROR (Status);

  // Need to switch to new stack
  FspReservedMemBase = (UINT32)GetFspReservedMemoryFromGuid (
                         HobList,
                         &FspReservedMemSize,
                         &gFspReservedMemoryResourceHobGuid
                         );
  ASSERT (FspReservedMemBase > 0);

  // Prepare Global Data structure
  LoaderReservedMemSize = 0x100000;
  LoaderHobStackSize    = 0x001000;

  OldLdrGlobal   = LdrGlobal;
  MemPoolStart   = FspReservedMemBase - LoaderReservedMemSize;
  MemPoolEnd     = FspReservedMemBase - LoaderHobStackSize;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolEnd - sizeof (LOADER_GLOBAL_DATA), 0x10);
  LdrGlobal      = (LOADER_GLOBAL_DATA *)(UINTN)MemPoolCurrTop;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolCurrTop - sizeof (STAGE_IDT_TABLE), 0x10);
  IdtTablePtr    = (STAGE_IDT_TABLE *)(UINTN)MemPoolCurrTop;
  MemPoolCurrTop = ALIGN_DOWN (MemPoolCurrTop - sizeof (STAGE_GDT_TABLE), 0x10);
  GdtTablePtr    = (STAGE_GDT_TABLE *)(UINTN)MemPoolCurrTop;
  CopyMem (LdrGlobal, OldLdrGlobal, sizeof (LOADER_GLOBAL_DATA));

  LdrGlobal->FspHobList        = HobList;
  LdrGlobal->StackTop          = FspReservedMemBase;
  LdrGlobal->MemPoolEnd        = MemPoolEnd;
  LdrGlobal->MemPoolStart      = MemPoolStart;
  LdrGlobal->MemPoolCurrTop    = MemPoolCurrTop;
  LdrGlobal->MemPoolCurrBottom = MemPoolStart;
  LdrGlobal->MemUsableTop      = (UINT32)(FspReservedMemBase + FspReservedMemSize);

  // Setup global data in memory
  LoadGdt (GdtTablePtr, NULL);
  LoadIdt (IdtTablePtr, (UINT32)(UINTN)LdrGlobal);
  SetLoaderGlobalDataPointer (LdrGlobal);
  DEBUG ((DEBUG_INFO, "Loader global data @ 0x%08X\n", (UINT32)(UINTN)LdrGlobal));

  // Setup new stack and continue
  StackTop  = LdrGlobal->StackTop;
  StackTop  = ALIGN_DOWN (StackTop, 0x100);
  DEBUG ((DEBUG_INFO, "Switch to memory stack @ 0x%08X\n", StackTop));
  SwitchStack (ContinueFunc, HobList, NULL, (VOID *)(UINTN)StackTop);
}



/**
  Continue Stage 1B execution.

  This function will continue Stage1B execution with a new memory-based stack.

  @param[in]  Context1        Pointer to STAGE1B_PARAM in main memory.
  @param[in]  Context2        Unused.

**/
VOID
EFIAPI
ContinueFunc (
  IN VOID                      *Context1,
  IN VOID                      *Context2
  )
{
  UINT32       Idx;
  EFI_STATUS   Status;
  UINT32      *OemFvBase;
  VOID        *OemEntry;
  VOID        *HobList;

  HobList = Context1;

  Status = CallFspTempRamExit (mFspmBaseAddr, NULL);
  ASSERT_EFI_ERROR (Status);

  Status = CallFspSiliconInit ();
  ASSERT_EFI_ERROR (Status);

  for (Idx = 0; Idx < 3; Idx++) {
    Status = CallFspNotifyPhase (mNotify[Idx]);
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "HobList is located at 0x%08x\n", HobList));
  DEBUG ((DEBUG_INFO, "\n============= FSP-R Exit =============\n"));

  DEBUG ((DEBUG_INFO, "\nJump into OEM entry\n"));

  OemFvBase = (UINT32 *)(*(UINT32 *)(FixedPcdGet32(PcdFspAreaBaseAddress) - 4));
  OemEntry  = (VOID *)OemFvBase[0];
  JumpToOemEntry (OemEntry, HobList, 0, 0x80000);

  while (1);
}