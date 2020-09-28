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

    git clone -b qemu_fsp_x64 https://github.com/universalpayload/fspsdk.git
    cd fspsdk
    git submodule update --init
    # Build 64 bit QEMU FSP
    python buildFsp.py build qemu -a x64
    # Build 32 bit QEMU FSP
    python buildFsp.py build qemu -a ia32

.. _EDK2: https://github.com/tianocore/edk2.git

FSP YAML Configuration
------------------------
This code base prototyped YAML based FSP UPD configuration options instead of the old DSC
based FSP UPD configuration. As part of it, the standard ConfigEditor tool is also added
to support FSP UPD binary patching.

* To configure FSP UPD using ConfigEditor tool

  - Launch IntelFsp2Pkg\ConfigEditor.py

  - Load FSP yaml configuration file using menu "Open Config YAML file"

  - Load FSP binary file using menu "Load Configuration Data from Binary"

  - Make FSP UPD configuration modifications as required

  - Save the modified UPD changes back into FSP binary using menu "Save Configuration Data to Binary"

|

* To convert FSP UPD format from dsc or bsf to yaml format::

     python IntelFsp2Pkg\Tools\FspDscBsf2Yaml.py  FspBsfOrDscFile  FspYamlFile

* To export FSP UPD configuration into a full DLT format::

     python IntelFsp2Pkg\Tools\GenCfgData.py GENDLT  FspYamlFile;FspBinFile  FspDltFile -DFULL

* To export FSP UPD configuration changes from the default value into DLT format::

     python IntelFsp2Pkg\Tools\GenCfgData.py GENDLT  FspYamlFile;FspBinFile  FspDltFile

* To generate FSP UPD configuration binary::

     # Generate FSP UPD default binary
     python IntelFsp2Pkg\Tools\GenCfgData.py GENBIN YamlFile BinOutFile

     # Patch a binary using DLT file
     python IntelFsp2Pkg\Tools\GenCfgData.py GENBIN YamlFile;DltFile BinOutFile

* To generate FSP UPD configuration C header file::

     # Generate full FSP UPD header
     python IntelFsp2Pkg\Tools\GenCfgData.py GENHDR YamlFile HeaderOutFile

     # Generate FSPM UPD header only
     python IntelFsp2Pkg\Tools\GenCfgData.py GENHDR YamlFile@FSPM_UPD HeaderOutFile

* To generate a single full combined YAML file::

     python IntelFsp2Pkg\Tools\GenCfgData.py GENYML YamlFile  FullYamlrOutFile

