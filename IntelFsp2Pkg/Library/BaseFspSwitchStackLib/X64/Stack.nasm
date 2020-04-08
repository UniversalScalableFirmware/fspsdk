;------------------------------------------------------------------------------
;
; Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------

    SECTION .text



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


%macro POPAD   0

       pop     rax
       pop     rbx
       pop     rcx
       pop     rdx
       pop     rsi
       pop     rdi
       pop     rbp
       popfq

       pop     r8
       pop     r9
       pop     r10
       pop     r11
       pop     r12
       pop     r13
       pop     r14
       pop     r15

       movdqu  xmm6,  [rsp + 0x00]
       movdqu  xmm7,  [rsp + 0x10]
       movdqu  xmm8,  [rsp + 0x20]
       movdqu  xmm9,  [rsp + 0x30]
       movdqu  xmm10, [rsp + 0x40]
       movdqu  xmm11, [rsp + 0x50]
       movdqu  xmm12, [rsp + 0x60]
       movdqu  xmm13, [rsp + 0x70]
       movdqu  xmm14, [rsp + 0x80]
       movdqu  xmm15, [rsp + 0x90]
       add     rsp, 0x10 * 10

       lidt    [rsp]
       add     rsp, 16

       %endmacro

extern ASM_PFX(SwapStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Pei2LoaderSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Pei2LoaderSwitchStack)
ASM_PFX(Pei2LoaderSwitchStack):
    xor     rax, rax
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Loader2PeiSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(Loader2PeiSwitchStack)
ASM_PFX(Loader2PeiSwitchStack):
    jmp     ASM_PFX(FspSwitchStack)

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; FspSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
global ASM_PFX(FspSwitchStack)
ASM_PFX(FspSwitchStack):
    ; Save current contexts
    cmp     rax, 0
    jnz     SkipSaveContents

    PUSHAD
    sub     rsp, 4* 8

SkipSaveContents:
    ; Load new stack
    mov     rcx, rsp
    call    ASM_PFX(SwapStack)
    mov     rsp, rax

    ; Restore previous contexts
    add     rsp, 4 * 8
    POPAD

    ret
