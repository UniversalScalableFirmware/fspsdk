;; @file
; Provide macro for QEMU CAR initialization
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

;
; Set to 1 to enable debug
;
NO_EVICTION_MODE_DEBUG  EQU     1

;
;args 1: Status
;
%macro STATUS_CODE 1
%if NO_EVICTION_MODE_DEBUG
                    mov  al, %1
                    out  080h, al
%endif
%endmacro

%macro STATUS_CODE2 1
                    mov  al, %1
                    out  080h, al
%endmacro

FVHEADER_LEN_OFF    EQU 30h
FFSHEADER_LEN       EQU 18h

IMAGE_BASE_ADDRESS  EQU 0FFFF0000h

;
; Set to 1 to enable debug support for "Deterministic BSP selection"
;
AP_ENTRY_DELAY      EQU         10h
AP_EXECUTION_DELAY  EQU         1000h

;
; Define the segment used for AP start-up
; It should be on the top of the recovery FV
; Seg  = 0100h - (BlockNumber of Recovery FV)
; Here 0FCh = 0100h - 04h
;
AP_SEG                      EQU   0FFh

;
; Commands defined in the AP SIPI code
;
AP_SIPI_COLLECT_MAX_RATIO   EQU   001h
AP_SIPI_PROGRAM_MAX_RATIO   EQU   002h
AP_SIPI_SWITCH_BSP          EQU   003h
