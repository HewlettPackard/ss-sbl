#
# slingshot base link driver
#
obj-m := $(SBL_NAME).o

ifeq ($(PLATFORM_CASSINI_HW),1)
$(SBL_NAME)-m += sbl_pml_pcs_nic.o \
                 sbl_module_nic.o \
		 sbl_link_nic.o \
		 sbl_media_nic.o \
		 sbl_sbm_serdes_nic.o \
		 sbl_inst_nic.o
else ifeq ($(PLATFORM_ROSETTA_HW),1)
$(SBL_NAME)-m += sbl_module_sw.o \
		 sbl_link_an_sw.o \
		 sbl_sbm_serdes_sw.o
else
$(error Platform not defined! Define one of PLATFORM_ROSETTA_HW PLATFORM_CASSINI_HW)
endif


$(SBL_NAME)-m += sbl_pml_pcs.o \
		 sbl_link.o \
		 sbl_link_an.o \
		 sbl_media.o \
		 sbl_pml.o \
		 sbl_pml_mac.o \
		 sbl_pml_llr.o \
		 sbl_timers.o \
		 sbl_debug.o \
		 sbl_serdes_fn.o \
		 sbl_test.o \
		 sbl_sbm_serdes.o \
		 sbl_counters.o \
		 sbl_fec.o \
		 sbl_serdes.o \
		 sbl_inst.o \
		 sbl_pml_serdes_op.o \
		 sbl_misc.o \
		 sbl_internal.o

ccflags-y :=	-I$(M)
