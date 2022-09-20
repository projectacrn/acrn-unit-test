Welcome to acrn-unit-test
#########################

acrn-unit-test is to provide unit tests for `acrn-hypervisor`_. The unit tests 
are tiny guest operating systems that generally execute only tens of lines of 
C and assembler test code in order to obtain its PASS/FAIL result. Unit tests 
provide ACRN and virtual hardware functional testing by targeting the features 
through minimal implementations of their use per the hardware specification. 
The simplicity of unit tests make them easy to verify they are correct, 
easy to maintain, and easy to use in timing measurements.

acrn-unit-test inherits from kvm-unit-tests and will keep kvm-unit-tests 
Framework and API. For the more information, please refer to: 
http://www.linux-kvm.org/page/KVM-unit-tests.

Building the test
*****************

  .. code-block:: bash
  
     git clone https://github.com/projectacrn/acrn-unit-test.git
     cd acrn-unit-test

x86 directory contains the test suite for ACRN.

To create all test images do:
 
  .. code-block:: bash
     
     ./configure --arch=x86_64
     make

Test images are created in x86/\*.flat

or to create one specified test image do
  
  .. code-block:: bash
     
     ./configure --arch=x86_64
     make x86/hello_acrn.flat

Test image is created in x86/hello_acrn.flat

Running the test
*****************

Creating ACRN User VM launch script
===================================

You can setup ACRN test environment according to the `ACRN Introduction`_ and 
`Getting Started Guide`_.

For acrn-unit-test, it uses COM1 to print all logs and supports only 2 CPU cores. 
So you need free COM1 and set 2 vCPU for User VM while you create ACRN scenario.xml.
  
  .. code-block:: bash
  
     <console_vuart>COM Port 1</console_vuart> => <console_vuart></console_vuart>

     <cpu_affinity>
      <pcpu>
        <pcpu_id>0</pcpu_id>
      </pcpu>
      <pcpu>
        <pcpu_id>1</pcpu_id>
      </pcpu>
     </cpu_affinity>

When you get User VM launch script, such as launch_user_vm_id1.sh, you need 
update it in order to run acrn-unit-test.

To limit vCPU number == 2:
  
  .. code-block:: bash
     
     `add_cpus   0 2`

to enable COM1 to print logs:

  .. code-block:: bash
     
     -s 1:0,lpc -l com1,stdio

to add --debugexit -E $1 as acrn-dm parameter:
  
  .. code-block:: bash 
     
     acrn-dm --debugexit -E $1 ${dm_params[*]}

and to disable OVMF and virtio-blk at last:
  
  .. code-block:: bash

     #    --ovmf /usr/share/acrn/bios/OVMF.fd
     #    `add_virtual_device    4 virtio-blk ./win10-ltsc.img`

Running the test with the updated launch script
===============================================

  .. code-block:: bash
     
     ./launch_user_vm_id1.sh x86/hello_acrn.flat

Adding a test
*************

Create the new unit test's main code file

  .. code-block:: bash
     
     cat > x86/new-unit-test.c
	
     #include <libcflat.h>
     int main(void)
     {
        report(true, "hello!");
        return report_summary();
     }

Ensure the appropriate makefile, e.g. x86/Makefile.common, has been updated 
by adding it to a tests variable 
  .. code-block:: bash	

     tests-common += $(TEST_DIR)/new-unit-test.flat

.. note::
   the tests-common variable identifies tests shared between similar architectures, 
   e.g. i386 and x86_64. Use the tests makefile variable of a specific architecture's 
   makefile to build the test specifically for that architecture.

You can now build and run the test

  .. code-block:: bash
     
     ./configure --arch=x86_64
     make
     ./launch_user_vm_id1.sh x86/new-unit-test.flat 

Contributing
************

Directory structure
===================

  .. code-block:: bash
     
     .:                  configure script, top-level Makefile
     ./scripts:          general architecture neutral helper scripts for building and running tests
     ./lib:              general architecture neutral services for the tests
     ./lib/x86:          architecture dependent services for the tests
     ./x86:              the sources of the tests and the created objects/images

Style
=====

Currently there is a mix of indentation styles so any changes to
existing files should be consistent with the existing style.  For new
files:

  - C: please use standard linux-with-tabs, see Linux kernel
    doc Documentation/process/coding-style.rst
  - Shell: use TABs for indentation

Exceptions:

  - While the kernel standard requires 80 columns, we allow up to 120.

Header guards:

Please try to adhere to the following patterns when adding
"#ifndef <...> #define <...>" header guards:
    
  .. code-block:: bash
  
     ./lib:             _HEADER_H_
     ./lib/<ARCH>:      _ARCH_HEADER_H_
     ./lib/<ARCH>/asm:  _ASMARCH_HEADER_H_
     ./<ARCH>:          ARCH_HEADER_H

Patches
=======

Patches are welcome at the ACRN mailing list <acrn-dev@lists.projectacrn.org>.

Please prefix messages with: [acrn-unit-test PATCH]

You can add the following to .git/config to do this automatically for you:

    [format]
        subjectprefix = acrn-unit-test PATCH

We strive to follow the Linux kernels coding style so it's recommended
to run the kernel's ./scripts/checkpatch.pl on new patches.

You also can find the more information about how to contribute to ACRN in this
`Contribution Guide`_ document:

.. _acrn-hypervisor: https://github.com/projectacrn/acrn-hypervisor
.. _ACRN Introduction: https://projectacrn.github.io/latest/introduction/
.. _Getting Started Guide: https://projectacrn.github.io/latest/getting-started/
.. _Contribution Guide: https://projectacrn.github.io/latest/developer-guides/contribute_guidelines.html
