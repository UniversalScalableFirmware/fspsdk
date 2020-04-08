/** @file
  Graphics Output Protocol functions for the QEMU video controller.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <IndustryStandard/Pci30.h>
#include <Library/PeimEntryPoint.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/FspCommonLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Ppi/Graphics.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Guid/GraphicsInfoHob.h>
#include <IndustryStandard/Bmp.h>
#include <FspsUpd.h>
#include "QemuVideo.h"

#define  BPP     4

EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE   *mMode;
QEMU_VIDEO_PRIVATE_DATA              mPrivate;

QEMU_VIDEO_BOCHS_MODES  mQemuVideoBochsModes[] = {
  {  640,  480 },
  {  800,  600 },
  { 1024,  768 },
};

/**
  PEI Graphics PPI structure instance.
**/
EFI_PEI_GRAPHICS_PPI mQemuPeiGraphicsPpi = {
  QemuPeiGraphicsInit,
  QemuPeiGraphicsGetMode,
};

/**
  Ppis to be installed
**/
EFI_PEI_PPI_DESCRIPTOR           mPeiGraphicsPpi[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiGraphicsPpiGuid,
    &mQemuPeiGraphicsPpi
  }
};

EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *Mode = NULL;

/**
  Convert a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
  is passed in a GopBlt buffer will be allocated by this routine. If a GopBlt
  buffer is passed in it will be used if it is big enough.

  @param[in]      BmpImage      Pointer to BMP file
  @param[in]      BmpImageSize  Number of bytes in BmpImage
  @param[in]      GopBlt        Buffer containing GOP version of BmpImage.
  @param[in,out]  GopBltSize    Size of GopBlt in bytes.
  @param[in,out]  PixelHeight   Height of GopBlt/BmpImage in pixels
  @param[in,out]  PixelWidth    Width of GopBlt/BmpImage in pixels

  @retval EFI_SUCCESS           GopBlt and GopBltSize are returned.
  @retval EFI_UNSUPPORTED       BmpImage is not a valid *.BMP image
  @retval EFI_BUFFER_TOO_SMALL  The passed in GopBlt buffer is not big enough.
                                GopBltSize will contain the required size.
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate.

**/

EFI_STATUS
ConvertBmpToGopBlt (
  IN     VOID      *BmpImage,
  IN     UINTN     BmpImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
  OUT UINTN        *PixelHeight,
  OUT UINTN        *PixelWidth
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT64                        BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  UINT32                        DataSizePerLine;
  BOOLEAN                       IsAllocated;
  UINT32                        ColorMapNum;
  UINTN                         MemPages;

  if (sizeof (BMP_IMAGE_HEADER) > BmpImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize != (sizeof (BMP_IMAGE_HEADER) - OFFSET_OF (BMP_IMAGE_HEADER, HeaderSize))) {
    return EFI_UNSUPPORTED;
  }

  //
  // The data size in each line must be 4 byte alignment.
  //
  DataSizePerLine = ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
  BltBufferSize = MultU64x32 (DataSizePerLine, BmpHeader->PixelHeight);
  if (BltBufferSize > (UINT32) ~0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image       = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));
  if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
      case 1:
        ColorMapNum = 2;
        break;
      case 4:
        ColorMapNum = 16;
        break;
      case 8:
        ColorMapNum = 256;
        break;
      default:
        ColorMapNum = 0;
        break;
    }
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) != sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image         = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader   = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize = MultU64x32 ((UINT64) BmpHeader->PixelWidth, BmpHeader->PixelHeight);

  //
  // Ensure the BltBufferSize * sizeof (EFI_GOP_BLT_PIXEL) doesn't overflow
  //
  if (BltBufferSize > DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    return EFI_UNSUPPORTED;
  }
  BltBufferSize = MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = (UINTN) BltBufferSize;
    MemPages =  (*GopBltSize/ EFI_PAGE_SIZE) + 1;
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               MemPages,
               ((EFI_PHYSICAL_ADDRESS*) GopBlt)
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    DEBUG ((DEBUG_INFO, "GopBlt = 0x%X \n", *GopBlt));
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN) BltBufferSize) {
      *GopBltSize = (UINTN) BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth   = BmpHeader->PixelWidth;
  *PixelHeight  = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[ (BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
        case 1:
          //
          // Convert 1-bit (2 colors) BMP to 24-bit color
          //
          for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
            Blt->Red    = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Red;
            Blt->Green  = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Green;
            Blt->Blue   = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Blue;
            Blt++;
            Width++;
          }

          Blt--;
          Width--;
          break;

        case 4:
          //
          // Convert 4-bit (16 colors) BMP Palette to 24-bit color
          //
          Index       = (*Image) >> 4;
          Blt->Red    = BmpColorMap[Index].Red;
          Blt->Green  = BmpColorMap[Index].Green;
          Blt->Blue   = BmpColorMap[Index].Blue;
          if (Width < (BmpHeader->PixelWidth - 1)) {
            Blt++;
            Width++;
            Index       = (*Image) & 0x0f;
            Blt->Red    = BmpColorMap[Index].Red;
            Blt->Green  = BmpColorMap[Index].Green;
            Blt->Blue   = BmpColorMap[Index].Blue;
          }
          break;

        case 8:
          //
          // Convert 8-bit (256 colors) BMP Palette to 24-bit color
          //
          Blt->Red    = BmpColorMap[*Image].Red;
          Blt->Green  = BmpColorMap[*Image].Green;
          Blt->Blue   = BmpColorMap[*Image].Blue;
          break;

        case 24:
          //
          // It is 24-bit BMP.
          //
          Blt->Blue   = *Image++;
          Blt->Green  = *Image++;
          Blt->Red    = *Image;
          break;

        default:
          //
          // Other bit format BMP is not supported.
          //
          if (IsAllocated) {
            FreePool (*GopBlt);
            *GopBlt = NULL;
          }
          return EFI_UNSUPPORTED;
          break;
      };

    }
    ImageIndex = (UINTN) (Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return EFI_SUCCESS;
}

/**
  FillFrameBufferAndShowLogo: Fill frame buffer with the image

  @param[in] GtConfig                    GRAPHICS_CONFIG to access the GtConfig related information
             GtConfig.GraphicsConfigPtr  Address of the Graphics Configuration Table

  @retval    EFI_STATUS
**/

EFI_STATUS
FillFrameBufferAndShowLogo(
  IN  GRAPHICS_CONFIG  *GtConfig
)
{
  VOID*                         Buffer;
  UINT32                        Size;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt = NULL;
  UINTN                         BltSize;
  UINTN                         Height;
  UINTN                         Width;
  EFI_STATUS                    Status = EFI_SUCCESS;
  UINTN                         LogoDestX;
  UINTN                         LogoDestY;
  UINTN                         SrcY,DstY;
  UINT8                         *SrcAddress;
  UINT8                         *DstAddress;
  UINT32                        BytesPerScanLine;

  DEBUG ((DEBUG_INFO, "FillFrameBufferAndShowLogo: Start \n"));

  if (Mode == NULL) {
    DEBUG ((DEBUG_INFO, "Returning from FillFrameBufferAndShowLogo() due to invalid mode\n"));
    return EFI_UNSUPPORTED;
  }

  ///
  /// Get the Logo pointer and Size .
  ///
  Buffer = GtConfig->LogoPtr;
  Size =   GtConfig->LogoSize;

  if (Buffer == 0) {
    DEBUG ((DEBUG_ERROR, "No Logo information. Returning from FillFrameBufferAndShowLogo()\n"));
    return EFI_UNSUPPORTED;
  }
  ///
  /// Convert Bmp Image to GopBlt
  ///
  Status = ConvertBmpToGopBlt (
             Buffer,
             (UINTN) Size,
             (VOID **) &Blt,
             &BltSize,
             &Height,
             &Width
             );
  ASSERT_EFI_ERROR (Status);

  //
  // if Convert Bmp to blt successful fill frame buffer with the image
  //
  if (!EFI_ERROR (Status)) {
    //
    // if Convert Bmp to blt successful Center the logo and fill frame buffer.
    //
    LogoDestX = (Mode->Info->HorizontalResolution - Width) /2;
    LogoDestY = (Mode->Info->VerticalResolution - Height) /2;
    BytesPerScanLine = Mode->Info->PixelsPerScanLine*sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    //
    // Fill framebuffer with the logo line by line
    //
    for (SrcY = 0, DstY = LogoDestY; DstY < (LogoDestY + Height); SrcY++, DstY++) {
      DstAddress = (UINT8 *) (UINTN) (Mode->FrameBufferBase + DstY * BytesPerScanLine + LogoDestX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      SrcAddress = (UINT8 *) ((UINT8 *) Blt + (SrcY * Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)));
      CopyMem (DstAddress, SrcAddress, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }
  }
  FreePool (Blt);

  DEBUG ((DEBUG_INFO, "FillFrameBufferAndShowLogo: END \n"));

  return Status;
}

/**
  CallPpiAndFillFrameBuffer: Call GraphicsInitPeim PeiGraphicsPpi to initalize display and get Mode info.
  Publish GraphicsInfoHob and call FillFrameBufferAndShowLogo

  @param[in]  GtConfig            GRAPHICS_CONFIG to access the GtConfig related information
              GtConfig.LogoPtr    Address of Logo to be displayed
              GtConfig.LogoSize   Logo Size

  @retval    EFI_STATUS
**/
EFI_STATUS
EFIAPI
CallPpiAndFillFrameBuffer (
  IN   GRAPHICS_CONFIG             *GtConfig
  )
{
  EFI_STATUS                  Status = EFI_SUCCESS;

  EFI_PEI_GRAPHICS_INFO_HOB   *PlatformGraphicsOutput;

  DEBUG ((DEBUG_INFO, "CallPpiAndFillFrameBuffer: Begin \n"));


  Mode       = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  if(Mode == NULL){
    return EFI_OUT_OF_RESOURCES;
  }

  Mode->Info = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if(Mode->Info == NULL){
    return EFI_OUT_OF_RESOURCES;
  }
  ///
  /// Call PeiGraphicsPpi.GraphicsPpiGetMode to get display resolution
  ///
  DEBUG ((DEBUG_INFO, "GraphicsPpiGetMode Start\n"));
  Status = QemuPeiGraphicsGetMode (Mode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GraphicsPpiGetMode failed. \n"));
    return Status;
  }

  ///
  /// Print Mode information recived from GraphicsPeim
  ///
  DEBUG ((DEBUG_INFO, "MaxMode:0x%x \n", Mode->MaxMode));
  DEBUG ((DEBUG_INFO, "Mode:0x%x \n", Mode->Mode));
  DEBUG ((DEBUG_INFO, "SizeOfInfo:0x%x \n", Mode->SizeOfInfo));
  DEBUG ((DEBUG_INFO, "FrameBufferBase:0x%x \n", Mode->FrameBufferBase));
  DEBUG ((DEBUG_INFO, "FrameBufferSize:0x%x \n", Mode->FrameBufferSize));
  DEBUG ((DEBUG_INFO, "Version:0x%x \n", Mode->Info->Version));
  DEBUG ((DEBUG_INFO, "HorizontalResolution:0x%x \n", Mode->Info->HorizontalResolution));
  DEBUG ((DEBUG_INFO, "VerticalResolution:0x%x \n", Mode->Info->VerticalResolution));
  DEBUG ((DEBUG_INFO, "PixelFormat:0x%x \n", Mode->Info->PixelFormat));
  DEBUG ((DEBUG_INFO, "PixelsPerScanLine:0x%x \n", Mode->Info->PixelsPerScanLine));

  ///
  /// Publish GraphicsInfoHob to be used by platform code
  ///
  PlatformGraphicsOutput = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));

  if (PlatformGraphicsOutput == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to build GraphicsInfoHob. \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  PlatformGraphicsOutput->GraphicsMode.Version              = Mode->Info->Version;
  PlatformGraphicsOutput->GraphicsMode.HorizontalResolution = Mode->Info->HorizontalResolution;
  PlatformGraphicsOutput->GraphicsMode.VerticalResolution   = Mode->Info->VerticalResolution;
  PlatformGraphicsOutput->GraphicsMode.PixelFormat          = Mode->Info->PixelFormat;
  PlatformGraphicsOutput->GraphicsMode.PixelInformation     = Mode->Info->PixelInformation;
  PlatformGraphicsOutput->GraphicsMode.PixelsPerScanLine    = Mode->Info->PixelsPerScanLine;
  PlatformGraphicsOutput->FrameBufferBase                   = Mode->FrameBufferBase;
  PlatformGraphicsOutput->FrameBufferSize                   = (UINT32)Mode->FrameBufferSize;

  ///
  /// Display Logo if user provides valid Bmp image
  ///
  if (GtConfig->LogoPtr != 0) {
    DEBUG ((DEBUG_INFO, "FillFrameBufferAndShowLogo Start\n"));
    Status = FillFrameBufferAndShowLogo ( GtConfig);
  }

  DEBUG ((DEBUG_INFO, "CallPpiAndFillFrameBuffer: End \n"));

  return Status;
}


/**
  This fucntion is the Entry point of GFX PEIM Module

  @param[in] FileHandle     EFI File handle passed by BIOS.
  @param[in] PeiServices    PEI services pointer passed by reference by BIOS.

  @retval EFI_SUCCESS       If the PPI is installed correctly.
  @retval EFI_DEVICE_ERROR  If the PPI installation fails.
**/
EFI_STATUS
EFIAPI
PeiGraphicsEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS     Status;
  FSPS_UPD      *FspsUpd;
  GRAPHICS_DATA *GfxPtr;
  UINT32         Idx;
  BOOLEAN        Found;
  UINT32         ResX;
  UINT32         ResY;

  FspsUpd = (FSPS_UPD *)GetFspSiliconInitUpdDataPointer();
  GfxPtr  = (GRAPHICS_DATA *)(UINTN)FspsUpd->FspsConfig.GraphicsConfigPtr;

  if (!(GfxPtr && (GfxPtr->Signature == GRAPHICS_DATA_SIG))) {
    DEBUG ((DEBUG_INFO, "NO valid graphics config data found!\n"));
    return EFI_SUCCESS;
  }

  Found = FALSE;
  for (Idx = 0; Idx < sizeof(mQemuVideoBochsModes) / sizeof(mQemuVideoBochsModes[0]); Idx++) {
    if ((mQemuVideoBochsModes[Idx].ResX == GfxPtr->ResX) && \
        (mQemuVideoBochsModes[Idx].ResY == GfxPtr->ResY)) {
      Found = TRUE;
      break;
    }
  }
  if (!Found) {
    Idx = 1;
  }

  ResX = mQemuVideoBochsModes[Idx].ResX;
  ResY = mQemuVideoBochsModes[Idx].ResY;

  mMode = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  mMode->Info->Version              = 0;
  mMode->Info->PixelFormat          = PixelBlueGreenRedReserved8BitPerColor;
  mMode->SizeOfInfo                 = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  mMode->FrameBufferBase            = 0xF0000000;
  mMode->Info->HorizontalResolution = ResX;
  mMode->Info->VerticalResolution   = ResY;
  mMode->Info->PixelsPerScanLine    = (((ResX * BPP) + 63) & 0xFFFFFFC0) / BPP;
  mMode->FrameBufferSize            = mMode->Info->PixelsPerScanLine * ResY * BPP;
  mMode->MaxMode                    = 1;
  mMode->Mode                       = 0;

  Status = PeiServicesInstallPpi (&mPeiGraphicsPpi[0]);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    DEBUG ((DEBUG_ERROR, "Failed to install Pei Graphics PPI\n"));
    ASSERT (FALSE);
  }

  return Status;
}

/**
  This function returns the current graphics mode information.

  @param[out] Mode    Pointer to graphics mode information

  @retval EFI_SUCCESS     Mode information is returned.
  @retval EFI_NOT_READY   Mode information is not returned.
**/
EFI_STATUS
GetCurrentMode (
  OUT EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *GfxMode
  )
{
  if (mMode == NULL) {
    DEBUG ((DEBUG_ERROR,"[ERROR]:[GetCurrentMode()]:[No current mode created]\n"));
    return EFI_NOT_READY;
  }

  GfxMode->Info->Version              = mMode->Info->Version;
  GfxMode->Info->PixelFormat          = mMode->Info->PixelFormat;
  GfxMode->SizeOfInfo                 = mMode->SizeOfInfo;
  GfxMode->FrameBufferBase            = mMode->FrameBufferBase;
  GfxMode->Info->HorizontalResolution = mMode->Info->HorizontalResolution;
  GfxMode->Info->VerticalResolution   = mMode->Info->VerticalResolution;
  GfxMode->Info->PixelsPerScanLine    = mMode->Info->PixelsPerScanLine;
  GfxMode->FrameBufferSize            = mMode->FrameBufferSize;
  GfxMode->MaxMode                    = mMode->MaxMode;
  GfxMode->Mode                       = mMode->Mode;

  return EFI_SUCCESS;
}

/**
  Display I/O port write

  @param[in] Private              Private data structure pointer.
  @param[in] Reg                  Register offset.
  @param[in] Data                 Register data.

**/
VOID
BochsWrite (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT16                   Reg,
  UINT16                   Data
  )
{
  MmioWrite16  (Private->Bar2 + 0x500 + (Reg << 1), Data);
}

/**
  Display I/O port read

  @param[in] Private              Private data structure pointer.
  @param[in] Reg                  Register offset.

  @retval                         Register data.
**/
UINT16
BochsRead (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT16                   Reg
  )
{
  return MmioRead16  (Private->Bar2 + 0x500 + (Reg << 1));
}

VOID
VgaOutb (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINTN                    Reg,
  UINT8                    Data
  )
{
  MmioWrite8  (Private->Bar2 + 0x400 - 0x3c0 + Reg, Data);
}

/**
  Qemu GFX mode initialization

  @param[in] Width      Resolution width.
  @param[in] Height     Resolution height.
  @param[in] Bpp        Bytes per pixel.
  @param[in] PciBase    PCI resource base to use.

**/
VOID
BochsInitMode (
  IN  UINT16  Width,
  IN  UINT16  Height,
  IN  UINT8   Bpp,
  IN  UINT32  PciBase
)
{
  QEMU_VIDEO_PRIVATE_DATA  *Private;
  UINT32  Address;
  UINT32  Data;

  Address = PCI_LIB_ADDRESS (0, 1, 0, PCI_VENDOR_ID_OFFSET);
  if (PciRead32(Address) != QEMU_VGA_VID_DID) {
    DEBUG ((DEBUG_ERROR, "Not supported GFX device!\n"));
    return;
  }

  Private = &mPrivate;
  Address = PCI_LIB_ADDRESS (0, 1, 0, PCI_BASE_ADDRESSREG_OFFSET + 0 * 4);
  Data    = PciRead32(Address) & ~0xF;
  if (Data == 0) {
    PciWrite32 (Address, PciBase);
    PciWrite32 (Address + 2 * 4, PciBase + 0x08000000);
    Data = PciBase;
  }
  mMode->FrameBufferBase = Data;

  Address = PCI_LIB_ADDRESS (0, 1, 0, PCI_BASE_ADDRESSREG_OFFSET + 2 * 4);
  mPrivate.Bar2 = PciRead32(Address) & ~0xF;

  Address = PCI_LIB_ADDRESS (0, 1, 0, PCI_COMMAND_OFFSET);
  PciWrite8(Address, EFI_PCI_COMMAND_BUS_MASTER | EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_IO_SPACE);

  BochsWrite (Private, VBE_DISPI_INDEX_ENABLE,      0);
  BochsWrite (Private, VBE_DISPI_INDEX_BANK,        0);
  BochsWrite (Private, VBE_DISPI_INDEX_X_OFFSET,    0);
  BochsWrite (Private, VBE_DISPI_INDEX_Y_OFFSET,    0);
  BochsWrite (Private, VBE_DISPI_INDEX_BPP,         Bpp * 8);
  BochsWrite (Private, VBE_DISPI_INDEX_XRES,        Width);
  BochsWrite (Private, VBE_DISPI_INDEX_VIRT_WIDTH,  Width);
  BochsWrite (Private, VBE_DISPI_INDEX_YRES,        Height);
  BochsWrite (Private, VBE_DISPI_INDEX_VIRT_HEIGHT, Height);
  BochsWrite (Private, VBE_DISPI_INDEX_ENABLE,      VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

  VgaOutb    (Private, ATT_ADDRESS_REGISTER,     0x20);
}

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
  )
{
  EFI_STATUS                   Status;
  GRAPHICS_CONFIG              GfxCfg;
  FSPS_UPD                    *FspsUpd;
  UINT32                       ResX;
  UINT32                       ResY;

  DEBUG ((DEBUG_INFO, "[INFO]:[QemuPeiGraphicsInit]: \n"));

  ResX = mMode->Info->HorizontalResolution;
  ResY = mMode->Info->VerticalResolution;

  FspsUpd = (FSPS_UPD *)GetFspSiliconInitUpdDataPointer();
  DEBUG ((DEBUG_INFO, "Initialize GFX mode to %dx%dx%d\n", ResX, ResY, BPP));

  BochsInitMode ((UINT16)ResX, (UINT16)ResY, BPP, FspsUpd->FspsConfig.PciTempResourceBase);

  GfxCfg.LogoPtr           = (VOID *)(UINTN)FspsUpd->FspsConfig.LogoPtr;
  GfxCfg.LogoSize          = FspsUpd->FspsConfig.LogoSize;
  GfxCfg.GraphicsConfigPtr = (VOID *)(UINTN)FspsUpd->FspsConfig.GraphicsConfigPtr;
  Status = CallPpiAndFillFrameBuffer (&GfxCfg);

  return Status;
}

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
  OUT EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *GfxMode
  )
{
  EFI_STATUS Status;

  if (GfxMode == NULL) {
    DEBUG ((DEBUG_ERROR,"[ERROR]:[IntelPeiGraphicsGetMode()]:[Error creating Mode information]\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetCurrentMode (GfxMode);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO,"[INFO]:[IntelPeiGraphicsGetMode()]:[Current Mode HRes = %d]\n", GfxMode->Info->HorizontalResolution));
  DEBUG ((DEBUG_INFO,"[INFO]:[IntelPeiGraphicsGetMode()]:[Current Mode VRes = %d]\n", GfxMode->Info->VerticalResolution));

  return EFI_SUCCESS;
}

