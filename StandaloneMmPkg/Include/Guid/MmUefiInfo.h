/** @file
  GUIDs for MM Event.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MM_UEFI_INFO_H__
#define __MM_UEFI_INFO_H__

#define MM_UEFI_INFO_GUID \
  { 0xa37721e4, 0x8c0b, 0x4bca, { 0xb5, 0xe8, 0xe9, 0x2, 0xa0, 0x25, 0x51, 0x4e }}

extern EFI_GUID gMmUefiInfoGuid;

#pragma pack(1)
typedef struct {
  EFI_PHYSICAL_ADDRESS      EfiSystemTable;
} EFI_MM_COMMUNICATE_UEFI_INFO_DATA;

typedef struct {
  EFI_GUID                              HeaderGuid;
  UINTN                                 MessageLength;
  EFI_MM_COMMUNICATE_UEFI_INFO_DATA    Data;
} EFI_MM_COMMUNICATE_UEFI_INFO;
#pragma pack()

#endif