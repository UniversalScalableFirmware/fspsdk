===============
   FSP SDK
===============

FSP SDK is a fork of EDK2_ project. The purpose of this repo is to create a
basic infrastructure to support create FSP binary.  For demonstration, QEMU virtual
platform is used here.


QEMU FSP SDK Build Steps
------------------------
* Prepare EDK2 build environment following instructions listed `here <http://https://github.com/tianocore/tianocore.github.io/wiki/Getting-Started-with-EDK-II>`_

* Build QEMU FSP binary

.. code-block:: none

    git clone -b qemu_fsp_at_reset https://github.com/universalpayload/fspsdk.git
    cd fspsdk
    git submodule update --init
    # Build 32 bit QEMU FSP
    python buildFsp.py build -p qemu -a ia32

.. _EDK2: https://github.com/tianocore/edk2.git

FSP @ Reset
------------------------
This branch demonstrated a POC for enabling bootable FSP on QEMU platform.

Traditional FSP has FSP-T/M/S components. This POC extends this idea one step further to
add a new FSP-R component to handle the platform boot flow.  The FSP-R component does not
provide any API interface. Instead, it takes control from reset vector and then start
from there. Internally, it will call FSP-T/M/S to complete the required initialization.
And eventually, it jumps into an OEM boot block (OBB) entry point.

In this flow, FSP-R reuses the UPD binary block to provide platform configurations to
assist the platform initialization. These UPDs need to be configured statically so that
the platform can be initialized properly from the reset vector.


