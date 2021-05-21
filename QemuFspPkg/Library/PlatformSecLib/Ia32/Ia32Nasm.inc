;; @file
; IA32 architecture MSRs
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
; WSM CPUID vs. stepping
;
%define WSM_A0_STEPPING 0x206C0
%define IA32_MTRR_CAP 0xFE
%define MTRR_PHYS_BASE_0 0x200
%define MTRR_PHYS_MASK_0 0x201
%define MTRR_PHYS_BASE_1 0x202
%define MTRR_PHYS_MASK_1 0x203
%define MTRR_PHYS_BASE_2 0x204
%define MTRR_PHYS_MASK_2 0x205
%define MTRR_PHYS_BASE_3 0x206
%define MTRR_PHYS_MASK_3 0x207
%define MTRR_PHYS_BASE_4 0x208
%define MTRR_PHYS_MASK_4 0x209
%define MTRR_PHYS_BASE_5 0x20A
%define MTRR_PHYS_MASK_5 0x20B
%define MTRR_PHYS_BASE_6 0x20C
%define MTRR_PHYS_MASK_6 0x20D
%define MTRR_PHYS_BASE_7 0x20E
%define MTRR_PHYS_MASK_7 0x20F
%define MTRR_PHYS_BASE_8 0x210
%define MTRR_PHYS_MASK_8 0x211
%define MTRR_PHYS_BASE_9 0x212
%define MTRR_PHYS_MASK_9 0x213
%define MTRR_FIX_64K_00000 0x250
%define MTRR_FIX_16K_80000 0x258
%define MTRR_FIX_16K_A0000 0x259
%define MTRR_FIX_4K_C0000 0x268
%define MTRR_FIX_4K_C8000 0x269
%define MTRR_FIX_4K_D0000 0x26A
%define MTRR_FIX_4K_D8000 0x26B
%define MTRR_FIX_4K_E0000 0x26C
%define MTRR_FIX_4K_E8000 0x26D
%define MTRR_FIX_4K_F0000 0x26E
%define MTRR_FIX_4K_F8000 0x26F
%define MTRR_DEF_TYPE 0x2FF

%define MTRR_MEMORY_TYPE_UC 0x0
%define MTRR_MEMORY_TYPE_WC 0x1
%define MTRR_MEMORY_TYPE_WT 0x4
%define MTRR_MEMORY_TYPE_WP 0x5
%define MTRR_MEMORY_TYPE_WB 0x6

%define MTRR_DEF_TYPE_E 0x800
%define MTRR_DEF_TYPE_FE 0x400
%define MTRR_PHYSMASK_VALID 0x800

;
; Define the high 32 bits of MTRR masking
; This should be read from CPUID EAX = 080000008h, EAX bits [7:0]
; But for most platforms this will be a fixed supported size so it is
; fixed to save space.
;
%define MTRR_PHYS_MASK_VALID 0x800
%define MTRR_PHYS_MASK_HIGH 0xF      ; For 36 bit addressing
;MTRR_PHYS_MASK_HIGH           EQU 0000000FFh      ; For 40 bit addressing

%define IA32_MISC_ENABLE 0x1A0
%define FAST_STRING_ENABLE_BIT 0x1

%define CR0_CACHE_DISABLE 0x40000000
%define CR0_NO_WRITE 0x20000000

%define IA32_PLATFORM_ID 0x17
%define IA32_BIOS_UPDT_TRIG 0x79
%define IA32_BIOS_SIGN_ID 0x8B
%define PLATFORM_INFO 0xCE
%define NO_EVICT_MODE 0x2E0
%define NO_EVICTION_ENABLE_BIT 0x1

;
; MSR definitions
;
%define MSR_IA32_PLATFORM_ID 0x17
%define MSR_APIC_BASE 0x1B
%define MSR_SOCKET_ID 0x39
%define MSR_IA32_FEATURE_CONTROL 0x3A
%define MSR_CLOCK_CST_CONFIG_CONTROL 0xE2
%define MSR_CLOCK_FLEX_MAX 0x194
%define MSR_IA32_PERF_STS 0x198
%define MSR_IA32_PERF_CTL 0x199
%define MSR_IA32_MISC_ENABLES 0x1A0
%define MSR_IA32_MC8_MISC2 0x288
%define MSR_IA32_MC7_CTL 0x41C

;
; CSR definitions
;
%define CSR_DESIRED_CORES 0x80            ; D0:F0:R80h
%define CSR_DESIRED_CORES_LOCK_BIT 16
%define CSR_DESIRED_CORES_SMT_BIT 8
%define CSR_DESIRED_CORES_COUNT_MASK 7
%define CSR_DESIRED_SMT_CORE_COUNT_MASK 0x107

%define CSR_DESIRED_CORES_HT_DISABLED_BIT 0xFFFFF100
%define CSR_DESIRED_CORES_MC_DISABLED_BIT 0xFFFC
%define CSR_DESIRED_CORES_HT_DISABLED 0x100
%define CSR_DESIRED_CORES_MC_DISABLED 0x1

%define CSR_SVID_SDID 0x2C            ; D0:F0:R2Ch
%define DEAFULT_SVID_SDID 0x80868086       ; DWORD Access & Write Once ONLY

;
; Processor MSR definitions
;
%define MSR_BBL_CR_CTL3 0x11E ; L2 cache configuration MSR
%define B_MSR_BBL_CR_CTL3_L2_NOT_PRESENT 23    ; L2 not present
%define B_MSR_BBL_CR_CTL3_L2_ENABLED 8    ; L2 enabled
%define B_MSR_BBL_CR_CTL3_L2_HARDWARE_ENABLED 0    ; L2 hardware enabled

%define P6RatioBitsMask 0x1F  ; Bitmask for cpu ratio
%define P6_FREQ_LOCKED_BIT 15d

;
; Local APIC Register Equates
;
%define LOCAL_APIC_ID_REG 0xFEE00020
%define APIC_ICR_HI 0xFEE00310
%define APIC_ICR_LO 0xFEE00300
%define ANDICRMask 0xFFF32000      ; AND mask for ICR Saving reserved bits
%define ORSelfINIT 0x4500      ; OR mask to send INIT IPI to itself
%define ORAllButSelf 0xC0000      ; OR mask to set dest field = "All But Self"

;
; Cache control macro
;
%macro DISABLE_CACHE 0
    mov   eax, cr0
    or    eax, CR0_CACHE_DISABLE + CR0_NO_WRITE
    wbinvd
    mov   cr0, eax
%endmacro

%macro ENABLE_CACHE 0
    mov   eax, cr0
    and   eax, ~ (CR0_CACHE_DISABLE + CR0_NO_WRITE)
    wbinvd
    mov   cr0, eax
%endmacro

%define VENDOR_ID_REG 0
%define PCI_REVISION_ID_REG 8
%define CPU_GENERIC_UNCORE_DEV 0
%define CPU_GENERIC_UNCORE_FUNC 0
%define CPU_LINK_1_DEV 2
%define CPU_LINK_1_FUNC 4

%define B0_CPU_STEPPING 0x10

%define BLOCK_LENGTH_BYTES 2048


