/** @file
  QEMU instance of Platform Sec Lib header file.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _PLATFORM_SEC_LIB_H_
#define  _PLATFORM_SEC_LIB_H_

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/TemporaryRamSupport.h>

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FspCommonLib.h>
#include <FspmUpd.h>
#include <FspsUpd.h>

#define Flat32Start                  _ModuleEntryPoint
extern UINT32                        *TopOfCar;
extern IA32_DESCRIPTOR                BootGdtDescr;

#define DATA_LEN_OF_PER0 0x18
#define DATA_LEN_OF_MCUD 0x18
#define DATA_LEN_AT_TOP_OF_CAR (DATA_LEN_OF_PER0+DATA_LEN_OF_MCUD+4)

FSP_INFO_HEADER *AsmGetFspInfoHeader (VOID);


#endif







