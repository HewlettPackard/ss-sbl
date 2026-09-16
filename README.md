Slingshot Base Link (SBL)
=========================

SBL is the module which manages the port macro for both Rosetta and Cassini.

A design document is located here:
https://connect.us.cray.com/confluence/display/ESERV/Slingshot+Base+Link

Building
========

Note, the SBL module requires two ENV variables to be set for it to build
properly. These are:
SBL_EXTERNAL_BUILD=1
 * SBL is meant to build as a submodule to rosetta_drivers or cxi-driver

Target platform. One of:
PLATFORM_ROSETTA_HW=1
PLATFORM_CASSINI_HW=1
PLATFORM_CASSINI_EMU=1
PLATFORM_CASSINI_SIM=1

Rosetta builds:
The slingshot_base_link repo must be checked out to the following location:
rosetta_drivers/knl/

Rosetta sC image build:
This build will automatically call make with SBL_CONFIG set as follows:
SBL_CONFIG := SBL_EXTERNAL_BUILD=1 PLATFORM_ROSETTA_HW=1
See hms-scimage to build the sC image with the SBL module include.
$ git clone ssh://git@github.hpe.com:hpe/hpc-hms_ec-hms-scimage

Rosetta Standalone Build:
SBL can be checked out and built as follows:
$ SBL_EXTERNAL_BUILD=1 PLATFORM_ROSETTA_HW=1 make


Cassini builds:
The slingshot_base_link repo must be checked out in the same directory as
cxi-driver.

Cassini environment build:
This build will automatically call make with SBL_CONFIG set as follows:
SBL_CONFIG := SBL_EXTERNAL_BUILD=1 PLATFORM_CASSINI_SIM=1
See devbootstrap to build the Cassini environment with the SBL module included:
$ git clone ssh://git@github.hpe.com:hpe/hpc-shs-devbootstrap.git

Cassini Standalone Build:
SBL can be checkout out and built as follows:
$ SBL_EXTERNAL_BUILD=1 PLATFORM_CASSINI_SIM=1 make
