;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

STACK_FSP_INFO_HDR_OFFSET    EQU   3 * 8
STACK_SAVED_RAX_OFFSET       EQU   4 * 8

;
; Define SSE macros using SSE 4.1 instructions
; args 1:XMM, 2:IDX, 3:REG
%macro PUSHAD  0
       push    0    ; Idt
       push    0
       sidt    [rsp]

       sub     rsp, 0x10 * 10
       movdqu  [rsp + 0x00], xmm6
       movdqu  [rsp + 0x10], xmm7
       movdqu  [rsp + 0x20], xmm8
       movdqu  [rsp + 0x30], xmm9
       movdqu  [rsp + 0x40], xmm10
       movdqu  [rsp + 0x50], xmm11
       movdqu  [rsp + 0x60], xmm12
       movdqu  [rsp + 0x70], xmm13
       movdqu  [rsp + 0x80], xmm14
       movdqu  [rsp + 0x90], xmm15

       push    r15
       push    r14
       push    r13
       push    r12
       push    r11
       push    r10
       push    r9
       push    r8

       pushfq
       push    rbp
       push    rdi
       push    rsi
       push    rdx
       push    rcx
       push    rbx
       push    rax

       %endmacro

;
; Following functions will be provided in C
;
extern ASM_PFX(Loader2PeiSwitchStack)
extern ASM_PFX(FspApiCallingCheck)

;
; Following functions will be provided in ASM
;
extern ASM_PFX(FspApiCommonContinue)
extern ASM_PFX(AsmGetFspInfoHeader)

;----------------------------------------------------------------------------
; FspApiCommon API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
global ASM_PFX(FspApiCommon)
ASM_PFX(FspApiCommon):
  ;
  ; RAX holds the API index
  ;

  ;
  ; Stack must be ready
  ;
  push   rax
  add    rsp, 8
  cmp    rax, [rsp - 8]
  jz     FspApiCommon1
  mov    rax, 08000000000000003h
  jmp    exit

FspApiCommon1:
  ;
  ; Verify the calling condition
  ;
  PUSHAD
  push   0    ; FspInfoHeader
  push   0    ; ApiRet
  push   rdx  ; ApiParam 2
  push   rcx  ; ApiParam 1

  mov    rbp, rsp

  mov    rsi, rax
  mov    rdx, rcx  ; UPD Pointer
  mov    rcx, rsi  ; API Index
  call   ASM_PFX(FspApiCallingCheck)
  cmp    rax, 0
  mov    rax, rsi
  jz     FspApiCommon2
  mov    [rbp + STACK_SAVED_RAX_OFFSET], rax
exit:
  ret

FspApiCommon2:
  cmp    rax, 3   ;  FspMemoryInit API
  jz     FspApiCommon3

  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    [rbp + STACK_FSP_INFO_HDR_OFFSET], rax
  jmp    ASM_PFX(Loader2PeiSwitchStack)

FspApiCommon3:
  jmp    ASM_PFX(FspApiCommonContinue)
