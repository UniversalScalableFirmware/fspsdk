;; @file
; This is the code that initializes CAR mode for QEMU.
;
; Copyright (c) 2017-2018 Intel Corporation.
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------
;;

    SECTION .text

BITS     32

;_TEXT_PROTECTED_MODE      SEGMENT PARA PUBLIC USE32 'CODE'
;ASSUME  CS:_TEXT_PROTECTED_MODE, DS:_TEXT_PROTECTED_MODE

align 4
global ASM_PFX(ProtectedModeSECStart)
ASM_PFX(ProtectedModeSECStart):
  jmp     $

global ASM_PFX(SecPlatformInit)
ASM_PFX(SecPlatformInit):
  jmp     $

global ASM_PFX(SecCarInit)
ASM_PFX(SecCarInit):
  ;
  ; After memory initialization is complete, please follow the algorithm in the BIOS
  ; Writer's Guide to properly transition to a normal system configuration.
  ; The algorithm covers the required sequence to properly exit this mode.
  ;
  jmp    $

