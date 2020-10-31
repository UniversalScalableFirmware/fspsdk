/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef  _BOOT_FW_H_
#define  _BOOT_FW_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <FspUpd.h>
#include <FsptUpd.h>
#include <FspmUpd.h>
#include <FspsUpd.h>


#define  ALIGN_UP(address, align)     (((address) + ((align) - 1)) & ~((align)-1))
#define  ALIGN_DOWN(address, align)   ((address) & ~((align)-1))


#define  CPU_EXCEPTION_NUM           32
#define  CPU_INTERRUPT_NUM           256

#define  STAGE_IDT_ENTRY_COUNT       34
#define  STAGE_GDT_ENTRY_COUNT       7

typedef struct {
  UINT64        LdrGlobal;
  IA32_IDT_GATE_DESCRIPTOR  IdtTable[STAGE_IDT_ENTRY_COUNT];
} STAGE_IDT_TABLE;

typedef struct {
  IA32_SEGMENT_DESCRIPTOR   GdtTable[STAGE_GDT_ENTRY_COUNT];
} STAGE_GDT_TABLE;

//
// Record exception handler information
//
typedef struct {
  UINTN ExceptionStart;
  UINTN ExceptionStubHeaderSize;
} EXCEPTION_HANDLER_TEMPLATE_MAP;

#define  LDR_GDATA_SIGNATURE  SIGNATURE_32('L', 'D', 'R', 'G')

typedef struct {
  UINT32            Signature;
  UINT8             BootMode;
  UINT32            StackTop;
  UINT32            MemPoolEnd;
  UINT32            MemPoolStart;
  UINT32            MemPoolCurrTop;
  UINT32            MemPoolCurrBottom;
  UINT32            MemUsableTop;
  UINT32            DebugPrintErrorLevel;
  VOID             *FspHobList;
} LOADER_GLOBAL_DATA;


typedef struct {
  UINT32        CarBase;
  UINT32        CarTop;
  UINT64        TimeStamp;
  UINT32        HobList;
  struct _STATUS {
    UINT32      CpuBist           :  1;
    UINT32      StackOutOfRange   :  1;
    UINT32      RsvdBits          : 30;
  } Status;
} STAGE1A_ASM_PARAM;


extern const UINT32 mFsptBaseAddr;
extern const UINT32 mFspmBaseAddr;
extern const UINT32 mFspsBaseAddr;

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
  );

VOID
EFIAPI
JumpToOemEntry (
  IN VOID    *OemEntry,
  IN VOID    *HobList,
  IN UINT32   NemBase,
  IN UINT32   NemSize
  );

/**
  Return address map of exception handler template so that C code can generate
  exception tables.

  @param AddressMap[in, out]  Pointer to a buffer where the address map is returned.
**/
VOID
EFIAPI
AsmGetTemplateAddressMap (
  OUT EXCEPTION_HANDLER_TEMPLATE_MAP *AddressMap
  );

EFI_STATUS
EFIAPI
CallFspMemoryInit (
  UINT32                     FspmBase,
  VOID                       **HobList
  );

EFI_STATUS
EFIAPI
CallFspTempRamExit (
  UINT32               FspmBase,
  VOID                *Params
  );

EFI_STATUS
EFIAPI
CallFspSiliconInit (
  VOID
  );

EFI_STATUS
EFIAPI
CallFspNotifyPhase (
  FSP_INIT_PHASE  Phase
  );

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
  );

#endif