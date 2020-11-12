===============
   FSP SDK
===============

FSP SDK is a fork of EDK2_ project. The purpose of this repo is to create a
basic infrastructure to support create FSP binary.  For demonstration, QEMU virtual
platform is used here.

Standalone SMM Support
----------------------

This qemu_fsp_x64_smm branch is a POC that attempts to demonstrate the launch of a Standalone SMM module from FSP.

The Standalone SMM module is built using EDK2's StandaloneMmPkg with x64 implementation of StandaloneMmCpu.
https://moam.info/a-tour-beyond-bios-launching-standalone-smm-drivers-in-the-pei-_59ba8de81723ddd7c6870a1e.html



QEMU FSP SDK Build Steps
------------------------
* Prepare EDK2 build environment following instructions listed `here <http://https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II>`_

* Build QEMU FSP binary

.. code-block:: bash

  git clone -b qemu_fsp_x64 --single-branch https://github.com/universalpayload/fspsdk.git
  cd fspsdk
  git submodule update --init
  # Build 64 bit QEMU FSP
  python buildFsp.py build qemu -a x64
  
.. _EDK2: https://github.com/tianocore/edk2.git


SBL
---

The FSP with Standalone SMM module was tested with Slim Bootloader available here - https://github.com/mauricema/slimbootloader/tree/qemu_fsp_x64

Once the FSP is built, it needs to be copied to the Silicon\\QemuSocPkg\\FspBin folder as **FspRel.bin**. In addition to FspRel.bin, **Fsp.bsf** needs to be 
copied as well to avoid SBL build process building a fresh FSP.

SBL can be built using the command - python BuildLoader.py build -a x64 qemu
