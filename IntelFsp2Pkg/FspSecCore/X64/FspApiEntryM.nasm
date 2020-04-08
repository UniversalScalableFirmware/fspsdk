;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
    DEFAULT REL
    SECTION .text

STACK_FSP_INFO_HDR_OFFSET    EQU   3 * 8
STACK_API_PARAM1_OFFSET      EQU   0 * 8

FSP_HEADER_IMGBASE_OFFSET    EQU   1Ch
FSP_HEADER_CFGREG_OFFSET     EQU   24h

struc FSPM_UPD_COMMON
    ; FSP_UPD_HEADER {
    .FspUpdHeader:            resd    8
    ; }
    ; FSPM_ARCH_UPD {
    .Revision:                resb    1
    .Reserved:                resb    3
    .NvsBufferPtr:            resd    1
    .StackBase:               resd    1
    .StackSize:               resd    1
    .BootLoaderTolumSize:     resd    1
    .BootMode:                resd    1
    .Reserved1:               resb    8
    ; }
    .size:
endstruc

extern  ASM_PFX(SecStartup)
extern  ASM_PFX(FspApiCommon)
extern  ASM_PFX(AsmGetFspInfoHeader)
extern  ASM_PFX(AsmGetFspBaseAddress)
extern  ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))

global ASM_PFX(FspPeiCoreEntryOff)
ASM_PFX(FspPeiCoreEntryOff):
   ;
   ; This value will be pached by the build script
   ;
   DD    0x12345678

global ASM_PFX(AsmGetPeiCoreOffset)
ASM_PFX(AsmGetPeiCoreOffset):
   lea   rax, [ASM_PFX(FspPeiCoreEntryOff)]
   mov   eax, [rax]
   ret

;----------------------------------------------------------------------------
; FspApiCommonContinue API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
global ASM_PFX(FspApiCommonContinue)
ASM_PFX(FspApiCommonContinue):
  ;
  ; EAX holds the API index
  ;

  ;
  ; Update the FspInfoHeader pointer
  ;
  push   rax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    [rbp + STACK_FSP_INFO_HDR_OFFSET], rax
  pop    rax

  ;
  ; Create a Task Frame in the stack for the Boot Loader
  ;


  ;  Get Stackbase and StackSize from FSPM_UPD Param
  mov    rdx, [rbp + STACK_API_PARAM1_OFFSET]
  cmp    rdx, 0
  jnz    FspStackSetup

  ; Get UPD default values if FspmUpdDataPtr (ApiParam1) is null
  xchg   rbx, rax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    edx, [rax + FSP_HEADER_IMGBASE_OFFSET]
  add    edx, [rax + FSP_HEADER_CFGREG_OFFSET]
  xchg   rbx, rax

FspStackSetup:
  ;
  ; StackBase = temp memory base, StackSize = temp memory size
  ;
  mov    edi, [rdx + FSPM_UPD_COMMON.StackBase]
  mov    ecx, [rdx + FSPM_UPD_COMMON.StackSize]

  ;
  ; Keep using bootloader stack if heap size % is 0
  ;
  lea    rbx, [ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))]
  mov    bl,  [rbx]
  cmp    bl,  0
  jz     SkipStackSwitch

  ;
  ; Set up a dedicated temp ram stack for FSP if FSP heap size % doesn't equal 0
  ;
  add    rdi, rcx
  ;
  ; Switch to new FSP stack
  ;
  xchg   rdi, rsp                                ; Exchange edi and esp, edi will be assigned to the current esp pointer and esp will be Stack base + Stack size

SkipStackSwitch:
  ;
  ; If heap size % is 0:
  ;   EDI is FSPM_UPD_COMMON.StackBase and will hold ESP later (boot loader stack pointer)
  ;   ECX is FSPM_UPD_COMMON.StackSize
  ;   ESP is boot loader stack pointer (no stack switch)
  ;   BL  is 0 to indicate no stack switch (EBX will hold FSPM_UPD_COMMON.StackBase later)
  ;
  ; If heap size % is not 0
  ;   EDI is boot loader stack pointer
  ;   ECX is FSPM_UPD_COMMON.StackSize
  ;   ESP is new stack (FSPM_UPD_COMMON.StackBase + FSPM_UPD_COMMON.StackSize)
  ;   BL  is NOT 0 to indicate stack has switched
  ;
  cmp    bl, 0
  jnz    StackHasBeenSwitched

  mov    rbx, rdi                                ; Put FSPM_UPD_COMMON.StackBase to ebx as temp memory base
  mov    rdi, rsp                                ; Put boot loader stack pointer to edi
  jmp    StackSetupDone

StackHasBeenSwitched:
  mov    rbx, rsp                                ; Put Stack base + Stack size in ebx
  sub    rbx, rcx                                ; Stack base + Stack size - Stack size as temp memory base

StackSetupDone:

  ;
  ; Pass stack base and size into the PEI Core
  ;
  mov     rcx,  rcx
  mov     rdx,  rbx

  ;
  ; Pass the API Idx to SecStartup
  ;
  push    rax

  ;
  ; Pass the BootLoader stack to SecStartup
  ;
  push    rdi

  ;
  ; Pass entry point of the PEI core
  ;
  call    ASM_PFX(AsmGetFspBaseAddress)
  mov     r8, rax
  call    ASM_PFX(AsmGetPeiCoreOffset)
  lea     r9,  [r8 + rax]

  ;
  ; Pass Control into the PEI Core
  ;
  push    r9
  push    r8
  push    rdx
  push    rcx
  call    ASM_PFX(SecStartup)

  jmp     $
exit:
  ret


;----------------------------------------------------------------------------
; FspMemoryInit API
;
; This FSP API is called after TempRamInit and initializes the memory.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspMemoryInitApi)
ASM_PFX(FspMemoryInitApi):
  mov    rax,  3 ; FSP_API_INDEX.FspMemoryInitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; TempRamExitApi API
;
; This API tears down temporary RAM
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamExitApi)
ASM_PFX(TempRamExitApi):
  mov    eax,  4 ; FSP_API_INDEX.TempRamExitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; Module Entrypoint API
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp  $
  ; Add reference to APIs so that it will not be optimized by compiler
  jmp  ASM_PFX(FspMemoryInitApi)
  jmp  ASM_PFX(TempRamExitApi)
