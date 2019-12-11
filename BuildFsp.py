#!/usr/bin/env python
## @ BuildFsp.py
# Build FSP main script
#
# Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

##
# Import Modules
#
import os
import sys
import re
import imp
import errno
import shutil
import argparse
import subprocess
import multiprocessing
from ctypes import *

def get_file_data (file, mode = 'rb'):
    return open(file, mode).read()

def run_process (arg_list, print_cmd = False, capture_out = False):
    sys.stdout.flush()
    if print_cmd:
        print (' '.join(arg_list))

    exc    = None
    result = 0
    output = ''
    try:
        if capture_out:
            output = subprocess.check_output(arg_list).decode()
        else:
            result = subprocess.call (arg_list)
    except Exception as ex:
        result = 1
        exc    = ex

    if result:
        if not print_cmd:
            print ('Error in running process:\n  %s' % ' '.join(arg_list))
        if exc is None:
            sys.exit(1)
        else:
            raise exc

    return output


def check_files_exist (base_name_list, dir = '', ext = ''):
    for each in base_name_list:
        if not os.path.exists (os.path.join (dir, each + ext)):
            return False
    return True


def get_visual_studio_info ():

    toolchain        = ''
    toolchain_prefix = ''
    toolchain_path   = ''
    toolchain_ver    = ''

    # check new Visual Studio Community version first
    vswhere_path = "%s/Microsoft Visual Studio/Installer/vswhere.exe" % os.environ['ProgramFiles(x86)']
    if os.path.exists (vswhere_path):
        cmd = [vswhere_path, '-all', '-property', 'installationPath']
        lines = run_process (cmd, capture_out = True)
        vscommon_path = ''
        for each in lines.splitlines ():
            each = each.strip()
            if each and os.path.isdir(each):
                vscommon_path = each
        vcver_file = vscommon_path + '\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt'
        if os.path.exists(vcver_file):
            for vs_ver in ['2017']:
                check_path = '\\Microsoft Visual Studio\\%s\\' % vs_ver
                if check_path in vscommon_path:
                    toolchain_ver    = get_file_data (vcver_file, 'r').strip()
                    toolchain_prefix = 'VS%s_PREFIX' % (vs_ver)
                    toolchain_path   = vscommon_path + '\\VC\\Tools\\MSVC\\%s\\' % toolchain_ver
                    toolchain='VS%s' % (vs_ver)
                    break

    if toolchain == '':
        vs_ver_list = [
            ('2015', 'VS140COMNTOOLS'),
            ('2013', 'VS120COMNTOOLS')
        ]
        for vs_ver, vs_tool in vs_ver_list:
            if vs_tool in os.environ:
                toolchain        ='VS%s%s' % (vs_ver, 'x86')
                toolchain_prefix = 'VS%s_PREFIX' % (vs_ver)
                toolchain_path   = os.path.join(os.environ[vs_tool], '..//..//')
                toolchain_ver    = vs_ver
                parts   = os.environ[vs_tool].split('\\')
                vs_node = 'Microsoft Visual Studio '
                for part in parts:
                    if part.startswith(vs_node):
                        toolchain_ver = part[len(vs_node):]
                break

    return (toolchain, toolchain_prefix, toolchain_path, toolchain_ver)


def rebuild_basetools ():
    exe_list = 'GenFfs  GenFv  GenFw  GenSec LzmaCompress'.split()
    ret = 0
    workspace = os.environ['WORKSPACE']

    cmd = [sys.executable, '-c', 'import sys; import platform; print(", ".join([sys.executable, platform.python_version()]))']
    py_out = run_process (cmd, capture_out = True)
    parts  = py_out.split(',')
    if len(parts) > 1:
        py_exe, py_ver = parts
        os.environ['PYTHON_COMMAND'] = py_exe
        print ('Using %s, Version %s' % (os.environ['PYTHON_COMMAND'], py_ver.rstrip()))
    else:
        os.environ['PYTHON_COMMAND'] = 'python'

    if os.name == 'posix':
        if not check_files_exist (exe_list, os.path.join(workspace, 'BaseTools', 'Source', 'C', 'bin')):
            ret = subprocess.call(['make', '-C', 'BaseTools'])

    elif os.name == 'nt':

        if not check_files_exist (exe_list, os.path.join(workspace, 'BaseTools', 'Bin', 'Win32'), '.exe'):
            print ("Could not find pre-built BaseTools binaries, try to rebuild BaseTools ...")
            ret = run_process (['BaseTools\\toolsetup.bat', 'forcerebuild'])

    if ret:
        print ("Build BaseTools failed, please check required build environment and utilities !")
        sys.exit(1)


def prep_env():
    work_dir = os.path.dirname(os.path.realpath(__file__))
    os.chdir(work_dir)
    if sys.platform == 'darwin':
        toolchain = 'XCODE5'
        os.environ['PATH'] = os.environ['PATH'] + ':' + os.path.join(work_dir, 'BaseTools/BinWrappers/PosixLike')
        clang_ver = run_process (['clang', '-dumpversion'], capture_out = True)
        clang_ver = clang_ver.strip()
        toolchain_ver = clang_ver
    elif os.name == 'posix':
        toolchain = 'GCC49'
        gcc_ver = subprocess.Popen(['gcc', '-dumpversion'], stdout=subprocess.PIPE)
        (gcc_ver, err) = subprocess.Popen(['sed', 's/\\..*//'], stdin=gcc_ver.stdout, stdout=subprocess.PIPE).communicate()
        if int(gcc_ver) > 4:
            toolchain = 'GCC5'

        os.environ['PATH'] = os.environ['PATH'] + ':' + os.path.join(work_dir, 'BaseTools/BinWrappers/PosixLike')
        toolchain_ver = gcc_ver
    elif os.name == 'nt':
        os.environ['PATH'] = os.environ['PATH'] + ';' + os.path.join(work_dir, 'BaseTools\\Bin\\Win32')
        os.environ['PATH'] = os.environ['PATH'] + ';' + os.path.join(work_dir, 'BaseTools\\BinWrappers\\WindowsLike')
        os.environ['PYTHONPATH'] = os.path.join(work_dir, 'BaseTools', 'Source', 'Python')

        toolchain, toolchain_prefix, toolchain_path, toolchain_ver = get_visual_studio_info ()
        if toolchain:
            os.environ[toolchain_prefix] = toolchain_path
        else:
            print("Could not find supported Visual Studio version !")
            sys.exit(1)
        if 'NASM_PREFIX' not in os.environ:
            os.environ['NASM_PREFIX'] = "C:\\Nasm\\"
        if 'OPENSSL_PATH' not in os.environ:
            os.environ['OPENSSL_PATH'] = "C:\\Openssl\\"
        if 'IASL_PREFIX' not in os.environ:
            os.environ['IASL_PREFIX'] = "C:\\ASL\\"
    else:
        print("Unsupported operating system !")
        sys.exit(1)

    print ('Using %s, Version %s' % (toolchain, toolchain_ver))

    # Update Environment vars
    os.environ['EDK_TOOLS_PATH'] = os.path.join(work_dir, 'BaseTools')
    os.environ['BASE_TOOLS_PATH'] = os.path.join(work_dir, 'BaseTools')
    if 'WORKSPACE' not in os.environ:
        os.environ['WORKSPACE'] = work_dir
    os.environ['CONF_PATH']     = os.path.join(os.environ['WORKSPACE'], 'Conf')
    os.environ['TOOL_CHAIN']    = toolchain

    return (work_dir, toolchain)


def Fatal(msg):
    sys.stdout.flush()
    raise Exception(msg)


def CopyFileList (copy_list, fsp_dir, sbl_dir):
    print ('Copy FSP into Slim Bootloader source tree ...')
    for src_path, dst_path in copy_list:
        src_path = os.path.join (fsp_dir, src_path)
        dst_path = os.path.join (sbl_dir, dst_path)
        if not os.path.exists(os.path.dirname(dst_path)):
            os.makedirs(os.path.dirname(dst_path))
        print ('Copy:  %s\n  To:  %s' % (os.path.abspath(src_path), os.path.abspath(dst_path)))
        shutil.copy (src_path, dst_path)
    print ('Done\n')


def Prebuild(target, toolchain):

    rebuild_basetools ()

    workspace = os.environ['WORKSPACE']
    if not os.path.exists(os.path.join(workspace, 'Conf')):
        os.makedirs(os.path.join(workspace, 'Conf'))
    for name in ['target', 'tools_def', 'build_rule']:
        txt_file = os.path.join(workspace, 'Conf/%s.txt' % name)
        if not os.path.exists(txt_file):
            shutil.copy (
              os.path.join(workspace, 'BaseTools/Conf/%s.template' % name),
              os.path.join(workspace, 'Conf/%s.txt' % name))

    cmd = '%s -p QemuFspPkg/QemuFspPkg.dsc -m QemuFspPkg/FspHeader/FspHeader.inf -a IA32 -b %s -t %s -DCFG_PREBUILD' % (
        'build' if os.name == 'posix' else 'build.bat', target, toolchain)
    ret = subprocess.call(cmd.split(' '))
    if ret:
        Fatal('Failed to prebuild QEMU FSP !')

    FspGuid = {
        'FspTUpdGuid'       : '34686CA3-34F9-4901-B82A-BA630F0714C6',
        'FspMUpdGuid'       : '39A250DB-E465-4DD1-A2AC-E2BD3C0E2385',
        'FspSUpdGuid'       : 'CAE3605B-5B34-4C85-B3D7-27D54273C40F'
    }

    # !!!!!!!! Update VPD/UPD Information !!!!!!!!
    print('Preparing VPD/UPD Information...')

    pkgname = 'QemuFspPkg'
    cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgOpt.py UPDTXT %s/%s.dsc Build/%s/%s_%s/FV' % (
           workspace, pkgname, pkgname, pkgname, target, toolchain)
    ret = subprocess.call(cmd.split(' '))
    if not (ret == 0 or ret == 256):
        Fatal('Failed to generate UPD txt file !')

    print('UPD TXT file was generated successfully !')

    print('Generate UPD Header File ...')
    bdpgpath = 'BPDG' if os.name == 'posix' else 'BPDG.bat'

    fvdir = 'Build/%s/%s_%s/FV' % (pkgname, target, toolchain)
    for fspcomp in FspGuid:
        fspguid = FspGuid[fspcomp]
        cmd = '%s %s/%s.txt' % (bdpgpath, fvdir, fspguid)
        cmd = cmd + ' -o %s/%s.bin' % (fvdir, fspguid)
        cmd = cmd + ' -m %s/%s.map' % (fvdir, fspguid)
        filepath = '%s/%s/%s.bin' % (workspace, fvdir, fspguid)
        if os.path.exists(filepath):
            os.remove(filepath)
        ret = subprocess.call(cmd.split(' '))
        if ret:
            Fatal('Failed to generate UPD bin file !')

    cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgOpt.py HEADER %s/%s.dsc Build/%s/%s_%s/FV %s/Include/BootLoaderPlatformData.h' % (
          workspace, pkgname, pkgname, pkgname, target, toolchain, pkgname)
    ret = subprocess.call(cmd.split(' '))
    if ret:
        Fatal('Failed to generate UPD header file !')

    print('Generate BSF File ...')
    cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgOpt.py GENBSF %s/%s.dsc Build/%s/%s_%s/FV %s/QemuFsp.bsf' % (
          workspace, pkgname, pkgname, pkgname, target, toolchain, fvdir)
    ret = subprocess.call(cmd.split(' '))
    if ret:
        Fatal('Failed to generate UPD BSF file !')

    for fliename in ['FspUpd.h', 'FsptUpd.h', 'FspmUpd.h', 'FspsUpd.h']:
        shutil.copyfile('%s/%s' % (fvdir, fliename), '%s/Include/%s'%(pkgname, fliename))
    print('End of PreBuild...')


def Build (target, toolchain):
    cmd = '%s -p QemuFspPkg/QemuFspPkg.dsc -a IA32 -b %s -t %s -y Report%s.log' % (
        'build' if os.name == 'posix' else 'build.bat', target, toolchain, target)
    ret = subprocess.call(cmd.split(' '))
    if ret:
        Fatal('Failed to do Build QEMU FSP!')

    print('End of Build...')


def PostBuild (target, toolchain):

    print ('Start of PostBuild ...')

    patchfv = 'IntelFsp2Pkg/Tools/PatchFv.py'
    fvdir   = 'Build/QemuFspPkg/%s_%s/FV' % (target, toolchain)

    cmd1 = [
           "0x0000,            _BASE_FSP-T_,                                                                                       @Temporary Base",
           "<[0x0000]>+0x00AC, [<[0x0000]>+0x0020],                                                                                @FSP-T Size",
           "<[0x0000]>+0x00B0, [0x0000],                                                                                           @FSP-T Base",
           "<[0x0000]>+0x00B4, ([<[0x0000]>+0x00B4] & 0xFFFFFFFF) | 0x0001,                                                        @FSP-T Image Attribute",
           "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FFC) | 0x1000 | 0x0001 | 0x0002,                                      @FSP-T Component Attribute",
           "<[0x0000]>+0x00B8, 70BCF6A5-FFB1-47D8-B1AE-EFE5508E23EA:0x1C - <[0x0000]>,                                             @FSP-T CFG Offset",
           "<[0x0000]>+0x00BC, [70BCF6A5-FFB1-47D8-B1AE-EFE5508E23EA:0x14] & 0xFFFFFF - 0x001C,                                    @FSP-T CFG Size",
           "<[0x0000]>+0x00C4, FspSecCoreT:_TempRamInitApi - [0x0000],                                                             @TempRamInit API",
           "0x0000,            0x00000000,                                                                                         @Restore the value",
           "FspSecCoreT:_FspInfoHeaderRelativeOff, FspSecCoreT:_AsmGetFspInfoHeader - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP-T Header Offset"
           ]

    cmd2 = [
         "0x0000,            _BASE_FSP-M_,                                                                                       @Temporary Base",
         "<[0x0000]>+0x00AC, [<[0x0000]>+0x0020],                                                                                @FSP-M Size",
         "<[0x0000]>+0x00B0, [0x0000],                                                                                           @FSP-M Base",
         "<[0x0000]>+0x00B4, ([<[0x0000]>+0x00B4] & 0xFFFFFFFF) | 0x0001,                                                        @FSP-M Image Attribute",
         "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FFC) | 0x2000 | 0x0001 | 0x0002,                                      @FSP-M Component Attribute",
         "<[0x0000]>+0x00B8, D5B86AEA-6AF7-40D4-8014-982301BC3D89:0x1C - <[0x0000]>,                                             @FSP-M CFG Offset",
         "<[0x0000]>+0x00BC, [D5B86AEA-6AF7-40D4-8014-982301BC3D89:0x14] & 0xFFFFFF - 0x001C,                                    @FSP-M CFG Size",
         "<[0x0000]>+0x00D0, FspSecCoreM:_FspMemoryInitApi - [0x0000],                                                           @MemoryInitApi API",
         "<[0x0000]>+0x00D4, FspSecCoreM:_TempRamExitApi - [0x0000],                                                             @TempRamExit API",
         "FspSecCoreM:_FspPeiCoreEntryOff, PeiCore:__ModuleEntryPoint - [0x0000],                                                @PeiCore Entry",
         "0x0000,            0x00000000,                                                                                         @Restore the value",
         "FspSecCoreM:_FspInfoHeaderRelativeOff, FspSecCoreM:_AsmGetFspInfoHeader - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP-M Header Offset"
         ]

    cmd3 = [
         "0x0000,            _BASE_FSP-S_,                                                                                       @Temporary Base",
         "<[0x0000]>+0x00AC, [<[0x0000]>+0x0020],                                                                                @FSP-S Size",
         "<[0x0000]>+0x00B0, [0x0000],                                                                                           @FSP-S Base",
         "<[0x0000]>+0x00B4, ([<[0x0000]>+0x00B4] & 0xFFFFFFFF) | 0x0001,                                                        @FSP-S Image Attribute",
         "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FFC) | 0x3000 | 0x0001 | 0x0002                      ,                @FSP-S Component Attribute",
         "<[0x0000]>+0x00B8, E3CD9B18-998C-4F76-B65E-98B154E5446F:0x1C - <[0x0000]>,                                             @FSP-S CFG Offset",
         "<[0x0000]>+0x00BC, [E3CD9B18-998C-4F76-B65E-98B154E5446F:0x14] & 0xFFFFFF - 0x001C,                                    @FSP-S CFG Size",
         "<[0x0000]>+0x00D8, FspSecCoreS:_FspSiliconInitApi - [0x0000],                                                          @SiliconInit API",
         "<[0x0000]>+0x00CC, FspSecCoreS:_NotifyPhaseApi - [0x0000],                                                             @NotifyPhase API",
         "0x0000,            0x00000000,                                                                                         @Restore the value",
         "FspSecCoreS:_FspInfoHeaderRelativeOff, FspSecCoreS:_AsmGetFspInfoHeader - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP-S Header Offset"
         ]

    for fspt, cmd in [('T', cmd1), ('M', cmd2), ('S',cmd3)]:
        print ('Patch FSP-%s Image ...' % fspt)
        line = ['python', patchfv, fvdir, 'FSP-%s:QEMUFSP' % fspt]
        line.extend(cmd)
        ret = subprocess.call(line)
        if ret:
            Fatal('Failed to do PostBuild QEMU FSP!')

    copy_list = [
        ('QEMUFSP.fd',  'QEMU_FSP_%s.fd' % target),
        ('QemuFsp.bsf', 'QEMU_FSP.bsf'),
        ('FspUpd.h',    'FspUpd.h'),
        ('FsptUpd.h',   'FsptUpd.h'),
        ('FspmUpd.h',   'FspmUpd.h'),
        ('FspsUpd.h',   'FspsUpd.h'),
    ]
    CopyFileList (copy_list, fvdir, 'BuildFsp')

    print('End of PostBuild...')


def Main():
    curr_dir      = os.path.dirname (os.path.realpath(__file__))
    sbl_dir       = os.path.abspath (os.path.join(curr_dir, '../..'))
    fsp_repo_dir  = os.path.abspath (os.path.join(curr_dir, '../../../IntelFsp'))
    qemu_repo_dir = os.path.abspath (os.path.join(curr_dir, '../../../QemuFsp'))

    if len(sys.argv) < 2:
        target = 'DEBUG'
    else:
        if sys.argv[1] == '/r':
            target = 'RELEASE'
        elif sys.argv[1] == '/d':
            target = 'DEBUG'
        else:
            print ('Unknown target %s !' % sys.argv[1])
            return -1

    workspace, toolchain = prep_env()
    os.environ['WORKSPACE'] = workspace
    Prebuild  (target, toolchain)
    Build     (target, toolchain)
    PostBuild (target, toolchain)

    return 0


if __name__ == '__main__':
    sys.exit(Main())
