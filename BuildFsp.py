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

def copy_file_list (copy_list, fsp_dir, sbl_dir):
    print ('Copy FSP into Slim Bootloader source tree ...')
    for src_path, dst_path in copy_list:
        src_path = os.path.join (fsp_dir, src_path)
        dst_path = os.path.join (sbl_dir, dst_path)
        if not os.path.exists(os.path.dirname(dst_path)):
            os.makedirs(os.path.dirname(dst_path))
        print ('Copy:  %s\n  To:  %s' % (os.path.abspath(src_path), os.path.abspath(dst_path)))
        shutil.copy (src_path, dst_path)
    print ('Done\n')

def check_for_python():
    '''
    Verify Python executable is at required version
    '''
    cmd = [sys.executable, '-c', 'import sys; import platform; print(platform.python_version())']
    version = run_process (cmd, capture_out = True).strip()
    ver_parts = version.split('.')
    # Require Python 3.6 or above
    if not (len(ver_parts) >= 2 and int(ver_parts[0]) >= 3 and int(ver_parts[1]) >= 6):
        raise SystemExit ('ERROR: Python version ' + version + ' is not supported any more !\n       ' +
                          'Please install and use Python 3.6 or above to launch build script !\n')

    return version

def print_tool_version_info(cmd, version):
    try:
        if os.name == 'posix':
            cmd = subprocess.check_output(['which', cmd], stderr=subprocess.STDOUT).decode().strip()
    except:
        pass
    print ('Using %s, Version %s' % (cmd, version))


def check_for_openssl():
    '''
    Verify OpenSSL executable is available
    '''
    cmd = get_openssl_path ()
    try:
        version = subprocess.check_output([cmd, 'version']).decode().strip()
    except:
        print('ERROR: OpenSSL not available. Please set OPENSSL_PATH.')
        sys.exit(1)
    print_tool_version_info(cmd, version)
    return version

def check_for_nasm():
    '''
    Verify NASM executable is available
    '''
    cmd = os.path.join(os.environ.get('NASM_PREFIX', ''), 'nasm')
    try:
        version = subprocess.check_output([cmd, '-v']).decode().strip()
    except:
        print('ERROR: NASM not available. Please set NASM_PREFIX.')
        sys.exit(1)
    print_tool_version_info(cmd, version)
    return version

def check_for_git():
    '''
    Verify Git executable is available
    '''
    cmd = 'git'
    try:
        version = subprocess.check_output([cmd, '--version']).decode().strip()
    except:
        print('ERROR: Git not found. Please install Git or check if Git is in the PATH environment variable.')
        sys.exit(1)
    print_tool_version_info(cmd, version)
    return version

def get_openssl_path ():
    if os.name == 'nt':
        if 'OPENSSL_PATH' not in os.environ:
            os.environ['OPENSSL_PATH'] = "C:\\Openssl\\"
        if 'OPENSSL_CONF' not in os.environ:
            openssl_cfg = "C:\\Openssl\\openssl.cfg"
            if os.path.exists(openssl_cfg):
                os.environ['OPENSSL_CONF'] = openssl_cfg
    openssl = os.path.join(os.environ.get ('OPENSSL_PATH', ''), 'openssl')
    return openssl

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
        vscommon_paths = []
        for each in lines.splitlines ():
            each = each.strip()
            if each and os.path.isdir(each):
                vscommon_paths.append(each)

        for vs_ver in ['2019', '2017']:
            for vscommon_path in vscommon_paths:
                vcver_file = vscommon_path + '\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt'
                if os.path.exists(vcver_file):
                    check_path = '\\Microsoft Visual Studio\\%s\\' % vs_ver
                    if check_path in vscommon_path:
                        toolchain_ver    = get_file_data (vcver_file, 'r').strip()
                        toolchain_prefix = 'VS%s_PREFIX' % (vs_ver)
                        toolchain_path   = vscommon_path + '\\VC\\Tools\\MSVC\\%s\\' % toolchain_ver
                        toolchain = 'VS%s' % (vs_ver)
                        break
            if toolchain:
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
    exe_list = 'GenFfs  GenFv  GenFw  GenSec  LzmaCompress'.split()
    ret = 0
    fspsource = os.environ['WORKSPACE']

    if os.name == 'posix':
        if not check_files_exist (exe_list, os.path.join(fspsource, 'BaseTools', 'Source', 'C', 'bin')):
            ret = run_process (['make', '-C', 'BaseTools'])

    elif os.name == 'nt':

        if not check_files_exist (exe_list, os.path.join(fspsource, 'BaseTools', 'Bin', 'Win32'), '.exe'):
            print ("Could not find pre-built BaseTools binaries, try to rebuild BaseTools ...")
            ret = run_process (['BaseTools\\toolsetup.bat', 'forcerebuild'])

    if ret:
        print ("Build BaseTools failed, please check required build environment and utilities !")
        sys.exit(1)



def create_conf (workspace, sbl_source):
    # create conf and build folder if not exist
    workspace = os.environ['WORKSPACE']
    if not os.path.exists(os.path.join(workspace, 'Conf')):
        os.makedirs(os.path.join(workspace, 'Conf'))
    for name in ['target', 'tools_def', 'build_rule']:
        txt_file = os.path.join(workspace, 'Conf/%s.txt' % name)
        if not os.path.exists(txt_file):
            shutil.copy (
                os.path.join(sbl_source, 'BaseTools/Conf/%s.template' % name),
                os.path.join(workspace, 'Conf/%s.txt' % name))

def prep_env ():
    # check python version first
    version = check_for_python ()
    os.environ['PYTHON_COMMAND'] = '"' + sys.executable + '"'
    print_tool_version_info(os.environ['PYTHON_COMMAND'], version.strip())

    sblsource = os.path.dirname(os.path.realpath(__file__))
    os.chdir(sblsource)
    if sys.platform == 'darwin':
        toolchain = 'XCODE5'
        os.environ['PATH'] = os.environ['PATH'] + ':' + os.path.join(sblsource, 'BaseTools/BinWrappers/PosixLike')
        clang_ver = run_process (['clang', '-dumpversion'], capture_out = True)
        clang_ver = clang_ver.strip()
        toolchain_ver = clang_ver
    elif os.name == 'posix':
        toolchain = 'GCC49'
        gcc_ver = run_process (['gcc', '-dumpversion'], capture_out = True)
        gcc_ver = gcc_ver.strip()
        if int(gcc_ver.split('.')[0]) > 4:
            toolchain = 'GCC5'
        os.environ['PATH'] = os.environ['PATH'] + ':' + os.path.join(sblsource, 'BaseTools/BinWrappers/PosixLike')
        toolchain_ver = gcc_ver
    elif os.name == 'nt':
        os.environ['PATH'] = os.environ['PATH'] + ';' + os.path.join(sblsource, 'BaseTools\\Bin\\Win32')
        os.environ['PATH'] = os.environ['PATH'] + ';' + os.path.join(sblsource, 'BaseTools\\BinWrappers\\WindowsLike')
        os.environ['PYTHONPATH'] = os.path.join(sblsource, 'BaseTools', 'Source', 'Python')

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

    if 'SBL_KEY_DIR' not in os.environ:
        os.environ['SBL_KEY_DIR'] = "../SblKeys/"

    print_tool_version_info(toolchain, toolchain_ver)

    check_for_openssl()
    check_for_nasm()
    check_for_git()

    # Update Environment vars
    os.environ['WINSDK_PATH_FOR_RC_EXE']  = "C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.1A\\Bin\\"
    os.environ['EDK_TOOLS_PATH'] = os.path.join(sblsource, 'BaseTools')
    os.environ['BASE_TOOLS_PATH'] = os.path.join(sblsource, 'BaseTools')
    if 'WORKSPACE' not in os.environ:
        os.environ['WORKSPACE'] = sblsource
    os.environ['CONF_PATH']     = os.path.join(os.environ['WORKSPACE'], 'Conf')
    os.environ['TOOL_CHAIN']    = toolchain

    create_conf (os.environ['WORKSPACE'], sblsource)

    return toolchain

def fatal(msg):
    sys.stdout.flush()
    raise Exception(msg)


def pre_build(target, toolchain, fsppkg):

    workspace = os.environ['WORKSPACE']
    pkgname = fsppkg
    fspbase = fsppkg[:-3]
    updname = fspbase + 'Upd'
    fvdir = 'Build/%s/%s_%s/FV' % (pkgname, target, toolchain)
    if not os.path.exists (fvdir):
        os.makedirs(fvdir)

    FspGuid = {
        'FspTUpdGuid'       : '34686CA3-34F9-4901-B82A-BA630F0714C6',
        'FspMUpdGuid'       : '39A250DB-E465-4DD1-A2AC-E2BD3C0E2385',
        'FspSUpdGuid'       : 'CAE3605B-5B34-4C85-B3D7-27D54273C40F'
    }

    # !!!!!!!! Update VPD/UPD Information !!!!!!!!
    print('Preparing UPD Information...')

    # Generate combined YAML file
    cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgData.py GENYML %s/%s.yaml Build/%s/%s_%s/FV/%s.yaml' % (
           workspace, pkgname, updname, pkgname, target, toolchain, fspbase)
    ret = subprocess.call(cmd.split(' '))
    if not (ret == 0):
        fatal('Failed to generate combined YAML file !')

    print('Generate UPD Bianry File ...')
    for fsp_comp in 'TMS':
        cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgData.py GENBIN %s/%s.yaml@FSP%s_UPD Build/%s/%s_%s/FV/%s.bin' % (
               workspace, pkgname, updname, fsp_comp, pkgname, target, toolchain, FspGuid['Fsp%sUpdGuid' % fsp_comp])
        ret = subprocess.call(cmd.split(' '))
        if not (ret == 0):
            fatal('Failed to generate UPD binary file !')

    print('Generate UPD Header File ...')
    for fsp_comp in 'TMS ':
        if fsp_comp == ' ':
            fsp_comp = ''
            scope = 'FSP_SIG'
        else:
            scope = 'FSP%s_UPD' % fsp_comp
        fliename = 'Fsp%sUpd.h' % fsp_comp.lower()
        cmd = 'python %s/IntelFsp2Pkg/Tools/GenCfgData.py GENHDR %s/%s.yaml@%s %s/%s' % (
               workspace, pkgname, updname, scope, fvdir, fliename)
        ret = subprocess.call(cmd.split(' '))
        if not (ret == 0):
            fatal('Failed to generate UPD header file !')

        shutil.copyfile('%s/%s' % (fvdir, fliename), '%s/Include/%s'%(pkgname, fliename))

    # rebuild reset vector
    vtf_dir = os.path.join('QemuFspPkg', 'FsprInit', 'Ia32', 'Vtf0')
    x = subprocess.call([sys.executable, 'Build.py', 'ia32'],  cwd=vtf_dir)
    if x: raise Exception ('Failed to build reset vector !')

    print('End of PreBuild...')


def post_build (target, toolchain, fsppkg, fsparch):

    print ('Start of PostBuild ...')

    patchfv = 'IntelFsp2Pkg/Tools/PatchFv.py'
    fvdir   = 'Build/%s/%s_%s/FV' % (fsppkg, target, toolchain)
    fsp_arch   = 4 if fsparch == 'x64' else 0
    build_type = 1 if target == 'RELEASE' else 0
    cmd1 = [
           "0x0000,            _BASE_FSP-T_,                                                                                       @Temporary Base",
           "<[0x0000]>+0x00AC, [<[0x0000]>+0x0020],                                                                                @FSP-T Size",
           "<[0x0000]>+0x00B0, [0x0000],                                                                                           @FSP-T Base",
           "<[0x0000]>+0x00B4, ([<[0x0000]>+0x00B4] & 0xFFFFFFFF) | 0x0001,                                                        @FSP-T Image Attribute",
           "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FF8) | 0x1000 | 0x000%d | 0x0002,                                     @FSP-T Component Attribute" % build_type,
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
         "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FF8) | 0x2000 | 0x000%d | 0x000%d | 0x0002,                            @FSP-M Component Attribute"  % (build_type, fsp_arch),
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
         "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FF8) | 0x3000 | 0x000%d | 0x000%d | 0x0002,                            @FSP-S Component Attribute"  % (build_type, fsp_arch),
         "<[0x0000]>+0x00B8, E3CD9B18-998C-4F76-B65E-98B154E5446F:0x1C - <[0x0000]>,                                             @FSP-S CFG Offset",
         "<[0x0000]>+0x00BC, [E3CD9B18-998C-4F76-B65E-98B154E5446F:0x14] & 0xFFFFFF - 0x001C,                                    @FSP-S CFG Size",
         "<[0x0000]>+0x00D8, FspSecCoreS:_FspSiliconInitApi - [0x0000],                                                          @SiliconInit API",
         "<[0x0000]>+0x00CC, FspSecCoreS:_NotifyPhaseApi - [0x0000],                                                             @NotifyPhase API",
         "0x0000,            0x00000000,                                                                                         @Restore the value",
         "FspSecCoreS:_FspInfoHeaderRelativeOff, FspSecCoreS:_AsmGetFspInfoHeader - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP-S Header Offset"
         ]

    cmd4 = [
         "0x0000,            _BASE_FSP-R_,                                                                                       @Temporary Base",
         "0xFFFFFFFC,        FsprInit:__ModuleEntryPoint,        @Patch FSP-R Entry",
         "<[0x0000]>+0x00AC, [<[0x0000]>+0x0020],                                                                                @FSP-R Size",
         "<[0x0000]>+0x00B0, [0x0000],                                                                                           @FSP-R Base",
         "<[0x0000]>+0x00B4, ([<[0x0000]>+0x00B4] & 0xFFFFFFFF) | 0x0001,                                                        @FSP-R Image Attribute",
         "<[0x0000]>+0x00B6, ([<[0x0000]>+0x00B6] & 0xFFFF0FF8) | 0x4000 | 0x000%d | 0x000%d | 0x0002,                           @FSP-R Component Attribute"  % (build_type, fsp_arch),
         "<[0x0000]>+0x00B8, 0,                                                                                                  @FSP-R CFG Offset",
         "<[0x0000]>+0x00BC, 0,                                                                                                  @FSP-R CFG Size",
         "0x0000,            0x00000000,                                                                                         @Restore the value"
         ]

    for fspt, cmd in [('T', cmd1), ('M', cmd2), ('S',cmd3), ('R',cmd4)]:
        print ('Patch FSP-%s Image ...' % fspt)
        line = ['python', patchfv, fvdir, 'FSP-%s:QEMUFSP' % fspt]
        line.extend(cmd)
        ret = subprocess.call(line)
        if ret:
            fatal('Failed to do PostBuild QEMU FSP!')

    copy_list = [
        ('QEMUFSP.fd',  'QEMU_FSP_%s.fd' % target),
        ('QemuFsp.yaml', 'QEMU_FSP.yaml'),
        ('FspUpd.h',    'FspUpd.h'),
        ('FsptUpd.h',   'FsptUpd.h'),
        ('FspmUpd.h',   'FspmUpd.h'),
        ('FspsUpd.h',   'FspsUpd.h'),
    ]
    copy_file_list (copy_list, fvdir, 'BuildFsp')

    print('End of PostBuild...')


def main():

  toolchain = prep_env ()

  ap = argparse.ArgumentParser()
  sp = ap.add_subparsers(help='command')

  def cmd_build(args):
      # Check if BaseTools has been compiled
      rebuild_basetools ()

      # Build a specified DSC file
      def_list = []
      if args.define is not None:
          for each in args.define:
              def_list.extend (['-D', '%s' % each])

      arch_list = []
      if args.arch == 'x64':
          arch_list = ['-a', 'IA32', '-a', 'X64']
      else:  # IA32
          arch_list = ['-a', 'IA32']

      fsp_pkg = args.platform[0].upper() + args.platform[1:] + 'FspPkg'
      target  = 'RELEASE' if args.release else 'DEBUG'

      pre_build (target, toolchain, fsp_pkg)

      cmd_args = [
          "build" if os.name == 'posix' else "build.bat",
          "--platform", '%s/%s.dsc' % (fsp_pkg, fsp_pkg),
          "-b",         target,
          "--tagname",  os.environ['TOOL_CHAIN'],
          "-n",         str(multiprocessing.cpu_count()),
          "-D", "FSP_ARCH=%s" % args.arch.upper()
          ] + arch_list + def_list
      run_process (cmd_args)

      post_build (target, toolchain, fsp_pkg, args.arch)

  buildp = sp.add_parser('build', help='build FSP binary')
  buildp.add_argument('-p',  '--platform', choices=['qemu'], required = True, help='Specify FSP platform name to build')
  buildp.add_argument('-a',  '--arch', choices=['ia32', 'x64'], default='x64', help='Specify the ARCH for build. Default is to build x64.')
  buildp.add_argument('-r',  '--release',     action='store_true', help='Release build')
  buildp.add_argument('-d',  '--define', action='append', help='Specify macros to be passed into DSC build')
  buildp.set_defaults(func=cmd_build)

  def cmd_clean(args):
    workspace = os.environ['WORKSPACE']
    dirs  = ['Build', 'Conf']
    files = [
      os.path.join (workspace, 'Report.log')
    ]

    for dir in dirs:
      dirpath = os.path.join (workspace, dir)
      print ('Removing %s' % dirpath)
      shutil.rmtree(dirpath, ignore_errors=True)

    for file in files:
      if os.path.exists(file):
        print ('Removing %s' % file)
        os.remove(file)

    print('Clean Done !')

  cleanp = sp.add_parser('clean', help='clean build dir')
  cleanp.set_defaults(func=cmd_clean)

  args = ap.parse_args()
  if len(args.__dict__) < 1:
    # No arguments or subcommands were given.
    ap.print_help()
    ap.exit()

  args.func(args)


if __name__ == '__main__':
  main()

