/** @file
  QEMU video controller register definitions.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _QEMU_VIDEO_H_
#define _QEMU_VIDEO_H_

#define QEMU_VGA_VID_DID                 0x11111234
#define QEMU_VGA_VID_DID2                0x11114321

#define VBE_DISPI_IOPORT_INDEX           0x01CE
#define VBE_DISPI_IOPORT_DATA            0x01D0

#define ATT_ADDRESS_REGISTER             0x3C0

#define VBE_DISPI_INDEX_ID               0x0
#define VBE_DISPI_INDEX_XRES             0x1
#define VBE_DISPI_INDEX_YRES             0x2
#define VBE_DISPI_INDEX_BPP              0x3
#define VBE_DISPI_INDEX_ENABLE           0x4
#define VBE_DISPI_INDEX_BANK             0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH       0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT      0x7
#define VBE_DISPI_INDEX_X_OFFSET         0x8
#define VBE_DISPI_INDEX_Y_OFFSET         0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0xa

#define VBE_DISPI_DISABLED               0x00
#define VBE_DISPI_ENABLED                0x01
#define VBE_DISPI_GETCAPS                0x02
#define VBE_DISPI_8BIT_DAC               0x20
#define VBE_DISPI_LFB_ENABLED            0x40
#define VBE_DISPI_NOCLEARMEM             0x80

#define GRAPHICS_DATA_SIG    SIGNATURE_32 ('Q', 'G', 'F', 'X')

typedef struct {
  VOID*      LogoPtr;
  UINT32     LogoSize;
  VOID*      GraphicsConfigPtr;
} GRAPHICS_CONFIG;

typedef struct {
  UINT32     Signature;
  UINT32     ResX;
  UINT32     ResY;
} GRAPHICS_DATA;

typedef struct {
  UINT64                                 Signature;
  UINT32                                 Bar2;
} QEMU_VIDEO_PRIVATE_DATA;

typedef struct {
  UINT32    ResX;
  UINT32    ResY;
} QEMU_VIDEO_BOCHS_MODES;

/**
  Qemu GFX callback initialization after PCI enumeration

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
**/
EFI_STATUS
EFIAPI
QemuPostPciEnumerationCallback (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor,
  IN  VOID                        *Ppi
  );

/**
  This function performs the PEI module initialization.

  @param[in]  GraphicsPolicyPtr   Pointer to the graphics policy structure.

  @retval   EFI_SUCCESS       If initialization is successful.
  @retval   EFI_DEVICE_ERROR  If initialization fails.
**/
EFI_STATUS
EFIAPI
QemuPeiGraphicsInit (
  IN  VOID  *GraphicsPolicyPtr
  );

/**
  This function returns the PEI graphics mode information.

  @param[out]  Mode   Pointer to graphics mode structure.

  @retval EFI_INVALID_PARAMETER   If Mode is NULL.
  @retval EFI_NOT_READY           If internal function call to ::GetCurrentMode() fails.
  @retval EFI_SUCCESS             If the mode information is returned in input argument.
**/
EFI_STATUS
EFIAPI
QemuPeiGraphicsGetMode (
  OUT EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode
  );

#endif
